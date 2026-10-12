"""Summarize read-only Ghidra captures; never imports or mutates a program.

Run with --elf PRIVATE_ELF --captures local/assessment --output docs/assessment.
Raw listings/decompilation must remain in ignored storage. CSV contains metadata.
Instruction counts refer to decoded .text instructions in returned CFG block
intervals, not guaranteed original function bodies. Empty CFGs remain unknown.
"""
import argparse
import bisect
import collections
import csv
import hashlib
import json
from pathlib import Path
import re
import struct


def components(nodes, edges):
    adjacency = {n['id']: set() for n in nodes}
    for edge in edges:
        a, b = edge['source_id'], edge['target_id']
        if a in adjacency and b in adjacency:
            adjacency[a].add(b)
            adjacency[b].add(a)
    unseen = set(adjacency)
    count = 0
    while unseen:
        count += 1
        pending = [unseen.pop()]
        while pending:
            for nxt in adjacency[pending.pop()] & unseen:
                unseen.remove(nxt)
                pending.append(nxt)
    return count


def summarize(elf, captures, output):
    raw = elf.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    assert raw[:6] == b'\x7fELF\x01\x01', 'Expected ELF32 little endian'
    header = struct.unpack_from('<16sHHIIIIIHHHHHH', raw)
    shoff, shentsize, shnum, stridx = header[6], header[11], header[12], header[13]
    headers = [struct.unpack_from('<IIIIIIIIII', raw, shoff+i*shentsize) for i in range(shnum)]
    strings_header = headers[stridx]
    strings = raw[strings_header[4]:strings_header[4]+strings_header[5]]
    sections = []
    for h in headers:
        name = strings[h[0]:strings.find(b'\0', h[0])].decode('ascii')
        sections.append(dict(name=name, type=h[1], flags=h[2], address=h[3], offset=h[4], size=h[5]))
    functions = json.loads((captures/'mcp-functions.json').read_text())
    metrics = json.loads((captures/'metrics-full.json').read_text())
    listings = json.loads((captures/'text-listing.json').read_text())
    assert len(functions) == len(metrics) == 7544
    assert len({f['entry_point'] for f in functions}) == len(functions)
    assert {f['entry_point'] for f in functions} == {m['address'] for m in metrics}
    pattern = re.compile(r'^([0-9a-f]{8}) ((?:[0-9a-f]{2} ){4})(\S+)(?: (.*))?$')
    instructions = {}
    for line in listings:
        match = pattern.match(line)
        if match:
            address = int(match[1], 16)
            instructions[address] = (match[3].lstrip('_'), match[4] or '', bytes.fromhex(match[2]))
    addresses = sorted(instructions)
    text = next(s for s in sections if s['name'] == '.text')
    mapped = bytearray(text['size'])
    covered = bytearray(text['size'])
    byte_pattern = re.compile(r'^([0-9a-f]{8}) ((?:[0-9a-f]{2} )+)')
    for line in listings:
        match = byte_pattern.match(line)
        if match:
            offset = int(match[1], 16)-text['address']
            values = bytes.fromhex(match[2])
            assert 0 <= offset < text['size'] and offset+len(values) <= text['size']
            mapped[offset:offset+len(values)] = values
            covered[offset:offset+len(values)] = b'\x01'*len(values)
    assert all(covered), 'Incomplete .text listing capture'
    assert mapped == raw[text['offset']:text['offset']+text['size']], 'Loaded .text differs from disk'
    # Check every returned instruction against disk, not just the import path.
    assert all(raw[text['offset']+a-text['address']:text['offset']+a-text['address']+4] == ins[2]
               for a, ins in instructions.items()), 'Ghidra/disk .text mismatch'
    decomp = {}
    for filename in ['pilot-decomp.json', 'extra-decomp.json', 'pad-decomp.json']:
        for op in json.loads((captures/filename).read_text()).get('operations', []):
            result = op.get('result', {})
            if 'entry_address' in result:
                decomp[result['entry_address']] = result
    config = json.loads((captures/'pilot-config.json').read_text())
    assert len(config) == 20 and len({c['address'] for c in config}) == 20
    configs = {c['address']: c for c in config}
    edges = {(e['source_address'], e['target_address']) for m in metrics for e in m['call']['edges']}
    callees, callers = collections.defaultdict(set), collections.defaultdict(set)
    for a, b in edges:
        callees[a].add(b)
        callers[b].add(a)
    rows, cfg_ins = [], set()
    for f, m in zip(functions, metrics):
        assert f['entry_point'] == m['address']
        graph = m['graph']
        selected = set()
        for node in graph['nodes']:
            lo, hi = int(node['start_address'], 16), int(node['end_address'], 16)
            selected.update(addresses[bisect.bisect_left(addresses, lo):bisect.bisect_right(addresses, hi)])
        cfg_ins.update(selected)
        comp = components(graph['nodes'], graph['edges'])
        cc = len(graph['edges'])-len(graph['nodes'])+2*comp if comp else None
        addr = f['entry_point']
        conf = configs.get(addr, {})
        body = decomp.get(addr, {})
        indirect = sum(instructions[a][0] == 'jalr' for a in selected)
        other_indirect = sum(instructions[a][0] == 'jr' and instructions[a][1] != 'ra' for a in selected)
        cop2 = sum((int.from_bytes(instructions[a][2], 'little') >> 26) in (0x12, 0x36, 0x3e) for a in selected)
        mmi = sum(int.from_bytes(instructions[a][2], 'little') >> 26 == 0x1c for a in selected)
        syscall = sum(instructions[a][0] == 'syscall' for a in selected)
        row = dict(binary_sha256=digest, elf_address='0x'+addr, analysis_name=f['name'],
                   original_identifier='unknown', signature=f['signature'], calling_convention=f['calling_convention'],
                   body_min=f['start_address'], body_max=f['end_address'],
                   cfg_instruction_count=len(selected) if graph['nodes'] else 'unknown',
                   instruction_count_method='decoded .text instructions in returned CFG blocks; body completeness unverified',
                   cfg_blocks=len(graph['nodes']), cfg_edges=len(graph['edges']), cfg_components=comp,
                   cyclomatic_estimate=cc if cc is not None else 'unknown',
                   callers=len(callers[addr]), callees=len(callees[addr]),
                   jalr_sites=indirect, indirect_jump_sites=other_indirect,
                   cop2_related_sites=cop2, mmi_opcode_sites=mmi, syscall_sites=syscall,
                   subsystem=conf.get('subsystem', 'Unknown'), confidence=conf.get('confidence', 'unknown'),
                   evidence=conf.get('evidence', 'E-0002-02/E-0002-03; structural inventory only'),
                   code_complexity=('unknown' if cc is None else 'low' if cc <= 5 else 'medium' if cc <= 20 else 'high' if cc <= 60 else 'very_high'),
                   dependency_complexity=('high' if indirect or len(callees[addr]) > 10 or len(callers[addr]) > 50 else 'medium' if len(callees[addr]) > 2 else 'low'),
                   hardware_coupling=('high' if cop2 or syscall else 'medium' if mmi else 'unknown'),
                   data_structure_uncertainty=conf.get('data_uncertainty', 'unknown (no verified layouts)'), verification_difficulty=conf.get('verification', 'unknown'),
                   decompile_status='output_returned_unverified' if body else 'not_probed',
                   reported_body_bytes=body.get('body_size', 'unknown'),
                   decompiler_warnings=len(re.findall('WARNING', body.get('decompiled_code', ''))),
                   analysis_status='Decompiled' if body else 'Unidentified', verification_status='not_verified',
                   pilot_id=conf.get('id', ''))
        rows.append(row)
    output.mkdir(parents=True, exist_ok=True)
    with (output/'FUNCTION_INVENTORY.csv').open('w', newline='', encoding='utf-8') as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    direct_targets = set()
    for a, (mnemonic, operands, _) in instructions.items():
        if mnemonic == 'jal' and (match := re.search(r'0x([0-9a-f]+)', operands)):
            direct_targets.add(match[1].zfill(8))
    entries = {f['entry_point'] for f in functions}
    summary = dict(sha256=digest, elf_bytes=len(raw), elf_entry=hex(header[4]), elf_machine=header[2],
        sections=sections, function_count=len(rows), unique_call_edges=len(edges),
        default_named=sum(bool(re.match(r'^(thunk_)?FUN_', f['name'])) for f in functions),
        named_other=sum(not re.match(r'^(thunk_)?FUN_', f['name']) for f in functions),
        thunk_name_prefix=sum(f['name'].startswith('thunk_') for f in functions),
        empty_cfg=sum(not m['graph']['nodes'] for m in metrics),
        disconnected_cfg=sum(components(m['graph']['nodes'], m['graph']['edges']) > 1 for m in metrics),
        text_decoded_instructions=len(instructions), text_decoded_bytes=len(instructions)*4,
        loaded_text_sha256=hashlib.sha256(mapped).hexdigest(),
        loaded_text_matches_disk=True,
        text_instruction_byte_fraction=len(instructions)*4/text['size'],
        cfg_covered_unique_instructions=len(cfg_ins),
        jalr_sites=sum(v[0]=='jalr' for v in instructions.values()),
        indirect_jump_sites=sum(v[0]=='jr' and v[1]!='ra' for v in instructions.values()),
        direct_jal_targets_without_function_entry=sorted(direct_targets-entries),
        decomp_probed_unique=len(decomp), decomp_outputs=len(decomp), decomp_failed=0,
        decomp_unprobed=len(rows)-len(decomp),
        warnings_functions=sum(bool(re.search('WARNING', d['decompiled_code'])) for d in decomp.values()),
        complexity_counts=dict(collections.Counter(r['code_complexity'] for r in rows)),
        subsystem_counts=dict(collections.Counter(r['subsystem'] for r in rows)),
        highest_callers=sorted([dict(address=r['elf_address'], name=r['analysis_name'], callers=r['callers']) for r in rows],key=lambda r:r['callers'],reverse=True)[:10],
        largest_cyclomatic=sorted([dict(address=r['elf_address'], cc=r['cyclomatic_estimate']) for r in rows if isinstance(r['cyclomatic_estimate'],int)],key=lambda r:r['cc'],reverse=True)[:10])
    (output/'MEASUREMENTS.json').write_text(json.dumps(summary, indent=2)+'\n')
    (output/'CALL_EDGES.csv').write_text('caller,callee\n'+''.join(f'0x{a},0x{b}\n' for a,b in sorted(edges)))
    print(json.dumps({k:v for k,v in summary.items() if k not in ('sections','direct_jal_targets_without_function_entry')},indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--captures', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    summarize(args.elf, args.captures, args.output)

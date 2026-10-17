"""TASK-0041: conservative PS2 reference-to-retail symbol correspondence.

No Ghidra mutations. --capture reads retail initialized sections through MCP.
Reference symbols and derived matches are public; original bytes stay local/.
Instruction normalization masks only direct jump targets, gp operands and
paired absolute-address operands. It never masks arbitrary constants/branches.
Uniqueness is measured over the complete reference and retail function inventories.
Normalized matches remain candidates until separately corroborated.
"""
import argparse
import csv
import hashlib
import json
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEBUG_SHA = '815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b'
RETAIL_SHA = '7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2'
MAP_SHA = '20baece850349d26f95a5da1c1f6e9eacd6c3f5574e560b8ff2f4e536f9f19d1'
OUT = ROOT / 'reverse/naming'
CACHE = ROOT / 'local/task0041'


class Elf:
    def __init__(self, path):
        self.raw = Path(path).read_bytes()
        if self.raw[:6] != b'\x7fELF\x01\x01':
            raise ValueError('Expected little-endian ELF32')
        self.sha = hashlib.sha256(self.raw).hexdigest()
        h = struct.unpack_from('<16sHHIIIIIHHHHHH', self.raw)
        sections = [struct.unpack_from('<IIIIIIIIII', self.raw, h[6] + i*h[11]) for i in range(h[12])]
        s = sections[h[13]]
        strings = self.raw[s[4]:s[4]+s[5]]
        self.sections = {strings[s[0]:].split(b'\0')[0].decode(): s for s in sections}
        self.symbols = []
        if '.symtab' in self.sections:
            ss = self.sections['.symtab']
            st = sections[ss[6]]
            strings = self.raw[st[4]:st[4]+st[5]]
            for offset in range(ss[4], ss[4]+ss[5], ss[9]):
                n, a, size, info, other, section = struct.unpack_from('<IIIBBH', self.raw, offset)
                self.symbols.append(dict(address=a, size=size, kind=info & 15,
                                         name=strings[n:].split(b'\0')[0].decode(errors='replace'), section=section))

    def read(self, address, size):
        for s in self.sections.values():
            if s[1] != 8 and s[3] <= address and address+size <= s[3]+s[5]:
                off = s[4]+address-s[3]
                return self.raw[off:off+size]
        return b''


def signed(value):
    return value-65536 if value & 32768 else value


def unique_bytes(image, pattern):
    first = image.find(pattern)
    return first >= 0 and image.find(pattern, first+1) < 0


def closed_leaf(raw):
    """Reject indirect calls/tail jumps and all PC-relative exits, including COP branches."""
    if len(raw) % 4:
        return False
    for offset in range(0, len(raw), 4):
        word, = struct.unpack_from('<I', raw, offset)
        op, rs, rt = word >> 26, (word >> 21) & 31, (word >> 16) & 31
        if op in (2, 3):
            return False
        if op == 0 and (word & 63) in (8, 9):
            if (word & 63) != 8 or rs != 31:
                return False
        branch = op in (4, 5, 6, 7, 20, 21, 22, 23) or (op == 1 and rt in (0,1,2,3,16,17,18,19))
        branch = branch or (op in (16,17,18) and rs == 8)
        if branch and not 0 <= offset+4+4*signed(word & 65535) < len(raw):
            return False
    return True


def normalize(raw, entry, gp):
    words = list(struct.unpack('<'+'I'*(len(raw)//4), raw))
    normalized = words[:]
    high = {}
    calls, data = [], []
    for i, word in enumerate(words):
        op, rs, rt, imm = word >> 26, (word >> 21) & 31, (word >> 16) & 31, word & 65535
        if op in (2, 3):
            target = ((entry+i*4+4) & 0xF0000000) | ((word & 0x3FFFFFF) << 2)
            calls.append((i*4, target))
            normalized[i] &= 0xFC000000
        if op == 15:
            high[rt] = (i, imm << 16)
            continue
        if rs == 28 and op in (9, 25, 32, 33, 35, 36, 37, 39, 40, 41, 43, 49, 57, 55, 63):
            normalized[i] &= 0xFFFF0000
            data.append((i*4, gp+signed(imm)))
        elif rs in high and (op in (9, 13, 25, 32, 33, 35, 36, 37, 39, 40, 41, 43, 49, 57, 55, 63)):
            index, value = high[rs]
            address = value + (imm if op == 13 else signed(imm))
            if 0x100000 <= address < 0x340000:
                normalized[index] &= 0xFFFF0000
                normalized[i] &= 0xFFFF0000
                data.append((i*4, address))
        # Kill tracked high halves on register writes, including calls' v0/v1.
        if op == 0:
            high.pop((word >> 11) & 31, None)
        elif op not in (2, 3, 4, 5, 6, 7, 20, 21, 22, 23, 40, 41, 43, 49, 57, 63):
            high.pop(rt, None)
        if op == 3:
            # Calls execute after the delay slot. Conservatively retain at most
            # saved registers; a call-spanning pair is not normalized.
            high = {r: v for r, v in high.items() if 16 <= r <= 23}
    encoded = struct.pack('<'+'I'*len(normalized), *normalized)
    return hashlib.sha256(encoded).hexdigest(), calls, data, sum(a != b for a, b in zip(words, normalized))


def write_csv(path, rows, fields):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('w', encoding='utf-8', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--debug-elf', type=Path, required=True)
    p.add_argument('--map', type=Path, required=True)
    p.add_argument('--capture', action='store_true')
    args = p.parse_args()
    debug = Elf(args.debug_elf)
    retail = Elf(json.loads((ROOT/'config/ps2_elf.local.json').read_text())['elf'])
    if debug.sha != DEBUG_SHA or retail.sha != RETAIL_SHA or hashlib.sha256(args.map.read_bytes()).hexdigest() != MAP_SHA:
        raise ValueError('Input SHA mismatch')
    CACHE.mkdir(parents=True, exist_ok=True)
    # Raw retail data is accepted only after equality with live Ghidra sections.
    ranges = [(n, s[3], s[5]) for n, s in retail.sections.items()
              if n in ('.text', '.rodata', '.data', '.lit4', '.sdata')]
    if args.capture:
        sys.path.insert(0, str(ROOT/'tools'))
        from ghidra import Client
        c = Client()
        manifest = []
        for name, start, size in ranges:
            output = bytearray()
            for address in range(start, start+size, 4096):
                r = c.call('tools/call', {'name': 'memory', 'arguments': dict(file_name='SLUS_203.62',
                    action='read', address=f'{address:08X}', length=min(4096, start+size-address), format='hex')})
                if r.get('isError'):
                    raise RuntimeError(r)
                data = r.get('structuredContent') or json.loads(r['content'][0]['text'])
                output.extend(bytes.fromhex(data['hex_data']))
            if output != retail.read(start, size):
                raise ValueError('Live Ghidra bytes differ from SHA-bound retail ELF')
            (CACHE/f'retail{name}.bin').write_bytes(output)
            manifest.append(dict(section=name, address=f'0x{start:08X}', size=size,
                                 sha256=hashlib.sha256(output).hexdigest()))
        (CACHE/'retail-memory-manifest.json').write_text(json.dumps(dict(retail_sha256=RETAIL_SHA, sections=manifest), indent=2))
    manifest = json.loads((CACHE/'retail-memory-manifest.json').read_text())
    if manifest['retail_sha256'] != RETAIL_SHA:
        raise ValueError('Invalid cache identity')
    for row in manifest['sections']:
        data = (CACHE/f"retail{row['section']}.bin").read_bytes()
        if hashlib.sha256(data).hexdigest() != row['sha256'] or data != retail.read(int(row['address'],16),row['size']):
            raise ValueError('Cache mismatch')
    names = defaultdict(list)
    for line in args.map.read_text(errors='replace').splitlines():
        m = re.match(r'^\s+0x([0-9a-fA-F]+)\s+(.+?)\s*$', line)
        if m:
            names[int(m[1],16)].append(m[2])
    df = []
    for symbol in debug.symbols:
        a, size = symbol['address'], symbol['size']
        if symbol['kind'] != 2 or not 0x100000 <= a < 0x29D84C or not size or size % 4:
            continue
        text = names.get(a, [])
        symbol = dict(symbol, demangled=' | '.join(text), **dict(zip(('signature','calls','data','masked'),normalize(debug.read(a,size),a,0x32C9F0))))
        df.append(symbol)
    rf = []
    with (ROOT/'ps2_decomp/ghidra_functions.csv').open(encoding='utf-8-sig') as handle:
        for row in csv.DictReader(handle):
            a, size = int(row['Start'],16), int(row['Size'])
            raw = retail.read(a,size)
            if not raw or size % 4:
                continue
            rf.append(dict(address=a,size=size,raw=raw,**dict(zip(('signature','calls','data','masked'),normalize(raw,a,0x3358F0)))))
    di, ri = defaultdict(list), defaultdict(list)
    for f in df:
        di[(f['size'],f['signature'])].append(f)
    for f in rf:
        ri[(f['size'],f['signature'])].append(f)
    unique = {}
    for key in di.keys() & ri.keys():
        if len(di[key]) == len(ri[key]) == 1:
            unique[di[key][0]['address']] = ri[key][0]['address']
    rows = []
    for f in df:
        matches = ri.get((f['size'],f['signature']),[])
        for g in matches:
            is_unique = unique.get(f['address']) == g['address']
            if not is_unique:
                continue
            edge_ok = edge_bad = edge_unknown = 0
            for (offset,target),(goffset,gtarget) in zip(f['calls'],g['calls']):
                if f['address'] <= target < f['address']+f['size']:
                    expected = g['address']+target-f['address']
                else:
                    expected = unique.get(target)
                if expected is None:
                    edge_unknown += 1
                elif expected == gtarget:
                    edge_ok += 1
                else:
                    edge_bad += 1
            data_ok = data_unknown = 0
            for (_,a),(_,b) in zip(f['data'],g['data']):
                if unique.get(a) == b or debug.read(a,4) and debug.read(a,4) == retail.read(b,4):
                    data_ok += 1
                else:
                    data_unknown += 1
            exact = debug.read(f['address'],f['size']) == g['raw']
            # Conservative triage, not automatic authority to rename.
            status = 'ambiguous'
            if is_unique:
                status = 'relocation_equivalent_candidate'
                if edge_bad:
                    status = 'conflicting_call_targets'
                elif (exact and f['size'] >= 32 and not f['calls'] and not f['data']
                      and closed_leaf(g['raw'])
                      and unique_bytes(debug.read(0x100000, debug.sections['.text'][5]), g['raw'])
                      and unique_bytes(retail.read(0x100000, retail.sections['.text'][5]), g['raw'])):
                    status = 'supported_exact_leaf'
                elif f['size'] >= 64 and edge_ok and not edge_unknown and not data_unknown:
                    status = 'candidate_instruction_calls_data'
                elif f['size'] >= 64 and not f['calls'] and not data_unknown and data_ok:
                    status = 'candidate_instruction_data_leaf'
            rows.append(dict(retail_address=f"0x{g['address']:08X}",debug_address=f"0x{f['address']:08X}",
                mangled_name=f['name'],reference_name=f['demangled'],size=f['size'],status=status,
                byte_identical=exact,masked_instructions=f['masked'],matched_calls=edge_ok,
                conflicting_calls=edge_bad,unresolved_calls=edge_unknown,matched_data=data_ok,
                unresolved_data=data_unknown,normalized_sha256=f['signature'],evidence_id='E-0041-M01'))
    rows.sort(key=lambda r:(r['retail_address'],r['debug_address']))
    match_fields = ['retail_address','debug_address','mangled_name','reference_name','size','status',
                    'byte_identical','masked_instructions','matched_calls','conflicting_calls',
                    'unresolved_calls','matched_data','unresolved_data','normalized_sha256','evidence_id']
    write_csv(OUT/'task0041-debug-retail-matches.csv',rows,match_fields)
    ambiguous = []
    for key in sorted(di.keys() & ri.keys()):
        if len(di[key]) == len(ri[key]) == 1:
            continue
        ambiguous.append(dict(size=key[0],normalized_sha256=key[1],
            debug_addresses=';'.join(f"0x{x['address']:08X}" for x in di[key]),
            debug_names=' | '.join(x['demangled'] or x['name'] for x in di[key]),
            retail_addresses=';'.join(f"0x{x['address']:08X}" for x in ri[key]),
            debug_count=len(di[key]),retail_count=len(ri[key]),status='ambiguous_do_not_adopt'))
    write_csv(OUT/'task0041-ambiguous-signatures.csv',ambiguous,
              ['size','normalized_sha256','debug_addresses','debug_names','retail_addresses',
               'debug_count','retail_count','status'])
    catalog = [dict(debug_address=f"0x{f['address']:08X}",size=f['size'],mangled_name=f['name'],
                    reference_name=f['demangled'],evidence_id='E-0041-I01') for f in df]
    write_csv(OUT/'task0041-debug-functions.csv',catalog,
              ['debug_address','size','mangled_name','reference_name','evidence_id'])
    from collections import Counter
    summary = dict(debug_sha256=DEBUG_SHA,map_sha256=MAP_SHA,retail_sha256=RETAIL_SHA,
                   debug_functions=len(df),retail_functions=len(rf),unique_matches=len(unique),
                   counts=dict(Counter(r['status'] for r in rows)),
                   ambiguous_signature_groups=len(ambiguous),
                   matched_retail_entries=len({r['retail_address'] for r in rows}),
                   coverage_limit='Contiguous existing Ghidra function ranges only; no missing-function creation or changed-code similarity assignment.')
    (OUT/'task0041-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(summary))


if __name__ == '__main__':
    main()

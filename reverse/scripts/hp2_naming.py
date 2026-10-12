"""Shared TASK-0007 provenance, read-only collection and conservative rename helpers."""
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
PRIVATE = ROOT / 'ps2_decomp/naming'
ELF = Path('D:/ps2_game/SLUS_203.62')
SHA = '7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2'
PROGRAM = 'SLUS_203.62'
sys.path.insert(0, str(ROOT / 'tools'))
from ghidra import Client

def dump(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2), encoding='utf-8')

def check_elf():
    digest = hashlib.sha256(ELF.read_bytes()).hexdigest()
    if digest != SHA:
        raise ValueError('ELF identity mismatch')
    return digest

def rpc(client, name, **args):
    result = client.call('tools/call', {'name': name, 'arguments': dict(file_name=PROGRAM, **args)})
    if result.get('isError'):
        raise RuntimeError(result)
    data = result.get('structuredContent')
    if data is None:
        text = '\n'.join(c['text'] for c in result.get('content', []) if c.get('type') == 'text')
        try:
            data = json.loads(text)
        except ValueError:
            data = {'text': text}
    if isinstance(data, dict) and 'output_id' in data and 'data' not in data:
        chunks, offset = [], 0
        while True:
            part = client.call('tools/call', {'name': 'read_tool_output', 'arguments': {
                'action': 'read', 'session_id': data['session_id'], 'output_id': data['output_id'],
                'view': 'json', 'offset': offset, 'max_chars': 40000}})
            if part.get('isError'):
                raise RuntimeError(part)
            partdata = part['structuredContent']
            chunks.append(partdata['content'])
            following = partdata.get('nextOffset')
            if following is None:
                break
            if following <= offset:
                raise RuntimeError('Stored output cursor did not advance')
            offset = following
        data = json.loads(''.join(chunks))
    return data

def functions(client):
    rows, cursor = [], None
    while True:
        args = dict(action='list', page_size=50)
        if cursor:
            args['cursor'] = cursor
        data = rpc(client, 'functions', **args)
        rows.extend(data['data'])
        cursor = data.get('next_cursor')
        if not cursor:
            break
    if len(rows) != len({r['entry_point'] for r in rows}):
        raise ValueError('Duplicate function address')
    return rows

def bounds(rows):
    return {r['entry_point']: (r['start_address'], r['end_address']) for r in rows}

def verify_state(before, after, changes, aliases=(), reverse=False):
    if bounds(before) != bounds(after):
        raise ValueError('Function boundaries changed')
    expected = {r['entry_point']: r['name'] for r in before}
    if not reverse:
        expected.update({r['address']: r['new_name'] for r in list(changes)+list(aliases)})
    if any(r['name'] != expected[r['entry_point']] for r in after):
        raise ValueError('Unexpected name change')
    old = {r['entry_point']: r for r in before}
    for row in after:
        for field in ('calling_convention', 'namespace'):
            if old[row['entry_point']].get(field) != row.get(field):
                raise ValueError(f'{field} changed')
    return True

def default_name(name):
    return bool(re.fullmatch(r'(?:FUN_|sub_|func_)[0-9a-fA-F]+', name))

def validate_plan(rows, changes, reverse=False):
    by_addr = {r['entry_point'].lower(): r for r in rows}
    addresses, targets = set(), set()
    for change in changes:
        address = change['address'].removeprefix('0x').lower()
        if address in addresses or change['new_name'] in targets:
            raise ValueError('Duplicate address or target name')
        addresses.add(address)
        targets.add(change['new_name'])
        if change['confidence'] != 'high' or not change['evidence_ids']:
            raise ValueError('Insufficient naming evidence')
        if change['category'] not in ('analysis_purpose', 'trusted_original'):
            raise ValueError('Invalid naming category')
        if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', change['new_name']):
            raise ValueError('Invalid identifier')
        if change['category'] == 'analysis_purpose' and not change['new_name'].startswith('HP2_'):
            raise ValueError('Analysis names require HP2_ prefix')
        if not default_name(change['old_name']):
            raise ValueError('Existing custom name protected')
        expected = change['new_name'] if reverse else change['old_name']
        row = by_addr.get(address)
        if not row or row['name'] != expected:
            raise ValueError(f'Stale function name at {address}')
        size = int(row['end_address'], 16) + 1 - int(row['entry_point'], 16)
        if size % 4 or size > 65536 or size <= 0:
            raise ValueError('Untrustworthy function envelope')
        target = change['old_name'] if reverse else change['new_name']
        if any(r['name'] == target and r['entry_point'].lower() != address for r in rows):
            raise ValueError('Target name already exists')
    return True

def inspect_elf():
    check_elf()
    data = ELF.read_bytes()
    if data[:6] != b'\x7fELF\x01\x01':
        raise ValueError('Expected ELF32 little endian')
    hdr = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
    shoff, entsize, count, stridx = hdr[6], hdr[11], hdr[12], hdr[13]
    raw = [struct.unpack_from('<IIIIIIIIII', data, shoff+i*entsize) for i in range(count)]
    strtab = raw[stridx]
    names = data[strtab[4]:strtab[4]+strtab[5]]
    def string(buf, start):
        return buf[start:].split(b'\0', 1)[0].decode('ascii', errors='replace')
    sections = [dict(name=string(names, r[0]), type=r[1], flags=r[2], address=r[3],
        offset=r[4], size=r[5], link=r[6], entsize=r[9]) for r in raw]
    symbols, strings = [], []
    for sec in sections:
        if sec['type'] in (2, 11) and sec['entsize']:
            linked = sections[sec['link']]
            snames = data[linked['offset']:linked['offset']+linked['size']]
            for at in range(sec['offset'], sec['offset']+sec['size'], sec['entsize']):
                name, value, size, info, other, idx = struct.unpack_from('<IIIBBH', data, at)
                symbols.append(dict(name=string(snames, name), address=f'{value:08x}', size=size, type=info & 15))
        if sec['type'] == 1 and sec['name'] in ('.rodata', '.data', '.sdata'):
            buf = data[sec['offset']:sec['offset']+sec['size']]
            for match in re.finditer(rb'[\x20-\x7e\t\r\n]{5,}\x00', buf):
                strings.append(dict(address=f"{sec['address']+match.start():08x}",
                    section=sec['name'], text=match.group()[:-1].decode('ascii')))
    return dict(sha256=SHA, sections=sections, symbols=symbols,
        debug_sections=[s['name'] for s in sections if s['name'].startswith(('.debug', '.stab'))], strings=strings)

def export_map(path, rows):
    with path.open('w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['Name', 'Start', 'End', 'Size'])
        for row in rows:
            start, end = int(row['entry_point'], 16), int(row['end_address'], 16)+1
            if ',' in row['name'] or '"' in row['name']:
                raise ValueError('Upstream CSV parser cannot accept quoted names')
            writer.writerow([row['name'], f'0x{start:08X}', f'0x{end:08X}', end-start])

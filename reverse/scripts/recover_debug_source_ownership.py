"""TASK-0042: derive source ownership from authorized Debug ECOFF records.

Exact marker text is retained; MAP object basenames never become source paths.
No native ABI, inheritance or retail byte/layout equivalence is inferred.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import csv
import json
from pathlib import Path
import re
import struct

from match_debug_reference import Elf, DEBUG_SHA, MAP_SHA, ROOT


def extract(elf_path, map_path):
    elf = Elf(elf_path)
    if elf.sha != DEBUG_SHA:
        raise ValueError('Unauthorized Debug ELF hash')
    import hashlib
    map_bytes = Path(map_path).read_bytes()
    if hashlib.sha256(map_bytes).hexdigest() != MAP_SHA:
        raise ValueError('Unauthorized reference MAP hash')
    hdr = struct.unpack_from('<HH23I', elf.raw, elf.sections['.mdebug'][4])
    if hdr[0] != 0x7009:
        raise ValueError('Unsupported ECOFF magic')
    strings = elf.raw[hdr[16]:hdr[16] + hdr[15]]
    def string(offset):
        if not 0 <= offset < len(strings):
            raise ValueError('Local string offset out of range')
        end = strings.find(b'\0', offset)
        if end < 0:
            raise ValueError('Unterminated local string')
        return strings[offset:end].decode('ascii', errors='replace')
    map_owners = []
    for line_no, line in enumerate(map_bytes.decode('ascii', errors='replace').splitlines(), 1):
        match = re.match(r' \.text\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(.+)', line)
        if match:
            start, size = int(match[1], 16), int(match[2], 16)
            map_owners.append((start, start + size, match[3], line_no))
    functions = []
    for fi in range(hdr[19]):
        foff = hdr[20] + fi * 72
        fdr = struct.unpack_from('<10I2H7I', elf.raw, foff)
        if fdr[4] + fdr[5] > hdr[9]:
            raise ValueError('FDR symbol extent outside local table')
        current_path, current_marker, tu = '', None, ''
        for si in range(fdr[4], fdr[4] + fdr[5]):
            off = hdr[10] + si * 12
            iss, value, bits = struct.unpack_from('<III', elf.raw, off)
            name = string(fdr[2] + iss)
            if bits == 0x8f384045 and re.search(r'\.(?:cpp|cc|c|hpp|h|s)$', name, re.I):
                current_path, current_marker = name, off
                if not tu and re.search(r'\.(?:cpp|cc|c|s)$', name, re.I):
                    tu = name
            if bits & 63 not in (6, 14) or not name:
                continue
            owners = [(obj, line) for start, end, obj, line in map_owners if start <= value < end]
            functions.append(dict(debug_address=f'0x{value:08X}', mangled_name=name,
                definition_path=current_path, translation_unit=tu,
                fdr_index=fi, fdr_file_offset=f'0x{foff:X}',
                symbol_index=si, symbol_file_offset=f'0x{off:X}',
                path_marker_file_offset=f'0x{current_marker:X}' if current_marker is not None else '',
                map_object=owners[0][0] if len(owners) == 1 else '',
                map_line=owners[0][1] if len(owners) == 1 else '',
                evidence_id='E-0042-S001'))
    return functions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', required=True)
    parser.add_argument('--map', required=True)
    args = parser.parse_args()
    functions = extract(args.elf, args.map)
    by_key = defaultdict(list)
    for row in functions:
        by_key[(row['debug_address'], row['mangled_name'])].append(row)
    debug_names = list(csv.DictReader((ROOT / 'reverse/naming/task0041-debug-functions.csv').open()))
    inventory = json.loads((ROOT / 'reverse/classes/template-inventory.json').read_text())
    class_owners = []
    for cls in inventory['classes']:
        prefix = cls['name'] + '::'
        matches = []
        for name in debug_names:
            if not name['reference_name'].startswith(prefix):
                continue
            owned = by_key[(name['debug_address'], name['mangled_name'])]
            if len(owned) == 1 and owned[0]['definition_path']:
                matches.append(dict(owned[0], reference_name=name['reference_name']))
        if matches:
            class_owners.append(dict(class_name=cls['name'], current_source=cls['source'],
                definition_paths=sorted(set(x['definition_path'] for x in matches)),
                translation_units=sorted(set(x['translation_unit'] for x in matches)),
                debug_member_evidence=matches,
                limits='Exact Debug class-member scope. Shared base implementation does not establish exclusive derived ownership or native inheritance.'))
    adopted_path = ROOT / 'reverse/naming/task0042-adopted-retail-names.csv'
    if not adopted_path.exists():
        adopted_path = ROOT / 'reverse/naming/task0041-adopted-retail-names.csv'
    adopted = list(csv.DictReader(adopted_path.open()))
    retail = {}
    for row in adopted:
        matches = by_key[(row['debug_address'], row['mangled_name'])]
        if len(matches) == 1:
            retail[row['retail_address']] = dict(matches[0], retail_address=row['retail_address'],
                reference_name=row['reference_name'], correspondence_evidence=row['evidence_id'])
    scoped = []
    for path in sorted((ROOT / 'src/game_core').glob('*')):
        if path.suffix not in ('.cpp', '.hpp'):
            continue
        text = path.read_text(encoding='utf-8')
        addresses = sorted(set('0x' + x.upper() for x in re.findall(r'(?:Sub_|Type_|(?:ELF|0x)\s*)([0-9A-Fa-f]{8})', text)))
        owned = [retail[a] for a in addresses if a in retail]
        if owned:
            scoped.append(dict(path=path.relative_to(ROOT).as_posix(), mapped_address_mentions=owned,
                unresolved_addresses=[a for a in addresses if a not in retail],
                definition_paths=sorted(set(x['definition_path'] for x in owned if x['definition_path'])),
                translation_units=sorted(set(x['translation_unit'] for x in owned if x['translation_unit'])),
                disposition='Address mentions include calls and comments; independently identify definitions before splitting by proven owner. Preserve unknown projections explicitly port-side.'))
    output = dict(schema_version=1, task='TASK-0042', reference_sha256=DEBUG_SHA,
        map_sha256=MAP_SHA, evidence=dict(id='E-0042-S001',
        observation='Exact ECOFF local file markers immediately preceding procedure symbols; FDR and SYMR offsets retained.',
        limits='Debug source ownership only. Retail assignment requires independently adopted address correspondence. TU ownership does not assert original declaration header. Source-file mention inventories include callees and comments, not just definitions.'),
        debug_function_count=len(functions), adopted_retail_with_ownership=len(retail),
        source_files=scoped, class_owners=class_owners)
    target = ROOT / 'reverse/naming'
    (target / 'task0042-source-ownership.json').write_text(json.dumps(output, indent=2) + '\n', encoding='utf-8')
    columns = list(next(iter(retail.values())).keys())
    with (target / 'task0042-source-ownership.csv').open('w', newline='', encoding='utf-8') as handle:
        writer = csv.DictWriter(handle, columns)
        writer.writeheader()
        writer.writerows(sorted(retail.values(), key=lambda x: x['retail_address']))
    print(json.dumps(dict(debug_functions=len(functions), adopted_retail=len(retail), scoped_files=len(scoped))))


if __name__ == '__main__':
    main()

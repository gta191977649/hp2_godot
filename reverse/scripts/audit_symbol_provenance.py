"""TASK-0040: inventory address identifiers and audit live PS2 symbol provenance.

Raw Ghidra responses stay under ignored local/. Public output contains only
derived symbol metadata and existing evidence links, never original code/bytes.
Class attribution is not a recovered method name or exclusive ownership.
"""
import argparse
import csv
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SHA = '7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2'
PRIVATE = ROOT / 'local/task0040'
OUT = ROOT / 'reverse/naming/task0040-global-symbol-provenance.csv'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--live', action='store_true')
    args = parser.parse_args()
    occurrences = {}
    for path in sorted((ROOT / 'src/game_core').rglob('*')):
        if path.suffix not in ('.cpp', '.hpp', '.h'):
            continue
        for number, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
            for identifier in set(re.findall(r'\b(?:Sub|Type)_[0-9a-fA-F]{8}\b', line)):
                occurrences.setdefault(identifier, []).append(f'{path.relative_to(ROOT).as_posix()}:{number}')
    classes = list(csv.DictReader((ROOT / 'reverse/classes.csv').open(encoding='utf-8-sig')))
    bindings = json.loads((ROOT / 'reverse/classes/name-binding.json').read_text())['bound']
    PRIVATE.mkdir(parents=True, exist_ok=True)
    cache = PRIVATE / 'symbols.json'
    if args.live:
        sys.path.insert(0, str(ROOT / 'tools'))
        from ghidra import Client
        config = json.loads((ROOT / 'config/ps2_elf.local.json').read_text())
        if hashlib.sha256(Path(config['elf']).read_bytes()).hexdigest() != SHA:
            raise ValueError('ELF SHA mismatch')
        client = Client()
        info = client.call('resources/read', {'uri': 'ghidra://program/SLUS_203.62/info'})
        (PRIVATE / 'program-info.json').write_text(json.dumps(info, indent=2))
        responses = {}
        for address in sorted({x[-8:].upper() for x in occurrences}):
            responses[address] = client.call('tools/call', {'name': 'symbols', 'arguments': {
                'file_name': 'SLUS_203.62', 'action': 'get', 'address': address}})
        cache.write_text(json.dumps({'sha256': SHA, 'responses': responses}, indent=2))
        # Bounded deeper audit of root address-grouped driving entry files.
        entries = sorted({p.stem[4:12] for p in (ROOT / 'src/game_core').glob('Sub_*.hpp')})
        for address in entries:
            for action in ('decompile', 'references_to'):
                response = client.call('tools/call', {'name': 'inspect', 'arguments': {
                    'file_name': 'SLUS_203.62', 'action': action, 'address': address, 'page_size': 500}})
                (PRIVATE / f'{address}-{action}.json').write_text(json.dumps(response, indent=2))
        for address in ('002AEA70', '002AEA88', '002C7090', '002C7098'):
            response = client.call('tools/call', {'name': 'inspect', 'arguments': {
                'file_name': 'SLUS_203.62', 'action': 'references_to', 'address': address, 'page_size': 500}})
            (PRIVATE / f'{address}-references_to.json').write_text(json.dumps(response, indent=2))
        response = client.call('tools/call', {'name': 'memory', 'arguments': {
            'file_name': 'SLUS_203.62', 'action': 'read', 'address': '002C7090', 'length': 558, 'format': 'hex'}})
        (PRIVATE / 'rtti-region.json').write_text(json.dumps(response, indent=2))
    saved = json.loads(cache.read_text())
    if saved['sha256'] != SHA:
        raise ValueError('Cache SHA mismatch')
    rows = []
    for identifier, sites in sorted(occurrences.items()):
        address = '0x' + identifier[-8:].upper()
        raw = saved['responses'][address[2:]]
        symbol = raw.get('structuredContent', {})
        if not symbol:
            try:
                symbol = json.loads(raw['content'][0]['text'])
            except (ValueError, KeyError, IndexError):
                symbol = {}
        members = [c['class_name'] for c in classes if address in c['functions'].split()]
        constructors = [c['class_name'] for c in classes if address in c['constructors'].split()]
        labels = [label for label, hits in bindings.items() if any(h['function'] == address for h in hits)]
        rows.append(dict(identifier=identifier, address=address, ghidra_name=symbol.get('name', ''),
                         ghidra_source=symbol.get('source', ''), ghidra_namespace=symbol.get('namespace', ''),
                         query_error=str(raw.get('isError', False)).lower(),
                         attributed_classes=';'.join(members), constructor_classes=';'.join(constructors),
                         referenced_qualified_labels=';'.join(labels),
                         exact_method_name='unresolved',
                         evidence_id='E-0040-N01;E-0027-C01/C04',
                         source_occurrences=';'.join(sites)))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open('w', encoding='utf-8', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(json.dumps({'identifiers': len(rows), 'addresses': len(saved['responses']),
                      'attributed': sum(bool(r['attributed_classes']) for r in rows),
                      'constructor_bridges': sum(bool(r['constructor_classes']) for r in rows),
                      'qualified_label_hits': sum(bool(r['referenced_qualified_labels']) for r in rows),
                      'query_errors': sum(r['query_error'] == 'true' for r in rows),
                      'sources': sorted({r['ghidra_source'] for r in rows}), 'output': str(OUT)}))


if __name__ == '__main__':
    main()

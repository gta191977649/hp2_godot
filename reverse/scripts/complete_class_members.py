"""TASK-0043: derived complete member registry and Debug-only per-class catalogue.

Uses only previously SHA-bound adopted correspondences and exact ECOFF ownership.
Void/no-argument entries are host gap interfaces, never reconstructed PS2 ABI.
"""
from __future__ import annotations
import argparse
import collections
import csv
import io
import json
import re
from pathlib import Path
import generate_class_templates as g

ROOT = g.ROOT

def split_member(reference):
    head = reference.split('(', 1)[0]
    if '::' not in head:
        return None
    return tuple(head.rsplit('::', 1))

def source_path(original):
    text = original.replace('\\', '/')
    while text.startswith('../'):
        text = text[3:]
    if not text or text.startswith('/') or ':' in text or '..' in text.split('/'):
        return None
    return 'src/game_core/' + text

def csv_text(rows, fields):
    stream = io.StringIO(newline='')
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator='\n')
    writer.writeheader(); writer.writerows(rows)
    return stream.getvalue()

def derive():
    base, _, symbols, _ = g.model(include_complete=False)
    classes = {c['name']: c for c in base}
    adopted = g.read_csv('reverse/naming/task0042-adopted-retail-names.csv')
    ownership = {r['retail_address']: r for r in g.read_csv('reverse/naming/task0042-source-ownership.csv')}
    members = []
    ledger_path = ROOT / 'reverse/naming/task0043-identifier-renames.json'
    renames = ({r['old_identifier']: r['new_identifier'] for r in json.loads(ledger_path.read_text())['renames'] if r['entity_kind'] == 'function'} if ledger_path.exists() else {})
    counts = collections.Counter(split_member(r['reference_name']) for r in adopted if split_member(r['reference_name']))
    existing_defs = g.definitions()
    placements = {}
    for row in adopted:
        parts = split_member(row['reference_name'])
        if not parts:
            continue
        owner, method = parts
        address = row['retail_address']
        old = classes.get(owner)
        owned = ownership.get(address, {})
        member = dict(elf_address=address, debug_address=row['debug_address'], owner=owner,
            reference_name=row['reference_name'], mangled_name=row['mangled_name'],
            evidence_id=row['evidence_id'] + '; E-0042-S001; E-0043-M01',
            record=symbols.get(address, {}).get('record_path') or
                f'reverse/functions/{g.BINARY}/unknown/{address[2:].lower()}.md',
            original_definition_path=owned.get('definition_path', ''),
            original_translation_unit=owned.get('translation_unit', ''))
        if not g.VALID_NAME.fullmatch(owner):
            member.update(exclusion_reason='Template or nested declaring scope cannot be represented by a standalone class without inventing template ABI', method='', source='', name_limit='exact original reference retained')
            members.append(member); continue
        source = source_path(owned.get('definition_path', ''))
        if not source or Path(source).suffix.lower() not in ('.cpp', '.cc', '.c'):
            source = source_path(owned.get('translation_unit', ''))
        if not source or Path(source).suffix.lower() not in ('.cpp', '.cc', '.c'):
            member.update(exclusion_reason='No exact source translation-unit marker available', method='', source='', name_limit='exact original reference retained')
            members.append(member); continue
        if old:
            header = old['header']
        else:
            header = (source_path(owned['definition_path']) if Path(owned.get('definition_path', '')).suffix.lower() in ('.h', '.hpp') else str(Path(source).with_suffix('.hpp')).replace('\\', '/'))
            placements.setdefault(owner, dict(source=source, header=header, namespace='hp2::recovered',
                placement_provenance='E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim'))
        current = old and old['methods'].get(address)
        if current:
            chosen = (method if old['namespace'] == 'hp2::recovered' and g.VALID_NAME.fullmatch(method) and method != owner and counts[parts] == 1 else current['name'])
            source = current.get('source', old['source'])
            limit = 'existing whole-entry identifier retained; signature remains separately classified'
        elif method == owner or method == '~' + owner:
            chosen = 'Sub_' + address[2:]
            limit = 'constructor/destructor entry is not a native C++ lifetime operation'
        elif not g.VALID_NAME.fullmatch(method):
            chosen = 'Sub_' + address[2:]
            limit = 'operator or non-identifier spelling requires unconfirmed ABI/signature'
        elif counts[parts] > 1:
            chosen = 'Sub_' + address[2:]
            limit = 'multiple original overloads cannot share a void/no-argument placeholder'
        else:
            chosen = method
            limit = 'exact original method spelling; void/no-argument host gap interface only'
        # Existing whole-entry owns its prior signature. A typed projection alone is
        # deliberately insufficient to claim complete entry coverage.
        # Projection carriers are host scopes, never names for a whole-entry class method.
        # Constructor/operator/overload placeholders must retain their address.
        if current and old['namespace'] != 'hp2::recovered' and counts[parts] == 1 and g.VALID_NAME.fullmatch(method) and method != owner:
            chosen = renames.get(chosen, chosen).rsplit('::', 1)[-1]
        member.update(method=chosen, source=source, header=header, name_limit=limit, exclusion_reason='', template_action='preserved_whole_entry' if current else 'added_whole_entry')
        members.append(member)
    mapped = {(r['debug_address'], r['mangled_name']) for r in adopted}
    debug = []
    for row in g.read_csv('reverse/naming/task0041-debug-functions.csv'):
        parts = split_member(row['reference_name'])
        if parts and (row['debug_address'], row['mangled_name']) not in mapped:
            debug.append(dict(owner=parts[0], debug_address=row['debug_address'],
                reference_name=row['reference_name'], mangled_name=row['mangled_name'],
                status='unmapped_debug_method_no_retail_entry', evidence_id=row['evidence_id']))
    registry = dict(schema_version=1, binary_id=g.BINARY,
        reference_sha256='815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b',
        evidence_id='E-0043-M01', classes=placements, members=members)
    return registry, debug

def coverage(registry):
    definitions = g.definitions()
    classes = {c['name']: c for c in g.model()[0]}
    debug_owners = g.debug_canonical_owners()
    rows = []
    for member in registry['members']:
        if member['exclusion_reason']:
            state, signature = 'explicit_unrepresentable_entry', ''
        elif member['owner'] in debug_owners:
            # Keep the Retail attribution as historical evidence. Its old noarg
            # shell is deliberately absent from the canonical Debug owner, and
            # the Debug body is not a Retail implementation/signature claim.
            state = 'historical_retail_entry_superseded_by_debug_owner'
            signature = 'historical Retail attribution; no current Debug signature inference'
        else:
            hits = g.entry_definitions(classes[member['owner']], member['elf_address'], member['method'], definitions.get(member['elf_address'], []))
            if len(hits) != 1:
                raise ValueError('Expected one whole-entry definition: ' + str(member))
            state = ('added_template' if member['template_action'] == 'added_whole_entry' else 'existing_throwing_template') if hits[0]['stub'] else 'existing_implementation'
            signature = hits[0]['signature']
        rows.append(dict(elf_address=member['elf_address'], debug_address=member['debug_address'],
            owner=member['owner'], reference_name=member['reference_name'], method=member['method'],
            status=state, source=member['source'], signature=signature,
            reason=('Explicit Debug owner supersession; see ' + debug_owners[member['owner']].get('registry_source', 'reverse/classes/task0044-debug-canonical-owners.json') + '. ' + member['name_limit']
                    if state == 'historical_retail_entry_superseded_by_debug_owner'
                    else member['exclusion_reason'] or member['name_limit']), evidence_id=member['evidence_id']))
    return rows

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true')
    parser.add_argument('--registry-only', action='store_true')
    args = parser.parse_args()
    registry, debug = derive()
    outputs = {'reverse/classes/task0043-member-registry.json': json.dumps(registry, indent=2) + '\n',
        'reverse/naming/task0043-unmapped-debug-methods.csv': csv_text(debug, ['owner','debug_address','reference_name','mangled_name','status','evidence_id'])}
    if not args.registry_only:
        rows = coverage(registry)
        outputs['reverse/naming/task0043-member-coverage.csv'] = csv_text(rows, ['elf_address','debug_address','owner','reference_name','method','status','source','signature','reason','evidence_id'])
    for path, text in outputs.items():
        if args.write:
            g.write_text(path, text)
        elif not (ROOT / path).exists() or (ROOT / path).read_text() != text:
            raise ValueError('Stale generated member catalogue: ' + path)
    print(f'COMPLETE_MEMBERS_OK: {len(registry["members"])} mapped class methods; {len(debug)} unmapped Debug methods')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())


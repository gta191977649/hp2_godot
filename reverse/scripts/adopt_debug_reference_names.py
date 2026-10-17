"""TASK-0041: adopt only audited names, preserving addresses and behavior status.

Normalised instruction candidates are deliberately excluded. Supported names
come from raw exact leaves or explicitly reviewed address bridges. Debug ABI,
layouts and behavior are not transplanted into host projections.
"""
import argparse
import csv
import io
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SHA = '7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2'
DEBUG = '815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b'
NAMING = ROOT / 'reverse/naming'


def read_csv(path):
    with path.open(encoding='utf-8-sig', newline='') as handle:
        return list(csv.DictReader(handle))


def csv_text(rows, fields):
    stream = io.StringIO(newline='')
    w = csv.DictWriter(stream, fieldnames=fields, lineterminator='\n')
    w.writeheader()
    w.writerows(rows)
    return stream.getvalue()


def save(path, text):
    # Avoid reopening unchanged files repeatedly (notably shared class headers).
    if not path.exists() or path.read_text(encoding='utf-8') != text:
        path.write_text(text,encoding='utf-8')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--write', action='store_true')
    args = p.parse_args()
    mappings = {}
    for r in read_csv(NAMING/'task0041-debug-retail-matches.csv'):
        if r['status'] == 'supported_exact_leaf':
            mappings[r['retail_address']] = dict(retail_address=r['retail_address'],
                debug_address=r['debug_address'],reference_name=r['reference_name'] or r['mangled_name'],
                mangled_name=r['mangled_name'],basis=r['status'],evidence_id='E-0041-M01')
    slot_path = NAMING/'task0041-debug-vtable-slots.csv'
    if slot_path.exists():
        for r in read_csv(slot_path):
            if (r['status'] != 'supported_vtable_slot'
                or r['retail_target_distinct_reference_name_sets'] != '1'
                or not r['reference_names'] or ' | ' in r['reference_names']):
                continue
            address = r['retail_address']
            if address in mappings and mappings[address]['reference_name'] != r['reference_names']:
                raise ValueError('Conflicting independently supported spellings: '+address)
            mappings[address] = dict(retail_address=address,debug_address=r['debug_address'],
                reference_name=r['reference_names'],mangled_name=r['mangled_names'],
                basis='unique_complete_vtable_slot_correspondence',evidence_id='E-0041-V01')
    reviewed_path = NAMING/'task0041-reviewed-bridges.json'
    bridges=[]
    if reviewed_path.exists():
        payload = json.loads(reviewed_path.read_text())
        bridges = payload if isinstance(payload,list) else payload['bridges']
        for r in bridges:
            mappings[r['retail_address']] = dict(retail_address=r['retail_address'],debug_address=r['debug_address'],
                reference_name=r['reference_name'],mangled_name=r.get('mangled_name',''),
                basis='independently_reviewed_bridge',evidence_id=r['evidence_id'])
    rows = sorted(mappings.values(),key=lambda r:r['retail_address'])
    projection_plan=[]
    projection_names={'00137C88':'YawFrictionBoost','00137D90':'Move','00138E08':'ResolveForces',
                      '0018BCE0':'SetDesiredGear','0019D4D0':'GetDerivative','001A4E88':'ModifyGroundForce'}
    for address,name in projection_names.items():
        r=mappings.get('0x'+address)
        if r:
            projection_plan.append(dict(old_identifier='Sub_'+address,source_identifier=name,
                elf_address='0x'+address,reference_name=r['reference_name'],evidence_id=r['evidence_id']))
    type_plan=[]
    type_addresses={'001F8C10','001F90F0','00249DE0','00249C78','0024A690',
                    '002495F0','00248568','002490D8','002491B0','001F1A78'}
    for r in bridges:
        if r['retail_address'][2:] in type_addresses:
            type_plan.append(dict(old_identifier='Type_'+r['retail_address'][2:],
                new_identifier=r['class_identifier'],elf_address=r['retail_address'],
                debug_address=r['debug_address'],evidence_id=r['evidence_id']))
    plan = []
    inventory = json.loads((ROOT/'reverse/classes/template-inventory.json').read_text())
    aliases_path=NAMING/'task0041-class-identifier-overrides.json'
    aliases=json.loads(aliases_path.read_text()) if aliases_path.exists() else {}
    for cls in inventory['classes']:
        cls['name']=aliases.get(cls['name'],{}).get('name',cls['name'])
        candidates = []
        for member in cls['members']:
            r = mappings.get(member['elf_address'])
            if not r:
                continue
            m = re.fullmatch(re.escape(cls['name'])+r'::([A-Za-z_]\w*)\(.*',r['reference_name'])
            if not m or m[1] == cls['name']:
                continue
            candidates.append((member,m[1],r))
        counts = Counter(x[1] for x in candidates)
        for member,name,r in candidates:
            if counts[name] > 1 or any(x['method']==name and x['elf_address']!=member['elf_address'] for x in cls['members']):
                continue
            plan.append(dict(class_name=cls['name'],elf_address=member['elf_address'],method=name,
                             source_identifier=cls['name']+'::'+name,header=cls['header'],source=cls['source'],
                             debug_address=r['debug_address'],reference_name=r['reference_name'],evidence_id=r['evidence_id']))
    index = read_csv(ROOT/'reverse/symbols.csv')
    fields = list(index[0])
    present = {r['elf_address'] for r in index}
    added = changed = 0
    for r in rows:
        address = r['retail_address']
        record = f'reverse/functions/sha256-{SHA}/reference/{address[2:].lower()}.md'
        existing = [x for x in index if x['elf_address']==address]
        if not existing:
            row = dict.fromkeys(fields,'unknown')
            row.update(binary_id='sha256-'+SHA,elf_address=address,original_identifier=r['reference_name'],
                       recovered_name=r['reference_name'],signature='retail ABI unresolved; debug reference: '+r['reference_name'],
                       calling_convention='unknown',subsystem='reference',confidence='high for static name correspondence only',
                       status='Identified',record_path=record,pc_implementation='none',evidence_ids=r['evidence_id'])
            index.append(row)
            existing=[row]
            added += 1
        for row in existing:
            row['original_identifier'] = r['reference_name']
            row['recovered_name'] = r['reference_name']
            if r['evidence_id'] not in row['evidence_ids'].split(';'):
                row['evidence_ids'] += ';'+r['evidence_id']
            changed += 1
            if args.write and row['record_path'] != record:
                original_record = ROOT/row['record_path']
                if not original_record.is_file():
                    raise ValueError('Missing original record: '+str(original_record))
                text=original_record.read_text(encoding='utf-8')
                marker='\n\n## TASK-0041 original-name correspondence\n'
                text=text.split(marker,1)[0]
                text += (marker+'\n'+f'{r["evidence_id"]}: `{r["reference_name"]}`; debug entry '
                         f'{r["debug_address"]}, retail entry {address}. This supersedes earlier\n'
                         'unknown-name statements only; retail ABI, fields, behavior and status are unchanged.\n'
                         f'Naming evidence: `{record}`.\n')
                save(original_record,text)
        # Keep independent reference name evidence separate from existing behavior records.
        body = (f'# {address} - {r["reference_name"]}\n\n'
                f'- Retail ELF: SLUS_203.62; executable revision unknown; SHA-256 {SHA}.\n'
                f'- Debug reference ELF SHA-256: {DEBUG}; address {r["debug_address"]}.\n'
                f'- Evidence: {r["evidence_id"]}; TASK-0041.\n'
                '- Status: Identified (name correspondence only; no behavior promotion).\n\n'
                '## Confirmed naming observation\n\n'
                f'Original reference spelling: `{r["reference_name"]}`. Matching basis: {r["basis"]}.\n'
                'See reverse/naming/task0041-adopted-retail-names.csv and the TASK-0041 report\n'
                'for exact matching boundaries and counterevidence. Raw bytes remain ignored.\n\n'
                '## Hypotheses\n\nNone adopted about retail field layout or complete behavior.\n\n'
                '## Unresolved questions\n\nRetail ABI, layout and behavioral equivalence remain unverified. Debug parameter\n'
                'types describe the reference build and do not replace native projection signatures.\n')
        if args.write:
            path = ROOT/record
            path.parent.mkdir(parents=True,exist_ok=True)
            save(path,body)
    if args.write:
        (NAMING/'task0041-adopted-retail-names.csv').write_text(csv_text(rows,
            ['retail_address','debug_address','reference_name','mangled_name','basis','evidence_id']),encoding='utf-8')
        (NAMING/'task0041-adopted-source-names.csv').write_text(csv_text(plan,
            ['class_name','elf_address','method','source_identifier','header','source','debug_address','reference_name','evidence_id']),encoding='utf-8')
        (NAMING/'task0041-adopted-type-names.csv').write_text(csv_text(type_plan,
            ['old_identifier','new_identifier','elf_address','debug_address','evidence_id']),encoding='utf-8')
        (NAMING/'task0041-adopted-projection-names.csv').write_text(csv_text(projection_plan,
            ['old_identifier','source_identifier','elf_address','reference_name','evidence_id']),encoding='utf-8')
        for path in (ROOT/'src').rglob('*'):
            if path.suffix not in ('.cpp','.hpp','.h'):
                continue
            original=path.read_text(encoding='utf-8');text=original
            for item in type_plan:
                text=re.sub(r'\b'+item['old_identifier']+r'\b',item['new_identifier'],text)
                if path.name in ('fmenus.hpp','eventtree.hpp','options.hpp'):
                    evidence=(f'// TASK-0041 {item["evidence_id"]}; original class spelling; retail {item["elf_address"]}, '
                              f'debug {item["debug_address"]}. Native interface/layout remain port projections.\n')
                    if evidence not in text:
                        pattern=r'(?m)^(\s*)(class|struct) '+re.escape(item['new_identifier'])+r'\b'
                        text=re.sub(pattern,lambda m:m[1]+evidence+m[1]+m[2]+' '+item['new_identifier'],text,count=1)
            for item in projection_plan:
                text=re.sub(r'\b'+item['old_identifier']+r'\b(?!\.(?:hpp|cpp|h)\b)',item['source_identifier'],text)
                if path.stem.startswith(item['old_identifier']):
                    evidence=(f'// TASK-0041 {item["evidence_id"]}; retail {item["elf_address"]}: {item["reference_name"]}.\n'
                              '// Functions here use host projection signatures; original member scope/ABI is recorded separately.\n')
                    if evidence not in text:
                        text=evidence+text
            if text != original:
                save(path,text)
        (ROOT/'reverse/symbols.csv').write_text(csv_text(index,fields),encoding='utf-8')
        for old,alias in aliases.items():
            for cls in inventory['classes']:
                if cls['name'] != alias['name']:
                    continue
                for filename in (cls['header'],cls['source']):
                    path=ROOT/filename;text=path.read_text(encoding='utf-8')
                    from generate_class_templates import mask_comments_strings
                    masked=mask_comments_strings(text)
                    for match in reversed(list(re.finditer(r'\b'+re.escape(old)+r'\b',masked))):
                        text=text[:match.start()]+alias['name']+text[match.end():]
                    text=re.sub(r'(?m)^// TASK-0041 E-0041-V01: exact class spelling from named debug vtable; allocator label [^\n]*\n','',text)
                    evidence='// TASK-0041 '+alias['evidence_id']+': exact class spelling from named debug vtable; allocator label '+old+'.\n'
                    if evidence not in text:
                        text=evidence+text
                    save(path,text)
        # Only rename class members with a matching original class scope. Shared
        # inherited templates keep their existing class attribution and address name.
        import sys
        sys.path.insert(0,str(ROOT/'reverse/scripts'))
        from generate_class_templates import class_range
        for item in plan:
            old = 'Sub_'+item['elf_address'][2:]
            path=ROOT/item['source'];text=path.read_text(encoding='utf-8')
            text=text.replace(item['class_name']+'::'+old,item['source_identifier'])
            evidence=f'// TASK-0041 {item["evidence_id"]}; debug {item["debug_address"]}: {item["reference_name"]}; retail {item["elf_address"]}.\n'
            if evidence not in text:
                text=text.replace('void '+item['source_identifier']+'(',evidence+'void '+item['source_identifier']+'(',1)
            save(path,text)
            path=ROOT/item['header'];text=path.read_text(encoding='utf-8');span=class_range(text,item['class_name'])
            if not span:
                raise ValueError('Missing class declaration')
            a,b=span;chunk=text[a:b+1]
            chunk=re.sub(r'\b'+old+r'\b',item['method'],chunk)
            comment='    // Debug reference: '+item['reference_name']+'; '+item['evidence_id']+'; retail ABI unconfirmed.\n'
            if comment not in chunk:
                chunk=chunk.replace('    void '+item['method']+'();',comment+'    void '+item['method']+'();',1)
            save(path,text[:a]+chunk+text[b+1:])
        # Preserve the original TASK-0040 source audit as the denominator, so a
        # successful rename does not disappear from the coverage report.
        from generate_class_templates import mask_comments_strings
        remaining=set()
        for path in (ROOT/'src/game_core').rglob('*'):
            if path.suffix in ('.cpp','.hpp','.h'):
                remaining.update(re.findall(r'\b(?:Sub|Type)_[0-9A-Fa-f]{8}\b',
                    mask_comments_strings(path.read_text(encoding='utf-8'))))
        candidates={r['retail_address']:r for r in read_csv(NAMING/'task0041-debug-retail-matches.csv')}
        coverage=[]
        for original in read_csv(NAMING/'task0040-global-symbol-provenance.csv'):
            address=original['address'];adopted=mappings.get(address);candidate=candidates.get(address)
            coverage.append(dict(original_identifier=original['identifier'],retail_address=address,
                adopted_original_name=adopted['reference_name'] if adopted else '',
                candidate_reference_name=candidate['reference_name'] if candidate and not adopted else '',
                status='adopted_static_name' if adopted else 'candidate_only' if candidate else 'unresolved_or_ambiguous',
                address_token_remains_in_code=original['identifier'] in remaining,
                evidence_id=adopted['evidence_id'] if adopted else 'E-0041-M01'))
        save(NAMING/'task0041-source-coverage.csv',csv_text(coverage,list(coverage[0])))
    print(json.dumps(dict(adopted_names=len(rows),new_symbol_records=added,index_rows_touched=changed,
                          source_member_renames=len(plan),source_type_renames=len(type_plan),write=args.write)))


if __name__ == '__main__':
    main()

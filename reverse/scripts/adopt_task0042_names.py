"""Merge audited TASK-0042 names without changing behavior or historical ledgers.

Only supported C/U evidence is adopted. Existing record sections are preserved;
the bounded TASK-0042 section is replaced in place on subsequent runs.
"""
import argparse
import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

from adopt_debug_reference_names import ROOT, NAMING, SHA, DEBUG, read_csv, csv_text, save


def merge_rows(baseline, payloads):
    mappings={x['retail_address']:dict(x) for x in baseline}
    additions=[]
    evidence=set()
    for payload in payloads:
        if payload['retail_sha256']!=SHA or payload['debug_sha256']!=DEBUG:
            raise ValueError('Evidence binary identity mismatch')
        for row in payload['bridges']:
            if not row['status'].startswith('supported'):continue
            if row['evidence_id'] in evidence:raise ValueError('Duplicate evidence ID')
            evidence.add(row['evidence_id'])
            if not row.get('live_symbol_matches_elf',row.get('live_debug_symbol_confirmed',False)):
                raise ValueError('Missing live original symbol confirmation: '+row['evidence_id'])
            item=dict(retail_address=row['retail_address'],debug_address=row['debug_address'],
                reference_name=row['reference_name'],mangled_name=row['mangled_name'],
                basis=row['status'],evidence_id=row['evidence_id'])
            old=mappings.get(item['retail_address'])
            if old and any(old[k]!=item[k] for k in ('debug_address','reference_name','mangled_name')):
                raise ValueError('Conflicting correspondence: '+item['retail_address'])
            if not old:
                mappings[item['retail_address']]=item
                additions.append(item)
    return sorted(mappings.values(),key=lambda x:x['retail_address']),additions


def upsert_section(text, title, body):
    marker='## '+title+'\n'
    section=marker+'\n'+body.rstrip()+'\n'
    found=re.search(r'(?m)^'+re.escape(marker),text)
    if not found:return text.rstrip()+'\n\n'+section
    following=re.search(r'(?m)^## ',text[found.end():])
    end=found.end()+following.start() if following else len(text)
    return (text[:found.start()]+section+'\n'+text[end:]).rstrip()+'\n'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write',action='store_true')
    args=parser.parse_args()
    baseline=read_csv(NAMING/'task0041-adopted-retail-names.csv')
    payloads=[json.loads((NAMING/f'task0042-{part}-bridges.json').read_text()) for part in ('candidate','unresolved')]
    adopted,additions=merge_rows(baseline,payloads)
    mappings={x['retail_address']:x for x in adopted}
    partitions=json.loads((NAMING/'task0042-projection-partitions.json').read_text())['functions']
    projections=defaultdict(list)
    for part in partitions:
        if part['elf_address']:projections[part['elf_address']].append(part)
    index=read_csv(ROOT/'reverse/symbols.csv');fields=list(index[0]);new_records=0
    statuses={ (x['elf_address'],x['record_path']):x['status'] for x in index }
    for row in adopted:
        address=row['retail_address']
        existing=[x for x in index if x['binary_id']=='sha256-'+SHA and x['elf_address']==address]
        record=f'reverse/functions/sha256-{SHA}/reference/{address[2:].lower()}.md'
        if not existing:
            item=dict.fromkeys(fields,'unknown')
            item.update(binary_id='sha256-'+SHA,elf_address=address,original_identifier=row['reference_name'],
                recovered_name=row['reference_name'],signature='retail ABI unresolved; debug reference: '+row['reference_name'],
                calling_convention='unknown',subsystem='reference',confidence='high for static name correspondence only',
                status='Identified',record_path=record,pc_implementation='none',evidence_ids=row['evidence_id'])
            index.append(item);existing=[item];new_records+=1
        sources=sorted(set(p['source'] for p in projections.get(address,[])))
        for item in existing:
            item['original_identifier']=item['recovered_name']=row['reference_name']
            ids=[x for x in item['evidence_ids'].split(';') if x]
            if row['evidence_id'] not in ids:ids.append(row['evidence_id'])
            item['evidence_ids']=';'.join(ids)
            if sources:item['pc_implementation']='; '.join(sources)+' (host projections; retail ABI unverified)'
            key=(item['elf_address'],item['record_path'])
            if key in statuses:assert item['status']==statuses[key]
        # Existing function records keep their original behavior/status text and
        # every section after the earlier TASK-0041 naming addendum.
        if row in additions:
            body=(f'{row["evidence_id"]}: recovered reference spelling `{row["reference_name"]}`; '
                f'Debug entry {row["debug_address"]}; retail entry {address}.\n\n'
                f'Retail SHA-256: `{SHA}`; Debug SHA-256: `{DEBUG}`.\n'
                f'Basis: `{row["basis"]}`. Evidence ledger: `reverse/naming/task0042-adopted-retail-names.csv`; '
                'the candidate/unresolved bridge JSON retains independent call/table proofs and limitations.\n\n'
                'This supersedes earlier unknown-name statements only. Existing behavior status, retail fields, '
                'host signatures and throwing gaps are unchanged. No hardware equivalence or Verified promotion.\n')
            paths={ROOT/x['record_path'] for x in existing}
            paths.add(ROOT/record)
            for path in paths:
                text=path.read_text(encoding='utf-8') if path.exists() else (
                    f'# {address} - {row["reference_name"]}\n\n'
                    '- Status: Identified (name correspondence only).\n\n'
                    '## Unresolved questions\n\nRetail ABI, layout and behavioral equivalence remain unverified.\n')
                text=upsert_section(text,'TASK-0042 original-name correspondence',body)
                if args.write:path.parent.mkdir(parents=True,exist_ok=True);save(path,text)
    # Update current source locations by function address, including addresses
    # with multiple host projections. Never infer headers from original TU names.
    for item in index:
        if item['binary_id']!='sha256-'+SHA or item['elf_address'] not in projections:continue
        sources=sorted(set(p['source'] for p in projections[item['elf_address']]))
        item['pc_implementation']='; '.join(sources)+' (host projections; retail ABI unverified)'
    for address,parts in projections.items():
        sources=sorted(set(p['source'] for p in parts))
        destinations='; '.join('`'+x+'`' for x in sources)
        for path in (ROOT/f'reverse/functions/sha256-{SHA}').rglob(address[2:].lower()+'.md'):
            text=path.read_text(encoding='utf-8')
            for part in parts:
                old=part['old_source']
                text=text.replace('`'+old[:-4]+'.hpp` and `.cpp`',destinations)
                text=text.replace('`'+old[:-4]+'.hpp` and `.cpp`,',destinations+',')
                for suffix in ('.cpp','.hpp'):
                    text=text.replace(old[:-4]+suffix,part['source'])
            text=upsert_section(text,'TASK-0042 current projection locations',
                'Current host projection implementation(s): '+destinations+'.\n\n'
                'Location evidence: `reverse/naming/task0042-projection-partitions.json`, joined by retail address. '
                'Original definition ownership is recorded separately; helper namespaces and signatures remain port-side.\n')
            if args.write:save(path,text)
    from generate_class_templates import mask_comments_strings
    remaining=set()
    for path in (ROOT/'src/game_core').rglob('*'):
        if path.suffix in ('.cpp','.hpp','.h'):
            remaining.update(re.findall(r'\b(?:Sub|Type)_[0-9A-Fa-f]{8}\b',mask_comments_strings(path.read_text(encoding='utf-8'))))
    candidates={x['retail_address']:x for x in read_csv(NAMING/'task0041-debug-retail-matches.csv')}
    coverage=[]
    for old in read_csv(NAMING/'task0040-global-symbol-provenance.csv'):
        address=old['address'];name=mappings.get(address);candidate=candidates.get(address)
        coverage.append(dict(original_identifier=old['identifier'],retail_address=address,
            adopted_original_name=name['reference_name'] if name else '',
            candidate_reference_name=candidate['reference_name'] if candidate and not name else '',
            status='adopted_static_name' if name else 'candidate_only' if candidate else 'unresolved_or_ambiguous',
            address_token_remains_in_code=old['identifier'] in remaining,
            evidence_id=name['evidence_id'] if name else 'E-0041-M01'))
    if args.write:
        save(NAMING/'task0042-adopted-retail-names.csv',csv_text(adopted,list(baseline[0])))
        save(NAMING/'task0042-source-coverage.csv',csv_text(coverage,list(coverage[0])))
        save(ROOT/'reverse/symbols.csv',csv_text(index,fields))
    print(json.dumps(dict(adopted_names=len(adopted),new_adoptions=len(additions),new_symbol_records=new_records,
        coverage=dict(Counter(x['status'] for x in coverage)),write=args.write)))


if __name__=='__main__':main()

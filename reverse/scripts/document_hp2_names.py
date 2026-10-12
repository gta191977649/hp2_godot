"""Publish metadata/knowledge records only; proprietary code remains private."""
from hp2_naming import *

plan=json.loads((PRIVATE/'rename-plan.json').read_text())
review=json.loads((ROOT/'reverse/naming/TASK-0007-reviewed.json').read_text())
before=json.loads((PRIVATE/'before.json').read_text())['functions']
after=json.loads((PRIVATE/'after.json').read_text())['functions']
old={r['entry_point']:r for r in before}; current={r['entry_point']:r for r in after}
inventory={r['elf_address'].removeprefix('0x').lower():r for r in csv.DictReader((ROOT/'docs/assessment/FUNCTION_INVENTORY.csv').open())}
client=Client()
changes=plan['renames']+plan['automatic_aliases']
mapping=[]
index_path=ROOT/'reverse/symbols.csv'
with index_path.open(encoding='utf-8') as f:
    reader=csv.DictReader(f); fields=reader.fieldnames; index=list(reader)
binary_id='sha256-'+SHA
for item in changes:
    address=item['address']; row=current[address]
    if row['name']!=item['new_name']: raise ValueError('Mapping name mismatch')
    if old[address]['calling_convention']!=row['calling_convention']:
        raise ValueError('Calling convention changed')
    alias=item['category']=='automatic_thunk_alias'
    original=next(c for c in plan['renames'] if c['address']==item['target']) if alias else item
    subsystem=original['subsystem']
    symbol=rpc(client,'symbols',action='get',address=address)
    mapping.append(dict(address='0x'+address.upper(),old_name=item['old_name'],new_name=item['new_name'],
        category=item['category'],original_identifier='unknown',confidence='high',
        old_source=item['old_symbol']['source'],new_source=symbol['source'],
        evidence_ids=';'.join(item['evidence_ids']),evidence=item['evidence']))
    record=ROOT/'reverse/functions'/binary_id/subsystem/(address+'.md')
    status='Identified' if alias else 'Decompiled'
    probe='ELF direct J with NOP delay slot' if alias else f'ps2_decomp/naming/probes/{address}.json; SHA-256 {item["probe_sha256"]}'
    metric=inventory[address]
    content=f'''# {item['new_name']} — 0x{address.upper()}

- Task/date: TASK-0007 / 2026-10-11 (Australia/Sydney)
- Binary ID: {binary_id}
- ELF SHA-256: {SHA}
- Region/version: USA/NTSC inferred; exact revision unknown; boot VER 1.00.
- Original identifier: unknown. Previous Ghidra identifier: `{item['old_name']}`.
- Naming category: {item['category']}; analysis confidence: high for the stated purpose only.
- Status: {status}; original behavior: not verified; PC implementation: none.
- Ghidra language/compiler: r5900:LE:32:default / default; Ghidra 12.0.4.
- Current analysis signature: `{row['signature']}`; calling convention: {row['calling_convention']} (unverified).
- Current body envelope: {row['start_address']}–{row['end_address']} inclusive; not an ownership claim beyond observed evidence.
- Evidence: {'; '.join(item['evidence_ids'])}; source reference: {probe}.
- Symbol source: {item['old_symbol']['source']} -> {symbol['source']}; this is Ghidra provenance, not original ELF-symbol evidence.
- Historical TASK-0002 caller/callee counts: {metric['callers']} / {metric['callees']}; indirect call sites: {metric['jalr_sites']}.

## Confirmed findings

{item['evidence']}

## Hypotheses

The descriptive name summarizes the observed operation. Original developer/SDK
name, exact class/type layout, caller preconditions and complete ABI are unknown.
Decompiler signatures are analysis hypotheses; no prototype or type was changed.

## Unresolved questions and validation

Full behavior and PS2 hardware/runtime fidelity are unverified. Original/PCSX2
comparison was not performed. Generated C++ is private instruction translation,
not a recovered native-core implementation. Synthetic naming/rollback tests
validate tool safety, not game semantics. See docs/FUNCTION_NAMING.md for code checks.

## History and continuation

2026-10-11: named in TASK-0007 with before/after snapshot and rollback map.
No Git commit made. Next: ABI/body and original comparison tasks before implementation.
'''
    if record.exists():
        raise ValueError(f'Preserve existing record: {record}')
    record.parent.mkdir(parents=True,exist_ok=True); record.write_text(content,encoding='utf-8')
    key=(binary_id,'0x'+address.upper())
    if any((r['binary_id'],r['elf_address'])==key for r in index):
        raise ValueError('Preserve existing indexed symbol; explicit merge required')
    index.append(dict(binary_id=binary_id,elf_address=key[1],original_identifier='unknown',
        recovered_name=item['new_name'],signature=row['signature'],calling_convention=row['calling_convention'],
        subsystem=subsystem,confidence='high',status=status,record_path=record.relative_to(ROOT).as_posix(),
        pc_implementation='none',evidence_ids=';'.join(item['evidence_ids'])))
with (ROOT/'reverse/naming/TASK-0007-name-map.csv').open('w',newline='',encoding='utf-8') as f:
    writer=csv.DictWriter(f,fieldnames=list(mapping[0])); writer.writeheader(); writer.writerows(mapping)
with index_path.open('w',newline='',encoding='utf-8') as f:
    writer=csv.DictWriter(f,fieldnames=fields); writer.writeheader(); writer.writerows(index)
for item in review['deferred']:
    address=item['address']
    record=ROOT/'reverse/functions'/binary_id/'unknown'/(address+'.md')
    if record.exists(): raise ValueError('Preserve existing candidate record')
    record.parent.mkdir(parents=True,exist_ok=True)
    record.write_text(f'''# Deferred naming review — 0x{address.upper()}

- Task/date: TASK-0007 / 2026-10-11 (Australia/Sydney)
- Binary: {binary_id}; USA/NTSC inferred; exact executable revision unknown, boot VER 1.00.
- Current identifier: `{current[address]['name']}`; original identifier: unknown.
- Suggested interpretation: `{item['candidate']}`; confidence: {item['confidence']}.
- Status: Unidentified (candidate only); no new name applied; not Verified.
- Evidence ID: E-0007-P-{address}; private current probes: ps2_decomp/naming/probes/{address}.json.

## Confirmed observations and limitations

{item['reason']}

## Hypotheses and next action

The candidate is an interpretation, not an original symbol or confirmed whole-function purpose.
Retain current name. Resolve boundaries, ABI and dependency semantics before promotion.
Camera/FlightAreas authorship is unknown; their existing names are not independently proved.
No original comparison or behavior implementation/test was performed. No Git commit made.
''',encoding='utf-8')
screen=list(csv.DictReader((ROOT/'reverse/naming/TASK-0007-screening.csv').open(encoding='utf-8')))
by_address={r['address'].lower():r for r in screen}
pilots=json.loads((ROOT/'docs/assessment/PILOT_SELECTION.json').read_text())
pilot_rows=[]
for pilot in pilots:
    r=by_address['0x'+pilot['address']]
    pilot_rows.append(dict(pilot_id=pilot['id'],address=r['address'],old_name=r['old_name'],
        current_name=current[pilot['address']]['name'],candidate=r['proposed_name'],
        disposition=r['disposition'],confidence=r['confidence'],reason=r['reason']))
with (ROOT/'reverse/naming/TASK-0007-pilot-review.csv').open('w',newline='',encoding='utf-8') as f:
    writer=csv.DictWriter(f,fieldnames=list(pilot_rows[0])); writer.writeheader(); writer.writerows(pilot_rows)
dump(PRIVATE/'published-metadata.json',dict(indexed=len(mapping),candidate_records=len(review['deferred']),pilots=len(pilot_rows)))
print(f'Published {len(mapping)} name mappings/records and {len(pilot_rows)} pilot dispositions')

"""Catalog every observed function without inventing original names/classes."""
from hp2_naming import *
from collections import Counter

OUT=ROOT/'ps2_decomp/full-recovery'
PUBLIC=ROOT/'reverse/recovery'

def main():
    check_elf();PUBLIC.mkdir(parents=True,exist_ok=True)
    rows=json.loads((OUT/'snapshot-before.json').read_text())['functions']
    paths={}
    for p in (ROOT/'ps2_decomp/recompiled-named').glob('*.cpp'):
        match=re.search(r'_0x([0-9a-f]+)\.cpp$',p.name)
        if match:
            address=f'{int(match[1],16):08x}'
            if address in paths:raise ValueError('Duplicate translation address')
            paths[address]=p.relative_to(ROOT).as_posix()
    police={
        '001a1ce8':('CopDriver','HP2_CopDriverConstruct','E-0008-02'),
        '001a0dd8':('CopMover','HP2_CopMoverConstruct','E-0008-02'),
        '001a2110':('CopDriver','HP2_CopDriverThink','E-0008-03'),
        '001a1030':('CopMover','HP2_CopMoverMove','E-0008-04'),
        '001a1e90':('CopDriver','HP2_CopDriverChooseAimPoint','E-0008-05'),
        '001a2a90':('CopDriver','HP2_CopDriverCheckLowSpeedTimeout','E-0009-02'),
        '001a0ea0':('CopMover','HP2_CopMoverSelectTypeCoefficient','E-0009-01')}
    candidate_paths={}
    for path in (OUT/'recompiled-candidate').glob('*.cpp'):
        match=re.search(r'_0x([0-9a-f]+)\.cpp$',path.name)
        if match:
            address=f'{int(match[1],16):08x}'
            if address in candidate_paths:raise ValueError('Duplicate candidate translation address')
            candidate_paths[address]=path.relative_to(ROOT).as_posix()
    catalog=[]
    for row in rows:
        address=row['entry_point'];probe=OUT/'functions'/(address+'.json')
        exported=json.loads(probe.read_text()) if probe.exists() else {}
        cls,proposed,evidence=police.get(address,('unknown','unknown','none'))
        purpose_name=row['name'] if row['name'].startswith('HP2_') else proposed
        catalog.append(dict(binary_sha256=SHA,address='0x'+address.upper(),
            current_name=row['name'],original_identifier='unknown',analysis_purpose_name=purpose_name,
            class_association=cls,original_signature='unknown',ghidra_signature=row['signature'],
            start='0x'+row['start_address'].upper(),end_inclusive='0x'+row['end_address'].upper(),
            export_status=exported.get('status','pending'),evidence=evidence,
            pseudocode='ps2_decomp/full-recovery/pseudocode/'+address+'.c' if exported.get('pseudocode_sha256') else 'none',
            instruction_translation=paths.get(address,'none'),
            candidate_instruction_translation=candidate_paths.get(address,'none'),
            candidate_boundary_status='static-flow candidate; not repaired in Ghidra' if address=='001a1030' else 'unchanged',
            native_body='src/game_core/port/police/cop_control.cpp' if address in ('001a0ea0','001a2a90') else 'none',
            native_status='Tested local body; original unverified' if address in ('001a0ea0','001a2a90') else 'not_implemented'))
    assert len(catalog)==len({r['address'] for r in catalog})==7544
    with (PUBLIC/'TASK-0009-functions.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=list(catalog[0]));writer.writeheader();writer.writerows(catalog)
    elf=inspect_elf();hints=[]
    for string in elf['strings']:
        matches=list(re.finditer(r'\b([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*)::(~?[A-Za-z_][A-Za-z0-9_]*)',string['text']))
        for match in matches:
            hints.append(dict(binary_sha256=SHA,string_address='0x'+string['address'].upper(),
                class_text=match[1],method_text=match[2],association_status='unassociated diagnostic text',
                confidence='candidate',evidence_id='E-0009-03'))
    with (PUBLIC/'TASK-0009-class-hints.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=['binary_sha256','string_address','class_text','method_text','association_status','confidence','evidence_id'])
        writer.writeheader();writer.writerows(hints)
    excluded={f'{int(r["Start"],16):08x}' for r in json.loads((ROOT/'ps2_decomp/triage-exclusions.json').read_text())['functions']}
    summary=dict(task='TASK-0009',elf_sha256=SHA,functions=len(catalog),
        export_counts=dict(Counter(r['export_status'] for r in catalog)),
        instruction_files=sum(r['instruction_translation']!='none' for r in catalog),
        instruction_translations=sum(r['instruction_translation']!='none' and r['address'][2:].lower() not in excluded for r in catalog),
        instruction_placeholders=len(excluded),
        trusted_original_function_names=0,native_local_bodies=2,verified_functions=0,
        class_method_text_hints=len(hints),distinct_class_texts=len({r['class_text'] for r in hints}),
        symbol_count=len(elf['symbols']),debug_sections=elf['debug_sections'])
    dump(OUT/'catalog-summary.json',summary)
    print(json.dumps(summary))

if __name__=='__main__':main()

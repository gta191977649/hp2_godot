"""Validate naming consistency, address exclusions, output and representative C++ syntax."""
from hp2_naming import *
import subprocess

check_elf()
before=json.loads((PRIVATE/'before.json').read_text())['functions']
after=json.loads((PRIVATE/'after.json').read_text())['functions']
if bounds(before)!=bounds(after): raise ValueError('Boundary mismatch')
plan=json.loads((PRIVATE/'rename-plan.json').read_text())
expected={r['entry_point']:r['name'] for r in before}
expected.update({r['address']:r['new_name'] for r in plan['renames']+plan['automatic_aliases']})
if any(r['name']!=expected[r['entry_point']] for r in after): raise ValueError('Unexpected name change')
named=ROOT/'ps2_decomp/recompiled-named'
cpp=list(named.glob('*.cpp'))
if len(cpp)!=7545: raise ValueError('Generated file population mismatch')
placeholders=[p.name for p in cpp if 'TODO_NAMED(' in p.read_text(encoding='utf-8')]
excluded=json.loads((ROOT/'ps2_decomp/triage-exclusions.json').read_text())['functions']
if len(placeholders)!=28: raise ValueError('Placeholder count changed')
actual_addresses={int(p.split('_0x')[-1].removesuffix('.cpp'),16) for p in placeholders}
if actual_addresses!={int(r['Start'],16) for r in excluded}: raise ValueError('Placeholder addresses changed')
log=(PRIVATE/'recompile-named.log').read_text()
if not log.rstrip().endswith('Recompilation completed successfully'): raise ValueError('Incomplete generation')
for item in plan['renames']+plan['automatic_aliases']:
    path=named/(item['new_name']+'_0x'+f'{int(item["address"],16):x}'+'.cpp')
    if not path.exists(): raise ValueError(f'Missing named function {path.name}')
    if 'void '+path.stem+'(' not in path.read_text(encoding='utf-8'):
        raise ValueError('Declaration/code name mismatch')
# Every exported row must retain Ghidra's name and exact current envelope.
mapped=list(csv.DictReader((PRIVATE/'ghidra_functions_named.csv').open(encoding='utf-8')))
if len(mapped)!=7544: raise ValueError('Map population mismatch')
by_addr={int(r['entry_point'],16):r for r in after}
for r in mapped:
    actual=by_addr[int(r['Start'],16)]
    if r['Name']!=actual['name'] or int(r['End'],16)!=int(actual['end_address'],16)+1:
        raise ValueError('Map differs from Ghidra snapshot')
base=['g++','-std=c++20','-msse4.1','-mavx','-fsyntax-only',
    '-I','D:/dev/PS2Recomp-0.4/ps2xRuntime/include',
    '-I','D:/dev/PS2Recomp-0.4/ps2xRuntime/src/lib/Kernel','-I',str(named)]
probes=[named/(r['new_name']+'_0x'+f'{int(r["address"],16):x}'+'.cpp') for r in plan['renames']]
probes += [named/'register_functions.cpp',named/'HP2_SpliceListToTail_0x114b20.cpp',
    named/'HP2_DemoCameraCoordinator_Update_0x19a4b8.cpp',named/'sub_0020D8C0_0x20d8c0.cpp']
results=[]
for path in probes:
    command=base+[str(path)]
    result=subprocess.run(command,capture_output=True,text=True,timeout=60)
    (PRIVATE/('syntax-'+path.stem+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
    results.append(dict(file=path.name,command=command,exit_code=result.returncode))
    print(path.name+': '+str(result.returncode),flush=True)
    if result.returncode: raise RuntimeError('Syntax probe failed')
dump(PRIVATE/'named-validation.json',dict(task='TASK-0007',sha256=SHA,bounds_unchanged=True,
    total_functions=7544,direct_renames=16,automatic_alias_renames=11,cpp_files=len(cpp),
    translated_functions=7516,placeholders=28,syntax_probes=results,
    total_generated_bytes=sum(p.stat().st_size for p in named.iterdir() if p.is_file()),
    full_link_and_original_comparison='not performed',verification='not_verified'))
print('Named-output assertions passed')

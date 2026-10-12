"""Generate a separate candidate map/output; NEVER repair live Ghidra implicitly."""
from hp2_naming import *
import argparse
import subprocess

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--generate',action='store_true');args=parser.parse_args()
    check_elf();out=ROOT/'ps2_decomp/full-recovery'
    rows=json.loads((out/'snapshot-before.json').read_text())['functions']
    audit=json.loads((out/'police-flow-audit.json').read_text())['functions'][1]
    if not (audit['complete_static_flow'] and audit['highest_reached']=='001a1bc8' and
            audit['entry_word']=='27bdfdf0' and audit['returns'][0]['delay_word']=='27bd0210'):
        raise ValueError('Candidate control-flow preconditions not met')
    names={'001a1ce8':'HP2_CopDriverConstruct','001a0dd8':'HP2_CopMoverConstruct',
        '001a2110':'HP2_CopDriverThink','001a0ea0':'HP2_CopMoverSelectTypeCoefficient',
        '001a2a90':'HP2_CopDriverCheckLowSpeedTimeout','001a1030':'HP2_CopMoverMove'}
    changes=[]
    for row in rows:
        address=row['entry_point']
        if address in names:
            candidate_end='001a1bcb' if address=='001a1030' else row['end_address']
            changes.append(dict(address=address,old_name=row['name'],analysis_purpose_name=names[address],old_end=row['end_address'],candidate_end=candidate_end))
            row.update(name=names[address],end_address=candidate_end)
    mapfile=out/'ghidra_functions_candidate.csv';export_map(mapfile,rows)
    exclusions=json.loads((ROOT/'ps2_decomp/triage-exclusions.json').read_text())['functions']
    skip=[r['Start'] for r in exclusions if int(r['Start'],16)!=0x1a1030]
    if len(skip)!=27:raise ValueError('Unexpected exclusions')
    generated=out/'recompiled-candidate'
    config='\n'.join(['[general]',f'input = "{ELF.as_posix()}"',f'ghidra_output = "{mapfile.as_posix()}"',
        f'output = "{generated.as_posix()}"','single_file_output = false','patch_syscalls = false',
        'patch_cop0 = false','patch_cache = false','stubs = []','skip = '+json.dumps(skip),'[patches]','instructions = []',''])
    configfile=out/'hp2-candidate.toml';configfile.write_text(config,encoding='utf-8')
    dump(out/'candidate-generation-manifest.json',dict(task='TASK-0009',elf_sha256=SHA,changes=changes,
        excluded_addresses=skip,map_sha256=hashlib.sha256(mapfile.read_bytes()).hexdigest(),
        live_ghidra_changed=False,status='candidate boundary/name override; not original behavior verified'))
    if args.generate:
        if generated.exists():raise ValueError('Preserve prior candidate output; select another directory before regenerating')
        command=[str(ROOT/'local/ps2recomp-build/ps2xRecomp/ps2_recomp.exe'),str(configfile)]
        with (out/'recompile-candidate.log').open('w',encoding='utf-8') as log:
            subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    print('Candidate config/manifest prepared; live Ghidra remains unchanged')

if __name__=='__main__':main()

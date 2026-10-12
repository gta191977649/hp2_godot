"""TASK-0008 read-only bounded police AI probes; output remains private."""
from hp2_naming import *
import argparse

parser=argparse.ArgumentParser()
parser.add_argument('addresses',nargs='+')
args=parser.parse_args()
check_elf(); client=Client(); out=ROOT/'ps2_decomp/police-ai'
for address in args.addresses:
    row=rpc(client,'functions',action='get',address=address)
    entry=row['entry_point']
    probes=dict(function=row)
    for action,extra in [('decompile',dict(timeout=30)),
                         ('listing',dict(end_address=f'{min(int(row["end_address"],16),int(entry,16)+796):08x}',max_lines=200)),
                         ('references_to',dict(page_size=50))]:
        probes[action]=rpc(client,'inspect',action=action,address=entry,**extra)
    dump(out/(entry+'.json'),probes)
    code=probes['decompile'].get('decompiled_code','')
    (out/(entry+'.ghidra.c')).write_text('// Unverified Ghidra pseudocode; TASK-0008; ELF SHA256 '+SHA+'\n'+code,encoding='utf-8')
    print(entry+' '+row['name']+' body='+str(probes['decompile'].get('body_size')),flush=True)

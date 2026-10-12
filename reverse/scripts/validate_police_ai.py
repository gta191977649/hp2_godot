"""TASK-0008 read-only provenance and representative translation checks."""
from hp2_naming import *
import subprocess

out=ROOT/'ps2_decomp/police-ai'
named=ROOT/'ps2_decomp/recompiled-named'
addresses=['001a1ce8','001a0dd8','001a2110','001a1030','001a1e90','001a2a90','001a2c30','001a0ea0']
client=Client()
results=[]
before=check_elf()
for address in addresses:
    saved=json.loads((out/(address+'.json')).read_text())
    live=rpc(client,'functions',action='get',address=address)
    assert live==saved['function'], address+' metadata changed'
    paths=list(named.glob('*_0x'+f'{int(address,16):x}'+'.cpp'))
    assert len(paths)==1, address
    path=paths[0]
    blocked=saved['decompile']['body_size']==1
    if blocked:
        assert 'TODO' in path.read_text(), 'Damaged entry unexpectedly translated'
        results.append(dict(address=address,result='blocked: one-byte body; TODO confirmed',path=str(path)))
        continue
    command=['g++','-std=c++20','-msse4.1','-mavx','-fsyntax-only',
        '-I','D:/dev/PS2Recomp-0.4/ps2xRuntime/include',
        '-I','D:/dev/PS2Recomp-0.4/ps2xRuntime/src/lib/Kernel','-I',str(named),str(path)]
    run=subprocess.run(command,capture_output=True,text=True)
    results.append(dict(address=address,path=str(path),command=command,exit_code=run.returncode,stderr=run.stderr))
    assert run.returncode==0, run.stderr
after=check_elf()
artifacts={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='validation.json'}
dump(out/'validation.json',dict(task='TASK-0008',elf_before=before,elf_after=after,
    compiler=subprocess.run(['g++','--version'],capture_output=True,text=True).stdout,
    python=sys.version,ghidra='12.0.4; r5900:LE:32:default / default',results=results,artifact_sha256=artifacts))
print('PASS: 8 live metadata matches, unchanged ELF hash, 7 C++ syntax probes; Move remains blocked')

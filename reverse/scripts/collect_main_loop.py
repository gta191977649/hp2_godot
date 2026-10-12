"""TASK-0010 read-only MCP evidence capture; source/listings remain private."""
from hp2_naming import *

out=ROOT/'ps2_decomp/main-loop'
check_elf();client=Client()
anchors=['0012f000','0012f378','0014dcd8','001680e8','001a33f0','001a3500','001a35f0','0012f190',
    '00131a30','00131a80','00131ac8','00131cc8','00131da8','001346d8','0014d898','0014d940','001d81f0','001d7e48']
main=rpc(client,'inspect',action='listing',address='0012f378',end_address='0012f683',max_lines=250)
listing=main['data']
sites=[]
for line in listing.splitlines():
    match=re.match(r'([0-9a-f]{8}).*\bjal (?:0x)?([0-9a-f]{8})',line)
    if match:sites.append(dict(call_pc=match[1],target=match[2]))
assert len({s['target'] for s in sites})==39
addresses=list(dict.fromkeys(anchors+[s['target'] for s in sites]))
before=[]
for address in addresses:
    row=rpc(client,'functions',action='get',address=address);before.append(row)
    path=out/(address+'.json')
    existing=json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
    if existing.get('function')!=row:
        probes=dict(function=row,decompile=rpc(client,'inspect',action='decompile',address=address,timeout=30),
            listing=rpc(client,'inspect',action='listing',address=address,end_address=row['end_address'],max_lines=1000))
        dump(path,probes)
after=[rpc(client,'functions',action='get',address=a) for a in addresses]
assert before==after
dump(out/'main-sites.json',sites)
dump(out/'metadata.json',dict(task='TASK-0010',elf_sha256=check_elf(),functions=before,
    metadata_unchanged=True,ghidra='12.0.4; r5900:LE:32:default / default',
    artifacts={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('*.json')
        if p.name=='main-sites.json' or re.fullmatch(r'[0-9a-f]{8}\.json',p.name)}))
print(f'{len(sites)} main-loop call sites, 39 distinct targets; {len(addresses)} probes; metadata/ELF unchanged')

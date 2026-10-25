"""TASK-0097: original event coverage with Debug field evidence; no game assets exported to Git."""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path
import struct
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.ps2_assets import path
from tools.asset_extractor.elab_chunks import parse_chunks_top

DEBUG_SHA="815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b"
RETAIL_SHA="7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"

def walk(nodes):
    for node in nodes:
        yield node
        yield from walk(node.children)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true',help='Compare derived public matrix without rewriting it')
    args=parser.parse_args()
    for name,expected in [('ps2_debug_elf',DEBUG_SHA),('ps2_elf',RETAIL_SHA)]:
        config=json.loads((ROOT/'config'/f'{name}.local.json').read_text(encoding='utf-8'))
        if hashlib.sha256(Path(config['elf']).read_bytes()).hexdigest()!=expected:
            raise ValueError('Configured authorized ELF hash mismatch: '+name)
    bundle=path('FRONTEND','FRONTB.BUN').read_bytes()
    chunks,consumed=parse_chunks_top(bundle)
    if consumed!=len(bundle):raise ValueError('Incomplete original bundle parsing')
    public=[];private=[]
    sizes={0:1,1:4,2:1,3:12,4:16,5:4,6:4}
    for node in walk(chunks):
        if node.tag!=0x80034a00:continue
        tables={c.tag:bundle[c.offset+8:c.offset+8+c.size] for c in node.children}
        name=tables[1].split(b'\0',1)[0].decode('ascii')
        if name not in ('Championship','Hot Pursuit'):raise ValueError('Unaudited tree variant')
        records=tables[0x34a05]
        if len(records)!=32*188:raise ValueError('Unexpected full-tree record count')
        for offset in range(0,len(records),188):
            r=records[offset:offset+188]
            number,kind,car_class,opponent_select,n_cops=struct.unpack_from('5b',r,0x28)
            n_cars,n_tracks,n_goals,n_rewards,n_opponents=struct.unpack_from('5b',r,0x3b)
            objective_index=struct.unpack_from('<h',r,0x42)[0]
            def entry(tag,stride,index):
                data=tables[tag]
                if index<0 or (index+1)*stride>len(data):raise ValueError('Original event table reference outside data')
                return data[index*stride:(index+1)*stride]
            def refs(offset,count,tag,stride):
                return [entry(tag,stride,struct.unpack_from('<h',r,offset+i*2)[0]) for i in range(count)]
            cars=refs(0x44,n_cars,0x34a01,4);tracks=refs(0x6c,n_tracks,0x34a02,4)
            goals=refs(0x7c,n_goals,0x34a03,8);rewards=refs(0x8c,n_rewards,0x34a04,8)
            opponents=refs(0x9c,n_opponents,0x34a06,4)
            objective=struct.unpack('<HHI',entry(0x34a03,8,objective_index))
            public.append(dict(tree=name,event_number=number,event_type=kind,car_class=car_class,
                car_choices=n_cars,track_stages=n_tracks,prerequisites=n_goals,rewards=n_rewards,
                opponents=n_opponents,objective_type=objective[0],objective_data=objective[2],
                debug_state_bytes=sizes[kind],retail_state_bytes=16 if kind==3 else sizes[kind],
                bTickets=r[0x37],bRoadBlocks=r[0x34],bBarricadeSmokeables=r[0x36],
                bSpikeStrips=r[0x35],bHelicopter=r[0x38],behavior_verified='no',
                evidence_id='E-0097-D01/D02/V01'))
            private.append(dict(tree=name,event_number=number,name=r[8:40].split(b'\0',1)[0].decode('ascii'),
                cars=[struct.unpack('<I',x)[0] for x in cars],tracks=[struct.unpack('<I',x)[0] for x in tracks],
                prerequisites=[struct.unpack('<HHI',x) for x in goals],objective=objective,
                rewards=[list(x) for x in rewards],opponents=[list(x) for x in opponents],
                cop_strength=struct.unpack_from('<i',r,0x30)[0],opponent_select=opponent_select,
                damage=struct.unpack_from('<i',r,0xb8)[0],boost_bits=struct.unpack_from('<I',r,0xac)[0]))
    target=ROOT/'reverse/task0097/event-coverage.csv'
    import io
    output=io.StringIO(newline='');writer=csv.DictWriter(output,fieldnames=list(public[0]));writer.writeheader();writer.writerows(public)
    generated=output.getvalue()
    if args.check:
        if target.read_text(encoding='utf-8')!=generated.replace('\r\n','\n'):raise ValueError('Stale event coverage matrix')
    else:
        target.parent.mkdir(parents=True,exist_ok=True);target.write_text(generated,encoding='utf-8',newline='')
        private_dir=ROOT/'local/task0096/root';private_dir.mkdir(parents=True,exist_ok=True)
        (private_dir/'asset-event-bindings.json').write_text(json.dumps(dict(source_sha256=hashlib.sha256(bundle).hexdigest(),events=private),indent=2),encoding='utf-8')
    hot=[r for r in public if r['tree']=='Hot Pursuit']
    print('PASS original asset64 events; Hot Pursuit32, types',dict(collections.Counter(r['event_type'] for r in hot)))
    print('Retail/Debug race-KO storage incompatibility affects',sum(r['event_type']==3 for r in hot),'Hot Pursuit nodes; no progress storage shared')
if __name__=='__main__':main()

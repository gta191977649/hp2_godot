"""Read-only, resumable whole-binary Ghidra pseudocode export (TASK-0009)."""
from hp2_naming import *
import argparse
from collections import Counter
from datetime import datetime, timezone
import time
import threading
from concurrent.futures import ThreadPoolExecutor

OUT=ROOT/'ps2_decomp/full-recovery'

def atomic(path,data):
    path.parent.mkdir(parents=True,exist_ok=True)
    temporary=path.with_suffix(path.suffix+'.tmp')
    temporary.write_text(json.dumps(data,indent=2),encoding='utf-8')
    temporary.replace(path)

def snapshot(client):
    rows=[];cursor=None
    while True:
        result=rpc(client,'functions',action='list',page_size=50,**({'cursor':cursor} if cursor else {}))
        rows.extend(result['data']);cursor=result.get('next_cursor')
        if not cursor:break
    if len(rows)!=len({r['entry_point'] for r in rows}):raise ValueError('Duplicate entries')
    return rows

def export_one(client,row,retry=False):
    address=row['entry_point'];path=OUT/'functions'/(address+'.json')
    if path.exists():
        prior=json.loads(path.read_text())
        if prior['elf_sha256']!=SHA or prior['function']!=row:raise ValueError('Stale export '+address)
        if not retry or prior['status']!='decompile_failed':return prior['status']
    size=int(row['end_address'],16)+1-int(address,16)
    record=dict(task='TASK-0009',elf_sha256=SHA,executable_revision='unknown',function=row,
        captured_at=datetime.now(timezone.utc).isoformat(),status='decompile_failed',original_identifier='unknown',
        proposed_name='unknown',native_implementation='none',verification='not_verified')
    if size==1 or size>65536 or size<=0 or size%4:
        record.update(status='boundary_blocked',reason='Untrustworthy function envelope',envelope_bytes=size)
    else:
        try:
            result=rpc(client,'inspect',action='decompile',address=address,timeout=5)
            if not result.get('decompiled_code','').strip():raise ValueError('No pseudocode returned')
            record.update(status='pseudocode_exported',decompile=result)
            if result.get('body_size')==1:record.update(status='boundary_blocked',reason='Decompiler reports one-byte body')
            code='// UNVERIFIED Ghidra pseudocode, not buildable native C++; TASK-0009\n// ELF SHA256 '+SHA+'\n'+result['decompiled_code']
            output=OUT/'pseudocode'/(address+'.c');output.parent.mkdir(parents=True,exist_ok=True)
            output.write_text(code,encoding='utf-8')
            record['pseudocode_sha256']=hashlib.sha256(output.read_bytes()).hexdigest()
        except Exception as error:
            record['error']=str(error)
    atomic(path,record)
    return record['status']

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--retry-failed',action='store_true')
    parser.add_argument('--limit',type=int)
    parser.add_argument('--workers',type=int,default=4,choices=range(1,5));args=parser.parse_args()
    check_elf();client=Client();rows=snapshot(client)
    atomic(OUT/'snapshot-before.json',dict(elf_sha256=SHA,functions=rows))
    statuses=Counter();start=time.monotonic()
    local=threading.local()
    def worker(row):
        if not hasattr(local,'client'):local.client=Client()
        return export_one(local.client,row,args.retry_failed)
    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        for i,status in enumerate(executor.map(worker,rows[:args.limit] if args.limit else rows),1):
            statuses[status]+=1
            if i%100==0 or i==len(rows):
                atomic(OUT/'progress.json',dict(processed=i,total=len(rows),counts=statuses,elapsed_seconds=time.monotonic()-start))
                print(f'{i}/{len(rows)} {dict(statuses)} elapsed={time.monotonic()-start:.1f}s',flush=True)
    after=snapshot(client)
    if rows!=after:raise ValueError('Ghidra metadata changed during read-only export')
    atomic(OUT/'snapshot-after.json',dict(elf_sha256=check_elf(),functions=after))
    atomic(OUT/'export-summary.json',dict(task='TASK-0009',functions=len(rows),processed=sum(statuses.values()),counts=statuses,
        metadata_unchanged=True,elf_sha256=SHA,elapsed_seconds=time.monotonic()-start,
        ghidra='12.0.4; r5900:LE:32:default / default',python=sys.version,timeout_seconds=5))
    print('Export pass finished; this is not original source/name recovery.',flush=True)

if __name__=='__main__':main()

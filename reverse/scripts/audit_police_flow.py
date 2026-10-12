"""Read-only MIPS control-flow audit of undefined police code units.

Only branch/jump encodings are interpreted; never changes Ghidra boundaries.
Explicit indirect jumps and out-of-window paths block a complete audit.
"""
from hp2_naming import *
from collections import deque

def audit(start,limit,data,sections):
    sec=next(s for s in sections if s['name']=='.text')
    def word(pc):return struct.unpack_from('<I',data,sec['offset']+pc-sec['address'])[0]
    queue=deque([start]);visited=set();delays=set();returns=[];calls=[];unresolved=[];branches=[]
    while queue:
        pc=queue.popleft()
        if pc in visited:continue
        if not start<=pc<limit:
            unresolved.append(dict(address=f'{pc:08x}',reason='Flow outside candidate window'));continue
        visited.add(pc);w=word(pc);op=w>>26;rs=(w>>21)&31;rt=(w>>16)&31;fn=w&63
        imm=w&65535; signed=imm if imm<32768 else imm-65536
        if op==0 and fn==8:
            delays.add(pc+4)
            if rs==31:returns.append(dict(address=f'{pc:08x}',delay_word=f'{word(pc+4):08x}'))
            else:unresolved.append(dict(address=f'{pc:08x}',reason=f'Indirect JR register {rs}'))
        elif op==0 and fn==9:
            delays.add(pc+4);calls.append(dict(address=f'{pc:08x}',target=f'indirect register {rs}'));queue.append(pc+8)
        elif op in (2,3):
            target=((pc+4)&0xf0000000)|((w&0x3ffffff)<<2);delays.add(pc+4)
            if op==3:calls.append(dict(address=f'{pc:08x}',target=f'{target:08x}'));queue.append(pc+8)
            else:queue.append(target)
        elif op in (4,5,6,7,20,21,22,23) or (op==1 and rt in (0,1,2,3,16,17,18,19)) or (op in (16,17,18) and rs==8):
            target=pc+4+(signed<<2);delays.add(pc+4)
            branches.append(dict(address=f'{pc:08x}',target=f'{target:08x}',encoding=f'{w:08x}'))
            if op==1 and rt>=16:
                calls.append(dict(address=f'{pc:08x}',target=f'{target:08x}'));queue.append(pc+8)
            else:queue.extend([target,pc+8])
        else:queue.append(pc+4)
    all_words=visited|delays
    return dict(start=f'{start:08x}',candidate_limit_exclusive=f'{limit:08x}',
        highest_reached=f'{max(all_words):08x}',reached_words=len(all_words),
        unvisited_words=[f'{p:08x}' for p in range(start,limit,4) if p not in all_words],
        returns=returns,calls=calls,branches=branches,unresolved=unresolved,
        entry_word=f'{word(start):08x}',complete_static_flow=not unresolved,
        limitation='Raw branch topology only; no instruction/type/ABI validation or ownership repair')

def main():
    elf=inspect_elf();data=ELF.read_bytes()
    result=dict(task='TASK-0009',elf_sha256=SHA,functions=[
        audit(0x001a0e88,0x001a0ea0,data,elf['sections']),
        audit(0x001a1030,0x001a1bd0,data,elf['sections'])])
    dump(ROOT/'ps2_decomp/full-recovery/police-flow-audit.json',result)
    for f in result['functions']:print(json.dumps({k:v for k,v in f.items() if k not in ('branches','calls','unvisited_words')}))

if __name__=='__main__':main()

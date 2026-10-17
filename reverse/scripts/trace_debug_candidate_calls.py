"""TASK-0042 candidate audit; read-only Ghidra, raw evidence stays in local/.

Every correspondence retains a finite evidence graph to independent anchors.
An anchored caller must have a complete equal normalized instruction stream,
or exactly the complete byte hashes of a previously manually reviewed bridge.
The same call offset must target the candidate pair. Constructor roots require
same-offset this stores of two independently supported complete named tables.
This provides independent context beyond candidate-body signature uniqueness.
Outgoing-only roots require every non-J/JAL instruction word raw identical,
no data-address operands and a finite target proof ending at independent anchors
or raw-identical globally unique closed leaves. They never use normalized-only
callee candidates as proof.
"""
import argparse
import csv
import hashlib
import json
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

from match_debug_reference import Elf, normalize, ROOT, DEBUG_SHA, RETAIL_SHA, write_csv, signed, closed_leaf, unique_bytes


def table_stores(raw):
    """Small straight-line value tracker; only constant writes through this aliases."""
    regs={0:('constant',0),4:('this',0)}; result=[]; clear_after=None
    for off in range(0,len(raw),4):
        word=struct.unpack_from('<I',raw,off)[0]
        op,rs,rt,imm=word>>26,(word>>21)&31,(word>>16)&31,word&65535
        value=None; dest=None
        if op==15: dest=rt;value=('constant',imm<<16)
        elif op in (9,25,13):
            dest=rt
            if rs in regs:
                kind,num=regs[rs]
                if op!=13:value=(kind,(num+signed(imm))&0xffffffff)
                elif kind=='constant':value=(kind,num|imm)
        elif op==0:
            dest=(word>>11)&31
            if word&63 in (33,45) and rt==0:value=regs.get(rs)
        elif op==43:
            if regs.get(rs,('',))[0]=='this' and regs.get(rt,('',))[0]=='constant':
                result.append((off,regs[rt][1],regs[rs][1]+signed(imm)))
        elif op not in (2,3,4,5,6,7,20,21,22,23,40,41,43,49,57,63):dest=rt
        if dest:
            regs.pop(dest,None)
            if value:regs[dest]=value
        if clear_after==off:
            regs={r:v for r,v in regs.items() if r==0 or 16<=r<=23}
        if op in (2,3,4,5,6,7,20,21,22,23):clear_after=off+4
    return result


def rows(name):
    return list(csv.DictReader((ROOT / 'reverse/naming' / name).open(encoding='utf-8-sig')))


def unpack(result):
    if result.get('isError'):
        raise RuntimeError(result)
    return result.get('structuredContent') or json.loads(result['content'][0]['text'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--debug-elf', type=Path, required=True)
    parser.add_argument('--capture', action='store_true')
    args = parser.parse_args()
    debug = Elf(args.debug_elf)
    retail = Elf(json.loads((ROOT/'config/ps2_elf.local.json').read_text())['elf'])
    assert debug.sha == DEBUG_SHA and retail.sha == RETAIL_SHA
    manifest = json.loads((ROOT/'local/task0041/retail-memory-manifest.json').read_text())
    assert manifest['retail_sha256'] == RETAIL_SHA
    for item in manifest['sections']:
        raw = (ROOT/'local/task0041'/('retail'+item['section']+'.bin')).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == item['sha256']
        assert raw == retail.read(int(item['address'],16),item['size'])
    cache = ROOT/'local/task0042/candidates'
    cache.mkdir(parents=True, exist_ok=True)
    sys.path.insert(0,str(ROOT/'tools'))
    from ghidra import Client
    client = Client() if args.capture else None
    def query(name, action, address, **kw):
        path = cache/f'{address:08X}-{name}-{action}.json'
        if args.capture:
            result = client.call('tools/call',dict(name=name,arguments=dict(file_name='NFSHP2.ELF',action=action,address=f'{address:08X}',**kw)))
            unpack(result)
            path.write_text(json.dumps(result,indent=2))
        return unpack(json.loads(path.read_text()))
    # Match one complete live text page, not just an address or program label.
    identity = query('memory','read',0x100000,length=4096,format='hex')
    assert bytes.fromhex(identity['hex_data']) == debug.read(0x100000,4096)
    catalog = {int(x['debug_address'],16):x for x in rows('task0041-debug-functions.csv')}
    matches = {int(x['retail_address'],16):x for x in rows('task0041-debug-retail-matches.csv')}
    anchors = rows('task0041-adopted-retail-names.csv')
    amap = {int(x['debug_address'],16):int(x['retail_address'],16) for x in anchors}
    reverse = {v:k for k,v in amap.items()}
    caller_evidence = defaultdict(list)
    reviewed={int(x['debug_address'],16):x for x in json.loads((ROOT/'reverse/naming/task0041-reviewed-bridges.json').read_text())['bridges']}
    for anchor in anchors:
        da,ra = int(anchor['debug_address'],16),int(anchor['retail_address'],16)
        size=int(catalog[da]['size'])
        db,rb=debug.read(da,size),retail.read(ra,size)
        if len(db)!=len(rb): continue
        dn,dc,_,_=normalize(db,da,0x32C9F0)
        rn,rc,_,_=normalize(rb,ra,0x3358F0)
        reviewed_changes=[]
        if dn != rn:
            review=reviewed.get(da,{})
            reviewed_changes=review.get('structure_offset_changes',[])
            if not reviewed_changes: continue
            if (hashlib.sha256(db).hexdigest()!=review.get('debug_instruction_sha256') or
                hashlib.sha256(rb).hexdigest()!=review.get('retail_instruction_sha256')):
                continue
            adjusted=bytearray(db)
            for change in reviewed_changes:
                off=int(change['function_offset'],16)
                dw=struct.unpack_from('<I',db,off)[0];rw=struct.unpack_from('<I',rb,off)[0]
                assert dw & 65535 == int(change['debug_field_offset'],16)
                assert rw & 65535 == int(change['retail_field_offset'],16)
                assert dw >> 16 == rw >> 16
                struct.pack_into('<I',adjusted,off,rw)
            if normalize(bytes(adjusted),da,0x32C9F0)[0] != rn:
                # R01 includes reviewed lq/sq and long-lived absolute-address
                # operands outside the conservative normalizer. Accept only
                # byte-for-byte the complete bodies previously manually reviewed.
                if (hashlib.sha256(db).hexdigest()!=review.get('debug_instruction_sha256') or
                    hashlib.sha256(rb).hexdigest()!=review.get('retail_instruction_sha256')):
                    continue
        if len(dc)!=len(rc): continue
        # Reject any known independent target contradiction in the anchor.
        if any((dt in amap and amap[dt]!=rt) or (rt in reverse and reverse[rt]!=dt)
               for (_,dt),(_,rt) in zip(dc,rc)): continue
        for (do,dt),(ro,rt) in zip(dc,rc):
            assert do==ro
            caller_evidence[(dt,rt)].append(dict(debug_caller=f'0x{da:08X}',retail_caller=f'0x{ra:08X}',
                caller_name=anchor['reference_name'],caller_evidence_id=anchor['evidence_id'],call_offset=f'0x{do:X}',
                caller_size=size,complete_normalized_body_equal=dn==rn,reviewed_field_offset_changes=reviewed_changes,
                reviewed_complete_body_hashes_equal_record=bool(reviewed_changes)))
    dispositions=[]
    bridges=[]
    scoped=[x for x in rows('task0041-source-coverage.csv') if x['status']=='candidate_only']
    for index,source in enumerate(scoped,1):
        ra=int(source['retail_address'],16); match=matches[ra]; da=int(match['debug_address'],16)
        evidence=f'E-0042-C{index:03d}'
        size=int(match['size']); db=debug.read(da,size); rb=retail.read(ra,size)
        dn,dc,dd,_=normalize(db,da,0x32C9F0); rn,rc,rd,_=normalize(rb,ra,0x3358F0)
        assert dn==rn
        known=[]; conflicts=[]
        for (off,dt),(_,rt) in zip(dc,rc):
            if dt in amap or rt in reverse:
                item=dict(offset=f'0x{off:X}',debug_target=f'0x{dt:08X}',retail_target=f'0x{rt:08X}')
                (known if amap.get(dt)==rt else conflicts).append(item)
        callers=caller_evidence[(da,ra)]
        # Imported symbol is corroborated against the original ELF symbol record.
        live=query('functions','get',da)
        live_ok=live.get('entry_point','').lower().removeprefix('0x')==f'{da:08x}' and live.get('name')==match['mangled_name']
        refs=query('inspect','references_to',da,page_size=500)
        reftext=refs.get('data','')
        callers=[x for x in callers if f"{int(x['debug_caller'],16)+int(x['call_offset'],16):08x}" in reftext.lower()]
        query('inspect','decompile',da)
        supported=bool(callers) and not conflicts and live_ok
        status='supported_anchored_caller' if supported else 'unresolved_candidate'
        reason='Complete normalized candidate body plus independently adopted caller and identical direct-call offset; caller complete normalized body equals or is byte-identical to a complete manually reviewed TASK-0041 bridge; no known call-target contradiction.' if supported else 'No independently adopted caller with complete equal instruction layout, or live symbol/call contradiction; normalized uniqueness alone is insufficient.'
        diffs=[]
        for off in range(0,size,4):
            a,b=struct.unpack_from('<I',db,off)[0],struct.unpack_from('<I',rb,off)[0]
            if a!=b: diffs.append(dict(offset=f'0x{off:X}',debug_opcode=a>>26,retail_opcode=b>>26,changed_bit_mask=f'0x{a^b:08X}'))
        bridge=dict(evidence_id=evidence,retail_address=source['retail_address'],debug_address=match['debug_address'],
            reference_name=match['reference_name'],mangled_name=match['mangled_name'],status=status,size=size,
            live_symbol_matches_elf=live_ok,independent_callers=callers,known_outgoing_calls=known,conflicting_outgoing_calls=conflicts,
            differing_instruction_words=len(diffs),instruction_differences=diffs,
            debug_instruction_sha256=hashlib.sha256(db).hexdigest(),retail_instruction_sha256=hashlib.sha256(rb).hexdigest(),
            raw_evidence_directory='local/task0042/candidates',rationale=reason,
            limitations='Name correspondence only; no Debug layout, ABI, implementation, or hardware equivalence adoption.')
        # Constructor identity receives independent corroboration from two complete
        # already-supported named primary/secondary tables written through this.
        tables=json.loads((ROOT/'reverse/naming/task0041-debug-vtable-ledger.json').read_text())['pairs']
        pairs={(int(t['debug_vtable'],16),int(t['retail_vtable'],16)):t for t in tables if t['status']=='supported_unique_vtable'}
        table_proofs=[]
        for do,dt,dthis in table_stores(db):
            for ro,rt,rthis in table_stores(rb):
                if do==ro and dthis==rthis and (dt,rt) in pairs:
                    table_proofs.append(dict(store_offset=f'0x{do:X}',this_offset=f'0x{dthis:X}',**pairs[(dt,rt)]))
        if match['mangled_name'].startswith('__') and len({x['debug_vtable'] for x in table_proofs})>=2 and not conflicts and live_ok:
            bridge['status']='supported_constructor_named_tables'
            bridge['named_table_store_proofs']=table_proofs
            bridge['rationale']='Complete unique normalized constructor body plus same-offset writes through this to at least two independently supported complete named virtual tables; live original ELF constructor symbol confirmed.'
        bridges.append(bridge)
        dispositions.append(dict(original_identifier=source['original_identifier'],retail_address=source['retail_address'],debug_address=match['debug_address'],reference_name=match['reference_name'],status=status,evidence_id=evidence,independent_callers=len(callers),known_callees=len(known),conflicting_callees=len(conflicts),reason=reason))
        if index%25==0: print(f'Processed {index}/{len(scoped)}',flush=True)
    # Rooted expansion only: each parent was supported before this iteration;
    # cycles without a baseline anchor cannot create a new supported mapping.
    retail_sizes={int(x['Start'],16):int(x['Size']) for x in csv.DictReader((ROOT/'ps2_decomp/ghidra_functions.csv').open(encoding='utf-8-sig'))}
    pair_matches={(int(x['debug_address'],16),int(x['retail_address'],16)):x for x in matches.values()}
    debug_text=debug.read(debug.sections['.text'][3],debug.sections['.text'][5])
    retail_text=retail.read(retail.sections['.text'][3],retail.sections['.text'][5])
    def outgoing_proof(da,ra,path=()):
        if (da,ra) in path:return None
        if amap.get(da)==ra:return dict(basis='TASK-0041 independent adopted anchor',debug_address=f'0x{da:08X}',retail_address=f'0x{ra:08X}')
        prior=next((x for x in bridges if int(x['debug_address'],16)==da and int(x['retail_address'],16)==ra and x['status'].startswith('supported') and (da,ra) not in path),None)
        if prior:return dict(basis='Previously independently supported TASK-0042 bridge',evidence_id=prior['evidence_id'],debug_address=f'0x{da:08X}',retail_address=f'0x{ra:08X}')
        if da not in catalog or ra not in retail_sizes:return None
        size=int(catalog[da]['size'])
        if size!=retail_sizes[ra] or size%4:return None
        db,rb=debug.read(da,size),retail.read(ra,size)
        _,dc,dd,dm=normalize(db,da,0x32C9F0);_,rc,rd,rm=normalize(rb,ra,0x3358F0)
        # Even numerically equal GP offsets refer to different GP bases. No data
        # relocation is accepted through a coincidentally equal instruction.
        if dd or rd:return None
        if not dc:
            if size<32 or db!=rb or not closed_leaf(db) or dm or rm or not unique_bytes(debug_text,db) or not unique_bytes(retail_text,rb):return None
            return dict(basis='Raw identical globally unique closed leaf, no data operands',debug_address=f'0x{da:08X}',retail_address=f'0x{ra:08X}',size=size,instruction_sha256=hashlib.sha256(db).hexdigest())
        if (da,ra) not in pair_matches or len(dc)!=len(rc):return None
        control=bytearray(db)
        for off in range(0,size,4):
            dw,rw=struct.unpack_from('<I',db,off)[0],struct.unpack_from('<I',rb,off)[0]
            if dw>>26 in (2,3):
                if dw>>26!=rw>>26:return None
                struct.pack_into('<I',control,off,0)
            elif dw!=rw:return None
        if not closed_leaf(bytes(control)):return None
        callees=[]
        for (do,dt),(ro,rt) in zip(dc,rc):
            if do!=ro:return None
            if da<=dt<da+size:
                if rt!=ra+dt-da:return None
                proof=dict(basis='Same intra-function direct transfer offset')
            else:
                proof=outgoing_proof(dt,rt,path+((da,ra),))
                if proof is None:return None
            callees.append(dict(call_offset=f'0x{do:X}',debug_target=f'0x{dt:08X}',retail_target=f'0x{rt:08X}',proof=proof))
        return dict(basis='Every non-direct-transfer word raw identical; all direct targets independently proven; no data operands',debug_address=f'0x{da:08X}',retail_address=f'0x{ra:08X}',size=size,callees=callees)
    expanded=set()
    while True:
        changed=False
        for child in bridges:
            if child['status'].startswith('supported') or not child['live_symbol_matches_elf'] or child['conflicting_outgoing_calls']:continue
            proof=outgoing_proof(int(child['debug_address'],16),int(child['retail_address'],16))
            if proof:
                child['status']='supported_raw_code_and_proven_callees'
                child['outgoing_call_proof']=proof
                child['rationale']='Every non-J/JAL instruction word is raw identical; no normalized data operands; every direct target has finite independently anchored or raw-unique closed-leaf proof.'
                changed=True
        for parent in bridges:
            if not parent['status'].startswith('supported') or parent['retail_address'] in expanded:
                continue
            expanded.add(parent['retail_address'])
            da,ra=int(parent['debug_address'],16),int(parent['retail_address'],16)
            size=parent['size']
            _,dc,_,_=normalize(debug.read(da,size),da,0x32C9F0)
            _,rc,_,_=normalize(retail.read(ra,size),ra,0x3358F0)
            for child in bridges:
                if child['status'].startswith('supported') or not child['live_symbol_matches_elf'] or child['conflicting_outgoing_calls']:
                    continue
                dt,rt=int(child['debug_address'],16),int(child['retail_address'],16)
                refs=query('inspect','references_to',dt,page_size=500).get('data','').lower()
                for (do,dtarget),(ro,rtarget) in zip(dc,rc):
                    if dtarget!=dt or rtarget!=rt or do!=ro or f'{da+do:08x}' not in refs:
                        continue
                    child['status']='supported_rooted_call_chain'
                    child['independent_callers']=[dict(debug_caller=parent['debug_address'],retail_caller=parent['retail_address'],
                        caller_name=parent['reference_name'],caller_evidence_id=parent['evidence_id'],call_offset=f'0x{do:X}',
                        caller_size=size,complete_normalized_body_equal=True)]
                    child['rationale']='Complete equal normalized body and same direct-call offset in an already supported parent; finite evidence chain ends at TASK-0041 adopted anchor; live reference corroborates incoming call.'
                    changed=True
                    break
        if not changed: break
    for item,bridge in zip(dispositions,bridges):
        item['status']=bridge['status']
        item['reason']=bridge['rationale']
        item['independent_callers']=len(bridge['independent_callers'])
    out=ROOT/'reverse/naming'
    (out/'task0042-candidate-bridges.json').write_text(json.dumps(dict(task_id='TASK-0042',debug_sha256=DEBUG_SHA,retail_sha256=RETAIL_SHA,policy=__doc__,bridges=bridges),indent=2)+'\n')
    write_csv(out/'task0042-candidate-dispositions.csv',dispositions,list(dispositions[0]))
    print(json.dumps(dict(Counter(x['status'] for x in dispositions))))


if __name__=='__main__': main()

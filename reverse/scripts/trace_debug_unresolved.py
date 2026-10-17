"""TASK-0042 bounded unresolved-name review using independently adopted callers.

Full normalized parent shape only aligns direct call positions. Adoption also
requires independently supported parent identity, child shape equality, and no
conflicting caller targets. Raw MCP evidence stays below ignored local/.
"""
import csv
import json
import sys
import struct
from collections import defaultdict, Counter
from pathlib import Path

from match_debug_reference import ROOT, Elf, normalize, DEBUG_SHA, RETAIL_SHA, MAP_SHA, write_csv
import hashlib
from difflib import SequenceMatcher
from match_debug_vtables import verify_retail_cache, unique_anchors, pair_tables


def rows(name):
    return list(csv.DictReader((ROOT / 'reverse/naming' / name).open(newline='')))


def main():
    debug = Elf(ROOT.parent.parent / 'ps2_game/GAME/DEBUG_VER/NFSHP2.ELF')
    retail = Elf(json.loads((ROOT/'config/ps2_elf.local.json').read_text())['elf'])
    assert debug.sha == DEBUG_SHA and retail.sha == RETAIL_SHA
    verify_retail_cache(retail)
    df = {int(x['debug_address'],16): x for x in rows('task0041-debug-functions.csv')}
    rf = {int(x['Start'],16): int(x['Size']) for x in csv.DictReader((ROOT/'ps2_decomp/ghidra_functions.csv').open(encoding='utf-8-sig'))}
    scope = [x for x in rows('task0041-source-coverage.csv') if x['status']=='unresolved_or_ambiguous']
    ambiguous=rows('task0041-ambiguous-signatures.csv')
    ambiguous_by_retail=defaultdict(list)
    for group in ambiguous:
        for a in group['retail_addresses'].split(';'): ambiguous_by_retail[int(a,16)].append(group)
    scoped = {int(x['retail_address'],16) for x in scope}
    adopted = rows('task0041-adopted-retail-names.csv')
    extra=ROOT/'reverse/naming/task0042-candidate-bridges.json'
    if extra.exists():
        adopted += [x for x in json.loads(extra.read_text())['bridges'] if x['status'].startswith('supported')]
    known = {int(x['debug_address'],16): int(x['retail_address'],16) for x in adopted}
    reviewed={int(x['debug_address'],16):x for x in json.loads((ROOT/'reverse/naming/task0041-reviewed-bridges.json').read_text())['bridges']}
    # Search every initialized non-text retail section, rather than limiting the
    # search to the historical allocator-discovered table catalogue.
    anchors = unique_anchors(rows('task0041-debug-retail-matches.csv'))
    table_pairs=[]
    for symbol in debug.symbols:
        if not symbol['name'].startswith('_vt$') or symbol['size'] < 16 or symbol['size']%8: continue
        da,size=symbol['address'],symbol['size']
        dw=struct.unpack('<'+'I'*(size//4),debug.read(da,size))
        starts=set()
        for i in range(3,len(dw),2):
            if dw[i] not in anchors: continue
            pattern=struct.pack('<I',anchors[dw[i]])
            for sn,s in retail.sections.items():
                if sn not in ('.data','.rodata','.sdata'): continue
                raw=retail.read(s[3],s[5]); at=raw.find(pattern)
                while at>=0:
                    start=s[3]+at-i*4
                    if start%4==0 and s[3]<=start and start+size<=s[3]+s[5]: starts.add(start)
                    at=raw.find(pattern,at+1)
        for ra in starts:
            rw=struct.unpack('<'+'I'*(size//4),retail.read(ra,size))
            count=pair_tables(dw,rw,anchors)
            if count is not None: table_pairs.append((symbol,ra,dw,rw,count))
    by_d=Counter(p[0]['address'] for p in table_pairs)
    by_r=Counter(p[1] for p in table_pairs)
    table_paths=defaultdict(list)
    for s,ra,dw,rw,count in table_pairs:
        if by_d[s['address']]!=1 or by_r[ra]!=1: continue
        for i in range(3,len(dw),2):
            if rw[i] in scoped:
                table_paths[rw[i]].append(dict(debug_address=f'0x{dw[i]:08X}',
                    debug_vtable=f"0x{s['address']:08X}",retail_vtable=f'0x{ra:08X}',
                    debug_table_symbol=s['name'],slot_offset=f'0x{i*4:X}',distinct_unique_anchors=count))
    parent_proposals = defaultdict(list)
    checked = 0
    for parent in adopted:
        da, ra = int(parent['debug_address'],16), int(parent['retail_address'],16)
        if da not in df or ra not in rf: continue
        ds, rs = int(df[da]['size']), rf[ra]
        if ds != rs: continue
        dn = normalize(debug.read(da,ds),da,0x32C9F0)
        rn = normalize(retail.read(ra,rs),ra,0x3358F0)
        reviewed_shape=False
        if dn[0] != rn[0]:
            review=reviewed.get(da,{})
            if not review.get('structure_offset_changes'):continue
            draw,rraw=debug.read(da,ds),retail.read(ra,rs)
            if hashlib.sha256(draw).hexdigest()!=review.get('debug_instruction_sha256') or hashlib.sha256(rraw).hexdigest()!=review.get('retail_instruction_sha256'):continue
            reviewed_shape=True
        checked += 1
        for (offset,dt),(ro,rt) in zip(dn[1],rn[1]):
            if rt not in scoped: continue
            parent_proposals[rt].append(dict(debug_address=f'0x{dt:08X}',
                parent_debug_address=parent['debug_address'], parent_retail_address=parent['retail_address'],
                parent_reference_name=parent['reference_name'],parent_evidence_id=parent['evidence_id'],
                call_offset=f'0x{offset:X}', parent_shape_equal=not reviewed_shape,
                parent_reviewed_layout_evidence=parent['evidence_id'] if reviewed_shape else ''))
    # Explicitly reviewed unequal-layout bATan arms, not fuzzy call matching.
    # Both are the same normalized-coordinate inverse-sine step before the
    # quadrant correction; retail inserts denominator guards ahead of each.
    for do,ro in ((0x70,0x84),(0xAC,0xD0)):
        assert (struct.unpack('<I',debug.read(0x253C60+do,4))[0]&0x3FFFFFF)*4==0x253B18
        assert (struct.unpack('<I',retail.read(0x25A890+ro,4))[0]&0x3FFFFFF)*4==0x25A748
        parent_proposals[0x25A748].append(dict(debug_address='0x00253B18',parent_debug_address='0x00253C60',
            parent_retail_address='0x0025A890',parent_reference_name='bATan(float, float)',parent_evidence_id='E-0042-U192',
            call_offset=f'0x{do:X}',retail_call_offset=f'0x{ro:X}',parent_shape_equal=False,
            parent_reviewed_layout_evidence='E-0042-U192: explicitly reviewed corresponding inverse-sine arms'))
    bridges, dispositions = [], []
    for index,row in enumerate(scope,1):
        ra = int(row['retail_address'],16)
        paths = parent_proposals[ra]
        tables=table_paths[ra]
        targets = {int(p['debug_address'],16) for p in paths}
        candidates=[]
        for da in sorted(targets):
            if da not in df or ra not in rf: continue
            ds, rs = int(df[da]['size']), rf[ra]
            if ds != rs: continue
            dn = normalize(debug.read(da,ds),da,0x32C9F0)
            rn = normalize(retail.read(ra,rs),ra,0x3358F0)
            if dn[0] != rn[0]: continue
            conflict = [(hex(d),hex(r)) for (_,d),(_,r) in zip(dn[1],rn[1]) if d in known and known[d] != r]
            if conflict: continue
            candidates.append(da)
        evidence=f'E-0042-U{index:03d}'
        status = 'unresolved_no_supported_caller_path'
        detail = 'No aligned direct-call edge from an independently adopted parent; no name inferred from proximity.'
        folded=ambiguous_by_retail[ra]
        if folded:
            status='unresolved_folded_signature_without_context'
            detail='Body signature belongs to nonunique group(s); byte/instruction equality cannot select one original identity without an independently established caller or table path.'
        if paths:
            status='unresolved_changed_child_shape'
            detail='Supported parent call positions exist; whole child normalized shape does not establish correspondence.'
        if len(targets)>1:
            status='unresolved_folded_or_conflicting_targets'
            detail='Multiple distinct Debug targets reached from aligned supported callers; nonexclusive implementations are not collapsed to one name.'
        if len(targets)==1 and len(candidates)==1 and df[candidates[0]]['reference_name']:
            da=candidates[0]
            status='supported_caller_and_child_shape'
            detail='Independently adopted parent identity, complete normalized parent shape, exact direct-call position, complete child shape, and no known outgoing target contradiction.'
            bridges.append(dict(evidence_id=evidence,retail_address=row['retail_address'],debug_address=f'0x{da:08X}',
                reference_name=df[da]['reference_name'],mangled_name=df[da]['mangled_name'],status=status,
                size=rf[ra],byte_identical=debug.read(da,rf[ra])==retail.read(ra,rf[ra]),
                caller_paths=paths, limitation='Static name correspondence only; layout, host ABI and behavior equivalence unverified.'))
        if ra==0x133130 and targets=={0x133198}:
            # Reviewed live Debug decompilation/listing: exact control/dataflow,
            # two Car object load offsets increase by eight in retail; remaining
            # differences are GP data relocations. Do not copy Debug Car layout.
            da=0x133198
            dw=struct.unpack('<23I',debug.read(da,92));rw=struct.unpack('<23I',retail.read(ra,92))
            differences=[(i*4,a,b) for i,(a,b) in enumerate(zip(dw,rw)) if a!=b]
            assert [(o,a^b) for o,a,b in differences]==[(0,0x38),(0x24,0x80),(0x28,0x70),(0x30,0x38)]
            status='supported_reviewed_caller_and_field_offsets'
            detail='CarCollisionBody::IsBlinking exact parent call +0x18; complete 92-byte control/dataflow agrees; two object loads change +0x858/+0x85C to +0x860/+0x864; two GP relocations.'
            bridges.append(dict(evidence_id=evidence,retail_address=row['retail_address'],debug_address=f'0x{da:08X}',
                reference_name=df[da]['reference_name'],mangled_name=df[da]['mangled_name'],status=status,size=92,
                caller_paths=paths,reviewed_changes=[dict(offset=f'0x{o:X}',changed_bit_mask=f'0x{a^b:08X}') for o,a,b in differences],
                limitation='Car layout differs; name only, no field/layout adoption.'))
        reviews={
            0x133070:(0x1330D8,108,[0x4,0xC,0x40,0x44,0x58],
                'Car::PleaseResetMeLater: named BottomOut parent +0x29C; complete control/dataflow; store offset +0x854 becomes +0x858; one direct call and three relocated address words.'),
            0x1A4D28:(0x1A2500,352,[0x4,0x38,0x3C,0x44,0x48,0x6C,0x80,0xC4,0xC8,0xD0,0xEC,0xF8,0x114,0x124],
                'Wheel::GetLateralForce: named GuessGroundForce parent +0x3A4; complete control/dataflow and constants unchanged; four Table::GetValue direct targets, seven GP relocations, three absolute low address operands outside normalizer pairing.')}
        if ra in reviews and targets=={reviews[ra][0]}:
            da,size,offsets,reason=reviews[ra]
            dw=struct.unpack('<'+'I'*(size//4),debug.read(da,size));rw=struct.unpack('<'+'I'*(size//4),retail.read(ra,size))
            differences=[(i*4,a,b) for i,(a,b) in enumerate(zip(dw,rw)) if a!=b]
            assert [o for o,a,b in differences]==offsets
            assert all(a>>16==b>>16 or a>>26==b>>26==3 for o,a,b in differences)
            status='supported_reviewed_caller_and_code'
            detail=reason
            bridges.append(dict(evidence_id=evidence,retail_address=row['retail_address'],debug_address=f'0x{da:08X}',
                reference_name=df[da]['reference_name'],mangled_name=df[da]['mangled_name'],status=status,size=size,
                caller_paths=paths,reviewed_changes=[dict(offset=f'0x{o:X}',changed_bit_mask=f'0x{a^b:08X}') for o,a,b in differences],
                rationale=reason,limitation='Static name correspondence only; actual data and object layout must remain retail.'))
        changed_reviews={
          0x1377D8:(0x137798,'Named BottomOut caller at +0x90; both bodies read wheel-ground count, driver throttle and body linear/angular velocity; identical vector squared-length reductions, orientation-z absolute checks and 0/1/2 return classification. Retail inserts Car::IsBlinking veto before damping classification (independently recovered 00133130).'),
          0x138CF8:(0x138C80,'Named PhysicsCar constructor caller; both bodies call RigidBody::SetState once, then Suspension::SetState and Wheel::SetState in four-wheel loop; RPM sentinel 100.0 and gear/clutch synchronization tail agree. Retail additionally clears drivetrain +0x4C history; shifting array layout differs as already established E-0041-R02.'),
          0x139490:(0x139418,'Named Move caller at unchanged call position; complete common tail writes engine/body/aerodynamics boost and 1.0-minus-draft, then four-wheel friction loop. Retail adds car blown-tire/driver condition and scales engine boost only; remaining body agrees including wheel bound 3 and field stores.'),
          0x18AE70:(0x189020,'Named PhysicsCar constructor caller +0x298; same argument storage, four-drive-wheel copy loop, named DriveTrain vtable store, CalculateShiftPoints call and FIRST_GEAR/clutch-state initialization sequence. Retail inserts zero initialization +0x4C and moves shift arrays by four bytes, consistent with E-0041-R02.'),
          0x18B948:(0x189AF0,'Named IterateForces caller +0x48; both bodies dispatch same four clutch states, clamp angular-speed difference by signed ClutchLimiter, call GetTotalEngineTorque in engaged case and ApplyClutchTorque for engaged/transitional cases. Retail inserts previous-slip-history sign/2000.0 checks and optional half stiffness, with +0x4C history write.'),
          0x1A5520:(0x1A2CF8,'Named BeltMover::Move caller +0x5C4; long common body retains slip/contact/8.0/1.0 gates, blown-tire bit extraction, identical vector normalization and tire-direction dot sequence, width/terrain scaling, two elevation calls, second normalization and skid/no-skid tail. Retail removes VisibleViewBits-based nullable-car selection and passes terrain directly; blown-tire field offset changes +0x848 to +0x84C.'),
          0x1C0D78:(0x1BDE60,'Named Wheel::SetState caller +0x6C; both bodies have two eIdentity calls, eRotateY wheel-angle conversion and conditional two eRotateZ calls for wheel index <2, then two four-row matrix writes into distinct indexed output arrays. Retail additionally applies suspension height and YRenderOffset to brake translation, matching existing wheel translation computation.'),
          0x25A890:(0x253C60,'Four independently named caller contexts BeltMover::Move, Mellovator::ResolveForces, Wheel::GuessGroundForce and Wheel::FinalizeGroundForce; both bodies have x/y sign quadrant XOR 3, absolute-coordinate ordering, two bASin call arms and identical 0x4000/0x8000 quadrant correction tail. Retail adds squared-length threshold guards before each normalization; this is behavior counterevidence, not an identity contradiction.'),
          0x25A748:(0x253B18,'Both explicitly reviewed inverse-sine arms of independently rooted bATan call this pair, despite shifted call offsets. Complete common body has sign folding, unit saturation to 0x4000, 65536 scaling, 11-step halving search and interpolated angle/slope table. All 1672 bytes of named Debug bASinTable equal retail table at 0x0031FDC0; signed short versus masked 16-bit return presentation is not adopted as ABI equivalence.')}
        if status=='unresolved_changed_child_shape' and ra in changed_reviews:
            da,reason=changed_reviews[ra]
            if targets=={da}:
                ds,rs=int(df[da]['size']),rf[ra]
                draw,rraw=debug.read(da,ds),retail.read(ra,rs)
                dw=struct.unpack('<'+'I'*(ds//4),draw);rw=struct.unpack('<'+'I'*(rs//4),rraw)
                diff=[dict(kind=tag,debug_start=f'0x{a*4:X}',debug_end=f'0x{b*4:X}',retail_start=f'0x{c*4:X}',retail_end=f'0x{d*4:X}')
                    for tag,a,b,c,d in SequenceMatcher(None,dw,rw,autojunk=False).get_opcodes() if tag!='equal']
                status='supported_reviewed_changed_body'
                detail=reason
                bridges.append(dict(evidence_id=evidence,retail_address=row['retail_address'],debug_address=f'0x{da:08X}',
                    reference_name=df[da]['reference_name'],mangled_name=df[da]['mangled_name'],status=status,size=rs,
                    debug_size=ds,caller_paths=paths,rationale=reason,raw_instruction_difference_spans=diff,
                    retail_raw_evidence_path=f'local/task0042/unresolved/retail-{ra:08X}.json',
                    limitation='Original identity/name only; explicit changed behavior and layouts remain retail-authoritative, not equivalent or Verified.'))
                if ra==0x25A748:
                    table=next(s for s in debug.symbols if s['name']=='bASinTable')
                    td=debug.read(table['address'],table['size']);tr=retail.read(0x31FDC0,table['size'])
                    assert td==tr and table['size']==1672
                    bridges[-1]['independent_named_data_anchor']=dict(name='bASinTable',debug_address=f"0x{table['address']:08X}",
                        retail_address='0x0031FDC0',size=1672,byte_identical=True,sha256=hashlib.sha256(td).hexdigest())
        table_targets={int(p['debug_address'],16) for p in tables}
        if status.startswith('unresolved') and len(table_targets)==1:
            da=next(iter(table_targets))
            if da in df and df[da]['reference_name']:
                status='supported_complete_unique_vtable'
                detail='Complete unique named vtable correspondence over all initialized retail data, exact header/deltas/zero shape, at least two distinct globally unique code anchors, no known target contradictions.'
                bridges.append(dict(evidence_id=evidence,retail_address=row['retail_address'],debug_address=f'0x{da:08X}',
                    reference_name=df[da]['reference_name'],mangled_name=df[da]['mangled_name'],status=status,
                    size=rf.get(ra),table_paths=tables,limitation='Shared implementations do not prove exclusive class ownership.'))
        dispositions.append(dict(original_identifier=row['original_identifier'],retail_address=row['retail_address'],
            status=status,reference_name=bridges[-1]['reference_name'] if status.startswith('supported') else '',
            debug_address=bridges[-1]['debug_address'] if status.startswith('supported') else '',
            supported_caller_paths=len(paths),supported_vtable_paths=len(tables),candidate_debug_targets=';'.join(f'0x{x:08X}' for x in sorted(targets|table_targets)),
            candidate_reference_names=' | '.join(sorted({df[x]['reference_name'] for x in targets|table_targets if x in df and df[x]['reference_name']})),
            ambiguous_signature_groups=len(folded),ambiguous_debug_functions=sum(int(g['debug_count']) for g in folded),
            evidence_id=evidence,reason=detail))
    out=ROOT/'reverse/naming'
    private=ROOT/'local/task0042/unresolved'
    private.mkdir(parents=True,exist_ok=True)
    (private/'caller-proposals.json').write_text(json.dumps({f'0x{a:08X}':p for a,p in parent_proposals.items()},indent=2))
    if '--capture' in sys.argv:
        sys.path.insert(0,str(ROOT/'tools'))
        from ghidra import Client
        client=Client()
        table_capture=private/'bASinTable-live-memory.json'
        if not table_capture.exists():
            table_results=[]
            for program,address,image in (('NFSHP2.ELF',0x316E80,debug),('SLUS_203.62',0x31FDC0,retail)):
                query=dict(action='read',file_name=program,address=f'0x{address:08X}',length=1672,format='hex')
                result=client.call('tools/call',dict(name='memory',arguments=query))
                data=result.get('structuredContent') or json.loads(result['content'][0]['text'])
                assert bytes.fromhex(data['hex_data'])==image.read(address,1672)
                table_results.append(dict(query=query,result=result))
            table_capture.write_text(json.dumps(dict(debug_sha256=DEBUG_SHA,retail_sha256=RETAIL_SHA,results=table_results),indent=2))
        addresses={p['debug_address'] for ps in parent_proposals.values() for p in ps}
        addresses.update(p['parent_debug_address'] for ps in parent_proposals.values() for p in ps)
        for address in sorted(addresses):
            destination=private/f'debug-{address[2:]}.json'
            if destination.exists(): continue
            captures=[]
            for action in ('decompile','references_to','listing'):
                query=dict(action=action,file_name='NFSHP2.ELF',address=address)
                if action=='listing':query['max_lines']=160
                if action=='references_to':query['page_size']=500
                result=client.call('tools/call',dict(name='inspect',arguments=query))
                if result.get('isError'):raise RuntimeError(result)
                captures.append(dict(query=query,result=result))
            destination.write_text(json.dumps(dict(debug_sha256=DEBUG_SHA,results=captures),indent=2))
        for row in dispositions:
            if row['status']!='unresolved_changed_child_shape':continue
            address=row['retail_address'];destination=private/f'retail-{address[2:]}.json'
            if destination.exists():continue
            captures=[]
            for action in ('decompile','listing'):
                query=dict(action=action,file_name='SLUS_203.62',address=address)
                if action=='listing':query['max_lines']=1000
                result=client.call('tools/call',dict(name='inspect',arguments=query))
                if result.get('isError'):raise RuntimeError(result)
                captures.append(dict(query=query,result=result))
            destination.write_text(json.dumps(dict(retail_sha256=RETAIL_SHA,results=captures),indent=2))
    for bridge in bridges:
        raw_path=private/f"debug-{bridge['debug_address'][2:]}.json"
        if not raw_path.exists():raise ValueError('Run --capture to obtain required live Debug evidence')
        raw=json.loads(raw_path.read_text())
        assert raw['debug_sha256']==DEBUG_SHA
        decomp=raw['results'][0]['result']['structuredContent']
        assert decomp['target_name']==bridge['mangled_name']
        refs=raw['results'][1]['result']['structuredContent']['data'].lower()
        for path in bridge.get('caller_paths',[]):
            site=int(path['parent_debug_address'],16)+int(path['call_offset'],16)
            assert f'{site:08x}    unconditional_call' in refs
            path['live_debug_reference_confirmed']=True
        bridge['live_debug_symbol_confirmed']=True
        bridge['raw_evidence_path']=raw_path.relative_to(ROOT).as_posix()
        if bridge['status']=='supported_reviewed_changed_body':
            retail_capture=json.loads((ROOT/bridge['retail_raw_evidence_path']).read_text())
            assert retail_capture['retail_sha256']==RETAIL_SHA
            assert retail_capture['results'][0]['query']['file_name']=='SLUS_203.62'
            bridge['live_retail_decompilation_reviewed']=True
        bridge['debug_instruction_sha256']=hashlib.sha256(debug.read(int(bridge['debug_address'],16),bridge.get('debug_size',bridge['size']))).hexdigest()
        bridge['retail_instruction_sha256']=hashlib.sha256(retail.read(int(bridge['retail_address'],16),bridge['size'])).hexdigest()
    (out/'task0042-unresolved-bridges.json').write_text(json.dumps(dict(task_id='TASK-0042',debug_sha256=DEBUG_SHA,
        retail_sha256=RETAIL_SHA,map_sha256=MAP_SHA,independently_adopted_aligned_parents=checked,
        vtable_search=dict(scope='All initialized retail .data/.rodata/.sdata sections; every named Debug _vt$ table with complete two-anchor policy',
            compatible_pairs=len(table_pairs),unique_pairs=sum(by_d[p[0]['address']]==by_r[p[1]]==1 for p in table_pairs),
            new_scoped_names=0,limitation='Single-anchor and changed-size tables intentionally remain unsupported; no inferred exclusive owner.'),
        summary=dict(Counter(x['status'] for x in dispositions)),bridges=bridges),indent=2)+'\n')
    write_csv(out/'task0042-unresolved-dispositions.csv',dispositions,list(dispositions[0]))
    print(json.dumps(dict(summary=dict(Counter(x['status'] for x in dispositions)),bridges=len(bridges))))


if __name__=='__main__': main()

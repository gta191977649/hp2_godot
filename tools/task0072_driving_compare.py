"""Exact port-side driving transport parity; captures remain ignored local data."""
import argparse
import json
from pathlib import Path

def compare(a, b):
    errors=[]
    checks=0
    def equal(x,y,tag):
        nonlocal checks
        checks+=1
        if x!=y: errors.append(tag)
    for side,p in [('reference',a),('candidate',b)]:
        if not p.get('finished') or p.get('error'):errors.append(side+': incomplete fixture')
        if len(p.get('timeline',[]))!=723 or len(p.get('captures',[]))!=7:errors.append(side+': incomplete timeline/captures')
        if len(p.get('teardown',[]))!=2:errors.append(side+': missing teardown')
    for key in ['engine','method','driver','adapter','viewport_size','track','car','elapsed','main_steps']:
        equal(a.get(key),b.get(key),'configuration '+key)
    for x,y in zip(a.get('timeline',[]),b.get('timeline',[])):
        tag=str((x['session'],x['index']))
        equal((x['session'],x['index']),(y['session'],y['index']),tag+' index')
        for key in ['step_before','step_after','step_delta','native_tick_calls','elapsed','native_dt','input','body_bits','camera_bits','wheels']:
            equal(x['driving'].get(key),y['driving'].get(key),tag+' driving '+key)
        for z in (x,y):
            if z['driving'].get('native_tick_calls')!=1 or z['driving'].get('step_delta') not in (0,1): errors.append(tag+' invalid native scheduler update')
    for x,y in zip(a.get('captures',[]),b.get('captures',[])):
        tag=str((x['session'],x['index']))
        equal((x['session'],x['index']),(y['session'],y['index']),tag+' capture index')
        for key in ['packet_digest','commands','visible_instances','sections','sun_quads']:
            equal(x['native'].get(key),y['native'].get(key),tag+' native '+key)
        equal(x['native'].get('lod',{}).get('native_culling_views'),y['native'].get('lod',{}).get('native_culling_views'),tag+' all view command digests/selection')
        for fa,fb in zip(x.get('reflection_faces',[]),y.get('reflection_faces',[])):
            for field in ['face','status','transform_bits','fov','near','far','keep_aspect','projection','cull_mask','camera_projection_bits']:
                equal(fa.get(field),fb.get(field),tag+' face camera '+field)
        for key in ['effects','environment_available','environment_matrix_bits']:
            equal(x.get(key),y.get(key),tag+' '+key)
        equal(sorted(x['captures']),sorted(y['captures']),tag+' image targets')
        for key in ['final','colour','native_depth']:
            if key not in x['captures'] or key not in y['captures']:errors.append(tag+' missing '+key)
        for key in x['captures'].keys() & y['captures'].keys():
            for field in ['sha256','width','height','format','raw_bytes']:
                equal(x['captures'][key].get(field),y['captures'][key].get(field),tag+' '+key+' '+field)
    return dict(task='TASK0072',passed=not errors,checks=checks,errors=errors,scope='723 original-scheduler driving tick calls and7 native-resolution image checkpoints including reload; port-side parity only, no PS2 runtime verification')

if __name__=='__main__':
    p=argparse.ArgumentParser(__doc__)
    p.add_argument('reference',type=Path);p.add_argument('candidate',type=Path);p.add_argument('--output',type=Path)
    args=p.parse_args();result=compare(json.loads(args.reference.read_text()),json.loads(args.candidate.read_text()))
    result['image_and_state_parity']=result['passed']
    runtime_errors=[]
    for label,path in [('reference',args.reference),('candidate',args.candidate)]:
        logs=[path.parent/name for name in ['errors.log','errors.txt'] if (path.parent/name).exists()]
        if not logs:runtime_errors.append(label+': missing runtime error log')
        for log in logs:
            if 'ERROR:' in log.read_text(encoding='utf-8',errors='replace'):runtime_errors.append(label+': runtime error in '+log.name)
    result['runtime_lifecycle_clean']=not runtime_errors
    result['errors'].extend(runtime_errors)
    result['passed']=result['passed'] and not runtime_errors
    rendered=json.dumps(result,indent=2)
    if args.output:args.output.write_text(rendered+'\n')
    print(rendered);raise SystemExit(0 if result['passed'] else 1)

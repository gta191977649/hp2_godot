"""TASK0063 explicit successor. Draft observations never authorize source changes."""
from __future__ import annotations
from collections import Counter
from functools import lru_cache
import copy
import hashlib
import json
from pathlib import Path
import audit_task0061_review as predecessor
import audit_task0044_migration as identities
import audit_task0048_review as transition
ROOT=Path(__file__).resolve().parents[2]
REVIEW=ROOT/'reverse/task0063/source-review.json'
SCOPE=ROOT/'reverse/task0063/review-scope.json'
PREDECESSOR_REVIEW_SHA256='8e1bc51d1d9df5c65cbef560337900f5a63e7e400a418b805c2fcfc217487618'
PREDECESSOR_SCOPE_SHA256='25dbaf892a66fdb243bba611b044dbe647273ba1e7ccb2395c8ccf9348a0e947'

def fingerprint(path):
    return hashlib.sha256(path.read_bytes().replace(b'\r\n',b'\n')).hexdigest()

def _chain_files():
    # Every frozen input participates in the cache key; edits cannot hide behind caching.
    modules=[];module=predecessor
    while module is not None and module not in modules:
        modules.append(module);module=getattr(module,'predecessor',None)
    paths={predecessor.REVIEW}
    for module in modules+[identities]:
        for name in ('SCOPE','REVIEW','ALIAS_REVIEW','BASELINE','SUCCESSOR_BASELINE','SUCCESSOR_REVIEW'):
            value=getattr(module,name,None)
            if isinstance(value,Path) and value.is_file():paths.add(value)
    return tuple(sorted((str(path),fingerprint(path)) for path in paths))

@lru_cache(maxsize=2)
def _historical_from_frozen_inputs(chain_key):
    before=predecessor.historical_identities()
    final=transition.reviewed_identities(before,json.loads(predecessor.REVIEW.read_text()))
    if not predecessor.check_current(current_identities=final)['passed']:
        raise ValueError('TASK0061 frozen reviewed state failed its predecessor chain')
    return final

def historical_identities():
    if fingerprint(predecessor.SCOPE)!=PREDECESSOR_SCOPE_SHA256:
        raise ValueError('Frozen TASK0061 scope changed')
    if fingerprint(predecessor.REVIEW)!=PREDECESSOR_REVIEW_SHA256:
        raise ValueError('Frozen TASK0061 review changed')
    return copy.deepcopy(_historical_from_frozen_inputs(_chain_files()))

def immutable_physics(path,name=None):
    if path=='src/game_core/indep/src/skids.cpp':return True
    return (path.startswith('src/game_core/port/driving/') or
        (path.startswith('src/game_core/') and Path(path).stem.lower() in {
        'physicscar','physicswheel','rigidbody','dynamiccollisionbody','collisionbody',
        'contact','world','voronoiclip','polyhedra'}))

def scope_errors(review,current,*,before=None,auxiliary_fingerprints=None):
    errors=[]
    scope=json.loads(SCOPE.read_text())
    if scope.get('task')!='TASK-0063':return ['Wrong source scope task']
    slots={}
    for row in scope.get('entries',[]):
        key=(row.get('path'),row.get('name'))
        if key in slots:errors.append('Duplicate scope slot '+str(key))
        slots[key]=row
    for side in ('before','after'):
        for row in review.get(side,[]):
            key=(row.get('path'),row.get('name'));slot=slots.get(key)
            if immutable_physics(str(row.get('path','')),row.get('name')):errors.append('Unrelated physics is immutable: '+str(key))
            if slot is None:errors.append('Unmapped function '+str(key));continue
            for field in ('evidence_id','evidence_record','reason'):
                if not slot.get(field) or row.get(field)!=slot.get(field):errors.append('Scope evidence/reason mismatch '+str(key)+' '+field)
            record=ROOT/str(slot.get('evidence_record',''))
            if not record.is_file():errors.append('Missing evidence record '+str(key))
            elif slot.get('evidence_id','') not in record.read_text(encoding='utf-8'):errors.append('Evidence ID absent from record '+str(key))
            if slot.get('kind') not in ('port_adapter','recovered','explicit_gap'):errors.append('Unclassified owner '+str(key))
            allowed={
                'src/platform/godot_bridge/ps2_render_host.cpp',
                'src/platform/godot_bridge/ps2_track_renderer.cpp',
                'src/platform/godot_bridge/track_particle_renderer.cpp',
                'src/platform/godot_bridge/particle_mesh_stream.hpp',
                'src/platform/godot_bridge/track_command_batches.hpp',
                'src/platform/godot_bridge/track_postprocess.cpp',
                'src/platform/godot_bridge/track_sun_renderer.cpp',
                'src/game_core/port/track/particles.cpp',
                'src/game_core/port/track/particles.hpp',
                'src/game_core/port/track/props_visibility.hpp',
                'src/game_core/port/track/settings.hpp',
                'src/game_core/extensions/viewer_ui.cpp',
            }
            if str(row.get('path',''))=='src/platform/godot_bridge/track_command_batches.hpp' and row.get('name')!='eligible_track_opaque_queue':
                errors.append('Only the exact experimental opaque-queue helper is permitted: '+str(key))
            if slot.get('kind')!='port_adapter' or str(row.get('path','')) not in allowed:
                errors.append('TASK0063 only permits explicit host rendering/cache and TASK0062 props UI bodies: '+str(key))
            if side=='after' and row.get('stub') and slot.get('kind')!='explicit_gap':errors.append('New throwing stub lacks explicit gap review '+str(key))
            if slot.get('kind') in ('recovered','explicit_gap'):
                records=slot.get('function_records',[])
                if not records:errors.append('Missing original function records '+str(key))
                for relative in records:
                    path=ROOT/relative
                    if not path.is_file():errors.append('Missing original function record '+relative);continue
                    text=path.read_text(encoding='utf-8')
                    original=slot.get('original_name','').split('::')[-1]
                    if (not original or original not in text or path.stem.lower() not in text.lower() or
                        not any(sha in text for sha in ('7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2',identities.TARGET_SHA256))):
                        errors.append('Invalid original function record '+relative)
    auxiliary_scope=scope.get('auxiliary_sources',[])
    expected_auxiliary={row.get('path'):row for row in auxiliary_scope}
    if len(expected_auxiliary)!=len(auxiliary_scope):errors.append('Duplicate auxiliary scope path')
    rows=review.get('auxiliary_sources',[])
    if Counter(row.get('path') for row in rows)!=Counter(expected_auxiliary.keys()):errors.append('Auxiliary sources do not match exact scope')
    allowed_auxiliary={
        'src/tests/track_command_batches_tests.cpp':'E0063O01',
        'src/platform/godot_bridge/track_particle_renderer.cpp':'E0063D01',
        'src/platform/godot_bridge/track_postprocess.cpp':'E0063D01',
    }
    for row in rows:
        path=str(row.get('path',''));slot=expected_auxiliary.get(path)
        if path not in allowed_auxiliary:
            errors.append('Unapproved auxiliary source '+path);continue
        source=ROOT/path
        actual=fingerprint(source) if auxiliary_fingerprints is None and source.is_file() else (auxiliary_fingerprints or {}).get(path)
        if actual!=row.get('lf_sha256'):
            errors.append('Auxiliary source changed '+path)
        if slot is None or any(row.get(field)!=slot.get(field) for field in ('evidence_id','evidence_record','reason')):
            errors.append('Auxiliary scope mismatch '+path)
        if row.get('evidence_id')!=allowed_auxiliary[path] or row.get('evidence_record')!='reverse/task0063/host-preservation.md' or not row.get('reason'):
            errors.append('Auxiliary source evidence/reason missing '+path)
    # Stub classification is part of the current contract even for unchanged bodies.
    pins={identities._identity_key(row):row.get('stub',False) for row in transition.reviewed_identities(historical_identities() if before is None else before,review)}
    for row in current:
        key=identities._identity_key(row)
        if key in pins and bool(row.get('stub'))!=bool(pins[key]):errors.append('Stub classification changed '+str(key[:2]))
    return errors

def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if auxiliary_fingerprints is not None:
        if current_identities is None:raise ValueError('Historical auxiliary state requires explicit identities')
        expected={row['path'] for row in json.loads(REVIEW.read_text())['auxiliary_sources']}
        if set(auxiliary_fingerprints)!=expected:raise ValueError('Wrong historical auxiliary snapshot paths')
    if current_identities is None:
        from audit_task0064_review import check_current as latest
        return latest()
    before=historical_identities()
    if not REVIEW.is_file():raise ValueError('TASK0063 approved source review is required; draft inventory is not approval')
    review=json.loads(REVIEW.read_text())
    if (review.get('task')!='TASK-0063' or review.get('status')!='approved'
        or review.get('target_binary_sha256')!=identities.TARGET_SHA256
        or review.get('predecessor_review_lf_sha256')!=PREDECESSOR_REVIEW_SHA256
        or review.get('baseline_inventory_sha256')!=identities.inventory_digest(before)
        or review.get('scope_lf_sha256')!=fingerprint(SCOPE)):
        raise ValueError('TASK0063 review has wrong approval, target, predecessor, scope or derived inventory')
    current=identities.source_identities(identities.historical.collect_current()) if current_identities is None else current_identities
    amendment=identities.check_identity_review(before,current,review)
    errors=scope_errors(review,current,before=before,auxiliary_fingerprints=auxiliary_fingerprints)
    return dict(passed=amendment['passed'] and not errors,task0061_historical_passed=True,task0063_amendment=amendment,scope_errors=errors)

if __name__=='__main__':
    result=check_current();print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)

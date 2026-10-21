"""TASK0071 strict host-only source successor. Frozen prior reviews are immutable."""
import argparse
from collections import Counter
import json
import audit_task0070_review as predecessor
import audit_task0044_migration as identities
import audit_task0048_review as transition
ROOT=predecessor.ROOT
REVIEW=ROOT/'reverse/task0071/source-review.json'
SCOPE=ROOT/'reverse/task0071/review-scope.json'
EVIDENCE='reverse/task0071/host-rendering.md'
PREDECESSOR_REVIEW_SHA256='99eab2a39122a662915977e23f6a86eb20f831b879d1103e4c19ea0c324c7134'
PREDECESSOR_SCOPE_SHA256='1bee96543a3c4335051f223ff2fa25a7020a15b5afe6fff05ee3db0cba5b1857'
fingerprint=predecessor.fingerprint
require=predecessor.require
ALLOWED={
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::rebuild_display'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::render_skids'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::update_view'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::update'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::TrackPostprocess'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::release'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::stats'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::release'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::stats'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::TrackPostprocess'),
 ('src/platform/godot_bridge/track_postprocess.cpp','impl_'),
 ('src/platform/godot_bridge/track_postprocess.cpp','depth_material'),
 ('src/platform/godot_bridge/track_reflection_renderer.cpp','submit'),
 ('src/platform/godot_bridge/track_reflection_renderer.cpp','impl_'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','impl_'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','TrackSunRenderer::update'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','TrackSunRenderer::complete_readback'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','create_sun_target'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','TrackSunRenderer::stats'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','TrackSunRenderer::clear'),
}
EXTRA='''godot/scripts/runtime/render_contract.gd
godot/scripts/debug/encoded_view.gd
godot/scripts/runtime/encoded_transport_gpu_smoke.gd
godot/scenes/encoded_transport_gpu_smoke.tscn
tools/task0071_pixels.py
reverse/scripts/audit_task0070_review.py
src/platform/godot_bridge/ps2_vehicle_renderer.cpp
src/platform/godot_bridge/ps2_driving_runtime.cpp
src/platform/godot_bridge/ps2_track_renderer.cpp
src/platform/godot_bridge/track_reflection_renderer.cpp
src/platform/godot_bridge/track_sun_renderer.cpp
src/platform/godot_bridge/track_sun_renderer.hpp
godot/scripts/runtime/track_performance.gd
godot/scripts/runtime/track_performance_scene.gd
tools/task0071_performance.py
godot/scripts/debug/car_material_gpu_smoke.gd
godot/scripts/runtime/sky_depth_gpu_smoke.gd
godot/scripts/runtime/track_sun_gpu_smoke.gd
godot/scripts/runtime/track_particles_gpu_smoke.gd
godot/scripts/runtime/track_postprocess_gpu_smoke.gd
godot/scripts/runtime/track_postprocess_lifecycle_gpu_smoke.gd
godot/scripts/runtime/track_pipeline_smoke.gd
godot/scripts/runtime/track_fusion_gpu_smoke.gd'''.splitlines()
SHADERS='''car_base_shader.hpp car_base.gdshader car_modulate.glsl car_wireframe.gdshader embedded_shaders.hpp.in postprocess_composite.glsl.inc postprocess.glsl screen_projection.hpp sky_depth.hpp track_alpha_vertex.glsl track_debug_edges.gdshader track_debug_fill.gdshader track_depth.glsl track_effects.glsl track_fragment.glsl track_sampling.glsl track_shader.cpp track_shader.hpp'''.split()
AUXILIARY=predecessor.AUXILIARY|set(EXTRA)|{'src/platform/shaders/'+name for name in SHADERS}


def historical_identities():
    require(fingerprint(predecessor.REVIEW)==PREDECESSOR_REVIEW_SHA256,'Frozen TASK0070 review changed')
    require(fingerprint(predecessor.SCOPE)==PREDECESSOR_SCOPE_SHA256,'Frozen TASK0070 scope changed')
    review=json.loads(predecessor.REVIEW.read_text())
    result=transition.reviewed_identities(predecessor.historical_identities(),review)
    auxiliary={r['path']:r['lf_sha256'] for r in review['auxiliary_sources']}
    require(predecessor.check_current(current_identities=result,auxiliary_fingerprints=auxiliary)['passed'],'Historical predecessor failed')
    return result


def _slot(row):
    return row['path'],row['name']


def _annotation():
    return dict(kind='port_adapter',evidence_id='E0071H01',evidence_record=EVIDENCE,
                reason='Vulkan Forward+ host adaptation and measured resource/synchronization optimization; recovered algorithms and effect timing remain unchanged.')


def write_review():
    before=historical_identities()
    baseline=ROOT/'local/task0071/baseline/source-identities.json'
    require(baseline.exists(),'Missing preserved TASK0071 startup inventory')
    require(Counter(identities._identity_key(r)+(bool(r.get('stub')), ) for r in json.loads(baseline.read_text()))==Counter(identities._identity_key(r)+(bool(r.get('stub')), ) for r in before),'Startup inventory differs from predecessor')
    current=identities.source_identities(identities.historical.collect_current())
    sets=[Counter(identities._identity_key(r) for r in rows) for rows in (before,current)]
    changed=[]
    for index,rows in enumerate((before,current)):
        delta=sets[index]-sets[1-index]
        changed.append([dict(row,**_annotation()) for row in rows if delta[identities._identity_key(row)]])
    slots={_slot(row) for rows in changed for row in rows}
    require(slots<=ALLOWED,'Unreviewed identity changes: '+str(sorted(slots-ALLOWED)))
    require(all(not r.get('stub') for rows in changed for r in rows),'Stub change is outside host scope')
    scope=dict(task='TASK-0071',entries=[dict(path=path,name=name,**_annotation()) for path,name in sorted(slots)],
               auxiliary_sources=[dict(path=path,**_annotation()) for path in sorted(AUXILIARY)])
    SCOPE.write_text(json.dumps(scope,indent=2)+'\n',encoding='utf-8')
    review=dict(task='TASK-0071',status='approved',target_binary_sha256=identities.TARGET_SHA256,
                predecessor_review_lf_sha256=PREDECESSOR_REVIEW_SHA256,
                baseline_inventory_sha256=identities.inventory_digest(before),scope_lf_sha256=fingerprint(SCOPE),
                before=changed[0],after=changed[1],
                auxiliary_sources=[dict(path=path,lf_sha256=fingerprint(ROOT/path),**_annotation()) for path in sorted(AUXILIARY)])
    REVIEW.write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
    return check_current(current_identities=current)


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0072_review import check_current as latest
        return latest()
    require(auxiliary_fingerprints is None or current_identities is not None,'Historical auxiliary state requires explicit identities')
    require(auxiliary_fingerprints is None or set(auxiliary_fingerprints)==AUXILIARY,'Wrong auxiliary snapshot paths')
    before=historical_identities();review=json.loads(REVIEW.read_text());scope=json.loads(SCOPE.read_text())
    require(review.get('task')==scope.get('task')=='TASK-0071' and review.get('status')=='approved','Wrong task/approval')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_REVIEW_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    require(review.get('scope_lf_sha256')==fingerprint(SCOPE),'Changed scope')
    slots={_slot(r):r for r in scope['entries']}
    require(set(slots)<=ALLOWED and len(slots)==len(scope['entries']),'Wrong scope slots')
    require({_slot(r) for side in ('before','after') for r in review[side]}==set(slots),'Scope/review slots differ')
    for side in ('before','after'):
        for row in review[side]:
            require(not row.get('stub',False),'Stub replacement forbidden')
            require(all(row.get(k)==v and slots[_slot(row)].get(k)==v for k,v in _annotation().items()),'Function evidence mismatch')
    require('E0071H01' in (ROOT/EVIDENCE).read_text(),'Missing host evidence')
    auxiliary={r['path']:r for r in scope['auxiliary_sources']}
    require(set(auxiliary)==AUXILIARY and len(auxiliary)==len(scope['auxiliary_sources']),'Wrong auxiliary scope')
    require(len(review['auxiliary_sources'])==len(AUXILIARY) and {r['path'] for r in review['auxiliary_sources']}==AUXILIARY,'Wrong auxiliary review')
    for row in review['auxiliary_sources']:
        require(all(row.get(k)==v and auxiliary[row['path']].get(k)==v for k,v in _annotation().items()),'Auxiliary evidence mismatch')
        actual=fingerprint(ROOT/row['path']) if auxiliary_fingerprints is None else auxiliary_fingerprints.get(row['path'])
        require(actual==row.get('lf_sha256'),'Changed auxiliary source '+row['path'])
    current=identities.source_identities(identities.historical.collect_current()) if current_identities is None else current_identities
    amendment=identities.check_identity_review(before,current,review)
    pins={identities._identity_key(r):bool(r.get('stub')) for r in transition.reviewed_identities(before,review)}
    require(all(identities._identity_key(r) not in pins or pins[identities._identity_key(r)]==bool(r.get('stub')) for r in current),'Changed stub classification')
    return dict(passed=amendment['passed'],task0071_amendment=amendment)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write',action='store_true',help='Explicitly pin the settled allowlisted host changes')
    args=parser.parse_args()
    result=write_review() if args.write else check_current()
    print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)

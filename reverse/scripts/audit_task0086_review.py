"""Strict TASK0086 vehicle flares successor; frozen predecessor JSON is immutable."""
from collections import Counter
import json
import audit_task0085_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0086/source-review.json'
PREDECESSOR_SHA256 = '01c86cb0f7030eb8055c773e307e26134832995ad2a19d3ac1d3ff79538a4f47'
REPLACED = {
 ('src/game_core/port/driving/native_backend.cpp', 'NativeBackend::publish'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::get_state'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp', 'Ps2VehicleRenderer::apply_driving_snapshot'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp', 'Ps2VehicleRenderer::get_visual_state'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp', 'Ps2VehicleRenderer::release_display'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp', 'Ps2VehicleRenderer::update_view'),
 ('src/port/assets/vehicle_geometry.cpp', 'decode_vehicle_geometry'),
 ('src/port/vehicle/model.cpp', 'Model::load'),
}
ADDED = {
 ('src/game_core/port/driving/vehicle_flares.cpp', 'create_vehicle_flare_marker'),
 ('src/game_core/port/driving/vehicle_flares.cpp', 'read_vehicle_flare_resources'),
 ('src/game_core/port/driving/vehicle_flares.cpp', 'transform'),
 ('src/game_core/port/driving/vehicle_flares.cpp', 'vehicle_flare_angle_power'),
 ('src/game_core/port/driving/vehicle_flares.cpp', 'vehicle_flare_billboards'),
 ('src/game_core/port/driving/vehicle_flares.cpp', 'vehicle_flare_intensity'),
 ('src/game_core/port/driving/vehicle_flares.cpp', 'vehicle_flare_state'),
 ('src/game_core/port/driving/vehicle_flares.cpp', 'xyz'),
 ('src/platform/godot_bridge/particle_billboard_renderer.cpp', 'ParticleBillboardRenderer::clear'),
 ('src/platform/godot_bridge/particle_billboard_renderer.cpp', 'ParticleBillboardRenderer::stats'),
 ('src/platform/godot_bridge/particle_billboard_renderer.cpp', 'ParticleBillboardRenderer::update'),
 ('src/platform/godot_bridge/particle_billboard_renderer.cpp', 'billboard_host'),
 ('src/platform/godot_bridge/particle_billboard_renderer.cpp', 'impl_'),
 ('src/platform/godot_bridge/particle_billboard_renderer.cpp', 'texture'),
 ('src/platform/godot_bridge/particle_billboard_renderer.cpp', '~ParticleBillboardRenderer'),
 ('src/port/assets/vehicle_geometry.cpp', 'decode_vehicle_position_markers'),
 ('src/port/assets/vehicle_geometry.cpp', 'position_markers'),
 ('src/port/vehicle/model.hpp', 'marker_pivot'),
 ('src/port/vehicle/model.hpp', 'position_markers'),
}
STUB_REPLACEMENTS = set()
CHANGED_AUXILIARY = {
 'CMakeLists.txt',
 'reverse/scripts/audit_task0085_review.py',
 'reverse/tests/test_task0085_source_review.py',
 'src/game_core/port/driving/session.hpp',
 'src/platform/godot_bridge/ps2_driving_runtime.cpp',
 'src/platform/godot_bridge/ps2_vehicle_renderer.cpp',
 'src/port/vehicle/model.hpp',
}
ADDED_AUXILIARY = {
 'godot/scripts/runtime/race_scene_smoke.gd',
 'godot/scripts/debug/vehicle_flares_smoke.gd',
 'reverse/scripts/audit_task0086_review.py',
 'reverse/task0086/source-review.md',
 'reverse/tests/test_task0086_source_review.py',
 'src/game_core/port/driving/vehicle_flares.cpp',
 'src/game_core/port/driving/vehicle_flares.hpp',
 'src/platform/godot_bridge/particle_billboard_renderer.cpp',
 'src/platform/godot_bridge/particle_billboard_renderer.hpp',
 'src/port/assets/vehicle_geometry.hpp',
 'src/port/render/vehicle.hpp',
 'src/tests/vehicle_flare_tests.cpp',
 'src/tests/vehicle_marker_tests.cpp',
}
identities = prior.identities
transition = prior.transition
fingerprint = prior.fingerprint
require = prior.require


def historical_state():
    require(fingerprint(prior.REVIEW)==PREDECESSOR_SHA256,'Frozen TASK0085 review changed')
    old,frozen = prior.historical_state()
    review = json.loads(prior.REVIEW.read_text())
    before = transition.reviewed_identities(old,review)
    frozen.update({path:row['after'] for path,row in review['replacements'].items()})
    frozen.update(review['additions'])
    require(prior.check_current(current_identities=before,auxiliary_fingerprints=frozen)['passed'],
            'Historical TASK0085 snapshot failed')
    return before,frozen


def validate(before,current,frozen,review,auxiliary):
    require(review.get('task')=='TASK-0086' and review.get('evidence')=='E-0086-S01','Wrong task/evidence')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong Debug target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    for side,scope in [('before',REPLACED),('after',REPLACED|ADDED)]:
        rows = review[side]
        require(len(rows)==len(scope) and {(r['path'],r['name']) for r in rows}==scope,'Wrong function scope')
        for row in rows:
            require(row.get('evidence_id')=='E-0086-S01' and bool(row.get('reason')),'Missing function evidence')
            expected = side=='before' and (row['path'],row['name']) in STUB_REPLACEMENTS
            require(bool(row.get('stub'))==expected,'Unexpected stub transition')
    result = identities.check_identity_review(before,current,review)
    require(result['passed'],'Unreviewed function identity change')
    successor = transition.reviewed_identities(before,review)
    pins = {identities._identity_key(r):bool(r.get('stub')) for r in successor}
    require(all(pins.get(identities._identity_key(r))==bool(r.get('stub')) for r in current),'Changed live stub classification')
    replacements,additions = review['replacements'],review['additions']
    require(set(replacements)==CHANGED_AUXILIARY and set(additions)==ADDED_AUXILIARY,'Wrong auxiliary scope')
    require(not ADDED_AUXILIARY & frozen.keys(),'Added auxiliary already inherited')
    expected = dict(frozen)
    for path,row in replacements.items():
        require(row['before']==frozen[path],'Wrong auxiliary baseline '+path)
        expected[path] = row['after']
    expected.update(additions)
    require(auxiliary==expected,'Unreviewed auxiliary mutation or missing path')
    return dict(passed=True,task='TASK-0086',replaced_functions=len(REPLACED),added_functions=len(ADDED),
                modified_auxiliary=len(replacements),added_auxiliary=len(additions))


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0087_review import check_current as latest
        return latest()
    require(current_identities is not None,'Historical auxiliary snapshot requires explicit identities')
    before,frozen = historical_state()
    require(REVIEW.is_file(),'TASK0086 source is not yet settled/reviewed')
    review = json.loads(REVIEW.read_text())
    current = current_identities
    auxiliary = ({p:fingerprint(ROOT/p) for p in set(frozen)|ADDED_AUXILIARY}
                 if auxiliary_fingerprints is None else auxiliary_fingerprints)
    require('E-0086-S01' in (ROOT/'reverse/task0086/source-review.md').read_text(),'Missing review evidence')
    return validate(before,current,frozen,review,auxiliary)


def inspect_delta():
    before,frozen = historical_state()
    current = identities.source_identities(identities.historical.collect_current())
    old,new = (Counter(identities._identity_key(r) for r in rows) for rows in (before,current))
    return dict(before=[r for r in before if (old-new)[identities._identity_key(r)]],
                after=[r for r in current if (new-old)[identities._identity_key(r)]],
                auxiliary=sorted(p for p,h in frozen.items() if fingerprint(ROOT/p)!=h))


if __name__=='__main__':
    print(json.dumps(check_current(),indent=2))

"""Strict TASK0089 water effects successor; frozen predecessor JSON is immutable."""
from collections import Counter
import json
import audit_task0088_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0089/source-review.json'
PREDECESSOR_SHA256 = '583208d733c67735396dc33277e9445be9eb51755084374f84742885e9303123'
REPLACED = {
 ('src/game_core/port/driving/car_effects.cpp', 'WheelParticleEffects::prepare'),
 ('src/game_core/port/driving/car_effects.cpp', 'WheelParticleEffects::trigger'),
 ('src/game_core/port/driving/native_backend.cpp', 'NativeBackend::advance'),
 ('src/game_core/port/driving/native_backend.cpp', 'NativeBackend::reset'),
 ('src/game_core/src/care.cpp', 'Car::TriggerEffect'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::_bind_methods'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::emit_wheel_particles'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::get_driving_audio_bank'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::get_state'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::load_config'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::reset_audio'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::update_audio'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp', 'Ps2VehicleRenderer::apply_driving_snapshot'),
 ('src/port/assets/vehicle_geometry.cpp', 'decode_vehicle_geometry'),
 ('src/port/assets/vehicle_geometry.cpp', 'solid'),
 ('src/port/vehicle/model.cpp', 'Model::load'),
 ('src/port/vehicle/model.cpp', 'Model::select'),
}
ADDED = {
 ('src/game_core/indep/src/car.cpp', 'trigger_drive_over_effects'),
 ('src/game_core/port/driving/native_backend.cpp', 'NativeBackend::terrain_at_body'),
 ('src/game_core/port/driving/splash_audio.cpp', 'load_splash_sound_bank'),
 ('src/game_core/port/driving/water_effects.cpp', 'SplashAudioMixer::update'),
 ('src/game_core/port/driving/water_effects.cpp', 'trigger_water_terrain_state'),
 ('src/game_core/port/driving/water_effects.cpp', 'turn_wipers_on_for_duration'),
 ('src/game_core/port/driving/water_effects.cpp', 'wiper_snapshot'),
 ('src/game_core/port/driving/water_effects.hpp', 'reset'),
 ('src/game_core/port/driving/water_effects.hpp', 'trigger'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::take_splash_audio_events'),
 ('src/port/assets/vehicle_geometry.cpp', 'rigid_animation'),
 ('src/port/vehicle/model.cpp', 'Model::apply_wiper_state'),
}
STUB_REPLACEMENTS = set()
CHANGED_AUXILIARY = {
 'CMakeLists.txt',
 'godot/scripts/runtime/collision_audio_transport_smoke.gd',
 'godot/scripts/runtime/driving_audio.gd',
 'reverse/scripts/audit_task0088_review.py',
 'reverse/tests/test_task0088_source_review.py',
 'src/game_core/port/driving/car_effects.hpp',
 'src/game_core/port/driving/native_backend.hpp',
 'src/game_core/port/driving/session.hpp',
 'src/game_core/port/driving/terrain_effects.hpp',
 'src/platform/godot_bridge/ps2_driving_runtime.cpp',
 'src/platform/godot_bridge/ps2_driving_runtime.hpp',
 'src/platform/godot_bridge/ps2_vehicle_renderer.cpp',
 'src/port/render/vehicle.hpp',
 'src/port/vehicle/model.hpp',
 'src/tests/driving_particle_tests.cpp',
 'src/tests/gpu_particle_emission_tests.cpp',
}
ADDED_AUXILIARY = {
 'godot/scripts/debug/water_effects_smoke.gd',
 'reverse/scripts/audit_task0089_review.py',
 'reverse/task0089/source-review.md',
 'reverse/tests/test_task0089_source_review.py',
 'src/game_core/port/driving/splash_audio.cpp',
 'src/game_core/port/driving/splash_audio.hpp',
 'src/game_core/port/driving/water_effects.cpp',
 'src/game_core/port/driving/water_effects.hpp',
 'src/tests/vehicle_wiper_tests.cpp',
 'src/tests/water_effects_tests.cpp',
}
identities = prior.identities
transition = prior.transition
fingerprint = prior.fingerprint
require = prior.require


def historical_state():
    require(fingerprint(prior.REVIEW)==PREDECESSOR_SHA256,'Frozen TASK0088 review changed')
    old,frozen = prior.historical_state()
    review = json.loads(prior.REVIEW.read_text())
    before = transition.reviewed_identities(old,review)
    successor_auxiliary = dict(frozen)
    successor_auxiliary.update({path:row['after'] for path,row in review['replacements'].items()})
    successor_auxiliary.update(review['additions'])
    # Reuse the identical immutable snapshots already computed above. Calling
    # prior.check_current here would recompute prior.historical_state recursively.
    require('E-0088-S01' in (ROOT/'reverse/task0088/source-review.md').read_text(),'Missing predecessor review evidence')
    require(prior.validate(old,before,frozen,review,successor_auxiliary)['passed'],
            'Historical TASK0088 snapshot failed')
    return before,successor_auxiliary


def validate(before,current,frozen,review,auxiliary):
    require(review.get('task')=='TASK-0089' and review.get('evidence')=='E-0089-S01','Wrong task/evidence')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong Debug target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    for side,scope in [('before',REPLACED),('after',REPLACED|ADDED)]:
        rows = review[side]
        require(len(rows)==len(scope) and {(r['path'],r['name']) for r in rows}==scope,'Wrong function scope')
        for row in rows:
            require(row.get('evidence_id')=='E-0089-S01' and bool(row.get('reason')),'Missing function evidence')
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
    return dict(passed=True,task='TASK-0089',replaced_functions=len(REPLACED),added_functions=len(ADDED),
                modified_auxiliary=len(replacements),added_auxiliary=len(additions))


def check_current():
    before,frozen = historical_state()
    require(REVIEW.is_file(),'TASK0089 source is not yet settled/reviewed')
    review = json.loads(REVIEW.read_text())
    current = identities.source_identities(identities.historical.collect_current())
    auxiliary = {p:fingerprint(ROOT/p) for p in set(frozen)|ADDED_AUXILIARY}
    require('E-0089-S01' in (ROOT/'reverse/task0089/source-review.md').read_text(),'Missing review evidence')
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

"""Strict TASK0082 vehicle feedback successor; frozen predecessor JSON is immutable."""
from collections import Counter
import json
import audit_task0081_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0082/source-review.json'
PREDECESSOR_SHA256 = 'bd32ad9e6e044071ea0074f4bc8491f19df42e13edb39343b7030fa9931ed6f0'
REPLACED = {
 ('src/game_core/indep/src/realdriver.cpp','RealDriver::StartHonkHorn'),
 ('src/game_core/indep/src/realdriver.cpp','RealDriver::StopHonkHorn'),
 ('src/game_core/port/driving/native_backend.cpp','NativeBackend::initialize'),
 ('src/game_core/port/driving/native_backend.cpp','NativeBackend::advance'),
 ('src/game_core/port/driving/native_backend.cpp','NativeBackend::publish'),
 ('src/game_core/port/driving/native_backend.cpp','NativeBackend::reset'),
 ('src/game_core/port/driving/track_environment.cpp','load_track_environment'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::_bind_methods'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::load_config'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::action'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::set_driving_input'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::set_track'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::tick'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::get_state'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::apply_driving_snapshot'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::get_visual_state'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::pump_load'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::rebuild_display'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::release_display'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::update_view'),
 ('src/port/vehicle/model.cpp','Model::load'),
}
ADDED = {
 ('src/game_core/indep/src/carsoundengine.cpp','EngineAudioMixer::update'),
 ('src/game_core/indep/src/realdriver.cpp','RealDriver::JoyHandler_HonkHorn'),
 ('src/game_core/indep/src/realdriver.cpp','set_horn_activation'),
 ('src/game_core/indep/src/realdriver.cpp','horn_activation'),
 ('src/game_core/src/care.cpp','vehicle_light_state'),
 ('src/port/vehicle/model.cpp','Model::driving_texture'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','vehicle_stream'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::reset_audio'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::update_audio'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::get_driving_audio_bank'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::get_driving_audio_state'),
 ('src/game_core/port/driving/vehicle_audio.hpp','reset'),
 ('src/game_core/port/driving/vehicle_audio.cpp','word'),
 ('src/game_core/port/driving/vehicle_audio.cpp','half'),
 ('src/game_core/port/driving/vehicle_audio.cpp','scalar'),
 ('src/game_core/port/driving/vehicle_audio.cpp','text'),
 ('src/game_core/port/driving/vehicle_audio.cpp','upper'),
 ('src/game_core/port/driving/vehicle_audio.cpp','hashed_header'),
 ('src/game_core/port/driving/vehicle_audio.cpp','put_big'),
 ('src/game_core/port/driving/vehicle_audio.cpp','decode_patch'),
 ('src/game_core/port/driving/vehicle_audio.cpp','load_vehicle_sound_bank'),
 ('src/game_core/port/driving/vehicle_audio.cpp','vehicle_sample_pitch_scale'),
 ('src/game_core/port/driving/vehicle_audio.cpp','vehicle_sample_gain_scale'),
}
STUB_REPLACEMENTS = {
 ('src/game_core/indep/src/realdriver.cpp','RealDriver::StartHonkHorn'),
 ('src/game_core/indep/src/realdriver.cpp','RealDriver::StopHonkHorn'),
}
# These exact auxiliary sets are completed from the settled task, never inferred
# during live validation. A review cannot expand them by adding its own paths.
CHANGED_AUXILIARY = {
 'CMakeLists.txt',
 'reverse/scripts/audit_task0081_review.py',
 'reverse/tests/test_task0081_source_review.py',
 'godot/project.godot',
 'godot/scripts/runtime/race_scene.gd',
 'src/game_core/port/driving/native_backend.hpp',
 'src/platform/godot_bridge/ps2_driving_runtime.cpp',
 'src/platform/godot_bridge/ps2_driving_runtime.hpp',
 'src/platform/godot_bridge/ps2_vehicle_renderer.cpp',
 'src/tests/driving_backend_tests.cpp',
 'src/tests/driving_player_tests.cpp',
}
ADDED_AUXILIARY = {
 'reverse/scripts/audit_task0082_review.py',
 'reverse/tests/test_task0082_source_review.py',
 'reverse/task0082/source-review.md',
 'src/game_core/port/driving/horn.hpp',
 'src/game_core/port/driving/vehicle_lights.hpp',
 'src/game_core/port/driving/vehicle_audio.hpp',
 'godot/scripts/runtime/driving_audio.gd',
 'godot/scripts/runtime/driving_audio_smoke.gd',
 'godot/scripts/debug/driving_lab.gd',
 'godot/scripts/debug/driving_input.gd',
 'src/game_core/port/driving/session.hpp',
 'src/game_core/port/driving/track_environment.hpp',
 'src/game_core/indep/inc/RealDriver.hpp',
 'src/platform/godot_bridge/ps2_vehicle_renderer.hpp',
 'src/port/vehicle/model.hpp',
 'src/tests/vehicle_model_tests.cpp',
 'src/tests/driving_audio_tests.cpp',
}
identities = prior.identities
transition = prior.transition
fingerprint = prior.fingerprint
require = prior.require


def historical_state():
    require(fingerprint(prior.REVIEW)==PREDECESSOR_SHA256,'Frozen TASK0081 review changed')
    old,frozen = prior.historical_state()
    review = json.loads(prior.REVIEW.read_text())
    before = transition.reviewed_identities(old,review)
    frozen.update({path:row['after'] for path,row in review['replacements'].items()})
    frozen.update(review['additions'])
    require(prior.check_current(current_identities=before,auxiliary_fingerprints=frozen)['passed'],
            'Historical TASK0081 snapshot failed')
    return before,frozen


def validate(before,current,frozen,review,auxiliary):
    require(review.get('task')=='TASK-0082' and review.get('evidence')=='E-0082-S01','Wrong task/evidence')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong Debug target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    for side,scope in [('before',REPLACED),('after',REPLACED|ADDED)]:
        rows = review[side]
        require(len(rows)==len(scope) and {(r['path'],r['name']) for r in rows}==scope,'Wrong function scope')
        for row in rows:
            require(row.get('evidence_id')=='E-0082-S01' and bool(row.get('reason')),'Missing function evidence')
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
    return dict(passed=True,task='TASK-0082',replaced_functions=len(REPLACED),added_functions=len(ADDED),
                modified_auxiliary=len(replacements),added_auxiliary=len(additions))


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0083_review import check_current as latest
        return latest()
    require(current_identities is not None,'Historical auxiliary snapshot requires explicit identities')
    before,frozen = historical_state()
    require(REVIEW.is_file(),'TASK0082 source is not yet settled/reviewed')
    review = json.loads(REVIEW.read_text())
    current = current_identities
    auxiliary = ({p:fingerprint(ROOT/p) for p in set(frozen)|ADDED_AUXILIARY}
                 if auxiliary_fingerprints is None else auxiliary_fingerprints)
    require('E-0082-S01' in (ROOT/'reverse/task0082/source-review.md').read_text(),'Missing review evidence')
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

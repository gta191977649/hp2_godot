"""Strict TASK0083 collision/tire audio successor; frozen predecessor JSON is immutable."""
from collections import Counter
import json
import audit_task0082_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0083/source-review.json'
PREDECESSOR_SHA256 = '7bb5cd394d9b83f98c4c1c735997340ee18e84d4f15700119b398714b951abc7'
REPLACED = {
 ('src/game_core/port/driving/native_backend.cpp', 'NativeBackend::advance'),
 ('src/game_core/port/driving/native_backend.cpp', 'NativeBackend::reset'),
 ('src/game_core/port/driving/static_collisions.cpp', 'StaticCollisions::clear'),
 ('src/game_core/port/driving/static_collisions.cpp', 'StaticCollisions::contacts'),
 ('src/game_core/port/driving/static_collisions.cpp', 'StaticCollisions::process'),
 ('src/game_core/port/driving/static_collisions.cpp', 'StaticCollisions::resolve'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::_bind_methods'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::get_driving_audio_bank'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::get_driving_audio_state'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::load_config'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::reset_audio'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::update_audio'),
}
ADDED = {
 ('src/game_core/indep/src/carsoundtires.cpp', 'TireAudioMixer::update'),
 ('src/game_core/port/driving/collision_audio.cpp', 'CollisionSoundSelector::select'),
 ('src/game_core/port/driving/collision_audio.cpp', 'big'),
 ('src/game_core/port/driving/collision_audio.cpp', 'collision_scrape_fade_adjust'),
 ('src/game_core/port/driving/collision_audio.cpp', 'collision_scrape_fade_finished'),
 ('src/game_core/port/driving/collision_audio.cpp', 'collision_scrape_fade_step'),
 ('src/game_core/port/driving/collision_audio.cpp', 'collision_scrape_start'),
 ('src/game_core/port/driving/collision_audio.cpp', 'collision_scrape_update_volume'),
 ('src/game_core/port/driving/collision_audio.cpp', 'collision_sound_ready'),
 ('src/game_core/port/driving/collision_audio.cpp', 'collision_sound_request'),
 ('src/game_core/port/driving/collision_audio.cpp', 'collision_sound_volume'),
 ('src/game_core/port/driving/collision_audio.cpp', 'decode'),
 ('src/game_core/port/driving/collision_audio.cpp', 'half'),
 ('src/game_core/port/driving/collision_audio.cpp', 'load_collision_sound_bank'),
 ('src/game_core/port/driving/collision_audio.cpp', 'load_ingame_sound_samples'),
 ('src/game_core/port/driving/collision_audio.cpp', 'name'),
 ('src/game_core/port/driving/collision_audio.cpp', 'scrape_volume'),
 ('src/game_core/port/driving/collision_audio.cpp', 'word'),
 ('src/game_core/port/driving/collision_events.cpp', 'accumulate_collision_audio'),
 ('src/game_core/port/driving/collision_events.cpp', 'collision_audio_alive'),
 ('src/game_core/port/driving/collision_events.cpp', 'update_collision_audio'),
 ('src/game_core/port/driving/collision_events.cpp', 'update_collision_scrape'),
 ('src/game_core/port/driving/collision_sound_state.hpp', 'reset'),
 ('src/game_core/port/driving/tire_audio.cpp', 'load_tire_sound_bank'),
 ('src/game_core/port/driving/tire_audio_state.hpp', 'reset'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::take_collision_audio_events'),
}
STUB_REPLACEMENTS = set()
CHANGED_AUXILIARY = {
 'CMakeLists.txt',
 'godot/scripts/runtime/driving_audio.gd',
 'godot/scripts/runtime/driving_audio_smoke.gd',
 'reverse/scripts/audit_task0082_review.py',
 'reverse/tests/test_task0082_source_review.py',
 'src/game_core/port/driving/native_backend.hpp',
 'src/game_core/port/driving/session.hpp',
 'src/platform/godot_bridge/ps2_driving_runtime.cpp',
 'src/platform/godot_bridge/ps2_driving_runtime.hpp',
}
ADDED_AUXILIARY = {
 'godot/scripts/runtime/collision_audio_transport_smoke.gd',
 'reverse/scripts/audit_task0083_review.py',
 'reverse/task0083/source-review.md',
 'reverse/tests/test_task0083_source_review.py',
 'src/game_core/port/driving/collision_audio.hpp',
 'src/game_core/port/driving/collision_events.hpp',
 'src/game_core/port/driving/collision_sound_state.hpp',
 'src/game_core/port/driving/static_collisions.hpp',
 'src/game_core/port/driving/tire_audio.hpp',
 'src/game_core/port/driving/tire_audio_state.hpp',
 'src/tests/driving_collision_audio_tests.cpp',
 'src/tests/driving_collision_events_tests.cpp',
 'src/tests/driving_tire_audio_tests.cpp',
 'src/tests/static_collision_tests.cpp',
}
identities = prior.identities
transition = prior.transition
fingerprint = prior.fingerprint
require = prior.require


def historical_state():
    require(fingerprint(prior.REVIEW)==PREDECESSOR_SHA256,'Frozen TASK0082 review changed')
    old,frozen = prior.historical_state()
    review = json.loads(prior.REVIEW.read_text())
    before = transition.reviewed_identities(old,review)
    frozen.update({path:row['after'] for path,row in review['replacements'].items()})
    frozen.update(review['additions'])
    require(prior.check_current(current_identities=before,auxiliary_fingerprints=frozen)['passed'],
            'Historical TASK0082 snapshot failed')
    return before,frozen


def validate(before,current,frozen,review,auxiliary):
    require(review.get('task')=='TASK-0083' and review.get('evidence')=='E-0083-S01','Wrong task/evidence')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong Debug target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    for side,scope in [('before',REPLACED),('after',REPLACED|ADDED)]:
        rows = review[side]
        require(len(rows)==len(scope) and {(r['path'],r['name']) for r in rows}==scope,'Wrong function scope')
        for row in rows:
            require(row.get('evidence_id')=='E-0083-S01' and bool(row.get('reason')),'Missing function evidence')
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
    return dict(passed=True,task='TASK-0083',replaced_functions=len(REPLACED),added_functions=len(ADDED),
                modified_auxiliary=len(replacements),added_auxiliary=len(additions))


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0084_review import check_current as latest
        return latest()
    require(current_identities is not None,'Historical auxiliary snapshot requires explicit identities')
    before,frozen = historical_state()
    require(REVIEW.is_file(),'TASK0083 source is not yet settled/reviewed')
    review = json.loads(REVIEW.read_text())
    current = current_identities
    auxiliary = ({p:fingerprint(ROOT/p) for p in set(frozen)|ADDED_AUXILIARY}
                 if auxiliary_fingerprints is None else auxiliary_fingerprints)
    require('E-0083-S01' in (ROOT/'reverse/task0083/source-review.md').read_text(),'Missing review evidence')
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

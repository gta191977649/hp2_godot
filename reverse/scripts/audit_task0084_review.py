"""Strict TASK0084 native camera successor; frozen predecessor JSON is immutable."""
from collections import Counter
import json
import audit_task0083_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0084/source-review.json'
PREDECESSOR_SHA256 = '42c543a80e59d31ff8101abff7ba51d198c5b41ddef8cb479d5054516a9fc688'
REPLACED = {
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::_bind_methods'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::action'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::load_config'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::set_track'),
}
ADDED = {
 ('src/game_core/indep/src/cameramover.cpp', 'DrivingCamera::update'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_add'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_cross'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_dot'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_inverse'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_length'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_normalize2'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_pitch_correction'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_scale'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_sub'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_test_segment'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_transform'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_translate'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_vector'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_wall_adjust'),
 ('src/game_core/indep/src/cameramover.cpp', 'camera_yaw_correction'),
 ('src/game_core/port/driving/camera.hpp', 'reset'),
 ('src/game_core/port/driving/native_backend.hpp', 'track_environment'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp', 'Ps2DrivingRuntime::update_driving_camera'),
}
STUB_REPLACEMENTS = set()
CHANGED_AUXILIARY = {
 'CMakeLists.txt',
 'godot/project.godot',
 'godot/scripts/debug/driving_input.gd',
 'godot/scripts/debug/driving_lab.gd',
 'reverse/scripts/audit_task0083_review.py',
 'reverse/tests/test_task0083_source_review.py',
 'src/game_core/port/driving/native_backend.hpp',
 'src/platform/godot_bridge/ps2_driving_runtime.cpp',
 'src/platform/godot_bridge/ps2_driving_runtime.hpp',
}
ADDED_AUXILIARY = {
 'godot/scripts/debug/driving_camera.gd',
 'godot/scripts/debug/driving_camera_smoke.gd',
 'godot/scripts/debug/driving_projection_checks.gd',
 'godot/scripts/debug/driving_smoke.gd',
 'reverse/scripts/audit_task0084_review.py',
 'reverse/task0084/source-review.md',
 'reverse/tests/test_task0084_source_review.py',
 'src/game_core/port/driving/camera.cpp',
 'src/game_core/port/driving/camera.hpp',
 'src/tests/driving_camera_tests.cpp',
}
identities = prior.identities
transition = prior.transition
fingerprint = prior.fingerprint
require = prior.require


def historical_state():
    require(fingerprint(prior.REVIEW)==PREDECESSOR_SHA256,'Frozen TASK0083 review changed')
    old,frozen = prior.historical_state()
    review = json.loads(prior.REVIEW.read_text())
    before = transition.reviewed_identities(old,review)
    frozen.update({path:row['after'] for path,row in review['replacements'].items()})
    frozen.update(review['additions'])
    require(prior.check_current(current_identities=before,auxiliary_fingerprints=frozen)['passed'],
            'Historical TASK0083 snapshot failed')
    return before,frozen


def validate(before,current,frozen,review,auxiliary):
    require(review.get('task')=='TASK-0084' and review.get('evidence')=='E-0084-S01','Wrong task/evidence')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong Debug target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    for side,scope in [('before',REPLACED),('after',REPLACED|ADDED)]:
        rows = review[side]
        require(len(rows)==len(scope) and {(r['path'],r['name']) for r in rows}==scope,'Wrong function scope')
        for row in rows:
            require(row.get('evidence_id')=='E-0084-S01' and bool(row.get('reason')),'Missing function evidence')
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
    return dict(passed=True,task='TASK-0084',replaced_functions=len(REPLACED),added_functions=len(ADDED),
                modified_auxiliary=len(replacements),added_auxiliary=len(additions))


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0085_review import check_current as latest
        return latest()
    require(current_identities is not None,'Historical auxiliary snapshot requires explicit identities')
    before,frozen = historical_state()
    require(REVIEW.is_file(),'TASK0084 source is not yet settled/reviewed')
    review = json.loads(REVIEW.read_text())
    current = current_identities
    auxiliary = ({p:fingerprint(ROOT/p) for p in set(frozen)|ADDED_AUXILIARY}
                 if auxiliary_fingerprints is None else auxiliary_fingerprints)
    require('E-0084-S01' in (ROOT/'reverse/task0084/source-review.md').read_text(),'Missing review evidence')
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

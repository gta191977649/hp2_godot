"""Strict TASK0087 Track12 gaps successor; frozen predecessor JSON is immutable."""
from collections import Counter
import json
import audit_task0086_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0087/source-review.json'
PREDECESSOR_SHA256 = '5eb78631b86cd079c7074730ebf2cec463eb8f988d520bfa6224ac50f54b9b40'
REPLACED = {
 ('src/game_core/port/driving/track_contact.cpp', 'debug_terrain_from_retail'),
 ('src/game_core/src/acidfxe.cpp', 'AcidEffectSystem::TriggerEmitter'),
 ('src/port/vehicle/model.cpp', 'Model::apply_driving_pose'),
 ('src/port/vehicle/model.cpp', 'Model::load'),
}
ADDED = {
 ('src/port/vehicle/model.hpp', 'wheel_blur_available'),
}
STUB_REPLACEMENTS = set()
CHANGED_AUXILIARY = {
 'CMakeLists.txt',
 'reverse/scripts/audit_task0086_review.py',
 'reverse/tests/test_task0086_source_review.py',
 'src/game_core/port/track/particles.hpp',
 'src/port/vehicle/model.hpp',
 'src/tests/static_collision_tests.cpp',
}
ADDED_AUXILIARY = {
 'godot/scripts/debug/track12_driving_gaps_smoke.gd',
 'reverse/scripts/audit_task0087_review.py',
 'reverse/task0087/source-review.md',
 'reverse/tests/test_task0087_source_review.py',
 'src/tests/driving_track_tests.cpp',
 'src/tests/track12_particle_tests.cpp',
 'src/tests/vehicle_blur_absence_tests.cpp',
 'src/tests/water_contact_tests.cpp',
}
identities = prior.identities
transition = prior.transition
fingerprint = prior.fingerprint
require = prior.require


def historical_state():
    require(fingerprint(prior.REVIEW)==PREDECESSOR_SHA256,'Frozen TASK0086 review changed')
    old,frozen = prior.historical_state()
    review = json.loads(prior.REVIEW.read_text())
    before = transition.reviewed_identities(old,review)
    successor_auxiliary = dict(frozen)
    successor_auxiliary.update({path:row['after'] for path,row in review['replacements'].items()})
    successor_auxiliary.update(review['additions'])
    # Reuse the identical immutable snapshots already computed above. Calling
    # prior.check_current here would recompute prior.historical_state recursively.
    require('E-0086-S01' in (ROOT/'reverse/task0086/source-review.md').read_text(),'Missing predecessor review evidence')
    require(prior.validate(old,before,frozen,review,successor_auxiliary)['passed'],
            'Historical TASK0086 snapshot failed')
    return before,successor_auxiliary


def validate(before,current,frozen,review,auxiliary):
    require(review.get('task')=='TASK-0087' and review.get('evidence')=='E-0087-S01','Wrong task/evidence')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong Debug target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    for side,scope in [('before',REPLACED),('after',REPLACED|ADDED)]:
        rows = review[side]
        require(len(rows)==len(scope) and {(r['path'],r['name']) for r in rows}==scope,'Wrong function scope')
        for row in rows:
            require(row.get('evidence_id')=='E-0087-S01' and bool(row.get('reason')),'Missing function evidence')
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
    return dict(passed=True,task='TASK-0087',replaced_functions=len(REPLACED),added_functions=len(ADDED),
                modified_auxiliary=len(replacements),added_auxiliary=len(additions))


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0088_review import check_current as latest
        return latest()
    require(current_identities is not None,'Historical auxiliary snapshot requires explicit identities')
    before,frozen = historical_state()
    require(REVIEW.is_file(),'TASK0087 source is not yet settled/reviewed')
    review = json.loads(REVIEW.read_text())
    current = current_identities
    auxiliary = ({p:fingerprint(ROOT/p) for p in set(frozen)|ADDED_AUXILIARY}
                 if auxiliary_fingerprints is None else auxiliary_fingerprints)
    require('E-0087-S01' in (ROOT/'reverse/task0087/source-review.md').read_text(),'Missing review evidence')
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

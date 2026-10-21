"""Strict TASK0081 lifecycle successor; frozen predecessor reviews are immutable."""
import json
import audit_task0079_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0081/source-review.json'
PREDECESSOR_SHA256 = '430cdcc93478a453dde8db3dcb443add442dd779ab8f49697b31e077c3104c43'
REPLACED = {('src/game_core/port/driving/native_backend.cpp','NativeBackend::initialize'),
            ('src/game_core/port/driving/native_backend.cpp','NativeBackend::advance')}
ADDED = {('src/game_core/indep/src/car.cpp','begin_movement_state'),
         ('src/game_core/indep/src/car.cpp','finish_movement_state'),
         ('src/game_core/indep/src/car.cpp','Car::BeginMovement'),
         ('src/game_core/indep/src/car.cpp','Car::FinishMovement'),
         ('src/game_core/indep/src/carstate.cpp','CarState::ResetCar')}
CHANGED_AUXILIARY = {'reverse/scripts/audit_task0079_review.py',
                     'reverse/tests/test_task0079_source_review.py',
                     'src/game_core/indep/inc/Car.hpp',
                     'src/game_core/indep/src/carstate.hpp',
                     'src/tests/driving_backend_tests.cpp'}
ADDED_AUXILIARY = {'reverse/scripts/audit_task0081_review.py',
                   'reverse/tests/test_task0081_source_review.py',
                   'reverse/task0081/source-review.md',
                   'reverse/task0081/lifecycle.md',
                   'src/game_core/port/driving/frame_lifecycle.hpp',
                   'src/tests/driving_player_tests.cpp'}
identities = prior.identities
transition = prior.prior.prior.transition
fingerprint = prior.fingerprint
require = prior.require


def historical_state():
    require(fingerprint(prior.REVIEW)==PREDECESSOR_SHA256,'Frozen TASK0079 review changed')
    old,frozen = prior.historical_state()
    review = json.loads(prior.REVIEW.read_text())
    before = transition.reviewed_identities(old,review)
    frozen.update({path:row['after'] for path,row in review['replacements'].items()})
    frozen.update(review['additions'])
    require(prior.check_current(current_identities=before,auxiliary_fingerprints=frozen)['passed'],
            'Historical TASK0079 snapshot failed')
    return before,frozen


def validate(before,current,frozen,review,auxiliary):
    require(review.get('task')=='TASK-0081' and review.get('evidence')=='E-0081-L01','Wrong task/evidence')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong Debug target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    for side,scope in [('before',REPLACED),('after',REPLACED|ADDED)]:
        rows = review[side]
        require(len(rows)==len(scope) and {(r['path'],r['name']) for r in rows}==scope,'Wrong function scope')
        for row in rows:
            require(row.get('evidence_id')=='E-0081-L01' and bool(row.get('reason')),'Missing function evidence')
            require(row.get('stub') is False,'Unexpected stub transition')
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
    return dict(passed=True,task='TASK-0081',replaced_functions=len(REPLACED),
                added_functions=len(ADDED),modified_auxiliary=len(replacements),added_auxiliary=len(additions))


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0082_review import check_current as latest
        return latest()
    require(current_identities is not None,'Historical auxiliary snapshot requires explicit identities')
    before,frozen = historical_state()
    review = json.loads(REVIEW.read_text())
    current = current_identities
    auxiliary = ({path:fingerprint(ROOT/path) for path in set(frozen)|ADDED_AUXILIARY}
                 if auxiliary_fingerprints is None else auxiliary_fingerprints)
    require('E-0081-L01' in (ROOT/'reverse/task0081/source-review.md').read_text(),'Missing review evidence')
    return validate(before,current,frozen,review,auxiliary)


if __name__=='__main__':
    print(json.dumps(check_current(),indent=2))

"""TASK0079 strict two-function driving successor; frozen reviews are immutable."""
import json
import audit_task0077_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0079/source-review.json'
PREDECESSOR_SHA256 = 'b7047efee6288382169b3026293872cbefb3a14e85f42a97261b363d99b409f2'
ALLOWED = {('src/game_core/indep/src/carstate.cpp', 'CarState::BeginTimestep'),
           ('src/game_core/port/driving/native_backend.cpp', 'NativeBackend::advance')}
CHANGED_AUXILIARY = {'reverse/scripts/audit_task0077_review.py'}
ADDED_AUXILIARY = {'reverse/scripts/audit_task0079_review.py',
                   'reverse/tests/test_task0079_source_review.py',
                   'reverse/task0079/source-review.md',
                   'reverse/task0079/driving-chain.md',
                   'src/game_core/indep/src/carstate.hpp',
                   'src/tests/driving_backend_tests.cpp'}
identities = prior.prior.identities
fingerprint = prior.prior.fingerprint
require = prior.prior.require


def historical_state():
    require(fingerprint(prior.REVIEW) == PREDECESSOR_SHA256, 'Frozen TASK0077 review changed')
    review76 = json.loads(prior.prior.REVIEW.read_text())
    before = prior.prior.transition.reviewed_identities(prior.prior.historical_identities(), review76)
    frozen = {row['path']: row['lf_sha256'] for row in review76['auxiliary_sources']}
    review77 = json.loads(prior.REVIEW.read_text())
    frozen.update({path: row['after'] for path, row in review77['replacements'].items()})
    frozen.update(review77['additions'])
    require(prior.check_current(current_identities=before, auxiliary_fingerprints=frozen)['passed'],
            'Historical TASK0077 snapshot failed')
    return before, frozen


def validate(before, current, frozen, review, auxiliary):
    require(review.get('task') == 'TASK-0079' and review.get('evidence') == 'E-0079-S01', 'Wrong task/evidence')
    require(review.get('target_binary_sha256') == identities.TARGET_SHA256, 'Wrong Debug target')
    require(review.get('predecessor_review_lf_sha256') == PREDECESSOR_SHA256, 'Wrong predecessor')
    require(review.get('baseline_inventory_sha256') == identities.inventory_digest(before), 'Wrong baseline')
    for side in ('before', 'after'):
        rows = review[side]
        require(len(rows) == len(ALLOWED) and {(row['path'], row['name']) for row in rows} == ALLOWED, 'Wrong function scope')
        for row in rows:
            require(row.get('evidence_id') == 'E-0079-S01' and bool(row.get('reason')), 'Missing function evidence')
            expected_stub = side == 'before' and row['name'] == 'CarState::BeginTimestep'
            require(bool(row.get('stub')) == expected_stub, 'Unexpected stub transition')
    result = identities.check_identity_review(before, current, review)
    require(result['passed'], 'Unreviewed function identity change')
    successor = prior.prior.transition.reviewed_identities(before, review)
    pins = {identities._identity_key(row): bool(row.get('stub')) for row in successor}
    require(all(pins.get(identities._identity_key(row)) == bool(row.get('stub')) for row in current), 'Changed live stub classification')
    expected = dict(frozen)
    replacements, additions = review['replacements'], review['additions']
    require(set(replacements) == CHANGED_AUXILIARY and set(additions) == ADDED_AUXILIARY, 'Wrong auxiliary scope')
    require(not ADDED_AUXILIARY & frozen.keys(), 'Added auxiliary already inherited')
    for path, row in replacements.items():
        require(row['before'] == frozen[path], 'Wrong auxiliary baseline ' + path)
        expected[path] = row['after']
    expected.update(additions)
    require(auxiliary == expected, 'Unreviewed auxiliary mutation or missing path')
    return dict(passed=True, task='TASK-0079', changed_functions=len(ALLOWED),
                modified_auxiliary=len(replacements), added_auxiliary=len(additions))


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0081_review import check_current as latest
        return latest()
    require(current_identities is not None,'Historical auxiliary snapshot requires explicit identities')
    before, frozen = historical_state()
    review = json.loads(REVIEW.read_text())
    current = current_identities
    auxiliary = ({path: fingerprint(ROOT / path) for path in set(frozen) | ADDED_AUXILIARY}
                 if auxiliary_fingerprints is None else auxiliary_fingerprints)
    require('E-0079-S01' in (ROOT / 'reverse/task0079/source-review.md').read_text(), 'Missing review evidence')
    return validate(before, current, frozen, review, auxiliary)


if __name__ == '__main__':
    print(json.dumps(check_current(), indent=2))

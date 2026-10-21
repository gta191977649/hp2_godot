"""Strict auxiliary-only P0 successor; all native identities remain unchanged."""
import argparse
import json
import audit_task0076_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0077/source-review.json'
PINS = {'review': '381652239705610ca3ee9cdb890e99f63fbde9080145d8d170b65610bbaa5aba',
        'scope': 'db3cbf17314b8d8e602579a76e2ed0404fd60ee53b9028e4b8a274b56a4a626a'}
CHANGED = {'godot/scripts/debug/profile_debug.gd', 'godot/scripts/runtime/track_performance.gd',
           'tools/run_track_costs.ps1', 'reverse/scripts/audit_task0076_review.py'}
ADDED = {'tools/task0077_baseline.py', 'reverse/tests/test_task0077_baseline.py',
         'reverse/tests/test_task0077_source_review.py', 'reverse/scripts/audit_task0077_review.py',
         'godot/scripts/debug/task0077_capture_boundary_smoke.gd', 'reverse/task0077/source-review.md'}


def baseline(current_identities=None):
    prior.require(prior.fingerprint(prior.REVIEW) == PINS['review'], 'Frozen TASK0076 review changed')
    prior.require(prior.fingerprint(prior.SCOPE) == PINS['scope'], 'Frozen TASK0076 scope changed')
    review = json.loads(prior.REVIEW.read_text())
    current = prior.identities.source_identities(prior.identities.historical.collect_current()) if current_identities is None else current_identities
    auxiliary = {row['path']: row['lf_sha256'] for row in review['auxiliary_sources']}
    # An explicit frozen auxiliary snapshot verifies the historical stage without
    # forwarding back to this live successor. Native source is always read live.
    result = prior.check_current(current_identities=current, auxiliary_fingerprints=auxiliary)
    prior.require(result['passed'], 'Native identities changed outside P0 auxiliary-only scope')
    return auxiliary


def check_auxiliary(frozen, review, fingerprints):
    prior.require(review.get('task') == 'TASK-0077' and review.get('evidence') == 'E-0077-P01', 'Wrong review/evidence')
    prior.require(review.get('predecessor') == PINS, 'Wrong predecessor pins')
    replacements = review.get('replacements', {})
    additions = review.get('additions', {})
    prior.require(set(replacements) == CHANGED and set(additions) == ADDED, 'Wrong auxiliary scope')
    prior.require(set(fingerprints) == set(frozen) | ADDED, 'Missing or extra live auxiliary paths')
    for path, old in frozen.items():
        if path in CHANGED:
            prior.require(replacements[path].get('before') == old, 'Wrong baseline ' + path)
            expected = replacements[path].get('after')
        else:
            expected = old
        prior.require(bool(expected) and fingerprints[path] == expected, 'Unreviewed auxiliary change ' + path)
    for path in ADDED:
        prior.require(bool(additions[path]) and fingerprints[path] == additions[path], 'Changed P0 tooling ' + path)


def check_current(*, current_identities=None, auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0079_review import check_current as latest
        return latest()
    prior.require(current_identities is not None, "Historical auxiliary snapshot requires explicit identities")
    frozen = baseline(current_identities)
    review = json.loads(REVIEW.read_text())
    check_auxiliary(frozen, review, {path: prior.fingerprint(ROOT / path) for path in set(frozen) | ADDED} if auxiliary_fingerprints is None else auxiliary_fingerprints)
    return dict(passed=True, task='TASK-0077', native_identity_changes=0,
                modified_auxiliary=len(CHANGED), added_auxiliary=len(ADDED))


def write_review():
    frozen = baseline()
    prior.require(CHANGED <= frozen.keys() and not ADDED & frozen.keys(), 'Invalid successor scope')
    review = dict(task='TASK-0077', evidence='E-0077-P01', predecessor=PINS,
                  replacements={path: dict(before=frozen[path], after=prior.fingerprint(ROOT / path)) for path in sorted(CHANGED)},
                  additions={path: prior.fingerprint(ROOT / path) for path in sorted(ADDED)})
    live = {path: prior.fingerprint(ROOT / path) for path in set(frozen) | ADDED}
    check_auxiliary(frozen, review, live) # Refuse every unrelated auxiliary mutation.
    REVIEW.write_text(json.dumps(review, indent=2) + '\n', encoding='utf-8')
    return check_current()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    print(json.dumps(write_review() if args.write else check_current(), indent=2))

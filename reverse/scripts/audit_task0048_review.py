"""TASK-0048 explicit successor. No baseline refresh or accept-current operation."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import audit_task0046_review as predecessor

ROOT = Path(__file__).resolve().parents[2]
BASELINE = ROOT / 'reverse/task0048/pre-source-identities.json'
REVIEW = ROOT / 'reverse/task0048/source-review.json'
ALIAS_REVIEW = ROOT / 'reverse/task0048/source-review-alias.json'
# Frozen identities derive from the immutable TASK0046 baseline and its reviewed delta,
# not a snapshot taken after TASK0048 implementation. Constants are recorded at review.
BASELINE_SHA256 = '0c782ccd13ae38018406c7ad9cdb9388a2c8dc3dbbb3a632dbe3721987587dc7'
PREDECESSOR_REVIEW_SHA256 = 'c93f5f8eedf077e0205c55eafcff6da98cdb1381b207539fbaa3896f885ebeab'
INITIAL_REVIEW_SHA256 = 'd156e2a9a349870ad8a4b2ccf802fb87c91b66cc23c635c32cc52f9d8384a1a0'

def reviewed_identities(before, review):
    """Derive a reviewed historical state; never read or accept current source here."""
    prior = predecessor.predecessor
    result = list(before)
    for permission in review.get('before', []):
        matches = [row for row in result if prior._identity_key(row) == prior._identity_key(permission)]
        if len(matches) != 1:
            raise ValueError('Initial TASK-0048 review cannot derive its frozen successor')
        result.remove(matches[0])
    result.extend({key: row[key] for key in prior.IDENTITY_KEYS + ('stub',)} for row in review.get('after', []))
    return result

def check_current(*, current_identities=None):
    if not BASELINE.exists() or not REVIEW.exists():
        raise ValueError('TASK-0048 frozen baseline and separate review must both exist')
    prior = predecessor.predecessor
    frozen = json.loads(BASELINE.read_text())
    review = json.loads(REVIEW.read_text())
    actual = hashlib.sha256(predecessor.REVIEW.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    if actual != PREDECESSOR_REVIEW_SHA256:
        raise ValueError('TASK-0046 review changed; TASK-0048 cannot refresh predecessor pins')
    if prior.inventory_digest(frozen['functions']) != BASELINE_SHA256:
        raise ValueError('Frozen pre-TASK-0048 inventory changed')
    for document in (frozen, review):
        if (document.get('task') != 'TASK-0048'
                or document.get('target_binary_sha256') != prior.TARGET_SHA256
                or document.get('predecessor_review_lf_sha256') != PREDECESSOR_REVIEW_SHA256):
            raise ValueError('TASK-0048 review has wrong identity or predecessor')
    if frozen.get('inventory_sha256') != BASELINE_SHA256 or review.get('baseline_inventory_sha256') != BASELINE_SHA256:
        raise ValueError('TASK-0048 review has wrong baseline identity')
    initial_fingerprint = hashlib.sha256(REVIEW.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    if initial_fingerprint != INITIAL_REVIEW_SHA256:
        raise ValueError('Initial TASK-0048 review changed; alias amendment cannot refresh its pins')
    if not ALIAS_REVIEW.exists():
        raise ValueError('TASK-0048 alias amendment must exist')
    alias = json.loads(ALIAS_REVIEW.read_text())
    if (alias.get('task') != 'TASK-0048'
            or alias.get('target_binary_sha256') != prior.TARGET_SHA256
            or alias.get('predecessor_source_review_lf_sha256') != INITIAL_REVIEW_SHA256):
        raise ValueError('TASK-0048 alias amendment has wrong identity or predecessor')
    historical = predecessor.check_current(current_identities=frozen['functions'])
    current = prior.source_identities(prior.historical.collect_current()) if current_identities is None else current_identities
    intermediate = reviewed_identities(frozen['functions'], review)
    amendment = prior.check_identity_review(frozen['functions'], intermediate, review)
    alias_amendment = prior.check_identity_review(intermediate, current, alias)
    return dict(passed=historical['passed'] and amendment['passed'] and alias_amendment['passed'],
        predecessors=historical, task0048_amendment=amendment, task0048_alias_amendment=alias_amendment)

if __name__ == '__main__':
    from audit_task0056_review import check_current as check_complete_chain
    result = check_complete_chain()
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result['passed'] else 1)

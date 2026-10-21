"""TASK-0046 strict successor; predecessor reviews are immutable, not refreshed."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import audit_task0044_migration as predecessor

ROOT = Path(__file__).resolve().parents[2]
BASELINE = ROOT / 'reverse/task0046/pre-source-identities.json'
REVIEW = ROOT / 'reverse/task0046/source-review.json'
BASELINE_SHA256 = '62943b398fab986f463a25257635023e667964d8cf5b6c68bfdebaa747ece356'
PREDECESSOR_REVIEW_SHA256 = '73a3301235943e5c07bbf37a7a8748f88b3c5d9278553830b2194a65d9fc1e75'

def check_current(*, current_identities=None):
    if not BASELINE.exists() or not REVIEW.exists():
        raise ValueError('TASK-0046 frozen baseline and separate review must both exist')
    frozen = json.loads(BASELINE.read_text())
    review = json.loads(REVIEW.read_text())
    actual = hashlib.sha256(predecessor.SUCCESSOR_REVIEW.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    if actual != PREDECESSOR_REVIEW_SHA256:
        raise ValueError('TASK-0045 review changed; TASK-0046 cannot refresh predecessor pins')
    if predecessor.inventory_digest(frozen['functions']) != BASELINE_SHA256:
        raise ValueError('Frozen pre-TASK-0046 inventory changed')
    for document in (frozen, review):
        if (document.get('task') != 'TASK-0046'
                or document.get('target_binary_sha256') != predecessor.TARGET_SHA256
                or document.get('predecessor_review_lf_sha256') != PREDECESSOR_REVIEW_SHA256):
            raise ValueError('TASK-0046 review has wrong identity or predecessor')
    if frozen.get('inventory_sha256') != BASELINE_SHA256 or review.get('baseline_inventory_sha256') != BASELINE_SHA256:
        raise ValueError('TASK-0046 review has wrong baseline identity')
    prior = predecessor.check_current(current_identities=frozen['functions'])
    current = (predecessor.source_identities(predecessor.historical.collect_current())
        if current_identities is None else current_identities)
    amendment = predecessor.check_identity_review(frozen['functions'], current, review)
    return dict(passed=prior['passed'] and amendment['passed'], predecessors=prior, task0046_amendment=amendment)

if __name__ == '__main__':
    from audit_task0056_review import check_current as check_complete_chain
    result = check_complete_chain()
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result['passed'] else 1)

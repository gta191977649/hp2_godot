"""TASK-0049 explicit display-only successor; historical reviews remain immutable."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import audit_task0048_review as predecessor

ROOT = Path(__file__).resolve().parents[2]
REVIEW = ROOT / 'reverse/task0049/source-review.json'
INITIAL_REVIEW_SHA256 = predecessor.INITIAL_REVIEW_SHA256
ALIAS_REVIEW_SHA256 = '6388d724875d826a24bf329d06b78246c2bb45290b1f89163914d12d7fd6be69'

def fingerprint(path):
    return hashlib.sha256(path.read_bytes().replace(b'\r\n', b'\n')).hexdigest()

def historical_identities():
    """Derive only from approved predecessor deltas, never from present source."""
    if fingerprint(predecessor.REVIEW) != INITIAL_REVIEW_SHA256:
        raise ValueError('Frozen TASK-0048 initial review changed')
    if fingerprint(predecessor.ALIAS_REVIEW) != ALIAS_REVIEW_SHA256:
        raise ValueError('Frozen TASK-0048 alias review changed')
    frozen = json.loads(predecessor.BASELINE.read_text())['functions']
    initial = predecessor.reviewed_identities(frozen, json.loads(predecessor.REVIEW.read_text()))
    final = predecessor.reviewed_identities(initial, json.loads(predecessor.ALIAS_REVIEW.read_text()))
    if not predecessor.check_current(current_identities=final)['passed']:
        raise ValueError('TASK-0048 approved historical state failed its predecessor chain')
    return final

def check_current(*, current_identities=None):
    before = historical_identities()
    identities = predecessor.predecessor.predecessor
    review = json.loads(REVIEW.read_text())
    if (review.get('task') != 'TASK-0049'
            or review.get('target_binary_sha256') != identities.TARGET_SHA256
            or review.get('predecessor_initial_review_lf_sha256') != INITIAL_REVIEW_SHA256
            or review.get('predecessor_alias_review_lf_sha256') != ALIAS_REVIEW_SHA256
            or review.get('baseline_inventory_sha256') != identities.inventory_digest(before)):
        raise ValueError('TASK-0049 review has wrong target, predecessor, or derived historical inventory')
    current = identities.source_identities(identities.historical.collect_current()) if current_identities is None else current_identities
    amendment = identities.check_identity_review(before, current, review)
    return dict(passed=amendment['passed'], task0048_historical_passed=True, task0049_amendment=amendment)

if __name__ == '__main__':
    from audit_task0051_review import check_current as check_complete_chain
    result = check_complete_chain()
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result['passed'] else 1)

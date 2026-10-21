"""TASK-0051 shared-runtime successor; immutable predecessor reviews are never refreshed."""
from __future__ import annotations
import json
from pathlib import Path
import audit_task0049_review as predecessor

ROOT=Path(__file__).resolve().parents[2]
REVIEW=ROOT/'reverse/task0051/source-review.json'
PREDECESSOR_REVIEW_SHA256='a43d436751eab677d66c69c9ae53c1f6e24d36e7bfd9433f7ed33a9f377ed131'

def historical_identities():
    if predecessor.fingerprint(predecessor.REVIEW)!=PREDECESSOR_REVIEW_SHA256:
        raise ValueError('Frozen TASK-0049 review changed')
    before=predecessor.historical_identities()
    review=json.loads(predecessor.REVIEW.read_text())
    final=predecessor.predecessor.reviewed_identities(before,review)
    if not predecessor.check_current(current_identities=final)['passed']:
        raise ValueError('TASK-0049 historical state failed its predecessor chain')
    return final

def check_current(*,current_identities=None):
    before=historical_identities()
    identities=predecessor.predecessor.predecessor.predecessor
    review=json.loads(REVIEW.read_text())
    if (review.get('task')!='TASK-0051'
            or review.get('target_binary_sha256')!=identities.TARGET_SHA256
            or review.get('predecessor_review_lf_sha256')!=PREDECESSOR_REVIEW_SHA256
            or review.get('baseline_inventory_sha256')!=identities.inventory_digest(before)):
        raise ValueError('TASK-0051 review has wrong target, predecessor or derived inventory')
    current=identities.source_identities(identities.historical.collect_current()) if current_identities is None else current_identities
    amendment=identities.check_identity_review(before,current,review)
    return dict(passed=amendment['passed'],task0049_historical_passed=True,task0051_amendment=amendment)

if __name__=='__main__':
    from audit_task0056_review import check_current as check_complete_chain
    result=check_complete_chain()
    print(json.dumps(result,indent=2))
    raise SystemExit(0 if result['passed'] else 1)

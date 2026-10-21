"""TASK-0044: explicit source-change review, separate from historical preservation.

The checked-in review pins both sides, including signature and body tokens. This
script deliberately has no --write/refresh/accept-current mode. A changed body is
an intentional reconstruction, never described as preserved or PS2 verified.
"""
from __future__ import annotations
import hashlib
from collections import Counter
import json
from pathlib import Path
import audit_task0043_preservation as historical
ROOT = Path(__file__).resolve().parents[2]
BASELINE = '8c7efde'
REVIEW = ROOT / 'reverse/task0044/source-migration-review.json'
SUCCESSOR_BASELINE = ROOT / 'reverse/task0045/pre-source-identities.json'
SUCCESSOR_REVIEW = ROOT / 'reverse/task0045/source-review.json'
# Frozen before TASK-0045 source edits. Only CRLF/LF normalization is permitted;
# formatting/content edits to the old review are not accepted as a refresh.
TASK0044_REVIEW_LF_SHA256 = '8d5ed24700c986fe51d464c56380a27a40fcc66bf2359e412b4a2f941cbbab1e'
TASK0045_BASELINE_SHA256 = '423f0738818d3be6467627a1185dfa9d8a05c1c800a79ef5675ea44512070ec9'
TARGET_SHA256 = '815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b'
IDENTITY_KEYS = ('path', 'name', 'signature_sha256', 'body_sha256')

def digest(values):
    return hashlib.sha256(' '.join(values).encode()).hexdigest()

def identity(row):
    return dict(path=row['path'], name=row['name'],
        signature_sha256=digest(row['signature_tokens']),
        body_sha256=digest(row['body_tokens']))

def check_review(before, after, review):
    errors=[];approved={}
    for side,rows in [('before',before),('after',after)]:
        remaining=list(rows);approved[side]=[]
        for permission in review.get(side,[]):
            if not permission.get('evidence_id') or not permission.get('reason'):
                errors.append(side+': missing evidence/reason');continue
            expected={key:permission[key] for key in ('path','name','signature_sha256','body_sha256')}
            matches=[row for row in remaining if identity(row)==expected]
            if len(matches)!=1:
                errors.append(side+': pinned review mismatch '+permission['path']+' '+permission['name']);continue
            remaining.remove(matches[0]);approved[side].append(matches[0])
        approved[side+'_remaining']=remaining
    report=historical.compare(approved['before_remaining'],approved['after_remaining'],{})
    report['review_errors']=errors
    report['reviewed_replaced_before']=len(approved['before'])
    report['reviewed_reconstructed_after']=len(approved['after'])
    report['passed']=report['passed'] and not errors
    return report

def inventory_digest(rows):
    return hashlib.sha256(json.dumps(rows, sort_keys=True, separators=(',', ':')).encode()).hexdigest()

def source_identities(rows):
    return [dict(identity(row), stub=row['stub']) for row in rows
            if not row['path'].startswith('src/tests/')]

def _identity_key(row):
    return tuple(row[key] for key in IDENTITY_KEYS)

def check_identity_review(before, after, review, *, historical_body_moves=False):
    """Check hash-only rows. Successor reviews preserve full identities strictly.

    Body-only matching is reserved for validating the frozen historical TASK-0044
    state under that checker's original body-multiset policy. New amendments may
    not silently move a function or change its signature while keeping its body.
    Unlike the old comparator, this admits no unreviewed new throwing stub.
    """
    errors = []
    remaining = {}
    counts = {}
    for side, rows in [('before', before), ('after', after)]:
        pending = list(rows)
        counts[side] = 0
        for permission in review.get(side, []):
            if (not permission.get('evidence_id') or not permission.get('reason')
                    or not all(permission.get(key) for key in IDENTITY_KEYS)):
                errors.append(side + ': missing identity/evidence/reason')
                continue
            expected = _identity_key(permission)
            matches = [row for row in pending if _identity_key(row) == expected]
            if len(matches) != 1:
                errors.append(side + ': pinned review mismatch ' + permission['path'] + ' ' + permission['name'])
                continue
            pending.remove(matches[0])
            counts[side] += 1
        remaining[side] = pending
    key = (lambda row: row['body_sha256']) if historical_body_moves else _identity_key
    before_counts = Counter(key(row) for row in remaining['before'])
    after_counts = Counter(key(row) for row in remaining['after'])
    missing = before_counts - after_counts
    added = after_counts - before_counts
    return dict(passed=not errors and not missing and not added,
        review_errors=errors, reviewed_replaced_before=counts['before'],
        reviewed_reconstructed_after=counts['after'],
        missing_or_changed=[dict(identity=identity, count=count) for identity, count in sorted(missing.items())],
        added_unreviewed=[dict(identity=identity, count=count) for identity, count in sorted(added.items())])

def check_review_chain(baseline, frozen, current, original_review, successor_review):
    """Retain every original pin, then check an independent strict amendment."""
    original = check_identity_review(baseline, frozen, original_review, historical_body_moves=True)
    successor = check_identity_review(frozen, current, successor_review)
    return dict(passed=original['passed'] and successor['passed'],
        task0044_frozen_review=original, task0045_amendment=successor)

def check_current(*, current_identities=None):
    review=json.loads(REVIEW.read_text())
    if review['baseline_commit']!=historical.git('rev-parse',BASELINE).strip():
        raise ValueError('Unexpected TASK-0044 baseline')
    if review['target_binary_sha256']!=TARGET_SHA256:
        raise ValueError('Unexpected TASK-0044 behavior target')
    baseline = historical.collect_revision(BASELINE)
    current = historical.collect_current() if current_identities is None else None
    if not SUCCESSOR_BASELINE.exists() or not SUCCESSOR_REVIEW.exists():
        raise ValueError('TASK-0045 frozen baseline and separate amendment must both exist')
    review_sha = hashlib.sha256(REVIEW.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    if review_sha != TASK0044_REVIEW_LF_SHA256:
        raise ValueError('TASK-0044 review changed; a successor must not refresh its pins')
    frozen = json.loads(SUCCESSOR_BASELINE.read_text())
    successor = json.loads(SUCCESSOR_REVIEW.read_text())
    if inventory_digest(frozen['functions']) != TASK0045_BASELINE_SHA256:
        raise ValueError('Frozen pre-TASK-0045 source inventory changed')
    if (frozen.get('task') != 'TASK-0045' or successor.get('task') != 'TASK-0045'
            or frozen.get('target_binary_sha256') != TARGET_SHA256
            or successor.get('target_binary_sha256') != TARGET_SHA256
            or frozen.get('inventory_sha256') != TASK0045_BASELINE_SHA256
            or successor.get('baseline_inventory_sha256') != TASK0045_BASELINE_SHA256
            or frozen.get('predecessor_review_lf_sha256') != TASK0044_REVIEW_LF_SHA256
            or successor.get('predecessor_review_lf_sha256') != TASK0044_REVIEW_LF_SHA256):
        raise ValueError('TASK-0045 amendment has the wrong target or predecessor')
    return check_review_chain(source_identities(baseline), frozen['functions'],
        source_identities(current) if current_identities is None else current_identities, review, successor)

if __name__=='__main__':
    from audit_task0056_review import check_current as check_complete_chain
    result=check_complete_chain();print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)

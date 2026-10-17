"""TASK-0046 cannot replace predecessor evidence or accept unreviewed drift."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'reverse/scripts'))
import audit_task0044_migration as prior
import audit_task0046_review as audit

class PlayerSourceReviewTests(unittest.TestCase):
    def fixture(self, directory):
        row = dict(path='src/game_core/synthetic.cpp', name='Example::Run',
            signature_sha256='a'*64, body_sha256='b'*64, stub=False)
        frozen = dict(task='TASK-0046', target_binary_sha256=prior.TARGET_SHA256,
            predecessor_review_lf_sha256=audit.PREDECESSOR_REVIEW_SHA256,
            functions=[row], inventory_sha256=prior.inventory_digest([row]))
        review = dict(task='TASK-0046', target_binary_sha256=prior.TARGET_SHA256,
            predecessor_review_lf_sha256=audit.PREDECESSOR_REVIEW_SHA256,
            baseline_inventory_sha256=frozen['inventory_sha256'], before=[], after=[])
        p = Path(directory); baseline = p/'baseline.json'; amendment = p/'review.json'
        baseline.write_text(json.dumps(frozen)); amendment.write_text(json.dumps(review))
        return row, frozen, review, baseline, amendment

    def test_strict_new_review_rejects_signature_and_body_drift(self):
        row = dict(path='src/game_core/synthetic.cpp', name='Example::Run',
            signature_sha256='a'*64, body_sha256='b'*64, stub=False)
        for key in ('path', 'name', 'signature_sha256', 'body_sha256'):
            changed = copy.deepcopy(row); changed[key] += 'x'
            self.assertFalse(prior.check_identity_review([row], [changed], {})['passed'])

    def test_baseline_and_predecessor_remain_required(self):
        with tempfile.TemporaryDirectory() as directory:
            _, _, _, baseline, review = self.fixture(directory)
            with patch.object(audit, 'BASELINE', baseline), patch.object(audit, 'REVIEW', review):
                review.unlink()
                with self.assertRaisesRegex(ValueError, 'must both exist'): audit.check_current()

    def test_frozen_identity_drift_fails_before_current_inventory(self):
        with tempfile.TemporaryDirectory() as directory:
            row, frozen, _, baseline, review = self.fixture(directory)
            expected = prior.inventory_digest([copy.deepcopy(row)])
            frozen['functions'][0]['body_sha256'] = 'c'*64
            baseline.write_text(json.dumps(frozen))
            with patch.object(audit, 'BASELINE', baseline), patch.object(audit, 'REVIEW', review), \
                 patch.object(audit, 'BASELINE_SHA256', expected):
                with self.assertRaisesRegex(ValueError, 'inventory changed'): audit.check_current()

    def test_predecessor_file_fingerprint_cannot_be_refreshed(self):
        with tempfile.TemporaryDirectory() as directory:
            _, _, _, baseline, review = self.fixture(directory)
            with patch.object(audit, 'BASELINE', baseline), patch.object(audit, 'REVIEW', review), \
                 patch.object(audit, 'PREDECESSOR_REVIEW_SHA256', '0'*64):
                with self.assertRaisesRegex(ValueError, 'TASK-0045 review changed'): audit.check_current()

if __name__ == '__main__': unittest.main()

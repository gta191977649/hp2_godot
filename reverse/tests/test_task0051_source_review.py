"""Shared runtime extraction remains bounded by exact historical identity reviews."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'reverse/scripts'))
import audit_task0051_review as audit

class RuntimeSourceReviewTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.before=audit.historical_identities()
        cls.review=json.loads(audit.REVIEW.read_text())
        cls.after=audit.predecessor.predecessor.reviewed_identities(cls.before,cls.review)

    def test_exact_historical_successor_passes(self):
        self.assertTrue(audit.check_current(current_identities=self.after)['passed'])

    def test_frozen_predecessor_fingerprint_cannot_refresh(self):
        with patch.object(audit,'PREDECESSOR_REVIEW_SHA256','0'*64):
            with self.assertRaisesRegex(ValueError,'TASK-0049 review changed'):audit.historical_identities()

    def test_body_signature_path_and_name_remain_exact(self):
        identities=audit.predecessor.predecessor.predecessor.predecessor
        for field in identities.IDENTITY_KEYS:
            after=copy.deepcopy(self.after);after[-1][field]+='x'
            self.assertFalse(audit.check_current(current_identities=after)['passed'],field)

    def test_unrelated_physics_cannot_change(self):
        after=copy.deepcopy(self.after)
        row=next(row for row in after if row['path'].endswith('/physicscar.cpp'))
        row['body_sha256']='0'*64
        self.assertFalse(audit.check_current(current_identities=after)['passed'])

    def test_unreviewed_new_stub_fails(self):
        after=copy.deepcopy(self.after);after.append(dict(after[-1],name='Unexpected::Stub',stub=True))
        self.assertFalse(audit.check_current(current_identities=after)['passed'])

    def test_removed_or_duplicated_current_body_fails(self):
        self.assertFalse(audit.check_current(current_identities=self.after[:-1])['passed'])
        self.assertFalse(audit.check_current(current_identities=self.after+[self.after[-1]])['passed'])

    def test_evidence_reason_and_predecessor_are_required(self):
        for field in ('target_binary_sha256','baseline_inventory_sha256','predecessor_review_lf_sha256','evidence_id','reason'):
            review=copy.deepcopy(self.review)
            if field in ('evidence_id','reason'):review['after'][0].pop(field)
            else:review[field]='0'*64
            with tempfile.TemporaryDirectory() as directory:
                path=Path(directory)/'review.json';path.write_text(json.dumps(review))
                with patch.object(audit,'REVIEW',path):
                    if field in ('evidence_id','reason'):
                        self.assertFalse(audit.check_current(current_identities=self.after)['passed'])
                    else:
                        with self.assertRaises(ValueError):audit.check_current(current_identities=self.after)

if __name__=='__main__':unittest.main()

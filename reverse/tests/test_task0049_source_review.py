"""Reject unreviewed successor drift without updating historical source pins."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'reverse/scripts'))
import audit_task0049_review as audit

class TrackDebugSourceReviewTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.before=audit.historical_identities()
        cls.review=json.loads(audit.REVIEW.read_text())
        cls.after=audit.predecessor.reviewed_identities(cls.before,cls.review)

    def test_exact_review_passes(self):
        self.assertTrue(audit.check_current(current_identities=self.after)['passed'])

    def test_predecessor_initial_pin_is_immutable(self):
        with patch.object(audit,'INITIAL_REVIEW_SHA256','0'*64):
            with self.assertRaisesRegex(ValueError,'initial review changed'):audit.historical_identities()

    def test_predecessor_alias_pin_is_immutable(self):
        with patch.object(audit,'ALIAS_REVIEW_SHA256','0'*64):
            with self.assertRaisesRegex(ValueError,'alias review changed'):audit.historical_identities()

    def test_every_identity_dimension_rejects_drift(self):
        identities=audit.predecessor.predecessor.predecessor
        for field in identities.IDENTITY_KEYS:
            after=copy.deepcopy(self.after);after[-1][field]+='x'
            self.assertFalse(audit.check_current(current_identities=after)['passed'],field)

    def test_additional_stub_is_rejected(self):
        after=copy.deepcopy(self.after);extra=dict(after[-1],name='Unexpected::Throw',stub=True);after.append(extra)
        self.assertFalse(audit.check_current(current_identities=after)['passed'])

    def test_unrelated_body_cannot_change(self):
        changed=copy.deepcopy(self.after)
        row=next(row for row in changed if row['path'].endswith('/native_backend.cpp'))
        row['body_sha256']='0'*64
        self.assertFalse(audit.check_current(current_identities=changed)['passed'])

    def test_review_metadata_and_evidence_are_required(self):
        for field in ('target_binary_sha256','baseline_inventory_sha256','predecessor_alias_review_lf_sha256','evidence_id','reason'):
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

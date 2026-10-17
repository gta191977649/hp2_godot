"""TASK0048 must retain historical pins and reject any unreviewed identity drift."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'reverse/scripts'))
import audit_task0044_migration as identities
import audit_task0048_review as audit

class TrackSourceReviewTests(unittest.TestCase):
    def row(self):
        return dict(path='src/game_core/example.cpp', name='Example::Run',
            signature_sha256='a'*64, body_sha256='b'*64, stub=False)

    def test_all_identity_dimensions_are_protected(self):
        for key in identities.IDENTITY_KEYS:
            row = self.row(); changed = copy.deepcopy(row); changed[key] += 'x'
            self.assertFalse(identities.check_identity_review([row], [changed], {})['passed'])

    def test_evidence_and_reason_cannot_be_omitted(self):
        row = self.row(); changed = copy.deepcopy(row); changed['body_sha256'] = 'c'*64
        for key in ('evidence_id', 'reason'):
            before = dict(row, evidence_id='E-0048-D03', reason='Explicit original placement review')
            after = dict(changed, evidence_id='E-0048-D03', reason='Explicit original placement review')
            after.pop(key)
            self.assertFalse(identities.check_identity_review([row], [changed], dict(before=[before], after=[after]))['passed'])

    def test_review_is_exact_not_name_only(self):
        row = self.row(); after = dict(row, body_sha256='c'*64)
        review = dict(before=[dict(row, evidence_id='E-0048-D03', reason='Before')],
            after=[dict(after, evidence_id='E-0048-D03', reason='After')])
        self.assertTrue(identities.check_identity_review([row], [after], review)['passed'])
        after['signature_sha256'] = 'd'*64
        self.assertFalse(identities.check_identity_review([row], [after], review)['passed'])

    def test_missing_frozen_documents_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            with patch.object(audit, 'BASELINE', Path(directory)/'missing.json'):
                with self.assertRaisesRegex(ValueError, 'must both exist'): audit.check_current()

    def test_predecessor_fingerprint_cannot_be_refreshed(self):
        with patch.object(audit, 'PREDECESSOR_REVIEW_SHA256', '0'*64):
            with self.assertRaisesRegex(ValueError, 'TASK-0046 review changed'): audit.check_current()

    def test_frozen_inventory_tamper_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'baseline.json'
            path.write_text(json.dumps(dict(functions=[self.row()])))
            with patch.object(audit, 'BASELINE', path):
                with self.assertRaisesRegex(ValueError, 'inventory changed'): audit.check_current()

    def test_initial_review_cannot_be_refreshed_by_alias_fix(self):
        with patch.object(audit, 'INITIAL_REVIEW_SHA256', '0'*64):
            with self.assertRaisesRegex(ValueError, 'Initial TASK-0048 review changed'): audit.check_current()

    def test_alias_review_requires_exact_predecessor(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'alias.json'
            value = json.loads(audit.ALIAS_REVIEW.read_text())
            value['predecessor_source_review_lf_sha256'] = '0'*64
            path.write_text(json.dumps(value))
            with patch.object(audit, 'ALIAS_REVIEW', path):
                with self.assertRaisesRegex(ValueError, 'alias amendment has wrong identity'): audit.check_current()

if __name__ == '__main__': unittest.main()

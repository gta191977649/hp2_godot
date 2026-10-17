"""TASK-0045 successor review must preserve predecessor pins and reject drift."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from contextlib import ExitStack
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'reverse/scripts'))
import audit_task0043_preservation as bodies
import audit_task0044_migration as audit

class SourceReviewChainTests(unittest.TestCase):
    def row(self,value,name='Run',path='src/game_core/fixture.cpp'):
        return bodies.extract(f'int Fixture::{name}(){{return {value};}}',path)[0]
    def permit(self,row):
        return dict(audit.identity(row),evidence_id='synthetic',reason='synthetic test only')
    def fixture(self):
        old,middle,new=self.row(1),self.row(2),self.row(3)
        original=dict(before=[self.permit(old)],after=[self.permit(middle)])
        successor=dict(before=[self.permit(middle)],after=[self.permit(new)])
        return old,middle,new,original,successor
    def check(self,old,middle,new,original,successor):
        return audit.check_review_chain(audit.source_identities([old]),audit.source_identities([middle]),
            audit.source_identities([new]),original,successor)
    def test_original_api_behavior_stays_unchanged(self):
        old,middle,new,original,successor=self.fixture()
        self.assertTrue(audit.check_review([old],[middle],original)['passed'])
        self.assertFalse(audit.check_review([old],[new],original)['passed'])
    def test_exact_successor_passes_without_refreshing_original(self):
        old,middle,new,original,successor=self.fixture();saved=copy.deepcopy(original)
        report=self.check(old,middle,new,original,successor)
        self.assertTrue(report['passed']);self.assertEqual(saved,original)
        self.assertEqual(1,report['task0045_amendment']['reviewed_replaced_before'])
    def test_refreshing_old_after_pin_is_rejected(self):
        old,middle,new,original,successor=self.fixture();original['after']=[self.permit(new)]
        self.assertFalse(self.check(old,middle,new,original,successor)['passed'])
    def test_unmatched_change_and_missing_replacement_are_rejected(self):
        old,middle,new,original,successor=self.fixture()
        self.assertFalse(self.check(old,middle,self.row(4),original,successor)['passed'])
        report=audit.check_identity_review(audit.source_identities([middle]),[],successor)
        self.assertFalse(report['passed'])
    def test_addition_needs_explicit_after_permission(self):
        row=self.row(5,'Added')
        self.assertFalse(audit.check_identity_review([],audit.source_identities([row]),{})['passed'])
        self.assertTrue(audit.check_identity_review([],audit.source_identities([row]),dict(after=[self.permit(row)]))['passed'])
    def test_new_throwing_stub_is_not_silently_accepted(self):
        row=bodies.extract('void Fixture::Missing(){hp2::not_implemented("0x00100000","synthetic");}','src/game_core/fixture.cpp')[0]
        self.assertTrue(row['stub'])
        self.assertFalse(audit.check_identity_review([],audit.source_identities([row]),{})['passed'])
    def test_same_body_signature_or_path_change_is_not_silent(self):
        old=self.row(1);changed=self.row(1,'Renamed')
        self.assertFalse(audit.check_identity_review(audit.source_identities([old]),audit.source_identities([changed]),{})['passed'])
        changed=self.row(1,path='src/game_core/moved.cpp')
        self.assertFalse(audit.check_identity_review(audit.source_identities([old]),audit.source_identities([changed]),{})['passed'])
    def test_missing_evidence_and_duplicate_permissions_are_rejected(self):
        row=self.row(1);permission=self.permit(row);permission['reason']=''
        self.assertFalse(audit.check_identity_review([],audit.source_identities([row]),dict(after=[permission]))['passed'])
        permission=self.permit(row)
        self.assertFalse(audit.check_identity_review([],audit.source_identities([row]),dict(after=[permission,permission]))['passed'])
    def test_exact_reviewed_removal_is_explicit(self):
        row=self.row(1)
        self.assertFalse(audit.check_identity_review(audit.source_identities([row]),[],{})['passed'])
        self.assertTrue(audit.check_identity_review(audit.source_identities([row]),[],dict(before=[self.permit(row)]))['passed'])
    def file_fixture(self,directory):
        old,middle,new,original,successor=self.fixture()
        original.update(baseline_commit='synthetic-commit',target_binary_sha256=audit.TARGET_SHA256)
        original_path=Path(directory)/'original.json';original_path.write_text(json.dumps(original),encoding='utf-8')
        original_hash=hashlib.sha256(original_path.read_bytes()).hexdigest()
        inventory=audit.source_identities([middle]);inventory_hash=audit.inventory_digest(inventory)
        frozen=dict(task='TASK-0045',target_binary_sha256=audit.TARGET_SHA256,functions=inventory,
            inventory_sha256=inventory_hash,predecessor_review_lf_sha256=original_hash)
        successor.update(task='TASK-0045',target_binary_sha256=audit.TARGET_SHA256,
            baseline_inventory_sha256=inventory_hash,predecessor_review_lf_sha256=original_hash)
        frozen_path=Path(directory)/'frozen.json';frozen_path.write_text(json.dumps(frozen),encoding='utf-8')
        successor_path=Path(directory)/'successor.json';successor_path.write_text(json.dumps(successor),encoding='utf-8')
        stack=ExitStack()
        for name,value in dict(REVIEW=original_path,SUCCESSOR_BASELINE=frozen_path,SUCCESSOR_REVIEW=successor_path,
                TASK0044_REVIEW_LF_SHA256=original_hash,TASK0045_BASELINE_SHA256=inventory_hash).items():
            stack.enter_context(patch.object(audit,name,value))
        stack.enter_context(patch.object(audit.historical,'git',return_value='synthetic-commit'))
        stack.enter_context(patch.object(audit.historical,'collect_revision',return_value=[old]))
        stack.enter_context(patch.object(audit.historical,'collect_current',return_value=[new]))
        return stack,original_path,frozen_path,successor_path
    def test_modified_predecessor_file_rejected_even_if_json_values_equal(self):
        with tempfile.TemporaryDirectory() as directory:
            stack,original,_,_=self.file_fixture(directory)
            with stack:
                self.assertTrue(audit.check_current()['passed'])
                original.write_text(original.read_text()+' ',encoding='utf-8')
                with self.assertRaisesRegex(ValueError,'TASK-0044 review changed'):audit.check_current()
    def test_deleting_both_successor_files_cannot_fall_back_to_old_check(self):
        with tempfile.TemporaryDirectory() as directory:
            stack,_,frozen,successor=self.file_fixture(directory)
            with stack:
                frozen.unlink();successor.unlink()
                with self.assertRaisesRegex(ValueError,'must both exist'):audit.check_current()
    def test_changed_frozen_inventory_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            stack,_,frozen,_=self.file_fixture(directory)
            with stack:
                value=json.loads(frozen.read_text());value['functions'][0]['body_sha256']='0'*64
                frozen.write_text(json.dumps(value),encoding='utf-8')
                with self.assertRaisesRegex(ValueError,'Frozen pre-TASK-0045'):audit.check_current()
    def test_missing_successor_cannot_disable_chain(self):
        with tempfile.TemporaryDirectory() as directory:
            stack,_,_,successor=self.file_fixture(directory)
            with stack:
                successor.unlink()
                with self.assertRaisesRegex(ValueError,'must both exist'):audit.check_current()
if __name__=='__main__':unittest.main()

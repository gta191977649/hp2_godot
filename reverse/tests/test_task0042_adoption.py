"""Reject unsupported naming and preserve independent later evidence sections."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from adopt_task0042_names import merge_rows, upsert_section, SHA, DEBUG


class Task0042AdoptionTests(unittest.TestCase):
    def fixture(self):
        return dict(retail_sha256=SHA,debug_sha256=DEBUG,bridges=[dict(
            retail_address='0x00100000',debug_address='0x00100010',reference_name='ExactName(void)',
            mangled_name='ExactName__Fv',status='supported_test_fixture',evidence_id='E-test',
            live_symbol_matches_elf=True)])

    def test_unknown_candidate_is_not_adopted(self):
        payload=self.fixture();payload['bridges'][0]['status']='unresolved_candidate'
        self.assertEqual(merge_rows([],[payload]),([],[]))

    def test_conflicting_reference_names_fail(self):
        payload=self.fixture();baseline=merge_rows([],[payload])[0]
        payload['bridges'][0]['reference_name']='Conflicting(void)'
        with self.assertRaisesRegex(ValueError,'Conflicting correspondence'):
            merge_rows(baseline,[payload])

    def test_missing_original_symbol_confirmation_fails(self):
        payload=self.fixture();payload['bridges'][0]['live_symbol_matches_elf']=False
        with self.assertRaisesRegex(ValueError,'Missing live original symbol'):
            merge_rows([],[payload])

    def test_upsert_preserves_later_independent_sections(self):
        original='# Record\n\n## TASK-0042 names\n\nOld.\n\n## Later evidence\n\nMust survive.\n'
        updated=upsert_section(original,'TASK-0042 names','New.')
        self.assertIn('## Later evidence\n\nMust survive.',updated)
        self.assertNotIn('Old.',updated)
        self.assertEqual(updated,upsert_section(updated,'TASK-0042 names','New.'))
        self.assertFalse(updated.endswith('\n\n'))


if __name__=='__main__':unittest.main()

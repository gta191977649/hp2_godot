"""Synthetic topology tests, not PS2 instruction/behavior equivalence tests."""
import struct
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from audit_police_flow import audit

class FlowTests(unittest.TestCase):
    def run_audit(self,words):
        data=struct.pack('<'+'I'*len(words),*words)
        sections=[dict(name='.text',address=0x1000,offset=0,size=len(data))]
        return audit(0x1000,0x1000+len(data),data,sections)
    def test_return_includes_delay_slot(self):
        result=self.run_audit([0x03e00008,0x27bd0020])
        self.assertTrue(result['complete_static_flow'])
        self.assertEqual(result['highest_reached'],'00001004')
        self.assertEqual(result['returns'][0]['delay_word'],'27bd0020')
    def test_both_branch_paths_are_walked(self):
        result=self.run_audit([0x14430003,0,0x03e00008,0,0x03e00008,0])
        self.assertTrue(result['complete_static_flow'])
        self.assertEqual(len(result['returns']),2)
        self.assertEqual(result['reached_words'],6)
    def test_indirect_and_external_jumps_block_completeness(self):
        indirect=self.run_audit([0x00800008,0])
        self.assertFalse(indirect['complete_static_flow'])
        external=self.run_audit([0x08000800,0])
        self.assertFalse(external['complete_static_flow'])
        self.assertEqual(external['unresolved'][0]['address'],'00002000')

if __name__=='__main__':unittest.main()

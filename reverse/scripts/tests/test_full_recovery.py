"""Synthetic resumable-export safety tests; never query a real binary/program."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import export_full_recovery as export

class RecoveryTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.directory=Path(self.tmp.name)
        self.row=dict(entry_point='00001000',start_address='00001000',end_address='0000100f',name='FUN_00001000')
        self.root=patch.object(export,'OUT',self.directory);self.root.start()
    def tearDown(self):
        self.root.stop();self.tmp.cleanup()
    def record(self):return json.loads((self.directory/'functions/00001000.json').read_text())
    def test_success_resume_and_stale_cache(self):
        with patch.object(export,'rpc',return_value=dict(decompiled_code='void f(void) {}',body_size=16)) as rpc:
            self.assertEqual(export.export_one(None,self.row),'pseudocode_exported')
            self.assertEqual(export.export_one(None,self.row),'pseudocode_exported')
            self.assertEqual(rpc.call_count,1)
            with self.assertRaises(ValueError):export.export_one(None,dict(self.row,name='ExternalChange'))
            self.assertEqual(rpc.call_count,1)
    def test_boundary_block_never_decompiles(self):
        with patch.object(export,'rpc') as rpc:
            self.assertEqual(export.export_one(None,dict(self.row,end_address='00001000')),'boundary_blocked')
            rpc.assert_not_called()
    def test_error_persisted_and_explicit_retry(self):
        with patch.object(export,'rpc',side_effect=RuntimeError('synthetic disconnect')) as rpc:
            self.assertEqual(export.export_one(None,self.row),'decompile_failed')
            self.assertIn('synthetic disconnect',self.record()['error'])
            export.export_one(None,self.row)
            self.assertEqual(rpc.call_count,1)
        with patch.object(export,'rpc',return_value=dict(decompiled_code='void f(void) {}',body_size=16)):
            self.assertEqual(export.export_one(None,self.row,retry=True),'pseudocode_exported')
    def test_empty_code_is_failure_not_success(self):
        with patch.object(export,'rpc',return_value=dict(decompiled_code=' ')):
            self.assertEqual(export.export_one(None,self.row),'decompile_failed')
            self.assertFalse((self.directory/'pseudocode/00001000.c').exists())
    def test_wrong_binary_cache_rejected(self):
        with patch.object(export,'rpc',return_value=dict(decompiled_code='void f(void) {}',body_size=16)):
            export.export_one(None,self.row)
        row=self.record();row['elf_sha256']='different';export.atomic(self.directory/'functions/00001000.json',row)
        with patch.object(export,'rpc') as rpc:
            with self.assertRaises(ValueError):export.export_one(None,self.row)
            rpc.assert_not_called()

if __name__=='__main__':unittest.main()

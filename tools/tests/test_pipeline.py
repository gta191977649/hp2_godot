import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from tools import pipeline as p

class Contracts(unittest.TestCase):
    def test_integer_and_float_differences(self):
        self.assertTrue(p.compare({'x': 1}, {'x': 2}, {'x': {'kind': 'integer'}}))
        self.assertTrue(p.compare({'x': 1}, {'x': 1.0}, {'x': {'kind': 'integer'}}))
        self.assertFalse(p.compare({'x': 1.0}, {'x': 1.001}, {'x': {'kind': 'float', 'absolute': .002}}))
        self.assertTrue(p.compare({'x': 1.0}, {'x': float('nan')}, {'x': {'kind': 'float'}}))

    def test_fixture_is_not_changed_or_verified(self):
        path = p.ROOT / 'tools/fixtures/demo.json'
        before = path.read_bytes()
        result = p.verify(None, actual={'frame': 4, 'position': 2})
        self.assertEqual(result['result'], 'FAIL')
        self.assertEqual(before, path.read_bytes())
        self.assertEqual(p.verify(None, actual={'frame': 4, 'position': 1})['verification_status'], 'tested_synthetic')

    def test_proposal_scope_and_rollback(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ('tools/fixtures/demo.json', 'src/tests/test.cpp', '../escape.cpp'):
                with self.assertRaises(ValueError): p.apply_proposal(root, {name: 'malicious'})
            path = root / 'src/game_core/demo.cpp'
            path.parent.mkdir(parents=True)
            path.write_text('original')
            saved = p.apply_proposal(root, {'src/game_core/demo.cpp': 'candidate'})
            self.assertEqual(path.read_text(), 'candidate')
            p.rollback(saved)
            self.assertEqual(path.read_text(), 'original')

    def test_retry_limit(self):
        with self.assertRaises(ValueError): p.controller(0)

    def test_failed_attempts_are_bounded_and_persisted(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(p, 'LOGS', Path(directory)), patch.object(p, 'build', side_effect=RuntimeError('compile failed')) as build:
            result = p.controller(2)
            self.assertEqual(build.call_count, 2)
            self.assertEqual(result['tasks'][0]['status'], 'failed')
            self.assertTrue((Path(directory) / 'synthetic-demo-attempt-2.json').exists())
            self.assertEqual(json.loads((Path(directory) / 'queue-state.json').read_text())['tasks'][0]['attempts'], 2)

    def test_dependency_blocks_without_build(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(p, 'LOGS', Path(directory)), patch.object(p, 'build') as build:
            p.write_json(Path(directory) / 'queue-state.json', {'tasks': [{'id': 'child', 'dependencies': ['missing'], 'status': 'not_tested'}]})
            self.assertEqual(p.controller(2)['tasks'][0]['status'], 'blocked')
            build.assert_not_called()

    def test_timeout_is_reported(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(p, 'LOGS', Path(directory)):
            with self.assertRaises(RuntimeError):
                p.run([p.sys.executable, '-c', 'import time; time.sleep(1)'], 'timeout', .01)
            self.assertEqual(json.loads((Path(directory) / 'timeout.json').read_text())['exit_code'], 124)

if __name__ == '__main__': unittest.main()

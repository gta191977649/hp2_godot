"""TASK-0042: finite evidence roots and conservative table-store tracking."""
import csv
import importlib.util
import json
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'reverse/scripts'))
import trace_debug_candidate_calls as candidate


class DebugCallchainEvidenceTests(unittest.TestCase):
    def test_table_store_requires_constant_and_this_alias(self):
        # addiu s0,a0,8; lui t0,0x2b; ori t0,t0,0x1230; sw t0,4(s0)
        words = [0x24900008, 0x3C08002B, 0x35081230, 0xAE080004]
        self.assertEqual(candidate.table_stores(struct.pack('<4I', *words)),
                         [(12, 0x2B1230, 12)])
        # A store through an unknown base is not constructor/vtable evidence.
        words[-1] = 0xAD280004
        self.assertEqual(candidate.table_stores(struct.pack('<4I', *words)), [])

    def test_call_clobbers_volatile_constants_after_delay_slot(self):
        words = [0x3C08002B, 0x35081230, 0x0C001000, 0, 0xAC880008]
        self.assertEqual(candidate.table_stores(struct.pack('<5I', *words)), [])

    def test_scoped_dispositions_are_complete_and_disjoint(self):
        def rows(name):
            with (ROOT/'reverse/naming'/name).open(newline='') as handle:
                return list(csv.DictReader(handle))
        c = rows('task0042-candidate-dispositions.csv')
        u = rows('task0042-unresolved-dispositions.csv')
        self.assertEqual((len(c), len(u)), (175, 196))
        self.assertEqual(sum(x['status'].startswith('supported') for x in c), 64)
        self.assertEqual(sum(x['status'].startswith('supported') for x in u), 21)
        key = 'retail_address'
        self.assertFalse({x[key] for x in c} & {x[key] for x in u})

    def test_rooted_candidate_chains_do_not_prove_each_other_in_a_cycle(self):
        bridges = json.loads((ROOT/'reverse/naming/task0042-candidate-bridges.json').read_text())['bridges']
        supported = {x['evidence_id']: x for x in bridges if x['status'].startswith('supported')}
        def visit(evidence, stack):
            self.assertNotIn(evidence, stack, 'circular caller evidence is not an independent root')
            item = supported[evidence]
            for parent in item.get('independent_callers', []):
                ancestor = parent.get('caller_evidence_id', '')
                if ancestor.startswith('E-0042-C'):
                    self.assertIn(ancestor, supported)
                    visit(ancestor, stack | {evidence})
        for evidence in supported:
            visit(evidence, set())


if __name__ == '__main__':
    unittest.main()

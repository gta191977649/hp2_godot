"""TASK-0041 synthetic correspondence checks; no proprietary inputs required."""
import contextlib
import csv
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    "debug_reference_matcher", Path(__file__).with_name("match_debug_reference.py"))
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)


def ins(op, rs=0, rt=0, imm=0):
    return (op << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF)


def raw(*words):
    return struct.pack("<" + "I" * len(words), *words)


class NormalizationTests(unittest.TestCase):
    def normalized(self, *words):
        return matcher.normalize(raw(*words), 0x100000, 0x330000)

    def test_small_arithmetic_constants_remain_distinct(self):
        self.assertNotEqual(self.normalized(ins(9, 8, 8, 17))[0],
                            self.normalized(ins(9, 8, 8, 18))[0])

    def test_address_shaped_integer_collision_is_only_candidate_evidence(self):
        # Both values happen to lie inside the executable address envelope.
        # With no dereference or identified symbol use they may still be integers.
        # This deliberately demonstrates why normalized matches cannot authorize names.
        a = self.normalized(ins(15, rt=8, imm=0x12), ins(13, 8, 8, 0x3456))
        b = self.normalized(ins(15, rt=8, imm=0x12), ins(13, 8, 8, 0x3457))
        self.assertEqual(a[0], b[0])
        self.assertNotEqual(a[2], b[2])
        self.assertEqual(a[3], 2)

    def test_jump_relocation_retains_target_evidence(self):
        a = self.normalized((3 << 26) | (0x123400 >> 2), 0)
        b = self.normalized((3 << 26) | (0x234500 >> 2), 0)
        self.assertEqual(a[0], b[0])
        self.assertEqual(a[1], [(0, 0x123400)])
        self.assertEqual(b[1], [(0, 0x234500)])
        self.assertNotEqual(a[1], b[1])

    def test_gp_load_relocation_retains_effective_address(self):
        a = self.normalized(ins(35, 28, 8, -16))
        b = self.normalized(ins(35, 28, 8, 32))
        self.assertEqual(a[0], b[0])
        self.assertEqual(a[2], [(0, 0x32FFF0)])
        self.assertEqual(b[2], [(0, 0x330020)])

    def test_absolute_memory_relocation_retains_address(self):
        a = self.normalized(ins(15, rt=8, imm=0x12), ins(35, 8, 9, 0x3456))
        b = self.normalized(ins(15, rt=8, imm=0x23), ins(35, 8, 9, 0x4567))
        self.assertEqual(a[0], b[0])
        self.assertEqual(a[2], [(4, 0x123456)])
        self.assertEqual(b[2], [(4, 0x234567)])

    def test_load_overwrites_tracked_high_half(self):
        # ld t0, 0(t1) invalidates the old lui t0. The following lw uses
        # the loaded pointer, whose value cannot be inferred from this stream.
        result = self.normalized(ins(15, rt=8, imm=0x12),
                                 ins(55, 9, 8), ins(35, 8, 10, 0x3456))
        self.assertEqual(result[2], [])
        self.assertEqual(result[3], 0)

    def test_branch_offsets_remain_distinct(self):
        self.assertNotEqual(self.normalized(ins(4, 8, 9, 2))[0],
                            self.normalized(ins(4, 8, 9, 3))[0])


class DuplicateInventoryTests(unittest.TestCase):
    def run_inventory(self, reference_duplicate=False, debug_extra=False, retail_extra=False):
        """Run the real classifier against tiny synthetic inventories and text."""
        body = raw(ins(9, 0, 2, 42), *([0] * 13), 0x03E00008, 0)
        class FakeElf:
            def __init__(self, path):
                self.debug = Path(path).name == "debug.elf"
                self.sha = matcher.DEBUG_SHA if self.debug else matcher.RETAIL_SHA
                self.text = bytearray(b"\xff" * 0x40000)
                addresses = ([0x110000, 0x120000] if reference_duplicate else [0x110000]) if self.debug else [0x130000]
                for address in addresses:
                    self.text[address-0x100000:address-0x100000+len(body)] = body
                if debug_extra if self.debug else retail_extra:
                    self.text[0x38000:0x38000+len(body)] = body
                self.sections = {".text": (0, 1, 0, 0x100000, 0, len(self.text))}
                self.symbols = ([dict(address=a, size=len(body), kind=2,
                                      name="symbol_" + str(a), section=1)
                                 for a in addresses] if self.debug else [])

            def read(self, address, size):
                offset = address - 0x100000
                return bytes(self.text[offset:offset+size]) if 0 <= offset and offset+size <= len(self.text) else b""

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config").mkdir()
            (root / "config/ps2_elf.local.json").write_text('{"elf":"retail.elf"}')
            (root / "ps2_decomp").mkdir()
            (root / "ps2_decomp/ghidra_functions.csv").write_text(
                "Start,Size\n0x00130000,64\n")
            cache = root / "cache"
            cache.mkdir()
            (cache / "retail-memory-manifest.json").write_text(json.dumps(
                dict(retail_sha256=matcher.RETAIL_SHA, sections=[])))
            map_path = root / "debug.map"
            map_path.write_bytes(b"")
            output = root / "output"
            with patch.multiple(matcher, ROOT=root, OUT=output, CACHE=cache, Elf=FakeElf,
                                MAP_SHA=hashlib.sha256(b"").hexdigest()), patch(
                    "sys.argv", ["matcher", "--debug-elf", "debug.elf", "--map", str(map_path)]), contextlib.redirect_stdout(io.StringIO()):
                matcher.main()
            with (output / "task0041-debug-retail-matches.csv").open() as handle:
                matches = list(csv.DictReader(handle))
            with (output / "task0041-ambiguous-signatures.csv").open() as handle:
                ambiguous = list(csv.DictReader(handle))
            return matches, ambiguous

    def test_duplicate_reference_bodies_never_emit_supported_mapping(self):
        matches, ambiguous = self.run_inventory(reference_duplicate=True)
        self.assertEqual(matches, [])
        self.assertEqual(len(ambiguous), 1)
        self.assertEqual(ambiguous[0]["debug_count"], "2")
        self.assertEqual(ambiguous[0]["status"], "ambiguous_do_not_adopt")

    def test_unique_raw_leaf_is_supported(self):
        matches, ambiguous = self.run_inventory()
        self.assertEqual(ambiguous, [])
        self.assertEqual(len(matches), 1)
        self.assertEqual(matches[0]["status"], "supported_exact_leaf")

    def test_unindexed_debug_duplicate_prevents_supported_leaf(self):
        matches, _ = self.run_inventory(debug_extra=True)
        self.assertEqual(len(matches), 1)
        self.assertFalse(matches[0]["status"].startswith("supported_"))

    def test_unindexed_retail_duplicate_prevents_supported_leaf(self):
        matches, _ = self.run_inventory(retail_extra=True)
        self.assertEqual(len(matches), 1)
        self.assertFalse(matches[0]["status"].startswith("supported_"))


class AdoptionSafeguardTests(unittest.TestCase):
    def test_closed_leaf_rejects_external_branches_and_nonreturn_jumps(self):
        for word in (ins(4, 8, 9, 8), ins(1, 8, 1, 8),
                     ins(16, 8, 0, 8), ins(17, 8, 0, 8), ins(18, 8, 0, 8),
                     (8 << 21) | 8, (8 << 21) | (31 << 11) | 9):
            with self.subTest(word=hex(word)):
                self.assertFalse(matcher.closed_leaf(raw(word, 0)))
        self.assertTrue(matcher.closed_leaf(raw(0x03E00008, 0)))
        self.assertTrue(matcher.closed_leaf(raw(ins(4, 8, 9, 1), 0, 0x03E00008, 0)))

    def test_uniqueness_detects_overlapping_occurrences(self):
        self.assertFalse(matcher.unique_bytes(b"aaaaa", b"aaaa"))
        self.assertTrue(matcher.unique_bytes(b"xaaaay", b"aaaa"))
        self.assertFalse(matcher.unique_bytes(b"xyz", b"aaaa"))


if __name__ == "__main__":
    unittest.main()

"""Synthetic offline extraction regressions; no proprietary fixture data."""
import json
from pathlib import Path
import struct
import tempfile
import unittest

from tools.asset_extractor.zzdata_archive import decompress, extract, name_hash


class ArchiveTests(unittest.TestCase):
    def test_overlapping_backreference_and_length_validation(self):
        packed = struct.pack('<4sHHII', b'COMP', 0x1001, 0, 7, 21)
        packed += b'\x02\x00A\x03\x01'
        self.assertEqual(decompress(packed), b'A' * 7)
        with self.assertRaises(ValueError):
            decompress(packed[:8] + struct.pack('<I', 6) + packed[12:])
        with self.assertRaises(ValueError):
            decompress(packed[:-1] + b'\x08')

    def test_duplicate_records_preserved_and_existing_data_protected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, output, names = [root / s for s in ('src', 'out', 'names')]
            source.mkdir()
            names.mkdir()
            (names / 'A.BIN').touch()
            (source / 'ZZDATA.BIN').write_bytes(b'abc' + bytes(2045) + b'def')
            (source / 'ZDIR.BIN').write_bytes(
                struct.pack('<6I', name_hash('A.BIN'), 0, 3, name_hash('A.BIN'), 1, 3))
            extract(source, output, names)
            manifest = json.loads((output / 'extraction_manifest.json').read_text())
            self.assertEqual([e['output_size'] for e in manifest['entries']], [3, 3])
            self.assertEqual((output / 'A.BIN').read_bytes(), b'abc')
            self.assertEqual((output / '_DUPLICATES/0001/A.BIN').read_bytes(), b'def')
            with self.assertRaises(FileExistsError):
                extract(source, output, names)


if __name__ == '__main__':
    unittest.main()

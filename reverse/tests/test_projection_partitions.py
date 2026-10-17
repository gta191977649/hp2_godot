"""TASK-0042 projection migration: preserve bodies, helpers and overload addresses."""
import json
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'reverse/scripts'))
import reorganize_driving_projections as migration
import generate_class_templates as generator
import adopt_debug_reference_names as legacy


def current_ledger():
    path = ROOT / 'reverse/naming/task0043-projection-bindings.json'
    if not path.exists():
        path = ROOT / 'reverse/naming/task0042-projection-partitions.json'
    return json.loads(path.read_text())


class ProjectionPartitionTests(unittest.TestCase):
    def test_every_migrated_entry_preserves_all_tokens_except_documented_renames(self):
        ledger = current_ledger()
        parsed = {}
        for row in ledger['functions']:
            if row['source'] not in parsed:
                text = (ROOT / row['source']).read_text()
                parsed[row['source']] = (text, migration.functions(text)[0])
            text, functions = parsed[row['source']]
            matched = [function for function in functions
                       if function['scope'] == row['source_scope'] and
                       ' '.join(function['signature'].split()) == ' '.join(row['source_signature'].split())]
            self.assertEqual(len(matched), 1, (row['old_source'], row['old_identifier']))
            function = matched[0]
            actual = text[function['start']:function['end']]
            actual = re.sub(r'\n    using namespace detail_[A-Za-z_0-9]+;', '', actual)
            self.assertEqual(migration.cpp_tokens_sha256(actual), row['migrated_tokens_sha256'],
                             (row['old_source'], row['old_identifier']))
        self.assertEqual(len(ledger['functions']), 151)
        self.assertEqual(len({row['old_source'] for row in ledger['functions']}), 14)

    def test_every_anonymous_chunk_preserves_helpers_constants_and_literal_values(self):
        ledger = current_ledger()
        support = (ROOT / 'src/game_core/port/driving/projection_support.hpp').read_text()
        _, namespaces = migration.functions(support)
        for row in ledger['helper_chunks']:
            matches = [namespace for namespace in namespaces if namespace[3] == row['scope'] and
                       migration.cpp_tokens_sha256(support[namespace[1]:namespace[2]]) == row['migrated_tokens_sha256']]
            self.assertEqual(len(matches), 1, (row['old_source'], row['scope']))
        self.assertEqual(len(ledger['helper_chunks']), 12)

    def test_unparsed_namespace_statement_cannot_disappear_silently(self):
        for text in ('namespace port {\nconstexpr int state = 7;\nvoid f() {}\n}',
                     'namespace port {\nvoid f() {}\nint state;\n}'):
            functions, namespaces = migration.functions(text)
            with self.assertRaisesRegex(ValueError, 'Unparsed namespace-level'):
                migration.validate_unparsed_statements(text, functions, namespaces)
        text = 'namespace port {\nnamespace {\nconstexpr int state = 7;\n}\nvoid f() {}\n}'
        functions, namespaces = migration.functions(text)
        migration.validate_unparsed_statements(text, functions, namespaces)

    def test_token_fingerprint_retains_literals_and_rename_excludes_provenance_strings(self):
        first = 'float Sub_00100000() { return 1.25f; /* evidence */ }'
        same = 'float Sub_00100000() { return 1.25f; }'
        self.assertEqual(migration.cpp_tokens_sha256(first), migration.cpp_tokens_sha256(same))
        self.assertNotEqual(migration.cpp_tokens_sha256(first), migration.cpp_tokens_sha256(same.replace('1.25', '1.5')))
        self.assertNotEqual(migration.cpp_tokens_sha256('gap("0x00100000")'), migration.cpp_tokens_sha256('gap("0x00100004")'))
        text = 'Sub_00100000(); service.Sub_00100000(); "Sub_00100000"; // Sub_00100000'
        self.assertEqual(migration.rename_code_tokens(text, {'Sub_00100000': 'Original'}),
                         'Original(); service.Original(); "Sub_00100000"; // Sub_00100000')

    def test_overloads_get_addresses_from_exact_partition_signature(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(generator, 'ROOT', Path(directory)):
            root = Path(directory)
            naming = root / 'reverse/naming'
            naming.mkdir(parents=True)
            (naming / 'task0019-original-name-mappings.csv').write_text('original_identifier,elf_address\n')
            signatures = ['int bRandom(int value)', 'float bRandom(float value)', 'float bRandom()']
            addresses = ['0x00100000', '0x00100004', '0x00100008']
            relative_source = 'src/game_core/src/random.cpp'
            source = root / relative_source
            source.parent.mkdir(parents=True)
            source.write_text('\n'.join(signature + ' { return 1; }' for signature in signatures))
            rows = [dict(source=relative_source, source_signature=signature, elf_address=address)
                    for signature, address in zip(signatures, addresses)]
            (naming / 'task0042-projection-partitions.json').write_text(json.dumps(dict(functions=rows)))
            (naming / 'task0042-adopted-projection-names.csv').write_text(
                'source_identifier,elf_address\n' + ''.join('bRandom,' + address + '\n' for address in addresses))
            definitions = generator.definitions()
            for signature, address in zip(signatures, addresses):
                self.assertEqual([definition['signature'] for definition in definitions[address]], [signature])

    def test_all_recovered_partition_signatures_have_their_own_generator_address(self):
        ledger = current_ledger()
        definitions = generator.definitions()
        for row in ledger['functions']:
            if not row['elf_address']:
                continue
            matches = [definition for definition in definitions.get(row['elf_address'], [])
                       if definition['path'] == row['source'] and
                       definition['signature'] == ' '.join(row['source_signature'].split())]
            self.assertEqual(len(matches), 1, (row['elf_address'], row['source_signature']))

    def test_legacy_writer_refuses_later_registry_before_any_write(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            legacy.ensure_legacy_write_allowed(root)
            marker = root / 'reverse/naming/task0042-projection-partitions.json'
            marker.parent.mkdir(parents=True)
            marker.write_text('{"sentinel":true}')
            with self.assertRaisesRegex(ValueError, 'refusing to replace later name records'):
                legacy.ensure_legacy_write_allowed(root)
            self.assertEqual(marker.read_text(), '{"sentinel":true}')

    def test_support_includes_use_the_src_include_root(self):
        for relative in ('port/not_implemented.hpp', 'port/driving/ee_scalar.hpp'):
            self.assertEqual(migration.normalize_include_line('#include "' + relative + '"'),
                             '#include "game_core/' + relative + '"')
        header = (ROOT / 'src/game_core/port/driving/projections.hpp').read_text()
        self.assertNotIn('#include "game_core/port/driving/projections.hpp"', header)
        self.assertLess(header.index('struct ContactProvider;'), header.index('ContactProvider* contact'))


if __name__ == '__main__':
    unittest.main()

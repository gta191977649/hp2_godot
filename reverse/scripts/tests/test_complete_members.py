"""TASK-0043 synthetic membership/overload/ownership safety checks."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import complete_class_members as complete
import generate_class_templates as templates

class MemberSafetyTests(unittest.TestCase):
    def test_scope_comes_before_argument_scopes(self):
        self.assertEqual(complete.split_member('World::Query(Other::Value const *)'), ('World', 'Query'))
        self.assertIsNone(complete.split_member('Plain(Other::Value *)'))

    def test_same_spelling_in_distinct_namespaces_is_not_the_same_class(self):
        text = 'namespace hp2::frontend { struct EventInfo { int field; }; } namespace hp2::recovered { class EventInfo { public: void GetCar(); }; }'
        bounds = templates.class_range(text, 'EventInfo', 'hp2::recovered')
        self.assertIn('GetCar', text[bounds[0]:bounds[1]])
        self.assertNotIn('field', text[bounds[0]:bounds[1]])

    def test_original_path_does_not_escape_repository(self):
        self.assertEqual(complete.source_path('..\\indep\\src\\engine.cpp'), 'src/game_core/indep/src/engine.cpp')
        self.assertIsNone(complete.source_path('D:/private.cpp'))
        self.assertIsNone(complete.source_path('src/../../private.cpp'))

    def test_canonical_owner_replaces_shared_attribution_without_inheritance(self):
        classes = [dict(name='Derived', functions=['0x00100000'], methods={'0x00100000':{}}, constructors=[], namespace='hp2::recovered')]
        registry = dict(binary_id=templates.BINARY, classes={'Base':dict(source='base.cpp',header='base.hpp',namespace='hp2::recovered',placement_provenance='synthetic')}, members=[dict(owner='Base',elf_address='0x00100000',method='Method',record='evidence.md',evidence_id='synthetic',reference_name='Base::Method(void)',name_limit='host gap',source='base.cpp')])
        result = {c['name']:c for c in templates.apply_complete_members(classes, registry)}
        self.assertEqual(result['Derived']['functions'], [])
        self.assertEqual(result['Base']['functions'], ['0x00100000'])
        self.assertNotIn('bases', result['Base'])

    def test_entry_signature_excludes_typed_partial_overload(self):
        cls = dict(name='Example',source='a.cpp',methods={'0x00100000':dict(source='b.cpp')},entry_signatures={'0x00100000':'void Example::Sub_00100000()'})
        definitions = [dict(path='b.cpp',name='Example::Read',signature='void Example::Read()'),dict(path='b.cpp',name='Example::Read',signature='int Example::Read(int input)')]
        self.assertEqual(templates.entry_definitions(cls,'0x00100000','Read',definitions),definitions[:1])

    def test_typed_carrier_in_other_namespace_cannot_satisfy_class_entry(self):
        cls = dict(name='Wheel', namespace='hp2::recovered', source='wheel.cpp', methods={'0x00100000':{}}, entry_signatures={'0x00100000':'void Wheel::ComputeAuxiliaries()'})
        definitions = [dict(path='wheel.cpp',name='Wheel::ComputeAuxiliaries',namespace='port::driving::native',signature='void Wheel::ComputeAuxiliaries()'),dict(path='wheel.cpp',name='Wheel::ComputeAuxiliaries',namespace='hp2::recovered',signature='void Wheel::ComputeAuxiliaries()')]
        self.assertEqual(templates.entry_definitions(cls,'0x00100000','ComputeAuxiliaries',definitions),definitions[1:])

    def test_fill_preserves_existing_body_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(templates,'ROOT',Path(directory)):
            root = Path(directory)
            (root/'src').mkdir()
            (root/'src/header.hpp').write_text('namespace hp2::recovered { class Example { public: void Existing(); }; }\n')
            original = 'namespace hp2::recovered {\nvoid Example::Existing() { const int evidence = 17; (void)evidence; }\n}\n'
            (root/'src/source.cpp').write_text(original)
            cls=dict(name='Example',namespace='hp2::recovered',header='src/header.hpp',source='src/source.cpp',origin='synthetic',evidence=['synthetic'],placement_provenance='synthetic',labels=[],constructors=[],functions=['0x00100000','0x00100008'],methods={'0x00100000':dict(name='Existing',record='record.md',evidence='synthetic'),'0x00100008':dict(name='Missing',record='record.md',evidence='synthetic',source='src/second.cpp')})
            known={'0x00100000':[dict(path='src/source.cpp',name='Example::Existing',signature='void Example::Existing()',stub=False)]}
            with patch.object(templates,'definitions',return_value=known):
                templates.fill_code([cls],False)
            self.assertEqual((root/'src/source.cpp').read_text(),original)
            self.assertIn('hp2::not_implemented("0x00100008"', (root/'src/second.cpp').read_text())
            known['0x00100008']=[dict(path='src/second.cpp',name='Example::Missing',signature='void Example::Missing()',stub=True)]
            before={p:p.read_bytes() for p in root.rglob('*') if p.is_file()}
            with patch.object(templates,'definitions',return_value=known):
                templates.fill_code([cls],False)
            self.assertEqual(before,{p:p.read_bytes() for p in root.rglob('*') if p.is_file()})

if __name__ == '__main__': unittest.main()

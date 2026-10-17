"""Do not confuse address provenance, entity identity or original declaring scope."""
import json
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'reverse/scripts'))
import adopt_task0043_identifiers as a


class IdentifierAdoptionTests(unittest.TestCase):
    def test_comments_literals_and_nonfunction_entities_are_preserved(self):
        source='StringHash_002534A8(x); "StringHash_002534A8"; // StringHash_002534A8\nEventLink_002008B8 r;'
        self.assertEqual(a.token_replace(source,a.RENAMES),
            'bStringHash(x); "StringHash_002534A8"; // StringHash_002534A8\nEventLink_002008B8 r;')

    def test_overloaded_whole_entry_shells_are_not_flattened(self):
        source='namespace hp2::recovered { void Wheel::Sub_001A4158() {} void Wheel::Sub_001A4170() {} }'
        self.assertEqual(a.token_replace(source,a.RENAMES),source)
        self.assertIn('SetState',a.token_replace('namespace port::driving { void Sub_001A4170() {} }',a.RENAMES))

    def test_field_offset_in_another_class_after_getter_is_not_renamed(self):
        bridge=dict(owner='EventTree_001FFC40',old_identifier='Field_0x2C',recovered_name='pRewards')
        source=('namespace hp2::frontend {\nint EventTree_001FFC40::Sub_00200938(int i) { return Field_0x2C[i]; }\n'
            'namespace inner { void tCarLogoAnimation::Bind() { Field_0x2C = 0; } }\n}')
        result=a.rewrite_fields(source,'src/game_core/indep/src/eventtree.cpp',[bridge])
        self.assertIn('return pRewards[i]',result)
        self.assertIn('Field_0x2C = 0',result)

    def test_record_field_needs_a_reviewed_receiver(self):
        bridge=dict(owner='EventRecord_001FFF60',old_identifier='Field_0x3C',recovered_name='nTracks')
        source='record.Field_0x3C++; widget.Field_0x3C++; Field_0x3C++;'
        result=a.rewrite_fields(source,'src/game_core/indep/src/eventtree.cpp',[bridge])
        self.assertEqual(result,'record.nTracks++; widget.Field_0x3C++; Field_0x3C++;')

    def test_byte_and_word_getstatus_keep_distinct_owner_scopes(self):
        source='hp2::memorycard::SliceStatusByte_00200018(0, 1); hp2::memorycard::SliceStatusWord_00200068(0, 1);'
        result=a.token_replace(source,a.RENAMES)
        self.assertIn('hp2::memorycard::EventRankState::GetStatus(0, 1)',result)
        self.assertIn('hp2::memorycard::EventTimeState::GetStatus(0, 1)',result)

    def test_containing_constructor_is_not_the_extracted_title_helper_name(self):
        self.assertNotIn('TitleStringId_001F96B8',a.RENAMES)
        self.assertIn('TitleStringId_001F96B8', (ROOT/'src/game_core/indep/src/eventtree.hpp').read_text())


if __name__=='__main__':
    unittest.main()

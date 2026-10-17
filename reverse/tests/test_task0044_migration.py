"""Version-aware preservation must reject unreviewed edits on either side."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'reverse/scripts'))
import audit_task0043_preservation as bodies
import audit_task0044_migration as audit
import generate_class_templates as generator
class MigrationReviewTests(unittest.TestCase):
 def fixture(self):
  before=bodies.extract('int Old::Run(){return 1;}','src/game_core/a.cpp')
  after=bodies.extract('int New::Run(){return 2;}','src/game_core/b.cpp')
  def permission(row):return dict(audit.identity(row),evidence_id='synthetic',reason='synthetic test only')
  return before,after,dict(before=[permission(before[0])],after=[permission(after[0])])
 def test_exact_pinned_review_passes(self):
  before,after,review=self.fixture();self.assertTrue(audit.check_review(before,after,review)['passed'])
 def test_changed_replacement_is_rejected(self):
  before,after,review=self.fixture();after[0]['body_tokens'][-2]='3'
  self.assertFalse(audit.check_review(before,after,review)['passed'])
 def test_changed_old_baseline_is_rejected(self):
  before,after,review=self.fixture();before[0]['body_tokens'][-2]='3'
  self.assertFalse(audit.check_review(before,after,review)['passed'])
 def test_missing_replacement_and_added_algorithm_are_rejected(self):
  before,after,review=self.fixture();self.assertFalse(audit.check_review(before,[],review)['passed'])
  after+=bodies.extract('int Extra(){return 99;}','src/game_core/new.cpp')
  self.assertFalse(audit.check_review(before,after,review)['passed'])
 def test_signature_and_evidence_are_required(self):
  before,after,review=self.fixture();review['after'][0]['signature_sha256']='wrong'
  self.assertFalse(audit.check_review(before,after,review)['passed'])
  before,after,review=self.fixture();review['before'][0]['evidence_id']=''
  self.assertFalse(audit.check_review(before,after,review)['passed'])
 def test_class_range_accepts_explicit_port_inheritance(self):
  text='namespace hp2::recovered {class Fixture : public port::driving::ObjectView { public: void Go(); };}'
  self.assertIsNotNone(generator.class_range(text,'Fixture','hp2::recovered'))
 def test_debug_owner_has_no_retail_body_credit(self):
  owners=generator.debug_canonical_owners()
  original=json.loads((ROOT/'reverse/classes/task0044-debug-canonical-owners.json').read_text())['classes']
  successor=json.loads((ROOT/'reverse/classes/task0046-debug-canonical-owners.json').read_text())['classes']
  self.assertEqual(14,len(original));self.assertFalse(set(original)&set(successor))
  self.assertEqual(set(original)|set(successor),set(owners));self.assertIn('Engine',owners)
  definitions=generator.definitions()
  self.assertFalse(any(row['namespace']=='hp2::recovered' and row['name'].split('::')[0] in owners
     for rows in definitions.values() for row in rows))
 def test_generator_does_not_recreate_superseded_noarg_members(self):
  with tempfile.TemporaryDirectory() as directory,patch.object(generator,'ROOT',Path(directory)):
   root=Path(directory);p=root/'src/fixture.hpp';p.parent.mkdir();p.write_text('namespace hp2::recovered {class Fixture : public Bound {public:float GetValue(float);};}')
   cls=dict(name='Fixture',namespace='hp2::recovered',header='src/fixture.hpp',source='src/fixture.cpp',functions=['0x00100000'],superseded_by_debug={'header':'src/fixture.hpp'})
   with patch.object(generator,'definitions',return_value={}):generator.fill_code([cls],False)
   self.assertNotIn('GetValue()',p.read_text());self.assertFalse((root/'src/fixture.cpp').exists())
 def test_symbol_lookup_never_crosses_binary_identity(self):
  own=dict(binary_id=generator.BINARY,elf_address='0x00100000',status='Identified')
  foreign=dict(binary_id='sha256-debug-fixture',elf_address='0x00100000',status='Decompiled')
  self.assertEqual(generator.symbols_for_binary([own,foreign],generator.BINARY),{'0x00100000':own})
 def test_symbol_writer_preserves_foreign_same_address_row(self):
  import csv
  with tempfile.TemporaryDirectory() as directory,patch.object(generator,'ROOT',Path(directory)):
   root=Path(directory);(root/'reverse').mkdir();record=root/'reverse/record.md';record.write_text('# Synthetic record')
   own=dict(binary_id=generator.BINARY,elf_address='0x00100000',pc_implementation='none',status='Identified',record_path='reverse/record.md')
   foreign=dict(binary_id='sha256-debug-fixture',elf_address='0x00100000',pc_implementation='foreign canonical binding',status='Decompiled',record_path='foreign.md')
   path=root/'reverse/symbols.csv'
   with path.open('w',newline='') as handle:
    writer=csv.DictWriter(handle,list(own));writer.writeheader();writer.writerows([own,foreign])
   cls=dict(name='Fixture',source='src/game_core/Fixture.cpp',functions=['0x00100000'],methods={'0x00100000':dict(name='Tick',record='reverse/record.md',evidence='synthetic')})
   generator.synchronize_records([cls],{'0x00100000':own},False)
   with path.open() as handle:rows=list(csv.DictReader(handle))
   self.assertEqual(rows[1],foreign);self.assertIn('TASK-0028 class entry coverage',rows[0]['pc_implementation'])
 def test_complete_member_coverage_keeps_superseded_retail_historical(self):
  import complete_class_members as complete
  member=dict(owner='Fixture',elf_address='0x00100000',debug_address='0x00200000',method='Tick',reference_name='Fixture::Tick(float)',source='fixture.cpp',exclusion_reason='',name_limit='historical gap interface',evidence_id='synthetic',template_action='added_whole_entry')
  with patch.object(complete.g,'definitions',return_value={}),patch.object(complete.g,'model',return_value=([dict(name='Fixture')],None,None,None)),patch.object(complete.g,'debug_canonical_owners',return_value={'Fixture':{}}):
   rows=complete.coverage(dict(members=[member]))
   self.assertEqual(rows[0]['status'],'historical_retail_entry_superseded_by_debug_owner')
   self.assertIn('no current Debug signature inference',rows[0]['signature'])
   self.assertIn('task0044-debug-canonical-owners.json',rows[0]['reason'])
 def test_complete_member_coverage_still_requires_unmigrated_entry(self):
  import complete_class_members as complete
  member=dict(owner='Fixture',elf_address='0x00100000',method='Tick',exclusion_reason='')
  with patch.object(complete.g,'definitions',return_value={}),patch.object(complete.g,'model',return_value=([dict(name='Fixture',source='fixture.cpp')],None,None,None)),patch.object(complete.g,'debug_canonical_owners',return_value={}):
   with self.assertRaisesRegex(ValueError,'Expected one whole-entry definition'):complete.coverage(dict(members=[member]))
if __name__=='__main__':unittest.main()

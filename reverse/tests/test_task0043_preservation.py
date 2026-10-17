"""Token preservation tests; no proprietary input required."""
import importlib.util
from pathlib import Path
import unittest

PATH = Path(__file__).resolve().parents[1] / 'scripts/audit_task0043_preservation.py'
SPEC = importlib.util.spec_from_file_location('audit_task0043', PATH)
audit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audit)


class PreservationTests(unittest.TestCase):
    def rows(self, text, path='src/game_core/example.cpp'):
        return audit.extract(text, path)

    def test_format_comments_and_nested_braces(self):
        before = self.rows('int Foo::Sub_00123456(int x) { if (x) { return 1; } return 0; }')
        after = self.rows('int Foo::Recovered(int x) { /* } */ if(x){return 1;} return 0; }')
        self.assertEqual(1, len(before))
        self.assertTrue(audit.compare(before, after, {'Sub_00123456': 'Recovered'})['passed'])

    def test_call_rename_with_body_preserved(self):
        before = self.rows('void f() { Foo::Sub_00123456(); const char* n = "Sub_00123456"; }')
        after = self.rows('void f() { Foo::Original(); const char* n = "Original"; }')
        self.assertTrue(audit.compare(before, after, {'Sub_00123456': 'Original'})['passed'])
        self.assertFalse(audit.compare(before, after, {})['passed'])

    def test_relocation_and_overload_multiset(self):
        before = self.rows('int C::f(int x) { return x; } int C::f(float x) { return 2; }')
        after = self.rows('int C::f(float x) { return 2; } int C::f(int x) { return x; }', 'src/game_core/moved.cpp')
        self.assertTrue(audit.compare(before, after, {})['passed'])
        self.assertFalse(audit.compare(before, after[:1], {})['passed'])

    def test_sole_throw_additions_only(self):
        added = self.rows('void C::Sub_00123456() { hp2::not_implemented("0x00123456", "Original"); }')
        report = audit.compare([], added, {})
        self.assertTrue(report['passed'])
        self.assertEqual(1, report['added_throwing_stubs'])
        self.assertFalse(audit.compare([], self.rows('void f(){ return; }'), {})['passed'])

    def test_lambda_and_string_body_remain_inside_function(self):
        rows = self.rows('void f() noexcept { auto f=[](){return "}";}; f(); }')
        self.assertEqual(1, len(rows))
        changed = self.rows('void f() noexcept { auto f=[](){return "{";}; f(); }')
        self.assertFalse(audit.compare(rows, changed, {})['passed'])

    def test_existing_body_cannot_disappear_or_change_constant(self):
        rows = self.rows('float f() { return 3.0f; }')
        self.assertFalse(audit.compare(rows, [], {})['passed'])
        self.assertFalse(audit.compare(rows, self.rows('float f() { return 4.0f; }'), {})['passed'])

    def test_scoped_field_alias_normalization(self):
        before = self.rows('int f() { return a.Field_0x3C + b.Field_0x3C; }')
        after = self.rows('int f() { return a.nTracks + b.Field_0x3C; }')
        self.assertTrue(audit.compare(before, after, {}, {'nTracks': 'Field_0x3C'})['passed'])

    def test_new_scope_qualification(self):
        before = self.rows('int f() { return SliceStatusByte(x); }')
        after = self.rows('int f() { return EventRankState::GetStatus(x); }')
        self.assertTrue(audit.compare(before, after, {'SliceStatusByte': 'EventRankState::GetStatus'})['passed'])

    def test_pinned_shared_stub_removal_requires_canonical_throw(self):
        import hashlib
        before = self.rows('void Wrong::Sub_00123456(){ hp2::not_implemented("0x00123456", "old"); }')
        after = self.rows('void Correct::Original(){ hp2::not_implemented("0x00123456", "original"); }')
        permission = {'source': before[0]['path'], 'name': before[0]['name'],
                      'elf_address': '0x00123456', 'canonical_source': after[0]['path'],
                      'canonical_name': after[0]['name'],
                      'baseline_body_sha256': hashlib.sha256(' '.join(before[0]['body_tokens']).encode()).hexdigest()}
        report = audit.compare(before, after, {}, permitted_removals=[permission])
        self.assertTrue(report['passed'])
        self.assertEqual(1, len(report['permitted_removed_shared_stubs']))
        self.assertFalse(audit.compare(before, [], {}, permitted_removals=[permission])['passed'])
        permission['baseline_body_sha256'] = 'incorrect'
        self.assertFalse(audit.compare(before, after, {}, permitted_removals=[permission])['passed'])

    def test_nonstub_removal_cannot_be_approved(self):
        import hashlib
        before = self.rows('int Wrong::Sub_00123456(){ return 1; }')
        after = self.rows('void Correct::Original(){ hp2::not_implemented("0x00123456", "original"); }')
        permission = {'source': before[0]['path'], 'name': before[0]['name'],
                      'elf_address': '0x00123456', 'canonical_source': after[0]['path'],
                      'canonical_name': after[0]['name'],
                      'baseline_body_sha256': hashlib.sha256(' '.join(before[0]['body_tokens']).encode()).hexdigest()}
        self.assertFalse(audit.compare(before, after, {}, permitted_removals=[permission])['passed'])

    def test_test_changes_reported_separately(self):
        before = self.rows('int main(){ check(1); }', 'src/tests/class_template_tests.cpp')
        after = self.rows('int main(){ check(1); check(2); }', 'src/tests/class_template_tests.cpp')
        report = audit.compare(before, after, {})
        self.assertTrue(report['passed'])
        self.assertEqual(1, len(report['test_body_changes_outside_production_preservation']))


if __name__ == '__main__':
    unittest.main()

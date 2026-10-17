"""TASK-0028: coverage, honest provenance and preservation of edited implementations."""
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("class_templates", ROOT / "reverse/scripts/generate_class_templates.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ClassTemplateTests(unittest.TestCase):
    def test_recovered_name_keeps_whole_entry_separate_from_typed_projection(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(MODULE, "ROOT", Path(directory)):
            root = Path(directory)
            naming = root / "reverse/naming"
            naming.mkdir(parents=True)
            (naming / "task0019-original-name-mappings.csv").write_text("original_identifier,elf_address\n")
            (naming / "task0041-adopted-source-names.csv").write_text(
                "source_identifier,elf_address\nFixture::Tick,0x00100000\n")
            source = root / "src/game_core/fixture.cpp"
            source.parent.mkdir(parents=True)
            source.write_text('void Fixture::Tick() { hp2::not_implemented("0x00100000", "Fixture::Tick"); }\n'
                              'int Fixture::Tick(int value) { return value; }\n')
            definitions = MODULE.definitions()["0x00100000"]
            cls = {"name": "Fixture", "source": "src/game_core/fixture.cpp", "entry_signatures": {
                "0x00100000": "void Fixture::Sub_00100000()"}}
            whole, = MODULE.entry_definitions(cls, "0x00100000", "Tick", definitions)
            self.assertTrue(whole["stub"])
            self.assertEqual(len(definitions), 2)

    def test_whole_entry_is_not_promoted_by_typed_slice_overload(self):
        cls = {"name": "Fixture", "source": "fixture.cpp", "entry_signatures": {
            "0x00100000": "void Fixture::Sub_00100000()"}}
        whole = {"path": "fixture.cpp", "name": "Fixture::Sub_00100000",
                 "signature": "void Fixture::Sub_00100000()", "stub": True}
        slice_ = dict(whole, signature="int Fixture::Sub_00100000(int x)", stub=False)
        self.assertEqual(MODULE.entry_definitions(cls, "0x00100000", "Sub_00100000", [slice_, whole]), [whole])

    def test_unknown_paths_stay_at_root_and_confirmed_directories_have_evidence(self):
        classes, _, _, policy = MODULE.model()
        layout = json.loads((ROOT / 'reverse/classes/task0042-class-layout.json').read_text())
        complete = json.loads((ROOT / 'reverse/classes/task0043-member-registry.json').read_text())
        decisions = json.loads((ROOT / "reverse/classes/source-path-decisions.json").read_text())
        decisions = {d["class_name"]: d for d in decisions["decisions"]}
        report = json.loads((ROOT / "reverse/classes/source-path-bindings.json").read_text())
        report = {c["class_name"]: c for c in report["classes"]}
        retained = set(policy["existing_classes"]) | set(policy["module_placements"])
        scoped = [c for c in classes if c["name"] not in retained]
        self.assertEqual(set(report), {c["name"] for c in scoped})
        for cls in scoped:
            name = cls["name"]
            if name in complete['classes']:
                placement = complete['classes'][name]
                self.assertEqual(cls['source'], placement['source'])
                self.assertEqual(cls['header'], placement['header'])
                self.assertEqual(report[name]['status'], 'confirmed_debug_translation_unit')
                self.assertTrue(report[name]['definition_evidence'])
                for key in ('source', 'header'):
                    self.assertTrue((ROOT / cls[key]).is_file(), cls[key])
                continue
            if name in layout['classes']:
                placement = layout['classes'][name]
                self.assertEqual(cls['source'], placement['source'])
                self.assertEqual(cls['header'], placement['header'])
                self.assertTrue(placement['debug_member_evidence'], name)
                self.assertEqual(placement['evidence_id'], 'E-0042-S001')
                for key in ('source', 'header'):
                    self.assertTrue((ROOT / cls[key]).is_file(), cls[key])
                continue
            expected = Path("src/game_core")
            if name in decisions:
                d = decisions[name]
                expected /= d["original_directory"]
                self.assertEqual(report[name]["status"], "confirmed_directory")
                self.assertEqual(report[name]["binding"], d)
                self.assertTrue(d["evidence_id"].startswith("E-0029-P"))
                self.assertTrue(d["source_reference_instructions"])
            else:
                self.assertEqual(report[name]["status"], "unknown")
            self.assertEqual(Path(cls["header"]).parent, expected, name)
            self.assertEqual(Path(cls["source"]).parent, expected, name)
            self.assertTrue((ROOT / cls["header"]).is_file())
            self.assertTrue((ROOT / cls["source"]).is_file())
        self.assertEqual(next(c['header'] for c in scoped if c['name']=='PsychoDriver'),
                         'src/game_core/indep/inc/PsychoDriver.hpp')
        self.assertEqual(Path(next(c['header'] for c in scoped if c['name']=='tCustomizeHUDScreen')).parent, Path('src/game_core'))
        self.assertFalse(list((ROOT / "src/game_core/port/class_templates").glob("*")))

    def test_exact_declaring_scope_has_one_canonical_entry_without_native_inheritance(self):
        classes, _, _, _ = MODULE.model()
        by_name = {c['name']: c for c in classes}
        layout = json.loads((ROOT / 'reverse/classes/task0042-class-layout.json').read_text())
        for transfer in layout['canonical_members']:
            address = transfer['elf_address']
            owner = by_name[transfer['owner']]
            self.assertIn(address, owner['functions'])
            self.assertEqual(owner['methods'][address]['name'], transfer['method'])
            for name in transfer['remove_from']:
                self.assertNotIn(address, by_name[name]['functions'])
        self.assertIn('0x0019C808', by_name['PhysicsSystem']['functions'])
        self.assertNotIn('0x0019C808', by_name['PhysicsCar']['functions'])
        self.assertIn('0x0012E1B8', by_name['cDBFilesBase']['functions'])
        self.assertNotIn('0x0012E1B8', by_name['cDBFileSys']['functions'])
        for name in ('PhysicsCar', 'PsychoDriver', 'cDBFileSys'):
            header = (ROOT / by_name[name]['header']).read_text()
            self.assertNotRegex(header, r'class\s+' + name + r'\s*:')

    def test_task0042_supported_methods_keep_ctor_dtor_entries_address_named(self):
        classes, _, _, _ = MODULE.model()
        by_name = {cls['name']: cls for cls in classes}
        supported = {}
        for family in ('candidate', 'unresolved'):
            for bridge in json.loads((ROOT / f'reverse/naming/task0042-{family}-bridges.json').read_text())['bridges']:
                if bridge['status'].startswith('supported'):
                    supported[bridge['retail_address']] = bridge
        for row in MODULE.read_csv('reverse/naming/task0042-adopted-source-names.csv'):
            bridge = supported[row['elf_address']]
            self.assertEqual(row['reference_name'], bridge['reference_name'])
            self.assertTrue(bridge['reference_name'].startswith(row['class_name'] + '::' + row['method'] + '('))
            self.assertEqual(by_name[row['class_name']]['methods'][row['elf_address']]['name'], row['method'])
        for name, address in [('CarDriver', '0x00136D00'), ('CarMover', '0x00136B78'),
                              ('PhysicsVehicle', '0x00139678'), ('CarController', '0x001C00A8'),
                              ('PhysicsSystem', '0x0019C170')]:
            self.assertEqual(by_name[name]['methods'][address]['name'], 'Sub_' + address[2:])

    def test_evidence_coverage_and_names(self):
        classes, labels, _, _ = MODULE.model()
        inventory = json.loads((ROOT / "reverse/classes/template-inventory.json").read_text())
        by_name = {c["name"]: c for c in inventory["classes"]}
        self.assertEqual(len(by_name), len(inventory["classes"]))
        self.assertEqual(set(by_name), {c["name"] for c in classes})
        self.assertEqual(len(labels), 23)
        self.assertTrue(all(not MODULE.VALID_NAME.fullmatch(r["class_name"]) for r in labels))
        for cls in classes:
            actual = by_name[cls["name"]]
            addresses = [m["elf_address"] for m in actual["members"]]
            self.assertEqual(len(addresses), len(set(addresses)), cls["name"])
            self.assertEqual(set(addresses), set(cls["functions"]), cls["name"])
            for member in actual["members"]:
                self.assertTrue((ROOT / member["record"]).is_file())
        manager = by_name["SlotPoolManager"]
        self.assertEqual({m["method"] for m in manager["members"]}, {"NewSlotPool", "DeleteSlotPool"})
        self.assertEqual(by_name["bFile"]["members"], [])  # Filename is a field expression.
        self.assertEqual(by_name["TitleScreen"]["header"], "src/game_core/indep/src/fmenus.hpp")
        self.assertEqual(by_name["tEventLeaf"]["header"], "src/game_core/indep/src/eventtree.hpp")

    def test_generator_appends_missing_member_and_preserves_recovered_body(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(MODULE, "ROOT", Path(directory)):
            root = Path(directory)
            naming = root / "reverse/naming"
            naming.mkdir(parents=True)
            (naming / "task0019-original-name-mappings.csv").write_text("original_identifier,elf_address\n")
            core = root / "src/game_core"
            core.mkdir(parents=True)
            header = core / "Fixture.hpp"
            source = core / "Fixture.cpp"
            header.write_text("namespace hp2::recovered { class Fixture { public: int Sub_00100000(); }; }\n")
            recovered = "int Fixture::Sub_00100000() { return 42; }"
            source.write_text('namespace hp2::recovered {\n' + recovered + '\n}\n')
            cls = {"name": "Fixture", "origin": "synthetic_test_only", "evidence": [], "labels": [],
                   "header": header.relative_to(root).as_posix(), "source": source.relative_to(root).as_posix(),
                   "namespace": "hp2::recovered", "placement_provenance": "synthetic", "constructors": [],
                   "functions": ["0x00100000", "0x00100004"],
                   "methods": {a: {"name": "Sub_" + a[2:], "record": "synthetic", "evidence": "synthetic"}
                               for a in ["0x00100000", "0x00100004"]}}
            MODULE.fill_code([cls], False)
            self.assertIn(recovered, source.read_text())
            self.assertEqual(source.read_text().count("Fixture::Sub_00100004()"), 1)
            first = (header.read_bytes(), source.read_bytes())
            MODULE.fill_code([cls], False)
            self.assertEqual(first, (header.read_bytes(), source.read_bytes()))
            MODULE.fill_code([cls], True)

    def test_comment_and_string_addresses_are_not_implementations(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(MODULE, "ROOT", Path(directory)):
            root = Path(directory)
            naming = root / "reverse/naming"
            naming.mkdir(parents=True)
            (naming / "task0019-original-name-mappings.csv").write_text("original_identifier,elf_address\n")
            core = root / "src/game_core"
            core.mkdir(parents=True)
            (core / "fixture.cpp").write_text('// void Fixture::Sub_00100000() {}\nconst char* s = "Sub_00100004() {}";\n')
            self.assertEqual(MODULE.definitions(), {})

    def test_existing_unindexed_knowledge_record_is_preserved(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(MODULE, "ROOT", Path(directory)):
            root = Path(directory)
            record = root / "reverse/functions/synthetic/unknown/00100000.md"
            record.parent.mkdir(parents=True)
            original = "Previously recorded boundary evidence must survive generation.\n"
            record.write_text(original)
            fields = ("binary_id", "elf_address", "original_identifier", "recovered_name", "signature",
                      "calling_convention", "subsystem", "confidence", "status", "record_path",
                      "pc_implementation", "evidence_ids")
            symbols = {"0x00100004": dict.fromkeys(fields, "unknown")}
            symbols["0x00100004"]["elf_address"] = "0x00100004"
            import csv
            prior_rows = [dict(symbols["0x00100004"], status="Tested"),
                          dict(symbols["0x00100004"], status="Identified")]
            with (root / "reverse/symbols.csv").open("w", newline="") as handle:
                writer = csv.DictWriter(handle, fieldnames=fields)
                writer.writeheader()
                writer.writerows(prior_rows)
            cls = {"name": "Fixture", "source": "src/game_core/fixture.cpp", "functions": ["0x00100000"],
                   "methods": {"0x00100000": {"record": record.relative_to(root).as_posix(), "evidence": "synthetic"}}}
            MODULE.synchronize_records([cls], symbols, False)
            self.assertEqual(record.read_text(), original)
            self.assertEqual(symbols["0x00100000"]["status"], "Identified")
            with (root / "reverse/symbols.csv").open(newline="") as handle:
                actual_rows = list(csv.DictReader(handle))
            self.assertEqual(actual_rows[:2], prior_rows)
            self.assertEqual(len(actual_rows), 3)

    def test_coverage_link_updates_do_not_retain_stale_generated_paths(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(MODULE, "ROOT", Path(directory)):
            root = Path(directory)
            record = root / "reverse/functions/synthetic/unknown/00100000.md"
            record.parent.mkdir(parents=True)
            record.write_text("Synthetic record; no behaviour.\n")
            row = dict(binary_id="synthetic", elf_address="0x00100000", original_identifier="unknown",
                       recovered_name="Sub_00100000", signature="unknown", calling_convention="unknown",
                       subsystem="unknown", confidence="unknown", status="Identified",
                       record_path=record.relative_to(root).as_posix(),
                       pc_implementation="TASK-0028 class entry coverage: old/path.cpp (templates/host projections; no status promotion)",
                       evidence_ids="synthetic")
            import csv
            with (root / "reverse/symbols.csv").open("w", newline="") as handle:
                writer = csv.DictWriter(handle, fieldnames=list(row))
                writer.writeheader()
                writer.writerow(row)
            cls = {"name":"Fixture", "source":"src/game_core/Fixture.cpp", "functions":["0x00100000"],
                   "methods":{"0x00100000":{"record":record.relative_to(root).as_posix(),"evidence":"synthetic"}}}
            symbols = {"0x00100000":row}
            MODULE.synchronize_records([cls], symbols, False)
            self.assertNotIn("old/path.cpp", (root / "reverse/symbols.csv").read_text())
            MODULE.synchronize_records([cls], symbols, True)


if __name__ == "__main__":
    unittest.main()

"""TASK-0028: fill missing class shells/members without overwriting recovered C++ bodies.

--write creates missing code/declarations and synchronizes derived metadata/tests.
--check is read-only and verifies coverage and generated metadata. C++ bodies remain editable:
changing a stub never gets undone by --write, and never promotes evidence status automatically.
Inputs are derived repository evidence only; this script never accesses Ghidra or raw game data.
"""
from __future__ import annotations

import argparse
import csv
import io
import json
import re
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SHA = "7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"
BINARY = "sha256-" + SHA
VALID_NAME = re.compile(r"[A-Za-z_][A-Za-z_0-9]*\Z")
METHOD = re.compile(
    r"(?m)^([^\n;{}]*\b([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*"
    r"\([^;{}]*\)\s*(?:const\s*)?)\{"
)


def read_csv(path: str) -> list[dict[str, str]]:
    with (ROOT / path).open(encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def mask_comments_strings(text: str) -> str:
    # Preserve offsets and newlines for brace matching; strings can contain braces too.
    return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda m: "".join("\n" if c == "\n" else " " for c in m[0]), text)


def closing_brace(masked: str, opening: int) -> int:
    depth = 1
    for i in range(opening + 1, len(masked)):
        depth += (masked[i] == "{") - (masked[i] == "}")
        if depth == 0:
            return i
    raise ValueError("Unbalanced C++ braces")


def class_range(text: str, name: str) -> tuple[int, int] | None:
    masked = mask_comments_strings(text)
    match = re.search(r"\b(?:class|struct)\s+" + re.escape(name) + r"\s*\{", masked)
    return (match.end() - 1, closing_brace(masked, match.end() - 1)) if match else None


def definitions() -> dict[str, list[dict]]:
    result: dict[str, list[dict]] = defaultdict(list)
    originals = {r['original_identifier']: r['elf_address'] for r in
                 read_csv('reverse/naming/task0019-original-name-mappings.csv')}
    overrides_path = ROOT / 'reverse/naming/task0041-adopted-source-names.csv'
    if overrides_path.exists():
        originals.update({r['source_identifier']: r['elf_address'] for r in
                          read_csv('reverse/naming/task0041-adopted-source-names.csv')})
    projection_path = ROOT / 'reverse/naming/task0041-adopted-projection-names.csv'
    if projection_path.exists():
        originals.update({r['source_identifier']: r['elf_address'] for r in
                          read_csv('reverse/naming/task0041-adopted-projection-names.csv')})
    for path in sorted((ROOT / "src/game_core").rglob("*.cpp")):
        text = path.read_text(encoding="utf-8")
        masked = mask_comments_strings(text)
        for match in METHOD.finditer(masked):
            # The greedy return-type prefix may consume Class:: in METHOD's group 2.
            # Recover the complete declarator token from the signature instead.
            name = re.search(r"([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*$", match[1].split("(", 1)[0])[1]
            address = re.search(r"_([0-9A-Fa-f]{8})$", name)
            if not address and name not in originals:
                continue
            end = closing_brace(masked, match.end() - 1)
            body = text[match.end():end]
            # Only a sole throwing call is classified as a template. Partial bodies can throw.
            clean = re.sub(r"//[^\n]*|/\*[\s\S]*?\*/", "", body).strip()
            stub = bool(re.fullmatch(r'hp2::not_implemented\("0x[0-9A-Fa-f]{8}",\s*"[^"\n]*"\);', clean))
            key = "0x" + address[1].upper() if address else originals[name]
            result[key].append({
                "path": path.relative_to(ROOT).as_posix(), "name": name,
                "signature": " ".join(text[match.start():match.end()-1].split()),
                "stub": stub,
            })
    return result


def entry_definitions(cls: dict, address: str, method: str, candidates: list[dict]) -> list[dict]:
    """Explicitly separate a whole entry from typed overloads implementing bounded slices."""
    own = [d for d in candidates if d["path"] == cls["source"]
           and d["name"] == cls["name"] + "::" + method]
    signature = cls.get("entry_signatures", {}).get(address)
    if signature:
        signature = signature.replace('Sub_' + address[2:], method)
        own = [d for d in own if d["signature"] == signature]
    return own


def model() -> tuple[list[dict], list[dict], dict[str, dict], dict]:
    policy = json.loads((ROOT / "reverse/classes/template-policy.json").read_text())
    assert policy["binary_id"] == BINARY
    classes: dict[str, dict] = {}
    labels = []
    for row in read_csv("reverse/classes.csv"):
        if row["binary_id"] != BINARY:
            raise ValueError("Unexpected binary identity")
        if not VALID_NAME.fullmatch(row["class_name"]):
            # Behaviour progress is tracked elsewhere; keep this catalogue attribution-only.
            labels.append({k: v for k, v in row.items() if k not in
                           ('functions_with_record', 'functions_implemented', 'functions_tested')})
            continue
        name = row["class_name"]
        classes[name] = {"name": name, "functions": set(row["functions"].split()),
                         "constructors": row["constructors"].split(),
                         "vtables": row["vtables"].split(), "labels": [],
                         "evidence": [row["evidence_id"]], "origin": "allocator_label"}
    for row in read_csv("reverse/naming/task0019-qualified-string-starts.csv"):
        match = re.match(r"([A-Za-z_]\w*)::", row["qualified_text"])
        if not match:
            raise ValueError("Unrecognized qualified-name evidence")
        name = match[1]
        cls = classes.setdefault(name, {"name": name, "functions": set(),
                                       "constructors": [], "vtables": [], "labels": [],
                                       "evidence": ["E-0019-N03"], "origin": "qualified_string"})
        cls["labels"].append({"text": row["qualified_text"], "address": row["full_string_start"]})
    originals = {}
    debug_overrides = {}
    class_identifier_overrides = {}
    alias_path = ROOT / 'reverse/naming/task0041-class-identifier-overrides.json'
    if alias_path.exists():
        class_identifier_overrides = json.loads(alias_path.read_text())
    if (ROOT / 'reverse/naming/task0041-adopted-source-names.csv').exists():
        debug_overrides = {(r['class_name'], r['elf_address']): r for r in
                           read_csv('reverse/naming/task0041-adopted-source-names.csv')}
    for row in read_csv("reverse/naming/task0019-original-name-mappings.csv"):
        name, method = row["original_identifier"].split("::")
        if not VALID_NAME.fullmatch(method):
            raise ValueError("Original method cannot be represented verbatim")
        classes[name]["functions"].add(row["elf_address"].upper().replace("0X", "0x"))
        originals[row["elf_address"].upper().replace("0X", "0x")] = row
    for name, placement in policy["existing_classes"].items():
        classes[name]["functions"].update(placement["additional_functions"])
    for name, attribution in policy.get("member_attributions", {}).items():
        classes[name]["functions"].update(attribution["additional_functions"])
        classes[name]["entry_signatures"] = attribution.get("entry_signatures", {})
    symbols = {r["elf_address"].upper().replace("0X", "0x"): r for r in read_csv("reverse/symbols.csv")}
    for cls in classes.values():
        name = cls["name"]
        placement = (policy["existing_classes"].get(name) or policy["module_placements"].get(name)
                     or policy.get("original_directory_placements", {}).get(name))
        if placement:
            cls.update(placement)
            cls["placement_provenance"] = placement.get("placement_provenance", "preserved_existing_module; class-to-original-file binding not asserted")
        else:
            base = policy.get("default_directory", "src/game_core") + "/" + name
            cls.update(header=base + ".hpp", source=base + ".cpp", namespace="hp2::recovered")
            cls["placement_provenance"] = "user_requested_root; port-side placement; original path unknown"
        cls["functions"] = sorted(cls["functions"])
        if name in class_identifier_overrides:
            alias = class_identifier_overrides[name]
            cls['name'] = alias['name']
            cls['origin'] = 'debug named vtable; allocator label ' + name
            cls['evidence'].append(alias['evidence_id'])
            name = cls['name']
        cls["methods"] = {}
        for address in cls["functions"]:
            original = originals.get(address)
            method = original["original_identifier"].split("::")[1] if original else "Sub_" + address[2:]
            if original and original["original_identifier"].split("::")[0] != name:
                raise ValueError("Conflicting original-name/class mapping")
            record = symbols.get(address, {}).get("record_path") or (
                f"reverse/functions/{BINARY}/unknown/{address[2:].lower()}.md")
            evidence = "E-0019-N03" if original else "E-0027-C01; E-0028-C01"
            if placement and address in placement.get("additional_functions", []):
                evidence = placement["evidence"] + "; E-0028-C01"
            attribution = policy.get("member_attributions", {}).get(name, {})
            if address in attribution.get("additional_functions", []):
                evidence = attribution["evidence"]
            override = debug_overrides.get((name, address))
            if override:
                method = override['method']
                evidence += '; ' + override['evidence_id']
            cls["methods"][address] = {"name": method, "record": record, "evidence": evidence}
    return sorted(classes.values(), key=lambda c: c["name"]), labels, symbols, policy


def provenance(cls: dict) -> str:
    labels = "\n".join(f"// Diagnostic text (not a callable signature): {r['text']} [{r['address']}]"
                       for r in cls["labels"])
    return (f"// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 {SHA}.\n"
            f"// Class spelling: {cls['origin']}; evidence: {', '.join(cls['evidence'])}.\n"
            f"// Placement: {cls['placement_provenance']}.\n"
            "// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.\n"
            "// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.\n"
            + (labels + "\n" if labels else ""))


def declaration(cls: dict, address: str) -> str:
    method = cls["methods"][address]
    ctor = " Constructor entry; NOT a C++ constructor." if address in cls["constructors"] else ""
    return (f"    // ELF {address}; {method['evidence']}; {method['record']}.{ctor}\n"
            "    // Port-side void/no-argument placeholder; original signature unknown.\n"
            f"    void {method['name']}();\n")


def cpp_body(cls: dict, address: str) -> str:
    method = cls["methods"][address]
    return (f"// ELF {address}; {method['evidence']}; {method['record']}.\n"
            f"void {cls['name']}::{method['name']}() {{\n"
            f"    hp2::not_implemented(\"{address}\", \"{cls['name']}::{method['name']} (signature and full body unresolved)\");\n"
            "}\n")


def write_text(path: str, text: str) -> None:
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    if not target.exists() or target.read_text(encoding="utf-8") != text:
        target.write_text(text, encoding="utf-8", newline="\n")


def fill_code(classes: list[dict], check: bool) -> None:
    defs = definitions()
    for cls in classes:
        path = ROOT / cls["header"]
        text = path.read_text(encoding="utf-8") if path.exists() else ""
        bounds = class_range(text, cls["name"])
        missing = []
        for address in cls["functions"]:
            method = cls["methods"][address]["name"]
            own = entry_definitions(cls, address, method, defs.get(address, []))
            if len(own) > 1:
                raise ValueError(f"Duplicate member definition: {cls['name']}::{method}")
            declaration_found = bounds and re.search(r"\b" + re.escape(method) + r"\s*\(",
                                                    mask_comments_strings(text[bounds[0]:bounds[1]]))
            if own and not declaration_found:
                raise ValueError(f"Definition without class declaration: {cls['name']}::{method}")
            if declaration_found and not own:
                raise ValueError(f"Declaration without definition: {cls['name']}::{method}; audit before generating")
            if not own:
                missing.append(address)
        if check and (not bounds or missing):
            raise ValueError(f"Missing class/member templates: {cls['name']}: {missing}")
        if check:
            continue
        if not bounds:
            block = provenance(cls) + f"namespace {cls['namespace']} {{\nclass {cls['name']} {{\npublic:\n"
            block += "".join(declaration(cls, a) for a in missing)
            if not cls["functions"]:
                block += "    // No member address is confirmed; no speculative members are declared.\n"
            block += f"}};\n}} // namespace {cls['namespace']}\n"
            if not text:
                text = "// PORT-SIDE template file; destination/header extension is not original-file evidence.\n#pragma once\n"
            text += "\n" + block
        elif missing:
            # Append public declarations only. Never replace a class or its fields/methods.
            insert = "\npublic:\n    // TASK-0028: missing entries; existing storage and bodies are preserved.\n"
            insert += "".join(declaration(cls, a) for a in missing)
            text = text[:bounds[1]] + insert + text[bounds[1]:]
        write_text(cls["header"], text)
        source_path = ROOT / cls["source"]
        source = source_path.read_text(encoding="utf-8") if source_path.exists() else ""
        header_include = cls["header"][len("src/"):]
        if not source:
            source = provenance(cls) + f'#include "{header_include}"\n'
        if missing:
            if (f'#include "{header_include}"' not in source and
                    f'#include "{Path(header_include).name}"' not in source):
                source += f'\n#include "{header_include}"\n'
            if '#include "game_core/port/not_implemented.hpp"' not in source:
                source += '\n#include "game_core/port/not_implemented.hpp"\n'
            source += f"\nnamespace {cls['namespace']} {{\n"
            source += "\n".join(cpp_body(cls, a) for a in missing)
            source += f"}} // namespace {cls['namespace']}\n"
        write_text(cls["source"], source)


def synchronize_records(classes: list[dict], symbols: dict[str, dict], check: bool) -> None:
    owners: dict[str, list[dict]] = defaultdict(list)
    for cls in classes:
        for address in cls["functions"]:
            owners[address].append(cls)
    fields = list(next(iter(symbols.values())))
    changed = False
    for address, attributed in sorted(owners.items()):
        method = attributed[0]["methods"][address]
        record = ROOT / method["record"]
        if address not in symbols:
            if check:
                raise ValueError(f"Missing symbol record: {address}")
            symbols[address] = dict.fromkeys(fields, "")
            symbols[address].update(binary_id=BINARY, elf_address=address,
                original_identifier="unknown", recovered_name="Sub_" + address[2:],
                signature="unknown", calling_convention="unknown", subsystem="unknown",
                confidence="high for address attribution only", status="Identified",
                record_path=method["record"], evidence_ids="E-0027-C01;E-0028-C01")
            content = (f"# Function {address} — attributed class entry\n\n"
                f"- Binary: SLUS_203.62; executable revision unknown; SHA-256 `{SHA}`.\n"
                "- Status: Identified; behaviour and original signature unknown; no original comparison.\n"
                f"- Original identifier: unknown; address-anchored `Sub_{address[2:]}`.\n"
                f"- Evidence: {method['evidence']}; derived class records or explicit member policy.\n\n"
                "## Confirmed\n\n"
                f"The entry is attributed to: {', '.join(c['name'] for c in attributed)}.\n"
                "See `reverse/classes.csv`, `reverse/classes/vtables.json`, `reverse/classes/template-policy.json` and the template inventory\n"
                "for constructor/slot attribution. Shared targets are not exclusive ownership.\n\n"
                "## Hypotheses\n\nNone adopted. No inheritance or field meaning is inferred.\n\n"
                "## Unresolved questions\n\n"
                "Original name, full signature, calling convention, fields and complete behaviour.\n"
                "A vtable target may lack a Ghidra-created function; template creation does not repair boundaries.\n\n"
                "## Continuation\n\n"
                f"Inspect {address} read-only in Ghidra; audit boundaries/callers before replacing the\n"
                "port-side template signature or throwing body. Add behavioural tests and retain addresses.\n")
            # A knowledge record can predate its symbols.csv row. Preserve it too.
            if not record.exists():
                write_text(method["record"], content)
            changed = True
        if not record.is_file():
            raise ValueError(f"Missing evidence record: {record}")
        row = symbols[address]
        locations = sorted({c["source"] for c in attributed})
        annotation = "TASK-0028 class entry coverage: " + "; ".join(locations) + " (templates/host projections; no status promotion)"
        current = row["pc_implementation"].split("TASK-0028 class entry coverage:", 1)[0].rstrip(" |")
        updated = (current + " | " if current and current != "none" else "") + annotation
        if row["pc_implementation"] != updated:
            if check:
                raise ValueError(f"Stale symbol implementation links: {address}")
            row["pc_implementation"] = updated
            changed = True
    if changed:
        # Preserve legacy duplicate/unrelated index rows verbatim rather than silently
        # collapsing them through the address lookup used for this scoped operation.
        index_path = ROOT / "reverse/symbols.csv"
        existing = read_csv("reverse/symbols.csv") if index_path.exists() else []
        present = set()
        for row in existing:
            address = row['elf_address'].upper().replace('0X', '0x')
            present.add(address)
            if address in owners:
                tail = symbols[address]['pc_implementation'].split('TASK-0028 class entry coverage:', 1)[1]
                prefix = row['pc_implementation'].split('TASK-0028 class entry coverage:', 1)[0].rstrip(' |')
                row['pc_implementation'] = ((prefix + ' | ') if prefix and prefix != 'none' else '') + 'TASK-0028 class entry coverage:' + tail
        existing.extend(row for address, row in symbols.items() if address not in present)
        stream = io.StringIO(newline="")
        writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(existing)
        write_text("reverse/symbols.csv", stream.getvalue())


def artifacts(classes: list[dict], labels: list[dict], symbols: dict[str, dict]) -> dict[str, str]:
    defs = definitions()
    inventory = {"schema_version": 1, "binary_id": BINARY, "version": "unknown executable revision",
                 "evidence": "E-0028-C01; E-0027-C01/C03; E-0019-N03", "classes": [],
                 "allocation_labels_only": labels}
    calls = []
    for cls in classes:
        item = {key: cls[key] for key in ("name", "namespace", "header", "source", "origin", "placement_provenance")}
        item.update(constructors=cls["constructors"], vtables=cls["vtables"],
                    diagnostic_labels=cls["labels"], members=[])
        for address in cls["functions"]:
            method = cls["methods"][address]
            candidates = defs.get(address, [])
            own, = entry_definitions(cls, address, method["name"], candidates)
            entry = {"elf_address": address, "method": method["name"], "record": method["record"],
                     "knowledge_status": symbols[address]["status"],
                     "coverage": "throwing_template" if own["stub"] else "existing_native_projection",
                     "signature": own["signature"], "signature_provenance": "port-side; original ABI unconfirmed",
                     "preserved_projections": [d for d in candidates if not d["stub"] and d != own]}
            if own["stub"] and entry["preserved_projections"]:
                entry["coverage"] = "throwing_template_with_preserved_partial_projection"
            item["members"].append(entry)
            if own["stub"]:
                calls.append((cls, address, method["name"]))
        item["coverage"] = "members_catalogued" if item["members"] else "unbound_class_name"
        inventory["classes"].append(item)
    sources = sorted({c["source"] for c in classes})
    cmake = "# TASK-0028. Explicit source list; includes empty shells to compile-check their headers.\n"
    cmake += "set(HP2_CLASS_TEMPLATE_SOURCES\n" + "".join(f"  {p}\n" for p in sources) + ")\n"
    test = "// TASK-0028 generated exception contract tests. No original-behaviour equivalence claim.\n"
    test += "".join(f'#include "{h[len("src/"):]}"\n' for h in sorted({c["header"] for c in classes}))
    test += '#include "game_core/port/not_implemented.hpp"\n#include <iostream>\n#include <string>\n\n'
    test += '''namespace {
template<class Body> bool expect_gap(const char* address, const char* member, Body&& body) {
    try { body(); }
    catch (const hp2::NotImplemented& error) {
        const std::string message = error.what();
        if (message.find(address) != std::string::npos && message.find(member) != std::string::npos) return true;
        std::cerr << "Wrong exception provenance: " << message << '\\n';
        return false;
    }
    catch (...) { std::cerr << "Wrong exception type: " << member << '\\n'; return false; }
    std::cerr << "Missing NotImplemented: " << member << '\\n';
    return false;
}
} // namespace
int main() {
    int failures = 0;
'''
    for cls, address, method in calls:
        name = cls["name"]
        setup = f"{cls['namespace']}::{name} instance;"
        if name in ("CopDriver", "CopMover"):
            setup = f"hp2::recovered::{name}Storage storage{{}}; hp2::recovered::{name} instance(storage);"
        test += (f'    failures += !expect_gap("{address}", "{name}::{method}", [] {{ '
                 f"{setup} instance.{method}(); }});\n")
    test += f'    std::cout << "CLASS_TEMPLATE_GAPS_CHECKED: {len(calls)}\\n";\n    return failures ? 1 : 0;\n}}\n'
    return {"reverse/classes/template-inventory.json": json.dumps(inventory, indent=2) + "\n",
            "reverse/classes/template_sources.cmake": cmake,
            "src/tests/class_template_tests.cpp": test}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true")
    mode.add_argument("--check", action="store_true")
    args = parser.parse_args()
    classes, labels, symbols, _ = model()
    fill_code(classes, args.check)
    synchronize_records(classes, symbols, args.check)
    for path, expected in artifacts(classes, labels, symbols).items():
        if args.check:
            if not (ROOT / path).exists() or (ROOT / path).read_text(encoding="utf-8") != expected:
                raise ValueError(f"Stale generated artifact: {path}; run --write")
        else:
            write_text(path, expected)
    print(f"CLASS_TEMPLATES_OK: {len(classes)} classes, {sum(len(c['functions']) for c in classes)} member attributions, {len(labels)} catalogue-only allocation labels")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

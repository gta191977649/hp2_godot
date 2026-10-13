"""TASK-0029 read-only Ghidra source-path sweep and conservative class-directory binding.

Raw responses and disassembly-derived instruction ownership stay under ignored local/.
Public output contains only derived paths/addresses, bounded candidates and observations.
No Ghidra rename, annotation, analysis or database mutation is performed.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from ghidra import Client
import recover_classes

SHA = "7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"
PRIVATE = ROOT / "local/task0029/source-paths"
PUBLIC = ROOT / "reverse/classes/source-path-bindings.json"


def unpack(result: dict):
    if result.get("isError"):
        raise RuntimeError(result)
    if "structuredContent" in result:
        return result["structuredContent"]
    blocks = [c.get("text", "") for c in result.get("content", []) if c.get("type") == "text"]
    try:
        return json.loads("\n".join(blocks))
    except ValueError:
        return {"data": "\n".join(blocks)}


def collect() -> None:
    PRIVATE.mkdir(parents=True, exist_ok=True)
    client = Client()
    available = client.call("tools/list", {})["tools"]
    names = {short: next(t["name"] for t in available if t["name"] == short or t["name"].endswith("_" + short))
             for short in ("memory", "inspect", "functions")}

    def query(tool: str, args: dict):
        args = dict(file_name="SLUS_203.62", **args)
        response = client.call("tools/call", {"name": names[tool], "arguments": args})
        filename = tool + "-" + args["action"] + "-" + args.get("address", "search") + "-" + str(args.get("cursor", "first")).replace(":", "_").replace("/", "_")
        (PRIVATE / (filename + ".json")).write_text(json.dumps({"query": args, "response": response}, indent=2))
        return unpack(response), response

    # One complete bounded regex search, paging to exhaustion. Includes all repeated header
    # strings, not just the first occurrence kept in the earlier path catalogue.
    args = dict(action="search", search_type="regex", search_value=r"(?:(?:\.{1,2}|[A-Za-z0-9_]+)[\\/])+[A-Za-z0-9_]+\.(?:cpp|hpp|h)",
                address_start="0x002AB000", address_end="0x003358F0", page_size=500)
    hits = []
    while True:
        data, raw = query("memory", args)
        hits.extend(data.get("data", []))
        cursor = data.get("next_cursor") or raw.get("next_cursor")
        if not cursor:
            break
        args["cursor"] = cursor
    records = []
    for index, hit in enumerate(hits):
        address = "0x" + hit["address"].upper().removeprefix("0X")
        length = hit["length"]
        data, _ = query("memory", dict(action="read", address=address, length=length + 1, format="hex"))
        raw_bytes = bytes.fromhex(data["hex_data"])
        path = raw_bytes[:length].decode("ascii")
        references = []
        ref_args = dict(action="references_to", address=address, page_size=500)
        while True:
            refs, raw = query("inspect", ref_args)
            text = refs.get("data", "")
            if not isinstance(text, str):
                raise ValueError("Unexpected references format")
            references.extend({"instruction": "0x" + m[1].upper(), "type": m[2]}
                              for m in re.finditer(r"(?m)^([0-9a-fA-F]{8})\s+(\S+)", text))
            cursor = refs.get("next_cursor") or raw.get("next_cursor")
            if not cursor:
                break
            ref_args["cursor"] = cursor
        records.append({"address": address, "path": path, "nul_terminated": raw_bytes[length] == 0,
                        "references": references})
        if (index + 1) % 50 == 0:
            print(f"SOURCE_PATH_SWEEP: {index + 1}/{len(hits)} strings", flush=True)
    (PRIVATE / "paths.json").write_text(json.dumps(records, indent=2) + "\n")
    (ROOT / 'reverse/classes/source-path-scan.json').write_text(json.dumps({
        'schema_version': 1, 'evidence_id': 'E-0029-S01', 'binary_id': 'sha256-' + SHA,
        'version': 'unknown executable revision', 'program': 'SLUS_203.62',
        'search_range': ['0x002AB000', '0x003358F0'],
        'search_pattern': args['search_value'], 'paths': records}, indent=2) + '\n')
    print(f"SOURCE_PATH_SWEEP_COMPLETE: {len(records)} occurrences / {len({r['path'] for r in records})} paths", flush=True)


def report() -> None:
    paths = json.loads((ROOT / 'reverse/classes/source-path-scan.json').read_text())['paths']
    current = json.loads((ROOT / "reverse/classes/template-inventory.json").read_text())
    policy = json.loads((ROOT / "reverse/classes/template-policy.json").read_text())
    all_classes = current["classes"]
    retained_modules = set(policy["existing_classes"]) | set(policy["module_placements"])
    moved = [c for c in all_classes if c["name"] not in retained_modules]
    decisions = {d['class_name']: d for d in json.loads((ROOT / 'reverse/classes/source-path-decisions.json').read_text())['decisions']}
    owners: dict[str, set[str]] = defaultdict(set)
    for cls in all_classes:
        for member in cls["members"]:
            owners[member["elf_address"]].add(cls["name"])
    instruction_owners: dict[int, set[str]] = defaultdict(set)
    # Exact instruction ownership; do not trust oversized inventory end-address envelopes.
    corpus = recover_classes.corpus()
    if not corpus:
        raise ValueError('Private instruction corpus is missing; retain the existing public ledger instead of manufacturing negative results')
    for entry, _, words in corpus:
        for address, _ in words:
            instruction_owners[address].add(f"0x{entry:08X}")
    candidates: dict[str, list[dict]] = defaultdict(list)
    for path in paths:
        for reference in path["references"]:
            entries = instruction_owners.get(int(reference["instruction"], 16), set())
            for entry in sorted(entries):
                attributed = owners.get(entry, set())
                for name in attributed:
                    candidates[name].append({"original_path": path["path"], "path_address": path["address"],
                        "reference_instruction": reference["instruction"], "reference_type": reference["type"],
                        "member_entry": entry, "member_class_owners": sorted(attributed),
                        "instruction_function_owners": sorted(entries), "nul_terminated": path["nul_terminated"]})
    result = {"schema_version": 1, "task": "TASK-0029", "binary_id": "sha256-" + SHA,
              "version": "unknown executable revision; SYSTEM.CNF VER 1.00", "program": "SLUS_203.62",
              "scope": "179 class pairs moved from port/class_templates; all source-path occurrences in canonical rodata/data range",
              "search_range": ["0x002AB000", "0x003358F0"],
              "source_path_occurrences": len(paths), "distinct_paths": len({r["path"] for r in paths}),
              "classes": []}
    for cls in moved:
        leads = candidates.get(cls["name"], [])
        row = {"class_name": cls["name"], "status": "unknown", "candidates": leads,
               "reason": "No confirmed class-specific source translation-unit binding; shared targets/header diagnostics are not ownership proof."}
        if cls['name'] in decisions:
            decision = decisions[cls['name']]
            if not any(p['path'] == decision['original_path'] and p['address'] == decision['path_address'] and p['nul_terminated'] for p in paths):
                raise ValueError('Reviewed path not present in fresh scan: ' + cls['name'])
            row.update(status='confirmed_directory', binding=decision, header=cls['header'], source=cls['source'])
            row['reason'] = decision['observation']
        result["classes"].append(row)
    PUBLIC.write_text(json.dumps(result, indent=2) + "\n")
    definite = []
    for cls in result["classes"]:
        direct = [c for c in cls["candidates"] if c["original_path"].endswith(".cpp") and
                  len(c["member_class_owners"]) == 1 and len(c["instruction_function_owners"]) == 1 and c["nul_terminated"]]
        if direct:
            definite.append((cls["class_name"], [(c["original_path"], c["member_entry"], c["reference_instruction"]) for c in direct]))
    print(json.dumps({"direct_cpp_candidates": definite,
                      "class_candidates": sum(bool(c['candidates']) for c in result['classes']),
                      "confirmed_directories": sum(c['status']=='confirmed_directory' for c in result['classes'])}, indent=1))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--collect", action="store_true")
    parser.add_argument("--report", action="store_true")
    args = parser.parse_args()
    if args.collect:
        collect()
    if args.report:
        report()


if __name__ == "__main__":
    main()

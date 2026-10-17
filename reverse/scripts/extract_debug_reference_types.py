"""TASK-0041: derive reference-only type names/fields from ELF ECOFF STABS.

No retail identity or layout is inferred. Input ELF bytes and raw STABS remain
outside Git. This deliberately bounded parser preserves incomplete coverage.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import unittest

EXPECTED_SHA = "815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b"
ROOT = Path(__file__).resolve().parents[2]
TYPE = re.compile(r"^([^:]+):Tt?(\d+)=([su])(\d+)(.*)$", re.S)
FIELD = re.compile(r"([^:;]+):(.+?),(-?\d+),(\d+);", re.S)


def local_strings(blob: bytes) -> tuple[int, bytes]:
    """Resolve ELF32 LE sections and ECOFF HDRR absolute local-string offsets."""
    if blob[:6] != b"\x7fELF\x01\x01":
        raise ValueError("Expected ELF32 little endian")
    shoff = struct.unpack_from("<I", blob, 32)[0]
    entsize, count, names_index = struct.unpack_from("<HHH", blob, 46)
    if entsize != 40 or shoff + count * entsize > len(blob):
        raise ValueError("Invalid ELF section table")
    sections = [struct.unpack_from("<10I", blob, shoff + i * 40) for i in range(count)]
    ns = sections[names_index]
    names = blob[ns[4]:ns[4] + ns[5]]
    for sec in sections:
        if names[sec[0]:].split(b"\0", 1)[0] != b".mdebug":
            continue
        start, size = sec[4:6]
        if start + size > len(blob) or size < 96:
            raise ValueError("Invalid .mdebug extent")
        header = struct.unpack_from("<HH23I", blob, start)
        if header[0] != 0x7009:
            raise ValueError("Unsupported ECOFF magic")
        length, offset = header[15:17]
        if offset < start or offset + length > start + size:
            raise ValueError("ECOFF local strings outside .mdebug")
        return offset, blob[offset:offset + length]
    raise ValueError("No .mdebug section")


def declarations(table: bytes, file_offset: int):
    """Join backslash continuations; retain first string's absolute file offset."""
    pending = b""
    pending_offset = 0
    offset = 0
    for fragment in table.split(b"\0"):
        if not pending:
            pending_offset = file_offset + offset
        pending += fragment
        offset += len(fragment) + 1
        if pending.endswith(b"\\"):
            pending = pending[:-1]
            continue
        text = pending.decode("ascii", errors="replace")
        pending = b""
        match = TYPE.match(text)
        if match:
            yield pending_offset, match


def parse_body(body: str):
    """Extract sequential top-level fields, stopping before method records.

    Complex inline aggregates are intentionally not interpreted. Method names
    are exact STABS tokens; operator/destructor encodings are not translated.
    Type IDs are compilation-unit-local and are deliberately not resolved.
    """
    method_at = re.search(r"[^:;]+::", body)
    data = body[:method_at.start()] if method_at else body
    methods = sorted(set(re.findall(r"(?:^|;)([^:;]+)::", body)))
    if re.search(r"=[su]\d+", data):
        return [], methods, "inline_aggregate_not_parsed"
    if data.startswith("!"):
        base = re.match(r"!(\d+),", data)
        if not base:
            return [], methods, "unparsed_base_prefix"
        data = data[base.end():]
        for _ in range(int(base[1])):
            end = data.find(";")
            if end < 0:
                return [], methods, "unparsed_base_prefix"
            data = data[end + 1:]
    fields = []
    pos = 0
    while pos < len(data) and data[pos:] not in ("", ";"):
        field = FIELD.match(data, pos)
        if not field:
            return fields, methods, "partial_fields"
        name, expr, bit_offset, bit_size = field.groups()
        # Visibility markers /0 /1 /2 belong to the type expression, not names.
        if any(c in name for c in "=,\\"):
            return fields, methods, "partial_fields"
        fields.append((name, int(bit_offset), int(bit_size)))
        pos = field.end()
    return fields, methods, "fields_parsed_type_ids_unresolved"


def extract(elf: Path, output: Path):
    blob = elf.read_bytes()
    sha = hashlib.sha256(blob).hexdigest()
    if sha != EXPECTED_SHA:
        raise ValueError("Input is not the authorized TASK-0041 debug reference")
    offset, table = local_strings(blob)
    variants = {}
    raw_declarations = set()
    for address, match in declarations(table, offset):
        name, _, kind, size, body = match.groups()
        fields, methods, coverage = parse_body(body)
        # Collapse identical extracted shapes, not compilation-unit-local IDs.
        raw_digest = hashlib.sha256(match.group().encode("ascii", errors="replace")).hexdigest()
        raw_declarations.add(raw_digest)
        canonical = [name, kind, int(size), fields, methods, coverage]
        digest = hashlib.sha256(json.dumps(canonical, separators=(",", ":")).encode()).hexdigest()
        key = (name, digest)
        if key not in variants:
            variants[key] = dict(name=name, declaration_sha256=digest, first_raw_declaration_sha256=raw_digest,
                kind={"s": "struct_or_class", "u": "union"}[kind], size_bytes=int(size),
                first_file_offset=f"0x{address:x}", occurrences=0, fields=fields,
                method_tokens=methods, coverage=coverage)
        variants[key]["occurrences"] += 1
    output.mkdir(parents=True, exist_ok=True)
    records = sorted(variants.values(), key=lambda r: (r["name"], r["declaration_sha256"]))
    for filename, columns, rows in (
        ("types.csv", ["name", "declaration_sha256", "first_raw_declaration_sha256", "kind", "size_bytes", "first_file_offset", "occurrences", "coverage"],
         [{k: r[k] for k in ("name", "declaration_sha256", "first_raw_declaration_sha256", "kind", "size_bytes", "first_file_offset", "occurrences", "coverage")} for r in records]),
        ("type_fields.csv", ["type_name", "declaration_sha256", "field_name", "bit_offset", "bit_size"],
         [dict(type_name=r["name"], declaration_sha256=r["declaration_sha256"], field_name=n, bit_offset=o, bit_size=s) for r in records for n, o, s in r["fields"]]),
        ("type_method_tokens.csv", ["type_name", "declaration_sha256", "method_token"],
         [dict(type_name=r["name"], declaration_sha256=r["declaration_sha256"], method_token=m) for r in records for m in r["method_tokens"]]),
    ):
        with (output / filename).open("w", encoding="utf-8", newline="") as handle:
            writer = csv.DictWriter(handle, fieldnames=columns, lineterminator="\n")
            writer.writeheader()
            writer.writerows(rows)
    summary = dict(evidence_id="E-0041-T01", debug_elf_sha256=sha,
        scope="Debug reference only; no retail equivalence or Verified behavior", 
        local_string_table_file_offset=f"0x{offset:x}", local_string_table_size=len(table),
        declaration_variants=len(records), raw_declaration_variants=len(raw_declarations),
        variant_identity="declaration_sha256 hashes extracted name/kind/size/fields/methods/coverage; first_raw_declaration_sha256 hashes original STABS declaration", unique_type_names=len({r["name"] for r in records}),
        declaration_occurrences=sum(r["occurrences"] for r in records),
        field_rows=sum(len(r["fields"]) for r in records),
        method_token_rows=sum(len(r["method_tokens"]) for r in records),
        limitations=["Only named :T / :Tt s/u declarations; enums, typedefs and anonymous types excluded",
          "STABS IDs are compilation-unit-local; derived-shape identity excludes type IDs and full type expressions",
          "Distinct source types can share one extracted shape; first raw declaration is representative only",
          "Nested inline aggregates skipped; partial fields explicitly marked",
          "Inheritance offsets, field types, signatures and method addresses not resolved",
          "Method tokens retain compiler encodings; not all overloads are enumerated",
          "Debug layout does not establish retail layout; no automatic retail rename"])
    (output / "type_summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


class ParserTests(unittest.TestCase):
    def test_ecoff_uses_absolute_string_offset(self):
        blob = bytearray(512)
        blob[:6] = b"\x7fELF\x01\x01"
        struct.pack_into("<I", blob, 32, 52)
        struct.pack_into("<HHH", blob, 46, 40, 3, 1)
        names = b"\0.shstrtab\0.mdebug\0"
        blob[200:200 + len(names)] = names
        struct.pack_into("<10I", blob, 92, 1, 3, 0, 0, 200, len(names), 0, 0, 1, 0)
        struct.pack_into("<10I", blob, 132, 11, 0x70000005, 0, 0, 300, 112, 0, 0, 4, 0)
        header = [0] * 25
        header[0], header[15], header[16] = 0x7009, 4, 396
        struct.pack_into("<HH23I", blob, 300, *header)
        blob[396:400] = b"abc\0"
        self.assertEqual(local_strings(blob), (396, b"abc\0"))
        struct.pack_into("<I", blob, 300 + 4 + 14 * 4, 511)
        with self.assertRaises(ValueError):
            local_strings(blob)

    def test_continuation_and_offsets(self):
        rows = list(declarations(b"X:Tt1=s8a:1,0,32;\\\0b:1,32,32;;\0", 100))
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0][0], 100)
        self.assertEqual(parse_body(rows[0][1][5])[0], [("a", 0, 32), ("b", 32, 32)])

    def test_base_array_and_method(self):
        fields, methods, status = parse_body("!1,020,2;x:ar1;0;3;4,64,128;Run::5:;2A.;;")
        self.assertEqual(fields, [("x", 64, 128)])
        self.assertEqual(methods, ["Run"])
        self.assertTrue(status.startswith("fields_parsed"))

    def test_nested_aggregate_is_not_flattened(self):
        self.assertEqual(parse_body("x:2=s4inner:1,0,32;;,0,32;;")[2], "inline_aggregate_not_parsed")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "reverse" / "reference" / ("sha256-" + EXPECTED_SHA))
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        unittest.main(argv=["extract_debug_reference_types"], exit=True)
    elif args.elf:
        extract(args.elf, args.output)
    else:
        parser.error("Supply --elf or --self-test")

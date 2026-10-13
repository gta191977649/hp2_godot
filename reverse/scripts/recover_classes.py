"""Recover the binary's classes: their names, their vtables, and the functions inside them.

Two independent axes, because the binary supports two and they answer different questions.

**Names.** The ELF is stripped -- no symtab, no DWARF, and `analyze list_rtti` finds no game
classes -- but .rodata still holds profiler and diagnostic labels of the form
`Class::Method()`. Those give real, verbatim class and method names (AGENT.md R2 "Recovered").
What they do *not* give is addresses: a label only names a method if some code hands that label
to a call. `--bind` scans every instruction word in the local disassembly corpus for the
lui/addiu or lui/ori pair that materializes each label's address, which is the complete test --
and it finds that almost none of them are referenced, because the instrumentation that used
them was compiled out of the retail build. Those names are live strings for a dead code path.

**Vtables.** A class with virtual methods has a vtable, and its constructor materializes the
vtable address and stores it into the object. `--vtables` mines the corpus for exactly that
shape -- a .rodata constant completed into a register and then stored -- and then reads each
candidate out of the binary to confirm it is a run of function pointers. That yields real
classes with real member addresses, named `Type_<ADDR>` per AGENT.md R2 because the vtable's
own name is not in the binary.

The vtable layout here is the old GCC/ARM one: 8-byte entries of {int16 delta, ..., void* fn},
with the pointer at +4 of each entry, which is why the recovered front-end code reads
`vtable + 0xC` for the first slot and `*(short *)(vtable + 8)` for its delta.

    py -3 reverse/scripts/recover_classes.py --bind       name -> address binding (offline)
    py -3 reverse/scripts/recover_classes.py --vtables    vtable discovery (needs Ghidra MCP)
    py -3 reverse/scripts/recover_classes.py --emit       write reverse/classes.csv

Offline passes read only repository files. `--vtables` reads the binary through the read-only
Ghidra client `tools/ghidra.py` (AGENT.md R5: inspect/memory only, never annotate).
"""
from __future__ import annotations

import argparse
import csv
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CORPUS = ROOT / "ps2_decomp" / "recompiled-named"
FUNCTIONS = ROOT / "ps2_decomp" / "ghidra_functions.csv"
QUALIFIED = ROOT / "reverse" / "naming" / "task0019-qualified-string-starts.csv"
OUT = ROOT / "reverse" / "classes"

# From `memory list_blocks` on SLUS_203.62, sha256 7335b8ed...facc2.
TEXT = (0x00100000, 0x002A5903)
RODATA = (0x002AB000, 0x002C72BD)
DATA = (0x002C7380, 0x0032D8C7)

# The corpus prints instruction words without leading zeros, so a fixed 8-digit match
# silently drops every R-type, j and jal (all of which are below 0x10000000). Dropping those
# would also disable register invalidation on R-type writes and make stale lui halves pair with
# unrelated low halves, so the width has to be variable.
INSTR = re.compile(r"//\s*0x([0-9a-f]+):\s*0x([0-9a-f]{1,8})\s")


def sx16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def in_range(value: int, bounds: tuple[int, int]) -> bool:
    return bounds[0] <= value <= bounds[1]


def scan(words: list[tuple[int, int]]) -> tuple[dict[int, list[int]], set[int]]:
    """(constant -> completing instruction addresses, constants that were then stored).

    A register's tracked high half is dropped as soon as anything else writes that register, so
    a stale `lui` cannot be paired with an unrelated low half. The "stored" set is the vtable
    signature: a pointer constant that is written into memory rather than used as an argument.
    """
    hi: dict[int, int] = {}
    live: dict[int, int] = {}          # register -> constant currently held
    found: dict[int, list[int]] = defaultdict(list)
    stored: set[int] = set()
    for address, word in words:
        op = word >> 26
        rs = (word >> 21) & 0x1F
        rt = (word >> 16) & 0x1F
        imm = word & 0xFFFF
        if op == 0x0F:                 # lui rt, imm
            hi[rt] = imm
            live.pop(rt, None)
            continue
        if op in (0x09, 0x0D) and rs in hi:   # addiu / ori off a tracked lui
            value = ((hi[rs] << 16) + sx16(imm)) & 0xFFFFFFFF if op == 0x09 \
                else ((hi[rs] << 16) | imm) & 0xFFFFFFFF
            found[value].append(address)
            live[rt] = value
            if rt != rs:
                hi.pop(rt, None)
            continue
        if op in (0x28, 0x29, 0x2B):   # sb / sh / sw rt, imm(rs)
            if rt in live:
                stored.add(live[rt])
            continue
        if op == 0:                    # R-type writes rd
            rd = (word >> 11) & 0x1F
            hi.pop(rd, None)
            live.pop(rd, None)
        elif op in (0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17, 0x02, 0x03):
            pass                        # branches and jumps write no GPR we track
        else:
            hi.pop(rt, None)
            live.pop(rt, None)
    return found, stored


def corpus() -> list[tuple[int, str, list[tuple[int, int]]]]:
    out = []
    for path in sorted(CORPUS.glob("*.cpp")):
        text = path.read_text(encoding="utf-8", errors="replace")
        words = [(int(m.group(1), 16), int(m.group(2), 16)) for m in INSTR.finditer(text)]
        if words:
            out.append((min(a for a, _ in words), path.name, words))
    return out


def entries() -> set[int]:
    """Every function entry address Ghidra knows, used to test whether a word is a method."""
    out = set()
    with FUNCTIONS.open(encoding="utf-8-sig", newline="") as handle:
        for row in csv.DictReader(handle):
            for key in ("start_address", "address", "entry_point"):
                if row.get(key):
                    out.add(int(row[key], 16))
                    break
            else:
                values = [v for v in row.values() if v and v.startswith("0x")]
                if values:
                    out.add(int(values[0], 16))
    return out


def bind() -> dict:
    wanted: dict[int, str] = {}
    with QUALIFIED.open(encoding="utf-8-sig", newline="") as handle:
        for row in csv.DictReader(handle):
            wanted[int(row["full_string_start"], 16)] = row["qualified_text"]
    hits: dict[str, list[dict]] = defaultdict(list)
    for entry, name, words in corpus():
        found, _ = scan(words)
        for address in set(found) & set(wanted):
            hits[wanted[address]].append({"function": f"0x{entry:08X}", "file": name,
                                          "sites": [f"0x{s:08X}" for s in sorted(found[address])]})
    return {"labels": len(wanted), "bound": dict(hits),
            "unbound": sorted(set(wanted.values()) - set(hits))}


def candidates() -> dict[int, list[str]]:
    """.rodata/.data constants that some function stores into memory: the vtable signature."""
    out: dict[int, list[str]] = defaultdict(list)
    for entry, _name, words in corpus():
        _found, stored = scan(words)
        for value in stored:
            if in_range(value, RODATA) or in_range(value, DATA):
                out[value].append(f"0x{entry:08X}")
    return dict(out)


def fetch_rodata() -> dict[int, int]:
    """.rodata as {address: word}, cached under the gitignored local/ tree.

    AGENT.md R7: raw bytes of the original image never enter Git, so the cache lives in local/
    and only derived addresses are written to reverse/. Ghidra lists undefined data one byte per
    line, so words are reassembled here (little-endian, as `memory list_blocks` reports).
    """
    cache = ROOT / "local" / "classes" / "rodata.json"
    if cache.exists():
        raw = json.loads(cache.read_text(encoding="utf-8"))
        return {int(k): v for k, v in raw.items()}

    sys.path.insert(0, str(ROOT / "tools"))
    from ghidra import Client                      # noqa: E402  read-only client
    client = Client()
    # Every listing row is "<address> <byte tokens...> <rendering>". One row can carry one byte
    # (undefined), four (a defined pointer) or a whole string's bytes, so the bytes have to be
    # taken as the leading run of two-hex-digit tokens; the rendering column that follows is
    # text such as `00h`, `00131474` or `"CopDriver"` and never matches that shape.
    head = re.compile(r"^([0-9a-f]{6,8})" + chr(32) + r"+(.*)$")

    data: dict[int, int] = {}
    address = RODATA[0]
    while address <= RODATA[1]:
        end = min(address + 0xF00, RODATA[1])
        result = client.call("tools/call", {"name": "inspect", "arguments": {
            "file_name": "SLUS_203.62", "action": "listing", "address": f"0x{address:08X}",
            "end_address": f"0x{end:08X}", "max_lines": 1000}})
        text = "".join(part.get("text", "") for part in result.get("content", []))
        seen = 0
        for row in text.splitlines():
            match = head.match(row.strip())
            if not match:
                continue
            base = int(match.group(1), 16)
            for offset, token in enumerate(match.group(2).split()):
                if len(token) != 2:
                    break
                try:
                    value = int(token, 16)
                except ValueError:
                    break
                data[base + offset] = value
                seen += 1
        if not seen:
            print(f"  no bytes returned at 0x{address:08X}; stopping", file=sys.stderr)
            break
        address = max(data) + 1
        print(f"  .rodata {len(data)}/{RODATA[1] - RODATA[0] + 1} bytes", file=sys.stderr)

    words = {}
    for base in range(RODATA[0], RODATA[1] - 2, 4):
        parts = [data.get(base + i) for i in range(4)]
        if None not in parts:
            words[base] = parts[0] | (parts[1] << 8) | (parts[2] << 16) | (parts[3] << 24)
    cache.parent.mkdir(parents=True, exist_ok=True)
    cache.write_text(json.dumps({str(k): v for k, v in words.items()}), encoding="utf-8")
    return words


def vtables() -> tuple[list[dict], list[dict]]:
    """Confirm each candidate is a vtable and list the member functions it holds.

    Old GCC/ARM layout, read straight off the recovered front-end code: the first slot's
    function pointer is at `vtable + 0xC` and its `this` delta is the int16 at `vtable + 8`
    (`001F1DD8` message 0x0C calls `(**(code **)(vtable + 0xC))(this + *(short *)(vtable + 8), 3)`
    and `001F1B08` message 0x12 does the same at +0x10/+0x14). Entries are 8 bytes, so slot k
    has its delta at `vtable + 8 + 8k` and its pointer at `vtable + 0xC + 8k`. The two words at
    `vtable + 0` and `+4` are the header and are not slots -- which is why a scan starting at
    `+4` finds nothing but zero.

    A candidate is accepted when at least two consecutive slots hold a word that is a known
    function entry. One slot alone is too weak: an isolated stored .rodata pointer followed by
    something that happens to look like a function address would pass. Those are counted and
    reported rather than claimed.
    """
    known = entries()
    words = fetch_rodata()
    accepted, weak = [], []
    for base, constructors in sorted(candidates().items()):
        if not in_range(base, RODATA):
            continue
        slots = []
        for index in range(0, 64):
            pointer = words.get(base + 0xC + index * 8)
            # The structural test is "a 4-aligned address inside .text". Whether Ghidra has
            # *created* a function there is a property of that database, not of the binary --
            # several of these targets are uncreated, which is exactly why the front-end work
            # had to read their listings instead of decompiling them. That state is recorded
            # per member rather than used to reject the slot.
            if pointer is None or pointer % 4 or not in_range(pointer, TEXT):
                break
            delta = words.get(base + 8 + index * 8, 0) & 0xFFFF
            signed = delta - 0x10000 if delta & 0x8000 else delta
            # A real slot's `this` delta is a small object offset. Across every slot accepted
            # here the observed values are only 0, -8 and -256, so a large one means the words
            # are not a vtable at all; random data would spread across the full 16 bits.
            if abs(signed) > 0x400:
                break
            slots.append({"slot": index, "offset": f"0x{0xC + index * 8:X}",
                          "function": f"0x{pointer:08X}",
                          "this_delta": delta - 0x10000 if delta & 0x8000 else delta,
                          "ghidra_function_created": pointer in known})
        if not slots:
            continue
        record = {"vtable": f"0x{base:08X}", "slots": len(slots),
                  "constructors": sorted(constructors), "members": slots}
        (accepted if len(slots) >= 2 else weak).append(record)
    return accepted, weak


def rodata_strings(minimum: int = 3) -> dict[int, str]:
    """NUL-terminated printable ASCII in .rodata, as {start address: text}.

    Built from the cached word map rather than a fresh read, so this pass is offline once the
    cache exists. Only the start of each string is keyed: that is the address code materializes.
    """
    words = fetch_rodata()
    data: dict[int, int] = {}
    for base, word in words.items():
        for index in range(4):
            data[base + index] = (word >> (8 * index)) & 0xFF
    out: dict[int, str] = {}
    start: int | None = None
    buffer: list[str] = []
    for address in range(min(data), max(data) + 2):
        byte = data.get(address)
        if byte is not None and 0x20 <= byte < 0x7F:
            if start is None:
                start = address
                buffer = []
            buffer.append(chr(byte))
            continue
        if start is not None and byte == 0 and len(buffer) >= minimum:
            out[start] = "".join(buffer)
        start = None
        buffer = []
    return out


# FUN_002577C0(size, label): the allocator the game calls with a debug name for the object it
# is about to build. docs/POLICE_AI_MAP.md found it independently at 0x00131410, where a
# `CopDriver` label at 0x002AE4E8 precedes the call to the constructor that writes vtable
# 0x002B5998. That shape -- label, allocate, construct -- is what binds a recovered class name
# to a concrete address, and it is the only mechanism in this stripped image that does.
NAMED_ALLOCATOR = 0x002577C0
A0, A1 = 4, 5


def jal_target(word: int) -> int | None:
    return ((word & 0x03FFFFFF) << 2) if (word >> 26) == 0x03 else None


def allocations(strings: dict[int, str]) -> list[dict]:
    """Every FUN_002577C0 call site, with the label in $a1 and the next call after it.

    MIPS executes the delay-slot instruction before the branch is taken, and the compiler
    routinely puts the low half of the label address there, so the instruction after each `jal`
    is applied before the argument registers are read.
    """
    out = []
    for entry, _name, words in corpus():
        hi: dict[int, int] = {}
        live: dict[int, int] = {}
        small: dict[int, int] = {}
        pending: list[dict] = []

        def step(word: int) -> None:
            op = word >> 26
            rs = (word >> 21) & 0x1F
            rt = (word >> 16) & 0x1F
            imm = word & 0xFFFF
            if op == 0x0F:
                hi[rt] = imm
                live.pop(rt, None)
                small.pop(rt, None)
            elif op in (0x09, 0x0D) and rs in hi:
                live[rt] = ((hi[rs] << 16) + sx16(imm)) & 0xFFFFFFFF if op == 0x09                     else ((hi[rs] << 16) | imm) & 0xFFFFFFFF
                small.pop(rt, None)
                if rt != rs:
                    hi.pop(rt, None)
            elif op == 0x09 and rs == 0:
                small[rt] = sx16(imm)
                live.pop(rt, None)
                hi.pop(rt, None)
            elif op == 0:
                rd = (word >> 11) & 0x1F
                for store in (hi, live, small):
                    store.pop(rd, None)
            elif op not in (0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17,
                            0x28, 0x29, 0x2B, 0x02, 0x03):
                for store in (hi, live, small):
                    store.pop(rt, None)

        for index, (address, word) in enumerate(words):
            target = jal_target(word)
            if target is None:
                step(word)
                continue
            if index + 1 < len(words):
                step(words[index + 1][1])      # delay slot runs before the call
            if target == NAMED_ALLOCATOR:
                label = live.get(A1)
                pending.append({"caller": f"0x{entry:08X}", "site": f"0x{address:08X}",
                                "label_address": f"0x{label:08X}" if label else None,
                                "label": strings.get(label or 0),
                                "size": small.get(A0), "next_call": None})
            elif pending and pending[-1]["next_call"] is None:
                pending[-1]["next_call"] = f"0x{target:08X}"
            live.pop(A0, None)
            live.pop(A1, None)
            small.pop(A0, None)
            small.pop(A1, None)
        out.extend(pending)
    return out


# Allocator labels are also used for plain file-scope allocations, where the label is the
# original __FILE__ path rather than a class name. Those belong to docs/ORIGINAL_SOURCE_TREE.md,
# not here, and are excluded by shape rather than by a hand-kept list.
def is_class_label(label: str) -> bool:
    lowered = label.lower()
    if lowered.endswith((".cpp", ".hpp", ".c", ".h", ".bun", ".elf", ".bin")):
        return False
    return not any(ch in label for ch in ("\\", "/", ":"))


def classes() -> list[dict]:
    """Join label -> constructor -> vtable -> members, then score each class against the records.

    The object size comes from the allocation site, the constructor from the call that follows
    it, the vtable from what that constructor stores, and the members from the vtable. Every
    link is read out of the binary; none of it is inferred from naming or from role.
    """
    confirmed, _weak = vtables()
    by_constructor: dict[str, list[dict]] = defaultdict(list)
    for table in confirmed:
        for constructor in table["constructors"]:
            by_constructor[constructor].append(table)

    records = {}
    with (ROOT / "reverse" / "symbols.csv").open(encoding="utf-8-sig", newline="") as handle:
        for row in csv.DictReader(handle):
            records[row["elf_address"].upper()] = row

    grouped: dict[str, list[dict]] = defaultdict(list)
    for row in allocations(rodata_strings()):
        if row["label"] and is_class_label(row["label"]):
            grouped[row["label"]].append(row)

    out = []
    for label, rows in sorted(grouped.items()):
        # The call immediately after the allocation is *usually* the constructor, but an
        # argument can be computed by a call in between, so it is only claimed as the
        # constructor when that function is independently seen storing a vtable into an object.
        # tEventTreeView is the example that forces this: the call after its allocation is
        # 00185568, which reads the profile's selected mode rather than constructing anything.
        first_calls = sorted({r["next_call"] for r in rows if r["next_call"]})
        constructors = [c for c in first_calls if c in by_constructor]
        sizes = sorted({r["size"] for r in rows if r["size"]})
        tables, members = [], []
        for constructor in constructors:
            for table in by_constructor.get(constructor, []):
                if table["vtable"] not in tables:
                    tables.append(table["vtable"])
                    members.extend(m["function"] for m in table["members"])
        functions = sorted(set(constructors) | set(members))
        known = [records[f.upper()] for f in functions if f.upper() in records]
        out.append({
            "class": label,
            "allocation_sites": len(rows),
            "object_sizes": sizes,
            "first_calls": first_calls,
            "constructors": constructors,
            "vtables": tables,
            "functions": functions,
            "records": len(known),
            "implemented": sum(1 for r in known if r["status"] in ("Implemented", "Tested")),
            "tested": sum(1 for r in known if r["status"] == "Tested"),
        })
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bind", action="store_true", help="label -> address binding, offline")
    parser.add_argument("--candidates", action="store_true", help="count vtable candidates, offline")
    parser.add_argument("--allocations", action="store_true",
                        help="named-allocator call sites: class label -> constructor")
    parser.add_argument("--emit", action="store_true", help="write reverse/classes.csv")
    parser.add_argument("--vtables", action="store_true",
                        help="confirm candidates against the binary and list their members")
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)

    if args.bind:
        result = bind()
        (OUT / "name-binding.json").write_text(json.dumps(result, indent=1), encoding="utf-8")
        print(f"{result['labels']} labels, {len(result['bound'])} bound, "
              f"{len(result['unbound'])} with no reference anywhere in the image")
        return 0

    if args.candidates:
        found = candidates()
        print(f"{len(found)} distinct stored .rodata/.data constants")
        rodata = {k: v for k, v in found.items() if in_range(k, RODATA)}
        print(f"{len(rodata)} of them in .rodata (where the known vtables live)")
        (OUT / "vtable-candidates.json").write_text(
            json.dumps({f"0x{k:08X}": v for k, v in sorted(rodata.items())}, indent=1),
            encoding="utf-8")
        return 0

    if args.emit:
        rows = classes()
        target = ROOT / "reverse" / "classes.csv"
        with target.open("w", encoding="utf-8", newline="") as handle:
            writer = csv.writer(handle)
            writer.writerow(["binary_id", "class_name", "name_provenance", "allocation_sites",
                             "object_sizes", "constructors", "first_call_after_allocation",
                             "vtables", "functions", "functions_with_record",
                             "functions_implemented", "functions_tested", "evidence_id"])
            for row in rows:
                writer.writerow([
                    "sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2",
                    row["class"], "allocator label (FUN_002577C0 argument 2), recovered verbatim",
                    row["allocation_sites"], " ".join(str(s) for s in row["object_sizes"]),
                    " ".join(row["constructors"]), " ".join(row["first_calls"]),
                    " ".join(row["vtables"]),
                    " ".join(row["functions"]), row["records"], row["implemented"],
                    row["tested"], "E-0027-C01"])
        with_vtable = sum(1 for r in rows if r["vtables"])
        print(f"{len(rows)} classes written to reverse/classes.csv; "
              f"{with_vtable} have a vtable, "
              f"{sum(len(r['functions']) for r in rows)} member functions identified, "
              f"{sum(r['records'] for r in rows)} of those have a record")
        return 0

    if args.allocations:
        found = allocations(rodata_strings())
        named = [row for row in found if row["label"]]
        (OUT / "allocations.json").write_text(json.dumps(found, indent=1), encoding="utf-8")
        print(f"{len(found)} named-allocator call sites, {len(named)} with a readable label, "
              f"{len({row['label'] for row in named})} distinct class labels")
        return 0

    if args.vtables:
        found, weak = vtables()
        members = sum(len(v["members"]) for v in found)
        unique = {m["function"] for v in found for m in v["members"]}
        (OUT / "vtables.json").write_text(
            json.dumps({"confirmed": found, "single_slot_unconfirmed": weak}, indent=1),
            encoding="utf-8")
        print(f"{len(found)} vtables confirmed ({members} member slots, "
              f"{len(unique)} distinct member functions); "
              f"{len(weak)} single-slot candidates left unconfirmed")
        return 0

    parser.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""Regenerate the tracked tables in docs/PROGRESS.md, and the contents list in docs/PROGRESS_LOG.md.

Modelled on gta-reversed's docs/ReversedClasses.md, which is produced from hooks.json every
time hooks change rather than being edited by hand. The same reasoning applies here: counts
written by hand go stale silently, and a progress number nobody can reproduce is worse than no
number at all. The inputs are the repository's structured records:

    reverse/symbols.csv                     one row per function with a knowledge record
    reverse/structures/<binary-id>/*.md     one file per recovered structure
    docs/assessment/FUNCTION_INVENTORY.csv  every function in the ELF (the denominator)
    reverse/binaries.csv                    binary identity
    reverse/classes.csv                     the recovered classes and their member functions
    src/**/*.{cpp,hpp}                      the hp2::not_implemented call sites (the frontier)

Only the regions between the `generated:` markers are rewritten; everything else in both files
is hand-written and is left exactly as it is. The regions are `stats`, `boundaries` and `tasks`
in docs/PROGRESS.md, and `contents` in docs/PROGRESS_LOG.md.

    py -3 reverse/scripts/generate_progress.py            rewrite the generated regions
    py -3 reverse/scripts/generate_progress.py --check     fail if they are out of date

This reads repository files only. It never touches the ELF, Ghidra or the asset tree.
"""
from __future__ import annotations

import argparse
import csv
import datetime
import re
import sys
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROGRESS = ROOT / "docs" / "PROGRESS.md"
LOG = ROOT / "docs" / "PROGRESS_LOG.md"
SYMBOLS = ROOT / "reverse" / "symbols.csv"
BINARIES = ROOT / "reverse" / "binaries.csv"
INVENTORY = ROOT / "docs" / "assessment" / "FUNCTION_INVENTORY.csv"
TASKS = ROOT / "tasks"

# The record status ladder, weakest first. "Verified" is deliberately included even though
# nothing has reached it: AGENT.md R1 reserves it for a recorded comparison against the
# original, and showing it empty is the point. Port-side tests never promote to it.
LADDER = ["Identified", "Decompiled", "Implemented", "Tested", "Verified"]
LADDER_MEANING = {
    "Identified": "address and role recorded; no decompilation kept",
    "Decompiled": "behaviour read out of the ELF and written down",
    "Implemented": "translated into `src/`, with the ELF address on the code",
    "Tested": "translated and covered by host tests",
    "Verified": "compared against the original — **nothing is here, by rule**",
}

BEGIN = "<!-- generated:{0}:begin -->"
END = "<!-- generated:{0}:end -->"


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def percent(part: int, whole: int) -> str:
    if whole == 0:
        return "n/a"
    value = 100.0 * part / whole
    return f"{value:.1f}%" if value < 10 else f"{value:.0f}%"


def wrap(text: str, width: int = 96) -> list[str]:
    """Wrap generated prose to the width the rest of docs/ uses, so diffs stay readable."""
    return textwrap.wrap(text, width=width) or [""]


def structure_records() -> tuple[int, int]:
    """(records inside the binary-id layout, records still at the top level).

    AGENT.md R6 puts structure records under reverse/structures/<binary-id>/<subsystem>/. Two
    pre-existing records sit directly in reverse/structures/ instead; they are counted and
    reported separately rather than hidden, because the split is itself a thing to fix.
    TEMPLATE.md and EXAMPLE_SYNTHETIC.md are infrastructure with no original provenance.
    """
    root = ROOT / "reverse" / "structures"
    ignore = {"TEMPLATE.md", "EXAMPLE_SYNTHETIC.md"}
    nested = len([p for p in root.glob("*/**/*.md") if p.name not in ignore])
    loose = len([p for p in root.glob("*.md") if p.name not in ignore])
    return nested, loose


def render_stats() -> str:
    symbols = read_csv(SYMBOLS)
    inventory = read_csv(INVENTORY)
    binaries = {row["binary_id"]: row for row in read_csv(BINARIES)}

    total = len(inventory)
    records = len(symbols)
    by_status: dict[str, list[dict[str, str]]] = {name: [] for name in LADDER}
    unknown_status: list[dict[str, str]] = []
    for row in symbols:
        by_status.setdefault(row["status"], unknown_status if row["status"] not in by_status else [])
        if row["status"] in by_status:
            by_status[row["status"]].append(row)
        else:
            unknown_status.append(row)

    subsystems = sorted({row["subsystem"] for row in symbols})
    lines: list[str] = []

    identity = ", ".join(
        f"`{row['sha256'][:12]}…` ({row['version']})" for row in binaries.values()
    )
    lines.append(f"### Functions with a record: {records} of {total} in the ELF [{percent(records, total)}]")
    lines.append("")
    lines.append(f"Binary: {identity or 'none recorded'}.")
    lines.append("")
    lines += wrap(
        "The denominator is every function `docs/assessment/FUNCTION_INVENTORY.csv` found in the "
        "ELF, including library and CRT code the port will never need, so treat it as a floor on "
        "how much is left rather than a measure of how much matters. Read the disclaimers above "
        "before quoting either number."
    )
    lines.append("")

    # Status ladder, strongest first: that is the order a reader cares about.
    lines.append("| Status | Records | Share of records | Meaning |")
    lines.append("|---|---|---|---|")
    for name in reversed(LADDER):
        rows = by_status.get(name, [])
        lines.append(
            f"| {name} | {len(rows)} | {percent(len(rows), records)} | {LADDER_MEANING[name]} |"
        )
    if unknown_status:
        names = ", ".join(sorted({row["status"] for row in unknown_status}))
        lines.append(
            f"| _off-ladder_ | {len(unknown_status)} | {percent(len(unknown_status), records)} "
            f"| status values not on the ladder: {names} |"
        )
    lines.append("")

    # Per subsystem, which is the closest thing this project has to gta-reversed's categories.
    # There is no per-subsystem denominator: FUNCTION_INVENTORY.csv classifies 7525 of its rows
    # as Unknown, so a per-subsystem percentage would be invented. Counts only.
    lines.append("#### By subsystem")
    lines.append("")
    lines.append("| Subsystem | Records | " + " | ".join(reversed(LADDER)) + " |")
    lines.append("|---" * (len(LADDER) + 2) + "|")
    for subsystem in subsystems:
        rows = [row for row in symbols if row["subsystem"] == subsystem]
        cells = [str(sum(1 for row in rows if row["status"] == name)) for name in reversed(LADDER)]
        lines.append(f"| {subsystem} | {len(rows)} | " + " | ".join(cells) + " |")
    cells = [str(len(by_status.get(name, []))) for name in reversed(LADDER)]
    lines.append(f"| **total** | **{records}** | " + " | ".join(f"**{cell}**" for cell in cells) + " |")
    lines.append("")

    nested, loose = structure_records()
    total_structures = nested + loose
    note = (f"Structure layout records: **{total_structures}**, against {records} function "
            f"records -- roughly {max(1, round(records / max(1, total_structures)))} to one. That "
            "ratio is the honest shape of the gap: a translated function whose structure layout "
            "is unknown is still guesswork at the field level.")
    if loose:
        note += (f" {loose} of them still sit directly in `reverse/structures/` rather than the "
                 "`<binary-id>/<subsystem>/` layout AGENT.md R6 specifies.")
    lines += wrap(note)
    return "\n".join(lines)


# AGENT.md R3: every untranslated recovered path calls hp2::not_implemented(elf_address, what),
# so the set of those call sites *is* the frontier. Nothing else in the tree knows it, which is
# why this is read out of the source instead of being maintained as a list by hand.
NOT_IMPLEMENTED = re.compile(
    r"""(?<![A-Za-z0-9_])not_implemented\(\s*(?:"(?P<address>[^"]*)"|(?P<expr>[^,]+?))\s*,\s*"(?P<what>[^"]*)\"""",
    re.DOTALL,
)
SOURCE_GLOBS = ("src/game_core/**/*.cpp", "src/game_core/**/*.hpp",
                "src/port/**/*.cpp", "src/port/**/*.hpp",
                "src/platform/**/*.cpp", "src/platform/**/*.hpp")


def boundary_rows() -> list[tuple[str, str, str]]:
    """(address, what, source location) for every live not_implemented call site.

    Tests are excluded: a test that asserts a boundary throws is not itself a gap. Sites whose
    address is a runtime expression rather than a literal are reported as such instead of being
    dropped, because dropping them would understate the frontier.
    """
    rows: list[tuple[str, str, str]] = []
    for pattern in SOURCE_GLOBS:
        for path in sorted(ROOT.glob(pattern)):
            if path.name == "not_implemented.hpp":
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            rel = path.relative_to(ROOT).as_posix()
            for match in NOT_IMPLEMENTED.finditer(text):
                line = text.count("\n", 0, match.start()) + 1
                address = match.group("address")
                if address is None:
                    address = "(runtime value: " + " ".join(match.group("expr").split()) + ")"
                what = " ".join(match.group("what").split())
                rows.append((address, what, f"{rel}:{line}"))
    return rows


def render_boundaries() -> str:
    rows = boundary_rows()
    literal = sorted({row[0] for row in rows if row[0].startswith("0x")})
    lines = [
        f"### Explicit untranslated boundaries: {len(rows)} call sites, "
        f"{len(literal)} distinct ELF addresses",
        "",
        *wrap("Each row is a recovered path that is reached but not translated, and that "
              "therefore throws `hp2::NotImplemented` naming its ELF address (AGENT.md R3). This "
              "is the work queue: a path leaves this table by being reversed, never by being "
              "given a default."),
        "",
        "<details>",
        "<summary>See all call sites</summary>",
        "",
        "| ELF address | What is missing | Call site |",
        "|---|---|---|",
    ]
    for address, what, where in sorted(rows):
        lines.append(f"| `{address}` | {what} | [{where}](../{where}) |")
    lines += ["", "</details>"]
    return "\n".join(lines)


CLASSES = ROOT / "reverse" / "classes.csv"


def render_classes() -> str:
    """One row per recovered class, bucketed by how far its member functions have got.

    Source: reverse/classes.csv, produced by reverse/scripts/recover_classes.py. Every column is
    read out of the binary -- the name from the allocator label, the object size from the
    allocation site, the constructor from the call that follows it when that function is also
    seen storing a vtable, and the member functions from the vtable's slots.
    """
    if not CLASSES.exists():
        return "_reverse/classes.csv is missing; run `py -3 reverse/scripts/recover_classes.py --emit`._"
    rows = read_csv(CLASSES)
    for row in rows:
        row["_functions"] = len(row["functions"].split())
        row["_records"] = int(row["functions_with_record"] or 0)
        row["_implemented"] = int(row["functions_implemented"] or 0)
        row["_tested"] = int(row["functions_tested"] or 0)
        row["_vtables"] = len(row["vtables"].split())

    total_functions = sum(row["_functions"] for row in rows)
    with_vtable = [row for row in rows if row["_vtables"]]
    lines = [
        f"### Classes: {len(rows)} names recovered, {len(with_vtable)} with a vtable, "
        f"{total_functions} member functions attributed",
        "",
    ]
    lines += wrap(
        "\"Attributed\" counts only members of a vtable reached from a named class whose "
        "constructor is corroborated. 252 confirmed vtables hold 1577 slots over 791 distinct "
        "functions in total (reverse/classes/vtables.json); the rest belong to classes this "
        "method cannot name. See docs/CLASS_RECOVERY.md."
    )
    lines.append("")
    lines += wrap(
        "Class names are recovered verbatim (AGENT.md R2) from the second argument of the named "
        "allocator `FUN_002577C0(size, label)`, which the game calls with the name of the object "
        "it is about to build. The object size is that call's first argument. Where the call "
        "right after the allocation is also seen storing a vtable, it is the constructor, and "
        "the vtable's slots give the class's virtual members. Everything in this table is read "
        "out of the binary; nothing is inferred from naming or role."
    )
    lines.append("")
    owners: dict[str, set[str]] = {}
    for row in rows:
        for table in row["vtables"].split():
            owners.setdefault(table, set()).add(row["class_name"])
    shared = {t: names for t, names in owners.items() if len(names) > 1}
    if shared:
        lines += wrap(
            f"{len(shared)} of the {len(owners)} vtables found are attributed to more than one "
            "class. That is the signature of a shared base: each derived constructor writes the "
            "base vtable too, and label variants of one class (the `DriveCameraMover(...)` rows) "
            "legitimately share one. For those classes the member list includes inherited slots, "
            "so their function count is attributed, not exclusive."
        )
        lines.append("")
    lines += wrap(
        "What this table does **not** show: a class whose objects are built through the plain "
        "allocator rather than the named one has no label to recover, so it is absent entirely "
        "-- the Language Options screen is one such, built with `FUN_00257690(0x10, 0x10)`. "
        "Non-virtual members are not listed either: nothing in a stripped image ties a "
        "non-virtual function to its class. So a class's identified-function count is a floor, "
        "and 0 means nothing has been bound yet, not that the class has no methods."
    )
    lines.append("")

    buckets = [
        ("Classes with translated functions",
         [r for r in rows if r["_implemented"]],
         "At least one member function is translated into `src/` with its ELF address on it."),
        ("Classes with recorded functions, none translated yet",
         [r for r in rows if not r["_implemented"] and r["_records"]],
         "Behaviour of at least one member has been read out of the ELF and written down."),
        ("Classes identified by name only",
         [r for r in rows if not r["_records"]],
         "Name, object size and -- where a vtable was found -- member addresses are known. No "
         "member has a record yet. This is where most of the binary still is."),
    ]
    for heading, group, note in buckets:
        share = percent(len(group), len(rows))
        lines.append(f"#### {heading} ({len(group)}/{len(rows)}) [{share}]")
        lines.append("")
        lines += wrap(note)
        lines.append("")
        if not group:
            lines.append("None.")
            lines.append("")
            continue
        group = sorted(group, key=lambda r: (-r["_implemented"], -r["_records"],
                                             -r["_functions"], r["class_name"]))
        collapse = len(group) > 12
        if collapse:
            lines.append("<details>")
            lines.append("<summary>See the full list</summary>")
            lines.append("")
        lines.append("| Class | Object size | Functions | Record | Implemented | Tested | vtable |")
        lines.append("|---|---|---|---|---|---|---|")
        for row in group:
            sizes = ", ".join(row["object_sizes"].split()) or "unknown"
            vtable = ", ".join(f"`{v}`" for v in row["vtables"].split()) or "not found"
            lines.append(
                f"| `{row['class_name']}` | {sizes} | {row['_functions']} | {row['_records']} "
                f"| {row['_implemented']} | {row['_tested']} | {vtable} |")
        lines.append("")
        if collapse:
            lines.append("</details>")
            lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def render_log_contents(log_text: str) -> str:
    """A contents list for the log, from its own second-level headings.

    The log is append-at-the-top and already long; without an index a reader has to scroll to
    find out what is in it. GitHub's heading anchors are lowercased, with punctuation dropped and
    spaces turned into hyphens, which is what this reproduces.
    """
    lines = []
    for line in log_text.splitlines():
        if not line.startswith("## ") or line.startswith("## Contents"):
            continue
        title = line[3:].strip()
        anchor = re.sub(r"[^a-z0-9 -]", "", title.lower()).replace(" ", "-")
        lines.append(f"- [{title}](#{anchor})")
    return "\n".join(lines) or "_No sections yet._"


def splice(text: str, name: str, body: str) -> str:
    begin, end = BEGIN.format(name), END.format(name)
    if begin not in text or end not in text:
        raise SystemExit(f"docs/PROGRESS.md is missing the {name} generated markers")
    head, rest = text.split(begin, 1)
    _, tail = rest.split(end, 1)
    stamp = datetime.date.today().isoformat()
    note = f"<!-- Generated by reverse/scripts/generate_progress.py on {stamp}. Do not edit by hand. -->"
    return f"{head}{begin}\n{note}\n\n{body.rstrip()}\n\n{end}{tail}"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true",
                        help="exit non-zero when any generated region is stale")
    args = parser.parse_args()

    original = PROGRESS.read_text(encoding="utf-8")
    updated = splice(original, "stats", render_stats())
    updated = splice(updated, "boundaries", render_boundaries())
    updated = splice(updated, "classes", render_classes())

    log_original = LOG.read_text(encoding="utf-8")
    log_updated = splice(log_original, "contents", render_log_contents(log_original))

    def bodies(text: str) -> list[str]:
        """The generated regions present in `text`, with the date stamp removed.

        The stamp changes every day, so comparing raw text would report a stale file for a
        repository nobody touched.
        """
        out = []
        for name in ("stats", "boundaries", "classes", "contents"):
            if BEGIN.format(name) not in text:
                continue
            section = text.split(BEGIN.format(name), 1)[1].split(END.format(name), 1)[0]
            out.append("\n".join(line for line in section.splitlines()
                                 if not line.startswith("<!-- Generated by")).strip())
        return out

    work = [(PROGRESS, original, updated, "docs/PROGRESS.md tables"),
            (LOG, log_original, log_updated, "docs/PROGRESS_LOG.md contents")]

    if args.check:
        stale = [label for _, before, after, label in work if bodies(before) != bodies(after)]
        if stale:
            print("stale, run `py -3 reverse/scripts/generate_progress.py`: "
                  + ", ".join(stale), file=sys.stderr)
            return 1
        print("PASS generated regions are current")
        return 0

    wrote = []
    for path, before, after, label in work:
        if bodies(before) == bodies(after):
            continue
        path.write_text(after, encoding="utf-8", newline="\n")
        wrote.append(label)
    print("UPDATED " + ", ".join(wrote) if wrote else "PASS generated regions already current")
    return 0



if __name__ == "__main__":
    raise SystemExit(main())

# AGENT.md — naming and evidence rules (binding)

This file is **binding** for every agent and human working in this repository. It sits above
`AGENTS.md` (session/process contract) and narrows one thing specifically: **where identifiers and
behaviour are allowed to come from**. `AGENTS.md` still applies in full; where the two overlap,
the stricter of the two wins.

The repository is a reverse-engineering port of *Need for Speed: Hot Pursuit 2* (PS2,
`SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`). The goal is behaviour
identical to the original game. Anything that is not traceable to the original binary or the
original game data is a liability, not a feature.

## R0. Two sources of truth, and nothing else

Code and documentation in this repository may only assert something about the original game when
it came from one of these:

1. **The authorized ELF, read through Ghidra / GhidraMCP** (`tools/ghidra.py`, read-only):
   decompilation, disassembly listings, memory reads, cross-references, jump tables, embedded
   strings, static data.
2. **The player's own original game data** (the configured ZZDATA tree — see R7): byte-level
   analysis of the shipped asset files.

Anything else — community wikis, other ports, analogy with other games, "this is how menus
usually work", an LLM's prior knowledge of HP2 — is **not** evidence. It may be used as a *lead to
go and check in Ghidra*, must be cited as such, and must never reach a code identifier, a struct
field, or a `Confirmed` sentence in a document.

The REA skill (`reverse-engineer-anything`) defines the investigation workflow: route the target,
work summary-first, keep observations / inferences / unknowns separate, cite stable evidence IDs.
Follow it. Do not promote confidence because a port-side test passed.

## R1. No guesswork. Unknown is a valid, required answer

- Never invent a field meaning, an enum value, a timing constant, a colour, an easing curve, or a
  state transition.
- When something is not recovered, say so in the place where a reader would otherwise assume it
  was recovered: the header comment, the doc section, the task file.
- A plausible reconstruction is a **hypothesis**. Label it. Hypotheses may not be presented as
  recovered behaviour, and may not silently become the basis of further work.
- `Verified` requires a recorded comparison against the original (hardware or PCSX2) over a
  stated scope. Port-side unit tests never produce `Verified`.

## R2. Identifier provenance is mandatory

Every type, function, constant, enum and field name in recovered code falls into exactly one of
three buckets, and the bucket must be obvious from the name itself:

| Bucket | When | Form | Example |
|---|---|---|---|
| **Recovered** | The exact identifier text was read out of the ELF or the game data | Use it **verbatim**, including original casing and underscores | `JOY_EVENT_MENU_SELECT`, `MENUITEM_%d`, `HIGHLIGHT_BAR`, `GAMEFLOW_STATE_IN_FRONTEND`, `TexturePack`, `World::DoTimestep` |
| **Address-anchored** | Behaviour is recovered but the original identifier is not | Append the ELF entry address: `Sub_<ADDR>` for a function, `Type_<ADDR>` for a type, `Field_0x<off>` for a struct field | `MenuList_00169F78`, `HandleJoyEvent_0016A458`, `Field_0x5c` |
| **Port-side** | No original counterpart exists (Godot glue, build tooling, test harness) | Must live under a `port::` namespace or a `Ps2*`/`*_observer` Godot class, and must be documented as port-side | `port::godot_bridge::Ps2FrontEndLayout` |

Rules that follow from this:

- **Never invent a descriptive English name for recovered code.** `FeMenu::MoveDown()` is
  forbidden; `MenuList_00169F78::Next_0016A5E0()` is correct. Descriptive wording belongs in the
  doc comment, where it is clearly an interpretation.
- When a recovered name later turns up for an address-anchored symbol, rename to the recovered
  name and keep the address in the doc comment and in `reverse/symbols.csv`.
- Source **file names and directory structure** follow the original source tree recovered from the
  ELF's `__FILE__` strings (`docs/ORIGINAL_SOURCE_TREE.md`). The front-end menu framework lives in
  `fmenus.*` because the ELF embeds `..\indep\src\fmenus.cpp`. Per-screen logic belongs in files
  mirroring `indep\src\frontendscreens\{selectcar,racesetup,options,joystickconfiguration}.cpp`.
  Do not create a new top-level layout that has no original counterpart.
- Every recovered function carries, in its doc comment, its **ELF address** and the evidence ID or
  record path backing it. A reader must be able to go from any line of recovered code to the
  Ghidra address it came from without searching.

## R3. Not implemented must fail loudly, never silently

If a recovered code path is reached but has not been implemented, it must raise
`hp2::NotImplemented` (`src/game_core/not_implemented.hpp`) via `HP2_NOT_IMPLEMENTED(...)`, naming
the ELF address and what is missing. Do not:

- return a default value and carry on,
- substitute a plausible approximation,
- quietly no-op.

A silent fallback is indistinguishable from recovered behaviour at runtime, which is exactly the
failure mode these rules exist to prevent. Keep the throw site at the boundary of the missing
work, so the message points at the function that still needs reversing.

## R4. Behaviour comes from the binary, not from taste

- Do not "improve" recovered logic: keep the original's wrap-around, its modulo, its off-by-one,
  its traversal order, its magic numbers.
- Do not replace recovered maths with an engine facility (Godot physics, Godot `Tween`,
  Godot theming) without an explicit recorded decision in `docs/DECISIONS.md`.
- Recovered logic stays free of Godot headers and types (`AGENTS.md` rule 7). The Godot side is a
  thin, clearly-labelled port-side adapter.
- Constants read out of the ELF keep their original representation and units (PS2 colour alpha
  range `0..0x80`, raw float bits, 640×480 front-end space).

## R5. Ghidra access is read-only

No annotations, renames, comments, bookmarks, data-type edits or function-boundary changes in the
Ghidra project. Analysis state belongs in this repository (`reverse/`), not in the database. Use
`inspect`, `memory` (read/search), `functions get/list`, `symbols list`, `analyze`. Never
`annotate` or `delete`.

## R5b. Everything written in this repository is in English

Documentation, code comments, identifier names, task files, evidence records, commit messages and
test failure strings are all written in English, without exception. The repository is the durable
project memory and has to stay readable to every later reader and tool; a document that is only
legible to one reader is a document the next session cannot verify against the binary.

This covers the `docs/`, `tasks/` and `reverse/` trees as well as source comments. Quoted material
keeps its original text: an ELF string, an asset filename, a localised line pulled out of the game
or a snippet of the user's own words stays verbatim, in quotes, with an English explanation
around it. Chat replies are not covered by this rule; answer the user in whatever language they
are writing in.

## R6. Record what you found, where the next session will look

A finding that only exists in a chat transcript does not exist. For each recovered function or
structure: a record under `reverse/functions/<binary-id>/<subsystem>/<address>.md` or
`reverse/structures/...`, a row in `reverse/symbols.csv`, and a stable evidence ID
(`E-<task>-NNN`). Keep confirmed findings, hypotheses and open questions in separate sections, and
keep counterevidence when a hypothesis is retired — do not delete the retired claim, mark it.

## R7. Proprietary material never enters Git

No ELF, ISO, Ghidra database, extracted asset, decoded texture, save file or captured frame in the
repository. The original asset tree is resolved at runtime through the single global entry point —
`HP2_PS2_ASSET_ROOT`, else `config/ps2_assets.local.json` (gitignored) — via
`tools/ps2_assets.py` (Python) and `src/platform/ps2_asset_root.hpp` (native). Never hardcode a
ZZDATA path anywhere else. Assets are decoded by the port's own recovered loader at runtime; no
pre-export step, no checked-in converted copies.

## R8. Known debt against these rules

These rules were written on 2026-10-12, after TASK-0011..0013 had already landed. Pre-existing
port-side names that R2 would now forbid are tracked for retrofit, not grandfathered silently:

- `hp2::frontend::ScreenManager` and its `request_screen` / `go_back` / `reset` members are
  invented descriptive names for `FUN_0014CC78` / `FUN_0016C4C0` / `FUN_0016C8C8` /
  `FUN_0016B418` / `FUN_0016B438` / `FUN_0016B460`.
- `hp2::frontend::parse_screens` / `parse_fonts` / `FeWidget` / `FeScreen` / `FeFont` / `FeGlyph`
  are port-side names for the asset-format reader; the original reader functions have not been
  located in the ELF yet.

New code must comply. Touching one of the above is an opportunity to rename it, not a licence to
add more.

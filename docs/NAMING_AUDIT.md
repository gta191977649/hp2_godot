# TASK-0016 — Original source/identifier audit

> TASK-0019 follow-up: embedded qualified identifiers must be cross-referenced
> at the beginning of their whole C string, not only the regex match offset.
> This recovered five original method/address mappings, including the previously
> tentative DemoDiscManager::Init association. See
> [FUNCTION_NAME_RECOVERY.md](FUNCTION_NAME_RECOVERY.md). TASK-0016 negative
> method-reference results were bounded offset queries, not proof of absent names.

Evidence E-0016-N01, 2026-10-12. Target US PS2 `SLUS_203.62`, revision unknown,
SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Read-only Ghidra MCP. No symbol, prototype, comment or function boundary changed.

## What the fresh sweep establishes

The canonical address range `00100000..003358F0` contains 43 `.cpp` path
occurrences, 246 `.hpp`, and nine `.h`: 298 path-pattern matches, 54 distinct
byte strings. All 43 `.cpp` strings are distinct. The earlier
ORIGINAL_SOURCE_TREE report's `.hpp` count of 50 was incomplete; this pass uses
page_size 500 and exhausts the returned result without a cursor. Mirrored EE
address aliases are excluded so they cannot inflate counts.

The `Class::member` text pattern returned 119 matches. One is the nested suffix
`nager::HandleSnapshot()` inside `ReplayManager::HandleSnapshot()` and is
excluded from the meaningful count: 118 nonoverlapping matches, 99 distinct
texts. This is a bounded qualified-name string sweep, not a complete original
symbol table. Names without `::`, RTTI, exports, assembly labels and indirect
function naming require their own evidence. `bFile::Filename` is a field
expression; matching `::` does not make a string a method.

The public address/text/reference catalog is
[task0016-original-identifiers.csv](../reverse/naming/task0016-original-identifiers.csv).
It retains nested/odd byte-run matches and marks them rather than silently
correcting the original text. For example `AbWare/inc/bWare.hpp` is an observed
byte run, not a proposed canonical source pathname.

## Cross-reference correction

Fresh `inspect references_to` on all 43 `.cpp` addresses and 119 qualified-name
match addresses found references for 40 source paths and one method-text address.
Most are Ghidra `PARAM` references, meaning a literal is passed at a call site.
This disproves the earlier blanket statement that all source strings are orphans.
It does not prove the absence of references at the remaining three strings:

| Original path | String address | Current references |
|---|---|---|
| `..\indep\src\fmenus.cpp` | `002B27F8` | none reported |
| `..\indep\src\lzcompress.cpp` | `002B6558` | none reported |
| `..\indep\src\movieplayer\movieplayer.cpp` | `002BDD08` | none reported |

Positive examples: `sound.cpp` at `002B62E8` is passed at `001AE754`, inside
`001AE608`, to an allocation call with source line `0x184`; `src/bEffect.cpp`
at `002C3698` is passed by `00270370` to assertion function `00250AC0` with
source line `0x14B`. Those provide local file provenance for those call sites.
They do not license assigning an entire address region to that source file.

`DemoDiscManager::Init()` at `002ADA80` has a `PARAM` reference at `0012E704`;
decompile confirms containing function `0012E6C8` logs that literal and parses
demo-disc options. This is a strong method association candidate, retained as
metadata. No function rename is applied in this task.

The remaining strings, including `CopMover::Move()`, `CopDriver::Think()`,
`World::DoTimestep()`, `Race::Update()`, and AIAction/Chopper method text, remain
string evidence. They must not replace address-anchored symbols merely because
the string describes behavior that looks similar.

## Current C++ source comparison

At the audit snapshot there are 33 `.cpp` files under `src/`, including tests
and host adapters. Four basenames match original paths:

| Current file | Exact original source tail | Assessment |
|---|---|---|
| `game_core/frontend/fmenus.cpp` | `indep/src/fmenus.cpp` | matches; per-function mapping still requires evidence |
| `game_core/frontend/frontendscreens/options.cpp` | `indep/src/frontendscreens/options.cpp` | matches; mixed main-menu handler provenance must stay explicit |
| `game_core/movieplayer/movieplayer.cpp` | `indep/src/movieplayer/movieplayer.cpp` | matches decoder source basename and directory tail |
| `game_core/audio/sound.cpp` | `indep/src/sound.cpp` | name exists; current VAG helper is port-side, not recovered bSound bodies |

Godot bridge classes (`Ps2MoviePlayer`, `Ps2FrontEndLayout`, `Ps2TextureLoader`)
and tests are port-side; their filenames are not expected to appear in the ELF.
The FFmpeg-backed AVI decoder is likewise a host backend, even though its source
is placed next to the original movieplayer subsystem. Naming does not promote
that backend to original reconstruction or Verified status.

The original middleware source names are `bDriverPS2.cpp`, `bEffect.cpp`,
`bRegion.cpp`, `bSample.cpp`, `bSegment.cpp`, `bSound.cpp`, `bVoice.cpp`, and
`bJukebox.cpp`. Recovering their literals does not justify renaming a generic
port-side PCM/VAG utility to `bSound.cpp`: the utility implements a host asset
format seam, not the middleware class lifecycle.

Recommended scoped cleanup, subject to actual owner integration:

- Title constructor/update/activation recovery was moved into `fmenus.*`
  during integration; `Type_0016F000` and `Sub_0016EE90` now follow the frontend
  source grouping. AVI container and FFmpeg services remain explicitly port-side
  in `movieplayer.*`. The public API is preserved for the media bridge/tests.
- Existing `screen_manager.*` contains invented `ScreenManager` names already
  listed as AGENT.md R8 debt. Move/rename using known `0016C4C0`/history addresses
  or an explicit port-side projection; do not invent an original class name.
- `cop_control.*`, `engine_shell.*` and `world_connections.*` are partial
  recovery/observer groupings. No corresponding original filename was found in
  this bounded scan. Keep their port versus recovered identity clear and defer
  source-path migration until file/address evidence is available.
- Asset parsers `elab_chunks.*`, `hp2_tpk_texture.*`, `fe_language.hpp` and the
  input/registry catalogs are port reader/data projections. Do not assign
  original middleware or class names based solely on a matching data format.

No automatic global rename is warranted by this audit. The repository requires
verbatim recovered names where associated, otherwise address-anchored recovered
identifiers and explicit `port::` or Ps2 host identifiers. A complete retrofit of
old naming debt remains separate from the startup/media implementation.

## Reproduce and continue

Ghidra MCP `memory search`, regex, canonical range above, page_size 500:
`[A-Za-z0-9_.\\/\-]+\.(cpp|hpp|h)(?=\x00|[^A-Za-z0-9_])` for source paths;
`[A-Za-z_][A-Za-z0-9_]*::[A-Za-z_~][A-Za-z0-9_]*(\(\))?` for qualified text.
Read result ranges with `memory read` (hex), joining any 4096-byte block boundary
before decoding. Query every `.cpp` and method-text address with `inspect
references_to`; decompile selected positive call sites to distinguish assertion,
allocation and logging references. Raw search/memory/reference/decompile results
are private under ignored `local/task0016/naming/`.

Next action for original-name adoption: select one catalog row, confirm its
referencing instruction/function and the string's role, add a per-function
record and original_identifier mapping, then perform the scoped rename with
address retained. Do not infer code identity from mere adjacent strings.

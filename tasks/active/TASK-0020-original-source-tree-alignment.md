# TASK-0020 — Align game_core with the original HP2 source tree

Status: active. User asked that `src/game_core`'s structure, file names, folder names,
class names and function names match the original HP2 project as closely as possible,
using the REA skill workflow and a string scan of the authorized ELF.

## Tooling reality

**Ghidra MCP is not connected to this session.** No Ghidra or REA MCP tool is registered
(`ToolSearch` for both returns nothing), so no cross-reference, decompilation or
call-site query was available. Earlier sessions (TASK-0016, TASK-0019) did have it.

A string scan does not need it. The authorized ELF is present locally at
`D:/ps2_game/SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`, which matches
`reverse/binaries.csv` exactly. A read-only scanner parses the two PT_LOAD program
headers and maps every printable-ASCII run's file offset to its ELF virtual address, so
each hit is address-anchored per AGENT.md R2. The ELF is read, never executed.

Scan result: 8,153 strings; **296 source-path hits, 54 distinct**. That corroborates the
TASK-0016 rescan recorded in NAMING_AUDIT.md (298 hits / 54 distinct) and supersedes the
original truncated page in ORIGINAL_SOURCE_TREE.md. Also 74 distinct `Class::Method`
strings and 23 `t`-prefixed pool-name strings.

Catalogue: `reverse/naming/task0020-original-source-tree.csv` (E-0020-N01 paths,
E-0020-N02 qualified identifiers, E-0020-N03 pool names). Every row is `string_only`:
a string's presence is not proof of which function or file it belongs to.

## What the evidence does and does not support

**Supported — file and folder names.** The compile-time paths are verbatim original
strings. Each file below has its own address, so moving our file to that path is
recovered-verbatim naming, not invention:

| original path | address | our file |
|---|---|---|
| `..\indep\src\fmenus.cpp` | 0x002B27F8 | `frontend/fmenus.cpp` |
| `..\indep\src\eventtree.cpp` | 0x002BA9F8 | `frontend/eventtree.cpp` |
| `..\indep\src\sound.cpp` | 0x002B62E8 | `audio/sound.cpp` |
| `..\indep\src\frontendscreens\options.cpp` | 0x002B9E00 | `frontend/frontendscreens/options.cpp` |
| `..\indep\src\frontendscreens\joystickconfiguration.cpp` | 0x002BE300 | `frontend/frontendscreens/joystickconfiguration.cpp` |
| `..\indep\src\movieplayer\movieplayer.cpp` | 0x002BDD08 | `movieplayer/movieplayer.cpp` |
| `src/bJukebox.cpp` + `include/bJukebox.h` | 0x002C4220 / 0x002C4208 | `audio/bJukebox.{cpp,hpp}` |

The original also names files we have not recovered at all (`car.cpp`,
`racecoordinator.cpp`, `cameramover.cpp`, `trackpath.cpp`, `lzcompress.cpp`,
`movieplayer\{cyclicbuffer,dctimage,ringbuffer}.cpp`, `speech\sedwedge.cpp`, the rest of
the bWare/bSound middleware). Those are future destinations, not files to create empty.

**Not supported — function and class renaming.** The 74 `Class::Method` strings and 23
pool names are real original identifiers, but every one is `string_only`: none is pinned
to an address of the function it names. TASK-0019 already settled this for exactly this
case -- "allocation labels are not type/method proof". Pinning needs cross-references,
which needs Ghidra MCP. So `Sub_<ADDR>` / `Type_<ADDR>` identifiers stay as they are
(AGENT.md R13: address-anchored is correct; a guessed descriptive name is not).

Tempting but refused without a call-site: `tExitScreenAnimation` (0x002B9BB0) for
`Type_002490D8`, `tMenuAnimElement` (0x002BF000) for `MenuHighlightAnim_00248870`,
`tOptionsScreen` / `tDisplayOptionsScreen` for the options screens. TASK-0019 explicitly
left 0x002490D8's identifier unknown; this scan does not change that.

**Namespaces are unchanged.** The original identifiers are plain classes
(`World::DoTimestep`), with no namespace evidence either way. `hp2::` / `port::` are
port-side scaffolding that AGENT.md's provenance rules do not reach, and changing them
would be churn without evidence.

## Layout decision

Files with a recovered original path move to it under `src/game_core/`. Files without one
cannot be made "consistent with the original" -- we do not know where they lived -- so
putting them beside the recovered paths would assert provenance we do not have. They move
under `src/game_core/port/` instead, which states plainly that they are port-side:

```text
src/game_core/
├── indep/src/                     # verbatim original paths (table above)
│   ├── fmenus.{cpp,hpp}
│   ├── eventtree.{cpp,hpp}
│   ├── sound.{cpp,hpp}
│   ├── frontendscreens/{options.{cpp,hpp},joystickconfiguration.cpp}
│   └── movieplayer/movieplayer.{cpp,hpp}
├── bSound/                        # bWare bSound middleware
│   ├── include/bJukebox.h
│   └── src/bJukebox.cpp
└── port/                          # no original path recovered; port-side
    ├── assets/{elab_chunks,hp2_tpk_texture,fe_language}
    ├── engine/{engine_shell,world_connections,call_sites.inc}
    ├── frontend/{screen_manager,frontendscreens/{modeselect,registry_0016adb0}}
    ├── input/joyevents
    ├── movieplayer/movie_audio
    ├── police/cop_control
    └── {scaffold,demo,not_implemented}
```

Headers keep the extension their original path gives them where one exists
(`bJukebox.h`, not `.hpp`). For the `indep` files only `.cpp` paths were recovered, so
their headers keep `.hpp`; that extension is port-side and is noted in each file.

The `bSound/` prefix on `bJukebox.cpp` is inferred, not verbatim: the recovered string is
`src/bJukebox.cpp`, and `bSound/include/bFragmentedVector2.hpp` (0x002AE1D0) establishes
`bSound` as the component root with `include/` beside `src/`. Recorded as inference.

## Acceptance criteria

- [x] Every moved file sits at a path backed by a recovered string, with the address in a comment.
- [x] Files without recovered paths are under `port/` and say so (26 files).
- [x] No identifier renamed without an address-anchored pin; `Sub_`/`Type_` names unchanged.
- [x] `ctest --preset native-debug` and every Godot check pass unchanged.
- [x] Evidence catalogue committed; ORIGINAL_SOURCE_TREE.md and ARCHITECTURE.md updated.
      NAMING_AUDIT.md left alone: its TASK-0016 counts are corroborated, not changed.

## Result

39 files moved with `git mv`, so history follows them. Includes rewritten in 53 files;
`CMakeLists.txt`, `reverse/scripts/validate_main_loop.py`,
`reverse/scripts/catalog_full_recovery.py` and `tools/pipeline.py` updated for the new
paths -- the `main_loop_catalog` test caught the first of those, which is what it is for.

Two cross-directory includes needed explicit paths after `bSound` split into `src/` and
`include/`, matching how the original's own assert text refers to `include/bJukebox.h`.

Verification: `ctest --preset native-debug` **22/22**; `--native-render-smoke`
(41 screens / 1085 quads / 7765 glyphs / 120 textures), `--runtime-smoke`,
`main.tscn --smoke-test`, `frontend_menu_observer --frontend-menu-smoke` and
`main_loop_observer --observer-smoke` all pass unchanged. Both presets build with no
warnings.

Drive-by fix: `docs/PROGRESS.md` held a corrupted copy of the original path
`indep\src\fmenus.cpp`: it was stored with a literal form-feed where the `\f`
belonged. It predates this task (present in HEAD) and is a corrupted record of original
evidence, so it was corrected rather than left.

## Next steps after this task

1. Reconnect Ghidra MCP, then pin the 74 `Class::Method` and 23 pool strings to addresses
   via cross-references so function/class renaming becomes evidence-backed.
2. The continuation hint already in ORIGINAL_SOURCE_TREE.md still stands: find the
   `{file, line, expr}` assert-record table and work backwards from it to batch-classify
   functions into their original source files.

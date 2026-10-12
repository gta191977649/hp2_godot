# TASK-0016 scrolling, localisation and background evidence

Target: US PS2 SLUS_203.62, revision unknown, ELF SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only Ghidra MCP via tools/ghidra.py Client, 2026-10-12. Raw decompilation
and memory reads are ignored under local/task0016/scroll/. No original execution comparison.

## E-0016-S01: localised text is numeric-ID lookup

FUN_0021E560 calls FUN_0021E2E8 using the language object at 0031C1C8.
0021E2E8 binary-searches eight-byte key/offset records from header +0x5C,
using count +0x50; matched text is header +0x60 plus record offset.
FUN_0021E0D0 loads the selected LANGUAGES/FE_*.BIN file through the original
file loader. FUN_0021E560 returns the exact ASCII fallback `NOT TRANSLATED PROPERLY` at
002BC958 when the ID search fails. DAT_002FC1C4 initializes to zero, but active
language startup selection remains unresolved here.
The host default of English is an explicit port preference, not a recovered default.

The configured player's LANGUAGES/FE_ENGLISH.BIN has SHA-256
8a05ce92e8a7d7a19e24e816c7ae90c7c71cbdd374c27bbe8db44a7b13af62d3,
107576 bytes. It is one 0x39000 chunk. After its 0x64-byte payload header,
two tables of 2437 eight-byte records precede a 68475-byte string blob.
Numeric IDs are sorted in the second table. Stored pointer words are not
portable file offsets; the port reader derives contiguous table/blob offsets.
All table string offsets validate within the blob. Strings contain byte-font
indices, including non-ASCII bytes; do not reinterpret as UTF-8 glyph indices.
The reader lives under hp2::port::assets; it is a port data reader rather than
a claim that the original chunk loader itself has been translated.

## E-0016-S02: scrolling hint lifecycle

FUN_002491B0 constructs the screen-specific object with SCROLLING_TEXT hash
75980CF0, and WHITEOUT_BAR 70A404EC. FUN_00144140 snapshots authored X.
Its path is initialized at X=650, ends at authored X, and has duration 0.25
(immediate 3E800000). This disproves the old 650 pixels/second description.
FUN_002486E8 clears the derivative fields; 001DA780 creates cubic Hermite
coefficients, so the interpolation has zero endpoint velocities.
FUN_002487E8 runs 001DAEC8 with both derivative limits zero, then writes
integer-truncated X while preserving authored Y. No sine easing is involved.

00249348: states 0/2 show text and clear white-bar alpha; on first update,
clear text ID, write pending string, reset path. Subsequent frames advance
path until it reports complete, then enter state 3. 00249518 copies newly
selected text; states 2/3 transition to 1 and restart fade clock. State 1
fades text down and bar up over 0.6 seconds, and after a strict >0.5-second
hold enters state 2 in the same update. Values are initialized ELF floats at
00331E48/00331E4C, not inferred timings. FUN_001449A8->00140EA8 and
00140E20 prove dynamic text clears +0x60. 00140E20 truncates to capacity-1.

The original object is per-screen, not a universally drawn author placeholder.
The 13 direct constructor callsites are 001B5E84,001B6CC8,001B848C,001EEBFC,
001EF4FC,001F3454,001F3F7C,001F9840,002164A0,00216ED0,00217588,
00240134,0024D4A0. This bounded list does not establish that every other
screen hides the similarly named widget: attachment/default visibility for
untranslated screens remains unresolved and must be surfaced explicitly.

Confirmed selection bindings:

| Screen | Message-4 function | Items in order, numeric IDs |
|---|---|---|
| New Main Menu | 00216938 | 2720,271F,2719 |
| Cops On Mode Select | 00217380 | 2721,271E,271C,271D,2719 |
| Cops Off Mode Select | 002179F0 | 2716,2717,2718,2719 |
| Options | 001EECA8 | 186B4 through 186BA |

The Type_002491B0 translation reproduces these states and the path. The host
supplies delta rather than reconstructing PS2 clock production. FeWidget has
one quad alpha; original 00248100/002481D8 preserve/multiply each of four
vertex alphas. Gradient-alpha quads and clipping are unresolved, and no
original comparison has established equivalence.

## E-0016-S03: background object partial recovery

002495F0 initializes two scalar paths, at (320,240), duration gp-3A94 = 50.0;
00249828 clears eight texture slots then adds hashes of strings at
002BEFB0/002BEFC0. 00249A40 binds GRID (00331E60), MASTER_BP_OBJECT
(002BEFD0), FADING_BP_OBJECT (002BEFE8), then resets the random cycle.
00249888 chooses start X in [160,165), Y [180,300), end X [300,400),
fades both object alphas to zero, initializes a path with duration 50 seconds,
and picks a nonempty texture slot using 002497B8/002497F0.
00249AE8 fades in over 1 second and out in the final second; at elapsed >50
it chooses a new cycle. 00249570 uses 0025A488(10000)*gp-6770, whose
initialized value is 0.0001. FUN_0025A3D0 returns the unsigned old RNG state
modulo bound (DIVU at 0025A3E4 disproves the decompiler signed cast), and updates state with XOR 1D872B41 and shift/XOR operations.
The initialized RNG state is 0x12345678 at 00332068. Type_002495F0 now translates
this blueprint object with an explicit shared-RNG input seam, including eight
texture slots and duplicate-removal/addition. Other original callers advance RNG;
the host startup draw count remains unresolved. GRID angle consumption and
original clipping remain unresolved.
Background movie integration is a separate TASK-0016 media scope.

## Validation and continuation

Synthetic src/tests/frontend_scroll_tests.cpp covers numeric lookup, invalid
offsets, Hermite midpoint, clamped endpoint, fade and exact selected-text swap.
PC tests are not original equivalence evidence. Run the native/godot presets
after adding this target. Continue E-0016-S03 from 002495F0/00249828 and
manager bindings at 00249A40; use ignored raw evidence rather than re-exporting.

Validation completed: g++ 13.2.0 (MinGW-W64 x86_64-ucrt-posix-seh) command
`g++ -std=c++17 -Isrc src/tests/frontend_scroll_tests.cpp src/game_core/frontend/fmenus.cpp src/game_core/assets/elab_chunks.cpp -o local/task0016/scroll/frontend_scroll_tests.exe`,
then `& local/task0016/scroll/frontend_scroll_tests.exe` passed FRONTEND_SCROLL_OK.
An ignored C++ asset probe instantiated FeLanguage using the configured root and
resolved IDs 2720,271F,2719,2727,186B4: FE_ASSET_PROBE_OK. Constructor validates
all 2437 numeric-table offsets and terminators. No proprietary strings/assets
were copied into tests. Full build/host checks belong to root integration.

E-0016-S03 implementation continuation: Type_002495F0 now updates the fading
blueprint sprite; Sub_0025A3D0 reproduces DIVU and state shifts. Synthetic tests
pass again after adding unsigned high-bit RNG cases, fixed random starts,
blueprint texture selection, drift midpoint, alpha fades and reset >50 seconds.
The same standalone command above passed FRONTEND_SCROLL_OK. GRID rotation
consumer, host startup RNG draw count, other texture-list mutation callsites,
quad gradient alphas and original comparison remain unresolved.

### E-0016-S04: background geometry correction and manager gate

Unlike the scrolling scalar path's integer writer, background reset/update call
0024AAE0 -> 001408E8/00140930, subtract measured width/height divided by two,
and call float setters 00140A20/00140A48. The random positions are centers.
Measurements run before changing the texture on reset; next frame uses the new
texture dimensions. Bind_00249A40 therefore accepts a WidgetMeasure seam.
Synthetic centered-geometry checks passed after correction, FRONTEND_SCROLL_OK
with the same recorded standalone compile/run command.

0016CCA8 (requested at its callsite 0016CD3C) advances the blueprint object
only when gp-0x53AC (iGpffffAC54 at00330544) equals3. This is GAMEFLOW_STATE_LOADING_TRACK_COMMON; IN_FRONTEND is row2
in the freshly decoded dispatcher table (startup/video scope evidence at
local/task0016/gameflow-table.json and gameflow-name3.json). It is a gameflow
gate, not a movie/background mode. Ordinary frontend must not advance this
object. The hypothesis that this is a mutually exclusive background mode is
retired; simultaneous media/sprite drawing is not disproved by this gate. Both 00249AE8 and manager0016CCA8 were inspected:
field+0x78 only increments and wraps at6.283, with no consumer in those bodies.
Treat GRID angle consumption as unresolved/dead state, not an invitation to
invent rotation. Bound 13 constructor references remain the scope of scrolling
attachment evidence; untranslated screens must expose their missing handler
rather than draw authored dynamic placeholders as real runtime text.

# Frontend rendering audit - TASK-0039

Read-only Ghidra MCP investigation on2026-10-17. Target: SLUS_203.62,
SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Executable revision remains unknown; boot VER1.00 is not an executable version.
The screenshot supplied by the user is a symptom/reference, not instruction or binary evidence.
No original-execution equivalence or Verified promotion is claimed.

## Confirmed corrections

| Area | Original evidence | Port correction |
|---|---|---|
| Event digit Y |001F7BF0 calls00140930 after transformed position/scale;00140930 subtracts half00140688 height | Centre using scaled maximum glyph height, instead of placing the top at node centre |
| Font height |0013FFA8 takes max glyph+10 into font+420;00140CD8 scales it | Header value28 is a glyph-table offset, not height. Frontend font heights are27,61,43,98 |
| Pair lookup |00140098 uses CURRENT glyph pair count+02 and first index+1C, compares PREVIOUS byte | Decode all eight-byte pair records and apply on draw |
| Measurement |00140D30 sums advances without pairs | Keep advance-only measurement; do not feed kerned draw extent back into centring/animations |
| Normal draw |0014261C..40 multiplies pair by X scale, discards abs>80;00142844..54 advances scale*(advance+scaled pair) | Preserve both scale applications and exact80 boundary |
| Alignment |001424F4..00142524 tests right bit1 before centre bit4 | Fix right alignment and mixed-bit precedence |
| Inline buttons |00141DB0/00141DD8, table002F09C0 and BUTTONSOUT at002AF1B0 | Switch embedded80..9D except92 to original icon font; retain unscaled icon pen advance |
| Helper glyph width |00141B58 and00141B9C multiply X scale twice | Preserve scale-squared width for helper paths, including wrapped/fixed/icon glyphs |
| Fixed-cell text |0013FFA8/001401D8 and00141F70 | Numeral-character max advance, unscaled half(cell-advance) origin, scaled cell step |
| Glyph UV |001422F0 and00141A98 use half-texel offset at both edges | Preserve pixel-space source offset without changing extent |
| Quad colours |00142F24..00142FE8 consumes four separate colours | Keep TL/TR/BR/BL colours through parser, frame and RenderingServer polygon |
| Quad UV |00142C0C..00142CF4 consumes all eight authored floats | Preserve independent UVs and per-edge half-texel terms |
| Quad alpha |001411D8(-1) changes all four alpha bytes, preserving RGB | Existing generic alpha adapter updates all corners; document fade multiplies each original corner separately |
| Clipping |00141808 changes positions/component UV edges but leaves corner colours | Extended packet crop preserves that behavior |
| Visibility/layer |00145F30 exact visibility1;00146600 signed layer comparison | Draw exactly1 and sort descending signed layers |

Stable evidence IDs: E-0039-N01, F01..F06, Q01..Q09. Individual function records
live under the binary frontend directory; detailed quad instructions and limitations are in
`task0039-quad-audit.md`. No original descriptive names were recovered by this audit.

The serialized font header offsets include the eight-byte chunk header. Glyph+18 is not
consumed as a bearing by either traced draw path, so it is not silently applied as one.
BUTTONSOUT has zero pairs and a stale out-of-bounds pair offset7452 in a948-byte payload;
the loader must not dereference or reject an unused table. A regression protects this case.

## Corpus coverage

E-0039-A01: the configured original FRONTB.BUN has41 screen documents,1015 quad widgets,
534 text widgets,255 nonuniform four-colour quads and18 nondefault UV sets. Its authored
text flags are417 left(2),105 centre(4),12 right(1). No authored quad has nonzero rotation
with mode2. This is a resource census, not proof that every dynamic state was executed.
Frontend bundle SHA-256: `869d11291994090a8a1730dcab399248b3e97ab4f2088df647017b755d5d714c`.

All41 documents are exercised by the native render smoke. Routed Ultimate Racer captures
exercise events1 and3; Options and controller documents provide shared-text/layout checks.
Opening raw document39 without its mode-select route shows authored placeholders: this is
an inspection context, not evidence that production routing or localization failed.

## Remaining scope and unresolved questions

| Question | Status / exact continuation |
|---|---|
| Whole frontend matches original pixels/state | Unresolved. Capture original menus with matching viewport, selection, profile and timing under TASK-0023 |
| FCR31 conversion/GS fixed coordinates | Static CVT.W.S is confirmed; rounding mode and native numeric parity are not. Fractional quad dimensions remain the inherited projection |
| GS padded texture versus cropped host sampler | Packet UV arithmetic is preserved in pixel space; GS/host texel-centre and filtering equivalence requires an original render oracle |
| Rotation | Fields and packet support exist; matrix/trig translation unresolved. Reached nonzero mode2 throws at001428F0 rather than drawing an unrotated substitute |
| Special textures / animation |00143078 conditional multi-pass path and001428F0 animated UV production remain untranslated; ordinary authored paths only |
| Wrapping/control characters |00144ED0 inspected; pair/rounded-step/helper width corrected. Existing word-break/whitespace projection is not a complete translation |
| Equal-layer traversal | Signed ordering confirmed; original insertion order within a layer not established |
| Event-tree transform/background/state animation | Existing TASK-0022 recovery/calibration is not promoted by the digit fix. Logo positions and selection-border phase still need original comparisons |
| Missing resources/per-screen dependencies | Existing missing assets, game-start/reward-render/reset gaps remain explicit; this task does not claim full game recovery |

## Reproduction

```powershell
py -3 reverse/scripts/inspect_frontend_rendering.py --elf (Get-Content config/ps2_elf.local.json | ConvertFrom-Json).elf
cmake --build --preset native-debug --parallel 6
cmake --build --preset godot-debug --parallel 6
ctest --preset native-debug --output-on-failure
ctest --preset godot-debug --output-on-failure
& local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --headless --path godot res://scenes/frontend_native.tscn -- --native-render-smoke
```

Raw MCP JSON (41 queries), per-query hashes and asset census remain ignored under `local/task0039/capture`.
Private screenshots remain under `local/task0039`. No proprietary captures/assets enter Git.
The script uses only MCP inspect and memory read; Ghidra annotations/state are unchanged.

Final validation: native full48/48 and post-relink focused4/4 passed. Earlier Godot full52/54
had a stale generated-table check (subsequently fixed) and unrelated CarViewer GDScript parse
failure/timeout. All41-document render smoke passed before the final zero-pair font correction;
later event3/Options windowed captures include the restored button glyph. Final host rerun was
invalidated when an external concurrent build cleaned shared binaries/DLL. TASK-0039 retains
the exact commands/results and sequential rerun prerequisite; do not quote that canceled run
as a source regression or a successful final acceptance run.

## TASK-0095 render ordering continuation

E-0095-N04 resolves authored node insertion and final bucket submission. Widgets
are appended to the loader list; equal-layer drawing reverses authored order.
Final render phases are24..20,15..0,19..16,28..25, so keyboard16..19 overlays
ordinary frontend0..15. Frame builder uses these phases and rejects unsupported
visible layers outside0..28. Native deterministic tie/phase tests and a full
keyboard window capture exercise the correction. Original runtime comparison
remains open. Records: reverse/.../frontend/task0095-name-entry.md.

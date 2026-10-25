# TASK-0092 - Retail Jukebox screen

- Status: active
- Parent: TASK-0091
- Scope: recover constructor/grouping, five-row presentation, category/song navigation,
  per-song mode, genre enable state, preview scheduling, Back commit and runtime integration.
- Binary: SLUS_203.62, US PS2, revision unknown, SHA-256
  7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Evidence: E-0092-J01 onward; raw MCP responses in ignored local/task0091/.
- Relevant existing task: TASK-0016 (audio), TASK-0017 (Back), TASK-0039 (rendering).

## Acceptance criteria

- [ ] Recover handler, grouping, metadata and profile layout with stable address evidence.
- [x] Bounded portable grouping/navigation state and five-row presentation decisions tested.
- [ ] Complete production presentation implemented with original boundary behavior.
- [x] Production menu route displays original song metadata and responds to input.
- [x] Preview/genre/song modes and Back resume/commit integrated with native audio/profile.
- [ ] Meaningful native and original-asset regressions pass; Godot integration checked.
- [ ] Original-runtime comparison recorded; only compared scope may be Verified.

## Findings

Read-only MCP confirms 001F1220 allocates 0x240 and calls 001F02D8. Constructor,
refresh 001F0478, preview 001F0CC8 and grouping 001EF240 decompile successfully.
001F0D10 begins with undefined bytes, so decompilation fails. Existing TASK-0017
listing and current memory can recover its jump table and defined interior without
creating functions or mutating the Ghidra database. This is not a connection failure.

## Next action

Production route Audio Options -> JukeBox Music -> Audio Options is connected,
with original metadata, row/status/arrow binding, delayed preview, stop, natural
completion auto-mode, preferred-song Back resume and isolated host Autosave.
Continue original-runtime comparison; full hardware async audio/card-error behavior
remains outside the bounded host device adaptation. TASK-0093 is now the single
implementation subtask. Whole-entry class templates still throw deliberately.

## Production continuation - 2026-10-25

E-0092-J04..J07 add metadata/widget/audio/profile and successful host-save closure.
The authored Jukebox document is hash95B2B627, current bundle index34, not27 as
older media notes claimed. Its background is options_movie_new.AVI.
Mode3 is frontend-eligible (static matrix002B6300); the prior predicate excluding
it is corrected. Back resumes snapshot+A4 (initial5), never the highlighted cursor.

`cmake --build --preset godot-debug --parallel 4` passed before the latest popup
and route additions. Godot4.7.2 Vulkan window capture of screen34 was inspected:
original titles/authors/statuses, genre focus/arrows and background video render.
Ignored capture: local/task0091/jukebox-live.png. No original comparison.
`ctest --preset native-debug -R '^(jukebox_screen|frontend_jukebox)$' --output-on-failure`
passed2/2; the subsequent expanded frontend_jukebox test passed1/1 in3.52s,
including exact Autosave timing, Back consumption, disk write and return history.
Its input/output saves are isolated under ignored local/task0091/autosave-fixture;
the test confirms the read-only input remains byte-identical.

## Validation - 2026-10-25

Tool versions: CMake3.29.2, GCC13.2.0 (MinGW UCRT), Python3.14.4.

- `py -3 reverse/scripts/inspect_jukebox_screen.py --elf (Get-Content config/ps2_elf.local.json | ConvertFrom-Json).elf`:
  passed16 read-only MCP queries (including the recorded expected missing-function result),
  three memory ranges match the SHA-bound Retail ELF. No Ghidra changes.
- `cmake --build --preset native-debug --target jukebox_screen_tests --parallel 4`:
  passed after fixing hexadecimal-literal spacing in the new code.
- `cmake --build --preset native-debug --target jukebox_tests frontend_actions_tests frontend_back_tests frontend_exit_tests frontend_runtime_tests frontend_modeselect_tests frame_builder_tests --parallel 4`:
  passed.
- `ctest --preset native-debug -R '^(jukebox|jukebox_screen|frontend_actions|frontend_back|frontend_exit|frontend_modeselect|frontend_runtime|frame_builder)$' --output-on-failure`:
  passed8/8,32.61s; existing JBC and frontend runtime tests use configured original assets.
- `cmake --build --preset godot-debug --target jukebox_screen_tests --parallel 4`:
  passed. This target builds portable core, not the extension or a windowed frontend.
- `ctest --preset godot-debug -R '^jukebox_screen$' --output-on-failure`: passed1/1,0.20s.

Initial isolated jukebox_screen regression also passed. No full extension build,
Godot visual validation, PCSX2 comparison or Verified promotion. The new test is
explicitly exempted from the existing tests ignore rule; other ignored tests stay ignored.
No commit or staging. All remaining acceptance criteria stay open.

Formatted original slot placeholders are now bound before popup wrapping. Vulkan
capture autosave-formatted.png exited0 and displays slot1, no literal%d. Capture
command in TASK-0095 session log; isolated original profile remains unchanged.

##2026-10-26 corrective audit scope

User reports BGM in states that should be silent. TASK-0092 is again the single
implementation scope; TASK-0095 Help work is deferred. Fresh read-only Retail MCP
captures in local/task0092-audit. E-0092-J08 separates001AEAB0 explicit start from
0027DFF8 update/completion. Accept silence through intro and skipped-second-intro
Title, explicit natural-end/static-Title start, no poll resurrection after stop,
and preview auto0 completion. Original-runtime comparison remains open.

Corrective validation: native selection `^(frontend_jukebox|frontend_runtime|jukebox_screen|jukebox|movieplayer)$`
passed5/5 in35.88s; builds native-debug selected targets and Godot-debug ps2_port
passed. Godot headless binding regression and Vulkan window host-audio regression
both passed; exact commands and fresh MCP evidence in
reverse/frontend/task0092-music-audit.md. Per-frame implicit start is removed.
Full Jukebox/Title parity is not established, original comparison remains open.
Next: original matched-state audio comparison, then remaining TASK-0091 work.

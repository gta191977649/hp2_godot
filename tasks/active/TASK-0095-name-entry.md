# TASK-0095 - Retail NameEntry keyboard

- Status: active
- Parent: TASK-0091. Current implementation scope after TASK-0094 host checks;
  TASK-0092..0094 retain open original comparison criteria.
- Binary: Retail SLUS_203.62 US PS2, revision unknown, SHA-256
  7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Scope:0024D408/0024D870 NameEntry,3x14 keyboard, seven-character buffer,
  P1/P2 pad gates, gender toggles, animations, Back commit and private Autosave.
- Excludes legacy Username rotating editor0016DBD0, tracked separately under parent.
- Evidence: E-0095-N01 controls/profile;N02 authored binding;N03 animation.
  Read-only captures ignored in local/task0091/name-agent.

## Acceptance

- [x] Portable controls, wrong-pad rejection, boundary/delete/case/commit tests.
- [ ] Authored widget binding, animation and production input routes.
- [ ] Native/Godot builds, real-asset roundtrip and window rendering.
- [ ] Matched original-runtime comparison. No Verified from PC tests.

## Confirmed scope

Names profile10D90/11460, flags10D83/11453. Original default localized names
27165/27166;00187DE8 copies at most11 bytes. Keyboard copies at most7 into
zeroed8 bytes. Back commits. Delete shifts at current cursor then moves cursor
left. Cursor is clamped0..6; grid wraps row0..2/col0..13. Reopening preserves
grid row/column but resets lowercase/name cursor. Outer Back compares original
12-byte names and flags before conditional Autosave. No invented cancel rollback.

## Checkpoint2026-10-25

`cmake --build --preset native-debug --target frontend_name_entry_tests --parallel 4`
passed. `ctest --preset native-debug -R '^frontend_name_entry$' --output-on-failure`:
1/1 passed1.74s. Godot `ps2_port` build passed. Vulkan route capture through Options
exposed keyboard equal-layer occlusion; visual criterion remains unchecked. Normal
inputs are covered natively; common Help and host second-pad adapter remain open.
Read task0095-name-entry.md for exact confirmed input/animation evidence. Continue
read-only investigation of original list insertion/traversal before changing order.

## Render dependency resolved - E-0095-N04

Original final layer buckets draw24..20,15..0,19..16,28..25, with reverse-authored
ties. Numeric descending alone was insufficient. Implemented final phases; ignored
Vulkan capture name-entry-phases-fixed.png visibly shows complete keyboard.
Native keyboard/frame tests2/2 passed2.12s. Added final-phase regression fixture.
Expanded native P2 commit and120Hz animation trace passes. Two-host-pad adapter
parses/imports successfully; physical device check remains unperformed.
Shared Help overlay still open, so task remains active, not Verified.

## Latest validation checkpoint

Full native-debug build passed; Godot-debug ps2_port build passed. Frontend CTest
selection19/19 passed48.17s. Godot headless native-render-smoke passed all41
loaded frontend documents (not a behavior-completeness claim). Headless editor
import passed. Final frame-order fixture and NameEntry routes are included.
Tools CMake3.29.2/GCC13.2.0/Python3.14.4/Godot4.7.2. Shared Help, physical devices
and original comparison stay open. Exact commands in docs/PROGRESS_LOG.md.

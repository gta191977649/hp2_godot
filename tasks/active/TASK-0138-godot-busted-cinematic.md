# TASK-0138 - Godot Busted cinematic

- Status: active
- Scope: present the donor's ticket/arrest camera, animated actors, hand props and monochrome view in Godot.
- Evidence: donor PAL1B13F0, PAL1B2C90, PAL1B30A8, PAL1A6B80, PAL10D778 and PAL11A9F0. ELF identity follows TASK-0123.
- Acceptance: enter the original arrest playback, observe animated actors and camera, and verify cleanup when it ends. Preserve ordinary race rendering and donor outcome.

Debug and Release staged with `cmake --build --preset godot-mvp-<configuration> --target stage_mvp --parallel 16`.
Godot 4.7.2 Forward+ passed the Track 13 frontend smoke with
`HP2_MVP_PURSUIT=1`, `HP2_MVP_TEST_ARREST_NIS=1` and
`HP2_MVP_DEBUG_CONSOLE=1`. The test deliberately enters donor arrest
playback. It observed two actor nodes, the monochrome camera and colored
BUSTED/REC overlay, a later pose, and zero actor nodes after playback.
See `local/task0138-final/smoke.log` and the three `busted*.png` captures.
Natural pursuit-to-arrest and all hand-prop variants remain open.
The capture's ImGui graph recorded a startup peak of 58.05 ms, so this
smoke does not establish hitch-free entry. Audio migration is TASK-0139.

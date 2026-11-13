# TASK-0133 - Responsive Godot frontend

- Status: active
- Scope: Godot layout, video framing, previews and HUD across live window sizes.
- Acceptance: preserve authored proportions and grouping, and exercise resize in the frontend and race with original assets.
- Authority: port-side presentation policy, with original object identities taken from the donor and its loaded layout.

The final layout keeps menu controls, the race HUD, previews and text in
one centered 640×480 design canvas. The menu grid tiles outside it.
Decorative bands fill the width and video fits within a 4:3 rectangle.
The race camera keeps its vertical field of view. The authored red and
silver cinematic strips keep their 4:3 framing. Separate black fill
covers the outer edges. Title and Loading use the full-width decorative
band rule with centered foreground content.

Validation uses `tools/test_mvp_frontend_resize.gd` in the staged Godot
project. Add `-- --race-track=12` for the direct race or set
`HP2_MVP_PURSUIT=1` for the event tree. Use an isolated profile and a
writable `HP2_MVP_SCREENSHOT_DIR`. The script checks 800×600, 1120×630,
1260×540 and 540×720 in the same process, with maximize/restore at the
main or pause menu. The normal frontend smoke is a separate check.

Natural finish and results verification remain open under TASK-0129.

Both `cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16`
and the equivalent Release build passed (CMake 3.29.2, Ninja 1.12.0,
GCC 13.2.0). Godot 4.7.2 Vulkan Forward+ passed the direct HUD, pause and
restart confirmation sweep and the Track 13 menu-to-race sweep. Logs are
`local/task0133-final2-direct.log` and `local/task0133-final2-pursuit.log`.
The latter includes a Loading capture and checks that the menu video
keeps its 4:3 rectangle and playback position across the resize.
The existing Track 12 frontend smoke also passed during this task.
The Select Track preview passed the same sweep with
`HP2_MVP_RESIZE_TRACK_PICKER=1` in `local/task0133-final2-picker.log`.

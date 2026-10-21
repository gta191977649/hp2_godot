# TASK-0074 - TrackViewer wireframe debug checkbox

- Status: completed
- Scope: port-side Effects checkbox using Godot viewport wireframe mode.
- User confirmation: frame render mode means geometry wireframe.
- No recovered logic, native command selection, physics or material changes.
- Defaults off; restore on defaults, unloaded renderer and scene exit.
- Apply to the source colour viewport as well as the direct main viewport;
  keep encoded depth/history and reflection consumers unchanged.

## Acceptance

- [x] Existing Effects action chain displays and toggles the checkbox.
- [x] Forward+ runtime toggle, new colour viewport and cleanup validated.
- [x] Documentation and generated progress updated; no Verified promotion.

## Validation plan

One bounded isolated Vulkan smoke with synthetic geometry; no gameplay or FPS
benchmark is necessary. Preserve all existing user edits. No native rebuild is
needed because this feature uses existing UI descriptors/actions and Godot APIs.

## Results and reproduction

Script-only feature: godot/scripts/runtime/track_controls.gd and
track_wireframe_debug.gd. No native/C++ or dependency changes; no DLL rebuild is
needed. The existing generic Effects ImGui descriptor supplies the checkbox.

PASS: isolated official Godot4.7.2 ed1daf0bf, Vulkan Forward+, Intel Arc B390,
synthetic BoxMesh and independent colour/depth viewports. Actual ImGui checkbox
clicks enable/disable and change colour pixels; depth pixels remain identical.
Replacement colour viewport follows the enabled state. Restore Original Defaults
and renderer scene exit restore the previous root debug mode. Runtime log has
TASK0074_WIREFRAME_OK and no final errors. This is PC host diagnostic validation,
not PS2/PCSX2 equivalence or original Verified status.

Private fixture/captures/logs: local/task0074/. Run the isolated fixture with:
`local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64.exe --path
local/task0074/godot --rendering-method forward_plus --rendering-driver vulkan
--audio-driver Dummy --script res://wireframe_smoke.gd` (one command line).
Initial fixture attempts lacked the isolated extension discovery cache and input
forwarding; these setup failures were corrected before the passing GPU check.
No production startup initialization or global mesh-generation option was added.

Scoped diff and generated progress checks pass. Existing user files preserved;
nothing staged or committed. Next: normal TrackViewer use via Effects panel.

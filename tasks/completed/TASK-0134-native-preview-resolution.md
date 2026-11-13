# TASK-0134 - Native frontend preview resolution

- Status: completed
- Scope: remove the select-car background artifact and render the 3D preview at the fitted window's pixel density.
- Acceptance: inspect Select Car after activation and live resize. Preserve the centered 4:3 composition and the Select Track preview.
- Authority: port-side Godot presentation. Original Flailer positions and preview camera data are read-only inputs.

The wide layout repeated even the clipped Select Car grid fragment.
The Godot renderer now tiles only grids that span the full authored
canvas and draws clipped fragments at their submitted positions.
The preview SubViewport renders at the fitted canvas's physical pixel
size, with 4× MSAA. A TextureRect composites it within the existing
640×480 design rectangle without changing the camera or menu layout.

Debug and Release staged with `cmake --build --preset godot-mvp-debug
--target stage_mvp --parallel 16` and the equivalent Release command
(CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0). Godot 4.7.2 Vulkan Forward+
passed the Select Car and Select Track resize sweep with
`HP2_MVP_RESIZE_TRACK_PICKER=1` and `tools/test_mvp_frontend_resize.gd`.
The 1280×900 window used a 1200×900 target, and 1120×630 used 840×630.
See `local/task0134-preview.log` and `local/task0134-shots`. The
previous vertical grid noise was absent from the Select Car capture.

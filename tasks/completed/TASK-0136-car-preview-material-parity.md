# TASK-0136 - Match car preview and race materials

- Status: completed
- Scope: use the race vehicle's unshaded texture and vertex color pipeline in Select Car.
- Acceptance: verify unshaded preview materials, authored vertex colors and resolved lamp textures in a live car preview.
- Authority: port-side Godot presentation. TASK-0135 records the lamp selection provenance.

Select Car and race vehicles use the same MeshConverter call, texture
modulation and RGB555 scaling. The preview light rig and its render-only
data export were removed. The resolved texture table still provides the
headlight and brake lamp bindings.

Both `cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16`
and the equivalent Release build exited 0 (CMake 3.29.2, Ninja 1.12.0,
GCC 13.2.0). Godot 4.7.2 Vulkan Forward+ ran
`tools/test_mvp_frontend_resize.gd` with `HP2_MVP_PURSUIT=1`. Lotus Elise
reported 22 unshaded preview surfaces with vertex colors and both lamp
texture keys. See `local/task0136-preview.log` and
`local/task0136-shots/select-car-1280x900.png`.

# TASK-0135 - Car preview vertex shading and lamp textures

- Status: completed
- Follow-up: TASK-0136 replaced the preview lighting rig with the shared unshaded race material. The lamp texture selection remains in use.
- Scope: use authored colors, normals, light inputs and lamp texture selection in the Godot car preview.
- Acceptance: verify a real car preview uses non-flat vertex shading and the expected lamp bindings.
- Authority: Windows donor `care.cpp`, `elighte.cpp`, and `ecstasye.cpp`. The donor PAL ELF hash remains unresolved.

The donor `care.cpp` SHA-256 is
`f7755e7dbf99fe9c68cf6ee9e9976bda2e4f2fe19dbda23933b9e132b0217322`.
The donor `elighte.cpp` SHA-256 is
`1ada7370e6be4545efe4eda65fa7099a1322a2e88ad32cc3494af28e7fe7179b`.
The preview uses the decoded RGB555 range and authored normals on lit
strips, with Godot ambient and directional lights driven by the selected
car preset. It aligns lit triangle winding to the authored normals.
The Godot light response is a port-side mapping.

The state-only frontend collector now runs the same lamp texture
selection as the donor car draw. Godot reads resolved replacement slots
and refreshes the preview mesh when those bindings change.

Debug and Release staged with `cmake --build --preset godot-mvp-debug
--target stage_mvp --parallel 16` and its Release equivalent (CMake 3.29.2,
Ninja 1.12.0, GCC 13.2.0). Godot 4.7.2 Vulkan Forward+ ran the resize
script with `HP2_MVP_PURSUIT=1`. The Lotus Elise reported 15 lit surfaces
with vertex color and normal arrays. The renderer bound
`ELISE_HEADLIGHT` (544547642) and `ELISE_BRAKELIGHT` (199674477).
Evidence is in `local/task0135-verify.log` and
`local/task0135-verify-shots/select-car-1280x900.png`.

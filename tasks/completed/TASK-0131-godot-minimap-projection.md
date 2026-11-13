# TASK-0131 - Godot minimap projection

- Status: completed
- Scope: match the Godot minimap texture and marker coordinates using the donor TrackMap projection.
- Acceptance: keep the world-to-map transform in the donor, preserve the projected texture coordinates in Godot, and validate the rendered map in a real race.
- Authority: Windows donor `indep/src/trackmap.cpp`, including its PAL 0x001BD1F8 mode table. The donor PAL ELF hash is unresolved. No new PAL symbol or body attribution is claimed.

The donor file SHA-256 is
`56bada38308893889989fae6f8a5a124d1753d4c3da09f1f4b6a6fbd10eb8f5a`.
The port's correction follows the donor's world-Y reflection and authored
map rotation. A forward displacement must become local positive Y because
`DrawMarker` negates it when projecting to screen. Fixed map modes retain
their fixed orientation. Godot carries the donor's clipped UVQ through
interpolation and divides in the fragment shader.

Debug staged with `cmake --build --preset godot-mvp-debug --target stage_mvp
--parallel 16`. Godot 4.7.2 Forward+ on Intel Arc B390 ran the local direct
probe with `--path build/godot-mvp-debug/mvp-project --quit-after 2400
--script local/task0131-moving.gd -- --race-track=12` and exited 0.
At committed step 211 the player was moving at 21.82 m/s. The screenshot
shows the start line behind and leading cars above the player marker.
See `local/task0131-moving-final.log` and `local/task0131-moving-final.png`.
This validates the Windows presentation. Exact PS2 parity and the full
first-event results flow are outside this check.

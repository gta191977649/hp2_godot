# TASK-0045 surface UI and presentation

E-0045-U01, 2026-10-17. This entire UI/rendering layer is port-side. It does not
establish original material colors, textures, input bindings or another tire model.
Original stored values and terrain catalogue data are supplied by the canonical
Debug backend, not recomputed in Godot or ImGui.

`Ps2DrivingLab.set_surface(mode,terrain)` delegates to Session's queued setting.
`get_state` exposes only committed snapshot settings/catalogue/grip, plus the shared
native descriptor boundaries, terrain IDs and tool colors. Rejected requests retain
the completed snapshot; the bridge reports the error. GDScript changes presentation
only after a successful native publication. Selecting a surface neither restarts the
car nor changes gear, pose, camera or input policy.

The existing Return button stays first at the same position in a fixed parent header.
Controls and diagnostics occupy a separate borderless scrolling child, so expanding
or focusing the grip child cannot scroll Return out of reach. Uniform/Zones and the
five catalogue materials follow it. A default-collapsed read-only child shows four
wheels: contact, terrain name/ID, terrain static/dynamic/rolling values, slipping,
Z force, current slip, traction usage/boost/catchup, road noise, slip/force pairs and
TireInfo static/dynamic coefficients. Values retain their stored meaning; units and
a combined grip coefficient are not invented. The pinned ImGui1.77 WIP provides
legacy Columns, so the implementation uses BeginChild/Columns with horizontal
scrolling rather than unsupported later Tables/BeginDisabled APIs.

The floor shader uses host world Z (confirmed native X mapping), with boundaries
and palette passed directly from the published descriptor. Translating the floor
with the car does not translate the zones; first/last zones extend outward.
Small depth-tested labels sit beside the lane at host X=-5/Y=0.4 and use the same
catalogue IDs/names. Stop removes the labels and restores
the original preview floor material/size/position. Existing WASD, pointer capture,
focus suspension, third-person camera and Return flow are preserved.

## Validation sources and remaining execution

`driving_smoke.gd` adds actual ImGui selector clicks, committed DIRT presentation,
five-zone traversal with mixed per-wheel materials, finite raw diagnostics and
preview material/label restoration. It retains the previous lifecycle/camera/input
assertions. `tools/test_drive_window.py --surfaces` adds optional PID-bound real
Win32 material selection for all five materials, zone traversal and screenshot;
all reports/screenshots stay under ignored local output. Existing ownership and
foreground checks remain required before sending input.

This worker did not run builds or launch a test window. Root owns serialized build,
headless smoke and real-window validation. The first root compile exposed the legacy
ImGui API boundary; source was corrected against the pinned header. No passing run
is claimed for the corrected latest sources until root records it.

Root validation reached `DRIVING_SURFACE_SMOKE_OK`, but the final Return click
initially failed. Root traced the cause to pointer recapture forwarding the press
to ImGui while bypassing its corresponding captured mouse-button release, leaving
ImGui's button state held. Root corrected release forwarding in `asset_viewer.gd`;
the subsequent host smoke passed (8.43 seconds), and the real-window surface run
passed 31 checks. The fixed header remains a layout improvement; parent scrolling
was a hypothesis, not the confirmed cause of that input failure.

Screenshot review then showed that the Zones description wraps to two lines: the
grip header is at client Y=188, so the previous Y=177 test click did not expand it.
Both smoke paths now click Y=188. Labels are reduced to 24 pixels at pixel size
0.005, moved to the lane side, and depth-tested to avoid obscuring the car. Root
must rerun and inspect the expanded diagnostics with these latest changes.


Final root window verification: `py -3 tools/test_drive_window.py --surfaces --output
local/task0045/window-final` passed31 checks. Root inspected `surfaces.png`: expanded four
wheel rows show live contacts/terrain, the small lane-side label does not obscure the car,
and the Return header stays visible. NativeJSON verifies all original raw diagnostic fields;
columns beyond the initial view scroll horizontally. This is UI evidence, not numerical
equivalence. Source review pins cover the final C++ header/child layout; GDScript forwards
captured button releases and retains motion ownership with the camera.

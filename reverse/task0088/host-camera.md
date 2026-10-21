# TASK-0088 host POV cycle, projection and held lookback

Status: implemented bounded Test drive host consumer; PC regression tested,
not Verified against the original game. Native evidence E-0088-C01 and retained
E-0084-C01/M01 bind Debug NFSHP2.ELF SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
version/build independently unknown. No benchmark was run.

## Host policy and recovered data

The user's TASK0088 instructions supersede TASK0084's Godot-default projection
exception. The native camera selects an available authored POV using the recovered
cycle. In the shipped no-dashboard fixture, V advances exterior1 -> exterior2 ->
bumper0 -> exterior1. V is a port-side key binding; it is not a claim about the
original PS2 button mapping. Keyboard repeat does not advance additional views.
B is an independent held binding to the original lookback parameter. Press starts
the original eased lookback; release restores the selected POV instead of selecting
the default exterior. Camera updates still do not integrate physical state.

DrivingCameraFrame supplies fov_bangle, near_z, hide_car and pose. The host uses
KEEP_WIDTH and bAngle*360/65536, restoring original71.52099609375-degree exterior
and90-degree bumper horizontal projections. The original near plane is copied
from the native frame. The old user exception of75-degree Godot vertical FOV is
no longer applied to Test drive. Native selection retains unsupported dashboard
rendering as an explicit boundary rather than silently omitting it.

The current single-car Ps2VehicleRenderer subtree is the main car and flare
consumer. The host suppresses it when the native frame marks CarNotToRender.
Current environment captures consume separate world faces and do not render this
car subtree. No material/resource destruction, cull-mask change or invented full
original per-view renderer ABI is introduced. Exterior switching restores the
subtree; Return restores preview visibility and camera FOV/aspect/near/transform.

Mouse input remains an explicit host override. Exterior input orbits the native
target; bumper input swivels around the native eye. The existing1.5-second idle
and exponential return policy remains port-side tuning. Returning converges to
the current native pose rather than a fabricated alternate camera. V and a changed
B hold clear temporary mouse offsets. Focus suspension, keyboard UI capture,
terminal failure and stop clear held lookback; B release is processed even if a
widget consumes the event. Merely switching POV does not clear a physically held B.

Skid projection scale now consumes the actual camera projection matrix, preserving
horizontal-FOV geometry at differing viewport aspect ratios.

## Validation

`ctest --preset godot-debug -R
 '^(driving_camera_host|driving_camera_track_host|driving_input_host|driving_lab_host|driving_asset_host|track_default_projection_host)$'
 --output-on-failure`:6/6passed42.05s on2026-10-21.
Godot4.7.2 stable ed1daf0bf; normal Debug extension supplied by the parent task.

- The camera fixture loads the real original911TURBO renderer and native vehicle,
  cycles to the actual forward-facing bumper, validates native outside/bumper
  projections through engine matrices at4:3/16:9/portrait aspects, then returns
  exterior visibility while preserving loaded reflection-resource counts.
- It checks independent B press/repeat/release with the original eased transition,
  forward/rear direction within bumper, consumed release, focus/UI clearing and
  selected-POV retention. It checks native mouse idle return for exterior and bumper,
  camera/physics isolation, restart, and original track-camera clearance.
- The existing full CarViewer interaction test now enters bumper before its
  terminal-error/Return path and verifies preview visibility, near plane and
  projection restoration on the same renderer instance.
- Centralized InputMap tests verify V/B actions and held-lookback hints.

The first camera smoke run queried reflection_surfaces from get_visual_state,
which does not own that diagnostic key. It was corrected to get_state; the
resource-preservation assertion remains. Final camera smoke passed without script
errors. PC tests establish host/native contracts, not PS2 numerical/visual/timing
or spatial-rendering equivalence. Dashboard, pursuit/birds-eye/split-screen,
active camera shake and original-runtime comparison remain outside this consumer.

After the final native bridge dashboard boundary rebuild, a focused rerun of
`ctest --preset godot-debug -R '^driving_camera_host$' --output-on-failure`
passed1/1 in3.92s. No other repeat or benchmark was needed.

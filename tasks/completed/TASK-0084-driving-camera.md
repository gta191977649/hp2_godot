# TASK-0084 - Recovered Test drive camera and temporary mouse orbit

Status: Completed (bounded exterior camera; not Verified)
Date: 2026-10-21 Australia/Sydney

## Scope and acceptance

User requests recovered HP2 default driving viewpoint, Godot default FOV,
mouse-driven SA-style temporary orbit with idle return, and V front/back view
switching according to original HP2 camera design. Explicit port-side exceptions:
Godot default projection and mouse/idle/V bindings. Do not claim those are PS2
inputs or tuning. Recover the underlying follow/front/back camera from Debug
NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.

- [x] Read-only evidence for original camera layout, pose/update and view modes.
- [x] Godot-independent bounded native pose producer, practical behavior tests.
- [x] Test drive default native pose + Godot default FOV/aspect policy.
- [x] Mouse orbit, idle return, V front/back, focus/UI/pause/restart lifecycle.
- [x] Native/host checks, strict preservation, records/progress and continuation.

Current driving_lab.gd uses invented distance/pitch/smoothing and a recovered
horizontal FOV override; replace within requested scope. Independent race_scene
camera is relevant shared consumer, but user explicitly requests Test drive.
Unknown original camera collision/topology/replay services remain explicit.
No Verified promotion from PC tests. Existing unrelated workspace edits preserved.

## Findings and implementation

Evidence E-0084-C01/M01/H01 under reverse/task0084. Native default fullscreen
POV1 uses authored Lag/Height/LatOffset, acceleration history, speed/heading,
yaw/pitch corrections, normal lookback easing and original topology/near-car
clearance. Default settings Spring0/LookbackMode1 are bound explicitly. All64
configuration camera records are exercised, including14original row0 fallbacks.
Final authored Angle/MaxAngle scratchrotation is not used by exterior output;
disassembly and a counterevidence test prevent inserting an invented pitch.
Stationary vector2 normalization uses evidenced EE zero-divide00251CE4 semantics.

V toggles normal/front lookback, not originalPOVcycling. Mouseorbits the live
native target and returns after1.5seconds idle (host rate6/second, sensitivity
.15degree/pixel). Godot75verticalFOV/KEEP_HEIGHT is the user's explicit projection
exception. Zero motion, key repeat, consumedUI and unfocusedinput are gated.
Suspension clears temporary orbit and retains selectedorientation. Exit restores
viewerpose/FOV/aspect. Native camera candidate/privateMemory protect physical
state; the original default CarTypeInfoArray row0 is bound from configured data.

## Validation

Tools CMake3.29.2/Ninja1.12.0/GCC13.2.0/Python3.14.4; Godot4.7.2 ed1daf0bf;
godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b unchanged.
- cmake --build --preset native-debug --target driving_camera_tests -j4: passed.
- ctest --preset native-debug -R '^driving_camera$' --output-on-failure:1/1passed3.42s (64rows,14fallbacks,wall,240drivingframes,Anglecounterevidence).
- cmake --build --preset godot-debug --target ps2_port -j6: passed; final DLL written to normal godot/bin (no staged-only artifact this task).
- ctest --preset godot-debug -R '^(driving_camera_host|driving_camera_track_host|driving_input_host|driving_lab_host|driving_asset_host)$' --output-on-failure:5/5passed16.37s.
- py -3 -m unittest discover -s reverse/tests -p test_task0084_source_review.py:7/7passed39.547s.
- historicalTASK0083 tests7/7passed21.019s; existing_body_preservation1/1passed41.69s.

Strictreview84 covers4replacements/19additions,9modified/10new auxiliarypins;
no original stub transitions. Frozen83 JSON unchanged. Review84 LF SHA256
4c9f10509d1ddadfbd329f85e1cbbca398cfc2202af40a8682e52daf1d2a6b96.
Early failed checks and their fixes are retained in host-camera.md: stationary
EE math, stale FaultLab arguments/cleanup and track fixture resume sequencing.
No assertions were removed to hide the failures. PC tests establish bounded port
behavior; no original-runtime numerical/visual comparison or Verified promotion.
No commit created; unrelated user edits preserved.

## Limits and reproducible continuation

Read reverse/task0084/{camera,modes,host-camera}.md before extending. No complete
original mover ABI, pursuit/player/birds-eye/in-car/split-screen/DOF/active shake
claim. Native topology uses recovered geometry with a port-owned candidate query,
not full original persistent TopologyCoordinate storage. PrivateMemory copying
is an explicit isolation seam; optimization requires read-only math API evidence.
Independent race_scene camera remains outside this user's Test drive scope.

Reproduce with the native driving_camera test and five hosttests above. Next
scoped recovery: original camera shake/pursuit/settings services and original
runtime camera pose captures before any Verified promotion. Original FOV evidence
remains accurate but user-requested Godot projection stays in force.

# TASK-0084 host camera policy and validation

E-0084-H01 is a port-side contract, not original game evidence. The user expressly
requests Godot default FOV, mouse temporary orbit/idle return and V front/back.
The native pose remains anchored by E-0084-C01/M01 to Debug NFSHP2.ELF SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.

The Test drive adapter now consumes update_driving_camera(elapsed,look_back),
which returns mapped position/target/up and original POV/lookback diagnostics.
Native camera candidate state and a private Memory copy protect physics and
previous camera state against failures. Load/restart/replay/record/track reset
camera history. Missing original camera rows bind original CarTypeInfoArray[0]
from configured CORVETTE index0; no numerical camera defaults are invented.

The mouse adapter uses0.15degree/pixel, clamps pitch override to+/-75degrees
and final elevation to+/-80, allows full wrapped yaw, waits1.5seconds after a
nonzero mouse move, then exponentially returns at rate6/second. Once both offsets
are below.0001radian it applies the exact current native pose. These values are
host interaction policy, not recovered PS2 tuning. Zero-motion events do not
restart the idle timer. Native follow history continues under the override.
V toggles normal/lookback and clears temporary offsets; key repeat, consumed UI
input and unfocused input do not toggle. Focus or console suspension clears mouse
override while preserving the user's selected orientation. Exiting Test drive
restores the viewer camera pose, FOV and keep_aspect. Independent race_scene camera
is deliberately outside this user's Test drive scope.

Projection is queried from a new Camera3D:75degree vertical FOV and KEEP_HEIGHT.
Three real viewport projections (4:3,16:9,portrait) verify that the vertical FOV
stays75 while horizontal FOV follows aspect. Original HP2 horizontal FOV evidence
is retained; TASK0084 is an explicit user-requested projection exception.

Tests use real native car/camera state and a small host viewer fixture to exercise
mouse, exact idle return, original front/back easing, V repeat/UI/focus gating,
resume, restart and absence of physical advancement. An additional run loads
original track11 and requires30finite constrained camera frames. The existing
actual CarViewer driving_asset_host test covers model loading, real driving input,
projection, mouse return and return-to-viewer restoration. Its old FaultLab test
double still took7driving arguments and lacked clear_particles after earlier
features; it was updated to the current8argument interface without weakening
its injected-failure checks. The first actual-scene run exposed that stale
harness; the corrected scenario passes.

The first camera run exposed stationary vector2 Normalize(0,0) host NaN handling;
E-0084-C01 documents the instruction-anchored EE fix. The first added track test
forgot that set_track pauses/reset Session; it now calls run before ticking,
matching the real Test drive startup sequence. Both failures and fixes are
recorded rather than hidden. No original execution or numerical/visual equivalence
comparison was performed; active camera shake and other mover branches remain
outside the supported native slice. See camera.md for the full boundaries.

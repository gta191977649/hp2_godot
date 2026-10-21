# TASK0054 world-effect screen projection

## E-0054-P01: host Y convention under offscreen rendering

Observation: the TASK0053 static centered Sun fixture could not distinguish a
vertical mirror. Original Track11 Sun native(-3900,650,1300) is unchanged, mapped
to host(650,1300,-3900). At camera(-732,300,-98), yaw/pitch-only diagnostic records
expected projection(480,270),(480,370.7323),(580.7323,270). Before correction,
strong on-minus-off contribution centroids were(480.26,270.46),(480.08,169.88),
(578.71,270.32): vertical motion was mirrored around270, horizontal was correct.
This reproduces the user's camera-following report for a stationary world anchor.
Ignored original captures/log: local/task0054/before-images and sun-before-preserved.log.

The Sun and particle adapters explicitly assign shader POSITION from top-origin
original CPU screen coordinates. Unlike ordinary geometry (and the sky), they
bypassed PROJECTION_MATRIX and its render-target Y sign. The shared host helper
screen_projection.hpp maps top-left UV to clip space and multiplies clipY by the
actual GPU projection's Y sign, passed explicitly from vertex stage. This applies
once to both adapters, including writing particle depth proxies that reuse the
source vertex function. It changes no native world positions, particle integration,
Sun layer offset/sweep, flare behavior, GS fixed16 XY, alpha, depth or readback rect.
Sun/flares remain camera-facing screen packets anchored by original projection;
not all lens-flare elements are independently fixed objects in world space.

First shader attempt referenced a vertex built-in from global helper scope and
failed compilation; corrected helper takes projectionY as an explicit argument.
Final preliminary actual-window centroids(479.99,268.25),(479.96,369.01),
(578.52,268.46) agree with expected off-center positions within~2pixels. Full
moving-camera regressions additionally check translation, real occlusion and
actual particle packet bounds. No syntheticSun or arbitrary camera offset used.

## Native evidence boundary

Retail SLUS_203.62 SHA256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only Ghidra decompilation refreshed at001252B0 StuffSun and00128DA0
particle group submission (existing E-0052-E05 / particle records). Sun loads
asset position+4/+8/+c, passes it to0011B350 projection and0011B2C0 camera transform,
then builds screen quads and32x32 readback. Original core is unchanged here.
Fresh requests/results: local/task0054/ghidra-001252b0.json and ghidra-00128da0.json.
Attempted002AD390 was not a Retail function; its tool error is retained separately
and is not evidence of a recovered executable implementation. Debug/string
addresses are not reinterpreted as Retail code. Native naming/provenance remains
in the pre-existing version-bound records.

This is a measured Godot transport fix, not original-console Verified behavior.

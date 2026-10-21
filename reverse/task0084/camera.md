# TASK-0084 native driving camera evidence

E-0084-C01. Behavior authority Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
version/build independently unknown. Read-only Ghidra captures in ignored
local/task0084/camera; camera-captures.json records response hashes.
Status: implemented bounded port-side projection, not Verified against PS2.
Shipped Retail GLOBAL/GLOBALB.BUN camera rows remain a separate asset authority.

## Confirmed records

- DriveCameraMover constructor0015BD10 stores the followed car and authored
  camera table, sets boom yaw/pitch and acceleration history to zero, initializes
  LastCarYaw from State.Angle, and TouchingGroundFactor1. Either missing full or
  split POV count selects CarTypeInfoArray row0 CameraInfo. ResetState0015BF28
  resets the corresponding camera state. Native host allocation is not this ABI.
- CameraInfo is CarTypeInfo+3C0: NumFullScreenPOVs0, NumSplitScreenPOVs4;
  fullscreen POV array+10 and split+90, stride20. POV has Type0, Lag4, Height8,
  LatOffsetC, Angle10(u16), MaxAngle12(u16). GetPOV0015BFE0 chooses the split
  array only for EVIEWMODE_TWOH. The current host binds fullscreen only.
- Original player default is POVIndex1, CameraSpring0 and LookbackMode1 (see
  modes.md / E-0084-M01). All64 shipped rows have either compatible outside
  POV1 or the documented zero-count fallback:50 explicit,14 fallback. Row0 is
  CORVETTE.911TURBO/ELISE POV1 is OUTSIDE2, Lag5.8, Height1.7, LatOffset0,
  Angle600, MaxAngle9158. Values are decoded from original data, not guessed.
- SetLookBack0015F678 sets DesiredLookBackParam0/1. Update0015C2B0 moves the current
  parameter toward that value at constructor LookbackSpeed*WorldTimeElapsed;
  bCos(param*32768) supplies a cosine eased half-turn. Non-birdseye lookback moves
  the camera from behind the car to its front. This is distinct from POV cycling.
- Update0015C2B0 uses reflected backward velocity, a speed threshold/blend and the
  body-forward vector to determine follow heading. Its yaw/pitch correction and
  ground factor use GetCorrectionYaw0015BC10/GetCorrectionPitch0015BC88. DIRT
  road-noise001530E8 perturbs boom pitch using RoadNoiseAmplitude327538.
- Original acceleration lag uses local longitudinal acceleration, twenty samples
  at a sixty-Hz fractional-time accumulator, asymmetric acceleration/deceleration
  tests, ten-sample edge history, yaw attenuation, hold timeout, speed attenuation,
  return and adjustment constants. Default CameraSpring0 retains that lag branch.
  Constants are read from the bound image at3274D4..327538 and3250A0..3250E8.
- eTranslate0011BDD8 and existing eRotateZ/eRotateY/eMulMatrix preserve the native
  row-vector convention. The outside eye comes from the inverse car/boom transform.
  CameraMover::IsoProjectionMatrix00157618 creates a right/down/forward (+Z) view,
  offsets its look target by the pre-constraint projection and recreates look-at.
  Returning position/target/up lets the host change projection/FOV separately.
- The zero-velocity/blend0 path uses vector2 bNormalize00251CC8, not vector3
  normalization. Listing confirms DIV.S at00251CE4. EE zero-divisor saturation
  through existing ee_div_s_zero preserves zero components; host0/0 NaNs were
  counterevidence against initially reusing the vector3 helper. Its divisor==0
  fallback is unitX, unlike the vector3 helper's requested-length X fallback.

## Counterevidence: authored Angle in the outside branch

The full Update loads Angle/MaxAngle and performs a final eRotateY on its scratch
boom matrix. It would be incorrect to assume this adds a pitch to the outside
pose. Listing0015D700..D8C0 confirms eRotateY0015D764 writes s5. The bOutside
branch0015D770 then calls IsoProjection00157618 at0015D784 using output stack2A0,
eye stack160, car+60 look position and projection stack150; it does not pass s5.
Only the in-car branch0015D794..D834 consumes the Angle-rotated matrix. The original
outside projection was already captured before this last scratch rotation. Native
tests mutate Angle/MaxAngle and require unchanged outside framing. Applying the
600 bAngle pitch directly would introduce behavior absent from this branch.

## Topology constraint slice

EnforceMinGapToWalls00157420 first calls ConstrainToTopology00152500 with
BY_SCALE,true, margin324F9C and wall-up1. TestSegmentToPoly001516D8 builds exact
polygon bounds, performs inclusive bounds overlap and strict segment intersection,
then accepts only a closer intersection. The scale branch shortens the boom by
margin. Existing TrackPolygon::SegmentIntersects and PointInShaft provide these
native geometry predicates. The gap loop performs up to three average-normal
adjustments for non-through terrain (TerrainTypeInfo.ShouldCameraGoThroughIt+0D),
shaft elevation>=0 and below324FA0. Native shape vertices supply the original local
longitudinal bounds for the near-car upward correction; the unrelated event proxy
at Car+34 is never misread as the original DynamicCollisionBody layout.

The native projection uses the repository's port-owned fresh Topology.intersecting
candidate query in a box around the original interest sphere. Persistent original
TopologyCoordinate cache/allocation and exact full-world polygon list ABI are not
claimed. Null topology is an explicit empty laboratory world. If a wall adjustment
needs near-car bounds and the caller supplied no original shape, it throws at
001351C8. Unsupported POV kinds/default-table omissions and nonfinite time are
explicit failures. The elapsed loop has an explicit bounded interval limit.

## Implementation and validation

Port-side DrivingCamera and DrivingCameraFrame are declared in camera.hpp;
reconstructed arithmetic lives in original indep/src/cameramover.cpp. No original
whole-entry stub is changed. update reads a private Memory copy supplied by the
host; state reset/candidate commit is host-owned and independent of driving physics.
FOV is deliberately absent from this native pose API: the user explicitly requests
Godot defaults while restoring the original view placement/movement.

Commands2026-10-21: cmake --build --preset native-debug --target
 driving_camera_tests -j4 passed; ctest --preset native-debug -R '^driving_camera$'
 --output-on-failure passed1/1 in3.42s (final Angle counterevidence test included). CMake3.29.2, Ninja1.12.0, GCC13.2.0,
Python3.14.4. Tests cover zero-elapsed stationary pose and authored5.8/1.7 values,
lookback easing and release, reset, invalid elapsed, all64 config rows/14fallbacks,
real wall scale/gap constraints and240native acceleration/steering frames while
requiring finite camera state and unchanged physical fields. PC regression is not
original numerical, visual or timing equivalence certification.

## Unresolved / outside this supported branch

Full original player/pursuit tracking, birdseye lookback mode0, in-car/dash movers,
splitscreen projection, tweakable overrides, original CameraSet/DOF and active
camera shake services are not implemented by this outside own-car projection.
The host pointer/mouse orbit and idle return are explicitly user-requested port
controls, not recovered original PS2 input. Original active render-camera cadence,
EE/VU exceptional domains beyond the recorded zero divide and original runtime
comparison remain unresolved. No original runtime footage was used as evidence.

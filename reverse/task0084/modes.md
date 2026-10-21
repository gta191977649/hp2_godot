# TASK-0084 original camera view modes and defaults

Evidence E-0084-M01. Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Version/build independently unknown. Read-only Ghidra inspection; private captures
local/task0084/modes. No original-runtime observation or Verified claim.

## Confirmed

- cPlayerSettings::ResetToDefaults001EA330 sets DriveCameraPOVIndex1, except
  SkipFE selects SkipFEPOVIndex. It sets DriveCameraAlternate0, CameraSpring0
  and LookbackMode1. These are independent fields, not two aliases for one mode.
- Player::ChangeCameraMover entry001752F0 constructs CM_DRIVE with current
  player POV index, lookback-speed constant003252E8, CameraSpring as DisableLag,
  lookback0 and PerfectFocus1; callsite0017545C belongs to that function and is
  not its entry. It then applies Player.bLookBack to DesiredLookBackParam and
  validates the selected POV via CycleDriveCameraPOVIndex.
- DriveCameraMover constructor0015BD10 selects CarTypeInfo.Camera; if either
  full-screen or split-screen count is zero it uses CarTypeInfoArray[0].Camera.
  GetPOV0015BFE0 uses the split-screen array only for EVIEWMODE_TWOH. Other modes
  use the full-screen array. POV records are32bytes; CameraInfo full-screen array
  begins10hex and split-screen array90hex.
- POVTypes has POV_BUMPER0, POV_INCAR1, POV_OUTSIDE2 (retained E-0050-U02 type
  capture). The prior original-asset E-0050-U02 probe establishes 911TURBO/ELISE
  full-screen POV1 OUTSIDE with Lag5.8, Height1.7, LatOffset0, Angle600,
  MaxAngle9158; POV2 is another OUTSIDE entry with Lag4.7 and Angle800. These
  records are authored camera parameters, not a formula for final eye position.
- Player::CycleDriveCameraPOVIndex00176EE0 cycles at most4 candidate indices,
  checking non-null GetPOV results. With SkipFE nonzero it accepts any available
  POV. Otherwise a car with no dash accepts BUMPER or OUTSIDE; a car with HasDash
  accepts INCAR. It falls back to index0 after4 unsuccessful attempts.
- Player::RacingJoyHandler00177198 distinguishes JOY_EVENT_CAMERA_POV_CHANGE
  from JOY_EVENT_CAMERA_LOOKBACK. POV_CHANGE advances index+1 after its real-time
  ignore gate and requires racing input mode and a CM_DRIVE mover. LOOKBACK
  writes Player.bLookBack from param!=0 and writes the current or saved drive
  mover DesiredLookBackParam0/1. It also notifies the animated driver if present.
- DriveCameraMover::SetLookBack0015F678 writes DesiredLookBackParam0 or1.
  LookingBack0015F650 tests it>0; IsLookingBack0015F698 tests equality1.
- In Update0015C2B0, LookBackParam approaches DesiredLookBackParam at
  LookbackSpeed*WorldTimeElapsed and clamps at its target. Its standard angle
  uses the recovered half-cosine progression, then bAngle conversion:
  weight=1-(cos(bAngle(LookBackParam*32768))+1)*.5;
  LookBackAngle=bAngle(weight*32768). Normal mode rotates the exterior boom
  about Z by this angle. Thus full standard lookback reaches a half-turn.
- Update's birds-eye branch is selected when a non-null Player has LookbackMode0.
  Playerless and default LookbackMode1 contexts select the normal branch.
  The full birds-eye and pursuit-car branches alter additional height/direction
  behavior and must not be assumed equivalent to ordinary exterior lookback.
- GetLookbackCar0015C138 may return a pursuer/pursuit target via pCopManager;
  absent manager gives no target and no slow-lookback adjustment. This is a
  separate service from switching the default view.

## Port policy and unresolved limits

V front/back toggling is the user's requested host binding. It can latch the
original normal lookback state while preserving the selected authored OUTSIDE
POV. The original racing callback shown above uses press/release param semantics;
there is no evidence here that the original PS2 maps a V key or latches lookback.
Do not describe a front/back toggle as the original POV_CHANGE cycle: that cycle
can include bumper/in-car and multiple exterior distances.

Godot default FOV/aspect and temporary mouse orbit with idle return are explicit
user-authorized host policies. They do not come from these recovered camera
settings. The full original camera still depends on topology, acceleration lag,
road noise, player/pursuit, camera shake and display services; recovery of those
belongs to the separate pose implementation record. This modes audit establishes
state and entry behavior, not complete original camera numerical equivalence.

## Reproducibility

Read-only inspect.decompile and inspect.references_to for the listed addresses;
queries and SHA-256 response hashes are retained in modes-captures.json. The
0017545c.json response explicitly identifies its enclosing entry001752F0.
Prior POV asset/type evidence is in reverse/task0050 and ignored local/task0050.
No Ghidra renames, comments, annotations or database writes were performed.

## Additional camera clearance evidence

Read-only captures added for GetBoomDistToWall0015DA58, TrackPolygon segment
intersection001515E0 and TestSegmentToPoly001516D8. Camera pose recovery should
follow the actual callsites: the active OUTSIDE Update0015C2B0 path uses
EnforceMinGapToWalls00157420/ConstrainToTopology00152500. The separate boom helper
is not evidence that this OUTSIDE path calls it.

GetBoomDistToWall initializes its result from3250F0 and expands the car/eye3D box
by3250F4. It iterates vertical topology polygons, skipping terrain with
ShouldCameraGoThroughIt!=0. TerrainTypeInfo type fixes that byte at+0D and table
stride90hex. It builds each candidate polygon box from four vertices, rejects
non-overlap, then retains minimum car-to-intersection distance. This helper's
four-vertex loop differs from TestSegmentToPoly's NumVertices loop.

TrackPolygon::SegmentIntersects001515E0 forms d1/d2 from each endpoint minus
vertex0 dotted with polygon normal. It requires d1*d2<0: an endpoint on the plane
or a coplanar segment does not pass. Intersection uses abs(d1)/(abs(d1)+abs(d2))
interpolation from the first endpoint, then existing PointInShaft00151428.

TestSegmentToPoly001516D8 initializes a3D box from vertex0 and expands through
actual NumVertices. It requires overlap with the passed segment box and a segment
intersection. In one-sided mode it rejects dot(intersection-target,normal)>=0.
The distance must be strictly below *pNearest before updating it and returningtrue;
equality is not a hit replacing the current nearest candidate.

EnforceMinGap00157420 first invokes ConstrainToTopology(eCONSTRAIN_BY_SCALE,true,
carpos,temporaryeye,324F9C,1). It then evaluates all interested polygons whose
terrain blocks cameras. For PointInShaft positions with nonnegative plane gap less
than324FA0, it averages normals times the gap deficit into an adjustment. It
repeats at most three adjustment passes and outputs adjustment.w0. The separate
pose recovery owns implementation and exact fresh-query service bindings.

bNormalize00251D28 (already implemented in src/bMath.cpp) divides by norm/length;
a zero resulting norm gives(length,0,0). eNormalizedDirection0011B520 uses the VU
reciprocal norm of p1-p0 instead. Their zero-vector domains must not be conflated.
No unknown exceptional EE/VU result is inferred from host standard math.

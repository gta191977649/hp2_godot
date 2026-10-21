# TASK-0081 - Driving frame state and reset leaf

2026-10-21 Australia/Sydney. Authority: Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
User-labelled build date2002-07-08 is not independently confirmed. No original
execution comparison or Verified claim. Ghidra access was read-only.

## Scope and connected implementation

NativeBackend now captures old frame motion once before movement, completes
acceleration/distance state once after all static collision retries, then publishes
and advances WorldTime. The existing Memory/collision/skid transaction owns these
new writes. Constructor initialization uses the complete recovered ResetCar leaf
before the existing explicit fixture placement/gear setup; it does not claim the
original whole Car constructor.

The shared source file remains original indep/src/car.cpp. Port-owned
begin_movement_state/finish_movement_state bind only the recovered state slices,
as declared in port/driving/frame_lifecycle.hpp. Whole Car::BeginMovement and
FinishMovement explicitly throw for their missing topology/path/Player remainder.
They are partial entries, not completed game lifecycle. Basic steering/forces
and static collision logic are otherwise unchanged.

## E-0081-L01 - Exact state stores and arithmetic

Fresh listings00131D78..00131E54,00131E58..00132000, full ResetCar decompile/
listing001BD6C0..001BD867 and Car/CarState type snapshots are indexed with hashes
in the ignored local/task0081/lifecycle/manifest.json. Public index: evidence.json.

- BeginMovement, positive t only: full16-byte Position Car+60 -> vOldPosition+470,
  Velocity+70 -> vOldVel+480, AngularVelocity+D0 -> vOldAngVel+490.
  The fixture also calls existing BeginTimestep001BD938 once for crossed terrain.
- FinishMovement00131E90..00131EE8, positive t only: subtract old from current
  velocity/angular velocity, multiply each of all4 lanes by separately evaluated
  1.0f/t, then write Car+80/E0 (State+40/A0). Do not derive acceleration after
  every solver substep or collision retry.
-00131EF0..00131F2C: Odometer State+368 adds speed*t unless speed<0 or1000<speed.
  Inclusive0/1000 and the two rejection comparisons are retained; unordered
  inputs are not silently converted into an in-range assertion or clamp.
-00131F30..00131F7C: DistanceTraveled State+36C adds
  sqrt((dx*dx+dy*dy)+dz*dz), oldPosition-currentPosition, then publishes the full
  current position to Car+470. W participates in acceleration, not XYZ distance.
- Position/path/interest checks after these ranges, lights/horn and non-null
  Player/replay consumers remain outside this supported slice.

### ResetCar001BD6C0

The full leaf clears motion/acceleration XYZ while preserving W; restores body
identity and Angle/SteeringAngle; clears six wheel scalar arrays, slip and
traction; restores four ROAD/contact entries. It sets ground count4 and calls
CalculateWheelMatrices before final scalar stores. Final values include RPM1000,
redline-sync -1, neutral1, clutch word1, boost/traction1; relative matrix3C0..3FF
and the full validity word400 become zero. Original store groups/order are retained.

Position/GeometryPosition, configuration/topology pointers, Odometer368,
DistanceTraveled36C, DriverMode39C and BurnOutAllow3B0 survive. A blanket object
zero/reset or a fixture restart is not an equivalent implementation.

## E-0081-R01 - Route reset prerequisites and explicit boundary

Fresh29 read-only route captures are indexed privately at
local/task0081/route/manifest.json. Car::GetRoute00133310/GetRoutePoint00133340
dispatch controller getters. The player route owner is TrackedController:
current point+0C; GetCurrentRoute00233EE0 uses point RouteNumber+6F to index
manager+10. FindWhereIAm00233F10 requires original route point search and
persistent TrackPathCoordinate.DistanceAroundTrack.

TrackRoutePoint::GetDistanceAway001BE750 combines XY distance and wrapped track
distance difference minus24; FindWhereIAm uses distance-aware flags0x22.
Nearest XY, a zero progress value or spawn is not an authorized substitute.

Original route data is present: LoaderTrackRoutes001BFFC8 accepts chunks34121/
34122; Debug framing001BF9B0 (428 + NumPoints*70) consumes the configured Retail
Track11 route payload311696 bytes exactly:16 routes, racing1..12 and traffic
50/51/60/61. Signpost payload600 bytes is75 rows. This confirms only the framing
of that one Retail file, not full Debug/Retail field equivalence. The bridge must
be independently established before loader/controller use. Raw assets stay private.

Remaining dependencies are code/data-layout recovery: route/signpost initialization,
persistent TrackPathCoordinate progress/length-wrap behavior, controller route
ownership and ResetCore movement lifetime. Full reset is therefore not claimed.

Car::Move00131B90 consumes ResetMe after mover execution, including remaining
movement after collision backup. The fixture now checks that request after each
canonical PhysicsCar Move and throws at original ResetToRoutePoint00132FB0 if it
cannot perform recovery. A rejected candidate preserves the committed frame and
request; it does not retry, teleport or clear the request silently. Full route
recovery remains the next task rather than being hidden behind a successful frame.

## Validation and remaining limits

Synthetic frame tests cover positive/zero timestep, all4 acceleration lanes,
XYZ-only distance, inclusive odometer limits/unordered input and full-entry throws.
Original-data tests check ResetCar write extent, preserved fields/W payloads,
four wheel matrices/defaults, production frame state and transactional recovery
rejection. Existing steering/shift/replay/contact/track tests provide regression
coverage. Exact final build/test commands and results are in TASK0081 and the
progress log. No tests claim EE or original trajectory equivalence.

No input/UI, damage, water, sound, Player or full World implementation was added.
Continue with route loader/coordinate/controller ownership above, not a generic
host reset policy. Matching runnable Debug original component traces are still
required for numerical verification.

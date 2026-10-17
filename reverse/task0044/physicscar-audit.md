# TASK-0044 PhysicsCar, timing and input audit

E-0044-P01..P06; 2026-10-17. Approved Debug target NFSHP2.ELF,
SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
Build date/revision not independently established. All observations below come from
live read-only Ghidra requests; private raw results are under
`local/task0044/physicscar`. No database mutations, emulator execution or synchronized
original numerical comparisons were performed. Ghidra imported types are useful
symbol evidence but instructions prevail when a type disagrees.

## P01: exact methods and members

Debug source marker `../indep/src/physicscar.cpp`; header markers include
`../indep/inc/PhysicsCar.hpp`, `CarMover.hpp`, `ComponentPhysics.hpp`.
The imported ABI calling convention is labelled __stdcall by Ghidra; this is not
a reason to impose a Windows host ABI on PS2 code. Native instance views carry
Memory plus a logical PS2 object address explicitly.

|Debug entry|Imported signature|
|---|---|
|001370c0|`PhysicsCar * __10PhysicsCarP11CarTypeInfoP9CarDriverP3Car(PhysicsCar * this, CarTypeInfo * info, CarDriver * driver, Car * car)`|
|00137410|`void DoSnapshot__10PhysicsCarP14ReplaySnapshot(PhysicsCar * this, ReplaySnapshot * snapshot)`|
|001375b0|`void Calibrate__10PhysicsCar(PhysicsCar * this)`|
|001376c8|`void _$_10PhysicsCar(PhysicsCar * this, int __in_chrg)`|
|00137748|`void ComputeAuxiliaries__10PhysicsCar(PhysicsCar * this)`|
|00137798|`DampingSuitability CheckDampingSuitability__10PhysicsCar(PhysicsCar * this)`|
|001378c0|`void BottomOut__10PhysicsCar(PhysicsCar * this)`|
|00137cc0|`float GetBrakeSettings__10PhysicsCari(PhysicsCar * this, int wheelpos)`|
|00137d18|`float Move__10PhysicsCarf(PhysicsCar * this, float t)`|
|00138c80|`void InitializeStateFromCar__10PhysicsCar(PhysicsCar * this)`|
|00138d90|`void ResolveForces__10PhysicsCar(PhysicsCar * this)`|
|00138ec0|`void IterateForces__10PhysicsCar(PhysicsCar * this)`|
|00139110|`void UpdateCar__10PhysicsCarf(PhysicsCar * this, float t)`|
|00139418|`void SetBoost__10PhysicsCarf(PhysicsCar * this, float fBoost)`|
|00139478|`void JumpStabilizer__10PhysicsCar(PhysicsCar * this)`|
|001395d0|`void SetFrictionBoost__10PhysicsCarf(PhysicsCar * this, float boost)`|

PhysicsCar size0x78; CarMover starts0 and PhysicsSystem/PhysicsVehicle subobject
starts+8 (ComponentList+8, Size+10, secondary vtable+14).

|Offset|Recovered member|Imported type|
|---|---|---|
|0x0|`Driver`|`CarDriver *`|
|0x4|`__vtable`|`PhysicsCar__vtable *`|
|0x18|`pBody`|`RigidBody *`|
|0x1c|`pSteering`|`Steering *`|
|0x20|`pSuspension`|`Suspension *[4]`|
|0x30|`pWheel`|`Wheel *[4]`|
|0x40|`pEngine`|`Engine *`|
|0x44|`pDriveTrain`|`DriveTrain *`|
|0x48|`pAerodynamics`|`Aerodynamics *`|
|0x4c|`CTI`|`CarTypeInfo *`|
|0x50|`State`|`CarState *`|
|0x54|`pCar`|`Car *`|
|0x58|`BrakeStandTimer`|`Timer`|
|0x5c|`BrakeStandTimerStart`|`Timer`|
|0x60|`BrakeStanding`|`bool`|
|0x64|`ThrottleReleased`|`bool`|
|0x68|`LurchsLeft`|`int`|
|0x6c|`RoofResetTimer`|`Timer`|
|0x70|`FrictionBoost`|`float`|
|0x74|`YawControlMultiplier`|`float`|

Original constructor clears the three Timer PackedTime fields, installs component
objects in body, four alternating wheel/suspension, engine, drivetrain order, sets
sway-bar pairs0/1 and2/3, then calls Calibrate, InitializeStateFromCar and
ComputeAuxiliaries. It finishes YawControlMultiplier=1, ThrottleReleased=1,
LurchsLeft=2, BrakeStanding=1. These booleans are declared bool but actual generated
stores in this scope are32-bit; preserve instruction stores rather than host bool ABI.

Vtables: CarMover2A67D0, PhysicsVehicle2A6B18, PhysicsCar secondary2A6AD8,
PhysicsCar primary2A6AF8, TwoWheelAckermanSteering2ADAD8. Names retain original
`_vt$...` symbol spellings in the private symbol capture.

## P02: algorithm comparison and concrete layout deltas

The existing normal PhysicsCar algorithm sequence remains recognizable, but address
replacement alone is insufficient. Debug Move returns its input t; its exact entry
is137D18, not Retail137D90. Existing projected exclusion of Car+20==1 corresponds
to the named REAL_CONTROL burnout path, not an arbitrary generic movement mode.
The pre-start flags now have recovered BrakeStanding/ThrottleReleased/LurchsLeft
names; RaceStage RACE_STAGE_RACING takes the ordinary running branch.

Calibrate: rigid body center of gravity/inertia; optional UseNewSuspensionCalibration;
front/rear static-load split; mass*gravity plus auto-balanced aerodynamic downforce;
four suspension calibrations, then four Wheel::Calibrate calls. Wheel calibration
loads WheelMomentOfInertia327D20. InitializeStateFromCar copies body/wheel state,
uses TestRPM32818C unless exactly100, then restores gear/clutch state. ComputeAuxiliaries
refreshes component auxiliaries, runs BottomOut, then secondary virtual ResolveForces.
ResolveForces, IterateForces and UpdateCar retain the recovered order and five
force iterations, rather than replacing them with host engine physics.

Crucial Car layout changes: Debug pCarDamage+844 versus Retail+848;
BlownTireBits+848 versus+84c; ResetMe+854 versus+858; BlinkTimer+858 versus+860;
BlinkDuration+85c versus+864; bPoofed+868 versus+870. State still starts+40 and is
0x410 bytes. CarTypeInfo stays0x560 with Body1e0, Suspension230, DriveTrain270,
Engine2b0, Aero2f0, Control300. All accesses need the Debug target identity.

Debug Move reads fDamageBoostOverride from Car+460. The existing bounded Retail
projection reads controls+460; do not carry that operand into Debug. Confirm from
Debug listing/register base, not by renaming the earlier projection comment.

## P03: globals and migration manifest

Debug gp is32C9F0 (original symbol `_gp`), Retail gp3358F0. Do not apply one universal
delta. Instruction-paired global candidates with individual source PCs are retained
in `global-map-candidates.json`; validate each referenced operand before using it.
Important Debug originals: pCurrentRace327258, WorldTimer327CF8,
WorldTimeElapsed327D08, UglyTimestepHack327D78, MaximumTimeStep327078,
NFSAdjust327074, SpeedDebugInfo3270A8, BrakeSettings2E7D90,
EBrakeSettings2E7DA0, SpeedDebugX2E7DB0, SpeedDebugR2E7DC0,
PostCollisionYawControlProfile327018. Named read-only symbol captures accompany them.
Constant areas also move: PhysicsCar coefficients start324DB0, timing constants
3256CC, yaw constants325914. Old virtual target checks must be replaced with actual
Debug entries rather than comparing Retail addresses inside a Debug image.

## P04: input conversion

RealDriver remains size280: Steering210 (bAngle), Throttle214, Brake218, EBrake21c,
DesiredGear220, SportGear224, NitroFlag228, pCar230, Port234,
JoySteeringDigital250, JoySteeringAnalog251, SteeringInput254,
ThrottleInput25c, BrakeInput264, ReverseInput26c, EBrakeInput274.
Debug JoyHandler_Steer13B3B8 inverts selected centered byte input; DoSteering13B438
uses current angle, GetYaw, SteeringRangeTable2E8080 or wheel range2E8068,
powerslide/countersteer terms, inverse SteeringCurveTable2E80B0,
SteeringSpeedTable2E8098, rate/return limits and Control.Steering before bAngle
encoding. DoThrottleBrakeReverse13B948 preserves explicit forward/reverse routing.
SetSteering1A3540 implements the two-wheel Ackerman matrices.

The direct laboratory0.5-radian request is a port policy, not this original controller
chain. Migration should feed named SteeringInput and execute the complete recovered
DoSteering path, or explicitly retain a raw steering-angle laboratory mode. Do not
present a fixed0.5-radian adapter as an original normalized-control algorithm.

## P05: scheduler and World gap

PrepareRealTimestep1A0C50 scales host video elapsed, adds quantization error,
clamps by PAL/NTSC limits, quantizes, logs/restores Joylog channels and writes
RealTimeElapsedQuantized/RealTimeElapsed. GetQuantizedRealTimeElapsed1A0C20 and
GetRealTimeElapsedFromQuantized1A0C08 are the direct conversions. ResetWorldTime1A0DF8,
PrepareWorldTimestep1A0E40 and AdvanceWorldTime1A0E50 retain packed timer/frame
accounting, including NeedToPrepareWorldTimestep and residual fractions.

Current NativeBackend calls the low-level timing preparation then PhysicsCar directly.
The original intermediate World::GetTimestep14D408 also processes ReplayManager,
playback pacing, fTimeScale/fTimeScaleTimer and callbacks. World::DoTimestep14D9A8
handles the ordered world/car thinking/movement/collision environment; it is not
substitutable with the isolated car loop. Capture of14D588 DoTimestepMove shows
registered dynamic collision bodies moved through virtual calls. The synthetic
single-car plane laboratory is an explicit narrower environment.

## P06: classify current laboratory substitutions

|Current choice|Classification / migration requirement|
|---|---|
|Identity starting orientation, zero velocities, wheel-based plane placement|Explicit synthetic initial state; no original start/grid placement claim|
|One copied CarTypeInfo, hand-built Car/driver/owner/query/Race objects|Port fixture; replace guessed offsets with Debug members and audited initializer chain|
|RaceStage4, DriverMode1, disabled AI/damage/visual services|Explicit supported context; original named state4 is RACE_STAGE_RACING, not a generic debug mode|
|Fixed0.5-radian normalized steering|Port input policy; complete RealDriver chain or clearly retain raw-angle mode|
|Direct throttle/brake fields, EBrake0|Port input seam; bypasses original reverse/filter/event handling|
|Synthetic horizontal contact and material1|Environment seam; not original road queries|
|steady_clock profiling ticker and raw tick subtraction|Port diagnostic service; not original bGetTicker tick units/rate|
|Explicit temporary pad/W0|Unresolved original stack lane projection; Debug bVector3 calls it pad, not a physical component; retain as limitation pending dependency/read audit|
|Host ordinary finite scalar/vector arithmetic|Implementation scope; no PS2 exceptional-FPU/VU numerical equivalence claim|
|Blur alpha from abs(omega)*0.02*90|Port visualization only; not original renderer algorithm|
|Transaction copy/rollback, bounds/finite guards, host pacing|Port safety and usability behavior; preserve separately from recovered algorithm|
|Missing burnout/diagnostic/effect paths throw|Explicit implementation gap; do not silently claim whole original entry recovered|

## Implementation migration sequence

1. Bind Debug image SHA and a separate address ledger before any virtual/global reads.
2. Give recovered PhysicsCar/Car/CarState instance methods a port ObjectView carrying
Memory and original address. Constructors use an explicitly named port initializer
seam for logical allocation; services are explicit additional port parameters.
3. Move one audited implementation into those original source owners. Legacy projection
entry points forward only. Rewrite globals/targets/Car offsets against Debug captures.
4. Move dependency dispatch to actual Debug component entries; retain exact original
call order and original state pointers. No aliasing Debug data into Retail globals.
5. Reconstruct RealDriver conversion and World timing layer independently from the
single-car fixture, then exercise class entry paths in the production backend.
6. Keep original synchronized comparison as a separate acceptance condition. Existing
Retail and port qualitative tests cannot certify Debug equivalence.

Validation for this audit: live read-only capture of16PhysicsCar functions, relevant
input/timing/helper functions, types and bounded original/Debug listings; no native
code changed during the read-only audit. Implementation continuation is separate.


## P07: implementation continuation and counterevidence

Canonical PhysicsCar instance bodies now live in the original physicscar.cpp and
bind Memory/address through ObjectView. Car and CarState helper bodies follow the
same model. Compatibility native entry points forward to those methods; they no
longer retain a second production algorithm. The obsolete typed five-iteration
IterateForces composite and declaration were removed; solver leaf fixtures and
actual Debug backend integration cover that chain. Unrelated historical Retail
PhysicsVehicle destructor gap00139678 remains preserved separately.

Newly established Debug differences beyond relocation:

- SetBoost00139418 directly stores the requested engine/body/aerodynamic/wheel boost;
  the Retail projected damaged-AI-specific extra engine multiplier is absent.
- GetBrakeSettings00137CC0 reads Control.BrakeBias at CarTypeInfo+30c. The rear
  `(1.0-bias)+(1.0-bias)` expression is compiled through soft-double helpers;
  the native implementation retains double promotion rather than float rounding.
- CalculateWheelMatrices001BDE60 leaves the brake translation at the unmodified
  WheelInfo.Position. Only the tire translation receives suspension height and
  YRenderOffset. Earlier projection adjusted both; the Debug canonical fixes it.
- YawFrictionBoost00137C10 reads mutable globals LowSpeedYawBoost327060,
  HighSpeedYawBoost327064, LowSpeedSpeed327068, HighSpeedSpeed32706c,
  MaxYawBonus327070 and threshold324dd0. Earlier hardcoded initial values are
  retired; a Memory context is an explicit extra port argument.

Fresh Debug constants3256c8..3256f4 and VIDEO_MODE enum were read and retained in
private captures. MODE_PAL=0 and MODE_NTSC=1. Canonical timing routines use direct
Debug operands and the old six scheduler wrappers forward only. The World class
layer described in P05 remains an audited gap; implementing the low-level clocks
alone does not recover original whole-world orchestration.

Validation so far: individual canonical source objects compiled; the moved mover
fixture and Memory-backed yaw laboratory test passed before adding the final
Debug-specific regressions. Those additions compiled, but a simultaneous build
held the shared archive/executable and prevented their final linked run. Root owns
serialized final validation. New `driving_debug_timing_tests.cpp` covers the actual
Debug clock operands, PAL/NTSC selection, Joylog replay/observation, packed clock
advancement and zero-delta behavior; it is awaiting root target registration/run.
No passing result is claimed for an unexecuted latest test binary.

Original Debug enum `DampingSuitability` was also captured: DS_NO_DAMP=0, DS_DAMP=1, DS_DOUSE=2 (size4). Canonical CheckDampingSuitability returns this recovered enum; only the historical port adapter exposes its underlying integer.


## P08: per-function records and retained fixture coverage

Forty per-function Debug records now live under the Debug SHA/driving directory:
32 implemented entries and8 decompiled entries. World timestep orchestration,
RealDriver conversion, PhysicsCar snapshot and Debug destruction are explicitly
Decompiled, not Implemented. `physicscar-symbols.csv` is a reviewable candidate
manifest; it does not itself mutate the shared symbols registry. Existing E-0042-S001
source ownership references are retained where independently available.

The historical schedule fixture now calls canonical Debug rigid-body and graph
initializers, checking untouched fourth lanes, both uniform curve headers/counts/
ranges/scales, irregular graph points and secondary scalar publication. Its timing
checks use the Debug3256c8 constant block and canonical timer entry points. The
constructor fixture uses Debug component vtables/virtual targets, explicit synthetic
GetSize leaf words, Debug blinking/clock/global operands and roughness samples,
while retaining arena/scratch, list links/counts, axle loading, synchronization,
sibling pointers and component allocation-boundary checks. These are synthetic
instruction contracts, not original car trajectories. Both migrated fixture binaries
await root-serialized execution; this child performed no further builds.

# TASK-0046 player controls audit

Authority: Debug NFSHP2.ELF SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
E-0046-C01 is the live read-only Ghidra query set in controls-manifest.json. Raw
decompilation/listings remain ignored under local/task0046/controls. No Verified claim.

## Confirmed fields, order and behavior

RealDriver is bound to logical PS2 memory, with CarDriver at offset0 and its virtual
table at22c. Steering210 is unsigned16; Throttle214, Brake218, EBrake21c are float;
DesiredGear220/SportGear224 are Gear; pCar230, Port234, StartLineState23c,
StopRightNow240, fForcedEBrakeStopTimer244, bPowerSlide248, ManualTransmission24c,
SteeringInput254, ThrottleInput25c, BrakeInput264, ReverseInput26c, EBrakeInput274,
LastNitroRequest278 and Reversing27c follow the Debug type/instruction operands.

The constructor0013AB98 initializes its empty DriveTarget (00216528/002167C0),
sets the RealDriver table2a6e60, resets joy values, starts StartLineState1 and
DesiredGearAUTOMATIC. Constructor does not initialize SportGear; do not add a guessed
original store. The synthetic zero-filled allocation supplies its initial untouched bytes.
DisableStartLineState0013B168 clears the flag then invokes SetLowGear0013B1D8;
the checked RealDriver virtual target has delta0. SetLowGear chooses FIRST_GEAR2
for manual1 and AUTOMATIC-1 otherwise. Host inheritance shares one ObjectView.

SetManualTransmission0013B198 type1 copies Car.State.CurrentGear(car+3c8), type0
selects automatic, other values select original sport mode2. Shift handlers0013BC90/
0013BD70 only act on nonzero event values. Manual up stops at config+288 plus1;
down stops at NEUTRAL1 and does not select REVERSE0. Both preserve reverse.
Sport mode-2 and SportGear behavior are retained; automatic events reroute to Player
only if one exists. Player reroute is an explicit unsupported path in this fixture.
CarDriver.SetDesiredGear00136C58 is the original inline store used for GUI neutral.

DoThrottleBrakeReverse0013B948 uses BrakeInput, not ReverseInput. ForwardSpeed is
Car+3c0; VelocityMagnitude is Car+3c4. Exact constants324EAC/B0/B4 and named
MaxSpeedForReverse327180, MinSpeedForThrottle327184,
MinSpeedForThrottleWhenReversing327188 remain image reads. Their values are not
replaced with customary driving thresholds. Forced stop suppresses throttle while its
timer/speed condition holds. Reverse pedal doubles then saturates at1. W during
backward motion is routed through the original brake/reverse state. Leaving reverse
calls SetLowGear. Car.State.Throttle publication remains Car+3cc.

DoSteering0013B438 preserves angle conversion, GetYaw, wheel/controller classification,
power-slide hysteresis, countersteering, original tables, return rate and minimum step.
The global constructor0013C750 initializes seven Table instances with their original
data/count/ranges. Named globals327150/170/174/178 and constants324E98/E9C remain
image reads. The final instructions0013B77C/78C/798 multiply by65536, CVT.W.S,
then INTEGER divide by360 before the halfword store; moving division before CVT is wrong.
The previous tool fixed0.5-radian scale is removed from production.

Drive0013B200 calls route diagnostic, nearby debug topology, controller presence,
DoThrottleBrakeReverse, DoSteering, StopRightNow handling, StartLineState handling,
then the optional Player nitro branch. The synthetic fixture has a connected nonwheel
controller, StartLineState disabled, pPlayer null, no route editor and no original replay
capture. These are explicit environment settings, not recovered race startup behavior.
DoSteering and player physics both receive the same nonwheel classification.

MaybeAddTrackRoutePoint001C2ED0 checks pTrackRouteEditor3281d4; null returns and
nonnull throws at missing TrackRouteEditor001C24A8. RealDriverAddNearbyDebugTopology
001FEE80 is an original unconditional return. ReplayManager.RegisterInterestingEvent
0013E070 checks CapturingFlag+4; when enabled it traverses the list+6c, compares node+8
with INSTANT_REPLAY and adds the event value at node+14c. Handbrake calls it only under
the original EBrake>.5, (WorldLoopCounter327d10+1)&7==0, speed>10 conditions.

## Tool interfaces and publication

Input carries continuous pedals/steering/handbrake and discrete transmission/shift/neutral.
The Godot adapter retains queued commands until a completed native step. Session replaces
the complete semantic frame (zero cancels pending commands), records requests with time and
surface and clears commands after success. Losing focus/capture must send zero explicitly.
The backend writes named RealDriver inputs then calls Drive before PhysicsCar.Move; it does
not set throttle/brake/steering outputs or force current gear every step.

PowertrainSnapshot reads completed CarState speed384, ForwardSpeed380, CurrentGear388,
EngineRPM390, driver mode24c and EBrake21c. It does not derive RPM from input. Original
UpdateCar computes RPM from engine angular velocity using its image coefficient.
Speed is published raw; presentation-unit conversion is separate tool formatting policy.
Skid initialization is per session; all mutable pool/list state lives in Memory. Failed
advance restores the arena descriptor and does not commit candidate memory. RenderSkids
also runs on candidate Memory and commits LRU changes only after successful rendering.

## Remaining boundaries

No whole Car/World/Player constructor, race, editor, nitro/radial blur or proprietary PS2
controller implementation is claimed. Original game replay is not Session input recording.
Nonempty DriveTarget route operations remain outside this constructor path. Ordinary host
scalar behavior and the preexisting explicit temporary W projection remain limitations.
Required supported paths have native tests; root records final commands/results and the
separate original runtime comparison attempt in TASK-0046.

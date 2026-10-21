# TASK-0080 - End-to-end driving-control audit

2026-10-21 Australia/Sydney. This is a source/static-evidence review of the
current checkout, with one synthetic host input probe. No gameplay code changed.
Driving authority: Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
User-labelled 2002-07-08 build date is not independently confirmed. Retail
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
remains separate track-asset/topology authority. No original equivalence claim.

## Result and search boundary

The supported single-car driving core is connected. The largest remaining gaps
are original Car frame lifecycle/recovery, complete input/device coverage,
special contacts, damage-dependent handling and full World/Player ownership.
Engine/DriveTrain class dashboard percentages do not measure those gaps.

Reviewed the default InputMap and driving adapter, both viewer and independent
race_scene consumers, GDExtension input/session/publication, NativeBackend,
RealDriver, PhysicsCar and component force chain, track contact/static collision,
wheel skid/particle integration and original Car/World orchestration. Fresh
read-only Ghidra captures cover lifecycle, input and six physics entries; the
machine-readable [evidence index](evidence.json) lists exact queries/hashes.
Existing TASK0044/0046/0048/0075 evidence supplies other component boundaries.
This is not an instruction-by-instruction rerecovery of every class member or
an execution/branch coverage report. Negative source searches are bounded to
the current src/ and godot/ trees.

Fresh program-info/import-file SHA binding confirmed the target and
r5900:LE:32:default language (ignored local/task0080/identity.json). This does
not certify every byte of mutable Ghidra database memory. Twenty newly indexed
functions received Decompiled records; existing symbol statuses were preserved.

## Connected chain

| Stage | Current execution and evidence |
|---|---|
| Input and host gates | project.godot -> driving_input.gd -> driving_lab.gd or race_scene.gd -> Ps2DrivingRuntime::set_driving_input. Normalized pedals, signed steering, handbrake and shift commands; focus/UI/pause clearing and completed-step command consumption. |
| Session and time | Session pacing/replay -> NativeBackend::advance -> PrepareRealTimestep001A0C50, AdvanceRealTime, PrepareWorldTimestep. Candidate state commits atomically; session input replay is port-side. |
| Driver | RealDriver::Drive0013B200 -> steering0013B438 and pedals/brake/reverse0013B948; manual/automatic shift callbacks. E-0080-I01 and E-0046-C01. |
| Frame start | TASK0079 BeginTimestep001BD938 resets crossed wheel terrain once, before physical movement/collision retries. Only that slice of original BeginMovement is connected. |
| Vehicle forces | PhysicsCar::Move00137D18 -> boost/calibration/steering/control -> engine/differential/clutch -> substeps and component derivatives/auxiliaries -> BottomOut/ResolveForces00138D90 -> aero/suspension/five-pass tire-clutch IterateForces00138EC0 -> rigid body forces and integration. E-0080-P01; ordinary leaf bodies are already implemented or inlined. |
| Road/static contact | Four native topology contacts -> actual car collision volume -> wafer/VClip witnesses -> contact-time backup/crowbar/scaled impulse -> remaining movement/recompute. E-0048-D04/D05/T06. |
| Publication and visuals | UpdateCar00139110 -> state, four wheel matrices and skids -> NativeBackend::publish -> bridge pose/velocity/contact/grip/RPM/gear/handbrake. Wheel smoke/spew shares AcidFX; update precedes emission. |

No additional missing ordinary engine torque, differential, clutch, suspension,
tire-force or rigid-body-integration call was established by this review. That
bounded finding does not certify their arithmetic against original execution.

## E-0080-L01 - Missing original Car lifecycle

World::DoTimestep0014D9A8 orders Think/DoWheelForces, Drive, BeginMovement,
DoTimestepMove, DoTimestepCollisions and FinishMovement before event/effect work.
DoTimestepMove0014D588 dispatches active DynamicCollisionBody movement.

The fixture directly invokes RealDriver and PhysicsCar, then static contacts.
It does not run the full Car wrappers. The important differences are:

1. **Recovery requests have no consumer.** BottomOut001378C0 can call
   PleaseResetMeLater001330D8, which writes Car+854. The original
   Car::Move00131B90 consumes ResetMe after mover execution, calls
   ResetToRoutePoint00132FB0 and clears the flag. Current source has the producer
   but no runtime reader of that flag. Thus automatic recovery is not connected;
   the source finding is not a claim that a particular track fall was reproduced.
2. **BeginMovement is partial.** Original00131D78 captures previous position,
   velocity and angular velocity for positive t, sets topology interest and
   conditionally updates LastInTroughPosition before BeginTimestep. The fixture
   only calls the terrain-reset slice; it has no persistent original Car path
   coordinate/interest lifecycle.
3. **FinishMovement is absent.** Original00131E58 publishes acceleration and
   angular acceleration, odometer/distance traveled, updates TrackPathCoordinate,
   checks missing topology/path distance and requests reset, then updates
   headlights/horn and optional Player/replay/camera state. The existing physical
   pose/RPM publication is present; these additional state/consumer stages are not.
4. **Original reset is not session restart.** ResetToRoutePoint00132FB0 ->
   ResetCore00132DF0 -> movement-mode switch, ResetCar state, flat placement,
   wheel/driver initialization, path/controller notifications and optional blink.
   race_scene R/Restart instead calls begin_session and reloads the fixture.
   Implementing recovery requires original route/controller ownership, not just
   calling NativeBackend::reset at the flag site.
5. **Race/multicar wrappers are intentionally missing.** Car::Think00131B10
   computes draft then calls controller Think. CalculateDraftFactor00131A80 scans
   other cars; CalculateNewCatchupMultipliers001351F8 is gated by non-null Player,
   race Boost and two players. These are not missing single-car engine torque.
   Car::ChangeGear00132B28 updates display/AnimatedDriver state; the actual current
   transmission gear is already published by PhysicsCar.

## E-0080-I01 - Input and control gaps

| Finding | Source / original entry | Impact and next dependency |
|---|---|---|
| Device selection | project.godot:26-52 defaults joypad events to device0. Synthetic Godot4.7.2 events on device1/3 yield steering0 while device0 gives correct signs/centering. | Default driving bindings do not cover another enumerated device. Add an explicit host device-selection/binding policy and test both device identities. No physical hardware claim. |
| Unplug ownership | driving_input.gd stores held values; driving chain has no joy_connection_changed listener. NativeBackend joystick service always returns true. | Disconnect-driven clearing/ownership is absent. Actual hardware hot-unplug behavior was not tested; do not claim observed stuck throttle. |
| Steering-wheel classification | NativeBackend::driving_services and PlayerServices both return false for IsJoystickTypeWheel. | Recovered wheel-specific steering tables/branches are unreachable; calibration and device classification are missing. |
| SPORT_SHIFT | RealDriver::SetManualTransmission0013B198 and callbacks support mode2; session.cpp:64 rejects transmission>1; viewer_ui.cpp:140-147 exposes only0/1. | Existing lower-level logic cannot be selected through the public interface. Expose only after a scoped input/UI/test change. |
| Independent race manual controls | race_scene has restart/exit and shift forwarding, but no manual/automatic mode or neutral selector. | Starts automatic. Q/E do not choose manual gear while automatic and Player is null. Viewer UI does expose manual mode. |
| Gamepad action completeness | Default joypad events supply steering/pedals; handbrake/shift bindings are keyboard-only. | A gamepad alone lacks the equivalent action set and rebinding/settings UI. |
| Original byte preprocessing | Backend writes normalized input fields directly. Fresh Steer0013B3B8, Throttle0013B7B0, Brake0013BB48 and EBrake0013BBC8 contain byte/digital priority and channel rules. Throttle includes stiff-throttle scaling. | Current generic max merging/deadzone is host input policy; full original device-to-control equivalence is not recovered. The normalized downstream driver is connected. |
| Reset/horn/siren | Reset0013BE30, HonkHorn0013BF58 not exposed as recovered semantic actions. | Route-point reset permission/ownership is missing; horn/police light-siren chain and audio consumers are missing. |

### Counterevidence to the previous nitro priority

Fresh JoyHandler_NitroBoost0013C028 requires a nonzero press, CarInfo+538==1,
TheRaceParameters.BeACop!=0 and NitroAvailable. It is a specialized police-mode
path, not a generic racer input omission. Drive0013B200's non-null Player remainder
uses NitroActive/WasActive for radial blur. The TASK0079 suggestion to prioritize
generic Player nitro is retained historically but superseded by this audit's
recovery/input/contact priorities. Do not add a generic boost button.

## Contacts and handling beyond the supported fixture

| Priority | Missing chain | Concrete boundary / consequence |
|---|---|---|
| P1 | Special terrain | track_contact.cpp:7-8 and static_collisions.cpp:41 reject PUDDLE4, DEEP_WATER5, MUD_PUDDLE20 and CAR32. Ordinary ground materials work, but these interactions reject a candidate frame. Special providers/consumers must be recovered. E-0048-D01. |
| P1 | Damage-dependent handling | collisionbody.cpp:375-386 supports original static impulse but throws for non-null damage/player/cop/animated consumers (001D5008/001D5188). PhysicsCar reads BlownTireBits+848 and CarDamage+844 for wheel behavior/speed limits, but the fixture has no corresponding damage producer. This is a handling gap, not only appearance. |
| P1/P2 | Post-impact state/events | GetPostCollisionYawControlBoost00135F10 reads event timestamp+10C; full collision event/player/damage consumers are not wired. Complete original post-impact control/feedback is therefore not established even though wall impulses work. |
| P2 | Multi-body/world collision | StaticCollisions and World::DoTimestepCollisions are bounded to one active car and immutable topology polygons. Other cars, traffic, police and arbitrary rendered scenery do not gain dynamic collision ownership. |
| P2 | Breakaway scenery | LoaderSmokeables00219C90 -> SpawnSmokeables0021A4E8 -> VirginSmokeable0021A9F8 -> ConsiderImpulse0021BC48 -> IsMoveable0021C828 remains outside driving integration. Need original anchor/break/body/lifetime chain. E-0048-T07. |
| P2 | Full terrain effects | Production calls the bounded wheel helper; full TriggerCarTerrainEffects00133988 still throws after the wheel slice. Smoke/spew/skid marks work; drive-over/bottom-out/water/flat-tire and other marker/effect consumers remain missing. E-0075-T01/C01. |
| P2 | Audio and haptics | No vehicle engine/tire/collision/horn audio chain found in driving runtime consumers. World DoWheelForces/MaybeRumble and original camera-shake consumers are absent; host chase camera is port-side. These affect driving feedback, not propulsion. |
| P3 | Placement and persistent race state | track_environment.cpp only selects ForwardStartingPosition Param0 and computes heading once; stored environment keeps topology/start/direction. Reverse/grid/race route progress, original streaming/controller ownership, opponents and race rules are not connected. |

## Bounded numerical and infrastructure limits

Native fixture supplies zero for explicit original temporary W-lane projections;
mode3 degenerate/nonfinite VRSQRT001A2AC4 and some conversion domains still throw.
EE zero-denominator division is bounded emulation, not full ISA equivalence.
Original allocation/destruction, ReplaySnapshot/Joylog capture ownership, whole
World/race scheduling and diagnostic restore branches remain separate work.
The current port input tape does not substitute for original replay state.

## Recommended implementation order (port planning, not original behavior)

1. Recover the original route/controller/reset ownership and the necessary
   BeginMovement -> Car::Move -> FinishMovement lifecycle. Test airborne/reset
   requests, out-of-topology recovery and collision retry order without inventing
   a teleport policy. Do not wire the full original wrappers before dependencies exist.
2. Close host control coverage: device ownership/disconnect clearing, wheel
   classification, SPORT_SHIFT, standalone transmission controls and gamepad actions.
   Decide/document normalized host input policy versus byte-accurate original preprocessing.
3. Recover special terrain and non-null collision event/damage consumers, then
   verify their handling changes with original traces.
4. Recover Smokeable and multicar World ownership; add audio/haptics/camera and
   complete effects, then specialized police controls as their dependencies permit.

Matched Debug original-runtime component traces remain a parallel certification
need. Existing PC tests demonstrate regression behavior only. Exact validation
and continuation are in TASK-0080 and docs/PROGRESS_LOG.md.

## Reproducible checks

The six relevant native targets were rebuilt (no work needed) and the matching
CTest subset passed6/6 in27.14s. No new runtime unit tests were added because this
task changes audit records and a diagnostic probe only. No full-suite/GPU/hardware
driving run was needed or claimed. Reproduce the host input finding with:

```powershell
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --headless --path godot --script (Join-Path $PWD 'reverse/task0080/input-device-probe.gd')
```

The probe is synthetic and asset-free. It prints18 cases; default device0 has
positive/negative steering and neutral0, while devices1/3 remain0. The public
script can be rerun after changing bindings rather than treating these results
as a permanent desired behavior. Physical unplug, force feedback and wheel
hardware remain untested.

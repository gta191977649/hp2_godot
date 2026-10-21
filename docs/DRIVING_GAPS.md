# Driving recovery gaps (TASK-0075)

TASK-0081 (2026-10-21) implements the exact Begin/FinishMovement state slices:
old-motion capture before movement, whole-frame acceleration/odometer/distance
after all collision retries, plus the complete CarState::ResetCar leaf used in
fixture initialization. Whole Car entries remain partial/throwing. Deferred
route-reset requests now reject the candidate frame at00132FB0 rather than being
silently ignored; full automatic recovery is still absent. Route assets exist,
but distance-aware route/controller/progress ownership is not yet implemented.
See [implementation and next dependencies](../reverse/task0081/lifecycle.md).

TASK-0080 (2026-10-21) now provides the complete current-chain audit:
[input through frame lifecycle and publication](../reverse/task0080/driving-chain-audit.md).
The first implementation priority is the original route/reset and Car frame
lifecycle: PleaseResetMeLater has a producer but no fixture consumer, and full
FinishMovement/path state is absent. Device ownership, SPORT_SHIFT interface,
special terrain and damage-dependent handling follow. Fresh Debug evidence
restricts the nitro callback to the police/BeACop scenario; TASK0079's generic
nitro-next suggestion is superseded. No gameplay code or verification status
was changed by this audit.

TASK-0079 (2026-10-21) audited CarDriver/CarState/Engine/DriveTrain against the
current controller and restored CarState::BeginTimestep001BD938 once per frame
before Move/collision retries. CrossedWheelTerrain now resets to ROAD, while
current terrain/contact and same-frame maxima remain intact. Engine/DriveTrain
already run the canonical torque/clutch/shift chain; low dashboard counts are
conservative unique evidence bindings, not absent physics. Allocation/destruction,
original replay, sound and non-null Player nitro dependencies remain separate.
See [audit and evidence](../reverse/task0079/driving-chain.md). No original-runtime
equivalence or Verified promotion.

This inventory describes the current source, including TASK-0046 player/skid work
and TASK-0048 real-track contact and static collision integration. Older TASK-0044
exclusions are historical scope, not a description of today's entire runtime.
No subsystem below is Verified against original hardware or PCSX2.

Behavior authority is Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`,
under the recorded TASK-0044 exception. Build date/revision is not independently
confirmed. Retail `SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`,
remains the separate authority for track assets/topology. Numerical address overlap
does not authorize mixing their layouts, constants or algorithms.

## Confirmed source coverage and remaining dependencies

Addresses in this table are Debug unless explicitly identified as Retail. Stable
evidence IDs refer to the linked audit records; this inventory adds no new binary
behavior claims or implementation-status promotions.

| Area | Address / evidence | Implemented and connected slice | Remaining boundary and next concrete step |
|---|---|---|---|
| Controller / player | `RealDriver::Drive 0013B200`, throttle/brake/reverse `0013B948`, steering `0013B438`, shift handlers `0013BC90/0013BD70`; E-0046-C01/P01 | Backend supplies semantic input fields, then calls original RealDriver before PhysicsCar. Automatic/manual, neutral, reverse and handbrake are connected. Original REAL_CONTROL burnout handling is present in Move. | Fixture is playerless, with explicit host joystick services. Non-null Player nitro/radial blur and event rerouting remain addressed throws. Recover those Player dependencies before installing an original Player; do not reintroduce the obsolete raw-output controller bypass. |
| Engine / drivetrain | Engine `00188B38/00188B88/00188C60`; DriveTrain `00189648/00189840/00189AF0/00189DD0/00189E10`; E-0044-E01 | Canonical engine torque/braking, differential, clutch, shifting and derivative chain uses Debug tables, including derivative table `003077C8` and shift storage `+4C/+70`. | Original allocator/destructor and replay snapshot ownership are outside the binding. EE exceptional arithmetic is bounded rather than a full ISA emulator. Capture matched Debug component entry/return states before claiming numerical equivalence. |
| Wheels / tire forces | `001A1948`, `001A1E88`, `001A1F50`, `001A2108`, `001A2500`, `001A2660`, `001A26B8`; E-0044-S01, E-0045-T01..T04/W01 | Contact state, load/slip, brake lock, force guess/lateral correction/finalization and derivative participate in the canonical five-pass solver. Stored Slip and CurrentSlip remain distinct observations. | Degenerate/nonfinite mode-3 basis length at `VRSQRT 001A2AC4` throws. CAR32 road-noise ownership remains excluded. Recover exceptional instruction domains and original CAR contact provider with source-bound state rather than a replacement tire model. |
| Suspension / steering | `Suspension::ComputeAuxiliaries 001A37A0`, force `001A3B10/001A3B50`, penalty impulse `001A3BE8`; `TwoWheelAckermanSteering::SetSteering 001A3540`; E-0044-S01/E01 | Four independent wheel queries, suspension ground-force exchange, penalty impulses and original steering matrices are connected. | General non-track body contacts require their original provider/ownership. Match airborne/contact-transition traces and temporary W-lane inputs; do not replace the recovered calculations with Godot suspension. |
| Road contact / spawn | `Wheel::FindDrivingSurface 001A1D58`, `CheckTerrain 001A1D80`; topology elevation `00151CC0`; `Car::FindFlatOnTrackPosition 00132788`; E-0044-S01, E-0048-D01..D03 | Actual track topology supplies elevation, stored plane normal and physical terrain independently per wheel and skid endpoint. Forward racer0 marker, spline heading and original flat-placement calculation feed startup. | Water/puddle IDs4/5/20 and CAR32 are explicitly rejected. Persistent path coordinate caching, reverse starts, complete race placement and track/world streaming ownership are outside this single-car binding. Start from the recorded marker/path and topology ledgers when expanding scope. |
| Static collision / contact impulse | `World::DoTimestepCollisions 0014D630`, polygon bodies `001D46D8`, witness `001D3230`, contact time `001D1408`, crowbar `001D1698`, Resolve `001D18D8`, impulse `001D1F58`; E-0048-T03..T06/D04/D05 | Actual car collision geometry, topology broad phase, wafer/Voronoi witnesses, backup/crowbar and scaled rigid-body impulse are connected after the first move. Native wall, corner and rollback checks exist. This is not the retired body-center segment guard. | Single active car / immutable track polygons only. Other cars, full active-Collision lifetime/event/audio, damage, animated reaction, player impact and police consumers are excluded; exact null branches are used. Recover non-null consumers and multi-body ownership before enabling them. |
| Scenery / breakaway objects | `LoaderSmokeables 00219C90`, `SpawnSmokeables 0021A4E8`, `VirginSmokeable 0021A9F8`, `ConsiderImpulse 0021BC48`, `IsMoveable 0021C828`; E-0048-T07 | Existing evidence distinguishes initially anchored Smokeable objects from immutable track-polygon collision. Drawing a sign does not supply its collision lifecycle. | Smokeable activation, anchor/break strength, emitters and dynamic lifetime are not connected to driving. Recover this chain as a separate scoped task; never attach guessed generic collision bodies to rendered scenery. |
| Body / aero | `RigidBody::GetDerivative 0019ADF8`, force `0019A6B8`, scaled impulse `0019A920`; aero `001AB3F0/001AB638`; E-0044-S01/E01, E-0048-D04 | Original mass/inertia, force/moment caps and order, damping, aero/downforce and contact impulses feed the same body state. | Damage-dependent callers and general exceptional numeric domains remain bounded. Matched original body/force traces must include original state, tables and contacts. Host trajectory plausibility is insufficient. |
| Integration / scheduling | `PrepareRealTimestep 001A0C50`, `PhysicsCar::Move 00137D18`, Euler `00199BB8`, Resolve `00138D90`, Iterate `00138EC0`, UpdateCar `00139110`; E-0044-P01/P02, E-0048-T06 | Canonical quantized clock, substeps, Euler/auxiliaries, five-pass resolution, publication, collision retry movement and WorldTime advancement are connected. Candidate Memory, collision witnesses and skid arena commit atomically. | Whole World/race/GameFlow, original allocation and replay capture remain incomplete. SpeedDebug/Race4 diagnostic restore is explicitly excluded. Port input replay is not original ReplaySnapshot. Establish a Debug oracle at Move with equal initial state/time/contact and compare per substep. |
| Terrain | Debug table `003102E8`, stride `90`; Retail table `00318C00`; E-0045-T01, E-0048-D01 | All33 original rows are observable. Independent Type/Name/scalar-bit reads establish the explicit cross-version material mapping. Original wheel tables compute grip/noise; render textures do not select physics. | Special water/puddle and CAR behavior is missing even though their numeric IDs and data rows are known. Trace their actual providers/effect consumers before expanding the supported terrain domain. |
| Ground skid marks | `Wheel::MaybeMakeSkids 001A2CF8`; E-0046-S01..S04 | Original Wheel -> SkidMaker -> SkidSet/SkidSegment -> ePoly path is connected after UpdateCar under the original visual eligibility gate. Original textures, compressed points, pool limits and render-dependent fading are retained. | Port arena/clan routing and render sink are bounded bindings; full world clan streaming and GS execution are not recovered. The overload without explicit services intentionally throws; it does not mean the production service overload is absent. |
| Vehicle particles / other effects | `Car::TriggerCarTerrainEffects 00133988`, `Car::TriggerEffect 0011E4E0`, terrain lists `0021CB40`, copy/search `00127380/00127438/001272F0`; E-0075-T01/C01/P01..P03 | TASK-0075 implements the bounded four-wheel smoke/spew loop with original terrain thresholds, contact/distance/slip gates, saturating counters, static wheel emitter coordinates, inherited velocity and negative-intensity TriggerEmitter calls. Dynamic emitter copies share existing particle simulation, RNG, capacities, update and rendering; original library definitions/textures supply particle appearance. Ground skid marks retain their separate trigger chain. | Full TriggerCarTerrainEffects still throws for its drive-over/bottom-out/water/wiper/sound/replay/damage/nitro/flat-tire remainder. Generic non-wheel TriggerEffect and marker-position lists remain outside the bounded service binding. Original World player-distance lookup, full effect ownership and runtime equivalence remain unresolved. Follow TASK-0075 validation records for exact host coverage. |

## Evidence and source reconciliation

- [Debug audit](DEBUG_DRIVING_AUDIT.md) and its linked TASK-0044 engine/solver/physics
  ledgers establish the original component addresses, globals and bounded projections.
- [Player and skid audit](../reverse/task0046/skid-audit.md),
  [controls audit](../reverse/task0046/controls-audit.md) and
  [player physics audit](../reverse/task0046/physics-player-audit.md) supersede the
  earlier assertion that RealDriver and all skid paths are absent.
- [Track integration](../reverse/task0048/driving-integration.md),
  [contact impulse](../reverse/task0048/collision-impulse.md) and
  [static collision orchestration](../reverse/task0048/static-collision-evidence.md)
  supersede the earlier synthetic-plane-only and diagnostic-wall-only scope.
- TASK-0049 topology overlays and TASK-0050 camera projection/scale investigation
  change inspection/presentation, not the underlying wheel or collision model.
  The original outside-camera follow/wall/noise behavior remains separate work.

Current source cross-checks: `NativeBackend::advance` calls RealDriver, Move and
static collision processing; `driving_services` binds the service-taking skid
overload; `TrackContact::GetElevation` supplies actual topology results and rejects
unsupported special materials. Historical Retail whole-entry templates can coexist
with these canonical Debug paths and must not be counted as absent Debug behavior.

## Validation boundary and next work

This document is a source/evidence inventory, not a new numerical validation run.
TASK-0048 records all30 starts,120 settling steps, real materials,15-step replay,
full wall/corner impulses and rejected-step rollback. Existing PC checks establish
regression coverage only. TASK-0038 retains original numerical certification work:
connect the exact Debug executable in a runnable scene, bind Debug entry/return
hooks, capture equal initial component/global/input/time/contact state, then report
per-field error and branch coverage. Retail oracle hooks are not interchangeable.

Priority after TASK-0075's bounded particle work: special terrain/contact consumers;
non-null collision damage/reaction/player dependencies; Smokeable lifecycle;
multi-car/world/race ownership. Each requires its own scoped task and direct original
evidence. This ordering is a port planning choice, not recovered game behavior.

## TASK-0082 vehicle feedback boundary

Original lamp ON/OFF textures, ordinary horn/flash input and twelve-voice engine
sample mixing now have bounded native producers and both host driving consumers.
H is a port-side horn binding; automatic night/brake selectors use original state.
Evidence: reverse/task0082/lights.md, horn.md and engine.md. Full light flares,
reverse/cop flare rendering, stereo delays, replay-specific audio, nitro/backfire,
damage sound and SPU effects remain unresolved.15 traffic configurations reference
an unavailable original bank;49 banks decode successfully. Host load reports
missing bank errors explicitly. PC regressions do not establish PS2 equivalence.

## TASK-0083 collision and drift feedback amendment

The TASK0080 audio gap is superseded for the supported static-wall impact/scrape
and tire skid/squall/burnout branches. Original InGame samples are decoded at
runtime; native impulse accumulation/delay/material/variant rules trigger impacts,
and slip/traction/terrain rules drive ten logical tire slots. Existing laboratory
and race consumers share the transport. Evidence reverse/task0083 documents exact
addresses, constants, host listener/cadence bindings and omitted services. Road
rolling noise, tire damage/impact/thumper/leaves, multi-car/Smokeable sound routing,
spatial azimuth/doppler and full PS2 sample ownership remain unimplemented.

## TASK-0084 camera gap amendment

Test drive's previous render-bounds distance/fixed-pitch chase camera is replaced
by bounded Debug exterior DriveCameraMover pose/history and original topology
constraints. TASK0088 supersedes the earlier V lookback toggle: V cycles original
available driving POVs and their projection; B holds original lookback and release
restores the selected POV. Mouse orbit and idle return are explicit host policy.
The latest user request supersedes the prior Godot-default FOV exception.
This does not restore every original mover, replay/pursuit,
settings/split-screen or camera effect; see reverse/task0084/camera.md for precise
limits and mode evidence. No original-runtime equivalence is established.

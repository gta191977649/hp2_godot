# TASK-0099 - Real race World and participants
## Current checkpoint2026-11-02

Task remains active; complete competition is not achieved. Latest immutablev20
passes real authored HP391source World clock/audio steps (13Cars+Chopper) and
Championship520steps (5Cars), natural RaceStage3→4 and originalRC/result/
resource-baseline close. Audio: HP407commands/312800PCM/nonzero1;
Champ536commands/416000PCM/nonzero1. Original roles, source car IDs, track13/12
and direction remain authored. This is explicit native source composition;
Current0/Desired4/Pending0 is not productionGameFlow6 or originalMainLoop.

Completed bounded scopes: W117 real Smokeable.Move/sleep/retirement; W119 real
Pursuit/pool/Barricade/NearFollow/RoadState snapshot/lifetime/pointer codecs;
W120 actual Chopper request/cancel/trigger leaves; W121 common nonarrest
pullover/player/ticket/boost/tire dependencies; W122 real Player route-reset/
ResetCore with TWO independent optional stackW operands; W123 corrects Belt
wheel radius+0x128 to original+0x12C. Oldv18 fails the actual four-wheel native
argument check, correctedv19 passes; corrected currentv20 long close passes.
No implicit unknown W, roster/flag/unlock override or alternate physics engine.

Native/Godot builds pass through all current core changes. The broad49check
run passed48/49 in135.31s; solefailure was a test assuming anAIracer inHP.
A second fixture incorrectly assumed a CopManager inChampionship. The final
roster-correct2tests pass12.09s: HPfullEndPlayer/ticket6.44s; ChampdirectAI
leaves+Playerreset5.63s. Together48unchangedpasses+2scopedpasses cover50distinct
checks; this is not represented as a single50/50run. Both failed hypotheses
are preserved. Pursuit public test1.25s and Playerreset5.40s pass. WholePursuit
snapshot has564bytes, exact codecs and actual source poolZERO initialization.
No positive wholeAI EndPullover manager transition is inferred from leaf tests.

Remaining reached evidence boundaries: native HP index391/count6→5 and
Championship index520/count10→9 in Resample217454 need exact original EE
arithmetic and conditionally four original prior-stack words. HostRNE's short
tail does not prove original EE reachability. Legacy8Car source clock/audio
still has distinct VU1A81DC operand uncertainty atindex170. Original Sony docs
are authorized for CPU semantics; no available hardware/PCSX2 result exists.
Retain these guards. Fresh whole-Car Belt restore also retains source heap-W
guard; retained-buffer restore is a separate tested domain.

ProductionGameFlow/loading/renderer/MainLoop, complete finish/progression and
original-runtime comparison remain open. Active pursuit/cops/roadblock/spikes,
Chopper Think/Move/Draw/weapons and arrest149028/BeACop descendants retain their
precise original callees where unrecovered. No Verified promotion or commit.
Exact logs, queries, hashes and scopes: reverse/task0099/evidence.json,
current-world-checkpoints.md and subsystem manifests. Next continuation must
preserve source evidence boundaries; never change source loops or fill tails
for a native pass. Reproducible bounded checks:

```powershell
cmake --build build/native-debug --target car_construction_tests pursuit_snapshot_tests cop_frame_tests -j4
ctest --test-dir build/native-debug -R '^(pursuit_snapshot_original|player_car_reset_original|authored_hp_pullover_original|authored_championship_pullover_leaves_original)$' --output-on-failure
build/native-debug/car_construction_tests.exe --authored-event 0 --world-frames 391
build/native-debug/car_construction_tests.exe --authored-event 0 --event-tree 0 --world-frames 520
```

## 2026-10-30 continuation history

Continuation2026-10-31: W98 actual active165-byte Chopper snapshot plus idle
reset/sound/removal tests PASS1.43s; W99 positive pooled AvoidCars103-byte
capture/restore PASS1.13s. Combined with the new authored Championship240step
CTest,3/3PASS10.96s. That actual5Car event completes all240source clock steps,
naturally reaches stage4 and closes original RC/result/resources to baseline.
Its longer1200step probe completes488steps then call489/index488 reaches actual
BeltMover constructor001FF108 (W100). HP snapshot is independently stopped at
controller snapshot target001A03B8, reached through Car00131108 (W101).
No old constructor-only/JustDrive or60-case corpus scope is retroactively widened.
W96 police Think initial states/dispatch remains active. Current commands,
binary/log hashes and distinct scopes: current-world-checkpoints.md.

Latest checkpoint: genuine authored Championship first-event request now passes
5-Car World construction, one source clock frame and original RC/result/resource
cleanup. Genuine HP first-event request creates13Cars plus Chopper and enters
World frame0, currently guarded at police virtual Think dispatch0021EF58.
W94 real enabled-sound13Car+Chopper construction/teardown baseline now passes;
W96 owns the next police dependency. W95 positive DrivingChoice selection and
W94 central unit pass2/2 in2.30s. Normal Godot DLL build passes102actual steps.
The immutable source clock+sound World probe now completes170full steps; call171
(index170) reaches unresolved extended-VU W rounding001A81DC, with input7E000A02,
Q64=402F091540000000/product64=47FF0B8271CD5500. Guard retained, no240frame pass.
See world-choice-frontier.json for exact binary/command/GDB/log hashes.
W93 central Avoid/honk test PASS1.17s. W89 bootstrap,
W90 selection, W92 catchup and ai_drive_target regression pass4/4 in4.54s.
The previous161-step missing action-pool prerequisite has been implemented.
The frame29 extended-VU guard below belongs to the older fixed1/60 World-only
probe, not this source-clock pipeline. It remains unresolved in that scope.
All PC passes are scoped implementation checks, never Verified/full match.

Current scoped acceptance: W96 original police dispatch/initial states; W98
actual Chopper snapshots/removal/deactivation and existing World integration;
W99 positive pooled AvoidCars capture/restore with full baseline. W97 read-only
GameFlow transition audit identifies genuine source callbacks/CarLoader ownership,
not a fabricated state6 handoff. Keep that production dependency open.
Central19related regressions pass35.34s+2.28s;6contracts pass6.28s.
Re-run relevant builds/tests and generated evidence contracts after new scopes.
Reproduce the current authored frontier with:
`build/native-debug/car_construction_tests.exe --authored-event 0`.
Reproduce continuous AI frontier with:
`build/native-debug/car_construction_tests.exe --participants --player --coordinator-owner --clock-frame --sound --world-frames 240`.

TASK-0099 remains active. Latest closure wave14/14PASS33.08s; normal Godot
DLL buildPASS58actual/61scheduled steps. W81 positive replay-event timing and
six real event handlers are now integrated; earlier empty-event W74 corpus is
not promoted to cover that correction. Track54 reset projection now clears all
reviewed mapped fields only behind the source Name-present gate, preserving
unmapped Length50. Evidence maps/records and generated progress are being finalized.

Ghidra MCP was initially unavailable (connection refused, no tool namespace).
The existing hp2_ps2 project/server is now restored at localhost8080 with14tools,
openDebug NFSHP2.ELF and the configured815b60...332f1b SHA verified. No database
annotations/analysis changes were made. W82 raw event status/goal proof, W83
real virgin conversion/wake-up, W84 real profile record initialization and W85
concrete real-clock Joylog dependencies are the current bounded source work.
No full match, hardware comparison or production launch acceptance is claimed.

W85 scope: restore PrepareRealTimestep001A0C50 with direct original Joylog calls,
keeping only primitive monitor I/O in its resource boundary; integrate source
clock/pause/Replay ordering into an explicitly bounded native World-frame probe.
Acceptance: genuine Joylog/bRepeat capture/read+baseline tests, loaded one-frame
World source clock path, source owner/provenance records, relevant builds/tests.
Outer GenerateJoyEvents/render/GameFlow remain separate required dependencies.


## 2026-10-29 current integration and CPU frontier

Task ID: TASK-0099; parent TASK-0096. Implement real World and source-owned
race dependencies in one shared Memory. Full race/production launch acceptance
is still open. Earlier sections below are chronological checkpoints.

Current source-consecutive World execution reaches call29 (index28), source
WorldFrames29/stage3, then throws at001A81DC for a nonexact extended VU W
product. Operands7E000A02 and Q64=402FD28640000000 produce
product64=47FFD50331C71900; discarded bits11C71900 are nonzero. Original Sony
VU manual documents extended finite values, overflow saturation and possible
LSB differences, but does not determine this unique result. The narrow proven
overflow domain is implemented; the nonextreme rounding guard remains. This
supersedes the earlier12/14-frame frontiers, not the immutable firstframe corpus.

The user explicitly permits original Sony EE CPU documents, and on2026-10-29
confirms no original hardware/configured PCSX2 is currently available and asks
to continue other implementation. Read-only local debug/PINE probes timed out.
No original runtime comparison exists. Typed LastNormal.W consumer audit is
recorded separately and does not remove the guard or assert global raw/VU-state
unobservability.

W73 actual countdown alpha/scale and W75 World pause/replay clock pass bounded
source tests. W72 real full64-event frontend/roster test passes3.30s. W74 Replay
codec/Car/AI snapshot subtree and W76 World/body/VC/cop forest are implemented
in explicitly recorded domains. Central7suite first passes5 and fails2:
World fixture lacked an original outer section (fixed fixture only); real
snapshot tree finds RealDriver.Port constructor-1 before MainLoop input
registration (W77 source prerequisite, no field override). Corrected World/AI
component pair passes2/2,total2.40s. W77 core input registration passes1.18s,
including original config tables, raw callbacks, duplicates, queue cancellation
and source pool/allocation baseline. Actual Player/RealDriver registration now enables a11660-byte genuine full
forest capture AND restore at preserved COUNTDOWN3, actual source input NONE/
rebind and complete RC/resource baseline. Central12/12PASS31.17s; normal
Godot ps2_port DLL buildPASS195actual/198scheduled steps. W78 raw frontend
result cleanup passes1.28s plus64event resources/close3.67s. W79 realbase and
three derived smokeable factories/UpdatePosition pass1.17s with fullbaseline.
The immutable30track x2direction capture/restore corpus PASSES60/60,
wall127.927s/sum377.350s. No earlier firstframe corpus is retroactively widened. W80 real request/unlock dependency
remains scoped active work. Keep these source scopes separate.

Acceptance checks: scoped native tests plus actual loaded-resource owner/close,
normal Godot DLL link, evidence/provenance contracts and generated progress.
PC passes establish implementation consistency, never Verified. Next command
to reproduce the current bounded source snapshot checks:

```powershell
cmake --build build/native-debug --target car_construction_tests input_registration_tests world_snapshot_tests ai_snapshot_tests -j 4
ctest --test-dir build/native-debug -R "^(snapshot_tree_original|input_registration_original|world_snapshot|ai_snapshot)$" --output-on-failure
```


## Current complete firstframe checkpoint

Actual World0014D9A8 firstframe now passes6.28s with genuine RC/source Player,
Race/Replay/8Cars, actual loaded event/scenery resources, real frame stages and
source teardown/allocation baseline. Central7tests pass23.63s; see world-frame.md.
Immutable30track x2direction firstframe corpus PASS60/60,407.932s. Source-consecutive
frames initially pass12 and reach actual positive Skid/Clan on13; corrected W71
128-set/40-Clan resource lifecycle now passes1.75s, rolling recheck next. W70 active finish/ghost/speech, W71 actual Skid/Clan,
W72 raw EventTree/Fred/roster producers remain scoped work. Earlier checkpoint
sections below are chronological scope history, superseded where stated here.
TASK-0099 remains active; full race/production launch is not accepted yet.

## 2026-10-29 TASK-0099 ownership and World-frame integration checkpoint

Actual RC001495B0 constructor passes30tracks x2directions:60/60,487.013s.
Historical manual-teardown probe scope remains explicit in race-coordinator-ctor-
corpus.json. Selected-track forward/reverse now execute actual RC001496C8
teardown, PlayerCleanup, queue/state/pools and loaded-resource baseline. These
are fixture DriverInfo identities, not the original Fred roster producer.

W65 scenery/Fern corpus60/60 passes188.77s:146376model allocations,144586bound
solids,8528rigid rows,124656animated paths,2030634particles. All1790NullSolid
references remain source nulls. Empty selected A resources/missing hashes are
unresolved residency/version questions; no alternative-track fallback is added.

Central W66 feedback/W69 Player game-state/memory checks PASS1.52/2.14/0.15s.
W68 initially failed a fixture no-cache expectation; corrected fixture passes
1.39s, production unchanged. W67 DAT wrap/ties, traffic gates/timer/visibility
and stable intrusive fractional-X sorting component checks PASS1.48s. See
local/task0099/world-frame/integration-ctest.log and subsystem notes.

World0014D9A8 source orchestration and tiny source-empty services are implemented
and syntax checked. Whole-frame linking/acceptance awaits W70 Race.Update and
actual event/scenery-loaded caller integration. W70 discovered original AI IDs
100..106; older0,2..8fixture corpus is preserved with its historical scope.
No full World/competition or Verified claim. TASK-0099 and TASK-0096 stay active.


## Current shared participant checkpoint, 2026-10-29

TASK-0099 remains active. Actual shared World, source-owned resources,64 Car
constructors and pooled Physics/Mellow movement are implemented in scoped domains.
Eight simultaneous participants now execute original prestart AI Think/Drive,
BeginMovement, shared dynamic-body movement, actual World collision scheduling
and FinishMovement with actual source grid/Race/Replay ownership and complete
CleanupCars->Race->World allocation baseline. The first actual8 collision check
passes7.38s; the30-track forward/reverse first-frame collision corpus passes60/60
in438.658s (see world-collision-corpus.json for exact sum and per-case hashes).
Full Player/RC initialization and full World.DoTimestep are not yet complete.

E-0099-W54 uses the user's explicit Sony EE CPU-document authorization to handle
SQRT.S(-2^-22) at001D4998 as sqrt(abs(x))=2^-11 with documented invalid flags.
The exact negative-root domain is supported; nonexact negative rounding stays
explicit. No geometry clamp, altered source instruction order or source flags
were introduced. See ee-sqrt.md and docs/DECISIONS.md.

Other passed bounded checks:30 authored track AcidFX corpus4108 sources/6568
particles, explicit unknown disc-W operand;3 raw UI bundles86screens/2784objects/
1601bitmap coordinate pairs; actual DriveCamera145frame POV/update test; actual
30-track480camera-plane crossings; real enabled sound/software voices/exact IOP
command queue and explicit PC PCM consumption. Positive damage/pursuit/feedback
and AI action/PID paths have scoped tests. PC success never establishes Verified.

Current parallel dependencies are W55 actual Player ownership (using W51 NIS and
W52 raw Flailer), W56 original RC.InitCars/Race initialization, W57 remaining
source World service callees. W47 active AI action reevaluation/Belt/boost,
original Player frame/RC startup and replay snapshot/World frame scheduling still
have reached guards. Production launch remains gated by these real dependencies.
No per-car NativeBackend, ParticleSimulation or Godot-physics substitute is used.

Validation commands/logs and exact limits are in the subsystem notes, particularly
shared-collisions.md, raw-acidfx.md, flailer-resources.md, ai-frame.md and
camera-startup.md. Preserve original source provenance and explicit unknown
operands while binding the genuine complete frame; do not close TASK-0099 or the
parent competition task on constructor/first-frame checks alone.

## Player-connected movement checkpoint (W55/W58/W59/W60)

Actual Player constructor/destructor and InitForRace use real3Flailer projects,
InGame RadarBeep, NIS/Average/Topology owners and eight authored NisInfo records.
Player/camera central regression2/2 PASS4.09s. New actual8 --player --collision-stage
passes forward track11, including genuine bidirectional Player/Car binding,
shared movement/collision, ordinary Player.FinishMovement and periodic replay
consumer ordering. Source RealDriver destructor now performs actual view
RadialBlurOff instead of clearing Player ownership to avoid the dependency.
The snapshot record in the periodic consumer assertion is explicitly a test
operand; original Replay.BeginFrame production remains required. No full
RealController frame/RC constructor/World.DoTimestep acceptance follows.
See player-construction.md, player-frame.md and car-feedback.md.

## World construction checkpoint, 2026-10-29

E-0099-W05..W18 now implement actual Debug World0014CA28 construction and
pre-simulation0014CC80 destruction with concrete original route/path/signpost,
texture/emitter, VirginSmokeable/scenery, police-roadblock/radar and speech
resources in one shared Memory. Thirty actual track bundles construct and close,
restoring the pre-World loaded-resource allocation baseline. Reverse scenery
rotation001502E0 is recovered with independent Retail field/consumer review.
Final regression/build results are recorded in reverse/task0099/world-construction.md.

Completed bounded criteria: original pool/list semantics; calibrated path distance;
actual route post-load/HeapSort/remap; reviewed12-to8 Signpost projection;
real graph vertices/edges/RB/ST lists; actual constructor resource ownership;
Virgin/static body source records/registration/order; loaded police geometry;
real25-state/92-transition speech startup and eight-draw voice shuffle; ordered
construction-state teardown with explicit active-path gaps. Native checks do not
establish original equivalence or Verified. Evidence/index/function/structure
records retain executable identities and query hashes.

TASK-0099 remains active: real RaceCoordinator00149800/InitCars0014A128,
live player/AI/traffic/cop creation and mode dependencies, full shared World
scheduling and live-object teardown are still required. Production launch must
not enter a partial substitute simulation. Subsequent rules/outcome/results/
rewards/save acceptance remains open in TASK-0096. Continue directly with the
original InitCars/Car dependency chain, using this real World and its Memory.
No new prerequisites or user approval gate have been introduced.

## Full Car construction checkpoint and mode integration

W32 actual00130B90 and construction-state00130FF8 passed all64 source car types
in both directions within the actual World:49damage/15traffic. W25 render corpus
and W26 shared cache are integrated, with real character/geometry/events. The
explicit test domain is original SoundEnabled0; requested sound remains W31's
actual factory/device dependency. No production sound flag is forced.

W33 pooled PhysicsCar lifetime, W34 actual60grids/480markers, and reverse64Car
passed3/3 in32.22s. Original clock bindings and unknown local vector W remain
explicit port operands. W36 mode replacement now integrates original control
0/1/2/3 and movement0/1/2; expanded actual64Car tests are pending. Required AI
Mellow4 is being recovered in W35; original default00326FA0=4 is preserved.

Immediate next action: complete W35 ctor dependencies, run expanded modes, then
actual Race/RacingCar/AddRacingCar publication and original World scheduler.
See reverse/task0099/car-construction.md and race-car.md for bounded status.

## Current participant dependency continuation, 2026-10-29

After the constructor checkpoint, continue within TASK-0099 rather than stop at
an initialized empty world. W19 restores positive collision IDs/witness lifecycle;
W20 audits exact scheduler0014D9A8; W21 restores base/null controls and original
InitPhysics pools; W22 restores CarState/CarDamage and source damage/markers;
W23 maps actual collision geometry and CarCollisionBody; W24 restores owned
player/AI control constructors. W25 solid/model/render construction and W26
shared topology/path-coordinate resource/cache ownership are current blockers to
actual Car00130B90. W27 investigates required sound factories/sample ownership.
Every scope retains exact evidence; no scalar prefix is a participant-ready Car.

Current native checks passed: car_control, collision_body_lifecycle,
car_state_damage (3/3,3.68s), control_construction(1/1,1.14s). Real damage corpus
covers44geometry files/956markers/115bodyinfo/387sector mappings and64CarState
inputs. W23 first geometry corpus test failed because an actual traffic file has
no authored CV; absence must take the original fallback after a complete lookup,
not be asserted impossible. Correct this acceptance expectation and rerun.

## Current implementation scope, 2026-10-29

User explicitly prioritizes real World and correctly recovered dependencies.
TASK-0099 is the sole implementation task for this pass, ahead of remaining
TASK-0098 presentation work. Recover and implement the required route/path,
TrackGraph, original slot pools and actual World constructor/resource sequence.
Reuse the authorized Debug identity; no single-car substitute or Godot physics.

Checks: fresh read-only decompiles/listings/types; native edge/signpost/list/pool
and original-data route regressions; integration into real shared Memory; normal
native/Godot builds; durable dependency/evidence/status records. Any unresolved
required initializer still throws at its original call boundary. A constructor
prefix or fixture-service test does not establish a fully initialized real World.

Historical pre-implementation audit2026-10-29: E-0099-W04 retains21 fresh read-only construction,
participant and graph/type captures in world-audit-evidence.json. Summary and
exact next implementation chain: reverse/task0099/world-construction-audit.md;
Debug structure record: reverse/structures/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/world/trackgraph.md.
This is investigation alongside active TASK-0098 production popup work, not new
World implementation or acceptance. Real construction/scheduling remains missing.

- Status: active
- Parent: TASK-0096; see parent for exact authority, constraints and acceptance.
- Dependencies: TASK-0098 native launch boundary; production frontend binding remains open.
- Scope: Original constructors/loading/time/route/reset/traffic/opponents/multi-body collision; shared-world memory/compact publication; no single-car fixture race.
- Acceptance: scope behavior/production routes/tests/evidence complete; original comparisons
  stay open until recorded. Detailed address-bound criteria added before activation.
- Current bounded implementation: first-event shared World participant registration/table and original constructors.

## Activation2026-10-28

User explicitly renewed first legitimate event full race roundtrip as the immediate milestone. TASK-0098 native launch handoff now has original-data tests; full presentation/profile integration is still open. This scheduling revision permits native World work before full frontend acceptance. Do not claim TASK-0098 complete or substitute its service fixtures for production resources.

Scope and checks: recover original World construction/participants before stepping races; first restore0014D028 BuildCarTable using the same shared logical Memory and original Car/DriverInfo links, stable category order and CarIndex stores. Continue real World/RaceCoordinator/Car initialization from E-0096-C01/ignored first-world captures. No per-car NativeBackend worlds, fixed-car scene, synthetic finish state or Godot vehicle-physics replacement.

Acceptance for current slice: original address/types/listing retained; category/order/bounds tests including real first-event driver records, shared storage and malformed-list rejection; native/Godot build, related regressions, evidence/tasks/progress updated. Full task acceptance remains real World/simulation/participants; list registration alone cannot complete it.


## Shared World and performance checkpoint2026-10-28

E-0099-W01 implements original shared-Memory BuildCarTable/GetCarByNumber/ResetTimeScale; E-0099-W02 implements DriveTarget constructor/scoped Clear and TrafficTeleporter route totals/timing. E-0099-W03 records required World ctor effect/pool/police/TrackGraph dependencies without pretending to implement them. No real World/Car simulation is running yet. The existing single-car backend remains a fixture.

E-0099-M01 adds read-only Memory footprint and a standalone Release clone/destroy baseline. Three independent120-warmup/600-sample runs of the2310172-byte imported image gave P99 0.8929/0.9468/0.9045ms; this excludes live world/simulation/rendering and heap counts/GPU timers. See reverse/task0099/memory-baseline.md. No full-frame60FPS or dirty-page replacement claim.

Final relevant build/12 regressions passed12.73s before source-review order corrections/read-only footprint extension; final validation is recorded below. Normal Godot DLL also built at that point. Source review corrected selected-ride lookup after character selection and per-generation location reads; native order probes passed after correction. Evidence/records42 scoped Debug entries and structure/version details are in task0098/first-event-launch.md and task0099/shared-world.md.

Exact next action remains original World/RaceCoordinator construction, real routes/graph/resources and Car/controller/mover initialization and ordered scheduling. The constructor dependency audit lists actual required pools; only explicitly proven original no-op functions may be empty. Production frontend/profile/record/GameFlow service providers from TASK-0098 remain unbound, and real finish/results/rewards/save plus original/performance acceptance are not complete. Continue autonomously without a confirmation gate.

Final validation2026-10-28:

- Final12-target native build command in reverse/task0098/first-event-launch.md passed after live-read ordering and read-only footprint changes.
- Final12-test command there passed12/12 in12.98s.
- Final original car identity/usage assertions: `cmake --build --preset native-debug --target race_parameters_tests --parallel 4` passed; `ctest --preset native-debug -R '^race_parameters$' --output-on-failure` passed1/1 in1.80s. All64 Debug enum/asset identities were independently audited, with no mismatch.
- Final `cmake --build --preset godot-debug --target ps2_port --parallel 4` passed including latest source order/DriveTarget/Memory changes. No window/gameplay acceptance is implied.
- `cmake -S . -B build/native-memory-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DPORT_BUILD_GODOT=OFF` and `cmake --build build/native-memory-release --target port_memory_copy_benchmark --parallel 4` passed. Three separate120/600 benchmark processes exited0; exact commands/results/source+binary hashes in memory-baseline.md/json.
- Generated progress/contracts and final whitespace/evidence checks are recorded after final metadata refresh. No commit/staging or Verified promotion.

The intended first race roundtrip remains unfinished: there is no actual complete World/controller/police/race/results/save session yet. The next implementation remains TASK-0099 original resources/routes/graph/World/Car construction; continue that chain, not further isolated frontend presentation work.

- Final `ctest --preset native-debug -R '^(progress_tables|recovery_progress_contracts)$' --output-on-failure`:2/2 passed2.20s after final metadata refresh.
- `git diff --check`:passed; existing line-ending notices only. All831 TASK-0098 and43 TASK-0099 indexed capture hashes matched files. Restricted-material scan of untracked files passed; staged files0. No commit made.


## W54 CPU evidence authorization and exact SQRT checkpoint,2026-10-29

The user explicitly approved original Sony EE manuals as supplemental CPU ISA
authority. The exception is limited to CPU instruction semantics/flags/formats;
AGENT R0 for all game behavior and recovered names/layouts remains binding.
Mirror-served original PDFs require publisher/version/page/SHA identity; no
community/emulator account is authority and no raw manual enters Git. The
accepted decision is recorded in docs/DECISIONS.md; technical evidence and the
scoped function map are reverse/task0099/ee-sqrt* (E-0099-W54).

Implemented: port SQRT.S normal/signed-zero carrier, exact negative magnitude
results and I/D/SI state, integrated only at original001D4998. The observed native
B4800000 operand yields exact3A000000 result. Original geometry and dot ordering
are unchanged. Nonexact negative rounding/unsupported encodings remain explicit
boundaries; CPU-wide bit equivalence/Verified is not claimed. Standalone primitive
probe PASS and foundation syntax PASS; actual8 collision-stage central run is
pending. Exact next action: run that stage, record its next genuine boundary and
extend only with evidence. Full competition closure is still an active criterion.


W54 central update: actual8 --participants --collision-stage invocation PASS7.38s
(root reported), preserved first log and hash in ee-sqrt-evidence.json. This
supersedes the pending collision-stage statement above, without widening exact
negative SQRT support or claiming original/complete race equivalence. The new
Monitor fixture failure was a missed expected delta clamp: authored easy factor
100 and scalar~0.005 make impact10 saturate1. Production source was correct;
fixture now separately checks medium nonsaturated accumulation and easy clamp.
Its corrected central foundation rerun remains pending.


## Independent dependency scopes2026-10-31 (same active TASK-0099)

W119: original active pursuit snapshot/constructor/destructor/pointer dependency
closure behind tCopManager; first recover exact source ownership and bounds,
then genuine source objects/positive codec/lifetime checks where supportable.
W120: original Chopper FollowThisCar001E44B8 chase child and strictly required
leaves; fresh read-only closure assessment precedes production edits.
W121: original EndPullover002200F0 reached by MonitorPullovers timeout, source
Car/Player/control restoration and valid object lifetime. Exact entry address
is a lead until fresh query confirms the target and original source owner.

These separate scopes preserve World/math guards and add no synthetic event,
pursuit, timer, callback, roster/state or lane defaults. Source leaf closure
may remain incomplete with precise guards. Acceptance requires provenance,
practical positive/negative tests and source cleanup; never Verified from PC
checks. Continue the pending source World dependencies while exact EE/prior
stack evidence is unavailable. No request for unavailable hardware is repeated.


W122 scope is recorded before source edits in player-car-reset.md: original
Car.ResetToRoutePoint00132FB0/ResetCore00132DF0 plus distance-biased path reset,
real-controller/replay direct leaves. Two optional raw stack W operands are
independent; tests must state them explicitly and cover missing operands. Full
source Car/controller/mover/topology lifetimes and cleanup remain required.
W120 central55-step buildPASS;4/4 tests24.43s. Original monitor transport is
reused, with natural platform0 no-op and exact unbound platform1 guard. Active
Chopper Think/Move/Draw/weapons remain separate source dependencies.


W123 scoped counterevidence correction (before edit): W122 ResetCore review
exposed Belt.Move current wheel radius read config+0x128. Existing original
MIPS200268/200284/200290 instead use config+0x12C, stride0x20. Root owns
beltmover.cpp only. Fresh live instruction/byte validation, exact offset fix
and a genuine authored Belt-to-Wheel.SetState argument check are required;
keep older native pins historical, never assert their trajectory as corrected.


## Related TASK-0110 outcome milestone, 2026-11-06

The new smallest-roundtrip task implements independent native finish consumers
(E-0110-F01/F02/F03): remaining finishers, final record publication, event progress
and retry, Fred options/scores/points/transient, task/point rewards. These are not
a production World launch or complete competition. The521-frame authored
Championship probe still reaches Resample00217454's prior-stack guard; no World
frontier or verification criterion is advanced. See TASK-0110 for current exact
commands, source scope and remaining production lifecycle work.

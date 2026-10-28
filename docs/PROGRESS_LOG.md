## 2026-10-28 TASK-0098 real-window failing acceptance and reflection binding

User explicitly requested Python/project-script testing, not computer-use. Actual
Godot4.7.2 Vulkan/Forward+ on Intel Arc B390 reproduces the reported _input ->
_send_joy_event -> _report stack at Retail001F6A28 from Main Menu/Ultimate Racer.
New strict acceptance command `py -3 reverse/scripts/check_frontend_selection_roundtrip.py`
FAILS with exit1; latest run-00ulygx6 has no Select Car checkpoint. No UI roundtrip
or parent criterion is marked complete. Logs/screenshots are ignored and the
runner isolates profile/save inputs/outputs; it uses actual Input.parse_input_event.

Fixed the capture-only false success: a reported runtime error still permits a
diagnostic screenshot but now emits FRONTEND_CAPTURE_FAIL/exit1. Positive actual
Main Menu capture exits0, negative Event trace exits1. Capturing is not acceptance.

E-0098-RB01 fresh readonly MCP confirms intrinsic reflection0x11 and Debug sorting
field4D. Host frontend pose now suppresses sorted strips' colour/depth and detaches
envmap next-passes, respecting subsequent capture/texture updates and restoring
main-car eligibility. New actual GPU material/occlusion tests pass, along with
existing modulation/environment tests. One initial test-material reset omission
was corrected in the fixture, not counted as a pass. This is a bounded prerequisite,
not the missing shared physical colour/depth executor or production page binding.

Native11-target build and11 selected CTests passed23.91s. Normal Godot DLL rebuild
passed (previous in-use lock absent). Exact GPU command, known failed invocation,
evidence and limitations: reverse/task0098/reflection-binding.md. No commit,
staging, proprietary exports or Verified promotion. Continue TASK-0098 with
E-0098-F03/F04 actual shared-target executor and real SelectionPage services;
keep the strict production route test red until it actually works.

## 2026-10-28 TASK-0098 approved full-closure plan and event-entry prerequisites

The user approved complete frontend (including Jukebox) plus all32 Ultimate Racer
events and real race/results/save roundtrips. Scheduling returns to production
Event -> selection -> cancel/reentry before TASK-0099. All original-comparison,
resource and performance criteria remain binding; no task is complete.

Read-only Debug MCP recovers StartEvent001F2BD8/HandleMessages001F2D20, including
mode/store order, live repeated multitrack status read, Port0-only continuation
and SELECT-only input. Native orchestration and actual-asset first-event tests
pass with explicitly labelled required host fixtures; production entry still
throws001F6A28. The63-registration runtime now includes the25 already evidenced
missing operands and exact class/document resolution. Asset-backed audit yields
76 authored documents and90 route/unknown rows, including27 unmapped documents;
no page or caller completeness is inferred. Details: reverse/task0098/production-entry.md.

Native builds passed,16 scoped CTests passed39.55s; Godot ps2_port target was
up-to-date. Initial header include-order failure was fixed by isolating the Debug
entry declaration/service header, not counted as a successful validation.
CMake3.29.2/GCC13.2.0/Python3.14.4 and godot-cpp pin unchanged. No PCSX2/listening
21512/28011 endpoint observed; original certification remains blocked by that
prerequisite while independent implementation remains possible.

HEAD changed externally cf037a6 -> c949d1c during work; it includes registry and
scope changes already made in this session. No commit/staging was performed by
this agent, and the new HEAD was preserved. Next: TASK-0098 E-0098-F03/F04 actual
shared physical colour/depth executor and real page services, then production
Debug entry/profile adapter and window Event/Select Car/cancel/reentry acceptance.
Do not substitute the new entry's host fixtures or resume World first.


Final checkpoint validation:
- Fixed existing unqualified port:: references in Timer declarations to ::port::;
  including the new entry header after FrontEndRuntime now compiles. No timer
  behavior changed; ultimate_event deliberately compiles this include order.
- Final native rebuild of the16-test target set passed; final scoped CTest
  command above passed16/16 in29.15s after this declaration fix.
- Final `cmake --build --preset godot-debug --target ps2_port --parallel 4`:
  compilation passed; normal link BLOCKED because godot/bin/ps2_port.dll is in
  use (Permission denied). The running user process was not stopped.
- Independent link using the freshly compiled objects/libraries and Ninja's exact
  generated linker argument list passed after changing only DLL/import-library
  output paths to ignored local/task0098/entry-build. Reproduce after releasing
  the DLL with the normal build command, or use the exact linker arguments saved
  in local/task0098/entry-build/manifest.json. This is link validation, not window
  acceptance; the running user's DLL was not replaced.
- `py -3 reverse/scripts/audit_frontend_closure.py --check`: passed63/76/90.
- `ctest --preset native-debug -R '^(progress_tables|recovery_progress_contracts)$' --output-on-failure`: passed2/2 in1.83s before final log refresh; final refresh/check recorded below.
- `git diff --check`: passed (existing CRLF notices only). Raw captures/build
  products remain ignored. No original comparison/window or Verified claim.

## 2026-10-28 - TASK-0098 native launch to TASK-0099 shared World

User renewed first legitimate event full roundtrip as the immediate milestone, retaining full career as final scope. Workspace started clean at cf037a6. Dependency scheduling now advances to TASK-0099 after bounded native launch handoff; TASK-0098 formal frontend/compositor/profile/flow providers remain incomplete and no task is marked completed.

Native Debug defaults/DriverInfo/RNG/character/traffic/cop and first EventTree/RaceEvent/StartRace behavior now consumes actual original car41/track13/character/reward data, builds13 driver records and requests original GameFlow unload/load. Snapshot precedes startup time reset. Required text/profile/records/flow test services are labelled fixtures.30 native startup reentries retain13 records and temporary scratch baseline; this is not30 full race retries. Shared World registration uses original Car/DriverInfo pointers, stable categories and signed identity in one Memory; no per-car world clone. DriveTarget/TrafficTeleporter constructor leaves and explicit unknown branches added. Actual World constructor/Car/AI/police/frame scheduling and outcome/save are still missing.

Source/listing review caught and corrected signed-class decompiler counterevidence, bSetRandomSeed warm-up, live selected-mode/location read order, and first event's actual one-car reward. Test-only compile/arithmetic/allocator-guard assumptions and an incidental stale-executable run after failed compilation are documented rather than counted as passes. Final twelve regressions passed12.98s; final identity/usage extension passed1/1 in1.80s. Final build/results are recorded in TASK-0099. Toolchain/pins unchanged; normal Godot DLL built.

Port-only Release image clone/destroy baseline:2310172 bytes,2 regions,120warm+600samples in three independent processes; P99 .8929/.9468/.9045ms, max2.7976/1.0535/1.9030ms. It is not a full NativeBackend/live-world/GPU/FPS result. Heap allocations unmeasured. Read-only footprint and zero growth assertions added; dirty-page rollback/view hazards remain open. See reverse/task0099/memory-baseline.md.

Read-only MCP worked; no PCSX2 process or21512/28011 listener found2026-10-28. No original comparison, Verified, playable race, full retry/resource or1080p60 acceptance. Exact continuation: TASK-0099 original World dependency construction and real cars/controllers/shared simulation; actual frontend/profile/GameFlow integration and real finish/results/rewards/save still required. No staging/commit/proprietary exports. Final progress contracts2/2 passed2.20s, whitespace/restricted-material checks passed, all831/43 indexed capture hashes matched; staged files0.

## 2026-10-27 - TASK-0098 page ownership and compositor preparation

User restored Ghidra MCP; continued readonly captured Debug evidence under the
renewed five-deliverable request. Added native single-player SelectionPage binding
with required resource/cancellation leases, copied nodes,22 native input callbacks,
loading/Tick/locked/alpha state,20 shared nebula registrations and ordered explicit
close. Normal close preserves duplicate preload and whole FrameEffects group kill.
Port abnormal-construction/disposal rollback is separate. Host fixture tests cover
delayed loads, pending switch exit, second pad, after-display effects,30 reentries
and injected construction failure; this is not production upload cancellation QA.

E-0098-O01..O04 adds independent VisibleCarSet copying, grouped integer-position
restoration, exact topbar/SetString and native preload/unload. E-0098-F04 adds Debug
byte-radix/shared-alpha order, Flailer depth domains and per-quad source layer.
602 registrations verify original600 initial pool is not an arbitrary hard cap;
teardown retains unrelated registrations and trims reusable host storage.
13 bounded evidence records and698 ignored capture hashes indexed. Constructor/
destructor/Render2D whole entries remain Decompiled/partial. No Verified promotion.

Scoped13/13 passed26.06s; normal Godot DLL build passed. Final real-asset topbar
checks passed1/1 in2.34s. Exact commands, source-review and progress contracts are
recorded in TASK-0098. Final page failure-gate1/1 passed1.19s, final Godot rebuild
passed, progress contracts2/2 passed1.91s, full diff whitespace check passed. All698
capture hashes and13 new scoped unique address keys/record paths passed. An initial
whole-catalog uniqueness assumption was corrected against seven unchanged Retail
alias addresses in HEAD. Toolchain/pins unchanged. No commit/staging/restricted files.

Remaining work: actual shared physical colour/depth GPU executor, original native
help/speech/rectangle services, production cancellation leases and formal event/
StartRace. Full World, police/rules/results/progression/save, Debug original-runtime
comparison and1080p performance acceptance are incomplete. Continue TASK-0098 from
page-ownership.md/composition-preparation.md;0099..0103 remain backlog.

## 2026-10-26 - TASK-0098 mutable frontend effects and dependency audit

Continued active TASK-0098 under the renewed request to finish all five TASK-0096
deliverables in one delivery. The end-to-end request remains unfulfilled. Native
E-0098-P01..P04 adds finite general inverse/Hermite math, mutable frontend bases,
original nebula transforms/shared colour matrices, zero-speed VRSQRT carrier and
shared1000/400 particle/group budget/RNG transactions and teardown. Shared systems
reject private-budget stepping and unbound dynamic triggers. One candidate is
used per shared step; ordinary driving/GPU-emission paths retain their defaults.
Definitions/unknown extra Z/W/global dynamic-trigger ownership boundaries remain
explicit. See reverse/task0098/frontend-effects.md for original sources, ISA-only
reference distinction, failed/corrected test assumptions and exact commands.

Validation: native scoped11/11 passed23.41s with original assets/Debug ELF; after
the final shared-accounting guards, effects/GPU-emission/driving3/3 passed6.56s.
Particle boundary source-review1/1 passed8.56s. Normal Godot DLL build passed
before final guards; final rebuild and generated-progress contracts recorded in
active TASK-0098. CMake3.29.2/GCC13.2.0/Python3.14.4; godot-cpp
e4b7c25e721ce3435a029087e3917a30aa73f06b unchanged. No original runtime, window,
performance or full retry/resource acceptance is asserted. Read-only process/
listener inspection found no PCSX2 or21512/28011 listening endpoint.

E-0098-F03 corrects prior compositor prose: reflection0x11 suppresses envmap and
sorted-alpha strips; stages2/4 flush solids only, stage5 flushes their shared
pending alpha domain. Existing stage1/7 metadata does not implement that domain.
An overbroad Release-disabled loader inference was rejected against the shared
base-class source. See compositor-correction.md. Independent read-only E-0096-C01
police/real-finish/result/autosave dependencies are durable in
reverse/task0097/lifecycle-continuation.md; no downstream implementation was
silently substituted or marked complete.

Exact continuation: TASK-0098 complete selector ownership/input/popup/help/speech/
after-render effects and corrected stage0/1/2/4/5/7 compositor/profile binding,
then formal event entry and actual launch. Only then activate TASK-0099 for shared
World/participants, followed by police, genuine finish/rewards/progression/results/
private transactional save. No staging/commit, synthetic outcome or Verified claim.

## 2026-10-26 - TASK-0098 selector loading, locked display and packet stages

Continued TASK-0098 under the renewed full TASK-0096 request. E-0098-T01 recovers
native selector Tick load/fade/scroll/orbit/camera/pose/light order and the bounded
static camera-offset stores. Delayed and superseded loads are tested, including
the original duplicate reinit at full overlay. Fresh00193AE0 listing is an original
no-op; no replacement spotlight effect is invented. E-0098-L02 implements locked
slide/timer/signed alpha and ordered task/BAC/event/points unlock descriptions;
localization and frame-owned rectangle rendering remain required services.
E-0098-F02 preserves background/interface stage ranges through widget emission
and production document concatenation; exact flat quad order remains unchanged.
Coverage validation exposes legacy unclassified event-tree packets. No shared-depth
compositor, formal selection entry or mirrored-car pass is implied complete.

Validation: native build of eight scoped targets passed; eight CTests passed25.34s
with actual configured assets/Debug ELF. Final normal Godot ps2_port DLL rebuild
passed. CMake3.29.2/GCC13.2.0/Python3.14.4 and pinned godot-cpp
e4b7c25e721ce3435a029087e3917a30aa73f06b unchanged. Original event audit passed64
events/32 Hot Pursuit; five KO storage incompatibilities remain isolated. Scoped
diff check passed. Required test-file ignore exceptions were added explicitly;
no proprietary file staged, and no staging/commit requested or performed.

Detailed evidence/commands/limits are in reverse/task0098/loading-lifecycle.md,
locked-display.md, render-stages.md and the active task. Original comparison is
blocked by no running matching PCSX2 scenario and timed-out21512/28011 probes;
Retail oracle is invalid for Debug. Full production selection, constructor/
destructor/FrameEffects/render composition, World/police/rules/results/private
save and resource/performance acceptance remain incomplete. No Verified promotion.

Exact continuation: TASK-0098 constructor/destructor and FrameEffects ownership,
reviewed stage0/1/2/4/7 shared-depth compositor, then actual native service/profile
adapter binding and event-to-selection-to-valid-launch route. Keep TASK-0099 in
backlog until those prerequisites exist. Do not substitute the single-car tool
fixture or fabricated outcome. Run scoped commands in active TASK-0098, regenerate
progress with `py -3 reverse/scripts/generate_progress.py`, then contract checks.

## 2026-10-26 - TASK-0098 selection input and display continuation

Continued original selector orbit/digital/analog/help state and typed page callback, including Param/Noise, bounded navigation, current-car skin reload and explicit dependency dispatch. Constructor loading overlay corrected to original130. Native alpha caches signed per-corner values and restores duplicates head-first per fresh00244080 evidence. Final native selection/camera/widgets3/3 passed3.60s; model/driving-camera regressions earlier3/3 passed14.94s; normal Godot DLL rebuilt. Vulkan original prop fixture passed2.51s with stale generation discarded. Earlier test-only field/arithmetic assertions corrected; no guessed production constants.

Evidence and commands in reverse/task0098/display-dependencies.md. Fresh frame order shows main solids before Flailer24..20, platform/particles, reflection+mainalpha, then three overlay layer ranges; single-pass viewer composition would be wrong. Locked text task/event/points priority captured but not implemented. Full Tick/loader/locked rendering/FrameEffects/compositing and formal production event route remain open, followed by World/police/outcomes/performance. No Verified or playable career claim. Continue TASK-0098; no staging/commit; user files preserved.

## 2026-10-26 - TASK-0098 original selection native core

Continued only0098 under parent0096. Implemented separate hp2::debug decoded
car/status/ride/list projections and original scalar bodies in original source
ownership. Both players' event restrictions, signed skin rules, bounded nonwrapping
car navigation, wrapping skins, stable original adjacent sorting, current-node
identity, per-game ride slots and selected-car settings are covered. Selector
confirm/popup/cancel/readiness-before-advance implemented as a scoped projection;
full constructor, sounds/Noise, outer Back/orbit/held input, model/presentation and
production launch remain explicit gaps. No viewer substitute or synthetic race.

Fresh read-only Ghidra MCP audited original Sort00249958 and Debug24-byte versus
Retail28-byte CarLanguageInfo. All50 identities/five common IDs match after explicit
field conversion. Original English assets resolve64 full-name bindings and the
non-COP acceleration/BHP accesses. Initial broad spec assertions failed on43004/
44004; original001953B0 does numeric top speed and hides COP specs, so final checks
follow actual calls. Missing text was not manufactured.27 original entry records
updated (26 scoped core Tested, partial callback Decompiled),176 capture hashes
indexed in reverse/task0098/evidence.json. Original raw data remains ignored.

Native build targets and8-test command are recorded verbatim in TASK-0098:
8/8 passed42.11s. Final neighbor-index/status/reentry selection1/1 passed1.08s.
Normal `cmake --build --preset godot-debug --target ps2_port --parallel 4` passed
again after final source edits. Progress tables/contracts2/2 passed2.48s; asset
inventory --check passed. Checkpoint diff whitespace check passed. No FPS/graphical/lifetime/PCSX2 equivalence claim.
CMake3.29.2/GCC13.2.0/Python3.14.4/Godot4.7.2 and existing godot-cpp pin unchanged.

Next continue active TASK-0098 native camera/Quaternion, original model loader,
reflection/lights/animation/help/speech and generation-safe host resource ownership;
then bind actual event entry. SetCameraMatrix consumes world->view; inverse-view
pose is required. Original runtime oracle still absent. Later0099..0103 World/
police/race/outcome/performance acceptance remains incomplete. No commit/staging;
all prior user changes preserved. This checkpoint is not a playable career.

##2026-10-26 - TASK-0096/0097 Ultimate Racer implementation start

Recorded user's expanded Debug authority for full single-player Ultimate Racer;
AGENT.md/DECISIONS and parent0096 preserve separate Retail scope and existing
uncommitted work. Established ordered0097..0103 subtasks. REA independent
read-only workers investigated selection, launch/World and police; main writer
implemented only current inventory/data foundation. Both ELF hashes match, MCP
is available.130 original Debug function entries/228 private capture hashes are
indexed as static evidence, not whole-entry recovery claims.

Added independent hp2::debug EventInfo fields/getters/state-size and EventTrack
SetRace in original eventtree.cpp/header ownership. All64 original records decode;
Hot Pursuit has32 nodes with types0:8,1:2,2:4,3:5,4:4,5:4,6:5. Debug KO progress12
versus Retail16 makes raw save rebind invalid for5 Hot Pursuit events. All table
references/packed track settings/bounds/original flag18 tested. No fake races or
outcomes. `cmake --build --preset native-debug --target ultimate_event_tests --parallel 4`
passed; `ctest --preset native-debug -R '^(ultimate_event|eventtree)$' --output-on-failure`
2/2 passed1.36s. `py -3 reverse/scripts/audit_ultimate_racer.py` and`--check` pass.
No FPS or original-runtime equivalence claim. Original oracle unavailable in checked
local tools/config/processes. Continue active TASK-0098 selection restrictions and
state machine, then original visual loader/camera/effects and race handoff. Related
commits:none. Raw binaries/assets/captures remain ignored.

##2026-10-26 - TASK-0092 corrective BGM lifecycle audit

User reports BGM in states that should be silent. Fresh read-only Ghidra MCP
rechecked Retail SHA7335b8ed...eafacc2 and Title/Jukebox start/stop/completion
call sites (E-0092-J08). Confirmed port bug: newly added per-frame music polling
implicitly called scheduler.enter, bypassing original explicit001AEAB0 timing.
Removed implicit selection. Separate explicit binding retains original startup/
main-menu requests; inactive/stale completion cannot resurrect a stopped track.
Original manual second-movie skip has no start call; natural last-movie end and
main-menu construction do. Do not generalize all static Title states as silent.
Jukebox original strict30-second inactive preview retry remains unchanged.

Native selected targets and Godot-debug ps2_port build passed.
`ctest --preset native-debug -R '^(frontend_jukebox|frontend_runtime|jukebox_screen|jukebox|movieplayer)$' --output-on-failure`:
5/5 passed35.88s. Added silence regressions and corrected the former test that
wrongly expected direct-Jukebox polling to start music. Godot4.7.2 headless
`--path godot --script D:/dev/hp2_godot/reverse/scripts/check_frontend_music_gate.gd`
passed binding gate; windowed command with `-- --host-audio --screen=22 --scale=1`
passed actual AudioStreamPlayer Title-silence/explicit-start check. Both exit0.
Raw captures ignored local/task0092-audit; metadata/index and audit are durable.
No computer-use/Ghidra mutations/Verified promotion. Full original comparison
remains open. TASK-0095 Help implementation deferred for this user-reported defect.

##2026-10-25 - TASK-0091 final validation checkpoint (work incomplete)

`cmake --build --preset native-debug --parallel 4`: passed all145 pending build
steps. `cmake --build --preset godot-debug --target ps2_port --parallel 4`: passed.
`ctest --preset native-debug -R '^(frontend_.*|jukebox|jukebox_screen|frame_builder|fmenus_menu|eventtree|screen_manager|progress_tables)$' --output-on-failure`:
19/19 passed48.17s, including all current frontend integration, input, viewport,
quad, frame and generated-progress checks. Godot4.7.2 headless command
`--path godot -- --native-render-smoke`: passed41 documents,1064 quads,
7700 glyphs,120 textures; authored empty document36. This proves frame production,
not complete behavior of41 screens. Headless editor import also passed.

Window NameEntry capture command:
`--path godot -- --screen=24 --scale=1 --menu-events=4,4,4,4,9,wait:1,9,wait:0.8 --capture=D:/dev/hp2_godot/local/task0091/name-entry-phases-fixed.png`.
Exited0 and inspected complete keyboard. Raw window captures remain ignored.
No computer-use, no Ghidra mutations, no staging/commit. Diff check passed.

Continuation: TASK-0095 shared Help dependency001E3B88/001E3DD8/001E4028,
then create a scoped controller/profile/credits task and proceed through remaining
TASK-0091 race setup/car/track/launch and endgame/pause/replay. Input/menu adapters
are incomplete for original repeat, calibration and physical devices. Retain
untranslated exceptions and open criteria. Original PCSX2 comparison prerequisites
are unavailable in checked PATH/process/local-tools/config locations; no Verified
promotion. Existing captures in local/task0091/{name,race,endgame}-agent provide
read-only evidence leads for subsequent implementation, never completed status.

##2026-10-25 - TASK-0095 keyboard render closure

Read-only MCP established original final layer buckets:24..20,15..0,19..16,
28..25, plus reverse-authored ties. CPU widget traversal alone was insufficient;
retired the earlier universal numeric-descending inference. Frame builder now
preserves these submission phases. Vulkan window capture visibly shows complete
NameEntry keyboard. Native keyboard/frame tests2/2 pass2.12s; final-phase fixture
added. General frontend regressions passed earlier except an old frame test's
hardcoded first-quad assumption (corrected to identify font texture); generated
progress check passed after regeneration. Shared Help and original comparison
remain open, as do the rest of TASK-0091. PCSX2 was not found on PATH, among
running processes, in configured tools or local config; no original comparison
was attempted or claimed. All raw captures remain ignored; no Git staging.

##2026-10-25 - TASK-0095 NameEntry checkpoint

Implemented portable Retail3x14 keyboard, overwrite/delete/case/cursor controls,
matching player gates, ungated Back commit, gender and whole-screen snapshot
comparison, profile writes and conditional Autosave. Original scalar animation
preserves velocity on retarget and snaps wrapping grid movement. Authored slots,
keyboard class and caret are bound. Native test1/1 passed1.74s; expanded P2 and
120Hz animation trace added subsequently. Godot extension builds and headless
editor import passes. Added explicit host two-pad adapter; physical devices not
tested. Reproducible Options route window capture reveals equal-layer occlusion:
keyboard glyphs are behind their panel. This remains an incomplete visual criterion,
not a passed comparison. Investigate original widget-list construction before
changing tie ordering (TASK-0039 Q07 previously identified this unknown).
Autosave slot formatting capture now displays original localized slot1 correctly.
Ten native frontend regressions passed10/10,38.67s; no original comparison.

##2026-10-25 - TASK-0091 / TASK-0092..0094 production integration checkpoint

Read-only Ghidra MCP only; computer-use excluded by user. Integrated Jukebox
real metadata and five-row widgets, strict preview timer, mode/genre toggles,
preferred-track Back resume, natural completion modes and private Autosave.
Recovered successful Autosave timing and authored FRONTB35 popup wrapping; PS2
card error/hardware paths remain explicit gaps. Audio Options now owns seven
normalized sliders, raw gain settings, derivative-preserving scalar animation,
output mode, defaults and menu feedback. Dedicated host audio bus supports live
Mono downmix; stereo carrier is used for the stored Dolby mode without claiming
SPU encoding. Window Audio capture leak resolved by draining mix before capture
exit. Display/HUD/calibration implemented with separate player snapshots and
asymmetric original calibration limits. Host offset translation is a recorded
port policy. Retail frontend bSin polynomial replaces incorrect standard-sine
projection; exceptional conversions and original numeric equivalence stay open.

Native4/4 integration tests passed38.97s; commands and remaining checks in
TASK-0094. Prior Jukebox/Audio window captures and isolated save roundtrip passed;
formatted Autosave/Display/HUD captures pending current Godot build. No original
PCSX2 comparisons, no Verified promotions. No proprietary files staged.

# Progress log â€” session narrative, newest first

## 2026-10-25 TASK-0091/0092 - Retail Jukebox state recovery

TASK-0091 records the complete-frontend request, including post-race/pause/replay
interfaces and an explicit non-racing launch boundary. The38-row registration
census is only a seed; complete screen/state coverage is unresolved. TASK-0092
is the current bounded implementation scope. The user excludes computer-use;
subsequent binary investigation uses read-only Ghidra MCP only.

E-0092-J01..J03 recover Jukebox grouping, five-row scroll decisions, category
and mode navigation, genre toggles, preview operands and strict timing gates.
Portable overloads in original options.cpp use explicit host state/services;
whole-entry throwing templates remain unchanged. A reproducible capture checks
three MCP memory ranges against the configured SHA-bound Retail ELF. The known
001F0D10 undefined-entry error is recorded, not repaired in the database.

Native targeted build and8/8 regressions passed, including configured-original
JBC and existing frontend runtime checks. The Godot-preset portable test target
built and passed1/1; this is not extension/windowed validation. CMake3.29.2,
GCC13.2.0, Python3.14.4. Exact commands/results are in TASK-0092. No original
comparison, Verified promotion, staging or commit. Proprietary evidence stays ignored.

Next: TASK-0092 production JBC metadata and original widget binding, then preview
transport, Back filtering/resume and profile/Autosave. Read FRONTEND_COMPLETION.md
and frontend/task0092-jukebox.md; Jukebox and the overall frontend remain incomplete.

## 2026-10-21 TASK-0081 - Recovered frame state and ResetCar leaf

Implemented exact Begin/FinishMovement state slices in original car.cpp with
explicit port-owned entry points. NativeBackend captures old motion once before
movement and computes acceleration/odometer/distance once after collision retries,
inside the existing candidate transaction. Full Car entries still throw for
topology/path/Player remainder. Complete ResetCar001BD6C0 now initializes fixture
state while preserving original position/distance/control/W fields and store order.

Deferred reset no longer disappears: after every PhysicsCar Move, a pending
ResetMe rejects the candidate at original ResetToRoutePoint00132FB0, preserving
the committed pose/clock/request. Full route recovery remains unimplemented.
E-0081-R01 establishes route data exists in Track11 (16 exactly framed routes),
but distance-aware point selection needs persistent wrapped track progress and
TrackedController ownership. No nearest-XY/spawn/host restart replacement.
E-0081-L01 exact listings/type evidence and E-0081-R01 dependency captures are
indexed in reverse/task0081/evidence.json. New route records are Decompiled only;
ResetCar is Implemented and full Begin/FinishMovement remain partial. No Verified.

Validation: native two directly affected tests passed2/2 initially13.50s and after
final store-order matching16.81s. Broader driver/backend/engine/drivetrain/surface/
mover/contact/track regression8/8 passed40.75s before that store-order adjustment;
the directly affected pair was then rebuilt/rerun. Final Godot ps2_port build
passed. Strict successor scope6/6 tests and historical79 scope5/5 passed. Exact
commands, tool pins, final closure checks and limitations are recorded in TASK0081.
No full suite/graphical/original-runtime comparison, no commit/proprietary additions
or Ghidra mutations. Existing workspace changes preserved.

TASK0081 closure: class-members/migration/single-chain/preservation4/4 passed,
27.63s. Generation/--check,35-capture/14-record metadata checks and whitespace
review passed. No frozen predecessor review or unrelated body changed.

Next action: recover route/signpost field bridge and loading, persistent
TrackPathCoordinate progress/length-wrap and TrackedController point ownership,
then connect original ResetCore/ResetToRoutePoint. This bounded first stage is
complete; the unsupported route guard must remain until real recovery exists.

## 2026-10-21 TASK-0080 - End-to-end driving chain review

Reviewed host input through driver, physics, contacts and publication, plus
original Car/World frame orchestration. E-0080-L01/I01/P01 preserve36 fresh
read-only Debug decompilation captures with SHA-bound import provenance and
public response-hash index reverse/task0080/evidence.json. Twenty new function
records are Decompiled only. No runtime code or existing status changed.

Most immediate gap is deferred reset: BottomOut/PleaseResetMeLater produces
Car+854, original Car::Move00131B90 consumes it through route-point recovery,
but current fixture does not. Full Begin/FinishMovement path, acceleration,
distance and out-of-topology recovery are missing. Existing ordinary engine,
clutch, suspension, tire, rigid body and static impulse chains are connected.

Input findings include device0-only default joypad mapping (synthetic confirmed),
missing disconnect ownership/wheel classification, SPORT_SHIFT2 hidden behind
validation restricted to0/1, standalone transmission selection and gamepad action
coverage. Original byte preprocessing differs from explicit normalized host input.
Fresh0013C028 police/BeACop gates supersede TASK0079 generic nitro-next priority.
Special terrain, damage-dependent handling/post-impact events, Smokeable/multicar,
full effects/audio/haptics and race ownership remain separately scoped gaps.
See reverse/task0080/driving-chain-audit.md for concrete source/evidence and order.

Validation: six native targets rebuilt (no work), matching CTest subset6/6 passed
in27.14s. Asset-free pinned Godot4.7.2 probe printed18 cases: device0 signed axes
work, device1/3 produce zero. Public reproducible script:
reverse/task0080/input-device-probe.gd. No actual unplug/hardware or original
numerical trace, no graphical/full-suite run. New unit tests omitted because
production code is unchanged. Closure source-protection/progress checks and
exact commands are recorded in TASK0080. No commit, proprietary additions or
Ghidra database edits. Existing workspace edits retained.

Closure: existing_body_preservation/progress_tables2/2 passed,11.12s;
generation/--check,36-capture/20-record metadata validation and git diff --check
passed. New records remain Decompiled; no original verification promotion.

Next action: scoped original route/controller/reset lifecycle recovery, using
Car::Move00131B90 -> ResetToRoutePoint00132FB0 -> ResetCore00132DF0 and necessary
Begin/FinishMovement state; do not replace that behavior with session restart.

## 2026-10-21 TASK-0079 - Driving chain state lifecycle

Scoped audit of Debug CarDriver, CarState, Engine and DriveTrain against the
current native controller. Existing RealDriver/PhysicsCar torque, clutch,
steering and shifting are connected; conservative dashboard address parsing
undercredits grouped methods, overloads and inline setters. No blanket status
promotion. Original allocation/replay/sound and non-null Player nitro are not
added to the playerless fixture. See reverse/task0079/driving-chain.md.

E-0079-S01 read-only Ghidra confirms CarState::BeginTimestep001BD938 resets
CrossedWheelTerrain[3..0] to ROAD1, called from Car::BeginMovement00131D78 at
00131E50. Implemented the full four-store function and called it once before
Move/collision retries, preserving candidate-frame rollback. Other BeginMovement
work remains outside the binding. No database writes or proprietary files added.

Validation: cmake --build --preset native-debug --target driving_player_tests
driving_backend_tests driving_engine_tests driving_drivetrain_tests
driving_surface_tests driving_mover_tests -j 6 passed. ctest --preset native-debug
-R '^(driving_player|driving_backend|driving_engine|driving_drivetrain|driving_surfaces|driving_mover)$'
--output-on-failure: 6/6 passed,26.21s, including original-data backend and surface
checks. New tests cover write extent, intra-frame maxima, snow-to-road reset and
rejected-frame rollback. Python test_debug_driving_chain.py:6 passed;
test_recovery_progress.py:29 passed. Godot extension build and final generated
checks are recorded in the task. Existing native_backend.hpp:25 misleading
indentation warning preserved. CMake3.29.2, Ninja1.12.0, GCC13.2.0,
Python3.14.4; godot-cpp pin e4b7c25e721ce3435a029087e3917a30aa73f06b matches.

Godot ps2_port build passed. Initial final contracts were4/5: the old TASK0077
rendering-only freeze correctly rejected this task's two intentional function
changes. A strict TASK0079 successor now pins only BeginTimestep and advance,
retains immutable predecessor reviews and rejects other identity/stub/auxiliary
changes. existing_body_preservation rerun:1/1 passed,7.87s. New TASK0079 scope
tests5/5 and existing TASK0077 scope tests5/5 passed. complete_class_members,
debug_driving_migration_rules, debug_driving_single_chain and progress_tables
passed in the initial run. Generated progress/check and Git whitespace review
passed. CarState now has7/18 uniquely evidenced Debug bodies (38.9%).

No matched original Debug trace, graphical smoke or full-suite rerun; native
checks do not establish PS2 equivalence. No commit created; pre-existing workspace
changes preserved. Next: read TASK-0079 and DRIVING_GAPS.md, then recover original
Player nitro dependencies in a new scoped task before enabling that input.


## 2026-10-21 TASK-0078 - Concise collapsed progress dashboard

User requested only a large overall count/percentage, collapsed CLASS coverage
and collapsed Functions grouped by class; every list starts closed. The generator
now owns the whole page, preserves the old page in PROGRESS_ARCHIVE_TASK0078.md,
and retains source/evidence distinctions in the JSON. The headline counts only
evidenced Retail bodies over its full inventory; class rows retain bound-method
denominators and unknown totals. Shared entries deduplicate globally. Unowned
functions remain Unassigned / global, with no inferred classes or invented names.

The TASK0041 name export omitted unnamed Debug entries; E-0078-I01 derives a
durable address-only 10069-entry inventory from TASK0073's already retained,
complete SHA-bound catalog. No Ghidra mutation or new behavior assertion. Source
statuses, game logic and original verification remain unchanged. P0 TASK0077
rendering work remains incomplete after interrupted graphical controls; no further
Godot runs are scheduled as part of this progress-layout request.

Validation and final counts are recorded in TASK-0078. The save watcher and build
refresh continue using the same entry points; the generated headline is inside
the dashboard markers to avoid self-triggered save loops. Session prose stays in
this log, rather than returning to the dashboard.

Final validation: 29 tooling tests and2 focused CTests pass; wrapper/generated
checks pass. Structural inspection confirms one headline and318 balanced details
groups, all default-collapsed. Actual watcher testing with offline MCP mocked
records exactly two refreshes (startup plus a debounced task save), with no output
feedback. An initial timer-based fixture missed its intended edit; retry starts
the edit after the initial snapshot and passes. The watcher exited; no background
service was left running. Counts:419 class rows,17886 unique entries, headline
30/7544 evidenced Retail bodies (0.4%). No native rebuild/graphics needed and no
Verified promotion. Reproduce with tools/track_progress.ps1 -Once or -Check.

## 2026-10-20 TASK-0073 - Automatic recovery progress

Added deterministic SHA/address-keyed class/member accounting, generated JSON and
Markdown, a save watcher and offline build refresh. Existing user changes were
preserved. Shells, throwing templates, directly partial bodies and evidence-backed
claims are separate; method totals remain unknown and statuses are never promoted.
All repository text is English. No game logic, Ghidra annotations or assets changed.

Read-only live MCP enumerated 7544 Retail and 10069 Debug entries, with no Debug
bound-address misses and 254 Retail misses. Those misses are catalog/entry questions,
not permission to discard records or infer whole-entry boundaries. Large responses
use stored-output chunks. Current program info omits SHA-256, so configured import
path plus original-file SHA binds provenance, explicitly without database-memory
certification. Private catalogs/status stay in ignored local/progress.

Usage and limits: docs/AUTO_PROGRESS.md. Exact final validation and continuation
are recorded in TASK-0073. No original verification or commit is claimed.

Final validation: 22 Python behavior tests and both targeted CTest checks pass;
native-debug build completed all178 steps and exercised automatic refresh. The
owned watcher smoke observed the real log save and updated its generated contents,
then was stopped. No background process is left running. Scoped diff checks pass.

## 2026-10-19 TASK-0071 - Vulkan migration and build-lock correction

Preserved existing uncommitted work and the original DLL before changing the
Windows default to Vulkan/Forward+. Added numeric-GS HDR viewport transport,
CarViewer presentation, strict backend startup, global profile method metadata,
selected-Track11 benchmark tooling and strict host-source successor checks.
Bounded actual Vulkan vehicle/fusion/particle/Sun/sky/postprocess/lifecycle and
ELISE reflection checks pass. Dense/nearby native packets agree across backends;
pixels differ and HDR per-primitive COLCLAMP remains an explicit gap.

The first graphical fixed-start series reports median-of-repeat wall32.737ms
Compatibility versus24.277ms Vulkan. The paired gain8.460ms does not exceed
baseline repeat spread8.518ms, so the conservative performance gate rejects it.
An earlier minimized console run was excluded. Dense/route measurements are not
final acceptance evidence: the user rebuild interrupted the ongoing matrix.
No Verified or complete-performance claim is made.

The user reported a build failure while the task-owned benchmark retained the
workspace DLL. Reproduced ld Permission denied at linking, stopped only the owned
matrix/benchmark, and reran `./build.ps1`: exit0. Bounded Vulkan frontend launch
with --gpu-profile --profiling --profile-debug exited0 and exported120 frames.
Subsequent checks use the isolated candidate project. The user's newly launched
normal profile process is preserved; do not stop it or run competing FPS tests.
TASK0071 remains active for the full criteria in its task file.

## 2026-10-19 TASK-0070 - Global live ImGui profiling

User clarified realtime in-game drawing, a global profile_debug.gd name, function
hotspots and ImGui presentation. Delivered the persistent global collector and
shared native ImGui panel with240-frame history, CPU/GPU/counter/function modes,
pause/frame inspection, clear and120-frame export. Headless/global frontend
capture works without a loaded track. Script entrypoints, native driving tick
and existing renderer section timings are explicitly inclusive wall scopes.

Actual ImGui input checks caught/fixed frontend viewer-input gating. Windowed
buttons, function rows, headless console/diagnostic tests and real Running game
scope capture pass. Native race runtime integration with profiling also passes
focus/restart/failure/exit/re-entry. Normal root Debug DLL rebuilt after user window
released its DLL lock; staging was used during the lock, no user process killed.
TASK0070 exact source successor retires the old script path and carries inherited
pins. Native logic/shaders/RNG unchanged; no full native sampling/FPS claim.

## 2026-10-19 TASK-0069 - Godot4.7.2 and profiler-driven diagnosis

User explicitly requested direct runtime upgrade without an old-engine baseline,
plus the active rendering API in the title. Official archive checksum/version
checked; all current build/test/launch paths select4.7.2. Retain godot-cpp ABI
pin; GPU particle-layout/Sun/postprocess/lifecycle5/5, headless controls/mesh/UI
diagnostics, actual OpenGL title and14 custom monitors pass. Normal Debug build
and editor import pass. New exact source successor carries inherited auxiliary
pins and TASK0068 port caches; old frozen review JSON stays unchanged.

Short current-pose warm profiler capture:2880x1695 colour/depth GPU medians
4.38/1.78ms; Sun synchronous readback wall5.78ms despite targetGPU0.019ms.
4096->16byte reduction does not materially reduce the wait and stays off.
History timestamp anomalies remain unresolved; do not add viewport times or
claim60FPS. Native/image cache fallback parity passes at the pose and nearby;
30-track static census covers60 props-on/off records. Track24 known colour debt
is not waived. Results and reproduction: docs/PERFORMANCE_TASK0069.md.
TASK0068's broader unimplemented GPU experiments remain active/deferred explicitly.
Bounded normal frontend boot also selects4.7.2/OpenGL and exits0; forced early
shutdown retains one AudioStreamWAV and AudioStreamPlaybackWAV. Preserve that
cleanup warning as a limitation; it is not evidence of a render-object leak or
normal user-close behavior.

## 2026-10-19 TASK-0061 - Live coordinates and host UI DPI

Completed camera HP2/Godot header, logical UI/physical framebuffer-input contract,
revisioned fonts and effective Windows per-window DPI. UiDisplay scales host text
in Debug/Release with original font baselines and weak lifecycle ownership. No
original glyph/world/render-resolution changes. Native scale2.0, Track11 forced
100/150/200/100 windows PASS; small-window header containment fixed. Console/viewer
regressions and Release text/native race lifecycle PASS. Debug3/3,Release2/2 tests
and9before/14after strict host-only review PASS. Physical cross-monitor/non-Windows
checks remain unavailable, no Verified/FPS claim. Delivery local/task0061/delivery,
all8DLLs/flavor and hashes, workspace resource junctions. Build output isolated from
the user's running root frontend; no process terminated or changes committed.

## 2026-10-19 TASK-0060 - Track host memory residency

Completed scoped Track11 ownership correction: lazy singleton/sky GPU surfaces,
exact decoded-attribute merging without intermediate GPU upload/readback, bounded
inactive draws with coordinated depth/uniform eviction, lazy debug overlays and
release of consumed raw bundle. On-demand memory categories distinguish payloads
from actual process bytes. Native culling/material/physics bodies unchanged;
strict successor13before/20after PASS. Debug8/8 and Release7/7 targeted checks,
GPU transport/ownership, actual viewer motion and Release race lifecycle PASS.
Debug/Release delivery peaks1909.84/1896.73MiB vs4237.52/4225.77MiB predecessor;
24native hashes and scoped1080p pixels match per flavor. Ownership clears/cancels
to zero; residual process bytes remain unattributed without allocation tracing.
PDH paging is system-wide and total process faults include soft faults. No PS2
Verified or FPS claim. Delivery local/task0060/delivery with8DLLs per flavor,
manifests and workspace resource junctions. No commit/staging. User's coordinate/
DPI follow-up starts separate TASK-0061; user-owned root frontend remains running.

This is the running narrative of how the port got here: what each session looked at, what it
found, what it got wrong and corrected, and the exact commands it ran. It is append-mostly and
deliberately long.

**For current state, use [PROGRESS.md](PROGRESS.md) instead.** That file is the tracker --
status tables, blockers and the next action -- and is the one the startup protocol in
`AGENTS.md` means. This log is where that tracker's claims came from.

Nothing here is deleted when it turns out to be wrong. A retired claim is marked as retracted in
place and kept, because the fact that a session believed it, and why, is itself evidence
(AGENT.md R6). The original header of this material was: `Project state â€” 2026-10-13 (Australia/Sydney)`.

## Contents

<!-- generated:contents:begin -->
<!-- Generated by reverse/scripts/generate_progress.py on 2026-10-28. Do not edit by hand. -->

- [2026-10-28 TASK-0098 real-window failing acceptance and reflection binding](#2026-10-28-task-0098-real-window-failing-acceptance-and-reflection-binding)
- [2026-10-28 TASK-0098 approved full-closure plan and event-entry prerequisites](#2026-10-28-task-0098-approved-full-closure-plan-and-event-entry-prerequisites)
- [2026-10-28 - TASK-0098 native launch to TASK-0099 shared World](#2026-10-28---task-0098-native-launch-to-task-0099-shared-world)
- [2026-10-27 - TASK-0098 page ownership and compositor preparation](#2026-10-27---task-0098-page-ownership-and-compositor-preparation)
- [2026-10-26 - TASK-0098 mutable frontend effects and dependency audit](#2026-10-26---task-0098-mutable-frontend-effects-and-dependency-audit)
- [2026-10-26 - TASK-0098 selector loading, locked display and packet stages](#2026-10-26---task-0098-selector-loading-locked-display-and-packet-stages)
- [2026-10-26 - TASK-0098 selection input and display continuation](#2026-10-26---task-0098-selection-input-and-display-continuation)
- [2026-10-26 - TASK-0098 original selection native core](#2026-10-26---task-0098-original-selection-native-core)
- [2026-10-25 TASK-0091/0092 - Retail Jukebox state recovery](#2026-10-25-task-00910092---retail-jukebox-state-recovery)
- [2026-10-21 TASK-0081 - Recovered frame state and ResetCar leaf](#2026-10-21-task-0081---recovered-frame-state-and-resetcar-leaf)
- [2026-10-21 TASK-0080 - End-to-end driving chain review](#2026-10-21-task-0080---end-to-end-driving-chain-review)
- [2026-10-21 TASK-0079 - Driving chain state lifecycle](#2026-10-21-task-0079---driving-chain-state-lifecycle)
- [2026-10-21 TASK-0078 - Concise collapsed progress dashboard](#2026-10-21-task-0078---concise-collapsed-progress-dashboard)
- [2026-10-20 TASK-0073 - Automatic recovery progress](#2026-10-20-task-0073---automatic-recovery-progress)
- [2026-10-19 TASK-0071 - Vulkan migration and build-lock correction](#2026-10-19-task-0071---vulkan-migration-and-build-lock-correction)
- [2026-10-19 TASK-0070 - Global live ImGui profiling](#2026-10-19-task-0070---global-live-imgui-profiling)
- [2026-10-19 TASK-0069 - Godot4.7.2 and profiler-driven diagnosis](#2026-10-19-task-0069---godot472-and-profiler-driven-diagnosis)
- [2026-10-19 TASK-0061 - Live coordinates and host UI DPI](#2026-10-19-task-0061---live-coordinates-and-host-ui-dpi)
- [2026-10-19 TASK-0060 - Track host memory residency](#2026-10-19-task-0060---track-host-memory-residency)
- [TASK-0032 â€” Debug CarViewer and TrackViewer scaffolds (2026-10-14)](#task-0032--debug-carviewer-and-trackviewer-scaffolds-2026-10-14)
- [TASK-0031 â€” F8 debug console and independent Debug FPS (2026-10-14)](#task-0031--f8-debug-console-and-independent-debug-fps-2026-10-14)
- [TASK-0030 â€” Concise README completion (2026-10-13)](#task-0030--concise-readme-completion-2026-10-13)
- [TASK-0029 â€” Root-level classes and Ghidra-confirmed directories (2026-10-13)](#task-0029--root-level-classes-and-ghidra-confirmed-directories-2026-10-13)
- [TASK-0028 â€” Editable C++ class/member templates (2026-10-13)](#task-0028--editable-c-classmember-templates-2026-10-13)
- [TASK-0027 â€” Class names, and the one mechanism that binds them (2026-10-13)](#task-0027--class-names-and-the-one-mechanism-that-binds-them-2026-10-13)
- [TASK-0026 â€” PROGRESS.md split into a tracker and this log (2026-10-13)](#task-0026--progressmd-split-into-a-tracker-and-this-log-2026-10-13)
- [TASK-0025 â€” The Language Options screen, reversed end to end (active)](#task-0025--the-language-options-screen-reversed-end-to-end-active)
- [TASK-0021 â€” The memory-card save system and the event tree it unblocked (active)](#task-0021--the-memory-card-save-system-and-the-event-tree-it-unblocked-active)
- [What gta-reversed actually does about "not implemented" (TASK-0020 follow-up)](#what-gta-reversed-actually-does-about-not-implemented-task-0020-follow-up)
- [NotImplemented reporting made central (TASK-0020 follow-up)](#notimplemented-reporting-made-central-task-0020-follow-up)
- [Ghidra MCP â€” still not connected, and why (RETRACTED 2026-10-13)](#ghidra-mcp--still-not-connected-and-why-retracted-2026-10-13)
- [TASK-0020 â€” Original source tree alignment (active)](#task-0020--original-source-tree-alignment-active)
- [TASK-0018 â€” Native runtime and replaceable rendering backend (active)](#task-0018--native-runtime-and-replaceable-rendering-backend-active)
- [TASK-0019 â€” Original function names (completed scoped investigation)](#task-0019--original-function-names-completed-scoped-investigation)
- [TASK-0017 â€” Submenu Back and overflow (active)](#task-0017--submenu-back-and-overflow-active)
- [TASK-0016 â€” Startup/media/audio/scroll integration (active)](#task-0016--startupmediaaudioscroll-integration-active)
- [TASK-0015 â€” Activation and animation integration (active; remaining gaps explicit)](#task-0015--activation-and-animation-integration-active-remaining-gaps-explicit)
- [TASK-0013 â€” Original front-end actually rendered (not a debug UI)](#task-0013--original-front-end-actually-rendered-not-a-debug-ui)
- [TASK-0012 update â€” Native runtime texture loader; standalone .exe built; original source tree recovered](#task-0012-update--native-runtime-texture-loader-standalone-exe-built-original-source-tree-recovered)
- [TASK-0011 update â€” Central GAMEFLOW_STATE_* dispatcher found; FUN_0016fa80 hypothesis retired](#task-0011-update--central-gameflowstate-dispatcher-found-fun0016fa80-hypothesis-retired)
- [TASK-0012 â€” PS2 original asset pipeline (active; container + P8/RGBA decode delivered)](#task-0012--ps2-original-asset-pipeline-active-container--p8rgba-decode-delivered)
- [TASK-0011 â€” Front-end menu minimal closed loop (active; scoped completion)](#task-0011--front-end-menu-minimal-closed-loop-active-scoped-completion)
- [TASK-0010 â€” Main-loop structural shell completed](#task-0010--main-loop-structural-shell-completed)
- [TASK-0009 â€” Whole-binary recovery active](#task-0009--whole-binary-recovery-active)
- [TASK-0008 â€” Police AI map completed](#task-0008--police-ai-map-completed)
- [TASK-0007 â€” Function naming completed](#task-0007--function-naming-completed)
- [TASK-0006 â€” PS2Recomp trial completed](#task-0006--ps2recomp-trial-completed)
- [Scope and milestones](#scope-and-milestones)
- [Assessment snapshot â€” TASK-0002](#assessment-snapshot--task-0002)
- [Delivered infrastructure](#delivered-infrastructure)
- [Environment observed at initialization (updates below)](#environment-observed-at-initialization-updates-below)
- [Initialization validation ledger â€” 2026-10-09](#initialization-validation-ledger--2026-10-09)
- [TASK-0014 front-end menu interaction (2026-10-12)](#task-0014-front-end-menu-interaction-2026-10-12)
- [Continuation](#continuation)
- [2026-10-14 â€” TASK-0033 native ImGui and original vehicle base material](#2026-10-14--task-0033-native-imgui-and-original-vehicle-base-material)
- [TASK-0033 compact viewer and loading follow-up â€” 2026-10-14](#task-0033-compact-viewer-and-loading-follow-up--2026-10-14)
- [TASK-0034 UV, continuous preview and shared materials â€” 2026-10-14](#task-0034-uv-continuous-preview-and-shared-materials--2026-10-14)
- [2026-10-14 - TASK-0035 wheels, glass environment and mirror binding](#2026-10-14---task-0035-wheels-glass-environment-and-mirror-binding)
- [TASK-0036 - Complete vehicle upload beyond host surface capacity](#task-0036---complete-vehicle-upload-beyond-host-surface-capacity)
- [2026-10-15 - TASK-0037 packet helper naming traceback](#2026-10-15---task-0037-packet-helper-naming-traceback)
- [2026-10-17 - TASK-0038 DrivingLab foundation and bounded driving evidence](#2026-10-17---task-0038-drivinglab-foundation-and-bounded-driving-evidence)
- [TASK-0038 continuation - 2026-10-17: native dependency recovery](#task-0038-continuation---2026-10-17-native-dependency-recovery)
- [2026-10-17 - TASK-0038 minimum drivable native loop completed](#2026-10-17---task-0038-minimum-drivable-native-loop-completed)
- [2026-10-17 - TASK-0038 user corrections: one CarViewer and original wheel bases](#2026-10-17---task-0038-user-corrections-one-carviewer-and-original-wheel-bases)
- [2026-10-17 - TASK-0038 Test drive WASD follow-up](#2026-10-17---task-0038-test-drive-wasd-follow-up)
- [TASK-0039 - Frontend rendering investigation and bounded fixes (2026-10-17)](#task-0039---frontend-rendering-investigation-and-bounded-fixes-2026-10-17)
- [TASK-0040 - Global symbol provenance audit (2026-10-17)](#task-0040---global-symbol-provenance-audit-2026-10-17)
- [2026-10-17 - TASK-0038 direct third-person Test drive redesign completed](#2026-10-17---task-0038-direct-third-person-test-drive-redesign-completed)
- [TASK-0041 - Symbol-rich PS2 reference recovery (2026-10-17)](#task-0041---symbol-rich-ps2-reference-recovery-2026-10-17)
- [TASK-0042 - Debug call chains and original source ownership (2026-10-17)](#task-0042---debug-call-chains-and-original-source-ownership-2026-10-17)
- [TASK-0043 - Full member mapping and identifier cleanup (2026-10-17)](#task-0043---full-member-mapping-and-identifier-cleanup-2026-10-17)
- [2026-10-17 - TASK-0044 Debug driving audit and canonical implementation](#2026-10-17---task-0044-debug-driving-audit-and-canonical-implementation)
- [2026-10-17 - TASK-0045 grip, terrain and Test drive material tests](#2026-10-17---task-0045-grip-terrain-and-test-drive-material-tests)
- [TASK-0046 - player transmission controls and original skids (2026-10-17)](#task-0046---player-transmission-controls-and-original-skids-2026-10-17)
- [TASK-0047 - Alpha archive extraction, 2026-10-17](#task-0047---alpha-archive-extraction-2026-10-17)
- [TASK-0048 - Real TrackViewer and track physics](#task-0048---real-trackviewer-and-track-physics)
- [TASK-0049 - Track orientation and contact debug display](#task-0049---track-orientation-and-contact-debug-display)
- [TASK-0050 - Track/car proportions and outside-camera projection](#task-0050---trackcar-proportions-and-outside-camera-projection)
- [TASK-0052 checkpoint (2026-10-18, incomplete)](#task-0052-checkpoint-2026-10-18-incomplete)
- [TASK-0052 subsequent validated checkpoint (2026-10-18)](#task-0052-subsequent-validated-checkpoint-2026-10-18)
- [TASK-0052 passing-build delivery requested (2026-10-18)](#task-0052-passing-build-delivery-requested-2026-10-18)
- [TASK-0053 runtime regression checkpoint (2026-10-18)](#task-0053-runtime-regression-checkpoint-2026-10-18)
- [TASK-0054 camera-motion projection regression (2026-10-18)](#task-0054-camera-motion-projection-regression-2026-10-18)
- [TASK-0056: Show LOD user-requested increment (2026-10-19)](#task-0056-show-lod-user-requested-increment-2026-10-19)
- [TASK-0056 authored LOD coverage clarification](#task-0056-authored-lod-coverage-clarification)
- [TASK-0057 - Track culling / generated LOD investigation complete](#task-0057---track-culling--generated-lod-investigation-complete)
- [TASK-0058 native culling integration complete (2026-10-19)](#task-0058-native-culling-integration-complete-2026-10-19)
- [TASK-0058 delivery dependency correction](#task-0058-delivery-dependency-correction)
- [TASK-0059 - Motion/rotation disappearance fixed with explicit particle gap](#task-0059---motionrotation-disappearance-fixed-with-explicit-particle-gap)
- [TASK-0062 - 2026-10-19](#task-0062---2026-10-19)
- [TASK-0063 - 2026-10-19](#task-0063---2026-10-19)
- [TASK-0064 - Live viewer draw count](#task-0064---live-viewer-draw-count)
- [TASK-0065 - Track11 dense matrix submissions](#task-0065---track11-dense-matrix-submissions)
- [TASK-0066 - Global batching without track-ID exceptions](#task-0066---global-batching-without-track-id-exceptions)
- [TASK-0067 - Live triangle/primitive counter](#task-0067---live-triangleprimitive-counter)
- [TASK-0072 worker integration,2026-10-20](#task-0072-worker-integration2026-10-20)
- [TASK-0075 - Original wheel particles and current driving gaps (2026-10-20)](#task-0075---original-wheel-particles-and-current-driving-gaps-2026-10-20)
- [2026-10-20 TASK-0076 driving render retention](#2026-10-20-task-0076-driving-render-retention)
- [2026-10-21 â€” TASK-0082 vehicle lights, horn and engine audio](#2026-10-21--task-0082-vehicle-lights-horn-and-engine-audio)
- [2026-10-21 â€” TASK-0083 collision and drift audio](#2026-10-21--task-0083-collision-and-drift-audio)
- [2026-10-21 â€” TASK-0084 recovered Test drive camera](#2026-10-21--task-0084-recovered-test-drive-camera)
- [2026-10-21 TASK-0085 GPU particle implementation](#2026-10-21-task-0085-gpu-particle-implementation)
- [2026-10-21 TASK-0086 original vehicle light sprites](#2026-10-21-task-0086-original-vehicle-light-sprites)
- [2026-10-21 TASK-0087 Track12 reported driving gaps](#2026-10-21-task-0087-track12-reported-driving-gaps)
- [2026-10-21 TASK-0088 original POV and held lookback correction](#2026-10-21-task-0088-original-pov-and-held-lookback-correction)
- [2026-10-21 TASK-0089 original water effects](#2026-10-21-task-0089-original-water-effects)
- [TASK-0090 - C++ portability audit (2026-10-21)](#task-0090---c-portability-audit-2026-10-21)
- [2026-10-26 TASK-0098 display dependencies](#2026-10-26-task-0098-display-dependencies)
- [2026-10-28 TASK-0098 actual production selection milestone accepted](#2026-10-28-task-0098-actual-production-selection-milestone-accepted)

<!-- generated:contents:end -->

---

## TASK-0032 â€” Debug CarViewer and TrackViewer scaffolds (2026-10-14)

Added asset-free Godot 3D viewers with explicitly synthetic fixtures, grid/axes, lighting,
orbit/free cameras, panels and command routing. Native port::extensions state/requests and
csys registration introduce no original format/ABI assumptions. viewer.load only records a
ZZDATA-relative request and reports the unavailable loader; no game resource is inspected.
The shared DebugTools autoload now keeps console, logs/history and FPS alive across scene
changes, separately from the frontend renderer. Command callbacks queue work; transitions
are deferred until after drawing. Returning creates a fresh frontend scene.

Native CTest 30/30 and Godot Debug CTest 32/32 passed. Console-off Debug and Release host
checks each passed 2/2; Release direct viewer entry exits 1 with its diagnostic. Real frontend
render smoke passed all 41 screens. Both synthetic viewers were captured and inspected in
D3D12 Forward+ windows, and windowed scripted camera/transition/input-isolation smoke passed.
Full commands, limits and continuation are in tasks/completed/TASK-0032-debug-viewers.md.

The initial viewer smoke terminated when first opening the restored console: upstream
LogWindow omitted EndChild when BeginChild returned false. The documented vendor patch
pairs them unconditionally; native tiny-viewport and host tests now pass without disabling
assertions. No reverse engineering, real asset loading or recovery-confidence changes.
Debug/console-on restored; prior uncommitted work preserved; no commit created.

## TASK-0031 â€” F8 debug console and independent Debug FPS (2026-10-14)

Port-side tooling only: pinned imgui-console 7840945b5a20ea9f4e83f98d1ad3d9379aede3ed
with bundled csys/ImGui, MIT licenses and documented focus/history patches. Native module
under game_core/extensions owns logs, commands and triangle conversion. The host uploads
the font, forwards clipboard/input and submits triangles under clip-parent canvas items.
F8 ignores repeat and captures input while updates continue. Removed native frontend's
TAB and PageUp/Down/bracket shortcuts. Debug FPS is independently positioned at top right.
Settings live under the Godot user directory. No original recovery state changes.

Validation is recorded in tasks/completed/TASK-0031-debug-console.md. Native CTest
29/29 and Godot Debug CTest 30/30 passed; Debug console-off and Release host smoke
checks passed sequentially. The final configuration is Debug/console-on. A default
D3D12 Forward+ window capture was also inspected. Different build presets share the
output DLL, so configuration checks must not race a link from another preset. Native tests cover bounded logs, commands, real TAB completion/history,
clipping and offsets. The synthetic Godot scene runs without assets and a Compatibility
window capture has been inspected. Upstream history initially skipped the newest command;
a targeted patch and real input regression test corrected it.

Toolchain: CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0 MinGW-w64, Python 3.14.4,
Godot 4.4.1.stable.official.49a5bc7b6. No commit created.

## TASK-0030 â€” Concise README completion (2026-10-13)

Completed the user's five-section README template in 59 lines, retaining its title/headings.
Explained native core/runtime/host responsibilities, partial frontend coverage, remaining
gameplay work, class-template limits, dependency/asset setup, build/run/test commands and
documentation entry points. Corrected the opening's renderer-only description to reflect
Godot's current window/input/audio/scheduling and remaining frontend orchestration duties.
All 12 local links and documented commands were checked; build.ps1 --help, diff and generated
progress checks passed. No runtime tests rerun for this documentation-only task, no behavior
or original-equivalence status changed, no commit created. Recovery priority remains TASK-0025.

## TASK-0029 â€” Root-level classes and Ghidra-confirmed directories (2026-10-13)

The user explicitly replaced TASK-0028's unknown-path placement rule: move all class pairs
out of port/class_templates into game_core, then apply original directories only when Ghidra
confirms them. Preserved all existing dirty/untracked work. 179 pairs / 358 files moved with
destination collision checks; 170 pairs remain at root, eight moved to indep/src and bFile
to src. Existing module classes stayed in place. The empty old directory contains no files.

Read-only Ghidra MCP scan over 002AB000..003358F0 found 298 source-path occurrences / 54
distinct paths, each byte-read and cross-referenced (323 references, maximum 40 per occurrence;
all below the 500-entry page size). Exact instruction ownership in the private corpus avoids
oversized function-envelope mistakes. Focused decompile and allocation-call listings establish
nine class-specific directory bindings, E-0029-S01/P01..P09. The parallel qualified-diagnostic
branch checked 38 classes / 100 distinct string addresses / 103 class-label entries and
found the class-qualified bFile assertion plus its src\\bFile.cpp argument in one call.
DemoDiscManager and SlotPool/SlotPoolManager retain unknown paths; nearby header strings are
proximity only. PsychoDriver/tCustomizeHUDScreen's generic container-header references are
not source-directory ownership. See docs/CLASS_SOURCE_PATHS.md for all accepted associations.

Exact original translation-unit names and binding instructions are preserved in
reverse/classes/source-path-decisions.json; the complete derived path scan and all 179
class results are public metadata. Raw Ghidra responses/listings stay under ignored
local/task0029. Per-class generated filenames/header extensions remain port-side partitions
inside the confirmed directories; no middleware root, inheritance, signature or behaviour
is invented. Four new path-only Identified records/index rows record previously unindexed
initialization/assertion entries without widening the existing 609-member template API.

Updated self-includes, CMake source list, generated tests/inventory, symbol source links and
the placement policy. The generator's default is now game_core root; it cannot recreate the
old temporary layout. Its symbol-link refresh now replaces obsolete generated annotations
instead of retaining stale paths. Added directory-evidence and stale-link regression tests.
Hash audit of all 358 original class files matched after allowing only include/provenance
changes and line-ending normalization. No class/method body or API was changed.

Validation: CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0, Python 3.14.4; ELF SHA-256 and godot-cpp
pin matched their registered values. `generate_class_templates.py --write` / `--check` passed;
`python -m unittest discover -s reverse/tests -p test_class_templates.py` passed six tests.
`cmake --preset native-debug`, `cmake --build --preset native-debug --parallel 8`, and
`ctest --preset native-debug` passed (28/28, including all 603 exception contracts).
Godot configure/build linked to local/task0029/godot-bin through the ignored deferred-output
include, then its normal output configuration was restored. `ctest --preset godot-debug
-R 'class_template_'` passed 3/3. Progress regeneration/check and diff review passed.
Optional empty-directory removal was rejected by command policy; all requested class-file
moves succeeded via the separate validated move-only command. No files were deleted or
overwritten, no proprietary material entered the source diff, and no commit was made.

TASK-0029 is completed. Continue unresolved class placement only from new direct constructor,
class-qualified diagnostic or original debug/type evidence. Unknown classes remain at root.
The frontend recovery priority remains TASK-0025's 00183830 profile/game reset; normal DLL
deployment still requires the running frontend to release godot/bin/ps2_port.dll.

## TASK-0028 â€” Editable C++ class/member templates (2026-10-13)

Implemented the user's evidence-driven template plan as a separate task. Target US PS2
SLUS_203.62, unknown executable revision, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; local Get-FileHash matched.
Ghidra MCP was available; fresh inspect/decompile/references and memory reads remained read-only.

190 valid class spellings now have declarations, including 38 qualified-diagnostic names beyond
the 152 valid allocator labels. The 23 space/parenthesis labels stay verbatim catalogue-only,
per the user's decision. Coverage is 609 member attributions over 370 distinct addresses:
603 throwing templates, six preserved native projections, and 134 classes without bound
members. Twelve full-entry templates coexist with separate preserved partial helpers.
Source paths without evidence use port/class_templates; existing screen/node module placements
remain explicitly port choices rather than claims of recovered exclusive source ownership.

Fresh allocation chains justify TitleScreen and tEventLeaf replacing their previous address
names. Unknown methods use Sub_<ADDR>; caller/test interfaces were renamed together, while
actual C++ bodies and storage were preserved. The NewSlotPool diagnostic confirms TASK-0019
and counters TASK-0027's constructor reading; bFile::Filename remains a field expression.
See docs/CLASS_TEMPLATES.md and reverse/classes/template-audit.json (E-0028-C01..C06).

The generator creates/appends missing code, never replaces edited bodies or existing records,
and synchronizes the coverage inventory, explicit CMake source list and exhaustive exception
tests. Added 343 Identified index rows and 342 records, indexing one already-existing record
without replacing it. Seven legacy duplicate-address index rows were preserved; no original
recovery status changed. Read-only diff audit matched 501 pre-existing bodies after the stated
identifier substitutions and retained all 368 original index status rows. Template tests
do not establish reconstructed behaviour or original ABI and promote nothing to Verified.

Validation (CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0, Python 3.14.4; dependency pins unchanged):

- `py -3 reverse/scripts/generate_class_templates.py --write` / `--check`: passed.
- `py -3 -m unittest discover -s reverse/tests -p test_class_templates.py`: 4 tests passed.
- `cmake --preset native-debug`; `cmake --build --preset native-debug --parallel 8`: passed.
- `ctest --preset native-debug`: 28/28 passed, including 603 throwing-entry checks.
- First metadata check caught volatile progress counters in the label-only catalogue after
  `recover_classes.py --emit`. Removed those unrelated counters; final coverage is stable.
- `cmake --preset godot-debug`: passed. Default build compiled bridge sources but linking
  could not write godot/bin/ps2_port.dll (Permission denied; running DLL left untouched).
- Isolated link passed with the ignored local/task0028/isolated_output.cmake deferred output
  override, configured through `-DCMAKE_PROJECT_PS2PCPort_INCLUDE=...`; the DLL is under
  local/task0028/godot-bin. Restored normal configuration with
  `cmake --preset godot-debug -U CMAKE_PROJECT_PS2PCPort_INCLUDE`.
- `ctest --preset godot-debug -R 'class_template_|police_control|movieplayer|frontend_viewport'`:
  6/6 passed. No live DLL replacement or visual/original comparison claimed.
- `recover_classes.py --emit`, `generate_progress.py` / `--check`, and `git diff --check`:
  passed. No proprietary binaries/assets are included in the source diff; no commit made.

TASK-0028 is completed. Continue a selected function in a separately scoped task using the
inventory's record/address, with original signature/boundary evidence before implementation.
For normal frontend deployment, rerun the Godot build once the current DLL is released. The
existing frontend completeness priority remains TASK-0025's 00183830 profile/game reset.

## TASK-0027 â€” Class names, and the one mechanism that binds them (2026-10-13)

User asked whether class names had been identified, wanted the task board out of the tracker,
and wanted a list of every class with its own progress -- which meant identifying the functions
inside each class first.

The answer to the question was "48, and almost uselessly": `.rodata` holds 115 `Class::Method()`
labels, but **7 of them are referenced by any code**. The profiler instrumentation that passed
them was compiled out of the retail build, so they are live strings for a dead path. That was
worth proving twice rather than once -- Ghidra `references_to`, and a scan of every instruction
word in the local corpus for the `lui`/`addiu`|`ori` pair that materializes each address, which
is the only way MIPS forms a 32-bit constant. A pointer search finds nothing either. One detail
decided it: the binding has to be done on the full string start, not the offset of the class
name inside the string, which is the difference between 1 hit and 7.

What does work is the **named allocator** `FUN_002577C0(size, label)`, which the game calls with
the name of the object it is about to build. TASK-0008 had found a single instance of this at
`0x00131410` -- a `CopDriver` label, then the constructor that writes vtable `0x002B5998` --
without generalising it. Generalised: 579 call sites, **175 class names** with object sizes, and
through constructor -> vtable -> slots, **791 member functions** across 252 confirmed vtables.

Two things made the vtable pass trustworthy rather than plausible:

- The layout was read off recovered code, not assumed. `001F1DD8` message `0x0C` calls
  `(**(code **)(vtable + 0xC))(this + *(short *)(vtable + 8), 3)`, so entries are 8 bytes with
  the pointer at `+0xC + 8k`. A scan starting at `+4` finds nothing but zero -- which is exactly
  what the first attempt did.
- Across all 1577 accepted slots every `this` delta is 0, -8 or -256. Random data would spread
  those over 16 bits, so the absence of a single arbitrary value is independent evidence that
  the runs are really vtables. It is now part of the acceptance test.

Four cross-checks against other tasks all matched, including the Controller and Jukebox vtables
whose entries `0x002402C0` and `0x001F0D10` Ghidra never created -- which is why the slot test
is "4-aligned inside .text" rather than "a known Ghidra function". Whether a function object
exists is a property of that database, not of the binary.

One defect found and fixed that would have quietly weakened any future corpus scan: the corpus
prints instruction words without leading zeros, so the fixed eight-digit match dropped **every**
R-type, `j` and `jal` -- hiding all calls and disabling register invalidation, which lets a
stale `lui` pair with an unrelated low half.

Names this hands the port, as an AGENT.md R2 upgrade still owed: `AudioOptionsScreen`,
`tDisplayOptionsScreen`, `tOptionsScreen`, `JoystickPreconfig`, `JukeBoxMusicScreen`,
`tCustomizeHUDScreen`, `tEventTreeView`, `tEventGroup`, `tEventLeaf`, `Autosaver`,
`BootMemcardLoader`, `CopsOnModeSelect`. The Language Options screen is *not* among them: it is
built with the unlabelled `FUN_00257690`, so its class name stays unknown. 175 is a floor.

Validated: `ctest --preset native-debug --output-on-failure` -> 25/25 passed. Write-up in
`docs/CLASS_RECOVERY.md`; the table is generated into `docs/PROGRESS.md` from
`reverse/classes.csv`, and the task board is gone from it.

## TASK-0026 â€” PROGRESS.md split into a tracker and this log (2026-10-13)

User said this document was too messy to track anything from, and pointed at
`D:\dev\gta-reversed\docs\ReversedClasses.md` as the model.

The property worth copying from that file turned out not to be its layout but the line in its
own header: it is regenerated from `hooks.json` every time hooks change. Hand-maintained
progress numbers go stale without anyone noticing, and a figure nobody can reproduce is worse
than no figure. So `docs/PROGRESS.md` is now generated from this repository's own records --
`reverse/symbols.csv`, `reverse/structures/`, `docs/assessment/FUNCTION_INVENTORY.csv`, the task
files, and the `hp2::not_implemented` call sites in `src/` -- by
`reverse/scripts/generate_progress.py`, with a `progress_tables` ctest that fails when the
tables are stale. The 891 lines of narrative that used to be in it are below, verbatim.

Three things the tracker shows that the reference does not, because this project has them:

- **The status ladder's top rung is empty and shown as such.** `Verified` needs a comparison
  against the original (AGENT.md R1); nothing has it. Printing `Verified 0` in the same table as
  the progress figure states the project's central limitation where it cannot be missed.
- **The frontier is exact.** R3 makes every untranslated recovered path name its own ELF
  address, so the `not_implemented` call sites are a machine-readable work queue: 86 sites
  across 62 distinct addresses.
- **Structure records are counted next to function records.** 4 against 368. A translated
  function whose structure layout is unknown is still guesswork at the field level, and
  reporting only function progress hides that.

Per-subsystem percentages were deliberately left out: `FUNCTION_INVENTORY.csv` classifies 7525
of its 7544 rows as `Unknown`, so any per-subsystem denominator would have been invented.

Two inconsistencies the generator surfaced rather than papered over, both recorded as follow-ups
in `tasks/completed/TASK-0026-progress-tracker.md`: the task files' `Status:` lines are written
three different ways, and two structure records sit outside the `<binary-id>/<subsystem>/` layout
R6 specifies.

Validated: `ctest --preset native-debug --output-on-failure` -> 25/25 passed.

## TASK-0025 â€” The Language Options screen, reversed end to end (active)

User hit `hp2::NotImplemented: screen item activation handler not translated (ELF 0x1f1dd8 ...)`
and asked for the language menu and its functions. That address is reached whenever any item on
the Language Options screen is activated, because the port had the screen's Back arm (TASK-0017)
but not its activation arm, and `dispatch_activation` had no route for it at all.

**Ghidra MCP was reachable again** through `tools/ghidra.py` at `http://127.0.0.1:8080/mcp`
(14 tools). Read-only throughout; no Ghidra state was modified (AGENT.md R5).

Full write-up: **docs/FRONTEND_MENU_LANGUAGE.md**. What it establishes:

- `001F1DD8` is the **class router**, not a handler. Message `0x0B` picks a constructor by
  `strcmp` against the class-name literals (`"Language Options Pre FE"` -> `operator new(0x10)` +
  `001F1A78(mem, screen, 0)`; `"Language Options"` -> `operator new(0x58)` + `001F1D70(mem,
  screen, 1)`), `0x0C` destroys, everything else forwards to `001F1B08`.
- The six language items are `MENUITEM_1..MENUITEM_6` (`0x52E039F3..0x52E039F8`) and map to
  language index 0..5. The hashes are **positional item names**, which is why the Options and
  Mode Select screens use the same six values for completely different targets.
- The locale order is read out of the ELF rather than assumed: the pointer table at
  `0x0031C168` is ENGLISH, FRENCH, GERMAN, ITALIAN, SPANISH, SWEDISH, then two development
  locales (`FE_LONGEST.BIN`, `FE_ID.BIN`) that no menu item selects. The port's
  `apply_language` already used that order; now it has a citation.
- **There is a seventh item, and the game always hides it.** `001F1A78` ends with
  `if (FUN_0016fdb0() == 0) FUN_0016d2a8(class, MENUITEM_7, screen, 2)`, and `0016FDB0` is
  `return 0;` -- so the branch is unconditional, the item's enabled bit is cleared and its
  widget hidden on every entry. The player's own `FRONTB.BUN` does author seven `MENUITEM_n`
  widgets, so this is observable behaviour, not a defensive check. Its activation arm
  (`DAT_00331A60 = 1`, then pop) is therefore recovered but unreachable from input.
- `instance+0x08` is the single bit of state that separates the two classes: `0` (the boot-time
  `Pre FE` prompt) reaches the `00183830` full profile and game reset, `1` does not. That holds
  on the activation arm exactly as TASK-0017 found it on Back.

One bug the tests caught, worth recording because the same shape can recur: `0021E0D0` takes the
**address** of the language global and writes it itself, so the handler's `== -1` check
afterwards is a re-read of what the loader left behind. The first port version copied
`instance+0x00` into the global at that point, which made the `-1` arm unreachable. The Back
overload has the same pattern but writes a context copy, so it cannot mask the arm -- noted as
an open item rather than changed under this task.

Validation on 2026-10-13, CMake 3.29.2 / Ninja 1.12.0 / GCC 13.2.0, host only:
`cmake --build --preset native-debug --parallel 4` then
`ctest --preset native-debug --output-on-failure` -> **24/24 passed**. The Godot extension
(`cmake --build --preset godot-debug`) relinks `godot/bin/ps2_port.dll` cleanly.
`frontend_runtime` drives the real screen off the player's own assets: it enters Language
Options, asserts seven authored items with `MENUITEM_7` disabled and hidden, activates a
language, and checks the locale applied and the screen popped. No original hardware or PCSX2
comparison was performed, so nothing on this path is `Verified` (AGENT.md R1).

**Blockers and explicit NotImplemented boundaries that remain:** `00183830` (so `Language
Options Pre FE` activation still cannot complete -- depends on TASK-0021's profile work), the
`001EE918` sub-object `001F1D70` builds at instance `+0x10`, the `0x0C` destructor arm, the
`0x12` arm, and no located reader of `DAT_00331A60` (search bound recorded in the task file).

**Exact next action:** TASK-0025 step 1 -- translate `FUN_00183830(0x002FC050)`, the full
profile and game reset, against the TASK-0021 `settings.db` profile layout, so the boot-time
`Language Options Pre FE` path can complete. Start from
`tasks/active/TASK-0025-language-options-screen.md` and `docs/FRONTEND_MENU_LANGUAGE.md`.

## TASK-0021 â€” The memory-card save system and the event tree it unblocked (active)

User asked to finish reversing `eventtree`, with the correct front-end implementation including
its animations and effects, and said the save system had to come first because the event tree
cannot be implemented without it. That was right: five of the six open questions in
`docs/EVENT_TREE.md` were blocked on save data.

**Ghidra MCP is connected for the first time in this project.** Earlier sessions recorded it as
unavailable. It is reachable at `http://127.0.0.1:8080/mcp` (14 tools) through the existing
read-only client `tools/ghidra.py`; no Ghidra state was modified (AGENT.md R5).

### The save system, recovered end to end

- Original source file `src\memorycard.cpp` (ELF string 0x002AD488), module at roughly
  0x0012A000..0x0012E6FF, immediately below `HP2_MainFrameLoop`.
- Save directory **`BASLUS-20362HP2`** (NTSC-U) / `BESLES-50731HP2` (PAL), built by
  `FUN_0012DEE0` from recovered format strings; `icon.sys`, icon `nfs6.ico`, magic `PS2D`.
- **One payload file, `settings.db`, of 0x114E0 = 70,880 bytes**, self-describing and with no
  separate header: `FUN_00245738` stamps the payload's own length into its first word and a
  checksum into its last. `FUN_001878B0` is a signed 32-bit sum of 0x4537 words == 0x114DC bytes,
  i.e. everything except the checksum â€” a figure derived from the loop's termination condition
  and then found to agree exactly with the checksum's offset.
- **The global player profile is that payload**: one 0x114E0-byte block at **0x002FC050**, with
  no serialiser at either end. Three arithmetic identities from three different functions confirm
  the base, one of them being that profile+0x1072C is `DAT_0030C77C`, the tree selector
  `docs/EVENT_TREE.md` already named.

### What that unblocked in the event tree

- The `0x34A03` table is a **requirement** table, not a link table: `row+0x02` names a
  prerequisite event and `row+0x04` is the goal it must meet. Four of the six bytes TASK-0018
  listed as unknown are now identified.
- `FUN_002010B8` fully decompiled: five status values (0 locked, 1 not attempted, 2 below the
  floor, 3 goal missed, 4 won) and four per-GoalType slice decoders. Two decoders and both
  completion gates are still unrecovered and now **raise `hp2::NotImplemented`** naming their ELF
  address instead of being silently skipped.
- The camera is recovered: `FUN_001FAB00` pins the selected node to (320, 240) and scales by a
  zoom **level**, 0 or 1, out of the two-float table `{0.4f, 1.0f}` at gp-0x4178 (gp ==
  0x003358F0, established from the already-known 350.0/130.0 pair). The port-side `kPortZoom =
  0.6f` and `kPortPanelScale = 0.26f` are gone.
- Nine unexplained ELF immediates resolved by hashing screen 39's real widget names:
  0x0880AC4C is `HELP_TEXT`, 0x027F7503 `HELP_BUTTON`, 0xAA5D7D3F `BOTTOM_MASK`, 0xA3B23510
  `EVENT_PANEL`, 0xA01BCDC6 the class EVENT_PANEL and EVENT_NUMBER share, and 0xA744BA7B /
  0xFD297FD4 / 0xF1EED4CD are screens 17, 18 and 19 â€” so the event tree is a four-document
  cluster, not one screen.
- Start node, tree type and view state all come from the profile (`FUN_001F5D00` /
  `FUN_001F5D28` / `FUN_001F5D50`, read from the listing because the decompiler lost the R5900
  argument order). `FUN_001F5D50` turns out not to be "tree selection" as recorded; it is a
  two-instruction setter.

### Defects found and fixed in the existing port

- **`EventTreeView::move_cursor` invented the left/right rule.** It stepped one place through the
  same-depth leaves in parse order; `FUN_001FA880` takes the nearest leaf in **X** on the
  requested side, strictly, with no wrap. Replaced, along with `FUN_001FA978` for up/down, which
  takes the nearest in X on one of the two adjacency lists.
- **The recovered "moved means sound, not moved means silence" rule was computed and discarded.**
  `event_tree_joy_event` now routes the recovered ids 0x13/0x14/0x15/0x18/0x8 through
  `PlayMenuSound_0016E038`, the same path every other screen uses.
- **State 2 did not exist.** Confirm and Back now drive the recovered 0 to 2 transitions.
- **`frontend_runtime` was passing under CTest while asserting almost nothing**: it had no
  `WORKING_DIRECTORY`, so it never resolved the asset root and returned early. Fixed; its runtime
  went from 0.35 s to about 17 s, and it now covers the event tree properly.
- Evidence debt: `E-0018-01..12` were cited in `docs/EVENT_TREE.md` and `docs/REVERSE_NOTES.md`
  but never allocated to any file, and 0x001FFC40 / 0x001F96B8 / 0x002010B8 had no records.
  Eleven records are filed under `E-0021-NN` with rows in `reverse/symbols.csv`; the
  `E-0018-NN` gap is noted but not retrofitted.

### Follow-up: the route into the tree was broken, and is now fixed

User reported that choosing Championship or Ultimate Racer from the main menu did not enter the
event tree. That was correct and it was a real gap: the first pass made the tree work once
screen 39 was already open, but never reversed how a player gets there. Three links were
missing.

1. **The class had no registration row.** `View Event Tree` does not register through
   `FUN_0016ADB0` like the other 37 classes -- it tail-calls the sibling `FUN_0016AD88` at
   0x001FB860. Its operands (0x001FB7F0..0x001FB830) are the class-name string 0x002BA718, the
   handler `FUN_001FB218` and the document hash **0x147F8A59**, which is exactly screen 39's
   stored name hash. The port's registry stopped at the `FUN_0016ADB0` rows, so
   `find_registration("View Event Tree")` returned null and `apply_pending_screen` raised
   NotImplemented at 0x0016C8C8. (E-0021-22)
2. **The transition is deferred behind an animation.** `FUN_00217020` / `FUN_002176D8` do not
   switch screens; they build a 0x1C-byte `tExitScreenAnimation` through `FUN_002490D8` -- the
   same `Type_002490D8` the port already implements for Options -- and the switch happens only
   when it finishes. (E-0021-25)
3. **The port ignored the chosen mode.** `FrontEndRuntime` held `event_tree_mode_ = 9` as a
   hardcoded field, so Championship would have opened the Hot Pursuit tree even with the first
   two fixed. `FUN_001FB218` message 0x0B shows the real source: `FUN_00185568(0x2FC050)`, the
   profile. The field is gone. (E-0021-24)

Also recovered on the way: five of the six mode-select globals are **inside the profile**
(`DAT_002FC06C` +0x1C, `DAT_002FC1B4` +0x164, `DAT_002FC1C0` +0x170, `DAT_0030C77C` +0x1072C,
`DAT_0030C784` +0x10734), so the handlers' writes are save writes; only `DAT_0031AB90` is
outside. `FUN_001F5D50(0x2FC1B4, n)`, which the port had marked "not translated", is now wired
through the profile. (E-0021-23)

Both routes are asserted end to end by the new `check_event_tree_route` in
`src/tests/frontend_runtime_tests.cpp`, which drives the real joy events and ticks the exit
animation out, and both were confirmed in the running app: screen 1 item 3 and screen 2 item 2
each reach screen 39, with different modes, different tree types and visibly different panel
artwork.

**A note on how this was missed.** The first pass's event-tree test opened screen 39 with
`menu_open(39)` directly, so it exercised the tree without ever exercising the route -- and it
passed. That is the same shape as the `frontend_runtime` CTest hole fixed earlier in this task.
The new test enters the way a player does.

### Validation (exact commands and results)

CMake 3.29.2, Ninja 1.12.0, MinGW GCC 13.2.0, Python 3.14.4, Godot 4.4.1.stable.official.49a5bc7b6.

- `cmake --build --preset native-debug --parallel 4` then `ctest --preset native-debug`:
  **23/23 passed** (22 before; `memorycard` is new).
- `cmake --build --preset godot-debug --parallel 4` then `ctest --preset godot-debug`:
  **23/23 passed**.
- `./tools/run_frontend.ps1 -RenderSmoke`:
  `NATIVE_RENDER_SMOKE_OK: screens=41 quads=1085 glyphs=7765 textures=120 empty=[36]` â€”
  identical to the pre-task baseline. One change was needed to keep it that way, and it is
  worth recording rather than hiding: once the tree started taking its mode from the profile,
  the smoke's cold `menu_open(39)` produced no tree at all (927 quads / 7733 glyphs /
  118 textures), because profile+0x1072C is 0 until a mode-select screen writes it. That is the
  correct recovered behaviour, not a regression â€” the original would show nothing either. The
  smoke now walks the real route once before its per-screen loop, so its screen-39 coverage is
  real rather than accidental.
- `./tools/run_frontend.ps1 -RuntimeSmoke`: `FRONTEND_RUNTIME_SMOKE_OK`.
- Screen 39 captured on DX12 (Intel Arc B390) and inspected: the tree renders with real panel
  art, the recovered 0.8 vertical squash, and the selected node at twice the size.
- `py -3 reverse/scripts/validate_main_loop.py --metadata-only` via the `main_loop_catalog`
  test: unchanged.

**Nothing was promoted to `Verified`.** No PCSX2 or original-hardware comparison was performed,
no real save file was read or written, and every test fixture is synthetic. A passing PC test
says the port reproduces what was read out of the ELF and nothing more (AGENT.md R1).

### Exact next action

Read `AGENT.md`, `docs/SAVE_SYSTEM.md`, `docs/EVENT_TREE.md` section 7 and
`tasks/active/TASK-0021-memorycard-profile.md`, then take TASK-0022's first step: decompile
`FUN_001F6B70` (the state-2 event detail sub-menu over document 17) and the `tLineSegment` build
inside `FUN_001F96B8`, read-only through GhidraMCP at 127.0.0.1:8080. TASK-0023 (the PCSX2
comparison at `D:\Game\pcsx2`) is the only route to a `Verified` promotion. Nothing is
committed.

## What gta-reversed actually does about "not implemented" (TASK-0020 follow-up)

Studied `D:/dev/gta-reversed` at the user's request, specifically how it *catches*. The finding
changes the comparison:

**It does not catch it.** Seven `catch (` sites exist in the whole project, none for an
unimplemented path. `NOTSA_UNREACHABLE` logs, shows an Abort/Retry/Ignore box (Retry calls
`__debugbreak()`) and then `exit(3)`. The one meaningful catch, in
`reversiblehooks/ReversibleHook/StatefulHook.h`, converts straight to `NOTSA_UNREACHABLE` with
the comment that restoring state properly would be work.

**It can afford that because an un-reversed function is not "unimplemented" there.** Its body is
a thunk into the retail binary -- `plugin::CallMethod<0x4FF900, CAEVehicleAudioEntity*, ...>(this, ...)`
-- because the project is a DLL inside the original game, so the original code is in the same
address space and simply runs. `NOTSA_UNREACHABLE` is reserved for genuine invariants, and its
465 sites are "this cannot happen", not "not reversed yet".

**That option is closed to us.** This is a native port with no original binary in process, and
PS2RECOMP_TRIAL.md records that the static recompilation attempt produced unverified instruction
translations only. There is nothing to delegate to, so an untranslated path has to fail, and
throwing-and-reporting is strictly better than their terminate: a Godot host can show the gap and
keep running. Our design is the right one for our architecture, not a weaker copy of theirs.

**What was worth taking:** their single process-wide net,
`SetUnhandledExceptionFilter(WindowsExceptionHandler)` in `source/app/app_debug.cpp`, which logs
exception code, address and registers. We had no equivalent. `hp2::guard` catches
`hp2::NotImplemented` -- the expected outcome -- but a `std::bad_alloc` or an asset parse error
would have crossed into godot-cpp, which is compiled `-fno-exceptions`, i.e. undefined behaviour
with no diagnostic. `src/platform/godot_bridge/boundary.hpp` now provides `catch_all`, and the
bound methods that reach into the runtime (`menu_open`, `menu_joy_event`, `menu_tick`,
`Ps2RenderHost::draw`) run through it, so a port bug becomes a reported error instead of a silent
editor crash. The net sits at the boundary rather than process-wide because that is where enough
context still exists to report rather than die.

## NotImplemented reporting made central (TASK-0020 follow-up)

User asked for one unified place instead of a macro at every site, with automatic catching and
the standard C++ assert dialog. `src/game_core/port/not_implemented.hpp` is now that place:

- The `HP2_NOT_IMPLEMENTED` macro is deleted. All 52 sites call
  `hp2::not_implemented(elf_address, what)` directly -- a `[[noreturn]]` function, no preprocessor.
- One reaction point, settable once. It always throws; on top of that
  `HP2_ASSERT_NOT_IMPLEMENTED=1` routes through the standard `assert()` with no code change
  (verified), and `hp2::set_not_implemented_handler` installs a debugger reaction.
- Studied `D:\dev\gta-reversed` at the user's request. Its `notsa::unreachable`
  (`source/Base.h`) appears at **465 call sites**, which settles the question of whether a mature
  reversing project can avoid the per-site marker. Borrowed: the reaction belongs in one place,
  and a debug build should offer Abort/Retry/Ignore with Retry breaking into the debugger --
  `src/platform/not_implemented_dialog.cpp` now does exactly that behind
  `HP2_NOT_IMPLEMENTED_DIALOG=1`, kept in `src/platform` so the core still depends on nothing but
  the standard library. **Not** borrowed: their macro compiles to `__builtin_unreachable()` in
  release. For them "unreachable" is an invariant; for us "not implemented" is reachable by
  definition, so compiling it away would manufacture the silent wrong answer R3 forbids.
- One catch point: `hp2::guard([&]{ ... })` returns the message or an empty string. The five
  hand-copied production `try`/`catch` blocks are gone. The six that remain are in tests, where
  catching *is* the assertion.

**The per-site call stays, and that is deliberate.** Each one carries the ELF address of the
original function that is missing; that address is evidence (AGENT.md R2/R3), nothing else in the
program knows it, and no mechanism can infer "this path was never reversed". Removing the marker
would turn a loud failure into a silent wrong result, which is the failure mode the rule exists to
prevent. AGENT.md R3 now spells this out along with the new API.

Validation: 22/22 CTest, all Godot checks unchanged, both presets build with no warnings.

## Ghidra MCP â€” still not connected, and why (RETRACTED 2026-10-13)

**This conclusion was wrong and cost several sessions.** The server was running the whole
time at `http://127.0.0.1:8080/mcp`, reachable through this repository's own read-only client
`tools/ghidra.py` (`from ghidra import Client`). It is not registered as a session tool, which
is why `session_connectors_status` and a tool search both come up empty -- that absence is not
evidence that it is unavailable. Probe the endpoint before concluding anything. The rest of
this section is kept as the retired claim, per AGENT.md R6.

`session_connectors_status` lists only Claude Docs, visualize and scheduled-tasks. No Ghidra or
REA MCP server is registered for this session, and `mcpServers` is empty at top level and for
every project in `~/.claude.json`, so there is nothing to re-enable or reconnect from inside the
session. The Ghidra side is ready -- **GhydraMCP v2.2.0-rc.3 is installed** into
`ghidra_12.0.4_PUBLIC/Extensions/GhydraMCP` (its `extension.properties` says `ghidraVersion=11.4.2`,
a mismatch worth checking) and its jar carries the `XrefsEndpoints` this work needs. What is
missing is the client half: no MCP bridge is installed anywhere on this machine. Pinning the 97
identifier strings to addresses is blocked until the user registers that bridge.

## TASK-0020 â€” Original source tree alignment (active)

User asked that `src/game_core` match the original HP2 project's structure, file names,
folder names, class names and function names, using the REA skill and a Ghidra MCP string
scan. **Ghidra MCP is not connected to this session** (no Ghidra or REA MCP tool is
registered), so the scan was done directly: the authorized ELF is present at
`D:/ps2_game/SLUS_203.62` with a SHA-256 matching `reverse/binaries.csv` exactly, and a
read-only scanner parses its PT_LOAD headers to address-anchor every string. 8,153
strings; 296 source-path hits / 54 distinct, corroborating the TASK-0016 rescan (298/54).
Catalogue in `reverse/naming/task0020-original-source-tree.csv` (E-0020-N01..N03).

Done: `src/game_core` restructured onto the recovered tree. Seven files whose original
path is recovered verbatim now sit at it under `indep/src/`, the bWare `bJukebox` pair
under `bSound/{src,include}` (the `bSound/` root inferred, noted as such), and the 26
files with no recovered path under `src/game_core/port/`, so no location implies
provenance it does not have. Every file opens with either its ELF string and address or a
note that its path is not evidence. Also fixed a pre-existing corrupted path record in
this file: `indep\src\fmenus.cpp` had been stored with a literal form-feed where the
`\f` should be.

**No identifier was renamed.** The 74 `Class::Method` and 23 pool-name strings the ELF
carries are real original identifiers, but every one is `string_only` â€” none is pinned to
the address of the function it names, and TASK-0019 already ruled that allocation labels
are not type/method proof. `Sub_<ADDR>` / `Type_<ADDR>` names therefore stand (AGENT.md
R13). Tempting but refused: `tExitScreenAnimation` for `Type_002490D8`,
`tMenuAnimElement` for `MenuHighlightAnim_00248870`.

Validation: 22/22 CTest and all five Godot checks pass unchanged after the move
(`--native-render-smoke` 41 screens / 1085 quads / 7765 glyphs / 120 textures). Stale
`src/game_core/...` paths in three scripts were updated; `main_loop_catalog` caught one.

Exact next action: reconnect Ghidra MCP, then pin the `Class::Method` and pool strings to
addresses via cross-references so identifier renaming becomes evidence-backed
(TASK-0020 next steps). Nothing is committed.

## TASK-0018 â€” Native runtime and replaceable rendering backend (active)

User resumed the deferred architecture task and authorized the migration. The front end
no longer renders through GDScript. D008 records one rendering contract: native
`render::Frame` in, a `render::Backend` out, no second command path.

Landed: Godot-free `port_runtime` library (frame contract, original-asset `Catalog`,
new `frame_builder` producing layer-ordered quads, per-glyph text, align bit and the
original `GetWidgetClip_00145F30` clips); `Ps2RenderHost`, a RenderingServer backend
owning canvas-item and texture RIDs with revision-gated uploads; `Ps2MoviePlayer`
exposing decoded frames as backend-independent bitmaps; native widget measurement
replacing the host round trip; and `scenes/frontend_native.tscn`, a host that owns only
window, input translation, delta, audio hardware and lifetime. The previous
`scenes/frontend.tscn` is retained unchanged.

Validation: `ctest --preset native-debug` 21/21 passed (new `frame_builder` test uses a
fake Resources and fake Backend, no Godot). `--native-render-smoke` renders all 41
screens natively (927 quads, 7733 glyphs, 118 textures). Old-vs-new pixel diff over all
41 screens: **32 byte-for-byte identical**; the 8 background-movie screens differ only in
video-decode and scroll phase (control experiment isolates this), and screen 39 differs
because only the old script draws the event-tree overlay. Every pre-existing Godot check
on the retained scene still passes. PC-to-PC only; nothing promoted to Verified.

Both scenes gained an always-on window-title FPS/frame-time readout naming the render path,
plus the same line in the Tab overlay. Uncapped main-menu throughput measured once on this
machine: native ~1380-1450 FPS vs GDScript ~1030-1195. Indicative only, not a benchmark.

Second pass: `port::runtime::FrontEndRuntime` now owns the front end (screens, fonts,
menu group, localisation, jukebox, animations, navigation history, mode-select globals,
frame production) with plain C++ results and no Godot. `Ps2FrontEndLayout` shrank from
846 to 314 lines and is a translation shim with no game state. Fixed a latent namespace
defect in options.hpp/modeselect.hpp (unqualified `port::frontend::` resolved to
`hp2::port::frontend` once any `hp2::port::*` namespace existed; the old bridge only
compiled because of include order). Deleted as unused: `src/port/runtime/value.hpp`,
`src/platform/services.hpp` with the `platform_api` target, and two uncalled bound
methods. Re-verified: 21/21 CTest, every Godot check unchanged, and nine screens
byte-identical both new-vs-old and against the pre-refactor captures, so the extraction
changed no output.

Third pass, in the sequenced order: (1) the event-tree overlay is native --
`port::runtime::EventTreeView` plus `append_event_tree`, with `render::Quad::texture == 0`
added as a solid-colour rect for the recovered axis-aligned `tLineSegment` connectors;
screen 39 renders byte-for-byte identically to the old host. (2) The front-end behaviour
checks moved from GDScript smokes into `src/tests/frontend_runtime_tests.cpp`, seven groups
under CTest driving the runtime directly with the player's own assets, reproducing the old
counts exactly (resolved=1524 missing=25, 8 Back routes, jukebox 5 then 7). Only the two
checks that genuinely need Godot stayed there. (3) The old host is deleted:
`scripts/frontend.gd` (1170 lines), `scenes/frontend.tscn`, `ps2_event_tree.{hpp,cpp}` and
ten bound methods, plus the whole host-measurement path in the runtime. `project.godot`
boots the native scene; `tools/run_frontend.ps1` drives it. Every bound method and bridge
class that remains has a caller.

Validation: 22/22 CTest; `--native-render-smoke` 41 screens / 1085 quads / 7765 glyphs /
120 textures; `--runtime-smoke`, `main.tscn`, both observer smokes pass; default scene
boots clean; nine screens 0 differing pixels against the pre-deletion native captures.

Open question carried forward: the highlight ghost period. The old `--animation-smoke`
expected a 2 s restart and failed; that failure was confirmed **pre-existing** by stashing
all TASK-0018 work and rebuilding. The native replacement measures instead of asserting â€”
scale 1.7071 at 2.01 s against a 1.0000 target, i.e. the 4 s default `open_menu_animation`
records (E-0016-08). Needs its own task against the original.

Exact next action: move startup/movie/music orchestration and screen selection into
`FrontEndRuntime`, the last front-end decision-making left in GDScript (TASK-0018 Next
steps 1). Nothing is committed.

## TASK-0019 â€” Original function names (completed scoped investigation)

RenderingServer work is deferred at the user's request; TASK-0018 remains incomplete.
Read-only Ghidra inventory classified 7,544 function symbols. Whole-C-string references
and MIPS instruction proof recover five original methods (E-0019-N03), now adopted
in reverse/symbols.csv; see FUNCTION_NAME_RECOVERY.md. No native implementations of
these five exist to rename. Requested 002490D8/00248C60 original identifiers remain
unknown: allocation labels are not type/method proof, and 00248C60 is shared by
entrance and exit lists. Source comments record this port enclosure limitation.
Metadata checks and git diff --check passed; no behavior changed or Verified promotion.
Next naming work: recover computed references for the remaining diagnostic/profiler
strings before adopting more names. TASK-0017 normal submenu paths and Audio/Display
viewport checks passed previously (18/18 CTest plus Godot smoke); remaining settings,
editor/save branches and original equivalence are still incomplete.

## TASK-0017 â€” Submenu Back and overflow (active)

User reports submenu Backspace failure and Audio items drawing below the hint/footer.
Confirmed host gap: onlyOptions24 Back is implemented. Root recovered manager history
strings (0016C4C0/0016B418/0016B438/0016C670) and replaced layout-index history with
class names. Independent recovery underway for per-screen0x11 permission/side effects
and original scrolling viewport, clipping/group offsets/arrows. No blanket permission
or guessed scrolling. Next integrate portable handlers and viewport tests, then actual
Godot traversal and Audio screenshot. Prior TASK-0016 remains preserved.

## TASK-0016 â€” Startup/media/audio/scroll integration (active)

Latest user correction: viewport backdrop is now opaque black, replacing Godot
default gray around movies/title art. Actual DX12 title screenshot inspected;
RuntimeSmoke PASS and Windows export refreshed. Host color preference, no new
PS2 equivalence claim.

Run now targets frontend.tscn. Original EA/NFS intro/title sequence, MV30 runtime decode,
common menu audio and frontend jukebox are integrated. First EA movie cannot be skipped;
second intro can. Title has a strict >4-second activation gate. Skipping does not start
music; natural completion does. English text comes from FE_ENGLISH.BIN. Confirmed hint bindings for Audio/Controller/NameEntry are integrated.

Godot 4.4.1 and binding pin e4b7c25e721ce3435a029087e3917a30aa73f06b are installed.
Actual Windows DX12 capture passed on Intel Arc B390. macOS Metal/LinuxBSD Vulkan defaults
are configured; those platforms are untested. Recovered blueprint state freezes in normal
IN_FRONTEND (2), updating only during LOADING_TRACK_COMMON (3).

User correction: BGM now uses original JBC0x8000-byte channel interleave and
matches the authorized map_tools_ps2 decoder byte-for-byte (E-0016-A06). Movie
audio uses raw stereo ADPCM, mix48000/pitch3762Ã·4096 (E-0016-A07), with0.16-second
video lag. BGM/menu neutral driver pitch is applied separately from PCM metadata.
Video vertical rows now follow original negative-stride decoding (E-0016-V06);
actual DX12 EA/main-menu screenshots were inspected and display upright.

Naming sweep E-0016-N01 completed:298 source-path occurrences,118 nonoverlapping
qualified-name matches. Startup functions moved into fmenus; host decoder remains
port-side. See NAMING_AUDIT.md for unverified naming debt.

Native and Godot builds PASS; both CTest16/16 PASS. UpdatedRuntimeSmoke,
ActivationSmoke, AnimationSmoke PASS. WindowsDebug export PASS; DLLs and license
notices present in ignored out/windows-debug. No staging/commit. Continue
TASK-0016 with original comparison and remaining boot/settings/handler gaps.
Final Windows export runtime smoke PASS/exit0; final EA4-second verbose capture
exited without leak warnings. Earlier transientObjectDB warning remains recorded.

## TASK-0015 â€” Activation and animation integration (active; remaining gaps explicit)

Main-menu Championship/Speed/Options now request registered documents 1/2/24; the first two
write DAT_002FC070=1/0. All 37 FUN_0016ADB0 registrations are in a port-side immutable table,
with class identity retained for shared documents. NameEntry is now resolved to layout 29
(handler 0024DBD8). Main-menu pulse and selection bars are driven every frame; the renderer
reads live position, scale, alpha, text/font and layer. Options exit motion is recovered as
zero-velocity cubic Hermite to x=650, duration 0.05 + original item index*0.05, and transitions
only after completion. Options Back pops history; Main Menu explicitly rejects Back.

Blocked: Display first needs 001EE6C0's runtime settings snapshot, whose three source blocks
are not represented yet. That choice throws before exiting. Destination screen internal
activation/Back handlers, scrolling text/background paths, global constructor bindings and
localisation remain unimplemented. No PCSX2/reference comparison; no Verified promotion.

Validation (2026-10-12): CMake 3.29.2, Ninja 1.12.0, MinGW GCC 13.2.0, Python 3.14.4,
Godot 4.3.stable.official.77dcf97d8; godot-cpp pin fbbf9ec4efd8f1055d00edb8d926eef8ba4c2cce.
`cmake --build --preset native-debug --parallel 4` and `ctest --preset native-debug`: PASS 11/11.
`cmake --build --preset godot-debug --parallel 4` and `ctest --preset godot-debug`: PASS 11/11.
`./tools/run_frontend.ps1 -ActivationSmoke`, `-AnimationSmoke`, `-Smoke`: PASS (41 screens,
1524 resolved resources, 25 missing). Direct Godot `--menu-smoke` also PASS.
Read-only GhidraMCP evidence E-0015-11/12/21 and E-0015-REG-01; server version not captured.
No commit or staging performed; prior uncommitted changes preserved.

Exact next action: read tasks/active/TASK-0015-frontend-activation-animation.md, then trace
DAT_002FC194, DAT_0030CDCC and DAT_0030D49C producers/readers before implementing the Display
snapshot state. Next independent recovery is Cops On/Off Mode Select handlers 002174A0/00217AF8.
Run `./tools/run_frontend.ps1 -Build` to try current implementation.


## TASK-0013 â€” Original front-end actually rendered (not a debug UI)

User rejected the previous deliverable as "the debug screen", asking for the real game front-end
rendered exactly like the PS2. Decoded the game's own screen/widget layout format (chunk
0x80010200: 41 screens of 0x00010210 textured quads and 0x00010212 text widgets, each carrying a
name hash, 24-byte name, layer byte, x/y, scale, texture hash and RGBA with PS2 alpha) and the
bitmap font format (chunk 0x00015010: 32-byte glyph records with atlas UV + advance, plus a
kerning table). Field identification was evidence-driven, not guessed: the texture-hash offset
(+0x50) was found by correlating every 4-byte-aligned offset across all 1549 widget records
against the real TexturePack hash set (507/1015 hits for quads vs 79 for the next best offset),
and the scale semantics were confirmed by TITLE_FULLSCREEN's 512x256 texture at (1.25, 1.88)
resolving to exactly the 640x480 declared in the screen header. Added native Godot-independent
parsers (src/game_core/frontend/fmenus.{hpp,cpp}, named after the original source file
indep\src\fmenus.cpp recovered earlier), a layout bridge (Ps2FrontEndLayout), hash-based texture
resolution with caching (Ps2TextureLoader::load_texture_by_hash), and an immediate-mode Godot
renderer (godot/scripts/frontend.gd) that draws quads with draw_texture_rect and text glyph-by-
glyph from the original font atlases. Result: the real Mode Select main menu (Championship /
Speed / Options with the HP2 logo), Options, Controller Configuration (with PlayStation button
glyphs drawn from the BUTTONSOUT icon font) and the Title Screen (HP2 splash + PRESS START) all
render from the player's own ZZDATA at runtime, with nothing pre-extracted. Headless smoke covers
all 41 screens and resolves 1524 widget resources (25 missing). Strong cross-validation with
TASK-0011: widget name hashes in the layout data are the same literal immediates the ELF's
frontend code uses (0xe0105663/4/5 = POPUP_1/2/3, 0xab671c9a = ANIMATING_MENUITEM inside
FUN_0016b478), and TASK-0011's previously-unexplained FUN_00249a40 is now understood as binding
the real GRID / MASTER_BP_OBJECT / FADING_BP_OBJECT background widgets by name. CTest 8/8; the
three pre-existing scene smokes unchanged; standalone out/windows-debug/port.exe rebuilt and
verified. Open and explicitly not faked: no interaction/animation state yet, text is the authored
placeholder strings (LANGUAGES/FE_*.BIN not parsed), kerning parsed but not applied, and the
screen-name <-> screen-index mapping is still unproven (screen 0 == main menu is a content
inference). Full write-up: docs/FRONTEND_LAYOUT.md; task: tasks/active/TASK-0013-frontend-layout.md.
Next: localised strings, then drive selection/highlight state so the real screens are operable.

## TASK-0012 update â€” Native runtime texture loader; standalone .exe built; original source tree recovered

Three user-directed follow-ups completed in this pass. (1) User required the reversed code to
load everything directly from the original PS2 assets at runtime, not via a manual Python export
step: ported tools/asset_extractor/hp2_tpk_textures.py's decode path (chunk parser + full
Modelulator-derived P8/4bpp RWBuffer decode) verbatim into Godot-independent C++
(src/game_core/assets/elab_chunks.*, hp2_tpk_texture.*), added a runtime ZZDATA-root resolver
(src/platform/ps2_asset_root.*, same env-var/config precedence as tools/ps2_assets.py) and a
Godot bridge (src/platform/godot_bridge/ps2_texture_loader.*, class `Ps2TextureLoader`). Added a
new CTest (`hp2_tpk_texture`) that decodes the same real texture via C++ and Python and asserts
byte-for-byte equality -- passed. frontend_menu_observer.gd now calls
`Ps2TextureLoader.load_texture("FRONTEND/FRONTB.BUN", "LOGO_NFS")` etc. directly at runtime; the
manually-exported PNGs under godot/assets/extracted/ were deleted. (2) User asked to search for
the original PS2 loader code via Ghidra MCP before trusting the ported algorithm: searched for
the TPK chunk tag constants and "TPK"/".tpk"/"eLab" strings as both ELF code and data -- zero
hits, confirming these are pure resource-file data never referenced by name in the ELF; most
likely explanation (not proven) is that real PS2 GS hardware natively samples swizzled/indexed
textures, so the original code probably never does a CPU-side "decode to linear RGBA" at all --
documented as an open item rather than claimed as a recovered original function. (3) User noted
that a `.cpp`/`.hpp` string search would reveal the original source tree: it did, spectacularly --
93 embedded `__FILE__`-style strings recovered the real original directory layout
(`indep\src\fmenus.cpp`, `indep\src\frontendscreens\{selectcar,racesetup,options,
joystickconfiguration}.cpp`, the bWare middleware's own `src\b*.cpp` files) and several confirmed
original class/method names (`World::DoTimestep`, `Race::Update()`, `RaceCoordinator`,
`ReplayManager::HandleSnapshot()`, `TrafficCar::Move()`, `TexturePack`/`TextureAnimPack` --
the last one confirming "TPK" is itself derived from the real original class name, not a
community-invented abbreviation). None of these strings had resolvable code cross-references this
session (likely retail-stripped assert/profiler call sites), so file-to-function attribution
remains unconfirmed per-address; full catalog in new docs/ORIGINAL_SOURCE_TREE.md.
Finally built and ran an actual standalone Windows executable (`out/windows-debug/port.exe` +
`port.pck` + `ps2_port.dll`, via the existing "Windows Debug" export preset, main scene
temporarily pointed at frontend_menu_observer.tscn for the export then reverted) -- launches
independently of the Godot editor/portable runtime, resolves the ZZDATA config, and renders the
same operable, evidence-backed menu closed loop with live-loaded real PS2 textures (NFS logo,
brushed metal panel). Screenshot captured and sent to the user. `out/` is gitignored; nothing
proprietary committed. No Ghidra mutations, no PCSX2 comparison, no Git commit.
Next: locate the actual GAMEFLOW_STATE_IN_FRONTEND sustained update loop (still the top open item
from the prior pass); cross-reference the newly-recovered fmenus.cpp/frontendscreens file names
against specific function addresses if a viable method is found; consider renaming port-side
`HP2_*` labels to align with recovered original class names where confidently attributable.

## TASK-0011 update â€” Central GAMEFLOW_STATE_* dispatcher found; FUN_0016fa80 hypothesis retired

User pointed at an existing, independently-validated local PS2 texture decoder
(D:\dev\eagl-godot\map_tools_ps2) and asked to combine it with continued Ghidra MCP work to
fully reverse the original menu source, explicitly "don't guess yourself." Fixed TASK-0012's
texture decoder by porting that project's validated decode path verbatim (root cause of the prior
artifact: a missing 0x80-byte alignment on the pixel/palette data-pool base offset); FRONTB.BUN
decode success rate went from 210/365 to 357/365 with zero visible artifacts (see
docs/ASSET_PIPELINE.md Â§4.4). Returned to the ELF side and obtained a full decompile of
FUN_0016fa80 (previously assumed to be an unaudited ~465KB function and the likely New Main Menu
update/tick routine) -- it is actually a clean 312-byte one-shot cleanup function; the "oversized
envelope" hypothesis from TASK-0006/0007/0011 is retired as almost certainly a Ghidra
function-record bookkeeping artifact, not real code. Tracing its single caller led to a major
structural discovery: the game's central `GAMEFLOW_STATE_*` state-machine dispatcher
(`FUN_001680d8`=request-transition, `FUN_001680e8`=per-frame pump, already known from TASK-0010 as
a main-loop StateDispatch target but previously uninterpreted) and its function-pointer table at
ELF 0x002faea4, whose rows point to ORIGINAL ASCII enum-name strings recovered verbatim from the
binary: GAMEFLOW_STATE_LOADING_FRONTEND, _UNLOADING_FRONTEND, _IN_FRONTEND,
_LOADING_TRACK_COMMON, _LOADING_TRACK_SPECIFIC, _RACING, _UNLOADING_TRACK_COMMON (row order
inferred, not re-verified), _EXIT_DEMO_DISC. This connects to TASK-0011's prior evidence:
GAMEFLOW_STATE_UNLOADING_FRONTEND's exit callback (0x001687a8, decompiled) calls FUN_0016f950
(the confirmed attract-sequence gate leading to "New Main Menu"); GAMEFLOW_STATE_IN_FRONTEND's
enter callback (0x00168830, decompiled) calls FUN_0016fa80 then unconditionally requests
GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC -- an unresolved contradiction with "IN_FRONTEND" sounding
like a sustained browse-the-menu state (its update slot is empty), flagged as the top open
question. 4 new/updated function records (E-0011-17 superseded/corrected, E-0011-18..22); 0x0016fa80's
record rewritten in place rather than left stale. Full write-up: docs/FRONTEND_MENU.md Â§8. No
Ghidra mutations; no PCSX2 comparison; no Git commit.
Next: resolve the IN_FRONTEND contradiction (decompile 0x00168db8/0x001690a0/etc. to see whether
gameplay ever returns to IN_FRONTEND, or whether the sustained menu-browsing loop lives entirely
inside one state's update callback rather than in this dispatcher), then the remaining open items
in docs/FRONTEND_MENU.md's updated priority list.

## TASK-0012 â€” PS2 original asset pipeline (active; container + P8/RGBA decode delivered)

User pointed at an authorized local PS2 disc dump (D:\ps2_game\GAME\ZZDATA) and asked for a
non-hardcoded global config entry point, plus reverse engineering that can actually render the
original UI matching real PS2 behavior, with file/function naming reflecting the recovered
structure. Delivered: `tools/ps2_assets.py` as the single global ZZDATA-path resolver (env var
`HP2_PS2_ASSET_ROOT` then gitignored `config/ps2_assets.local.json`; tracked
`config/ps2_assets.example.json` template). Reverse engineered and validated (byte-exact full
file consumption) a generic "eLab" nested-chunk container format shared by FRONTEND/FRONTB.BUN
(14.4MB, 1760 chunks) and GLOBAL/HUDTEX.BIN (1.5MB, 9 chunks) â€” `tools/asset_extractor/
elab_chunks.py`. Found real original frontend UI text in FRONTB.BUN including developer
placeholder/joke strings ("Phil is a loser, so I have to make this fake menu item") proving
unmodified shipped data, plus real labels (PRESS START, Championship, Options, Credits,
Controller, Language Select, Setup, Race Type) and `UwE*`-prefixed widget type names â€” not yet
cross-referenced to specific screens. Identified and validated (cross-checked against three
independent open-source community references: xan1242/xnfstpktool, PS2HomeDeveloper/
ps2-tim2-tool, leeao/PS2Textures, all cited in docs/ASSET_PIPELINE.md) the HP2 TPK texture
container's 164-byte `AncientTextureInfo` header and implemented P8 (8-bit indexed, PSMT8
swizzle + CSM1 CLUT reorder + PS2 alpha range) and RGBA32 pixel decode in
`tools/asset_extractor/hp2_tpk_textures.py`. Decoded 89/89 HUDTEX.BIN and 210/365 FRONTB.BUN
texture headers with zero failures (remainder skipped: unsupported 4bpp/DXT, not guessed).
Visually confirmed multiple decoded textures are clearly recognizable real game art (tachometer
gauges, car badges including a legible BMW roundel, a full `DISPLAYSETUP_BG` road scene, and a
`PURSUIT_TREE_BG` event-tree map) â€” sent to the user. All decoded images and chunk dumps are
gitignored (`tools/asset_extractor/output/`); no proprietary bytes committed. Open: a fine
speckle artifact remains in every P8 decode, most likely authentic PS2-era dithering but not
confirmed; 4bpp/DXT/swizzled-RGBA decode not implemented; the widget/layout record format (which
texture/text belongs to which named screen) is completely unparsed, so no Godot rendering of
real original layout exists yet â€” explicitly do not treat the decoded images as "the rendered
menu," only as validated raw material toward it. Full report: docs/ASSET_PIPELINE.md. Registered
analyzed asset file hashes in reverse/assets.csv. No Git commit (left to the user, per prior note
in TASK-0011).
Next: parse LANGUAGES/FE_ENGLISH.BIN for authoritative menu text, then reverse the UwEMENUITEM_*
widget record layout, then resolve the P8 artifact (ideally against a real PCSX2 screenshot),
then implement 4bpp/DXT decode â€” only after that attempt an actual layout-accurate Godot render.

## TASK-0011 â€” Front-end menu minimal closed loop (active; scoped completion)

User asked for the "original front-end flow minimal closed loop": confirm a main menu and its
adjacent pages, trace NewMainMenu creation/update/input-callback/page-switching, restore
navigation state in the native core, and show an operable Godot UI, menu closed loop first.
Confirmed via read-only GhidraMCP: string "New Main Menu" at ELF 0x002b2f00; CONFIRMED edge
TitleScreen -> New Main Menu (TitleScreen's instance handler FUN_0016eef8, message==1, calling
FUN_0014cc78->FUN_0016c4c0). Manager-level screen switch/find/history push-pop/reset fully
decompiled (FUN_0014cc78/0016c4c0/0016c8c8/0016b418/0016b438/0016b460). MumboJumboScreen
(FUN_0016f810) confirmed as a sibling registered screen class; its edge to Title/MainMenu is
NOT confirmed. The long-flagged oversized FUN_0016fa80 envelope (0x0016fa80..0x001e3dbf, ~465KB,
previously noted in TASK-0006/0007) routes several of its opening state values directly to
"New Main Menu" -- strong candidate for New Main Menu's own update/tick function, still
unaudited (TASK-0003 remains the right next step for it). New Main Menu's own class
constructor/dispatcher was not located; input wiring from raw joypad events
(JOY_EVENT_MENU_SELECT/CANCEL/BACK/HELP strings found, not traced to a call site) to
FUN_0016eef8's message dispatch was not traced. 16 new reverse/functions/.../frontend/ records
(E-0011-01..16) plus a reclassified/updated 0016fa80 record (E-0011-17, moved from unknown/ to
frontend/, Unidentified->Identified, confidence low->medium). reverse/symbols.csv updated.
Native `hp2::frontend::ScreenManager` (src/game_core/frontend/screen_manager.cpp) implements the
confirmed manager-level API as a structural projection (name registry, current screen, 12-slot
history with push/pop, reset); 9 new unit tests. Godot bridge `FrontEndMenuObserver` +
operable scene `godot/scenes/frontend_menu_observer.tscn` (buttons for the confirmed edge, an
explicitly-labelled unconfirmed sibling switch, Back, and Reset; live history/registry display;
evidence footer). Native CTest 7/7 and bridge CTest 7/7 passed (no regressions in the five
pre-existing tests); Godot headless import clean; new `--frontend-menu-smoke` passed
(FRONTEND_MENU_OBSERVER_SMOKE_OK); prior `--observer-smoke` and `--smoke-test` still pass
unchanged. Two real rendered screenshots captured and visually checked
(local/frontend-menu-observer.png, local/frontend-menu-observer-after-press-start.png, both
1380x760, git-ignored). Symlinked the user's existing `reverse-engineer-anything` (REA) skill
from `~/.agents/skills/` into `~/.claude/skills/` at the user's request so Claude Code can load
it; REA's own tool routing has no Ghidra/MIPS-R5900 provider (IDA-only for native targets), so
actual decompilation continued through the existing Ghidra MCP server as in prior sessions.
No Ghidra mutations, no Verified promotion, no PCSX2/original comparison, no Git commit (the
user committed the pre-existing scaffold independently mid-session; this task's new files are
uncommitted). Full report: docs/FRONTEND_MENU.md.
Next: TASK-0003-style boundary/ABI audit of FUN_0016fa80 (highest-value next step -- it is the
best lead for New Main Menu's real update logic and menu-item content); then trace
FUN_0016eef8's raw-input source; then locate New Main Menu's own construction site.

## TASK-0010 â€” Main-loop structural shell completed

Mainloop 0x0012F378 mapped: 61 callsites / 39 distinct targets, five startup sites,
12 guards and 19 nested World/controller/entity/mover connections. EngineShell
preserves ordered branches and saved s2/f20 operands; strict mode stops on missing
contracts, observation mode labels fixtures. Waiting resumes only the original tail.
Separate Godot observer supports six scenarios, pause/single-step/reset/boundary,
startup/world tabs; old main scene and PortCore APIs preserved.
Native and bridge CTest: 6/6 each. New observer headless smoke and prior demo smoke
passed; actual rendered screenshot inspected (local/main-loop-observer.png).
Full evidence validator checks 51 live function records, metadata/hash stability,
all 19 nested connection PCs and three vtable targets. No Ghidra changes/Verified
promotion or Git commit. Applied REA skill workflow; tools were existing Ghidra MCP.
Report, architecture graphs and extension example: docs/MAIN_LOOP_STRUCTURE.md.
Open: ./tools/run_main_loop_observer.ps1; -Build rebuilds; -Smoke checks headlessly.
Next: resume TASK-0009 with World object/partition/entity ABI and data contracts,
then recover/register a tested module handler. Original execution oracle, clock,
state enum and complete behavior remain unresolved. TASK-0005 unchanged.

## TASK-0009 â€” Whole-binary recovery active

User requested every function, correct names and classes; only the stripped ELF
is available. Full inventory: reverse/recovery/TASK-0009-functions.csv, 7,544 rows.
First pass exports 7,516 pseudocode bodies and records 28 boundary blockers;
49 class texts / 118 method-text hints await address proof. Two police local bodies
now have native code and partial class storage views. Native CTest 4/4 and five
export safety tests passed. No Verified functions or trustworthy original names.
Move control-flow candidate 0x001A1030â€“0x001A1BCB produces a full separate
instruction translation; candidate output retains 27 other placeholders. No live
Ghidra names/types/boundaries changed; previous outputs/config remain preserved.
Report and reproduction: docs/FULL_RECOVERY.md. Full restoration remains incomplete.
Next: R5900/pcode/ABI review of Move and tail-call entry 0x001A0E88 before any
journaled live repair; trace class hints and recover external-call contracts.
Existing TASK-0005 remains independently active. No commit made.

## TASK-0008 â€” Police AI map completed

Read-only MCP inspection links mode 4 to CopDriver and mode 5 to CopMover.
Think candidate 0x001A2110 has 2,432 defined bytes and existing C++ translation.
Move candidate 0x001A1030 has a one-byte body and TODO output; boundary repair
remains required. Eight police function records added with original names unknown;
proposed purpose names were not applied. Report: docs/POLICE_AI_MAP.md.
Private evidence/pseudocode: ps2_decomp/police-ai/. Eight live metadata matches,
unchanged ELF hash and seven non-placeholder C++ syntax probes passed; Move blocked.
No native behavior implemented or Verified status assigned. TASK-0005 untouched.
Next police action: scoped boundary/ABI audit of 0x001A1030 and undefined vtable
entry 0x001A0E88, then typed helper recovery with original behavior comparison.

## TASK-0007 â€” Function naming completed

Screened all 7,544 functions; reviewed all 20 pilots. Applied 16 evidence-backed
HP2_ purpose names and recorded 11 automatic jump aliases (27 entries total).
No trustworthy original developer names recovered; original identifiers remain unknown.
Ghidra saved; ELF SHA-256, function envelopes, namespaces and calling conventions
unchanged. Names/map/private regenerated C++ agree. Report: docs/FUNCTION_NAMING.md.
Private output: ps2_decomp/recompiled-named/ (7,516 translations + unchanged 28
placeholders). Eleven synthetic safety tests and 20 C++ syntax probes passed.
No full generated link/boot/original comparison; no Verified promotion.
Name rollback mapping/command and source-type limitations are recorded in the report.
Existing TASK-0005 remains independent and active; no implementation changes made there.
Next reverse action: TASK-0003 must audit damaged camera/FlightAreas entries,
the oversized 0x0016FA80 envelope and allocation return ABI before further naming.

## TASK-0006 â€” PS2Recomp trial completed

User-requested PS2Recomp/Ghidra generation trial completed on 2026-10-10.
See `docs/PS2RECOMP_TRIAL.md` for commands, tool/dependency identities and evidence.
Private outputs: `ps2_decomp/recompiled-triage/`: 7,516 translated functions,
28 explicit unresolved placeholders, registration and headers (~132 MiB).
Baseline full run was terminated near 6 GB memory while generating the oversized
`0x0016FA80` body envelope; its partial output is retained separately.
Tool build, artifact assertions and five representative C++ syntax probes passed.
Full generated build/link and original behavior comparison were not performed;
no equivalence or Verified status is claimed. Existing TASK-0005 remains active
with its own independent scope; this trial did not modify its implementation.
Next action: TASK-0003 should audit the `0x0016FA80` discontiguous body/envelope
alongside its damaged entries before unrestricted generation. Ghidra MCP at
127.0.0.1:8080 and the original authorized ELF are needed to re-export.

## Scope and milestones

M0 is complete for Windows x86-64. TASK-0002 static workload assessment is complete
for the current Ghidra snapshot; no game reconstruction or asset conversion was
performed. M1 is in progress: read-only MCP/R5900 connectivity and ELF identity
are established, but boundary/ABI quality and broad identification are unfinished.
M2â€“M6 have not been achieved. Current active tasks: TASK-0005 (independent infrastructure),
TASK-0009 (full-binary recovery), TASK-0011 (front-end menu; closed-loop scope delivered,
broader menu-content recovery remains open), TASK-0012 (PS2 asset pipeline; container + P8/RGBA
texture decode delivered, layout/text semantic decode and remaining pixel formats remain open).
Completed tasks: TASK-0000, TASK-0002, TASK-0006, TASK-0007, TASK-0008 and TASK-0010.
Next reverse task: TASK-0003 boundary/ABI audit (now prioritizing the FUN_0016fa80 envelope
surfaced by TASK-0011), then TASK-0004 original comparison setup and the scoped pilot backlog.

## Assessment snapshot â€” TASK-0002

Report: `docs/assessment/BINARY_REPORT.md`. Binary: D:/ps2_game/SLUS_203.62,
SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Region USA/NTSC inferred; exact executable revision unknown (boot VER 1.00).
Ghidra project hp2_ps2, R5900/default compiler. Existing annotations preserved.

- 7,544 unique function entries and 17,162 distinct static call relationships.
- 400,582 decoded .text instructions; complete loaded .text matched the disk ELF.
- 663 indirect jalr sites, 190 other indirect jr sites, 26 empty CFGs.
- 21 unique decompile outputs returned, 0 failed probes, 7,523 unprobed;
  global decompile success/failure totals remain unknown. Three probes contain warnings.
- 20 varied pilot selections and individual backlog tasks; none executed or verified.
- Six requested reports plus metadata/evidence exports, a reproducible summarizer,
  a risk register and explicit uncalibrated workload scenarios.

Blocked future work: trustworthy bodies/ABI for damaged entries; emulator/original
oracle setup; timed representative recovery; VU/IOP, layout, asset and adapter scope.
PCSX2 was not located in scoped tool checks; availability elsewhere is unknown.
Planning scenarios are ~12k / 79k / 480k person-hours under stated assumptions,
not a delivery forecast. See WORKLOAD_ESTIMATE.md before interpreting these totals.

Assessment validation: summary script assertions passed for population, uniqueness,
full .text byte coverage/disk agreement and pilot selection. All metadata exports
are separate from ignored local raw listings/decompilation. Final CSV/task/model
consistency checks are recorded in the completed assessment task.

## Delivered infrastructure

- Three-layer C++17 architecture; core and provisional service contracts contain no Godot dependency.
- Optional RefCounted GDExtension wrapper, sample arithmetic, smoke scene, and CTest.
- Session protocols, architecture/decision/roadmap/development documentation.
- Task lifecycle folders, templates, and next environment/provenance task.
- Empty real symbol/binary indexes; function/layout templates and symbolic dummy examples.
- Git initialized; pinned godot-cpp submodule; restricted-input/build/secret ignores.
- Existing `task.txt` and `eval.txt` preserved; no Git commit or remote/Issue created.

## Environment observed at initialization (updates below)

| Tool | Observation |
|---|---|
| CMake | 3.29.2, C:/Strawberry/c/bin/cmake.exe |
| Ninja | 1.12.0 |
| Compiler | MinGW-w64 GCC 13.2.0, x86-64 UCRT |
| Git | 2.54.0.windows.1; repository initialized here |
| Python | 3.14.4 via `py -3`; WindowsApps `python` alias failed |
| Java | Temurin OpenJDK 21.0.11 |
| Ghidra | 12.0.4 installation present |
| R5900 extension | ghidra-emotionengine-reloaded present; host compatibility metadata 12.0.4; operation untested |
| GhidraMCP | Extension description 0.7.0-pre15, compatibility 12.0.4; read-only connectivity confirmed in TASK-0002 |
| Existing Godot | 4.7.2.stable.official.ed1daf0bf in WinGet package directory, outside PATH |
| Pinned Godot | Portable 4.3.stable.official.77dcf97d8 downloaded to ignored local/tools/godot-4.3/ |
| godot-cpp | Submodule fbbf9ec4efd8f1055d00edb8d926eef8ba4c2cce |
| PCSX2 | Not located/validated in this initialization; not a build prerequisite |

The portable engine came from the official `godotengine/godot-builds` release
asset `4.3-stable/Godot_v4.3-stable_win64.exe.zip`. No global installation changed.
Downloaded archive SHA-256:
`8f2c75b734bd956027ae3ca92c41f78b5d5a255dacc0f20e4e3c523c545ad410`.
This is a local artifact identity, not an independently checked upstream checksum.

## Initialization validation ledger â€” 2026-10-09

| Check | Result | Evidence |
|---|---|---|
| Required directory structure | Passed | PowerShell Test-Path check of all 18 requested directories and 14 requested named files |
| Native configuration | Passed | `cmake --preset native-debug` exited 0 |
| Native compile | Passed | `cmake --build --preset native-debug` exited 0; core static library and test executable linked |
| Native unit tests | Passed | `ctest --preset native-debug`: 1/1 passed; arithmetic normal/zero/negative/int32 boundary sums |
| Bridge configuration | Passed | `cmake --preset godot-debug -DPython3_EXECUTABLE=<py-discovered-path>` exited 0 |
| Bridge compilation | Passed | `cmake --build --preset godot-debug --parallel 4` exited 0; ps2_port.dll linked |
| Bridge-build core tests | Passed | `ctest --preset godot-debug`: 1/1 passed |
| Godot import | Passed after fix | Portable 4.3 `--headless --path godot --editor --import` exited 0 with no errors |
| Godot scene smoke | Passed | Portable 4.3 `--headless --path godot -- --smoke-test` exited 0, printed `PORT_SMOKE_OK: 42` |
| DLL runtime dependencies | Passed | `objdump -p godot/bin/ps2_port.dll` shows Windows system imports; no external MinGW runtime DLLs |
| Ignore checks | Passed | `git check-ignore` matched 11 representative build, private asset, binary, Godot, Ghidra, local-config and secret paths |
| Original behavior comparison | Not performed | Explicitly outside initialization scope |

No unavailable tool blocks native builds. Python is usable with an explicit
interpreter. Assessment established ELF identity and read-only Ghidra/MCP access;
exact release matching and a validated original comparison setup remain needed.

First host import failed with Windows loader error 126 because the initial DLL
still imported `libwinpthread-1.dll`. Godot returned exit 0 despite those errors;
that attempt was treated as failed. Corrected MinGW link options to statically
link its runtimes, relinked, inspected imports, then reran import and scene smoke
successfully. No unresolved initialization validation failures remain.
Only the scaffold call is tested; visual rendering, original gameplay, MSVC,
Linux, and the installed 4.7.2 runtime were not validated.

## TASK-0014 front-end menu interaction (2026-10-12)

The real front-end screens are now operable, driven by the menu runtime translated out of the
ELF rather than by a Godot UI. Menu items are the screen's own MENUITEM widgets, navigation
wraps and skips disabled entries as the original does, the orange highlight and grey disabled
colours are the values stored in the binary, menu sounds resolve to the original bSound event
names, and keyboard input is mapped onto the recovered JOY_EVENT_* ids for PC debugging.
18 of the 41 screens now have their original names recovered by hash match (screen 0 is
NEW_MAIN_MENU), which confirms TASK-0013's content-based inference.

AGENT.md was added as the binding naming/evidence contract: identifiers are recovered-verbatim
or address-anchored, never invented descriptive names for recovered code, and an unimplemented
recovered path raises hp2::NotImplemented instead of degrading silently. Pop-up handling, the
FMenus Highlight Animation motion and FE_*.BIN localisation are explicitly in that state.

Validation: `ctest --preset native-debug` and `--preset godot-debug` 9/9 (new `fmenus_menu`
test, 16 groups); `./tools/run_frontend.ps1 -MenuSmoke` prints FRONTEND_MENU_SMOKE_OK;
`-Smoke` still prints `FRONTEND_SMOKE_OK: screens=41 resolved=1524 missing=25`. No PCSX2 or
original-hardware comparison was performed, so nothing from this task is Verified.

Exact next action: `Read AGENT.md, docs/FRONTEND_MENU_INTERACTION.md and
tasks/active/TASK-0014-frontend-menu-interaction.md, then trace the joypad-to-JOY_EVENT
generation path (ELF strings "GenerateJoyEvents()" 0x002B01C8, "EventNodeSlotPool" 0x002B01F0)
read-only through GhidraMCP; it is the last unrecovered link between hardware input and the
menu runtime.`

## Continuation

Front-end menu next step: "Read docs/PROGRESS.md, docs/FRONTEND_MENU.md and
tasks/active/TASK-0011-frontend-menu-closed-loop.md, then audit the FUN_0016fa80 envelope
(0x0016fa80..0x001e3dbf) boundary before deciding whether to decompile it; it is the strongest
lead for New Main Menu's real update/tick logic and eventual menu-item content. Start
read-only, preserve existing annotations and record evidence before any scoped repair."
(Superseded in part: the FUN_0016fa80 envelope was audited and retired in TASK-0011's later
pass, and the menu runtime it was a lead for is now recovered under TASK-0014.)

Original TASK-0003 continuation (still applicable to the broader boundary-audit backlog item,
now informed by the FUN_0016fa80 priority above): "Read AGENTS.md, docs/PROGRESS.md and
docs/assessment/BINARY_REPORT.md, then work on tasks/backlog/TASK-0003-boundary-abi-audit.md.
Audit the one-byte bodies at 0x0019a4b8 and 0x001ebd20 and the shared return/ABI contracts.
Start read-only, preserve existing annotations and record evidence before any scoped repair.
Do not start full game implementation."

Original request-file SHA-256 values after scaffold creation:

- task.txt: 15244A82D71F2D1C85BBF4DC7189937B7F0ADFD63E5E7A45C1D7A63BE0E1F75B
- eval.txt: 5B2E413E9B4153C2DF6E4341417D0943063A04734C0BA0E75A551263C10EC2FA


## 2026-10-14 â€” TASK-0033 native ImGui and original vehicle base material

Implemented the single native ImGui session for viewer panels/console/FPS, post-frame
actions, first-click capture and console-off Debug operation. Catalog retains all64 original
car rows and235 skin rows; ID-based loading uses original resource paths (including shared
HP resources), and color changes reuse original geometry then rebind textures/materials/UVs.
CarRenderInfo slices retain receiver evidence; shared address-named functions generate
material blocks and initialized frontend lights. VU reflection/base-RGBA and source normals
are projected into a host shader with GS texture/TEST/filter state and raw palette alpha.

Full available-skin probe:228 success,7 unavailable DIABLO/DIABLOHP associations,0 failures.
Source-over range audit proves current static data As<=127; the general As>128 fixture is
an explicit backend capability guard, not a required current static rendering gap.
Native36/36, Godot Debug39/39, console-off Debug host3/3 passed. Release functionality
passed; progress generator stale after concurrent task documentation edits is being fixed.
Compatibility GPU captures of player/police/traffic samples were inspected; captures stay
ignored. No original-output comparison and no Verified promotion. Full original render
entry, partial-view cone/clipping and additional post-base routes remain bounded gaps.


TASK-0033 final validation addendum: Release progress table failure was generated-document
staleness plus new punctuation written with a Windows default encoding; normalized the
affected inserted punctuation to UTF-8, regenerated and rechecked successfully. Release
host3/3 passed. Durable Release --all-skins probe again passed228/7-unavailable/0-failed.
Restored Debug/consoleON;14 relevant native/host/template checks passed30.42s. Optional
real GPU material oracle passed five RGB/alpha cases after correcting the host cubic
ALBEDO decoder/power encoder roundtrip. These are host checks, never Verified.


TASK-0033 visual-QA correction (T07): black bonnet triangles exposed incorrect GS ZMASK
state indexing. Re-read packet pointer assignments confirmed independent TEST bit0 and
depth bit1 (+0x50 plus phase gate). Corrected metadata/helper and native state-matrix
tests; a real GPU overlap test proves paint occludes a later source-over surface. Fixed
window capture is clean. The actual attribute CPU/GPU fixture also matches190,190,211
with the correct texture74-enabled flag4; an earlier scratch CPU false flag explained
the supposed5-byte discrepancy. Headless smoke now uses separate ImGui settings so
its small viewport cannot overwrite normal viewer layout/history.


TASK-0033 final matrix: native36/36 (32.20s), Godot Debug39/39 (55.48s), optional
GPU-enabled Debug40/40 (49.32s), console-off Debug GPU+hosts4/4 (24.75s), Release39/39
(8.61s). Final missing-car/color diagnostic regression passed20.17s and responsive
button-layout checks passed2/2. Default Debug/consoleON/GPU-optionOFF restored. Final
Release --all-skins again passed228 available/7 unavailable/0 failed, with supported
texture state and depth metadata. Corrected-window player/police/traffic captures
were inspected and remain ignored.

A provisional validation invocation continued after a duplicate local test-variable
compile error and therefore ran against the previous configuration DLL; its results
were rejected. The duplicate was removed, build-success gating was added to commands,
and the final sequential Release/Debug runs above are the accepted results. Early
progress-table/UTF-8 staleness was also regenerated and rechecked. No failing feature
check is being hidden behind the final matrix. Whole render-entry and partial-view
recovery remain active beyond this bounded static base slice; no Verified promotion.

## TASK-0033 compact viewer and loading follow-up â€” 2026-10-14

Contained all shared ImGui root windows and tooltips across drag/settings/resize; replaced
car list/color combo with a searchable selector and material-derived clickable paint tiles.
Advanced diagnostics start collapsed. Near-only solid filtering precedes VIF decode and
binding; Show LODs lazily compares original per-LOD assemblies side by side with labels.
A native worker, metadata probes, root-scoped LRU caches, staged cooperative GPU uploads,
immutable mesh/shader reuse and repaint node reuse remove blocking asset processing from UI.
Latest tickets win; cancellation cannot resurrect cleared/closed content. Worker shutdown
happens before Windows DLL unload. E-0033-B16 keeps original swatch fields unknown.

Native full36/36, Godot Debug full39/39, real GPU1/1, console-off Debug hosts3/3 and Release
full39/39 passed. Final Debug/on restored and focused8/8 passed. Full resource probe remains
228 loaded /7 unavailable /0 failed. Cold166ms versus warm1-2ms CPU is a cache comparison,
not a historical before/after claim; GPU uploads are spread across frames. Original appearance
is not Verified. Detailed commands, initial failures/fixes and timings are in TASK-0033.

## TASK-0034 UV, continuous preview and shared materials â€” 2026-10-14

E-0034-U01 identifies the legacy indexed decoder's reversed row output, with original
UV accessor arithmetic intact. Restore row-zero image order at the vehicle adapter;
retain raw GS alpha and frontend decoding behavior. Original Corvette and CL55 captures
were inspected. CarViewer navigation buttons removed; continuous mouse-adjustable
Preview Mode is explicitly port-side presentation.

E-0034-M01 rereads original light/material dispatch. A bounded CarRenderInfo render-context
slice feeds shared Ps2VehicleRenderer, retaining native binding/pose/LOD/VU/GS calculations.
The debug class is a thin request-policy wrapper. Rigid world transforms are checked.
All shader sources/specialization text move to src/platform/shaders with CMake embedding.
The user's follow-up adds Wheel Blur (high speed) / --wheel-blur using original four-wheel
WHEEL_BLUR overlays and original alpha scale90 (E-0034-W01), with toggle/texture regressions
and a window capture. Speed/time activation remains unknown; this is a forced debug sample.

Accepted checks: native36/36 (26.40s), console-off Debug hosts+GPU4/4 (7.09s), Release39/39
(8.57s) plus the common-backend real GPU oracle, Debug/on full40/40 (33.08s). Task file
records exact commands/tool versions and rejected stale-DLL/configuration runs, fixed
wrapper/test failures and private capture paths. Configuration assertions guard host smoke;
force relink when changing build directories sharing godot/bin/ps2_port.dll. No original
comparison or Verified promotion. Full reflection/environment/floor mirror and live-car
wheel activation remain TASK-0033 continuation work; default Debug/on is restored.

## 2026-10-14 - TASK-0035 wheels, glass environment and mirror binding

Preserved prior dirty work. Read-only Ghidra traced001C0D78 tire spin versus independent
blur matrices,00120EA8 float opacity and00219A60 phase integration. Added native bounded
matrix/strength helpers; shared backend updates existing nodes/uniforms. User follow-up adds
Wheel speed beneath Wheel Blur, range-150..150 rad/s with explicit port default80. Zero and
reverse input, reset and console coexistence are tested; animation does not reload assets.

Traced actual CARSELECTENVMAP frontend upload, original environment look-at and VU second
material block ST/alpha. Shared shader now adds original additive context2 with depth writes
off and depth testing retained. Original texture is loaded at runtime and cached; shaders
stay in platform/shaders. VU LOI latency transcription error was caught by independent4-ray
instruction fixtures and corrected; no tolerance conceals it. Native exact-bit and real GPU
ST/modulate/additive/alpha/depth checks pass. Live game capture/composition remains open.

User's red mirrors on silver McLaren were traced to omitted original mirror texture patch
bindings. Both mirror solids now receive original selected SKINn/SKINnB constructor pairs.
Actual assets are tested across all MCLAREN skins; real silver screenshot inspected, both
mirrors match paint. No per-model color override. Evidence W01/W02/R01/R02/R03/M01 and
address records/symbols updated. No Verified claim or proprietary files committed.

Validation: native36/36, Debug with GPU40/40, console-off Debug host/GPU4/4, Release39/39
plus real common-renderer GPU tests passed. Strengthened alpha/depth checks also passed
against Release. Toolchain CMake3.29.2/Ninja1.12.0/GCC13.2.0/Python3.14.4/Godot4.4.1,
Intel Arc B390 Compatibility. A mapped older DLL was moved into ignored local storage to
preserve the user's window; every build-tree switch forces DLL relink. Final Debug/on is
restored; full tests and generation/whitespace checks recorded in TASK-0035. Task remains
active pending original comparison and unresolved original live inputs; no commit created.

## TASK-0036 - Complete vehicle upload beyond host surface capacity

Completed a scoped host-only fix without altering native recovered behavior. The original
one-solid/one-ArrayMesh adapter exceeded the pinned Godot256-surface limit for ELISE and
ELISEHP. Added complete ordered mesh batch vectors, identity child batches beneath one
native part root, stable native-part metadata, independent source draw-count diagnostics,
union bounds and LOD spacing, full material/reflection/LOD-copy/wireframe handling, and
atomic batched-root replacement on async commit. Whole GPU batches remain cached.

New car_surface_batches regression scans62 available catalog IDs, finds the two oversized
entries (407 draw surfaces across near parts), and verifies part/surface/triangle/bounds
coverage, exact partition boundaries, cache resource identity, async upload, visibility,
LOD copies and cleanup. A test initially selected a scene child by native part array index;
warm-node reuse invalidates that ordering. Corrected selection to explicit part metadata,
then all coverage checks passed. The failure was not hidden by tolerances or skipped.

Debug full suite with real material GPU tests41/41 passed45.30s; Release full suite40/40
passed7.17s plus windowed material/reflection GPU checks. Real Compatibility corpus run
passed all62 available IDs, and an ELISE window capture was inspected with Loaded status
and no surface rejection. Proprietary capture remains ignored local/task0036. No Ghidra
mutation, no new original behavior assertions and no Verified promotion. Restored Debug/on
GPU-off and forced relink; focused host/generated checks6/6 passed10.58s, whitespace and
generated checks passed. Restricted untracked binary candidates: none. No commit created.

## 2026-10-15 - TASK-0037 packet helper naming traceback

Read-only Ghidra checked six original accessor symbols/source, the nearby getter/setter
cluster, direct consumers and upstream callback/duplication contexts. Confirmed packet
count, component-address and value-access roles. No recovered original class/function
spelling or source file was found: all six queried names are DEFAULT FUN labels.
eStripEntry text and duplicated-buffer allocation labels were investigated and explicitly
rejected as sufficient ownership/name evidence. No invented descriptive rename was made.

Clarified Type_0011BF98.hpp as a port-created address grouping, with0011BF98 not a confirmed
constructor and native container signatures not original ABI. Added detailed binary-specific
naming record, six-address provenance CSV, per-function cross-links and symbol evidence IDs.
TASK-0037 is complete as a naming audit; original spellings remain unresolved. No behavior
or verification status changed, no Ghidra mutation and no proprietary evidence committed.
Native geometry target rebuilt; existing geometry/metadata checks3/3 passed0.84s;
generated checks and git diff --check passed.


## 2026-10-17 - TASK-0038 DrivingLab foundation and bounded driving evidence

Implemented the independent Debug driving_lab scene, drivinglab queue/console entry,
Godot-free tool session/config/input/snapshot/tape contracts, Ps2DrivingLab bridge, native
ImGui controls and shared renderer absolute body/four-wheel publications. Input capture,
F8 and focus loss pause and release controls. Publications reject malformed/non-finite/
singular states before committing; the preview animator is rejected in driving-pose mode.
Brake roots now carry the same port-side wheel membership metadata as tires/blur roots.
Original static CarViewer behavior remains covered by existing regressions.

The complete driving objective was NOT achieved: Run/Step reaches00137100 and reports
hp2::NotImplemented without inventing a rest state/timestep/contact force. The visual
floor has no contact provider. Original scheduling, initialization, full drivetrain/wheel/
body dependency closure and synchronized native comparisons remain open. TASK-0038 stays
active. This is not a playable driving implementation or a completed plan.

Read-only Ghidra captures audit00137100/00137D90/00138E08/00138F38 and dependent fields.
Two finite scalar leaves00137C88 and001A4E88 are translated with instruction-derived tests.
The00152070 tail distinguishes selected output copies from a missing route that leaves
its vector's fourth lane untouched; do not replace that distinction with a guessed plane.
Stable evidence E-0038-001..006, records/symbols and a private-output hash inventory are
saved. Original ELF SHA remains7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
executable revision remains unknown. No Ghidra database mutation or Verified promotion.

After the user opened PCSX2 and requested no Computer Use plugin, used PINE and lightweight
window capture/native keys. PCSX2 v2.8.2 ran SLUS-20362 (CRC1d2818af, reported version1.00).
A JAGUAR original replay-context PhysicsCar exposed the expected pointer/table chain.
Captured18 bounded RAM windows twice per paused snapshot with exact equality, before and
after a roughly3-second non-atomic Cross/throttle sample. Input+0x214 became1.0 and wheel/
motion values changed. No original/native numeric equivalence or exact frame alignment
was established. No save slot was written. The temporary keyboard mapping was restored:
Pad1 compares equal to the backed-up SDL configuration; two temporary profiles removed.
PCSX2 remains paused. Raw data/backups/captures remain ignored under local/task0038.

Validation: native full37/37; Godot Debug full42/42 before final scoped additions, then
final driving/renderer/console/pipeline subset 10/10 and last UI/invalid-numeric subset3/3.
Final native driving/model subset2/2; Python read-only capture contracts 4/4. Console-off
Debug host subset 4/4; Release host subset2/2 and direct DrivingLab rejected with expected
exit1. No full Release rerun or GPU numeric oracle rerun. Window capture inspected a
complete original911TURBO and usable panel after correcting inherited tiny sizing and
isolating smoke settings. Generators/diff checks passed. Exact commands, tool versions,
limits and next addresses are in tasks/active/TASK-0038-driving-lab.md. Final build restored
Debug console ON / GPU tests OFF; no Git commit created.

Continuation: TASK-0038, recover00137100 initialization and00137D90/00138E08 through00138F38;
finish00152070 and surface table contracts before wiring the synthetic contact provider.
For stable original layout observations, pause the matching game and run
`py -3 -m tools.driving_oracle`. Read docs/DRIVING_LAB.md before extending the backend.


## TASK-0038 continuation - 2026-10-17: native dependency recovery

Used the REA investigation workflow with read-only Ghidra and independent init,
solver and runtime questions. Preserved the existing uncommitted DrivingLab work.
Added32 address-specific projections:12 initialization functions,8 drivetrain/
selector entries and 12 solver/math/suspension entries, with three CTest targets.
E-0038-I01..I13/D01..D07/S01..S11 link per-function evidence and closure maps.
Important corrections preserve untouched constructor bytes, mutable global W,
uncapped cross inputs, multiply grouping and ADDIU32 upper-bound wrap. No complete
production constructor, tire/contact closure or Verified promotion.

Connected supplied custom PCSX2 build d75a0ad through JSON/TCP21512. Source audit
shows step is a PC+4 breakpoint, so bounded owned entry/return breakpoints are the
comparison route. Current World/replay slots are zero; natural00137C88 breakpoint
timed out after3seconds. Left original paused at00111F0C/cycles4032240318 with
owned breakpoint removed; no settings/input/save edits. Paused register captures
work and retain raw data only in ignored local/.

Native build/full40/40 passed(41.71s); final added leaf rebuilt, new suite subset
3/3 passed. Godot Debug build passed; full45/46 passed(52.12s), sole progress-table
failure while index updates were ongoing; regenerate/check and failed-test rerun
1/1 passed. All46 have passing results; no final full rerun claimed. Python12/12
passed; init/drivetrain evidence recaptured66/16 queries with matching ELF SHA.
Initial address-argument compile error was corrected and rebuilt. Release,
console-off and windowed checks not repeated because no host flow changed.

Continue TASK-0038 from the init/solver maps linked in DRIVING_LAB.md: resolve
conversion/table and engine evaluation, tire force dependencies and constructor/
body virtual dispatch; recover road query before a contact provider. Enter an
active original driving scene for a synchronized leaf oracle; run
`py -3 -m tools.pcsx2_mcp` for paused captures. Existing Run/Step still reports
00137100. No commit created.


## 2026-10-17 - TASK-0038 minimum drivable native loop completed

The accepted drivable-first continuation now connects the supported911TURBO synthetic
horizontal-plane fixture end to end. Native constructor/reset and eleven-component graph,
engine/transmission tables and states, steering/tire/trig/contact, aero/body derivatives,
five-pass force solve, integration, publication and recovered scheduler are installed.
One port-owned SHA-bound ELF/data Memory image is authoritative; candidate copies commit
only finite complete states. Godot publishes actual body/four-wheel poses through the
shared renderer. No Godot physics or invented acceleration/grip was introduced.

Critical integration findings were recorded rather than tuned away: controller+A0 writes
required0xB0 port storage and unmapped guards; original zero-wheel DIV.S needs narrow EE
signed finite saturation instead of host NaN; Race4+A758zero is running context while a
separate mode 4 diagnostic path remains excluded; original event writers identify+218
footbrake and+21C handbrake. E-I34/E-E12..14 and B/T records retain the evidence and retired
assumptions.140numeric driving records are indexed, no new duplicate symbols or Verified
promotion. Whole class templates stay throwing for their untranslated ABI callers.

Original-data native checks cover2s rest, sustained4s acceleration, opposite turns from
matching prefixes,4s footbrake to zero and held rest. Source-speed observations0/11.7256/
32.307/0 are qualitative port acceptance only.180mixed-input reset snapshots and132 Session
record/replay snapshots match every published field exactly under varied render pacing,
pause and single-step. Finite extreme steering reaches an explicit CVT.W.S gap; failures
leave later31 complete snapshots equal to an untouched backend. Invalid elapsed arguments
and wrong-SHA synthetic input are rejected. No original bytes were copied into Git.

Godot asset smoke executes540 native driving steps, applies all real poses and replays them
exactly before checking independent renderer invalid-publication/preview/cleanup contracts.
Windowed pinned Godot run exited0, saved native-driving.png with save0 under ignored local/
and was inspected for complete car, wheel publication, stopped telemetry and usable panel.
Camera/floor/input scaling are explicit tool choices; original race camera/lighting and
road broadphase are not claimed. Native material1/roughness is sourced from original data.

Validation: final native47/47 passed48.12s; Godot Debug53/53 passed63.61s. Earlier full run
failed only stale generated class inventory/progress, then generators and final full run
passed. Console-off Debug host subset 4/4 passed 6.30s. Release extension build passed,
direct DrivingLab entry printed disabled and exited1, host gating subset2/2 passed0.77s.
Full Release suite/GPU numeric oracle not repeated. Debug console ON/GPU OFF restored and
DLL removed/relinked to avoid testing the shared Release artifact. Final build has no new
compiler warnings. Python emulator/capture contracts12/12; auxiliary class metadata7/7.
Class/progress generators and diff checks passed. Tool versions and exact commands are in
the active task; no commit created and pre-existing user changes preserved.

TASK-0038 remains active solely for its explicitly deferred synchronized original comparison.
No PCSX2 runtime state/settings edits this implementation slice; original World is absent
and emulator left paused. Continue with `py -3 -m tools.pcsx2_mcp` after entering an original
driving scene and pausing, then match owned function entry/return inputs and contacts before
any equivalence claim. CustomPC+4 stepping is insufficient across branches/delay slots.
For immediate use launch DrivingLab in Debug; W/S/A/D and F5/F 6/F7/R now drive the native loop.


## 2026-10-17 - TASK-0038 user corrections: one CarViewer and original wheel bases

Merged the separate drive scene into the existing CarViewer ImGui Test drive mode; same
selected car/skin, renderer and camera survive entry/exit. Return to preview restores cached
preview bindings/settings/camera. Removed all newly added F5/F 6/F7 simulation shortcuts;
existing camera R/F stay. No independent driving_lab.tscn remains; compatibility commands
route to CarViewer. Native session/controller is an embedded RefCounted tool adapter.

E-0038-U01 follows001A5D60 basis writes and0013B5B0 original axis inversion: positive native
steering means left/source+Y. Corrected host A-D and negative stickX; physics is unchanged.
E-0038-W01 follows00120EF8 render submission: raw001C0D78 wheel matrices require its existing
render-basis composition. Factored the same recovered composition for preview and driving,
with distinct tire/blur and brake handling. The missing stage caused the reported right-side
orientation. No guessed half-turn was added to the physics. An existing preview brake group
consuming blur matrices was corrected to its original brake matrix.

Native affected tests 4/4 passed16.46s; Godot Debug full53/53 passed 55.52s. Real ImGui clicks
queue Test drive/Return to preview only on release and retain the same window identity.
Host acceptance covers one scene/model/camera, selected skin, noF5/F 6/F7, original left/right
input, focus/F8 isolation,540-step native trajectory and exact replay, return to preview and
camera restoration. Windowed preview/rest/completed-drive captures inspected under ignored
local/task0038/unified-*.png. Original numerical equivalence remains unverified. Console-off/
Release and GPU numeric oracle not repeated this correction; existing gating is retained.

Initial checks found/fixed missing autoload flag and preview animation against a retained
old driving model during async return. Build DLL lock resolved by closing standalone running
frontend before relink; no editor, save or emulator modifications. Exact commands/limitations
are in TASK-0038. Concurrent unrelated frontend/frame changes preserved. No commit created.


## 2026-10-17 - TASK-0038 Test drive WASD follow-up

User found no WASD response after entering Test drive. Two tool-layer causes were confirmed:
mode entry left Session paused, and CarViewer's combined mouse-hover capture gate discarded
keyboard events while the cursor remained over ImGui. Entry now starts running immediately;
keyboard events respect actual keyboard capture separately from mouse/camera capture.
F8/focus/explicit Pause still clears input and requires Run to resume. Physics unchanged.

Prior host acceptance directly injected held/native input and therefore missed the normal
Godot keyboard delivery path. New Input.parse_input_event regression sends W/A/S/D with the
cursor over ImGui, verifies actual native throttle and movement, steering/brake values and
release clearing, then all existing mode/trajectory/replay/focus/preview checks. The harness
sets1280x800 so its hover coordinates are inside the panel. Final affected host 4/4 passed
5.89s; windowed equivalent passed and saved ignored wasd-input-fixed.png. Exact commands and
intermediate fixture corrections are in TASK-0038. No native rebuild/full-suite/Release/GPU
numeric rerun for the script-only fix. Concurrent TASK-0039 work preserved; no commit or
Verified promotion.

## TASK-0039 - Frontend rendering investigation and bounded fixes (2026-10-17)

User reported event-tree digit placement and requested the shared frontend UI be checked using
Ghidra MCP. Preserved the preexisting TASK-0038/user modifications. Read binding rules/docs,
created the independent active task, and used read-only MCP inspect/listing/memory against
SLUS_203.62 SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
39 initial MCP captures expanded to41 with the clip helper; raw exports/hashes stay ignored.

Confirmed header28 was a glyph-table offset, not font height.0013FFA8 derives max glyph height;
001F7BF0 calls00140930 to subtract half scaled event-digit height. Added current-glyph pair
slices, right-bit priority, advance-only measurement, actual draw kerning, original helper
scale-squared width/unscaled button advances and fixed cells. Serialized BUTTONSOUT has zero
pairs with a stale offset; tightened bounds initially rejected it, then a corpus/visual check
caught the missing glyph and a zero-pair regression restored it. No bearing was invented.

Independent quad evidence covers001428F0/00143078, four colours, all UVs, signed layers/exact
visibility and original component clipping. Extended the port packet/Godot polygon backend,
updated parser and generic alpha dispatch. Rotation remains a throwing gap. A containing-clip
regression corrected guards to compare the original right/bottom clip bounds. Resource census:
41 documents,1015 quads,534 texts,255 gradients,18 nondefault UVs, no authored nonzero mode2
rotation. Full dynamic paths and original pixel equivalence are still unresolved.

Validation: CMake3.29.2/GCC13.2.0/Python3.14.4/Godot 4.4.1, pinned godot-cpp confirmed. Native
build and full48/48 passed47.20s; after final relink the focused frontend_runtime/frame_builder/
frontend_quad/progress_tables subset 4/4 passed16.35s, and direct new regressions passed. Earlier
Godot build passed and41-document native-render smoke passed; routed Ultimate Racer event1/3
and Options captures inspected under ignored local/task0039. Earlier Godot full52/54 failed a
stale tracker check (regenerated) and an unrelated preexisting asset_viewer.gd:399 type inference
error causing car_render_host timeout. No unrelated viewer script edit was made.

Final Godot rerun/smoke was invalidated by an external parallel 4 CMake/Ninja rebuild cleaning
shared Godot outputs/DLL while tests ran. Missing executable/extension failures are a contention
result; owned hung host-test processes were canceled, external build left running. An earlier
preview DLL lock was resolved by graceful preview close; later native relink lock cleared after
its test exited. No final host full-pass claim. See TASK-0039 for exact commands and limitations.

Updated per-function records/symbol index, corrected historical FRONTEND_LAYOUT claims, added
FRONTEND_RENDER_AUDIT, decision/architecture notes and regenerated progress. No commit, staging,
Ghidra mutation or proprietary material added. Continue TASK-0039 after the external build
finishes: build and smoke sequentially; then obtain matched original Event1/3/Options captures,
FCR31/sampling evidence and special/rotated path dependencies. Keep the task active.


## TASK-0040 - Global symbol provenance audit (2026-10-17)

Preserved prior dirty work. Added reproducible source/live-Ghidra ledger for 562
Sub_/Type_ identifiers, existing class associations and 12 deeper driving entry
checks. 357 have class attribution, 51 constructor associations; no new exact
method spelling was established. One analyst-defined name was explicitly excluded
as original evidence; 169 missing symbol objects remain unresolved. Refreshed dead
PhysicsCar method-label xrefs and bounded 002C7090 runtime region. No behavior,
Ghidra database, class layout, or Verified status changed. See TASK-0040 report
for evidence IDs, search boundaries and reproduction prerequisites.


## 2026-10-17 - TASK-0038 direct third-person Test drive redesign completed

Implemented the accepted simplified workflow: actual Test drive click validates the first
native step, starts a rear third-person camera/captured mouse and drives immediately with
WASD. The UI has only Return to CarViewer and hints; manual simulation/debug controls were
removed from the mode, while native Session test instrumentation stays intact. Camera uses
confirmed host+Z vehicle forward, bounds-center target and explicit tool-side distance/
pitch/mouse/recenter constants. No recovered physics or wheel matrix algorithm changed.

The controller separates Starting/Driving/Suspended/Error. Camera release no longer pauses
simulation. Focus/F8 suspend temporarily, clear keys and auto-resume; a persistent elapsed
discard prevents the notification-resume path from integrating time spent suspended. Actual
keyboard capture supplies zero controls without permanent pause. Esc releases pointer,
world click captures, and pointer intent survives temporary suspension. Terminal native/
render failures keep the last complete pose, show the error and cannot auto-retry. Startup
failure preserves preview. Return restores model/skin/settings/camera/preview wheel bindings.

Host tests now click real ImGui actions and send normal keyboard/relative mouse events,
including sustained vehicle turn/rear-camera tracking, wrapped idle recenter, panel hover,
keyboard capture, Esc/click, focus/F8, elapsed discard, injected startup/runtime gaps and
repeated return/re-entry. Headless capture intent is separate from actual OS capture.
Added tools/test_drive_window.py: dedicated owned foreground-PID-checked Win32 SendInput,
owned focus probe, real22-check workflow and ignored JSON/log/screenshot artifacts. A short
owned topmost flag isolates the test from other preview windows. Actual W/A/D/S, mouse,
automatic resume and repeat entry passed; rear-view/minimal-UI screenshot inspected.

Validation: final Godot Debug54/54 passed52.01s; affected native 4/4 passed13.79s; two complete
successful22-check real-window runs (including final elapsed fix). Earlier OS harness runs
had premature exit/focus isolation issues; these were fixed with lifetime/isolation guards,
not relaxed product assertions. Full native/Release/console-off/GPU numeric oracle not
repeated; native algorithms untouched and existing Debug renderer/scene/gating regressions
passed. Exact commands/tool versions/limits are in TASK-0038; final DLL is Debug console ON.

Generator/diff/restricted-file checks completed after docs. All raw test artifacts remain
ignored; concurrent frontend/naming work preserved, no commit or Verified promotion.
TASK-0038 remains active only for deferred synchronized original numerical comparison.


## TASK-0041 - Symbol-rich PS2 reference recovery (2026-10-17)

User-authorized NFSHP2.ELF/MAP supplied a complete named reference: 8715 functions,
1284 named aggregate types, and rich STABS field/method tokens. Recorded SHA-bound
reference metadata separately from retail. Captured initialized retail sections
through read-only Ghidra and checked equality with the authorized retail ELF.

Normalized signatures provide 4410 unique candidates but do not grant names:
address-shaped integers, four-byte data collisions and circular call matching
were found and pinned by tests. Conservative adoption combines 614 whole-image
unique exact closed leaves,49 uniquely constrained full vtables and 20 independently
reviewed bridges. The resulting ledger names 940 retail addresses.699 previously
unindexed addresses now have Identified records; existing behavior status is unchanged.

Renamed 55 same-class shell methods,6 principal native projection functions and 4
packet projections. Recovered 10 former Type_ declarations; split the eleventh
packet grouping into eStripData and eStripDataE, with their actual member spellings.
Corrected allocator label CDBFileSys to exact cDBFileSys in source. Preserved native
interfaces, body logic, source placement and all pre-existing user edits. Counterevidence:
Move, SetDesiredGear, tAnimatedBlurb and LanguageOptionsScreenBase differ across
builds, so debug layouts/behavior were never copied. Constructors/destructors and
shared/overloaded placeholder conflicts remain address entries with exact names
in the catalog. No Ghidra modifications or Verified promotion.

The initial 562-identifier source audit now has 191 supported names,175 candidates
and 196 unresolved/ambiguous addresses. Tokens may remain for explicit port
projections or inherited/unrepresentable entries even when an original name is known.
Read DEBUG_REFERENCE_SYMBOLS.md, task0041-source-coverage.csv and the reviewed bridge
ledger for exact scope, prerequisites, validation and next recovery entry points.

TASK-0041 final validation: native/Godot-debug builds passed; native51/51 tests
passed43.10s and targeted Godot15/15 passed3.87s. Matcher/type/table tests13+4+3,
class-generator tests8 and generated/diff checks passed. Earlier source-path ledger
class spelling mismatch was regenerated and retested. No original equivalence or
commit claimed. Exact commands and continuation are in completed TASK-0041.


## TASK-0042 - Debug call chains and original source ownership (2026-10-17)

Reviewed the full remaining 175 candidate +196 unresolved audit using the
SHA-bound Debug ELF, live Ghidra imported symbols/decompilation/references and
retail caller/table context. Captured live Debug .text matches every authorized
ELF byte. Finite independent proof chains confirm64+21 additional names; total
adopted retail names1025. Historical562-source denominator now276adopted,
111candidate-only,175unresolved/ambiguous (including8folded cases). Changed
bodies and layout differences remain explicit counterevidence to Debug copying.

Exact ECOFF procedure/file markers recover original CPP ownership and selected
HPP spellings.193file mappings include same-TU merging and Car's final4method
merge into indep/src/car.cpp. Derived-shell attributions were corrected into
CarMover/CarDriver/PhysicsVehicle/CarController/cDBFilesBase and other actual
declaring scopes.203classes/578member attributions are now generated. Exact
source evidence places DriveTrain in engine.cpp, PhysicsSystem in
componentphysics.cpp and both packet classes in ecstasy/estripdatae.cpp. Unknown
companion headers stay explicitly port-side. No inferred native inheritance.

The14address-grouped driving CPPs were split into19partitions;151exported
functions and12anonymous helper chunks/47helpers were checked against original
snapshots. Token/hash tests preserve arithmetic, literals and throwing paths
while allowing68proved identifier replacements and local helper lookup. Original
constructors/destructors retain address-callable placeholders where native
lifetime semantics are unknown. Corrected overload binding uses path+signature
instead of one short-name-to-address dictionary. Legacy0041write is guarded.

Validation and exact commands are recorded in TASK-0042. No PS2 comparison,
Verified promotion, database mutation or commit is claimed. Continue with the
remaining bounded disposition rows and an additional independent caller/table
anchor; do not adopt normalized uniqueness by itself.

TASK-0042 final validation: native/Godot builds passed; Godot CTest60/60
passed59.91s. Native initial52/54passed48.06s, with two stale generated-metadata
checks refreshed and rerun2/2passed0.78s. Python discovery37/37passed. Source
bodies and original1597symbol-row statuses preserved;5newIdentifiedrows only.
Class/progress/diff checks passed. No restricted raw files or commit added.


## TASK-0043 - Full member mapping and identifier cleanup (2026-10-17)

Rechecked all 1,025 supported Retail correspondences: 822 class-method rows now
have complete dispositions, with 650 added whole-entry throwing templates, 169
existing throwing entries and three explicitly unrepresentable nested/template
entries. The generator represents 372 classes and 1,203 member attributions.
4,999 unmapped Debug methods are listed per declaring class without invented
Retail addresses. Removed 25 duplicate derived-class sole-throw aliases only
after pinning the original body and confirming the canonical same-address gap.

Restored 42 projected callable identifiers, one EventInfo struct name and 11
field names. Field identity is independently anchored by live both-version
getters and SHA-bound Debug STABS; identical offsets elsewhere were preserved.
Static host carriers separate conflicting original scopes without copying ABI.
Constructors/destructors/operators and unknown overload gap signatures retain
address-callable placeholders. TitleStringId_001F96B8 remains an extracted host
selector: the Debug constructor contains its branch inline, so the containing
constructor cannot supply a name for the extracted helper.

All 1,602 previous symbol rows/statuses remain. Existing production bodies are
checked against fixed Git revision 19a8de9 with only approved identifier changes;
projection hashes derive from that baseline, not the working code. See
CLASS_MAPPING_COMPLETION.md, TASK-0043 coverage/entity catalogues and the task
record for exact validation and remaining constraints. No behavior or Verified
status is promoted.

TASK-0043 final validation: native CTest 61/61 passed (57.82s); Godot CTest
67/67 passed (73.58s); both builds passed. Python tests 54 + 7 passed.
Whole-production-body preservation passed, including pinned removal of 25
duplicate throwing gaps; generated test changes are separate. Repeated normal
generation left all 2,637 checked files byte-identical. Final checks passed;
no raw/restricted artifacts or commit were added.


## 2026-10-17 - TASK-0044 Debug driving audit and canonical implementation

The user approved Debug NFSHP2.ELF behavior as the driving target, independently hashed
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b. This explicitly
supersedes Retail-only driving authority; other systems and historical evidence remain
Retail. Followed REA and project evidence/naming rules with read-only Ghidra. Initial Git
status was clean. No database edits, proprietary files, emulator/save/settings changes or
commit were made. Debug date/revision remains independently unknown.

Audited all 151 pre-existing driving projections with complete dispositions and 207 unique
Debug address records (185 Implemented, 22 Decompiled, none Verified). Canonical original
PhysicsCar, Car/CarState, Engine/DriveTrain, suspension/steering, wheel, body, component,
timer, table/aero and math source bodies now own the supported algorithms. Test drive
enters PhysicsCar::Move directly; compatibility interfaces forward, and the old composite
solver is removed. Host Memory/ObjectView, encoded dispatch, synthetic plane, raw input and
snapshot/camera/display policies remain explicit external boundaries. Full World/RealDriver,
tracks/collision, damage/effects and other excluded original branches are not claimed recovered.

Important corrections were substantive Debug differences: shift-table offsets/history-free
clutch, derivative table 003077C8, damping headers distinct from samples, Car field offsets,
mutable yaw globals, Debug math guards/homogeneous addend and full wheel rotation helpers.
Initial incorrect table-header and retained Retail derivative operands caused actual backend
failures and were corrected from instructions, with regressions. Final native Debug trajectory
reached 11.7256 then 32.307 m/s and braked to zero; opposite steering and 132-snapshot exact
replay passed. See DEBUG_DRIVING_AUDIT.md and reverse/task0044 for per-function details,
provenance, counterevidence and limitations.

Generator changes preserve foreign binary symbol rows on address collisions, retain historical
Retail attributions while suppressing superseded owner gaps, and do not overwrite canonical
bodies. The fixed historical TASK-0043 preservation scope remains intact. A separate reviewed
249-before/383-after migration pin set rejects unreviewed body/signature changes without an
accept-current mode. Negative mutation/collision tests passed. Progress separates Debug counts
from the Retail whole-image denominator. New records are UTF-8, no restricted files in the
untracked inventory; seven old Retail duplicate-address index rows are preserved unchanged.

Validation, exact commands: `cmake --preset native-debug`,
`cmake --build --preset native-debug -j6`, `ctest --preset native-debug --output-on-failure`
passed **64/64 in 142.26 seconds**. `cmake --preset godot-debug` and
`cmake --build --preset godot-debug -j6` passed.
`ctest --preset godot-debug -R 'driving_.*host|debug_console$|debug_viewers_host|car_render_host' --output-on-failure`
passed **5/5 in 12.17 seconds**; `ctest --preset godot-debug -R 'debug_console_host$' --output-on-failure`
passed **1/1 in 0.54 seconds**. `py -3 reverse/scripts/generate_class_templates.py --check`,
`py -3 reverse/scripts/complete_class_members.py`,
`py -3 reverse/scripts/audit_task0044_migration.py` and `git diff --check` passed.
`py -3 reverse/scripts/generate_progress.py` refreshed the tracker; its CTest passed.
Earlier failures in class-fixture binary identity/historical supersession and stale progress
were fixed before the final full pass. Complete-members uses default checking, not --check.

`py -3 tools/test_drive_window.py --output local/task0044/window-idle` passed **23** owned
Win32 SendInput checks. Actual Test drive click, W movement, A/D/S/releases, rear/orbit/idle
camera, Esc/capture, F8 and real focus suspend/resume, exit and repeated entry all passed.
The rear-view screenshot was inspected. Earlier desktop activity changed foreground/pointer
and caused guarded failures; user provided an idle desktop and the unchanged camera behavior
passed. A tentative capture-frame camera change was withdrawn after insufficient causal
evidence. Only harness diagnostics, actual focus readiness and target-PID guards remain.
No UI/native acceptance assertion was relaxed. Runtime reports/screenshots stay ignored.

Tool versions: GCC13.2.0/MinGW, CMake3.29.2, Ninja1.12.0, Python3.14.4, Godot4.4.1
49a5bc7b6; existing pinned godot-cpp. Window: Intel Arc B390/OpenGL driver32.0.101.9150.
Original certification command `py -3 -m tools.oracle probe --output local/task0044/pcsx2-probe.json`
returned blocked/timed out, with no active runnable Debug scene/process. Supplied PCSX2
package presence is not a runnable synchronized scenario. Its README requires launching
PCSX2/loading a game; the Debug directory contains ELF/MAP only. No original trace comparison
was performed. Existing Retail oracle hooks are invalid for Debug. The approved plan permits
this explicit blocker report while closing migration; numerical certification stays separate
and no Verified status changes. Continue with exact Debug ELF, live owned Move00137D18 /
component hooks and explicit Debug oracle mapping, equal initial/input/time/contact state,
then per-field errors and coverage. Full reproducible continuation is in the audit and task.


## 2026-10-17 - TASK-0045 grip, terrain and Test drive material tests

Implemented the approved Debug driving continuation without changing ordinary supported tire
arithmetic. The exact NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b
was rehashed. REA independent scopes covered tire/data evidence, source review and tool UI;
root integrated native settings/session/tests. All existing dirty TASK0044 work was preserved.
Read-only Ghidra audit covers33 terrain rows,31 bounded functions and original types/globals,
load/slip/friction/rolling/noise and boost/catchup/yaw sources. See reverse/task0045 and
DRIVING_GRIP.md. Five new functions are Decompiled only; Debug index212=185Implemented+
27Decompiled, none Verified. The sole original-body change corrects a diagnostic literal to
Debug VRSQRT001A2AC4. CAR32 noise projection guard is a known excluded-case issue, not original
behavior; full topology/water/walls/effects/EE exceptions remain outside the five-surface scope.

ROAD/DIRT/GRASS/SAND/SNOW feed the same original Wheel chain via per-wheel synthetic world
queries. Uniform and nativeX25/50/75/100 Zones use original coefficients/noise without tuning.
Surface choices commit with complete candidate snapshots and record/replay with inputs/time.
Read-only stored diagnostics preserve their original production phases and raw units; no grip
score/recomputed force is added. World-anchored color/label descriptors are explicitly tools.
The expanded panel uses supported pinned ImGui1.77 Columns, with fixed Return and independently
scrolling body. An actual recapture bug was fixed: ImGui now receives mouse-button releases
even when the world press captured the pointer; camera motion remains separate. Test failures
were retained until this root cause was established, not resolved by removing assertions.
Screenshot review corrected the actual expand coordinate and reduced/moved labels off the car.

Validation: `cmake --preset native-debug`; `cmake --build --preset native-debug --target
 driving_surface_tests driving_backend_tests -j6`; direct `build/native-debug/driving_surface_tests.exe`
passed. `cmake --build --preset native-debug --target driving_tire_tests -j6` and
`ctest --preset native-debug -R '^driving_tire$' --output-on-failure` passed1/1 in0.27s.
`cmake --preset godot-debug`; `cmake --build --preset godot-debug -j6` passed after correcting
legacy ImGui API use. `ctest --preset godot-debug --output-on-failure` ran72 checks in141.93s:
70passed, one old exact-assignment source assertion failed and new surface test skipped on
working directory. Corrected the assertion to require the SHA-bound load/catalogue/commit,
and registered repository working directory. Then `cmake --preset godot-debug` and
`ctest --preset godot-debug -R '^(driving_surfaces|debug_driving_single_chain)$' --output-on-failure`
passed2/2 in16.51s, including surface16.28s. Thus every distinct check has a passing final
result; a single uninterrupted72/72 run is not claimed. All six Godot host/batch tests and
other native/template/evidence tests passed in the full run. No production change was needed
for those two registration/assertion corrections.

`py -3 tools/test_drive_window.py --surfaces --output local/task0045/window-final` passed31
actual Win32 checks. Root inspected surfaces.png: expanded four contact/terrain rows, small
lane-side label, usable Return header and complete car; other columns scroll horizontally.
Uniform five selectors and all-zone/mixed-wheel traversal, WASD, camera, Esc/F8/focus recovery,
Return/re-entry/cleanup passed. Private screenshots/JSON/logs remain ignored. Earlier focus
interference was blocked by owned-window guards; user supplied an idle desktop.

`py -3 reverse/scripts/generate_class_templates.py --check`,
`py -3 reverse/scripts/complete_class_members.py`, chained source review and negative tests
passed. Frozen TASK0044 review was not refreshed; strict TASK0045 amendment pins11-before/
17-after identities and rejects deletion/baseline refresh/unreviewed changes. Source checks
are not original equivalence. `py -3 reverse/scripts/generate_progress.py` refreshes tables/
log contents. Final diff/link/UTF-8/restricted-extension checks passed; no proprietary binary,
raw captures, assets or runtime screenshots enter Git; no commit created.

Versions: GCC13.2.0 MinGW, CMake3.29.2, Ninja1.12.0, Python3.14.4, Godot4.4.1
49a5bc7b6; pinned godot-cpp/ImGui retained. Window OpenGL Intel Arc B390 driver32.0.101.9150.
`py -3 -m tools.oracle probe --output local/task0045/pcsx2-probe.json` returned blocked /
timed out, no runnable live Debug scenario/process. No emulator/save/settings changed.
The approved explicit-blocker alternative closes implementation acceptance only; numerical
certification remains separate. Next: runnable exact Debug scene, explicit Debug oracle hooks
at Move00137D18/components, equal initial/input/time/material/contact, then per-field errors
and coverage. Retail hooks cannot certify Debug and no Verified status was assigned.

## TASK-0046 - player transmission controls and original skids (2026-10-17)

REA independent scopes recovered original RealDriver control and required services,
the skid/update/render chain, and the Godot semantic input/render boundary. Root
integrated original REAL_CONTROL burnout, appended strict source reviews and performed
builds, regressions and owned-window verification. Existing TASK0044/45 dirty work and
frozen pins were preserved. No Ghidra database edits or commit.

Implemented default automatic/manual Q/E, manual neutral, original S brake/reverse,
Space handbrake, actual gear/RPM/native speed HUD and original textured skid rendering.
Godot actions and hints resolve one project InputMap; queued requests are acknowledged
by completed steps. No independent physics/skid algorithm or new function keys.

Counterevidence drove concrete corrections: REAL_CONTROL was a required burnout
branch; steering conversion performs integer DIV after CVT; skid intensity is a lower
bound; skid bbox expansion uses segment travel; Debug RenderingOrder/GlossMap choose
draw contexts instead of Retail bWriteZ. Independent U/V clamp/repeat was needed by
the actual original skid textures. Native pointer/layout/ABI boundaries remain explicit.

Native builds passed. Final targeted native mover/player/skid3/3 passed0.55s; integrated
surface/player/skid/backend4/4 previously passed41.52s. Full Godot suite75/76 passed
118.25s; old14-owner assertion was updated to check exact14 originals plus a disjoint
7-owner successor and its1/1 CTest passed3.78s. All76 distinct checks passed, not a
single uninterrupted76/76 run. A late DLL lock from a separate same-project frontend
was resolved by graceful closure of that exact runtime, then successful rebuild.

Actual OS second and final runs each passed69 checks. The first popup sample moved
after ImGui auto-fit; stable observed widget centers fixed the harness. A screenshot
rerun also encountered a partial JSON observer read, now retried. Neither correction
changed physics. Final local/task0046/window-final contains reports and privately held
controls-skids.png, visually inspected by root for original textured wheel trails and
Gear1/RPM1250/HandbrakeON. Full material/zone/camera/focus/F8/return acceptance passed.

The chained source review approves25 old/81 new identities without refreshing any
TASK0044/45 pins. Generator/version/identifier/evidence/progress checks pass. Debug
index258 unique (226 Implemented/32 Decompiled); no Verified entries. The original
probe `py -3 -m tools.oracle probe --output local/task0046/pcsx2-probe.json` remains
blocked/timed out. Continue TASK0038 with a runnable exact Debug scene and synchronized
equal-state/input/time/contact component traces. Whole race/world, full DMA/GS and
degenerate numerical domains remain unclaimed. Usage and exact limits: PLAYER_DRIVING.md
and completed TASK0046. No proprietary captures/assets entered tracked sources.


## TASK-0047 - Alpha archive extraction, 2026-10-17

Extracted all 405 records into the user-selected external GAME directory, decoded 81 COMP members and reread every output for SHA-256 validation (3251095824 bytes). Reference-tree hash candidates name 313 records; 92 unresolved paths and 3 repeated paths are retained explicitly. See reverse/structures/alpha-zzdata-task0047.md and tasks/completed/TASK-0047-alpha-zzdata-extraction.md. Two synthetic tests pass on Python 3.14.4; no game implementation, proprietary Git additions, commit or Verified promotion.


## TASK-0048 - Real TrackViewer and track physics

Recovered all 30 Retail catalogue entries, original resource reuse, geometry, textures,
mips, world lights, topology and forward starting markers. TrackViewer supports
asynchronous load/switch/clear, free camera, vehicle selection and real-track driving.
Four native wheel contacts publish elevation, normal, terrain and existing grip fields;
render materials and physics terrain are separate bindings. Synthetic driving remains.

Original class-owned topology, path, VClip, collision-body and contact operations feed
the existing Debug simulation. Full NativeBackend wall/corner impacts, unsupported-step
rollback, all real starts and replay passed. Actual Compatibility renderer windows were
inspected. Evidence and scoped source reviews are in reverse/task0048; use
TRACK_VIEWER.md for setup and explicit rendering/physics boundaries. No original
hardware numerical/visual comparison was available, so no Verified promotion.

Final TASK-0048 regression:84/84 registered tests PASS in124.21 seconds, all30 external
asset scenes PASS, strict successor audits and generated-document checks PASS.


## TASK-0049 - Track orientation and contact debug display

The indexed legacy decoder reverses rows before its optional flip; the new track-only
adapter cancels that conversion to retain raw-ST row order, matching linear32. Base
textures and original mip slices share that adapter; shared decoder and cars stay intact.
Asymmetric4/8/32-bit tests preserve raw RGBA together.

TrackViewer now provides independent, default-off Render Collision and Render Material
controls. Cached native diagnostic geometry uses actual wheel-contact polygon planes,
original XY perimeters and terrain IDs, preserving all road layers. Noncoplanar fourth
quad vertices are projected only in the diagnostic output. Five tool colors match Test
drive; other known IDs use stable hues and unknowns use magenta. Both modes share one fill.
The ImGui legend reports names/IDs and polygon counts. Loading/failure disables controls;
preferences survive scene replacement, and driving hides/restores effective overlays.

All30 contact-geometry scans and four Compatibility-window views passed. Initial test
expectations were corrected for original flags1 height ordering and collision-only fill;
clear now publishes an explicit empty native snapshot. TASK0049 successor audit reviews
8before/16after identities while preserving every historical JSON pin. See completed
TASK0049 for final build/regression results; no original numerical certification claimed.

TASK0049 final: native/GDExtension build PASS;87/87 full regression PASS in187.51sec;
all30 asset scenes and contact-geometry scans PASS; five actual-window views inspected.


## TASK-0050 - Track/car proportions and outside-camera projection

Investigated the all-track proportion report with independent Retail geometry,
scenery transform/culling bounds, Debug driving and actual Godot world matrices.
Elise mesh/CV/wheel dimensions agree; raw road vertices match packed topology and
all1388 inspected ordinary curve-sign instances across30 tracks fit original scene
bounds. Runtime car and track scale are both unit. No asset rescaling justified.

Found a confirmed shared projection-policy discrepancy: the tool used60-degree
vertical FOV, while original DriveCameraMover OUTSIDE uses0x32DC horizontal.
Corrected the projection and restore both FOV/aspect policy onreturn. Actual engine
matrix tests cover4:3,16:9 and portrait, failed entry and driving return. The original
follow/spring/obstruction camera remains unrecovered; no claim that this projection
alone explains all reported visual differences. See reverse/task0050 and TASK0050
for exact checks and final regression. No C++body changes or historical pin changes.

## TASK-0052 checkpoint (2026-10-18, incomplete)

Closed the TASK0051 baseline96/98 plus two corrected regressions and the isolated
Release windowed runtime check. Historical source-review pins remain frozen.
TASK0052 adds a shared native visual-frame description, public session Effects
gates, original camera ranges and CalculateH, indexed preparation and changed-only
GPU parameter submission. Main/capture rendering shares Ps2TrackRenderer's actual
submit function. Native sky, weather query, Sun/shadow helpers, postprocess stages
and AcidFX slices retain separate Retail/Debug evidence and original ownership.

Important counterevidence: Roadshine C3 and fused blend tests agree with the
translated original math, so no arbitrary strength reduction was introduced.
Godot's depth texture excludes transparent roads in the tested backend: the port
uses matching cached depth variants of the same source meshes/material discard
rules. Original ScreenWidth512, not host1920, drives native LOD for every view.
Depth shader programs must be shared; compiling one per strip caused avoidable
first-entry stalls. Reflection targets are six64 faces and one persistent256 atlas,
with unresolved per-draw PSMCT16 precision explicitly reported.

Tests and exact pending work are in the active TASK0052. The full live-car route
is still too slow (Debug Track11 median80.91ms before subsequent batching work),
so browser-only timings are not60FPS acceptance. Sun/AcidFX closure, full Release
all30 profiling, actual window UI checks and final successor audit remain pending.
All captures/binaries/assets stay under ignored local or external directories.
The isolated test DLL has not been promoted to godot/bin. No Verified promotion.


## TASK-0052 subsequent validated checkpoint (2026-10-18)

Ghidra read-only access restored. Integrated actual WorldEffects packets and common
Sun/scenery/particle alpha ranks, fixed sky far-plane sign that overpainted opaque
cars, added cached depth-mesh discovery/change notifications and isolated lifecycle
ownership. Added actual ImGui click, original-emitter GPU and sky negative-control
fixtures. Independent race scene now forwards UI input and clears captured driving
controls. Debug99/99 nonGPU and6/6 window regressions pass, plus separate particle
and ImGui window tests. Release92/93 initial, failed symbol-link metadata corrected
by standard class generator then4/4 rerun passes.80 new version/address-bound symbol
entries retain Decompiled status. Explicit TASK0052 source successor audit passes,
13 negative review tests pass, historical0051 unchanged. No task-complete/Verified
claim or normal-DLL promotion. Full commands/log paths/remaining prerequisites are
in the active task. All30 Release performance sweep still running; Track11 measured
37.304ms median,46.974msP95,69.383msP99 with original defaults at1080p after cache.


## TASK-0052 passing-build delivery requested (2026-10-18)

User deferred60FPS work. Stopped only the owned performance process, preserving
partial metrics; no new optimization patches applied. Promoted current Debug /
ImGui build to normal godot/bin and retained Debug/Release copies plus old DLL
backup under local/task0052/delivery. Debug build cache now uses the normal project
again. Task remains active for explicit original-rendering gaps and later performance.
Final build/test results and shortcuts are recorded in the delivery manifest.


## TASK-0053 runtime regression checkpoint (2026-10-18)

Fixed the evidenced exact-zero VU reciprocal-square-root carrier in Debug
SkidSet001A8150, retaining nonzero underflow as an explicit gap. Removed host
ignore_culling from custom-projected sky/Sun/particles: it bypassed camera layer
masks and leaked all capture skies into main/depth views. Camera-frustum custom
bounds preserve normal layer isolation. Track11 fixed-pose draws3921 ->1912;
Sun-facing original GPU coverage0 ->1024, road-occluded0. Original native visibility
remains14/128 sections,230 considered,102 visible/8407 loaded instances. Added
read-only Visibility counters. Roadshine OFF now skips its extra texture sample
while retaining road/water mask alpha; unchanged enabled integer fusion is tested.
Vehicle environment materials cache stable bindings and unsupported31C diagnosis,
while validating/updating the capture matrix each call. Shared builds/tests and
TASK0053 successor review are in progress; no Verified or60FPS claim.


TASK0053 validation complete over bounded fixes: Debug100/100 nonGPU and
Release94/94 including required preservation reruns after explicit review;
9/9 selected GPU/window checks including race rerun with Dummy audio driver.
Release independent actual window PASS for real track/car, controls, failure,
restart and re-entry. Passing binaries/launchers with hashes at
local/task0053/delivery. Remaining driving CPU submission cost is explicitly open;
final wet-pose Roadshine on/off/on34.973/33.445/33.791ms, same715draws. No broad
stutter-resolved or60FPS claim. Ghidra/read-only and original evidence status
unchanged. TASK0053 remains active for the remaining performance criterion.


## TASK-0054 camera-motion projection regression (2026-10-18)

User reports Sun and particles incorrectly follow camera. Actual original Sun
worldpoint diagnostic reproduced vertical mirror: expectedY370.73 vsobserved169.88
in540px viewport while X agrees. Explicit POSITION bypassed Godot render-target
projectionY sign. Shared host screen helper now preserves that sign for both
Sun and particle packets, including depth proxies. Native positions, simulation,
layer quads, alpha/depth/readback unchanged. Corrected preliminaryY369.01; dynamic
Sun GPU test passes, old53DLL fails it. Three moving-camera actualparticle tests
pass but oldDLL also passed broadunion bounds; stronger isolated shader transport
negative-control fixture is being added. Debug/Release builds pass; full tests
in progress. TASK0054 has independent two-body source successor review; historical
53 remains frozen. No original-console Verified claim.


TASK0054 completion: Debug101/101, Release95/95; final shared GPU5/5 PASS.
Strong Sun and asymmetric production-particle controls fail on frozen53 DLL,
pass current build. Release actual-window fixtures respect original defaults
(no Debug-only override); moving Sun/road occlusion and actualparticle/probe PASS.
Both binaries promoted/copied with launchers/hashes at local/task0054/delivery.
Only host projection corrected, no native source or physics delta. Original
Verified unchanged. Next independent work remains TASK0053 CPU cost/TASK0052 gaps.


2026-10-19 TASK0055: host optimization implementation. Frozen54 baseline DLLs
and successor source review retained; current Debug/Release build PASS. Resource
cache/dirty submission, closed vehicle depth-uniform contract and persistent
particle mesh buffers implemented. Pure pool/particle GPU fixture PASS; reflection
three nondegenerate ELISE poses exact atlas/final/color/rawdepth/native-packet PASS.
First reflection fixture offset-8 was colinear and failed both variants; corrected
fixture offset-4 without production math change. MultiMesh all30 stopped Track24
pose50 on exact color mismatch; rawdepth equal; reuse-only isolation PASS. Keep
MultiMesh experimental/off, no relaxed image threshold. All30 reuse correctness,
independent occlusion/opaque gates and exclusive Release performance remain in
progress. No FPS/default/Verified claim yet.


TASK0055 all30 reuse GPU completion: TRACK_OPTIMIZATION_GPU_OK tracks=30,
90 actual static poses exact final/intermediate-color/rawdepth and logical packet
parity. Post-sequence forest-fire pair516active/91visible equal, exact pixels;
clear/reload/invalid-track cleanup PASS. Log local/task0055/parity-reuse-all30.log.
Counts differ from the initial single-track fixture because the renderer preserves
its original RNG across prior track loads; each pair receives the same sequence.
Benchmark creates fresh renderer/session each repeat, resetting this caller seed.


Final cache correction: independent-process1080p probes isolated differences to
car reflection: raw native-depth and reflection-disabled images exact. Shared
materials yielded107 differing atlas pixels in all3 repeats; distinct materials
removed atlas differences in all3. Remove material sharing entirely, preserve
material identity. Native input/frame digests and camera/car pose bits match.
Residual tiny independent-process car-reflection raster differences also occur
within baseline repeats (1-18pixels); cause is unresolved, not waived as a zero-
difference controlled test. GLES3 source already declares invariant gl_Position;
no speculative precision/shader/depth patch was made.

Strengthened controlled fixture now1920x1080, real Ps2DrivingRuntime first native
snapshot, actual chase pose, real car capture binding, plus3 rotated/moving views.
Final cache passes exact native packet/view counts, atlas/final/color/rawdepth in
all4 poses (reflection-native-drive-1080p.log). All30 final-cache rerun in progress.
Public read-only render-target getter available Release for untimed diagnostics;
no Debug effect override relaxed. Successor review now17before/63after, host only.
Earlier14.9% wall result used unsafe sharing and is diagnostic history, NOT final
performance acceptance. Benchmark loading time is deterministic and final native
digest/pose bits plus color/depth/atlas captures occur outside sampled loops.
Temporary car-reflection-off capture occurs only after all samples/original
captures as isolation evidence, never to improve measured frames.


FINAL no-material-sharing candidate: all30/90pose exact final/color/rawdepth and
native digest PASS, dynamic post-sequence particles516/91 equal, failure/clear
cleanup PASS (parity-stable-cache-all30.log). Same final source104 non-GPU Debug
and98 Release checks pass;14 final GPU checks in progress. Snapshot hashes at
local/task0055/final-benchmark-inputs.json. No more production changes planned.

Final timed paths are local/task0055/final-baseline/all30.json and
local/task0055/final-optimized/all30.json; preserve earlier partial/diagnostic
directories. Run frozen54 baseline and final-cache optimized sequentially, no
concurrent builds/tests. Current benchmark optional --performance-isolate-reflection
only generates an extra untimed reflection-off diagnostic after original samples;
default false, not used for final timing. All timed samples retain original effects.
Comparator requires matching serialized pose and final native packet evidence,
in addition to all30/120/600/3, relative gain and no persistent wall tail regression.


2026-10-19 TASK0055 final acceptance: frozen54/final55 Release1080p all30,
120 complete-route warmup/600 samples/3 repeats each (180 complete runs),0 failures.
Comparator PASS; identical camera/body pose bits and native packet/sections/zone/
visible-instance data, effect permissions and draw distributions. Median-route
wall-10.0328%, prepare-27.6223%, host submit-33.1621%; no persistent>5% wall median/
P95/P99 regression. GPU timers unavailable. Median load10.116s->10.166s and worst
loading frame8.289s->8.896s: no loading improvement claimed. Report at
docs/PERFORMANCE_TASK0055.md; raw data/captures ignored under local/task0055/final-*.
Safe profile promoted only after gates; both Debug/Release constructor checks
pass. Isolated DLL/config delivery snapshots keep shared workspace script/asset
junctions and external ELF/assets; no proprietary data added to Git. Debug118/118,
Release98/98, final all30 GPU and enhanced1080p native-driving/reflection checks
pass. Material sharing removed after atlas evidence; MultiMesh failed color gate;
occlusion/opaque had no eligible work and remain off. Source review17before/63after
port-only identities preserves frozen54/native/physics. No original Verified claim.

### TASK0053 - explicit experimental MultiMesh trial, 2026-10-19

User requested trying existing instancing. Added ignored delivery MultiMesh viewer/
Release race launchers; Release startup flag assertion passed (instancing true,
safe caches true, occlusion/opaque false). Accepted default/source/binaries unchanged.
Known TASK0055 Track24 color mismatch remains; no new FPS/equivalence claim.
See TASK0053 trial section for reproduction and rollback.

### TASK0053 support / E-0051-S02: original track LOD resource recheck

User asked whether original track assets actually contain LOD. Read-only scan:
all30 TRACKB LZC packages have multi-model SceneryInfo rows; TRACKB11 has398
2-model rows, TRACKB21 has277 2-model and130 3-model rows. Detailed counts,
offsets, thresholds and coverage limitations appended to reverse/task0051/scenery.md.
No implementation changes; original runtime comparison still absent.

### TASK0056 - Godot HLOD source freeze and validation checkpoint

Implemented optional engine-selected original authored levels in the shared renderer,
independent core descriptors, original section eligibility and per-view persistent
range/depth resources. Original selection remains default. All30 host load/view/clear
passes, including original default texture-table binding exposed by all-level warmup.
E0056T01 restores bounded original eSolid/GetTextureInfo owners and retains source
aliases; no generated fallback pixels. Both ELF entry byte ranges match pinned files.
Prewarm/cancel preserve visual clock; GPU projection/range/capture and viewer/drive/
independent race checks pass. Debug107/Release101 non-GPU checks and final4 targeted
checks pass; final source review11before/19after preserves all prior native bodies.
Final original14-GPU regression and exclusive selected-route fresh-process benchmarks
run via local/task0056/final-regression-performance.ps1; see TASK0056 exact continuation.
No FPS improvement/default promotion claimed before measurements. No Verified claim.

## TASK-0056: Show LOD user-requested increment (2026-10-19)

Added port-only TrackViewer ImGui checkbox/action/snapshot forwarding. Shared
renderer expands original levels in either backend, disables level ranges while
checked, restores ranges when unchecked, and keeps original-mode draw-cache keys
separate. Depth/environment passes inherit source-node range state. No native
physics/recovered behavior changed. Prior benchmark is no longer running; its
partial outputs are diagnostic and do not establish performance improvement.

Debug and Release: `cmake --build build/godot-debug -j 6` and
`cmake --build build/godot-release -j 6` passed (GCC13.2/Godot4.4.1).
Targeted CTest debug_console/static_lods/track_hlod_source_review_rules/
progress_tables: 4/4 passed. Both-backend actual-window toggle/lifecycle smoke
passed SHOW_LOD_OK: local/task0056/show-lod-smoke.log. Standalone --check-only on
asset_viewer.gd cannot resolve DebugTools autoload in that invocation; the complete
scene smoke is the valid entry check. Delivery DLL snapshots refreshed; no commit.
PC checks do not imply PS2 Verified. Task56 broader performance closure remains.

Show LOD final: full scene entry TRACK_VIEWER_SMOKE_OK; Release debug_console
1/1 PASS. Standalone check-only autoload limitation did not occur in actual scene.
User requested one representative track only; no new catalogue sweep. GPU tests
restored opt-in OFF. Debug B5989342A24A054C835812C1006B5796C85AF908956C56B7DF3AE4F5CE9205C2;
Release04231C65DD003E9704A9346FC2E257EBDECD016D066893A7366033288FDAD829.

## TASK-0056 authored LOD coverage clarification

User observed mostly vegetation switches. Single Track11 asset inventory confirms
6922 single-distinct-reference/1483 two-reference non-sky instances. All24 multi
model tuples use REDWOOD/SHRB/FORESTLINE names;50 road-named tuples are single.
E0056H02 documents bounds and six unresolved slots. Built static_lods_tests and
ran --assets11 --lod-inventory PASS. No all30 run, no new recovered/runtime changes.

## TASK-0057 - Track culling / generated LOD investigation complete

Read-only Ghidra Retail00150950/00150378/0014FE18 refresh confirms original
hierarchical selection, not whole-track submission. Current HLOD CPU candidate
expansion differs. Godot4.4.1 official generator/ArrayMesh/GLES3 paths inspected.
Tracked headless Track11 probe passed24batches,5generated terrain levels, sampled
4017->3717triangles; no road reduction in sample. Synthetic reference1LOD. Not a
pixel, FPS or original-equivalence test. See TRACK_CULLING_AND_GENERATED_LOD.md
and reverse/task0057 results. Default code/physics unchanged. User one-track scope
respected; no all30 sweep. Follow-up is optional cached constrained terrain mesh LOD
plus original candidate culling, not blanket automatic decimation. No commit.

## TASK-0058 native culling integration complete (2026-10-19)

User revoked host HLOD. Original only accepted, all normal main/capture frames call
original runtime; Show LOD explicit diagnostic retained, host ranges0. Nine reviewed
port body replacements, original culling/physics unchanged. Source chain now has
TASK0058 successor pinned to TASK0056, both source-review test suites pass.
Fresh read-only Retail0010E638/0010CCB0/00150D98/eCreateView001070C0, paired Debug
view0010EA18/Camera001566E0 and original defaults inspected. No unknown wholecamera
claim. Debug/Release cmake --build -j6 PASS; --assets11 runtime PASS; targeted6/6
CTest PASS; native GPU and viewer/Test drive/Release race lifecycle+all6capture views
PASS. No all30 run. Native start view128sections/8407instances->14/203considered/
62selected; camera motion54/52selected. Evidence E0058R01, docs/NATIVE_TRACK_CULLING.md.
Task56 deferred, archived launchers forward58, current native launchers and DLL pins
under ignored local/task0058/delivery. Inspected start screenshot; offset100 screenshot
is off-road water because test camera follows a straight vector, not road topology.
No FPS or original Verified claim. No commit; unrelated edits and external assets kept.

TASK0058 final closure: Release targeted4/4 PASS, final source audit9/9 PASS,
diff whitespace check PASS. Native launch snapshots pinned in BUILD-INFO.json.
All owned test windows closed; no catalogue-wide test was run this increment.

## TASK-0058 delivery dependency correction

User reported Windows Error126 missing avcodec62/avutil60/swscale9 in the actual
Debug delivery folder. Root cause: snapshot code copied only extension/config;
prior checks used workspace and staged-build directories that already had sidecars.
Copied all8 matching built runtime DLLs into BOTH delivery bin folders, including
swresample6 and transitive FFmpeg libraries. Added tools/stage_godot_runtime.ps1
with mandatory dependency presence checks and per-flavor SHA256 manifests.
Extension hashes unchanged; no native/recovered code or rebuild needed.
Direct delivery Debug actual-window viewer/Test drive/return/clear passed
TRACK_NATIVE_VIEWER_DRIVE_OK. Release direct delivery starts and native captures
passed; final lifecycle result is recorded below after completion. No new tracks.

Delivery correction final: direct task58 Debug viewer/Test drive PASS and direct
task58 Release RACE_RUNTIME_SMOKE_OK PASS, including exit/re-entry. Logs
local/task0058/delivery-debug-check.log and delivery-release-check.log contain no
extension/dependency errors. Both manifests have8DLLs and matching extension pins.
Packaging follow-up closed; no all-track test, native-code change or commit.

## TASK-0059 - Motion/rotation disappearance fixed with explicit particle gap

Initial distant/return and68pose probes plus180continuous moved/rotated frames did
not trigger fatal state. User confirmed Track11 and simultaneous rotation; zone
crossing probe reproduced DUSTA_CS guard. Entire-atlas bound false-positive was
separated from real high-alpha gap (vertex199/tile89). Read-only Retail00128DA0/
Debug00128EF8 confirms original RGBA255 clamp, so no fake alpha attenuation.
Corrected atlas footprint, cached immutable bounds, caught/reported only per-quad
NotImplemented, compacted valid node/stream/alpha ranks; no global disable or
native/physics change. Unsupported quad omission is visible and incomplete; full
signed GS compositor is NOT claimed. Corrected Reset Camera world-origin bug.

Build Debug/Release PASS, targeted6/6 PASS, source successor3before/4after PASS.
Direct task59delivery Debug motion/reset PASS3gapframes with supported particles;
Release native/gap/culling/lifecycle PASS1gapframe. Only Track11 tested. Both bins
have8runtime DLLs, BUILD-INFO pins. Previous58launchers forward59. Inspected actual
fixed-gap.png shows visible scene and warning. Test harness Reset/autoload/path
failures preserved in task notes; final runs clean. Commands: --script
res://scripts/debug/track_camera_motion_smoke.gd -- --track-load=11 (Debug);
res://scripts/runtime/track_native_culling_smoke.gd -- --particle-gap-smoke
--native-capture-dir=<absolute directory> (Release). No commit or Verified claim.

## TASK-0062 - 2026-10-19

Added default-on TrackViewer Visibility Render props. Port-only transformed
scenery filter before shared main/reflection/HLOD batching; world-identity
geometry/collision preserved. Shared session gate and defaults/action/snapshot
wiring. Native mixed-frame and settings tests pass; Debug bridge build passes.
Staged headless controls PASS; actual ImGui window off/on PASS, Track11 commands
1186 ->581 ->1186. Paused fixture explicitly redraws before count inspection. No recovered body changed, no PS2 Verified.

## TASK-0063 - 2026-10-19

User-authorized render analysis/optimization implemented, then user replaced long
FPS matrices with static/native tests and drawcall statistics. Owned benchmark
terminated. Completed30-track/60row CPU census, native state/RNG rollback parity,
stream raw bytes and prior15image exact checks. Default state/transport/texture
caches remove redundant work; Sun reduction/Copy cutout queue remain off.
Debug/Release build and normal workspace build passed. Static table shows main
colour batches Track12 993->49 and Track42 925->51 with Props off; conditional
world passes1947/1847 include encoded-depth replay, before reflections/other
producers. These are not actual total GPU calls or FPS results. Frozen source
chain extended with exact port scope and auxiliary field/shader file hashes.
See PERFORMANCE_TASK0063.md and TRACK_DRAW_COUNTS.md. No commit or PS2 Verified.

## TASK-0064 - Live viewer draw count

Added Godot last-rendered-frame global draw calls below TrackViewer coordinates. Reads Performance monitor once per UI snapshot without GPU readback. Header remains auto-sized. Normal Debug build and native debug_console passed; no performance sampling run. TASK0064 explicitly reviews only the two presentation adapter bodies.

## TASK-0065 - Track11 dense matrix submissions

Implemented viewport engine counters and full camera/control export/restore. CPU-pretransformed meshes failed exact depth parity, so raw local attributes plus per-vertex matrix rows retain shader arithmetic. Final optimized default is restricted to Track11 without a car visual consumer; native selection, LOD, alpha order, effect quality and RNG remain unchanged. Seven1080p default pairs are byte-identical with heavy totals2409->669/3003->689, plus three ordinary Track11 marker poses. Capture/Track24/opaque failures remain recorded and disabled; a viewer-to-car transition plus four actual reflection atlas/colour/depth checks pass through baseline fallback. Track24 default poses retain764/799/796draws and exact pixels. Mobile and Forward+ pass their own comparisons, but cross-backend outputs differ, so Compatibility remains.30-track CPU census covers60rows; no sustainedFPS or PS2-equivalence claim. Normal Debug build and13targeted native/UI/host/source checks pass. TASK0065 pins exact11-before/13-after port adapter changes and auxiliary sources without refreshing old reviews. See PERFORMANCE_TASK0065.md for reproduction and remaining work.

## TASK-0066 - Global batching without track-ID exceptions

User explicitly requested global optimizations. Removed Track11-only policy field and numeric predicate; the optimized profile now applies the same matrix planner to all tracks. Preserved generic state-based fallbacks and independent rollback. Normal Debug build and14native/UI/host/source checks pass. Track11 three marker images remain exact; Track24 now uses batching by default (764->266 and799->231draws), with its known second-view colour mismatch still failing exact image validation while depth/native packets match. Did not widen image tolerance or claim all-track visual/FPS acceptance. Source successor validates three exact port changes, freezes previous review bytes via explicit historical auxiliary snapshots and carries all inherited live auxiliary pins. Future optimization policy is global/state-based, never a track allowlist. See PERFORMANCE_TASK0066.md.

## TASK-0067 - Live triangle/primitive counter

Added Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME to the existing UI snapshot and same-row Triangles/primitives display beside draw calls. Global last-rendered-frame scope includes triangle/line/point primitives; no GPU readback or rendering/native logic changes. Normal Debug build updates workspace DLL; native debug_console and headless TrackViewer startup pass. TASK0067 reviews exactly two port function changes and carries all inherited auxiliary pins without refreshing frozen predecessor reviews.


## TASK-0072 worker integration,2026-10-20

Preserved the uncommitted checkout and pre-split DLL in ignored worker-baseline.
Added a Godot-free CPU mesh builder, main-thread miss snapshots, native worker
groups and same-view joins with ordered resource creation/upload. Native28/28
relevant checks and engine0/2/4 packed-attribute/error/cache contracts passed.
Pre-split vs split0/Safe actual driving723 ticks/seven native-resolution checkpoints
passed8759 checks. Found inherited _notification delivery bypassed the old fixture
override; an explicit virtual lifecycle hook preserves production behavior while
isolating deterministic fixture focus. Earlier interrupted matrix runs are invalid.
Confirmed native zone->section->tree->instance selection remains present: observed
Track11 zone71 selects14/128 sections and considers235/8407 instances at first
checkpoint. Threading never replaces this selection. Final matrix/report pending.


TASK0072 closing worker checkpoint: full0/2/4 Safe driving image/state parity
passes8759 comparisons per pair. Separate differs in colour/final/reflection
hashes and reports wrong-thread RenderingDevice finalization/ObjectDB leaks.
No engine-only attribution was run. User stopped repeated windowed tests; all
owned running/queued Godot jobs cancelled before the timing matrix. No FPS gain
claimed, no default changed, task active. Sector chain confirmed active and
neighbour-zone/section exclusion tests added. Final builds/source pins recorded
in the performance report; no proprietary data or captures enter version control.


TASK0072 closing worker checkpoint:0/2/4 Safe driving image/state parity passes8759
comparisons per pair. Separate differs in colour/final/reflection hashes and reports
wrong-thread RenderingDevice finalization/ObjectDB leaks. No engine-only attribution
was run. User stopped repeated windowed tests; all owned running/queued Godot jobs
cancelled before timing. No FPS gain/default promotion, task active. Original sector
chain retained and neighbour-zone/section exclusion tests pass. Debug/Release builds
and native28/28 plus settled8/8 checks pass. No original Verified promotion.


TASK-0072 CPU submission phase: implemented versioned bulk vehicle snapshots,
source/order/field depth tracking, stable handles/names and per-view pure CPU math
in the shared Godot native group. Debug/Release builds, native5+2, Python7+7+21,
8759 bounded PC parity checks and552 unchanged core hashes pass. Necessary Godot
permission was renewed. Four consolidated performance processes show no gain:
drive median paired-0.430ms, moving rotation-2.090ms; all P95/P99 pairs regress.
No default promotion:0/Safe, candidates off, failed residency/Separate off. Uniform
reads-89.18% but8185 proxy creations persist; legitimate source churn defeats
near-every-frame discovery cache. Math TU remains outside current host-O2 list;
small-task snapshot/join costs need work. Raw maxima include frame719 sync export,
so sustained/export-separated acceptance remains pending. Whole-frame GPU N/A.
Task stays active, no60FPS/Verified claim, no commit. Continue from performance
report and ignored cpu-submit-phase paired metrics; no more window runs queued.


TASK-0074 completed: Effects > Debug (Godot) > Wireframe debug uses existing
port-side descriptors/actions and a lazy renderer-owned Godot viewport helper.
Default off; colour/root only; depth/history/reflection/native selection retained.
Actual isolated Vulkan ImGui click/pixel/depth/recreation/default/exit smoke passes;
no native rebuild or PS2 Verified claim. Private artifacts under local/task0074.


## TASK-0075 - Original wheel particles and current driving gaps (2026-10-20)

Preserved the pre-existing dirty workspace. TrackViewer preview controls no longer
write FOV; initial/reset/aspect-ratio/real-track-drive-return checks retain default
75/KEEP_HEIGHT. Driving and CarViewer/race policies remain separate.

Read-only Debug trace E-0075-T01/C01/P01..P03/001/D02/D03 recovers per-wheel
terrain smoke and rear tire-spew, saturating skid history, original source list
order/copies, static config wheel positions, negative intensity and live current
car velocity. World updates existing particles before terrain emission. Shared
GLOBALB library plus INGAMEB textures feed the existing particle algorithms and
renderer; no modern replacement particle behavior or physics is introduced.
The one-car tool supplies distance0/outside camera/all-view car visibility.
Dynamic storage clears without losing static groups/RNG. Flat synthetic depth
is explicitly host-side; real-track consumers share native depth and alpha order.

New native tests4/4, existing native regressions10/10 and seven related production
host/projection checks pass. Exact two-reset x120-step visual-on/off trajectory
comparisons pass. Vulkan flat and Track11 fixtures produce peaks469/467 and497/398
active/visible with no reported gaps in those bounded runs. Screenshots inspected
and ignored. Original probe timed out; no Verified or commit claim. Full terrain
remainder, first-person/mirror/multi-car/damage and signed GS blending remain open.
See DRIVING_EFFECTS.md, DRIVING_GAPS.md and TASK-0075 for commands/continuation.

TASK0075 final strict successor review passed10 replaced/33 bounded or host
identities and153 auxiliary pins;14 fail-closed review contract tests passed.
Frozen prior review/scope bytes remain unchanged. New pass-scoped historical
reuse removes repeated work without retaining caches across validation passes.
An initial broader CTest run timed out in debug_driving_migration_rules. Direct
verbose rerun of that unchanged suite passed12/12 in7.496seconds. The timeout
cause is unresolved; it is not evidence of an original behavior mismatch or a
confirmed historical-recursion defect in that unit suite.

Final TASK0075 bounded CTest rerun passed6/6 in42.03s (new review,14 review
contracts, class coverage/generator, original migration and single driving chain).
The unchanged migration suite passed in7.51s; the earlier timeout is retained as
an unresolved transient validation event, not a code defect diagnosis. Generated
progress and scoped source/diff checks pass after completion.

## 2026-10-20 TASK-0076 driving render retention

User reported world disappearing mid-drive and expensive vehicle reflections.
Found update_view/readback catch paths unloaded track+collision on rendering gaps;
retained independently rendered car explains a compatible source mechanism, though
field exception unknown. These catches now warn/expose per-frame error and retain
loaded state for retry. Default/restore Environment capture is off; opt-in retained.
Native settings, five initial CTests and controls host passed. Vulkan actual
missing-camera fault retained geometry/topology and recovered; capture scheduling
vanished when disabled and worked on opt-in. Inspected recovered road screenshot.
Source successor permits six exact host slots; TASK0075 frozen snapshots untouched.
14 predecessor review contracts and strict live source audit pass.
Initial main link was locked by user's app; isolated build/tests succeeded without
closing it. Later app exited independently; restored normal CMake output paths and
main Debug build passed. New DLL is installed in godot/bin for next launch.
See reverse/task0076/source-review.md for exact commands, versions and limits.
No Verified or performance-percentage claim; if reproduced, use new warning and
render_error/readback_error to investigate the exact dependency.

## 2026-10-21 â€” TASK-0082 vehicle lights, horn and engine audio

Recovered native lamp selection and original ON/OFF material assets; ordinary
horn input also flashes headlights. H is a port-side binding. Decoded original
MSH/MSB twelve-engine-plus-horn banks and recovered RPM/throttle/shift/timbre,
volume and pitch mixer slices.49vehicle rows decode;15traffic rows reference
unavailable bank47CB8E5A. Both driving hosts own playback via driving_audio.gd;
pause/restart stop/reset host voices. See reverse/task0082 evidence and strict
source-review.json. Full flares/SPU/stereo/replay/special audio remain unbound.
Native4/4tests passed43.21s; Godot extension build passed; headless audio transport
1/1passed1.64s. Dummy audio proves transport, not listening quality or original
runtime equivalence. No Verified promotion and no commit. Next: authored flare
marker/parameter/texture recovery; original executable comparison remains needed.

## 2026-10-21 â€” TASK-0083 collision and drift audio

Recovered original InGame impact variants, impulse/time/material gates and RNG;
static collision event lifetime now produces one impact per collision. Restored
persistent wall/wood scrape with nonlinear distance and original stepped fade.
Original tire skid/squall/burnout state and four looping samples supply ten logical
slots independently of skidmarks. Both driving consumers share playback transport.
Evidence E-0083-A01/C01/T01 under reverse/task0083;24functionrecords indexed.
Native5/5passed25.71s; final collision creation-order correction wall1/1passed13.23s;
Godot final staged2/2passed5.33s. Strict review83 7/7 and preservation1/1passed;
frozen82 unchanged. No Verified promotion or commit. Original spatial/SPU/cadence,
rolling/damage/tire-impact and multi-car consumers remain outside bounded scope.
Running user frontend locks normal DLL; final DLL validated under ignored
local/task0083/godot-stage/bin. Window preserved; normal cache output restored.
After closing it, run cmake --build --preset godot-debug --target ps2_port -j6.
Next recovery: original spatial sound/service cadence; see completed TASK0083.

## 2026-10-21 â€” TASK-0084 recovered Test drive camera

Test drive now consumes bounded original exterior DriveCameraMover POV1/history,
normal lookback easing and original topology clearance. V toggles front/back;
mouse orbit temporarily overrides the live native pose and returns after1.5s idle.
Godot default75verticalFOV/KEEP_HEIGHT replaces the old horizontal-FOV override
by explicit user request. Focus/UI/exit lifecycle and viewer projection restoration
are tested. Evidence E-0084-C01/M01/H01 under reverse/task0084;19entryrecords indexed.
Native camera1/1passed3.42s including64configs/14fallbacks/wall/240drivingframes;
five hostchecks passed16.37s including actualCarViewer and realtrack11. Strict84
7/7 and preservation1/1passed; frozen83 unchanged. Final normal DLL built successfully.
No commit or Verified promotion. Camera shake/pursuit/in-car/birdseye/DOF/full
original topology cache and runtime comparison remain scoped future work.
Next: read completedTASK0084 and recover remaining original camera services;
reproduce with ctest --preset native-debug -R '^driving_camera$' and the hostsuite
listed in the task. Keep user-requested Godot projection policy when extending.


## 2026-10-21 TASK-0085 GPU particle implementation

Scope: user-approved native emission/lifetime ledger plus optional Godot GPU
simulation/draw backend, followed sequentially by TASK-0086 vehicle flares. No
vehicle physics replacement or Verified promotion. Original emission replay and
six native regressions passed. Actual Vulkan smoke, lifecycle and production
shader state probes run on pinned Godot4.7.2 Intel Arc B390; raw evidence remains
under local/task0085. The first counter-only GPU run was rejected after image
inspection revealed no births. Explicit request_process_time caused double
processing; use zero engine delta, shader epochs and committed
RenderingServer::particles_request_process(baseRID) instead. Deferred commit and
abort prevent failed host frames from submitting GPU births. Main-view timing
only; capture faces reuse output. Native default remains pending the full matrix.
A first benchmark attempt produced no samples because its driving harness still
referenced TASK0084-removed yaw_offset; adapt the harness to driving_camera.yaw.
That invalid capture remains preserved, and is excluded from performance claims.


TASK-0085 completion: user explicitly stopped further benchmarking. Terminated
only task-owned matrix Python/PowerShell/Godot processes, preserved raw runs,
and did not promote incomplete timings. GPU remains optional, native default.
Final GPU CTests3/3 passed26.25s. Proceed sequentially to TASK-0086 flares.


## 2026-10-21 TASK-0086 original vehicle light sprites

Recovered marker/pivot/normal bindings and original static flare resources, exact
bit-float angular power, intensity and ePoly consumer state. Bound twelve native
channels through driving snapshots to fixed attached billboard instances sharing
static mesh/materials. Real BMWM5COP horn toggles already recovered FAST cop state;
no new native state setter. Original three textures decode from GLOBAL pack.
Native and GPU flat/Track11 tests cover actual pixels, occlusion9920->0, copphase,
reset, reload and cleanup. Original runtime equivalence remains unverified. A
first Track11 test incorrectly assumed monotonic size with changed angle; corrected
the assertion while preserving recovered formula. Added ObjectID cleanup after
identifying extension-wrapper teardown can follow child-node deletion. No further
benchmarking after the user's explicit instruction.


TASK-0086 validation completion: final vehicle flare GPU CTests3/3 passed37.33s,
including explicit live-node exit after ObjectID cleanup fix. Driving camera,
asset and track pipeline checks passed. Existing race smoke had a stale7-argument
horn mock and applied Test drive default75 to the separate original-horizontal-
FOV runtime; corrected only that test and race_runtime_host passed40.93s. The
first failed progress freshness check came from appending outside the compact
tracker generator; regenerated it. An atomic replace briefly returned Windows
WinError5; retry succeeded, and progress_tables passed2.26s. No remaining behavior
or build failure. Strict E-0086-S01 source preservation is final pending check.


TASK-0086 complete: final strict scope tests7/7 passed280.215s and
existing_body_preservation1/1 passed294.43s. ReviewSHA256
5eb78631b86cd079c7074730ebf2cec463eb8f988d520bfa6224ac50f54b9b40;
frozen85unchanged. Scope8replacements/19additions, allstubfalse,7modifiedaux and
13newpins. Earlier cancelled and auxiliary-classification test runs are superseded
by the final passes. Both TASK-0085 andTASK-0086 are completed with the explicit
performance-matrix waiver/native-default boundary. Raw evidence and screenshots
remain ignored. No commit or proprietary asset staging. Optional continuation:
original runtime equivalence/globalDMAalpha validation under TASK-0052, not another
performance matrix without a new user request.

## 2026-10-21 TASK-0087 Track12 reported driving gaps

Fresh read-only Debug TriggerEmitter evidence confirms zero-interval retries;
removed the premature throw while preserving RNG and adding an explicit throwing
port iteration budget. Actual Track12 gravel/sand emitters and full-pool RNG
regressions pass. Original optional null wheel-blur solids now load with an
explicit diagnostic rather than failing each driving frame. Original terrain
rows4/5/20 now reach existing native wheel/static contact logic; unsupported
CAR terrain remains explicit. Water-specific wipers/splash/replay are unresolved.

Native CTests track12_particles/gpu_particle_emission2/2 passed11.82s;
vehicle_blur_absence/vehicle_model2/2 passed; static_collisions/water_contacts2/2
passed38.64s, and driving_tracks passed18.26s. Godot4.7.2 on Intel ArcB390:
track12_driving_gaps_host/gpu2/2 passed29.47s. Fixture uses actual Track12,
native911TURBO driving and an absent-blur CROWNVIC render model, with test-only
positive blur alpha. No benchmark or original equivalence claim.

The running user game initially locked the normal DLL. Staged import and host
smoke passed without terminating it. After the user closed the game and
authorized replacement, normal CMake output paths were restored and the final
ps2_port DLL linked successfully. Both final host tests use that normal DLL.
No commit or proprietary asset staging.

TASK-0087 complete: strict source unit7/7 passed291.455s and existing_body_preservation1/1 passed293.52s. Final review SHA25665248eca7c5d4f621fc9c52cb838aec312ef7f5b974de3a2c27250c1c8ea68fc; frozen86 unchanged, no original stub transitions. Cached metadata validation passed after recording results. No additional benchmark. Optional continuation: original-runtime equivalence or water-specific wipers/splash/replay, with fresh scoped evidence.

## 2026-10-21 TASK-0088 original POV and held lookback correction

The user clarified that V must cycle original driving POVs and their FOV, while
B holds lookback and release restores the selected POV. This supersedes the
earlier TASK0084 lookback-toggle/Godot-projection exception. Read-only original
Update/Cycle evidence restores authored forward bumper, closer/exterior views,
horizontal90/71.520996-degree projection and native own-car suppression flags.
Native dashboard metadata is recovered but host dashboard rendering remains an
explicit throwing boundary. Godot's single-car main subtree suppression preserves
loaded resources and the independent world-face environment capture consumer.

Native camera1/1 passed4.18s, all64configs plus bumper/B/ground/reverse checks;
six host checks passed42.05s, final normal DLL rebuild and latest-DLL camera test
passed3.92s. V/B repeat/consumption/focus, mouse idle return, preview near/FOV and
visibility restoration are covered. No benchmark or Verified claim. Strict88 is
the remaining closure check; water effects research proceeds separately before
TASK0089 implementation. No commit or proprietary staging.

TASK-0088 complete: strict unit7/7 passed264.287s and preservation1/1 passed261.06s. Review SHA256583208d733c67735396dc33277e9445be9eb51755084374f84742885e9303123; frozen87 unchanged, no stub transitions. TASK0089 now active for authored DriveOver water spray, original splash audio and timed rigid-animation wipers; rain-screen RainSplash remains a separate subsystem.

## 2026-10-21 TASK-0089 original water effects

Restored authored water DriveOver groups, original body query/pair positions and channel blending; timed wiper state and original splash audio cooldown/RNG; original rigid animation matrices, model/skin/default texture binding and pixel-size gate. Native candidate ownership prevents render/capture audio duplication. Original InGame patches use existing gain/pitch helpers. Actual 911TURBO/GLOBAL controlled water tests pass for4/5/20; original MUDPUDDLE particle groups remain empty. No Godot physics or generated geometry/texture/phase.

Native11/11 passed65.01s; final nullsound water1/1 passed31.55s. Wiper/geometry/model final3/3 passed11.87s. Initial host fixture option-setting check was corrected before load; later independent race test exposed over-eager external-animation resolution for passive track solids. Retain unresolved metadata/diagnostics, throw only for required active animation, then rebuild normal DLL. Final race/water host2/2 passed21.07s; preceding six relevant host tests including Vulkan water4.94s passed. Ignored GPU screenshot inspected, no original equivalence claim. Source freeze sent for strict89; no further benchmark. Original replay capture, full World sound scheduler, damage/nitro/flat-tire and separate RainSplash remain outside slice. No commit or proprietary staging.

## TASK-0090 - C++ portability audit (2026-10-21)

Completed source/build review; see docs/CPP_PORTABILITY.md and tasks/completed/TASK-0090-cpp-portability-audit.md. Core searches found no direct Windows/Godot headers; guarded host API use is isolated. Non-Windows FFmpeg discovery and extension entries exist, while host test defaults, ARM64 Linux manifest coverage and non-Windows packaging/float parity require follow-up. Windows test enumeration found 133 tests; no cross-host pass is claimed. CMake 3.29.2, Python 3.14.4 and godot-cpp pin inspected. No behavior changes or new behavior tests; no commit. Continue TASK-0090 findings with the cross-host commands in the review; Linux/macOS hosts and suitable SDKs are required.


TASK-0089 completed: source scope unit7/7 passed (reported15152.077s wall includes environment suspension). Initial preservation run timed out10227.66s during machine wall jump, with no assertion failure; one authorized identical retry passed1/1 in349.32s. Final review SHA256a9fdb1c94b8c4902d5c723203d11c7c17b6764b888db6a2ab32f56d8afb846ce; frozen88 unchanged,17 replaced/12 added functions,16 modified auxiliary/10 new pins, no original stub transitions. Cached metadata check passed0.48s. Temporary task-owned awake helper released. No gameplay tests or benchmark repeated after functional closure. Final normal DLL includes camera and water fixes; optional continuation remains original runtime/World/replay or separate RainSplash evidence.

## 2026-10-26 TASK-0098 display dependencies

Continued original selection camera/quaternion/integer-angle/audio, spline-retarget, swatch/thumbnail/logo, wheel/body/reflection and selector light-packet recovery. Added real PLATFORM/CARCOVER shared native cache/worker/GPU upload adapter and stale-generation fixture. Cached signed-alpha animation and original fade-group transitions now tested including per-corner restoration. Exact addresses/evidence, build commands, passing tests and initial test-only failures are in reverse/task0098/display-dependencies.md. Latest native three regressions passed14.94s; latest normal Godot DLL built. No original oracle, Verified, production-page integration or career/FPS acceptance. TASK-0098 stays active; next original input/orbit/help, full Tick/load and FrameEffects, then production event entry. Existing uncommitted files preserved; no staging/commit.

## 2026-10-28 TASK-0098 actual production selection milestone accepted

FrontEndRuntime now owns native SelectionPage and real model/input/widget/camera/
lighting/audio/effect leases. One GPU colour/depth target executes stage/alpha
ordering. Fresh original constructor/projection evidence corrects FOV-adjusted
near and Retail points. RGB-only scanout fixes the blank vehicle caused by using
GS destination alpha as canvas transparency. Failed vertex-binding/teardown and
state-only green runs remain counterevidence.

Final real GPU-window script run-xci09xbl PASS: three first-event entry/cancel/
reentry cycles, real loaded car, visible-region check, swatch input, no native
errors or RID leaks. Godot/native builds passed; eight selected regressions and
contracts8/8 passed7.64s. No computer-use, staging, commit or Verified promotion.
All32 events, selection confirmation/race, Help/orbit/popup/locked text, original
comparison and Release performance remain incomplete. Exact commands and next
TASK-0098 steps: reverse/task0098/production-roundtrip.md. One final documentation
append initially failed Windows default text decoding; explicit UTF-8 corrected
it before final generation/validation.

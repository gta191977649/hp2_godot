# TASK-0118 - Fixed World clock and frame-independent MVP rendering

- Status: active
- Scope: independent Entities Bridge race clock, interpolation, fidelity tests.
- Source authority: Debug NFSHP2.ELF SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b. Host clock and Godot interpolation are port-side.
- Dependencies: TASK-0099, TASK-0115, TASK-0116 and TASK-0117. Track 13 victory is tracked by TASK-0100.

## Acceptance

- [ ] Original machine or PCSX2 driving, chase and NIS timing trace establishes the nominal PC cadence.
- [x] Diagnostic fixed mode preserves source order and renders interpolated committed state.
- [x] Five synthetic render schedules and five Release window FPS caps produce identical state on every sampled Tick.
- [x] Debug Championship drive/pause/exit and Hot Pursuit chase/pause/retry/exit with pose samples.
- [x] Fixed-clock Windows test covers the complete ticket cinematic and a separate 180 ms injected render stall.
- [x] Debug 120 FPS-cap window samples the airborne player, main camera, rear camera and gauge changes.
- [x] Ticket camera advances on both render frames within an authored key's two-tick interval.
- [x] Projective minimap animates between compatible committed World frames.
- [x] Window test checks particle IDs and PCM submission order around an injected render stall.
- [x] Detached-object determinant overflow regression covers first frame, held pose, blink, recovery and generation isolation.
- [ ] Reproduce Track 13 section 41 record 8 in the window and confirm resource and frame-time behavior at that collision.
- [ ] Sustained 120 FPS at 1080p with P95 render interval at or below 8.33 ms.
- [ ] Complete isolated particle/audio timing comparison and cross-platform/source cadence comparison.

## Source timing evidence

### 2026-11-18 high-speed steering and contact

The current MVP supplies 60/3600 s to each normal live World update and
uses the recorded intervals for replay. HP2FrameFix's PC version is a
reference for independent presentation and simulation clocks. The PC
ticker frequency does not establish a PS2 World interval.

The first solo BMW M5 run exposed incorrect presentation identity. The
old bridge used World.frameCounter, which can remain at 1 because the
source advances it in AI collision prediction.
The bridge now uses the committed World loop index, with the existing
one-based presentation convention and replay restore epoch. Native
World.frameCounter, tire and collision arithmetic, and race cadence
remain under their source owners.

The state-only runtime now offers two port-side, Godot-independent host
callbacks around a live World transaction. The MVP needs this lifetime
boundary to deliver scanner input and collect synchronous observations on
every tick when one render callback runs multiple World updates. The
existing render-time snapshot only exposes the last such update. The
default callbacks are empty. The diagnostic runs only with an explicit
trace path, `HP2_MVP_TEST_PROFILE=1` and `--race-track`. It starts a solo
Quick Race through the original start handler, on the authored track and
car definitions, from the grid with a fixed race seed. The fixture uses
the original scanner, including its dead zone and hysteresis. A smaller
raw pulse was filtered out, so the three-tick analog test uses 32 raw units.

Every committed tick records the World interval, source time scale and Timer,
nominal substep count (computed from the source maximum), last player
wheel force interval, raw stick, processed steering, vehicle state,
slip, and the count and summed impulse of the player's updated contact
records. A separate presentation trace records the car and camera poses.

Windows Godot 4.7.2 test results:

| Case | Entry and committed samples | Successful checks |
| --- | --- | --- |
| BMW M5, track 33 | 40.05 m/s, 945 ticks | 30, 60, 120 and 144 FPS, jitter and 180 ms stall. Separate 30.06 m/s keyboard-equivalent pulse matched at 30 and 60 FPS. |
| BMW M5, track 16 | 39.29 m/s, 1,900 ticks | 30 and 60 FPS. The response crossed an authored 0.4× slow-motion transition with 60 real-time quanta per tick. |
| Lamborghini Murciélago, track 13 | 40.04 m/s, 462 ticks | 30, 60, 120 and 144 FPS, jitter and 180 ms stall. Separate right pulse, keyboard-equivalent pulse and wall graze also matched between caps. |
| Lamborghini Murciélago, track 35 | 50.12 m/s, 1,648 ticks | 30, 60 and 144 FPS. |
| Lotus Elise control, track 13 | 30.06 m/s, 472 ticks | 30, 60, 120 and 144 FPS, jitter and 180 ms stall. |

The Murciélago's Vulkan window averaged 30.2, 60.0, 119.7 and 142.6 FPS
in the final matrix. Its car heading matched the committed heading at
corresponding presentation samples in the first window matrix. The
injected stalls yielded observed gaps of 197.3 to 197.7 ms in the
final runs. The largest yaw changes on the high-speed runs followed
recorded body contact and were identical across display schedules.
The Release race lifecycle passed pause, resume, restart and quit.
The Debug mouse replay smoke passed live seek and resumed slow motion.
The diagnostic refused to activate without the isolated-profile flag.

```powershell
cmake --build --preset godot-mvp-debug --target hp2_blackbox --parallel 7
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 7
$engine = './local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
$project = './build/godot-mvp-release/mvp-project'
py -3 tools/test_mvp_race_physics.py --godot $engine --project $project --output local/physics-probe/m5 --track 33 --cars BMWM5 --speed 40 --headless
py -3 tools/test_mvp_race_physics.py --godot $engine --project $project --output local/physics-probe/murcielago --track 13 --cars MURCIELAGO --speed 40
py -3 tools/test_mvp_race_physics.py --godot $engine --project $project --output local/physics-probe/elise --track 13 --cars ELISE --speed 30 --headless
py -3 tools/test_mvp_race_physics.py --godot $engine --project $project --output local/physics-probe/scale-m5 --track 16 --cars BMWM5 --speed 39 --distance 920 --schedules 30 60 --headless
py -3 tools/test_mvp_race_physics.py --godot $engine --project $project --output local/physics-probe/speed50-lambo --track 35 --cars MURCIELAGO --speed 50 --schedules 30 60 144 --headless
```

The extracted US retail SLUS-20362 ELF SHA-256 is
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
PCSX2 d75a0ad booted the retail disc with patch and cheat settings off.
PINE corroborated the retail code words at `0x00137C88` and `0x00137D90`.
The available save state had no active World at `0x00330288` and
fresh boot yielded no matched race trace. The reported PS2 60 FPS patch,
its exact speed/track/input reproduction, original-machine cadence and
vehicle response comparison remain open. The measured traces establish
render-rate independence for the sampled World fields and inputs.
Open coverage includes physical-device sampling during catch-up, full
multi-participant event ordering and per-retry collision substeps.

### TASK-0119 camera and collision follow-up

The MVP still defaults to `source`. The player, wheels, driving camera,
mirror and HUD blend on the same source fraction, with generation,
view and pose-reset checks. The cinematic camera retains the authored
two-step key interval. Fixed nodes continue using Godot interpolation.

Track 13 source-clock capture `local/task0119/source-pose` recorded 937
adjacent same-World-step pairs with rising interpolation fraction. The
player and main camera moved in all 937, and the mirror in 936. The fixed
`elise-ticket` capture recorded 56 airborne frames, 549 cinematic frames
and a completed ticket sequence by step 2676. The main camera moved in
all 272 same-step cinematic pairs. The 60 ms injected stall preserved the
captured particle IDs and PCM push ordering. The run counted 145 device
buffer skips across the session, so audio buffering remains open.

The source error log identifies address 76677984, section 41 record 8,
finite affine lanes and an infinite float determinant. The raw matrix was
unavailable. Debug `Smokeable::Move` `0x0021B688` calls the in-place
`eOrthonormalizeMatrix` `0x0011C108` before publishing the WorldObject.
See the retained EE/VU staging limits in `reverse/task0110/ortho-staging.md`.
The port-only regression uses a synthetic finite matrix with the same
overflow class. It covers first suppression, recovery, blinking, last
valid pose retention, generation isolation, and continued rejection of
other invalid transforms. Neither Track 13 drive reproduced the logged
record in the window.

```powershell
cmake --build --preset godot-release --target ps2_port entity_state_bridge_tests track_runtime_tests event_world_tests --parallel 10
ctest --test-dir build/godot-release -C Release --output-on-failure -R '^(entity_state_bridge|track_runtime|track_runtime_assets|track_environment|track_postprocess|event_world_original)$'
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --race-clock fixed --clock-1080p --clock-render-thread safe --clock-profile --clock-render-sample-limit 10000 --clock-effects-audit --clock-stall-ms 60 --clock-stall-on-signal --clock-stall-step 175 --render-cap 0 --clock-display-sync --drive-route --road-course-driver --delivery-pace --launch-only --drive-steps 5000 --brake-in-pursuit --result-timeout 140 --output local/task0119/elise-ticket
```

E-0118-C01, read-only Ghidra MCP, Debug NFSHP2.ELF (authority above).
Raw local capture: `local/task0118/ghidra-clock-evidence.json`, SHA-256
`DAA6243BE0ACCC4D33C01A6ACC5A40AE7ED5D34B3CE6E38EC30A22646ABDC281`.
MainLoop 0x0012F4C8 measures Count elapsed through bGetTickerDifference and
passes it to PrepareRealTimestep 0x001A0C50. Its frame-counter wait happens
after source clock advance and display. It gates pacing, while the measured
Count delta remains the input to PrepareRealTimestep. The latter uses scale
and packed feedback, and quantizes at 3600 before decoding. Debug data reads
from 0x003256CC through 0x003256DC are 3F82233A, 3CA3D70A, 3D75C28F,
3C888889 and 3D4CCCCE. These encode an approximately 1.0167 input multiplier,
PAL 20–60 ms clamps and NTSC 16.667–50 ms clamps. World::GetTimestep
0x0014D408 then uses the actual replay delta and source time scale.
PhysicsCar::Move separately computes its Euler substep count and duration
from the resulting World time. Source World updates are variable-step capable.
This evidence does not establish the dominant original-machine interval.

`py -3 -B -m tools.oracle probe --output local/task0118/pcsx2-probe.json`
timed out. No running PCSX2, available emulator command, or original-machine
trace was found. Continue with a matching Debug session and synchronized
observations at MainLoop/DoTimestep 0x0014D9A8, VBlank counter changes and the
PhysicsCar::Move integral substep conversion during active racing, chase
and NIS. Compare histograms before selecting a production cadence. No
PS2/PCSX2 or cross-platform numeric equivalence is claimed.

## Implementation and reproduction

`./build.ps1 --run --mvp --debug --race-clock=fixed --jobs 16` selects the
port-side 60 TPS diagnostic cadence. `source` remains the default. Render
frame elapsed never enters fixed World transactions. Godot controls the
catch-up budget, and the debug World panel shows source dt, alpha and catch-up
counts. Cars and wheel nodes interpolate natively. Source ResetCore stamps
vehicle discontinuities and generated/reused particles carry allocation
serials. The NIS bridge interpolates matching actor/clip bone samples once,
with Godot handling actor roots. Existing asset mappings and incomplete
source effects retain their prior scope.

```powershell
cmake --build --preset godot-mvp-debug --target ps2_port --parallel 9
cmake --build --preset godot-mvp-release --target ps2_port --parallel 8
cmake --build --preset godot-release --target event_world_tests real_world_clock_tests car_construction_tests entity_state_bridge_tests --parallel 8
ctest --test-dir build/godot-release -R '^(real_world_clock_original|world_clock_frame_original|world_clock_fidelity|event_world_original|entity_state_bridge|progress_tables)$' --output-on-failure
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --race-clock fixed --render-cap 30 --championship --launch-only --drive-route --drive-steps 560 --output local/task0118/championship-fixed-30
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --race-clock fixed --render-cap 120 --drive-route --road-course-driver --delivery-pace --launch-only --drive-steps 1480 --delivery-pursuit-retry --result-timeout 150 --output local/task0118/hot-pursuit-fixed-120
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --race-clock fixed --clock-fidelity --render-cap 30 --championship --launch-only --result-timeout 90 --output local/task0118/fidelity-fixed-30
```

For each subsequent cap use a fresh output directory and
`--clock-fidelity-reference local/task0118/fidelity-fixed-30/clock-fidelity.json`.
The native regression uses the same per-tick input sequence after countdown
under 30, 60, 120, 144 and 240 synthetic render schedules.

The Debug 30 FPS Championship window passed driving and source pause/quit.
The 120 FPS-capped Elise Delivery window ran at median 85 FPS in 240 sampled
render frames and passed source pursuit pause/retry/exit. In 118 consecutive
render-frame pairs sharing one committed Tick, 108 pairs had different
interpolated player positions. The output has distinct committed and visual
poses. The two captures do not establish full-rate 120/144/240 performance,
NIS replay equivalence, or isolated particle/audio timing through a stall.

The Release window fixture starts from the authored Championship and identical
fixture RNG seed after countdown. All five caps (30, 60, 120, 144, 240)
produce the same 300 named source snapshots and SHA-256
`2fc072831801c25112048935d06956766bc1e4cfdd5977084492b37d00f5595d`.
All copies of the DLL hash to
`00cac814c634a5efc0ebf6db1cc0e4921c72fbd06ef7153b05aaf3cbae0eecb2`.
Measured FPS for the five caps was 30/60/115/136/179 (30 FPS was read from
the final window probe since 300 ticks did not span 240 render frames at
that cap). The 120/144/240 cap captures contain 118/138/153 consecutive
same-tick frame pairs with different camera positions. The native harness
compares each named field and prints aggregate hash `741720bee0ead311`
for all five synthetic schedules. An earlier Release fixture reached all
300 measured ticks but kept advancing during quit and later hit the existing
0x0019A910 physics operand boundary. It now freezes at the committed sample
boundary. Lifecycle verification is covered by the separate keyboard runs.


The injected 180 ms render stall produced a 224.003 ms observed render gap.
Eight World ticks committed across that gap, matching the Godot catch-up
budget. The Debug Hot Pursuit window continued to step 3674, observed
level-three pursuit and returned normally. A separate normal-input test drove
fast enough to begin a chase, then applied the normal brake. It observed
83 sampled frames with active original ticket NIS (source steps 2437 to 2710).
The source returned to gameplay at step 2714 with one ticket, then paused and
quit normally. These runs establish lifecycle and no source stop over the
captured path. A dedicated particle/sound event timing comparison is still open.

```powershell
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --race-clock fixed --render-cap 60 --clock-stall-ms 180 --drive-route --road-course-driver --delivery-pace --launch-only --drive-steps 3650 --result-timeout 140 --output local/task0118/hot-pursuit-stall-ticket
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --race-clock fixed --render-cap 120 --drive-route --road-course-driver --delivery-pace --launch-only --drive-steps 5000 --brake-in-pursuit --result-timeout 140 --output local/task0118/hot-pursuit-brake-ticket
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --race-clock source --championship --launch-only --drive-route --drive-steps 140 --output local/task0118/championship-source
```

The explicit source-clock Championship launch, driving, pause and return smoke passed.

## 120 FPS presentation follow-up

The user restricted this follow-up to Godot and read-only Ghidra MCP.
Rechecked `MainLoop` 0x0012F4C8, `PrepareRealTimestep` 0x001A0C50,
`World::GetTimestep` 0x0014D408 and `PhysicsCar::Move` 0x00137D18 against
the same Debug ELF. The executable measures elapsed time and derives vehicle
substeps from the supplied World delta. Godot results establish the port's
60 host TPS schedule and the interpolation behavior. A normal-play original
runtime cadence distribution remains open, so the default is still
`source`.

The HUD stages candidate geometry and accepts it after the World presentation
barrier. Persistent Godot polygons interpolate matched live Flailer objects
within the same generation and screen. The tachometer can blend its
continuous fan UVs, while changed glyphs, page transitions and incompatible
geometry retain commit timing. Main and rear camera lens interpolation shares
the same fraction as the normal 3D poses. Source vehicle resets, POV changes
and newly visible vehicle parts invalidate stale transform history. The
standalone front end already ticks its source menu on every render frame.
Projective minimap polygons currently retain their source commit cadence.

E-0118-C02, Ghidra `MaxCameraMover::Update` 0x00159EC8 uses the integer expression
`(WorldTime - StartTime) / 2` to select an authored camera key. The first
120 FPS-cap ticket capture with the normal Godot transform interpolation
changed main-camera position in 135 of 272 same-World-tick render pairs.
The cinematic camera now interpolates across the original two-tick key
interval using committed samples, adding one render-side simulation tick
of history during that camera mode. StartTime changes and mode switches
reseed the history. Rechecked `eAnimationSolid::GetRotations` 0x001A5010
and `eAnimation::Render` 0x001A4230. Bone sample selection remains source
owned, with the existing separate render-side bone blend.

### Commands and local evidence

```powershell
cmake --build --preset godot-mvp-debug --target ps2_port --parallel 10
cmake --build --preset godot-release --target frame_builder_tests event_world_tests real_world_clock_tests car_construction_tests entity_state_bridge_tests --parallel 8
ctest --test-dir build/godot-release -R '^(frame_builder|real_world_clock_original|world_clock_frame_original|world_clock_fidelity|event_world_original|entity_state_bridge)$' --output-on-failure
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --headless --path build/godot-mvp-debug/project --script (Resolve-Path tools/test_mvp_hud_interpolation.gd)
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --race-clock fixed --render-cap 120 --clock-render-sample-limit 10000 --drive-route --road-course-driver --delivery-pace --launch-only --drive-steps 5000 --brake-in-pursuit --result-timeout 140 --output local/task0118/nis-camera-120-final
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --race-clock fixed --render-cap 120 --championship --launch-only --drive-route --drive-steps 560 --output local/task0118/hud-championship-120
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --race-clock source --championship --launch-only --drive-route --drive-steps 140 --output local/task0118/hud-source-championship
```

The Championship fixed and source Windows runs both passed menu launch,
driving, pause, quit and return. The preliminary Elise Delivery capture in
`local/task0118/airborne-ticket-120` sampled 54 all-wheel-airborne rendered
frames during source ticks 1881–1907. All 27 same-tick pairs advanced the
vehicle origin and heading and both camera origins. Maximum interpolation
residual against the committed origins was 0.000032 world units.
HUD geometry changed in 795 same-tick pairs. Median engine FPS was 120,
with 8.47/17.05/18.59 ms median/P95/P99 sampled render intervals.

The final Elise Delivery run passed original chase, ticket, return to driving,
pause, quit and return to the event tree. With Godot 4.7.2
`ed1daf0bf`, the final Debug DLL SHA-256 was
`e780f2a403a289c5603efb283dc4f679c860036d776426c3f6a25b2a5ad31ecd`.
Its 4,934 render samples report median engine FPS 120 and
8.28/17.03/18.22 ms median/P95/P99 render intervals. The capture has
48 airborne frames and all 24 consecutive same-tick airborne pairs changed
body position, orientation, and both camera positions. 810 same-tick
pairs exercised at least one interpolating HUD row. The 553 NIS samples
span ticks 2438–2715. The main camera moved in all 276 same-tick pairs,
and one sampled bone moved in 212. Both cameras' lens values stayed
constant in these samples, so dynamic lens transitions still need a
window capture. No Godot script errors appeared in the final run.

The headless HUD fixture passed two distinct render fractions, unchanged
discrete glyphs, an uncommitted candidate, and a generation reset. The six
targeted CTest checks passed, including the native 300-tick fidelity test.
Window input used the isolated keyboard driver and its focus fixture, not
a manual gameplay or display scan-out measurement. The two 120 FPS captures
used live inputs and different routes through traffic, so the cinematic
counts diagnose held keys rather than a matched performance A/B.

Open follow-up: capture fast camera projection changes and independently
compare particle/sound presentation across stalls. Diagnose the sampled
17–18 ms slow frames before claiming sustained 120 FPS. A complete original
runtime cadence comparison requires evidence beyond the authorized Godot
and static Ghidra verification for this follow-up.

## 1080p pacing, minimap and effects follow-up

Godot 4.7.2 Debug, Intel Arc B390, Vulkan Forward+, original-quality
presentation and WASAPI. The earlier 17 ms trace used a 640×480 window.
This follow-up separately verifies the live root viewport is 1920×1080.
The fixed World interval and source arithmetic/order are unchanged. No
PCSX2 was launched. The original-runtime cadence remains unmeasured.

- The original synchronous Python window probe costs about 4.7 ms on its
  slow state-write frames. Clock profiling uses a low-priority Godot writer
  and a bounded latest-state handoff. MVP `--debug` keeps the native
  debugger and bitmap overlay. Timestamp collection runs with explicit
  profiling or a bounded F8 capture.
- Warm 1080p Safe frames with one World tick previously spent 4.36 ms median
  in the complete callback and 1.34 ms in World publication on the common
  step range. Batched source Memory reads, compact status replies, and
  comparing raw scenery before decoding reduced redundant presentation work.
  Recovered `DoTimestep` phases are timed separately when requested.
- Godot Separate produced render-server synchronizations in the original
  collector and a renderer `finalize` thread error on teardown. The
  presentation-time limiter experiment averaged 117.9 FPS with 10.43 ms
  P95 and was removed. Safe and the normal Godot display synchronization
  remain the supported path for this capture.
- Projective map fans use source draw kind, source owner, mode, ordinal and
  clip-corner lineage. Only compatible committed pairs interpolate.
  Godot blends both UV numerators and q; the projective divide remains in
  the existing shader. Source geometry and visibility are retained.
- Godot's particle visuals use indexed birth IDs with native transform
  interpolation. Freed nodes retain earliest-slot reuse order and new
  births reset interpolation. PCM uses a 3,200-frame startup buffer and
  bounded Godot playback-rate compensation for source/host clock drift.
  The audit compares the pool against each committed World particle
  snapshot and checks the accepted PCM block IDs and contiguous offsets.

The 60 ms test injected at World step 175 observed particle activity and
nonzero PCM on all 25 surrounding committed ticks, with no additional
Godot generator underruns in that span. The first 180 ms signal test
injected at step 1918 and observed five nonzero PCM ticks in the same
window. All 25 PCM blocks arrived in order, with 51 generator underruns
in the span. The second trace wrote the audit but timed out on the
post-drive pause interaction. The probe can report accepted PCM queue
order and generator underruns. It does not capture audio hardware loopback.

The subphase capture isolated the extreme slow run around steps 1346–1392
to `DoTimestepCollisions`, including 536 ms of collision work across one
eight-step catch-up frame. The Voronoi read cache now reuses decoded original
shape geometry within a single collision pass. It invalidates at the
source shape rebuild, plane rebind and teardown entry points. Witness
updates, feature walks and contact responses still run in source order.

The first cache-enabled 1080p signal-stall window
(`local/task0118/collision-cache-180`) passed driving, pause and event-tree
return. For matching route windows of source steps 301–1849, the
Godot post-draw interval yielded 120.00 FPS average, 8.38 ms median,
10.59 ms P95, 13.18 ms P99, and 25.92 ms maximum. The comparison before
the cache yielded 111.09 FPS and a 565.59 ms maximum. On the collision
route around steps 1321–1409, the observed worst collision CPU time went
from 536.21 ms across eight steps to 6.40 ms in one step, with maximum
post-draw interval 25.92 ms. These are live input routes, so the numbers
do not establish a matched replay comparison.

The final 180 ms injection occurred at step 1881. The observed render
gap was 210.66 ms. All 25 nearby World ticks had live particles and
ordered PCM submission, with five nonzero PCM ticks. The Godot generator
reported 49 underruns around the injected gap. During this run 2,114
same-tick render pairs exercised at least one interpolating minimap fan.
The slow-frame budget and lossless playback across 180 ms remain open.

The clean run with effects auditing and stall injection disabled
(`local/task0118/collision-cache-normal`) passed driving, pause and
event-tree return. Debug DLL SHA-256
`6c446a410ffbf5343dc2a58829763d97446d60432f19660e4e0734aa643ee819`.
Across 2,791 draw intervals at steps 301–1699, it averaged 119.78 FPS,
with 8.32 ms median, 10.25 ms P95, 13.99 ms P99 and 29.46 ms maximum.
17 intervals exceeded 16.67 ms. Most coincided with a long Godot draw
span, several with delayed root GPU samples around 20 ms. The
remaining cause within the Vulkan/render pipeline is open. The
cache-enabled 60 ms signal test at step 175 again preserved all 25
PCM blocks and particle ID sets. It measured a 77.84 ms render gap
with zero additional generator underruns around that gap.

All 11 targeted Release CTests passed. The fixed World fidelity test
retained hash `741720bee0ead311` on all five render schedules for the
same 300 committed steps. The focused checks also exercised contiguous
and adjacent-region Memory reads, shape reconstruction at a cached
address, and projective HUD interpolation with a clip-topology change.
The current build and captures are PC evidence.

### Reproduction

```powershell
cmake --build --preset godot-mvp-debug --target ps2_port --parallel 10
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --headless --path build/godot-mvp-debug/project --script (Resolve-Path tools/test_mvp_hud_interpolation.gd)
$clockArgs = @('--debug','--entities-project','build/godot-mvp-debug/project','--race-clock','fixed','--clock-1080p','--clock-render-thread','safe','--clock-profile','--clock-render-sample-limit','10000','--render-cap','0','--clock-display-sync','--drive-route','--road-course-driver','--delivery-pace','--launch-only','--result-timeout','120')
py -3 -B tools/test_event_window.py @clockArgs --drive-steps 1780 --output local/task0118/collision-cache-normal
py -3 -B tools/test_event_window.py @clockArgs --clock-effects-audit --clock-stall-on-signal --clock-stall-ms 60 --clock-stall-step 175 --drive-steps 1540 --output local/task0118/collision-cache-60
py -3 -B tools/test_event_window.py @clockArgs --clock-effects-audit --clock-stall-on-signal --clock-stall-ms 180 --clock-stall-step 1880 --drive-steps 2250 --result-timeout 125 --output local/task0118/collision-cache-180
cmake --build --preset godot-release --target shared_collision_foundation_tests memory_budget_tests track_map_tests player_hud_presentation_tests world_particle_snapshot_tests frame_builder_tests event_world_tests real_world_clock_tests car_construction_tests entity_state_bridge_tests --parallel 8
ctest --test-dir build/godot-release -R '^(shared_collision_foundation|memory_budget|track_map_original|player_hud_presentation_original|world_particle_snapshot|frame_builder|real_world_clock_original|world_clock_frame_original|world_clock_fidelity|event_world_original|entity_state_bridge)$' -V
```

`clock-profile.json` row column 7 is the last completed Godot draw wall
timestamp in microseconds. Compare adjacent positive timestamp increments
for the stated committed step range and with 1920×1080 in columns 14–15.
The viewport CPU/GPU counters are delayed scopes. They do not measure
display scan-out. In Separate mode those scopes are omitted to avoid a
synchronous render-server query.

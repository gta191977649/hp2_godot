# TASK-0137 - Track 13 pursuit frame pacing

- Status: active
- Scope: measure and improve the first Ultimate Racer event at native resolution on the 120 Hz host.
- Provenance: port-side Godot presentation and performance tooling.
- Acceptance: three matching 30–45% pursuit captures per build, with average ≥119 FPS, displayed P99 ≤8.8 ms and missed refreshes <1%. Preserve authored rendering and simulation. Check Debug and Release builds, frontend smoke, Track 12 and Track 13 pause/resume.
- Related: TASK-0128 display pacing, TASK-0129 natural race roundtrip.

## Godot 4.7.2 baseline

The Coding-Solo Godot MCP identified the staged project and launched the
editor. Its 14 tools expose no Visual Profiler controls or capture export.
Used Godot's built-in `--gpu-profile` on the Debug Forward+ run, without
Computer Use or custom in-game timing code. Started at the default window
size through the normal frontend and switched to fullscreen on the first
driving frame. The host reported 2880×1800 at 120 Hz on Intel Arc B390.

The first button-steering run stayed at 0 m through step 902. The opt-in
native racer AI reached 3,816 m at step 2709 and 5,634 m at step 5434.
Between those marks, 43 Godot log reports had 4.948 ms median GPU total
and a 6.911 ms maximum reported total. Median reported phase values
were 2.387 ms opaque, 1.431 ms depth pre-pass, 0.342 ms transparent,
0.625 ms tonemap and 0.132 ms canvas (42 reports). Godot prints periodic
phase averages with a single sampled total. These figures do not establish
GPU P99, CPU attribution, actual displayed cadence or police proximity.
The observed window title dropped to 73 FPS during the run.
The AI reached the finish and advanced through Events Unlocked, Reward
menu, Race Results and Best Total Times. The smoke stopped at
`end_game_selection` because Up did not change the End Game Menu selection.
The results-to-event-tree return remains open in TASK-0129.

Artifacts are `local/task0137-ai-native/gpu.stdout` and `game.stderr`.
To repeat, set `HP2_MVP_PURSUIT=1`, `HP2_MVP_PURSUIT_AI=1`,
`HP2_MVP_DEBUG_CONSOLE=1`, a fresh `HP2_PROFILE_DIR` and
`HP2_MVP_SCREENSHOT_DIR`, and set `HP2_ROUTE_CAPTURE` to the TASK-0129
course. Run the pinned Godot executable with
`--path build/godot-mvp-debug/mvp-project --gpu-profile --script <absolute tools/test_frontend_mvp.gd>`.
For this baseline a local copy of the smoke switched to fullscreen at
race start. Starting in fullscreen caused the menu smoke to select Track
11 instead. The test helper lives in `src/mvp/tests` and is Debug-only.

Next: collect the editor Visual Profiler CPU/GPU lanes or an equivalent
capture before selecting a production optimization. Windows WPR GPU capture
failed with policy error `0xc5585011`. The 120 Hz acceptance remains open.

Build and script validation: CMake 3.29.2 and Ninja 1.12.0, Debug and
Release build presets with `--parallel 16`, and Godot 4.7.2
`--headless --path build/godot-mvp-debug/mvp-project --script <absolute tools/test_frontend_mvp.gd> --check-only`.

## Visual Profiler attribution

Godot 4.7.2's own remote debugger sent `visual:profile_frame`,
`servers:profile_frame` and built-in Performance monitor packets. A local
loopback receiver sent the editor's profiler enable commands and saved
the packets. No editor automation or in-game probe was used.
Reproduce with `py -3 local/task0137-visual/collect.py native-repeat`.
The collector uses the same local fullscreen-at-race-start smoke as the
GPU baseline. `native/marks.txt` correlates existing route stdout with
capture time. Raw capture and analysis are in `local/task0137-visual`.

The 3,592 reports in window 16540–20131 cover about 3.69–5.49 km of the
first event. Godot loop P50/P95/P99 were 8.417/13.097/14.659 ms.
Visual render CPU P50/P95/P99 were 3.346/5.776/6.862 ms and GPU
4.875/6.959/7.164 ms. For loops ≥11 ms versus ≤9 ms, render CPU
medians rose from 2.489 to 5.224 ms. The depth and opaque CPU scopes
rose from 0.776 and 0.579 ms to 1.710 and 1.421 ms respectively.
The built-in render monitor reported about 4,460 draw calls in slow
neighborhoods versus 2,114 in normal neighborhoods. These are periodic
monitor samples, with 12 and 20 samples in the two groups.

Dense 3D submission is the first sustained hotspot to test with an
opaque static placement batching A/B. The 34.858 ms worst loop still
requires attribution outside the Visual Profiler scopes. Godot GPU
readback is delayed and the report sequence is not a display cadence
trace. The script also incurred isolated route-mark logging overhead.
Displayed 120 Hz acceptance was not run.

## Static opaque scenery batching A/B

The Godot track presenter optionally uses one MultiMesh per authored
section and repeated mesh. Eligibility requires opaque, depth-writing
materials on every surface and positive transform determinant. Each
batch has a union of the original transformed mesh bounds. Individual
nodes still handle the sky, alpha/cutout materials, mirrored transforms
and single placements. The geometry, material and simulation inputs
are unchanged. `HP2_MVP_SCENERY_BATCHING=1` opts in.

On the same Debug binary at 2880×1800 and 120 Hz, two normal frontend
entries used the donor racer AI and Godot's remote Visual Profiler.
Comparing the same 3.69–5.53 km interval, A/B reported:

| Metric | A | B |
| --- | ---: | ---: |
| Engine loop P50 / P95 / P99, ms | 8.47 / 13.95 / 27.23 | 8.31 / 12.08 / 25.28 |
| Render CPU P50 / P95, ms | 3.93 / 6.50 | 2.99 / 5.32 |
| Render GPU P50 / P95, ms | 4.84 / 7.10 | 4.56 / 6.38 |
| Draw calls, periodic median | 3197 | 2814 |

B batched 1902 of 8282 placements into 152 MultiMeshes, removing 1750
placement nodes. At 4.50–4.90 km, engine loop P95 improved from 15.04 to
13.69 ms. At 4.90–5.30 km, GPU median rose from 4.74 to 5.04 ms.
AI trajectories and camera orientation varied. This is one pair of
distance-matched runs, and the profiler reports engine timing rather than
actual displayed intervals.

Reproduce by setting `HP2_MVP_SCENERY_BATCHING` to 0 and 1 for
`py -3 local/task0137-visual/collect_ab.py ab-a` and `ab-b` respectively.
Use fresh run names when repeating. Summarize with
`py -3 local/task0137-visual/ab_summary.py ab-a ab-b`.
Both variants reached the endpoint and wrote screenshots. Fixed-camera
Release Track 12 captures at 1280×720 differed in 557 of 921600 pixels,
with mean absolute channel differences below 0.006 on the 0–255 scale.
The moving Track 13 screenshots have different camera poses. Debug and
Release staged via `cmake --build --preset godot-mvp-<configuration> --target stage_mvp --parallel 16`.
With batching enabled, the Release `tools/test_track12_mvp.gd` window
smoke passed under Forward+ (8178 placements, 14 vehicles, 49.37 m).

Keep batching opt-in pending repeated Track 13 checks and a same-pose
comparison in the dense chase sector. The next A/B should check whether
finer batch bounds reduce the residual GPU cost at 4.90–5.30 km.

## Frame pacing and allocation follow-up

The measured scenery batching path is now enabled by default, with
`HP2_MVP_SCENERY_BATCHING=0` retaining the baseline for regression checks.
The Godot host disables synchronous voice-allocation traces unless
`HP2_MVP_AUDIO_TRACE=1`. MinGW Debug uses `-O2` with symbols and checks.
The bridge retains both vehicle snapshot buffers and validates the
small car table without an allocating set. The projective minimap
retains meshes whose submitted vertices, indices, colors and UVQ match.
Changed geometry still rebuilds with the authored projection.

The pre-reuse, trace-disabled full route recorded 332 engine loops
over 20 ms in 31,040 driving reports after the fullscreen transition.
The longest loop was 40.69 ms with Visual Profiler render scopes of
3.66 ms CPU and 4.41 ms GPU. Static batching and audio trace removal
alone therefore do not satisfy the pacing target. The police mesh is
built on new car admission or appearance change. No evidence yet
attributes the recurring spikes to that mesh construction.

Debug and Release staged successfully after snapshot and minimap reuse.
Continue with `py -3 local/task0137-visual/collect_quiet.py pacing-repeat`.
The quiet harness stores route marks until the race ends, eliminating
periodic measurement-side stdout flushes during driving.

The quiet reuse run reported 0 loops over 20 ms in 4,572 reports from
3.69–5.53 km (P95 11.60 ms, P99 12.78 ms). The earlier trace-disabled run
reported 37 such loops in 4,870 reports. Build flags, route timing and
logging also differ, so this does not isolate the effect of allocations.
Over the longer run, 42 loops still exceeded 20 ms, including two
fullscreen-transition frames. The donor entered Reward menu, while
the smoke's route-distance threshold stopped the results checks.

Police radio queues APPEND_CHAIN / OPEN_CHAIN, previously decoded
synchronously inside the Windows audio command pump. Those requests now
decode copied file ranges on a worker. The existing request pump preserves
per-voice order and owner-thread XAudio2 submission. This is a Windows
transport change. The authored pursuit and speech producers are unchanged.
The code path is confirmed, but attribution of the user's recurring
spikes to individual speech packets remains unverified.

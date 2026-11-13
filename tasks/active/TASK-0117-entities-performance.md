# TASK-0117 — Entities Bridge frame time

- Status: active
- Scope: Independent MVP frontend, Godot audio delivery, and race submission at 1080p Forward+ with current quality.
- Dependencies: TASK-0116 edition, TASK-0115 pilot, TASK-0077 timing scopes, TASK-0105 selection performance.
- Provenance: Host optimization only. Retain existing source menu and World behavior and the recovered section selector.

## Acceptance

- [ ] Windowed frontend P99 at most 8.33 ms and warmed transitions at most 16.67 ms.
- [ ] Normal audio device checks pass, with zero steady-state underruns and lifecycle cleanup.
- [x] Track 12 Championship and Track 11 FreeRun average at least 60 FPS and P95 at most 16.67 ms, with the existing quality.
- [ ] Matched input/state and visual checks cover sector boundaries, both views, shadows, dynamic scenery and HUD.

## Implementation

On MinGW, the MVP Debug preset now builds `game_core`, `port_runtime` and
the extension at `-O2`, with Debug symbols/assertions and strict floating-point
flags. The pinned Godot executable is 4.7.2.stable.official.ed1daf0bf.

The frontend retains up to six inactive Control layouts, updates bindings to
the current source generation and changes only affected widget properties.
It decodes an initial batch of original frontend textures off-thread and
uploads one warm texture per frame. The port retains the SHA-validated
immutable ELF image for independent copies and reuses event status resources
only for matching profile event blocks and language. The worker that decodes
original AVI and intro PCM owns a single pending request and completed frame.
Godot owns the playback streams and menu voice pool. Source music scheduling
remains on the main thread and worker completions carry the request revision.

The race decodes the original track off-thread and stages Godot resources in
roughly 2 ms slices while the loading page is active. The selector still uses
the main/rear union from the recovered HP2 selector. The presenter diffs the
resident sections and preserves shadows, transparency and both viewport sizes.
WASAPI playback receives committed source PCM without a per-sample GDScript
diagnostic loop and starts on its first real PCM block. Source command feedback
and PCM generation stay in World.

## Reproduction

All window measurements use the Intel Arc B390, Vulkan Forward+, a 1920x1080
client, uncapped presentation and VSync disabled. Both DLL variants are tested
with `--debug` and the HP2 bitmap telemetry visible. The harness runs private
projects and saves. Samples and screenshots remain under ignored `local/task0117`.

```powershell
cmake --build --preset godot-mvp-debug --target ps2_port --parallel 7
cmake --build --preset godot-mvp-release --target ps2_port --parallel 7
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --mvp-frontend-roundtrip --championship-performance --performance-uncapped --performance-final-screenshot --race-tape-record local/task0117/roundtrip-debug.tape --output local/task0117/retained-debug-roundtrip
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship-performance --performance-uncapped --performance-engine-breakdown --performance-final-screenshot --race-tape-replay local/task0117/track12-debug.tape --output local/task0117/retained-release
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --mvp-freerun-performance --performance-final-screenshot --output local/task0117/retained-freerun-release
py -3 -B tools/test_event_window.py --debug --audio-driver Dummy --entities-project build/godot-mvp-debug/project --championship-performance --performance-uncapped --race-tape-replay local/task0117/track12-debug.tape --output local/task0117/candidate-debug-dummy
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --championship --launch-only --loading-smoke --presentation-smoke --output local/task0117/lifecycle-debug
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-debug/project --mvp-frontend-roundtrip --championship-performance --performance-uncapped --performance-final-screenshot --race-tape-record local/task0117/verified-roundtrip.tape --output local/task0117/verified-debug
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship-performance --performance-uncapped --performance-engine-breakdown --performance-lifecycle --performance-final-screenshot --output local/task0117/verified-live-release
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --mvp-freerun-performance --performance-final-screenshot --output local/task0117/verified-freerun
local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --debug --path build/godot-mvp-debug/project --quit-after 2400 -- --screen=0 --debug-telemetry --background-movie-smoke
local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --verbose --debug --audio-driver WASAPI --path build/godot-mvp-debug/project --quit-after 900 --script "$PWD/local/task0117/intro_audio_smoke.gd" -- --debug-telemetry
cmake --build --preset godot-release --target entity_state_bridge_tests async_asset_cache_tests track_runtime_tests event_world_tests free_run_launch_tests frontend_actions_tests frontend_audio_tests frontend_jukebox_tests movie_audio_tests selection_event_resources_tests --parallel 8
ctest --test-dir build/godot-release -R '^(entity_state_bridge|async_asset_cache|track_runtime|event_world_original|free_run_launch_original|frontend_actions|frontend_audio|frontend_jukebox|movie_audio|selection_event_resources)$' --output-on-failure
py -3 -B reverse/scripts/generate_progress.py
```

`track12-debug.tape` was recorded in `candidate-debug-wasapi3` using the same
Championship test with `--race-tape-record local/task0117/track12-debug.tape`
and the ordinary single-pass frontend route. The driver sends Win32 messages
to the owned window. Tape replays validate camera, racer addresses and poses,
and race state at the World boundary. The Options/back/reselection route
needs its own tape. Replaying the single-pass tape after that route correctly
reported a different state on the first World step and was excluded.

## Evidence so far

- The original Debug Track 12 WASAPI sample measured 39.30 FPS and
  30.72/38.22 ms P95/P99. The initially measured baseline Release was much
  faster already. The Debug improvement comes principally from native compile
  optimization, with separate reductions in host work and loading stalls.
- `lifecycle-debug` passed 42 owned-window checks including authored loading,
  pause/resume, Options and Audio, retry confirmation and cancellation, another
  generation on retry, fullscreen/windowed transitions and return to Event Tree.
- The background movie smoke passed animation, replacement, same-clip reuse
  and invalidation. The startup fixture observed an original 512-wide MV30
  frame and original stereo 48 kHz PCM playing through the Godot device.
  Ten targeted native CTests passed. A matched Dummy replay
  before the final audio start change reported 167.75 FPS and 85 skipped mixes,
  so it did not substantiate audio as the cause of the original Debug slowdown.
- Legacy `cmake --build --preset godot-release --target ps2_port --parallel 10`
  passed. The rebuilt DLL and legacy project sources passed isolated Godot
  `--headless --path local/task0117/legacy-smoke/project --import` and
  `--headless --path local/task0117/legacy-smoke/project --quit-after 20 -- --screen=0`.
- The matching Release replay with incremental loading measured 202.74 FPS
  (P95 8.304 ms, P99 9.098 ms). The same tape preserves gameplay state while
  the faster consumer can accumulate PCM relative to the physical sound clock.
  Use the live elapsed run to interpret audio latency. Viewport timing includes
  delayed engine CPU/GPU timestamps, with P95 1.912/3.939 ms for the grouped
  engine viewport scope. The Track 12 load pump returned across 236 frames
  with a 7.58 ms worst pump. Track 11 worst pump was 10.74 ms. These are pump
  scopes and exclude other work on the same frame.

### Final live sample

The builds use MinGW GCC 13.2.0, CMake 3.29.2, godot-cpp `e4b7c25e721ce3435a029087e3917a30aa73f06b`,
and Python 3.14.4. Each race row contains 600 measured frames after 120 warmup frames.

| Scenario | FPS | Mean ms | P95 ms | P99 ms | WASAPI skips in sample |
| --- | ---: | ---: | ---: | ---: | ---: |
| Track 12 Championship Debug | 148.19 | 6.748 | 9.994 | 13.665 | 0 |
| Track 12 Championship Release | 186.14 | 5.372 | 8.766 | 9.565 | 0 |
| Track 11 FreeRun Release | 388.54 | 2.574 | 3.976 | 4.315 | 0 |

Final live Track 12 Release P95 was 2.612 ms for native World, 0.012 ms for
its audio command/PCM scope, 2.203 ms engine viewport CPU, and 4.463 ms engine
viewport GPU. The latter two use delayed engine timestamps and represent the
grouped viewport scope. The mean draw count was 1871 and the sample created
83 native nodes and zero meshes. Track 11 averaged 629 draws and created 243
nodes and one mesh in its route. Startup skips before active measurement were
5/4/7, respectively. The steady external PCM queue was empty in all three live
samples. An empty external queue can coexist with buffered samples inside Godot.
The Release WASAPI lifecycle passed pause with zero pending PCM, retry and exit.

The full Debug frontend roundtrip covered Options, Audio Options, back to main,
mode selection, Event Tree, car selection, return and car selection again, and
two source popups. It measured 8.847 ms overall P99. The Release single-pass
sequence measured 7.864 ms P99. Warm Debug returns to Event Tree and Select Car
still took 37.54 and 60.52 ms. A 112.18 ms cold car selection frame was retained.
The all-screen 120 FPS and transition gate remains open. Track load pumps spread
work across 227–579 frames. Track 11 had an individual 19.484 ms pump, so the
2 ms slicing budget remains an inter-instance budget, not a hard per-frame cap.
Full audio listening checks and matched cross-sector visual comparisons remain open.

## Next action

Use `--mvp-frontend-roundtrip` with a fresh output path. Split the remaining
event-tree and selection entry scopes into native admission, Control geometry
and first preview uploads. Record the exact frame and resource responsible for
every warmed transition over 16.67 ms before changing the source boundary.
Retain the union until matched section boundary and shared-shadow evidence can
support per-view masking or same-section static batching.

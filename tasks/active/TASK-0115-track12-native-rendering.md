# TASK-0115 — Track 12 native rendering MVP

- Status: active
- Scope: Class 5 Parkland Race, Championship tree 0/event 1, TrackID 12. Add an opt-in native Godot presentation backend over the shared authored World.
- Dependencies: TASK-0110 authored race closure, TASK-0077 benchmark and legacy renderer, TASK-0112 source camera and UI.
- Behavior authority: TASK-0096 Debug NFSHP2.ELF SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b` (executable revision unknown). Retail SLUS_203.62 SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2` remains the existing asset/frontend authority.

## Acceptance

- [x] Original track, scenery and all five authored vehicles render in Godot from persistent resources. Windowed native runs and screenshots in `local/task0115/native-smoke2` and `local/task0115/native-final-race`.
- [x] Godot Controls draw the live HUD from the committed C++ bridge. The windowed retry exercise displays the new HUD and rear view after restarting.
- [ ] Actual windowed native-backend run reaches natural finish with normal inputs and passes retry/exit and cleanup.
- [ ] Legacy smoke and focused native and presentation regression checks pass.
- [ ] Three matched, live 1080p Release runs per renderer/effects profile with reproducible input and elapsed tape, resource counts and render timings.
- [ ] Report whether comparable effects-off P95 is at most 16.67 ms in all three native repeats.

## Implementation and evidence

The authored Event Tree still hands the selected request to
`Ps2DrivingRuntime.configure_event_world`. The runtime owns the original
countdown, World, Replay, player and AI driving, collision, camera and race
outcome. The new C++ bridge and Godot presenter consume committed state.
The legacy backend is the default. See [build, launch and benchmark notes](../../docs/TRACK12_MVP.md).

Release DLL with native HUD and road alpha fix:
`1336c4b1959f7c8e2e788c34113960bdd87be7d0b336a6c058a58db4c9f11239`.
Godot 4.7.2.stable.official.ed1daf0bf, godot-cpp
`e4b7c25e721ce3435a029087e3917a30aa73f06b`, MinGW GCC 13.2.0, CMake 3.29.2,
Python 3.14.4. Window runs use Vulkan 1.4.359 on Intel Arc B390.

```powershell
cmake -S . -B build/godot-release -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0115/bin
cmake --build build/godot-release --target ps2_port entity_state_bridge_tests --parallel 8
ctest --test-dir build/godot-release -R '^(entity_state_bridge|driving_schedule|event_world_original|race_finish_original|frontend_finish_original|authored_championship_clock_original|track_runtime|mesh_prepare)$' --output-on-failure
cmake --build build/godot-release --target entity_state_bridge_tests shared_collision_foundation_tests collision_sound_tests --parallel 4
ctest --test-dir build/godot-release -R '^(entity_state_bridge|shared_collision_foundation|collision_sound_original|progress_tables)$' --output-on-failure
```

Both CTest sets pass (8/8 and 4/4). The lifecycle fixture covers stale and
discarded commits, rejected poses, spawn/despawn, reused addresses, restart
generations and HUD operands. The initial window controls run passes physical
throttle, brake, handbrake and gear changes, pause/resume, mouse camera return,
and return to the Event Tree.

The first matched effects-off comparison predates the native HUD and road
alpha fix. Over source steps 177–776, legacy mean/P95/P99 were
54.821/86.552/113.629 ms, native 9.245/11.483/14.447 ms. Timing evidence is
under `local/task0115/{legacy-off-record,native-off-replay}`. The different
rendering and UI workloads are visible in those captures. Use the final matrix
for the migration decision.

The windowed Forward+ effects-on run exercised the authored pause, option
pages and retry, then completed two laps under the normal physical keyboard
path. At committed step 24082, 11:15.57 into the race, the next native tick
hit the generic exceptional `cvt_w_s` guard tagged `0x0018D880`. The guard's
diagnostic anchor does not identify the actual caller for this run. Source
cleanup on this failed trajectory also hit the mover destruction guard
`0x001316F8`. Retain both failures. No finish or save was produced. Artifacts
are in `local/task0115/native-final-race`. The existing normal retry and
abort path is separately covered by window tests.

The user ended testing and requested a build-script shortcut. Use
`./build.ps1 --run --mvp --preset godot-release`. The final timing matrix
remains incomplete. One attempted baseline was rejected because Windows
maximized the race viewport to 2880×1695 during capture. A second attempt also
lost the required viewport dimensions before export and its owned Godot
process exited. Neither attempt is included in the measured comparison.

The user's Debug profile confirmed that `--run --mvp --debug --jobs 16`
passes `--race-renderer=native` to Godot. Its captured frames belong to
the unchanged frontend. Startup now prints the qualifying event and the
race reports which renderer it selects, with track and event IDs.

## Next action

If testing resumes, restore a stable windowed 1920×1080 capture before running
the matrix in the guide. Retry the natural finish with the previously accepted
localized-road driver. If the conversion guard recurs, capture the actual caller
and operand before considering an address-specific recovery. Keep full visual
parity and PS2 equivalence open.

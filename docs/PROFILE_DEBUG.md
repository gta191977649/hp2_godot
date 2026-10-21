# Global live ImGui profiler

Run `./build.ps1 --profile`. The global autoload is
`godot/scripts/debug/profile_debug.gd` (`ProfileDebug`), shared across frontend,
TrackViewer and the actual driving/race scene. Rendering uses the existing native
ImGui frame, not Godot Control/CanvasLayer widgets. Normal standalone launches
do not collect or show the profiler. The window can be dragged, resized, scrolled
or collapsed, using the same DPI/input rules as the other ImGui tools.

## Live tools

- CPU/frame-wall, GPU-viewport, draw/primitive and function-hotspot categories.
- Up to240 recent frames; Pause freezes the display, not the game/simulation.
  Inspect a frozen frame with the slider; Resume returns to live data.
- Clear resets graph history. Export120frames starts a fresh bounded recording
  and displays the output path. The live graphs continue after JSON recording ends.
- Functions shortcut shows sorted function/section duration, calls and maximum
  call via hover. Script entries include frontend drawing, viewer process, game
  process/presentation/status and native driving tick boundaries. Existing native
  track render/cull/particle/Sun section timings remain visible in the game.
- The collector also publishes16 HP2 custom Performance monitors for editor use.

Arguments for direct engine launch: `--profile-debug`,
`--profile-debug-output=<absolute JSON path>`, `--profile-debug-frames=<1..600>`
and `--profile-debug-no-ui` (data-only captures). Old track-profile argument
spellings are accepted as compatibility aliases, not the public name.

To instrument another synchronous script function:

```gdscript
func update_something() -> void:
    var _profile_scope := ProfileDebug.scope("MyNode.update_something")
    # Existing function body, including early returns.
```

The returned RefCounted records duration when the local reference is released.
For an individual synchronous call, `ProfileDebug.call_traced(name, callable,
arguments)` returns the original result. Do not hold a scope across `await` and
interpret the wait as CPU execution; scopes deliberately measure wall time.
Disabled scopes return null without allocating a timing object.

## Reading the data

Function scopes are **inclusive wall duration**, not exclusive CPU time or a
complete engine/native sampling call graph. Parent/child entries overlap. Existing
native section rows explicitly identify their source; Sun readback is a previous
completed readback and may include GPU waiting. Uninstrumented functions need a
scope or a native sampling profiler; this UI does not invent their timings.

TASK0072 reads completed RenderingDevice viewport timestamp endpoints together
on the render thread, with their completed `source_frame`. This is a separate
clock domain from the CPU sample's `engine_frame`; delayed viewport workloads
must never be summed as total frame GPU time. Each completed frame is consumed
once. Missing endpoints, repeated completion frames and invalid intervals are
N/A and graph gaps. The former unversioned viewport cache could repeat old values;
the collector no longer treats that cache as fresh evidence. Draw counts include
the profiler's own ImGui drawing; DebugTools time includes its UI cost.

Schema2 exports include full numerical camera position/basis/projection, effects,
effective clipping planes, renderer generation, optimization controls, source
identities and launch flags. These CPU-context fields are not delayed GPU state.
Optional `--sun-gpu-telemetry=true` reads the production counter asynchronously
every30process frames. A count is exposed only when its RG count bytes agree and
its in-texture BA producer-frame stamp matches the requested source. Its
`source_frame_domain` is `engine_process_frame`; count/age remain N/A when
unavailable or stale. This readonly diagnostic never supplies native Sun state.

## Async export candidate

`--profile-async-export=true` independently enables asynchronous export; the
project fallback `hp2/profiling/async_export` remains false until performance
acceptance. Explicit `=false` retains synchronous export. The separate compact
history and bulk UI upload candidates use `--profile-compact-history=true` and
`--profile-ui-bulk-upload=true`; neither is implied by async export.

The main thread clones an owned capture snapshot, records
`export.snapshot_clone_ms`, and stops collecting that capture. One low-priority
worker validates that the snapshot contains no Object values, serializes JSON,
writes/flushes a temporary file and renames it to the requested output. The worker
never accesses a scene, camera, renderer or active Resource. Source SHA256 values
are cached at enabled-profiler startup, before benchmark warmup; exported
`source_identity.hash_capture_phase` records this difference from synchronous
export. Samples and camera/effect values still belong to the capture.

`export_profile()` returns the requested path immediately for accepted async
work; the file need not exist yet. Repeated calls for the pending capture return
the same path without cloning again. `exported` becomes true only after successful
write completion is polled on the main thread. `ui_state()` also exposes
`async_export`, `export_status`, `export_pending`, `export_error` and `exported`;
pending work is not displayed as a newly saved path. Status is idle, queued,
writing, complete or failed. Errors leave `exported=false`; a caller can retry
after correcting the output path.

New captures may start while earlier snapshots are written. Capture generations
prevent an older completion from completing a newer recording. The queue holds
at most four waiting captures plus one active worker; overflow stops collection
and reports failure. Normal-frame polling never waits for a live worker. Node
teardown explicitly joins/drains its owned jobs, so closing the application can
wait for outstanding file writes. Benchmarks must await completion outside timed
samples rather than joining a worker inside their frame loop.

The headless600-sample synthetic ownership fixture measured2.738ms for one
snapshot clone. This is not an actual Track11 capture cost, FPS gain or proof of
the16.67ms frame budget. Serialization/file work may still contend for CPU/storage;
measure the enabled candidate on the intended route before enabling its default.

```powershell
& local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --headless --path godot --script res://scripts/debug/task0072_profile_export_smoke.gd
& local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --headless --path godot --script res://scripts/debug/profile_debug_contract_smoke.gd
```

Both pass on Godot4.7.2. Export coverage includes queued captures, mutation/clear
isolation, pending-request deduplication, completed-file replacement, error/retry,
Object rejection, shutdown draining and temporary-file cleanup. The two injected
export failures deliberately print error messages while the fixture exits0.

## Validation

TASK0070 normal Debug build,4.7.2 import and strict host-only source successor pass.
Graphical/headless ImGui smoke checks global no-track sampling and actual injected
pause/clear/export/function buttons. Frontend capture contains
`FrontendNative._process`/`DebugTools._process`. A Running game fixture requires
`RaceScene._present_frame` and `Ps2DrivingRuntime.tick`, renders the actual ImGui
overlay and records20frames after loading. The full native race runtime smoke
(focus, restart, failure, scene exit/re-entry) also passes with profiling enabled.
Private screenshots/records are in `local/task0070/`; extracted game images are
never checked in. Frozen TASK0069 source review bytes remain unchanged.

TASK0071 now selects Vulkan/Forward+ by default. The same profiler and native
ImGui frame work on that backend; JSON includes `rendering_method` alongside
actual driver/API/adapter identity. Missing GPU values remain unknown, and
viewport timestamps are never an additive frame total. The deterministic
benchmark supports aligned collection after warmup and separate per-repeat
exports. See [Track11 results and remaining visual gaps](PERFORMANCE_TASK0071.md).


## TASK-0072 worker diagnostics

Profile context retains per-view render_preparation_views with source frame, scene
generation, requested job limit, observed concurrency and snapshot/dispatch/prepare/
wait/packing/upload/cache-commit CPU times. job_sum_ms overlaps wall time and must
not be summed. Threaded preparation changes neither GPU timestamp provenance nor
physics ticks. See [integration and matrix commands](RENDER_PREPARATION_TASK0072.md).


TASK0072 CPU submission candidate: `--track-submit-math=true` adds per-batch
host arithmetic to the same WorkerThreadPool group as mesh misses. Default false;
worker0/2/4 and Safe/Separate remain separate controls. Rendering stays ordered.
Per-view preparation exposes draw_snapshot_ms, submit_math_entries/tasks,
inverse_cache_hits, submit_math_fallbacks and peak_concurrent_jobs. Geometry
snapshot_ms excludes the separate draw snapshot. Worker accumulated time is not
frame wall time; dispatch/prepare/join overlap. View CPU rows carry source_frame;
old preparation snapshots must be attributed to their saved source frame.
Vehicle staging reports bulk snapshot reads separately from uniform reads.

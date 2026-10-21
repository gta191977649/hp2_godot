# Godot4.7.2 profiler integration and current render bottlenecks

TASK0070 follow-up: the collector is now the global `profile_debug.gd` autoload
and `--profile` draws a live native ImGui panel across game/frontend/viewer scenes.
See [current interface and tracing](PROFILE_DEBUG.md); the original measurements
and source-review snapshots below are historical evidence, not refreshed baselines.

## Delivered interface

Default engine: official **4.7.2.stable.official.ed1daf0bf**. Normal build/run,
CTest, frontend/observer launchers and pipeline tools select this runtime. There
is no4.4.1 comparison mode. The existing godot-cpp commit and minimum4.4 API ABI
are retained: the runtime upgrade does not require an unnecessary bindings/API
upgrade. New runtime loading and rendering tests are the compatibility evidence.

```powershell
.\build.ps1 --run      # normal frontend, active API suffix in window title
.\build.ps1 --profile  # same frontend + built-in GPU profile + HP2 monitors
.\build.ps1 --editor   # F5, then Debugger > Profiler / Visual Profiler / Monitors
```

The title uses the **active** runtime driver, not the requested project setting:
`... (OpenGL)`, `... (DX12)`, `... (Vulkan)` or `... (Metal)`; ANGLE reports
`OpenGL / DX11` only if its adapter/API text identifies that backend. A GPU smoke
actually observed `HP2 PS2 Native Port (OpenGL)` on the Intel Arc B390.

Editor-attached runs and `--profile` register14 `HP2/` Performance monitors:
native preparation, host/main/post/particle submission, particle simulation,
packing/upload, previous Sun readback, draws/primitives, byte/read deltas and
collector overhead. The normal standalone run does not enable this collector.
The collector records up to120 post-warmup samples, prints
`TRACK_PROFILE_EXPORTED <path>` and writes JSON under `user://debug/` by default.
`--track-profile-output=<absolute path>` overrides the destination. Metrics remain
live in Monitors after the bounded recording stops. Restarting a recording is
available through the port-only `TrackProfiler.begin_capture(warm, frames)` API.

Godot Visual Profiler gives engine rendering stages. Ordinary Profiler covers
scripts/main-loop categories; it does not give a complete native extension call
graph. Use native sampling if the remaining C++ work requires deeper attribution.
Relevant official documentation:
[debugger/profilers](https://docs.godotengine.org/en/4.7/tutorials/scripting/debug/debugger_panel.html),
[release](https://godotengine.org/article/maintenance-release-godot-4-7-2/).

## Bounded observations, not FPS acceptance

Track11 uses the exported HP2 position **(-675.7699,245.3238,-44.4960)**,
complete saved basis/FOV and effect controls. Deterministic initialization uses
60 visual steps at1/60s, then a frozen visual timestep. This is not a checkpoint
of the user's exact transient particle/history state. Each main report contains
60 samples after a separate warmup; no long FPS benchmark was run.

Private data: `local/task0069/warm-profile-2880.json`, `profile-1080.json`,
`profile-sun-reduction.json`, `profile-fog-off.json`, corresponding engine
`--gpu-profile` logs under `local/task0068/`, and source images/snapshots under
`local/task0069/`. Extracted/captured assets are not checked in.

| Measurement, median | Actual2880x1695 | Separate1920x1080 |
|---|---:|---:|
| Native CPU preparation |3.08ms|3.13ms|
| Host CPU submission |3.69ms|3.82ms|
| Main CPU submission, included in host |2.25ms|2.37ms|
| Particle CPU submission, included in host |0.38ms|0.40ms|
| Previous Sun synchronous readback wall time |5.78ms|3.41ms|
| Colour viewport GPU measurement |4.38ms|3.20ms|
| Encoded-depth viewport GPU measurement |1.78ms|1.40ms|
| Sun readback target GPU measurement |0.019ms|0.028ms|
| Collector CPU overhead |0.14ms|0.15ms|

Engine totals around605/609 draws and51.9k primitives include more than the world
mesh. Draw/primitive counts are not pixel coverage. Native/host times are Debug
CPU wall measurements; some CPU-only validation ran concurrently, so do not use
these numbers as production FPS acceptance. Never add the nested CPU categories.

Viewport timestamps describe earlier completed GPU work and are not all aligned
to the currently scheduled ping-pong target. History0 had no usable nonzero GPU
measurement; history1 produced inconsistent values, including176.65ms in the
reduction experiment. **History GPU cost is unresolved**. Keep raw observations;
do not average these anomalies into a trustworthy postprocessing budget. Do not
sum viewport times or derive GPU time by subtracting CPU time from frame time.
Generic CLI GPU-profile task averages can cross loading/warmup boundaries too.
Early cold reports had substantially higher GPU times than the warmed report.

## Attribution and next optimization decisions

1. **Sun synchronization is a material serial dependency.** Its GPU copy/count
   target is cheap, but `get_image()` blocks for several milliseconds, consistent
   with waiting on preceding rendering. The existing reduction experiment changes
   the transfer4096->16bytes while preserving the current-pose colour, depth,
   final pixels and native packet digest. Readback median5.78->5.69ms provides
   no meaningful evidence of a stall improvement. Keep reduction default-off.
   A future improvement must overlap or remove the synchronization dependency
   while preserving the original visibility result's availability/order; do not
   introduce an extra frame of occlusion latency as an alleged optimization.
2. **Colour and duplicate encoded depth are real GPU workloads.** The built-in
   profile reports substantial `Render 3D Transparent Pass` time. This identifies
   where to inspect alpha-test/blending shader state and fragment invocations;
   it does not prove overdraw or vertex bandwidth alone is the cause. Repair the
   generic opaque-queue counterexample before retrying Copy/cutout shader variants.
   Preserve native transparent ordering and depth-writing roads. Ordinary screen
   depth remains an incorrect replacement for the encoded pass.
3. **Fog/history cost is not yet isolated reliably.** The fog-off diagnostic did
   not demonstrate a reproducible GPU saving; colour/depth readings varied across
   runs and history timing was inconsistent. Retain the consumer/history semantics.
   Next use an aligned Visual Profiler frame plus GPU capture to isolate the
   composite shader, then test exact effect-combination specialization without
   changing integer blend/alpha/palette operations. The known half-resRGBA8 focus
   precision counterexample remains a reason not to enable staged focus.
4. **CPU caches remove repeated work globally.** Detailed batch statistics run
   only for detailed export; section ownership is indexed once per loaded scene;
   exact plans are reused until the full descriptor changes. Depth materials use
   notifications and particle staging is reused. Independent fallback arguments:
   `--track-submit-cache=false`, `--track-depth-param-cache=false`,
   `--track-particle-staging-cache=false`. Keep full first-bind/unknown-shader sync,
   native command generation and scene/config invalidation. Particle simulation
   and packing were small in this fixture, so replacing the simulation or adding
   matrix-texture indirection is not justified by these observations.

No optimization depends on a track ID. Actual resolution has2.35times1080p's
pixel area; the1080p measurement is a separate target, not a silent quality change.
No stable60FPS, PS2 equivalence or universal GPU-timing accuracy is claimed.

## Validation

- Normal Debug build and Godot4.7.2 headless editor import pass.
- Normal frontend boot reports the expected4.7.2/OpenGL runtime and exits0 in
  bounded startup checks. Forced early exit with `--quit-after` reports two
  retained audio objects (`AudioStreamWAV`, `AudioStreamPlaybackWAV`); this is
  a known shutdown-cleanup limitation, not a clean-exit assertion. Normal user
  close behavior has not been established by that forced-exit check.
- Native batching/cache and headless controls/mesh/diagnostics checks pass.
- Godot4.7.2 GPU particle, raw particle mesh reuse, Sun, postprocess and
  postprocess lifecycle tests:5/5 pass.
- New title/custom-monitor headless smoke and actual OpenGL window smoke pass.
- TASK0069 exact source successor passes, including inherited auxiliary pins;
  predecessor/global-policy and TASK0063 framework checks pass.
- All30 tracks load in the native CPU census:60 records, props-on/off. This is
  static submission coverage, not30-track visual/GPU or driving equivalence.
- Current-pose Sun reduction matches encoded depth/colour/final images exactly;
  nearby pose/native-packet comparison and cache fallback checks are recorded in
  the private result summary. Existing Track24 colour discrepancy is unchanged
  debt, not waived by this runtime upgrade.

# TASK0063 rendering cost attribution

Status: completed for the revised user scope: static/native tests and drawcall
statistics. The user stopped long FPS benchmarking. No60FPS or total-GPU
<1000draw acceptance claim is made.

## Contract

Intel Arc B390 / Godot4.4.1 /1920x1080, original effects, normal native LOD.
User goals: P95<=16.67ms and total all-pass draw calls<1000. Report misses.
Props/effect overrides are diagnostics, never a production quality reduction.

Original evidence and host-cost distinctions: reverse/task0063/cost-model.md
(E0063R01). Exact port-only source preservation: E0063H01 and successor review.

## Implemented options (independent)

- Default ON, `--track-particle-state-cache=true`: transaction copies mutable emitters,
  groups and RNG only. Native original update/call order unchanged.
- Default ON, `--track-particle-transport-cache=true`: cache pinned stream layout until
  growth, skip unused CPU indices, cache unchanged material parameters.
- Default ON, `--track-post-consumer-cache=true`: cache unchanged texture parameters;
  expose actual target/consumer activation. No skipped depth/history semantics.
- Default OFF, `--track-sun-reduction=true`: integer count of the exact original clipped
  depth<2 rectangle on GPU;2x2/16-byte transfer instead of32x32/4096bytes.
  Completion cadence unchanged. `--track-sun-reduction-compare=true` schedules
  the reference alongside it and rejects any unequal count (validation only).
- Existing `--track-opt-opaque_queue=true`: experimental Copy-blended cutout
  eligible for the opaque queue after its original alpha discard. The separate
  occluder rule is deliberately unchanged. Pixel/depth parity required.

## Static census results

All30 tracks inspected (60 Props on/off rows). See [full table](TRACK_DRAW_COUNTS.md).
These counts are one marker0-relative port camera per track, not full-route maxima.

| Track | Original commands | Main colour batches on/off | Conditional world colour+depth+env on/off |
|---|---:|---:|---:|
|11|990|153 /42|287 /77|
|12|2180|993 /49|1947 /97|
|24|1123|400 /35|792 /64|
|42|2652|925 /51|1847 /102|
|61|2883|614 /47|1220 /94|

Conditional counts exclude capture faces, vehicle, sky, particles, Sun, shadows,
and full-screen stages; they are not actual total GPU draws. Track12/42 already
exceed1000 conditional world passes in this fixed view, before those producers.
The unchanged native culling reduces loaded instance sets first, then batching
reduces surface commands; props transformations/state still split many batches.

Track11 split context:102 matrix,66 texture,10 lighting differences among152
state boundaries (overlapping reasons). Proposed Copy-cutout opaque eligibility
covers623 commands; it improves queue eligibility, not necessarily draw count.
Conservative instancing plans predict18/253/221/293/193 fewer colour batches for
tracks11/12/24/42/61 respectively, but earlier Track24 pixel mismatch still keeps
instancing OFF. Planner savings are not measured or accepted GPU savings.

Default cache choices remove proven redundant copies/indices/queries/setters;
180-step native state, raw buffer and15image parity support their semantics.
They do not reduce the world batch counts above and are not a measured FPS win.
Every cache can be independently reverted with its `=false` startup switch.

## Fast reproduction (no Godot window or FPS run)

```powershell
cmake --build --preset godot-release --target track_draw_inventory -j4
./build/godot-release/track_draw_inventory.exe --self-test
./build/godot-release/track_draw_inventory.exe --assets
```

Output is JSONL: native commands, host batches, depth/environment counts,
instancing eligibility, split causes and static particle resource counts.
`actual_gpu_draws` is explicitly null. Native original asset preparation and
camera/frustum self-tests run without launching Godot.

## Optional instrumentation (not run as final acceptance)

`get_state().costs` separates main submission, sky/shadow environment work,
reflection-view preparation/submission, postprocess, Sun and particle submission.
`previous_sun_readback_ms` is the previous completed callback, not same-frame GPU
time. Particle stats separate simulation, packet, packing and CPU upload calls.
Batch split reasons are nonexclusive counts; they are not GPU draw counts.

`tools/run_track_costs.ps1` launches the actual GUI executable in Normal window
mode to avoid inherited minimized console startup. Timed loops reject minimized
windows, disable VSync/caps and log progress. Keep the window visible.

```powershell
./tools/run_track_costs.ps1 -ProjectPath local/task0063/project `
  -OutputDirectory local/task0063/reference-start -Scenario all `
  -Tracks 11,12,24,42,61 -WarmFrames 120 -Samples 600 -Repeats 3
```

Forest fire: `-Tracks 11 -Pose forest_fire`. Resolution diagnostic: `-Height 720`.
Backend diagnostics: `-RenderingMethod mobile -RenderingDriver vulkan` or
`-RenderingMethod forward_plus -RenderingDriver vulkan`. No backend default change.

Analyze JSON with `reverse/scripts/analyze_track_costs.py`, `--output` JSON and
`--markdown` Markdown. It preserves unavailable GPU timers as null, separates
backend/pose/native build/candidate flags, reports conditional effect costs and
interactions, and rejects minimized/incomplete/duplicate routes.

## Validation so far

- Debug/Release builds pass. Native particle state/RNG/RenderEffects parity over
  180 varied steps, rollback after prior state advancement, and switch-back pass.
- All30 original tracks load/prepare through native runtime (private log).
- Actual GPU mesh byte transport, forest-fire packet/permission, postprocess
  numerical/lifecycle and Sun real-road occlusion fixtures pass with candidates.
- Fifteen current-run reference/candidate captured PNGs are byte-identical.
  Sun reduction comparison reports zero mismatches in exercised clear/occluded views.
- Analyzer11unit tests pass. Source successor retains frozen predecessor hashes.
- Final Debug9/9 and Release9/9 targeted native/source checks pass; successor
  framework19cases passes. Native particle original-math parity also passes.

## Retained limitations

Encoded-depth geometry capture remains: ordinary screen depth omitted an existing
transparent depth-writing road. Fused Focus remains: an RGBA8 staging experiment
has a precision counterexample before actual GPU validation. Full-size history
availability/scratch alpha remain because later BW/previous-frame effects depend
on them. Existing material-identity and MultiMesh pixel failures remain binding;
no previously rejected broad sharing/instancing is enabled by this work.

## Remaining optimization priorities

1. Preserve order while reducing transformed-prop submission/state changes.
   Attribute matrix/light/texture split causes before widening batches.
2. Resolve previously observed MultiMesh/material-sharing pixel differences before
   enabling instancing. A lower planner count is insufficient proof.
3. Reuse actual encoded-depth output rather than redraw the world only through a
   validated rendering integration. Vanilla screen-depth replacement is rejected
   by the transparent-road counterexample; no guessed depth/alpha substitution.
4. Exact Copy cutouts may use the experimental opaque queue, but ordering/depth
   tie parity must be checked before promoting it. Occluders still exclude holes.
5. Keep original particle spawn/clip/pixel-size rules; avoid GPUParticles replacing
   recovered RNG/lifetime. A future float intermediate could reduce repeated DOF
   evaluations, but the RGBA8 counterexample blocks a blind staged rewrite.

Compatibility remains the default. Mobile/Forward+ timing/captures and full matrix
runs were stopped/deferred by the revised user request; no backend gain claimed.
Portable RenderDoc preparation remains only in ignored local storage; no capture
was performed. No upgrade/system graphics layer registration occurred.

Minimized pilot files `matrix11-reference.json`, `reference-fire/` and
`visible-probe/` are excluded from performance evidence. The earlier permissive
pilot did not yet reject minimized timing. Only visible-window guarded runs count.

The guarded `reference-start/` run was stopped by the user after4complete rows.
Its metadata is marked cancelled_by_user; the analyzer rejects it as full matrix
acceptance. Those limited Props-off rows do not characterize all-effects output.
Debug/Release built successfully; normal `./build.ps1` reconfigured and linked the
workspace DLL path, so `./build.ps1 --run` picks up the selected caches.

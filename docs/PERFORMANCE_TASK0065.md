# Track11 dense-view submission optimization (TASK-0065)

Historical TASK0065 rollout below is superseded by [TASK0066 global enablement](PERFORMANCE_TASK0066.md): there is no current track allowlist. Original measurements/rejected image findings are retained.

Godot4.4.1 / Intel Arc B390 /1920x1080. No prolonged FPS benchmark was run.
Native Track11 camera position: (697.04,141.24,-17.98). The screenshot did not
supply its basis/FOV, so these are explicit diagnostic headings, not a claim to
reproduce its1510-draw frame exactly. Effects/Props/distance/native LOD stay on.

## Accepted default

The optimized profile enables matrix-attribute batching for **Track11 main views
without a car visual consumer**. Other tracks, capture views, car consumers,
nonidentity renderer transforms and static LOD inspection retain original
submissions. Explicit `--track-opt-world_batches=false` disables it;
`--track-opt-world_batches=true` opts into the experiment on other tracks.
Compatibility remains the default backend; MultiMesh/opaque queue remain off.

Adjacent unlit Copy commands can share a submission even with different matrices.
Equal texture/material/effect/light context, native LOD and section ownership
remain required; alpha/environment/fusion/bias states are barriers. Original local
attributes and triangle order stay intact. CUSTOM1..3 carry matrix rows evaluated
in the shader. The same geometry/shader serves encoded depth, preserving discard
and transparent road writers. No ordinary screen-depth substitution was made.

CPU baking was rejected after a one-pixel depth discrepancy; retaining GPU matrix
arithmetic resolves the exercised Track11 views. Matrices are part of cache keys;
raw/matrix node variants are distinct. Chunks cap at65536 vertices and reuse the
existing64MiB unreferenced LRU policy; active references may exceed that limit.
Matrix transport adds48bytes per vertex. This reduces submissions at a memory/
vertex-bandwidth cost; no FPS percentage is inferred from draw reduction.

## Actual engine totals with final default

All seven baseline/candidate pairs have identical native packet/selection data
and byte-identical final, colour and encoded-depth images at1080p.

| Heading in Godot axes | Before draws | Default draws |
|---|---:|---:|
| Forward (0,0,-1) |2409|669|
| Right (1,0,0) |86|84|
| Back (0,0,1) |68|68|
| Left (-1,0,0) |717|381|
| Normalized (-0.4,-0.1,-1) |3003|689|
| Normalized (0.5,0.1,-1) |597|317|
| Nearby (+3,+1,-5), forward |2376|692|

For the forward view, engine visible draws separate into colour1249->379 and
encoded depth1157->287. The remaining3 total draws are canvas work. Disabled
viewport counters can retain older values: summing every displayed viewport row
would overcount. Counters are last-frame engine statistics, not GPU durations.
Three ordinary Track11 marker-offset views also pass combined-cache parity.

## Diagnostics and reproduction

TrackViewer has expandable **Render statistics** and **Export draw diagnostic**.
Sampling occurs once per6frame-post-draw callbacks only while expanded. Exports
under `local/task0065/` include viewport sizes/update modes/visible-shadow-canvas
counters, backend, complete camera basis/FOV/aspect/clip planes, effect controls,
native estimates and renderer costs. Zero counters may be unsupported. Visible
primitives are not necessarily triangles. Particles share scene viewports; native
particle packet/upload statistics remain separate from actual GPU draw counts.

Restore a saved camera/control snapshot after loading its track with:

```powershell
& local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path godot res://scenes/debug/track_viewer.tscn -- --track-diagnostic=D:/absolute/path/snapshot.json
```

This restores camera/control settings, not transient particle/history simulation.
Normal `./build.ps1 --run` loads the updated workspace DLL and accepted default.

Bounded image/draw checks (each process exits on completion/failure):

```powershell
& local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path godot res://scenes/track_optimization_smoke.tscn -- --optimization-mode=default --dense-pose
& local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path godot res://scenes/track_optimization_reflection_smoke.tscn -- --optimization-mode=world_reuse --viewer-transition
./build/godot-debug/track_draw_inventory.exe --assets --dense11 --six-directions
```

## Rejected/deferred scope

- Track24 explicit matrix batching and original MultiMesh both change pixels at
  marker offset50. Default Track24 fallback passes three poses at unchanged
 764/799/796draws. The failure is retained rather than widening image tolerance.
- Matrix batching with car capture changes atlas pixels even if capture views
  themselves retain ordinary batches. Thus all car visual consumers fall back.
  Viewer-to-real-car transition plus four1080p final/colour/depth/atlas comparisons
  pass with that fallback. Original environment cadence/quality remain unchanged.
- Opaque-queue experiment changes the first dense view and stays off.
- Compatibility/Mobile/Forward+ each pass seven within-backend dense comparisons.
  Mobile and Forward+ output hashes differ across backends; no timing winner was
  established. No backend switch or engine upgrade is accepted.
- CPU-only census covers all30tracks/60Props rows, not30-track GPU regression.
  Representative marker batch candidates:11 153->90;12 993->374;24 400->128;
 42 925->368;61 614->157 (planner estimates, never accepted GPU savings).
- GPU timings, sustained60FPS/P95 and complete PS2 visual equivalence remain
  unestablished. Capture-consumer batching, safe Track24 matrix/instance behavior
  and original-depth integration remain subsequent optimization work.

## Verification

Normal Debug bridge build; native batching/instance boundary and runtime/effect
checks; actual engine mesh transport/cache tests; diagnostic pose export/restore;
explicit TASK0065 source successor and fail-closed framework checks. Private
logs/JSON/image artifacts are under `local/task0065/` (older failure PNGs under
`local/task0055/`). No proprietary assets or runtime artifacts enter Git.

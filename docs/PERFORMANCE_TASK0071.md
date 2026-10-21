# Vulkan Forward+ migration (TASK-0071)

## Target and evidence

The user selected Vulkan + Forward+ as the Windows default, with FPS/frame-time
improvement as the primary objective and original recovered effects/LOD/state as
constraints. Validation is restricted to Track11. Existing user changes were
preserved before editing under ignored `local/task0071/baseline/`, including a
SHA256 manifest, original Debug DLL and the 2904-entry source identity inventory.
No commit was created by this task.

Runtime: Godot4.7.2.stable.official.ed1daf0bf; godot-cpp
e4b7c25e721ce3435a029087e3917a30aa73f06b. Actual GPU: Intel Arc B390,
Vulkan1.4.359. This is a Windows result, not cross-platform validation.

Read-only Ghidra MCP rereads retained privately under `local/task0071/ghidra/`:

- E-0071-R01: Retail00108CC0 texture packet metadata/ALPHA and TEX1 fields.
- E-0071-R02: Retail0010AC50 environment packet setup and context state.
- E-0071-R03: Debug00108AE0 texture packet state used by the existing skid adapter.

Retail SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2,
executable revision unknown. Debug SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b,
user-labelled2002-07-08 Debug Build (embedded date not independently confirmed).
Both local ELF hashes were rechecked. No original algorithm or symbol status
changed. These rereads corroborate existing records, not PS2 runtime equivalence.

## Host colour and backend contract

The default is `forward_plus` / `vulkan`, with D3D12/OpenGL automatic fallback
disabled. `RenderContract` reports the actual backend and rejects an unexpected
default. Explicit engine renderer/driver arguments are diagnostic overrides.
Headless checks remain available but are never GPU evidence.

Forward+ colour/depth and reflection-face SubViewports use HDR2D to retain numeric
GS RGB in the 3D attachment. The shader passes those encoded numeric components
directly; fixed-function blending therefore operates in the intended numeric
domain. LDR canvas consumers resolve/clamp the result without another display
transfer. CarViewer uses the same offscreen presentation principle. Compatibility
retains its historical cubic inverse compensation. Raw depth bytes use the same
transport and nearest sampling; no Godot scene-depth substitution is introduced.

Important limitation: the floating attachment does not implement GS COLCLAMP
after **each primitive**. Clamping the resolved colour/atlas reproduces the final
bounded output for tested cases, but saturation followed by another blend can
differ. This is an explicit adapter gap; it is not evidence-backed GS behavior.
No blanket original-equivalence claim is made and full visual acceptance stays
open until this and recorded cross-backend differences are resolved or bounded.

MSAA, TAA, screen AA, debanding and the engine roughness limiter are disabled;
the port continues to use original material/effect data rather than Godot PBR
enhancements. The original effect switches, visibility, LOD and physics remain.

## Measurement protocol and reproduction

Use `tools/run_track_costs.ps1`, whose defaults now select Track11, default effects,
1080p, Vulkan/Forward+,120 warm frames,600 measured frames and three repeats.
It launches the graphical executable directly. Minimized runs are rejected.
VSync and frame caps are disabled. These are deterministic camera/visual routes
with paused vehicle physics, not a driven lap or a whole-game FPS guarantee.

```powershell
./tools/run_track_costs.ps1 -ProjectPath godot -OutputDirectory local/task0071/repro-start -Fixed
./tools/run_track_costs.ps1 -ProjectPath godot -OutputDirectory local/task0071/repro-dense -Fixed -Snapshot local/task0068/camera-snapshot.json
./tools/run_track_costs.ps1 -ProjectPath godot -OutputDirectory local/task0071/repro-route -Snapshot local/task0068/camera-snapshot.json
# Explicit diagnostic baseline: use the preserved project and the same benchmark harness.
./tools/run_track_costs.ps1 -ProjectPath local/task0071/baseline/godot -OutputDirectory local/task0071/repro-compat -Fixed -RenderingMethod gl_compatibility -RenderingDriver opengl3
py -3 tools/task0071_performance.py BASELINE.json CANDIDATE.json --output local/task0071/comparison.json
```

The preserved project uses only the new benchmark harness, with its baseline
production source/DLL intact. The private dense camera snapshot must exist;
the public default start fixture requires no captured snapshot. Original assets
remain configured through the existing global asset-root mechanism.

Each comparator requires matched engine/native build kind, GPU, resolution,
camera/car pose, native command digest, effect controls and complete sample sets.
The paired wall-time median saving must exceed the baseline repeat max-minus-min
range; repeatable median/P95/P99 regressions reject the candidate. Reported FPS is
1000 divided by the corresponding frame-time quantile, not mean instantaneous FPS.
Pixels and original-runtime comparisons are separate gates.

Profile mode (`-Profile`, optionally `-ProfileUi`) aligns collection with the sample
boundary and writes a separate recording per repeat. Plain samples have profiling
disabled. GPU timers remain delayed per-viewport observations: the benchmark
formerly added viewports; it now reports root-only fields and raw target rows.
Neither those fields nor their sum is presented as total frame GPU time.

## Validation status and limitations

Actual Vulkan numeric checks pass for vehicle VU/material/copy/source-over,
additive reflection/depth, shared track fusion, presentation resolve, particles,
Sun anchors/road occlusion, postprocess palettes/history and compositor lifecycle.
The sky negative control was Compatibility-specific; it now deliberately writes
near depth on either backend and the corrected positive/negative fixture passes.
Actual ELISE reflection/cache parity passes with exact atlas/colour/depth for its
same-backend comparison. These are scoped host checks, not Verified recovery.

Private dense/nearby cross-backend captures have equal native packet digests,
commands, sections, zone and visible-instance counts. Pixels are not identical.
`tools/task0071_pixels.py` reports all RGB/depth differences without silently
masking edges or claiming visual equivalence. Further attribution is required.

One initial console-launched baseline was invalidated when its window minimized
during repeat2; it is excluded. The graphical-executable matrix is the retained
measurement series. Final numbers and build/test results are recorded below
after all checks complete.

PCSX2 is installed and a historical ISO path exists, but neither current PINE nor
JSON DebugServer responded, and no matched Track11 camera/game-state reference
has been established. Existing settings include3x scaling and widescreen patches;
they are not an accepted native-resolution reference. A new original comparison
needs an identified runnable state, recorded settings and matched capture scope.
No recovered record is promoted to Verified.

## Results

Pending final paired matrix, optimization ablation, builds and source checks.

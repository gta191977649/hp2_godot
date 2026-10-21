# TASK-0085 GPU particles

Status: implemented and PC-tested. Extended benchmarking stopped at the user's explicit request. Native remains the default.
This is an authorized port presentation backend, not a Verified PS2 equivalent.

## Backend and timing contract

`hp2/rendering/particle_backend` accepts `native` (default) or `godot_gpu`.
The command-line override is `--particle-backend=native|godot_gpu`. Select before
loading the scene. Both Test drive and the standalone driving display use the
same manager. Unsupported original materials remain explicit dependency gaps.

Original emitters, RNG, distance checks, packed birth state, lifetime allocation
and shared 1000-particle/400-group limits remain native. GPU mode retains a CPU
lifetime ledger, not CPU position integration. Ordered transfer commands preserve
auto-emission before stepping and wheel births after stepping. Custom GPU
simulation preserves the original gravity, drag, velocity quantization, inherited
velocity, animation and Hermite curves. GPU sine/floating-point evaluation and
birth-centroid group sorting are recorded port differences.

Godot runs the particle process at zero engine delta with `speed_scale=0`.
The shader's explicit epoch and original visual timestep control integration;
unchanged epochs do not advance. `request_particles_process()` is deliberately
not used: the pinned Vulkan engine's extra dispatch consumed manual births in
the isolated probe. Emissions and epoch changes are deferred until the enclosing
frame succeeds. Abort restores the native ledger and prepared host metadata.
Capture faces reuse the main-view output. Pause does not produce Advance commands.

Each live original group owns one persistent GPUParticles3D node (1000 GPU slots).
Compatible immutable definitions share draw materials; process materials retain
per-group epochs. Slots are not the original allocation budget: the shared native
ledger still enforces the latter. There is no normal GPU state readback or CPU
quad mesh upload. `gpu_emission_payload_bytes` estimates API birth payloads,
not measured PCIe traffic. Godot video-memory counters include the entire scene.
GPU visible counts are estimates (`gpu_visibility_exact=false`); lifetime counts
remain exact. Unsupported high-alpha atlas bounds produce diagnostics and discard
affected quads, rather than silently changing the original blend rule.

## Reproduction

```powershell
cmake --build build/native-debug --target gpu_particle_emission_tests -j 6
ctest --test-dir build/native-debug -R '^gpu_particle_emission$' --output-on-failure
cmake --build build/godot-debug --target ps2_port -j 4
local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path godot --script res://scripts/debug/gpu_particle_lifecycle_smoke.gd -- --particle-backend=godot_gpu
local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path godot --script res://scripts/debug/gpu_particle_visual_smoke.gd -- --particle-backend=godot_gpu
py -3 tools/particle_backend_benchmark.py run local/task0085/benchmark-final
```

The benchmark uses Track11, original 911TURBO, actual driving and forest-fire
dense viewer routes, native A/A, GPU and presentation-off controls at 2880x1695
and 1920x1080. Each run warms 120 frames, samples 600 frames, repeats three times,
and freezes source/DLL/engine/project inputs. Runs are sequential. Wall frame
time is authoritative; delayed unverified viewport GPU queries are not promoted.
Presentation-off still performs native emission/lifetime bookkeeping. Flat-floor
burnout/steering and lifecycle fixtures are separate from timed route inputs.

Acceptance requires overall P95 improvement greater than 5% and twice A/A noise,
ordinary-scene regression no greater than 3%, and visual review. Failed gates keep
GPU optional. No original-game runtime comparison is available for Verified status.

## Results

The user explicitly requested no further repeated benchmarking on2026-10-21. The full matrix was stopped, partial/invalid captures are not accepted timing evidence, and no FPS/P95 benefit is claimed. GPU stays optional; automatic-default gates were not met by evidence. The first image
probe exposed missing GPU births despite successful counters; that run is rejected
as visual evidence. Corrected actual wheel smoke renders 27018 non-HUD pixels at
frame240, equal to the native image's count. Native/GPU image differences below
the HUD: frame240 2437 pixels, maximum channel error1/255; frame299 6937 pixels,
maximum channel error7/255. Raw images stay in ignored local/task0085.


Final build: cmake --build build/godot-debug --target ps2_port -j4 passed.
ctest --test-dir build/godot-debug -R '^gpu_particle_(state|lifecycle|visual)_gpu$'
--output-on-failure passed3/3 in26.25s. Additional steering0.6/handbrake wheel
smoke passed with27918 non-HUD pixels. Production shader motion/packed-state
readback tests pass including hidden update, paused idle and redisplay. Tests
are PC regressions only. GPU simulation slots use more memory per live group
than the shared original allocation budget; profiling is intentionally deferred.

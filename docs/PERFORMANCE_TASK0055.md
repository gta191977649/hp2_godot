# TASK0055 host rendering optimization results

Measured 2026-10-19 on Intel Arc B390, driver32.0.101.9150, Godot4.4.1 Compatibility/OpenGL3.3. GCC13.2, CMake3.29.2, Ninja1.12.0, Python3.14.4. No original-console Verified claim.

Both versions:1920x1080, VSync disabled,30 original catalogue tracks,120 complete-route warmup frames,600 sampled frames,3 fresh native sessions per track. Visual timestep1/60; physics paused after the original first native driving snapshot; deterministic camera loop. This is a camera-route benchmark, not a driven lap or an all-conditions60FPS claim. No concurrent project builds/GPU tests during timing.

Frozen TASK0054 Release versus final TASK0055 Release; binary/engine/script hashes: local/task0055/final-benchmark-inputs.json. Full samples, captures and per-route final native packet/serialized pose evidence remain in ignored local/task0055/final-{baseline,optimized}/. Comparator: reverse/scripts/compare_task0055_performance.py.

## Acceptance

- PASS:90/90 complete repeats per version,600 samples each; camera/car pose bits and final native packet digest/section/zone/instance values match in every pair.
- PASS: original effect permission maps and draw-call distributions match in all90 pairs; effective candidate policy is resource_reuse+particle_mesh_reuse only.
- PASS: median-route paired improvement: wall10.0328%, prepare27.6223%, host submission33.1621%.
- PASS: no route has >5% wall median/P95/P99 regression in at least2 of3 paired repeats. Single-repeat variation is retained.
- GPU and viewport CPU timers are unavailable (zero valid GPU samples); wall/native/host scopes are measured. Do not infer GPU time by subtracting CPU from wall time.

## Aggregate observations

The following table is the median of each statistic across all90 repeat summaries; it differs from the paired median-route improvement rule above.

| Metric | Frozen54 | Final55 |
|---|---:|---:|
| Wall median (ms) | 23.767 | 21.379 |
| Wall P95 (ms) | 32.040 | 28.183 |
| Wall P99 (ms) | 34.951 | 31.053 |
| Prepare median (ms) | 9.556 | 6.761 |
| Host submit median (ms) | 7.725 | 4.939 |
| Draw-call median | 1557.000 | 1557.000 |
| Video memory median (MiB) | 159.366 | 159.366 |

Median loading duration remains10.116s ->10.166s; worst observed loading frame8.289s ->8.896s. Loading stalls are not fixed by this work. Original default effects and distance were retained. Further loading/driver work remains in TASK0053. Original incomplete effects/shadows remain TASK0052.

## Per-route paired summaries

Negative changes mean faster. Each baseline/candidate value is the median of its3 repeats. Change percentages use the median of3 paired ratios.

| Track | Wall54 ms | Wall55 ms | Wall change | Submit change | P95 change | P99 change |
|---|---:|---:|---:|---:|---:|---:|
| 11 | 22.034 | 19.742 | -9.82% | -32.94% | -8.55% | -9.36% |
| 12 | 32.439 | 30.244 | -6.72% | -24.07% | -9.17% | -9.08% |
| 13 | 27.631 | 24.974 | -10.02% | -26.76% | -9.57% | -12.51% |
| 14 | 22.501 | 20.493 | -8.79% | -31.97% | -11.58% | -11.55% |
| 15 | 30.213 | 28.396 | -7.16% | -24.53% | -7.68% | -10.60% |
| 16 | 27.598 | 25.700 | -6.72% | -25.42% | -11.36% | -10.54% |
| 21 | 21.480 | 19.863 | -6.62% | -34.74% | -10.90% | -12.21% |
| 22 | 20.494 | 18.077 | -12.07% | -36.01% | -8.19% | -10.49% |
| 23 | 23.693 | 21.411 | -9.90% | -32.54% | -6.01% | -8.25% |
| 24 | 21.780 | 19.147 | -12.09% | -36.36% | -10.21% | -11.60% |
| 25 | 20.900 | 18.473 | -10.05% | -36.03% | -8.67% | -13.39% |
| 26 | 22.141 | 20.967 | -5.98% | -34.47% | -15.19% | -13.18% |
| 31 | 16.168 | 13.664 | -16.10% | -42.56% | -10.15% | -14.96% |
| 32 | 16.916 | 14.387 | -16.56% | -43.95% | -11.41% | -12.94% |
| 33 | 20.272 | 17.606 | -13.15% | -38.33% | -13.36% | -16.58% |
| 34 | 16.425 | 14.481 | -11.52% | -41.52% | -13.15% | -14.43% |
| 35 | 16.863 | 14.883 | -11.62% | -42.60% | -8.95% | -12.64% |
| 36 | 20.317 | 17.520 | -14.13% | -40.77% | -13.62% | -11.55% |
| 41 | 25.628 | 22.353 | -12.78% | -34.38% | -10.12% | -7.07% |
| 42 | 34.158 | 32.156 | -6.49% | -23.92% | -2.82% | -1.93% |
| 43 | 30.295 | 27.819 | -7.82% | -25.60% | -5.08% | -7.99% |
| 44 | 26.665 | 23.847 | -10.24% | -33.39% | -9.21% | -7.99% |
| 45 | 33.678 | 30.758 | -7.82% | -25.64% | -8.50% | -12.51% |
| 46 | 30.551 | 28.222 | -4.56% | -25.68% | -7.15% | -7.27% |
| 61 | 28.195 | 25.522 | -9.84% | -29.91% | -9.39% | -7.66% |
| 62 | 26.153 | 23.109 | -11.60% | -30.38% | -12.07% | -9.88% |
| 63 | 23.840 | 20.527 | -10.98% | -35.88% | -12.15% | -12.49% |
| 64 | 28.413 | 26.158 | -7.54% | -29.88% | -12.02% | -14.68% |
| 65 | 28.262 | 25.232 | -10.72% | -28.12% | -4.34% | -5.73% |
| 66 | 23.171 | 19.964 | -13.55% | -36.15% | -16.83% | -18.29% |

## Visual gates and excluded experiments

Final cache passes90 controlled actual-asset views across30 tracks, dynamic forest-fire color/depth and lifecycle checks, plus1920x1080 real native initial driving pose and3 camera/basis reflection poses. Comparisons require exact final/color/native-depth/atlas bytes and matching native frame inputs. Debug118/118 and Release98/98 regression checks pass, including14 actual-window GPU checks and optimized Release driving controls/focus/failure/re-entry.

Material object sharing was removed: it changed107 actual Track11 atlas pixels in3/3 independent-process probes; retaining distinct material identity restored exact atlas pixels3/3. MultiMesh changed Track24 color and remains experimental/off. Occlusion/opaque probes found zero eligible work in inspected views; their image parity is no usefulness proof, so both stay off. No quality/distance reduction or guessed friction/lighting coefficient was used.

Separate-process car reflection raster variation of1-18pixels also occurs in baseline repeats, sometimes with identical atlas, encoded depth and serialized poses. Controlled same-session tests remain exact; no tolerance was relaxed and no speculative shader/depth patch made. This residual repeatability question is recorded under TASK0053.

## Default and rollback

The accepted optimized host policy is now the project default for the common TrackViewer/Test drive/race renderer. Override at launch with --track-optimization-profile=baseline. Original effects, native visibility/LOD and physical/contact logic are unchanged. Release read-only render-target diagnostics are available; Debug effect overrides remain gated. No native/recovered function or physics changed in this task.

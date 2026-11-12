# TASK-0105 selection performance

## 2026-11-12 Debug Championship selection

E-0105-P04. The real first Championship route reached five available cars with
a private empty profile. The harness selected Lotus Elise (car 41, skin 4)
through normal navigation. Godot 4.7.2 official ed1daf0bf, Intel Arc B390,
Windows, Vulkan Forward+, Safe, 1920×1080 window / 1920×1025 content,
120 Hz display, VSync off and max FPS 0. Each run sampled four consecutive
120-frame segments, with 30 discarded frames between UI states. Engine
`--debug` activates the passive ProfileDebug collector even without its panel.
Its capture/export was disabled for both runs and the median collector cost
was about 0.12 ms. Sampling exported once after the measured segments.

| Debug UI state | Baseline frame P50/P95/P99 ms | Candidate frame P50/P95/P99 ms |
|---|---:|---:|
| Info on | 31.512 / 37.227 / 39.897 | 11.277 / 12.614 / 13.520 |
| FPS bar only | 30.668 / 32.721 / 34.522 | 11.118 / 12.490 / 12.982 |
| All ImGui drawing suspended | 30.438 / 32.313 / 33.542 | 10.914 / 12.572 / 13.104 |
| Info restored | 31.414 / 33.703 / 34.829 | 11.488 / 12.641 / 13.712 |

With Info on, baseline median ImGui host cost was 0.643–0.660 ms, including
0.332–0.337 ms drawing/DrawData, 0.133–0.138 ms packing and 0.148–0.154 ms
Canvas calls. The FPS-only median was 0.207 ms and suspension 0.008 ms. The
3.524 ms difference between the two baseline Info P95s exceeds the 1.390 ms
gap between restored Info and full suspension. The selection packet alone
took 23.084–23.340 ms median. These host timings are inclusive and GPU
timestamps can describe older completed frames.

E-0105-P05. The port packet now projects and shades each referenced surface
vertex once per surface and reuses the projected clip position and reflection
direction for the environment pass. Expansion retains the index order and
the renderer receives the same draw structure. Debug GNU/Clang builds
optimize the port selection translation unit with `-O2 -fno-fast-math
-ffp-contract=off -fno-strict-aliasing` while keeping symbols and assertions.
The measured candidate combines both changes. Packet median dropped to
3.534–3.687 ms. The candidate still uploads about 2.1 MB of expanded vertices
per frame. Remaining Info-on P95 CPU submission is 4.120–4.352 ms, packet
generation is 4.012–4.017 ms and selection GPU is 1.728–2.279 ms. Inspect
frontend executor buffer upload and per-draw setup next. All sampled frames
were over 8.33 ms in both builds, with the candidate Info P95 still about
4.3 ms over budget. No VSync acceptance was run.

Baseline manifest `local/task0105/run-go3df37x/manifest.json`, DLL SHA-256
`c63663c1d5bce496d0298cf4fe6c3c0ce1a6fa2dc444192a59dc52eadd2bd181`.
Candidate `local/task0105/run-l461mi8i/manifest.json`, DLL SHA-256
`7a4ac8aaac5a49ab80d9af9a91f846c45e4b851476a60fa67b112cb2c088393a`.
The initial fixture stopped before sampling because the default Championship
car was 13. It was corrected to navigate to Elise. Candidate F8 open/close,
skin change, resize, back/reentry and clean exit passed in the production
scene. The final capture was visually checked for the preview, floor reflection,
swatches, and Info. Animation pose and skin differ between captures, so this
does not establish pixel equivalence.

```powershell
cmake --preset godot-debug -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0105/debug-candidate-bin
cmake --build --preset godot-debug --target ps2_port selection_page_tests frontend_runtime_tests debug_console_tests --parallel 6
py -3 reverse/scripts/measure_selection_performance.py --label debug-candidate --debug-dll local/task0105/debug-candidate-bin/ps2_port.dll
ctest --preset godot-debug -R '^(selection_page|frontend_runtime|debug_console)$' --output-on-failure
```

CMake 3.29.2, GCC 13.2.0 and Python 3.14.4. Both samples used isolated projects
and DLLs. Run `./build.ps1 --debug` to rebuild the ordinary launch DLL.
Native regression passed 3/3 in 155.90 s. Progress generation and the
`progress_tables` check passed. Python runner syntax and the scoped Git
diff whitespace check passed.

## Confirmed port findings

E-0105-P01, 2026-10-28: the previous selection host read back every Control
Subviewport, copied/converted its pixels, uploaded those pixels to a local RD,
submitted/synchronized that RD, read its final framebuffer and uploaded the
result again. The production path now samples Control device textures directly
on the main RD and presents its colour attachment through Texture2DRD. Queued
packets own their resources until the render-thread callback finishes; final
device cleanup also runs there. Destination alpha remains internal to the
compositor by retaining opaque target alpha, matching the prior RGB8 presenter.
Stages 0/1/2/4/5/7, reversed depth and shared alpha ordering are unchanged.

E-0105-P02: pipelines, vertex storage, vertex views and sampler uniform sets are
reused. Unused bindings/views/textures are pruned; bitmap ownership prevents
pointer-reuse upload mistakes. Static Control targets update on changes; movie,
animation and interactive hover/focus layers continue updating. Original model
indices are already persistent; texture/environment bindings are now indexed
once per published model. Environment sampling uses the existing reviewed
CARSELECTENVMAP hash 4ECC769C, including for PLATFORM.

E-0105-P03: promise-based tickets replace std::async in selection. Dropping a
ticket cancels publication without waiting for decoding. A frontend-owned worker
survives page cancellation/reentry and owns the decode cache. Its immutable model
templates have an 8-entry / 128MiB approximate geometry-and-texture payload LRU;
the existing separate geometry/texture decode-cache limits still apply. This is
not a 128MiB total process-memory guarantee. Mutable model copies are prepared
on the worker; only matching page-owned job, type, skin and request generation
may publish. Main/reflection consumers share load ownership. Skin preparation
also runs there; the previously published skin remains visible until its exact
replacement is ready. This delivery latency is port policy, not recovered PS2
timing. No recovered game function or verification status changed.

The first experiment used a DLL static worker and hung during Windows process
shutdown. It was rejected. Explicit frontend ownership joins the worker before
DLL teardown; subsequent production roundtrips and benchmark exits passed.

## Measurement and results

Environment: Windows x64, Intel Arc B390, Vulkan/Forward+, Godot
4.7.2.stable.official.ed1daf0bf, CMake3.29.2, Python3.14.4, MinGW-w64 GCC13.2.0;
godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b. The window is
1920x1080; the Container toolbar leaves a 1920x1025 selection target. Timings
exclude screenshot/export work. The benchmark uses the actual empty-profile
first-event route, without unlocking cars or creating a replacement selector.

Baseline: `local/task0105/run-sfj5foka`, DLL SHA-256
109420f4bb3358186ca0b8f0e0c6ce312ad4c1132aab78a139c050e265cd40be.
Candidate measurement/capture: `local/task0105/run-rvnms8dq`, DLL SHA-256
516e40eabba4c87f5917a8af99f2ec45d3f5a616860ddfc86676c957709b8d11.

| Scenario | Baseline frame P50/P95/P99 ms | Candidate frame P50/P95/P99 ms |
|---|---|---|
| Idle | 78.682 / 80.734 / 81.734 | 8.334 / 8.509 / 8.572 |
| Car-direction input (one available car) | 79.166 / 80.878 / 81.551 | 8.331 / 8.559 / 8.729 |
| Skin changes | 78.616 / 81.876 / 82.391 | 8.327 / 8.874 / 9.235 |
| Warm skin changes | 77.809 / 80.076 / 81.517 | 8.323 / 9.005 / 9.304 |
| Reentry, after entry input returns | 77.950 / 80.327 / 81.353 | 8.313 / 8.723 / 9.055 |

Rapid skin requests every two frames: P95 8.996ms, max9.383ms. Warm-skin
frontend CPU max6.844ms; synchronous input dispatch max0.004ms. Idle native
frontend CPU P95 5.628ms, selection GPU timestamp P95 2.184ms. These satisfy
the scoped idle16.7ms and warm switching50ms targets. GPU timestamps are read
asynchronously from completed engine frames, not by forcing a device sync.
Root-viewport GPU timing does not include the baseline's separate local device,
so it cannot establish a baseline-to-candidate total GPU speedup.

Selection readback is zero. Static textures upload zero bytes during idle;
dynamic vertex packets still upload about2MiB/frame. The manifest separates
texture and vertex upload counters. Baseline upload counters were unavailable;
the old summary's zero must not be interpreted as measured zero traffic.
Cold initial worker preparation was104.509ms; warm worker model copies were
roughly0.4-1.7ms. Worker time excludes queue wait and original UI fades.

**Remaining page-transition hitch:** two-frame input wall timing recorded
130.144ms for cancel and454.426ms for reentry (cold entry498.110ms). These
include page teardown/construction and two frame waits, and are not the steady
reentry row above. Full ELF/global/named-solid page setup remains synchronous.
No claim of hitch-free page entry/exit is made.

The60-second visual soak completed with zero port runtime errors, and its image
was reviewed for the nebula, environment detail, vehicle/reflection, swatches
and text. The strict runner failed because Windows invalidated the WASAPI
output device (`GetBufferSize error`); retain that failure. Soak captures have
different animation age/window dimensions and are not pixel-equivalence tests.

Clean final recheck with the identical DLL: `local/task0105/run-_zp0kpw1`
passed the strict runner with no runtime/driver/RID errors. Idle frame
P50/P95/P99=8.337/8.526/8.625ms; warm-skin maximum9.346ms and frontend CPU
maximum6.394ms; rapid-skin maximum9.393ms. Idle selection GPU P95=2.260ms.
Reentry stable frame maximum36.956ms; separate cancel/reentry input wall
times125.539/444.445ms. These confirm the sustained target and preserve the
remaining transition limitation rather than hiding it in percentile summaries.

## Validation and reproduction

```powershell
cmake --build --preset godot-debug --parallel 4
ctest --preset godot-debug -R '^(async_asset_cache|frontend_runtime|frame_builder|frontend_quad|frontend_audio|frontend_composition|selection_page|ultimate_selection_widgets|frontend_display|frontend_name_entry|frontend_effects)$' --output-on-failure
py -3 reverse/scripts/check_frontend_selection_roundtrip.py
py -3 reverse/scripts/check_frontend_controls.py
cmake --build --preset godot-release --target ps2_port --parallel 4
py -3 reverse/scripts/measure_selection_performance.py --label release
py -3 reverse/scripts/generate_progress.py
ctest --preset godot-debug -R '^progress_tables$' --output-on-failure
```

This machine's pre-existing Release cache writes the DLL to
`local/task0072/cpu-submit-phase/release-bin/ps2_port.dll`; copy that file to
`godot/bin/ps2_port.dll` before Release measurements, after all test windows exit.
The final normal frontend DLL is Release. Existing build warnings about service
aggregate initializers remain; neither build failed.

Native regression11/11 passed27.81s. After the final load-ownership correction,
async_asset_cache/selection_page/frontend_runtime3/3 passed21.37s. The cache test
covers nonblocking ticket disposal, queued cancellation, independent mutable
copies, latest-result isolation, exceptions, LRU and oversized-entry rejection.
Final Debug production roundtrip passed at
`local/task0098/selection-roundtrip/run-ozp0ck5_`; earlier run-xy24s9ik also passed.
Progress generation completed; progress_tables passed1/1 in1.26s, and
`git diff --check` passed. The generated recovery dashboard remains unchanged
because this task adds no recovered functions; session/task status lives in the
active task and indexed PROGRESS_LOG.md. The new synthetic test is explicitly
allowed through the repository's existing src/tests ignore rule.

Controls runner: `local/task0104/controls-acceptance/run-t2y8eyrj`. Startup/input,
main70 and reload8 checks passed. Layout executed1265 checks /26 screenshots;
358 failures concern only event glyph origins and the required number-raster
coverage fixture. No other failure labels or runtime/RID errors were reported.
These are the event-number follow-up already reopened in TASK-0104 before this
task; ordinary non-selection Control updates remain unchanged. Selection resize,
mouse, modal and roundtrip checks passed. The total Controls gate remains failed.

## Unresolved acceptance

- This production route exposes only car41. Direction requests do not benchmark
  actual cross-model switches. Native cache cancellation tests do not replace a
  multi-car real-window trace. Do not fabricate profile unlocks to claim coverage.
- Page entry/exit hitches above require separate setup/teardown profiling.
- The TASK-0104 event glyph gate remains failed; no unrelated geometry repair was
  folded into this task.
- No original runtime comparison, numerical equivalence or Verified promotion.

Continue TASK-0105 by profiling page initialization/teardown, then repeat the
measurement with a legitimately reachable supported multi-car scenario once
available. Private assets/configured Retail and Debug ELFs remain prerequisites.

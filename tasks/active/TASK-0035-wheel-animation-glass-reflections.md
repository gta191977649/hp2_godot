# TASK-0035 - Wheel animation and glass reflection recovery

- Status: active; bounded frontend implementation delivered, original comparison pending.
- Target: SLUS_203.62; executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Scope: original tire/blur matrices and strength; shared frontend glass/body environment pass;
  user-requested wheel-speed slider and mirror skin regression.
- Preserve all previous uncommitted TASK-0033/0034 work. No commit created.
- User follow-up overrides the one-control UI: retain Wheel Blur and add a speed slider below.

## Acceptance criteria

- [x] Trace tire/blur matrix writers, phase/direction and opacity with read-only Ghidra.
- [x] Animate existing resources; switch reset, speed adjustment, async reload persistence,
  console coexistence and no animation-triggered resource load.
- [x] Trace original frontend environment texture, view, VU ST/alpha and additive GS/depth state.
- [x] Integrate confirmed frontend reflection in shared native/backend; no PBR substitute;
  shaders remain under src/platform/shaders.
- [x] Correct both mirror skin bindings using original constructor patch tables.
- [x] Native, GPU, host, build matrix, generated documentation and real window capture checks.
- [ ] Original PS2/PCSX2 comparison of wheel phase/direction/opacity and changing glass reflection.
- [ ] Complete required original live inputs/alternate reflection contexts if comparison exposes gaps.

## Confirmed findings

E-0035-W01:00124060 ->00135FB8 carries separate tire and blur matrices.001C0D78 spins
tires about sourceY using phase times original16-bit conversion constant; blur does not
self-spin. Both receive front steering. Render00120EF8 flips wheels0/3 using sourceZ
half-turns. Config lateral correction is preserved; preview suspension/steering are zero.
00219A60 ->001A4170 publishes phase + delta*angular velocity.

E-0035-W02:00120EA8 is float clamp(abs(velocity)*0.02,0,1), not the integer casts shown
by its decompiler.00120EF8 scales by90 and submits above0.1. Viewer all-valid-wheel input
bypasses live contact/damage gates; default80 and range-150..150 rad/s are explicit port
preview choices, never claimed recovered vehicle physics. Speed changes preserve phase;
disabling resets phase/pose. Tire and blur matrices and per-wheel alpha stay separate.

E-0035-R01/R02/R03:00103E68 and00112858 upload original CARSELECTENVMAP (hash4ECC769C,
FRONTEND/FRONTB.BUN,256x256 PSMCT32) at TBP0=0x2800. Original look-at0011B520 and
001175C8 supply the bounded frontend environment orientation. VU002A8AD8 uses second
material coefficients, original reflected ray ST and alpha; context2 is additive
Cs*As/128+Cd, depth writes disabled, depth test retained. Shared renderer supplies this
pass to both glass and body; it does not generate a reflection map or use Godot PBR.

Retired hypothesis: PC439 LOI .5 was initially applied in the same VU cycle, halving z*z.
Independent instruction fixtures exposed the error; next-cycle semantics and z*z factor1
are now preserved in native and shader code. Exact4-ray UV bit fixtures and numeric alpha
checks pass. GPU tests independently check ST, raw texture modulation, alpha0/64/128,
additive framebuffer RGB and both depth-test/depth-write behavior.

E-0035-M01:0011E860 binds its two paint patch pairs at+0x528 to mirror objects+0x1C0/
+0x1D8. The port omitted this binding, leaving SKIN1 mirror textures after body repaint.
It now applies native constructor skin pairs to both mirrors. All original MCLAREN skins
have actual-asset regression coverage; inspected silver capture shows both mirrors silver.

Address records under reverse/functions/<binary-sha>/vehicle and reverse/symbols.csv hold
provenance. Private raw MCP evidence, instruction runners and captures are ignored under
local/task0035. No proprietary material was added to tracked files.

## Remaining gaps and continuation

### Race WINDOW follow-up (2026-11-16)

The Godot mesh converter had excluded depth-sorted race windows from
the paint shader. Sorted WINDOW surfaces with ALPHA `0x44` and
masked depth writes now use the glass shader in both race and Select Car.
Race glass samples the live race atlas, preserves the original in-game
light material and context-2 ALPHA selectors, and blends the window texture
over opaque scene color before adding its reflection. The source references
are `EcstasyFrame::RenderStrip` (PAL `10B08C–10B0F8`, PAL ELF identity
and hash unresolved) and retail E-0035-R01/R02/R03 above. Godot retains
depth tests. Prior transparent layers and overlapping glass remain a
limitation of using the opaque scene copy.

The Debug stage's `tools/test_mvp_race_post.gd` fixture runs with
`HP2_MVP_TEST_GLASS=1` and `-- --race-track=12`. It requires the real
WINDOW descriptor and live atlas binding, then disables only WINDOW's
reflection on a held race frame and checks that pixels change. Vulkan
changed 80 sampled pixels with 17 bound materials; D3D12 changed 340
with 16 in a separate race grid. The same run observed 13 track corona
batches bound to each camera on both drivers.
Release Vulkan Select Car also retained the original map binding:
19 reflective materials including WINDOW and 2,224 changed samples in
`tools/test_mvp_frontend_resize.gd` with `HP2_MVP_PURSUIT=1`.

The live race atlas was still feeding linear HDR face values to the GS
remap modulation. The Godot remap now encodes the face sample as display
bytes before multiplying by the authored RGB 96/128 and writing alpha 64.
The existing race atlas serves both WINDOW and body paint. This fixes the
Godot color conversion at the boundary with the donor's `CreateFacePrimitive`
(PAL `118150` and `118498`, PAL executable identity unresolved).

Staged both `godot-mvp-debug` and `godot-mvp-release` using
`cmake --build --preset <preset> --target stage_mvp --parallel 10`.
Release Track 12 passed the tightened `HP2_MVP_TEST_GLASS=1` test on
Vulkan and D3D12. Toggling only the player car's WINDOW reflection changed
725 and 680 sampled pixels respectively. Both drivers produced the
expected atlas RGBA bytes `102,141,168,64` from a known linear input.
A frozen Vulkan race also captured the same car and camera with the
old and corrected remap under `local/race-glass-matched/`.

No original runtime comparison or measured original driving input was available. This task
remains active, with no Verified promotion. Live game six-face capture/composition through
001175C8/00118810, alternate offsets/contexts, full render-entry branch coverage and live
wheel contact/damage conditions remain untranslated. General GS destination-read alpha>128
is still a gap; unsupported backend reports a missing recovered entry. Floor reflection is
outside this task. PC tests do not prove original visual/behavior equivalence.

Next: TASK-0035, obtain authorized PS2/PCSX2 captures with known wheel velocity/time and
matching frontend car/skin/camera. Compare phase, four-wheel orientation, blur opacity and
glass/body reflection under side/front views. Record differences, then follow original live
environment inputs only where required. Retain whole-entry NotImplemented boundaries.

## Validation

Toolchain: CMake3.29.2; Ninja1.12.0; GCC13.2.0; Python3.14.4; Godot4.4.1 stable49a5bc7b6;
Compatibility/OpenGL Intel Arc B390, driver32.0.101.9150. Commands run sequentially for DLL
configurations; delete generated godot/bin/ps2_port.dll to force relink on each switch.

- `cmake --preset native-debug`; `cmake --build --preset native-debug -j6`;
  `ctest --preset native-debug`:36/36 passed.
- `cmake --preset godot-debug -DPORT_ENABLE_GPU_TESTS=ON` (console ON);
  build and `ctest --preset godot-debug`:40/40 passed.
- `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=OFF -DPORT_ENABLE_GPU_TESTS=ON`;
  build, CTest regex `car_material_gpu|car_render_host|debug_console_host|debug_viewers_host`:
  4/4 passed, including strengthened alpha/depth and skin-reload persistence checks.
- `cmake --preset godot-release`; build; `ctest --test-dir build/godot-release --output-on-failure`:
  39/39 passed. Windowed car_material_gpu_smoke.tscn also passed the common backend,
  including strengthened environment alpha/depth tests. Release viewer requests are rejected.
- Windowed ignored local/task0035/visual_probe.gd loaded actual MCLAREN Silver #2, preview,
  animated wheels at25 rad/s; mclaren-mirror-fixed.png inspected: both mirrors match silver,
  glass/body environment highlight visible, slider directly below switch, FPS top-right.
  This is port output QA, not PS2 comparison. UI drag is automated in native ImGui test;
  no claim of a human/manual drag session.
- `py -3 reverse/scripts/generate_class_templates.py --write` and
  `py -3 reverse/scripts/generate_progress.py`; generated checks included in CTest.
- Final delivery restores Debug console ON, GPU tests OFF after final GPU/host checks.
  Final strengthened Debug/GPU suite:40/40 passed (44.11s). Restored Debug/on/GPU-off,
  rebuilt (no work), and UI/host/template/progress checks4/4 passed. `git diff --check`
  passed; untracked restricted binary/asset candidates: none.

An already-open user window may hold an older mapped DLL. Its file was renamed into ignored
local/task0035/in-use-debug.dll to allow link without terminating the user's window. Restart
that window to load the new build. No ELF, database, textures or captures staged; no commit.

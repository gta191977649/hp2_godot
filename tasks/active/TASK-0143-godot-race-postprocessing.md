# TASK-0143 - Forward+ race materials and postprocessing

- Status: active
- Scope: original water surfaces and texture context, race postprocessing before HUD, performance and track captures.
- Evidence: PAL donor render stages in ecstasye.cpp and eenveffectse.cpp, with the local retail track bundles. PAL ELF SHA-256 remains unresolved.
- Retail ELF SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
  Debug ELF SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
  The address and GS packet comparisons are in reverse/task0052/postprocess.md.

## Implementation

- The location texture inventory found normal replace (0A), source-over (44)
  and additive (48). Descriptor 29 selects draw priority, 4C selects S/T
  wrapping, 4F selects the alpha-test context and 50 selects depth writes.
  Godot applies source-over/additive and preserves authored overlays,
  including W_RDWATER and RD_PUDDLE with their separate additive masks.
  Godot transparent sorting remains surface-based.
- PAL113818..113D80 orders fog, focus, monochrome, destination filter, radial
  mask and history before HUD. Fog curve and focus thresholds follow
  PAL119768, 119C20..119F70. The focus compositor uses a half-resolution
  buffer and PAL2DA750 jitter offsets. PAL11A9F0..11B124 selects the green
  channel for monochrome. PAL10F4A0 and 10F600..10F670 identify the mask
  and history stages. The original source state chooses whether to run each.
- RacePost publishes the original operands. The Godot CompositorEffect
  runs after transparent 3D on the main camera. Inactive passes are skipped.
  GPU focus and history storage is owned by the render buffers and history
  is reset on race reload, replay restore, resize and motion-blur option changes.
  The existing sun and particle passes still use Godot scene depth.
- Full PS2 colour equivalence remains open. Reverse-Z conversion and the
  GS focus/history raster phase use host approximations. Godot hardware
  blending works in the Forward+ colour domain. The optional FilterGrey
  texture retains the original missing-resource gate. Additional GS
  destination-dependent materials, linked-layer ordering, road-shine and
  reflection equivalence need further original comparisons.

## Validation

Godot 4.7.2.stable.official.ed1daf0bf, Intel Arc B390. Debug and Release
staged, and the Track 12 lifecycle smoke passed. Track 12 Release
pixel checks at 960x720 passed on Vulkan and D3D12. The normal weather
pass changed 40,835 and 40,990 sampled pixels respectively. Diagnostic
inputs separately activated monochrome, focus, colour math and history.
These controlled inputs do not establish original trigger coverage.
The Vulkan hide/show check also isolated authored road-water surfaces on
Tracks 12 and 41. The captures and their hidden variants are in the gallery
directory.

The built-in Godot Visual Profiler sample of 489 Track 61 Debug Vulkan
frames at 960x720 reported whole frame P95 13.50 ms and max 19.06 ms.
Render GPU P95 was 2.40 ms. Post-transparent compositor GPU P95 was
0.145 ms. Full-route 120 Hz remains open.

The gallery uses Release Vulkan and local retail track assets. It covers
11–16, 21–26, 31–36, 41–46 and 61–66 at 1280x720 without HUD. The
camera script uses authored route points with terrain elevation to
sample multiple viewpoints. The final selection has one full-resolution PNG
per route, a contact sheet, HTML gallery and ZIP in local/task0143-gallery.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 16
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16
$env:HP2_PROFILE_DIR = "$PWD/local/task0143-repro"
$env:HP2_MVP_TEST_POST = '1'
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_race_post.gd" -- --race-track=12
```

Run with d3d12 for the second backend. Set HP2_PHOTO_DIR to an existing
directory and use tools/capture_mvp_track_scenery.gd for the screenshots.
The tests require a configured complete US disc.

## Light shafts and telephoto captures

The Debug ELF Ghidra inspection confirmed that CreateTextureContext
(00108AE0) takes ALPHA, TEX1 and CLAMP from TextureInfo. RenderStrips
(0010E120) uses the texture's alpha-sort flag, and AttachAlphaPolys
(0010BBF8) chooses the target's depth-write and alpha-test context.
The forest's S30_LIGHTBEAM_3001 and W_LIGHTBEAM07 use GRADIENTLIGHT
(885ADBA5). The desert's LIGHTRAY_01 uses LIGHTRAYGRAD_4 (BA0E31B4).
The shaft textures use zero authored mip levels and full-alpha testing.
The forest, desert and Mediterranean descriptors use source-over (44).
Alpine and Tropical GRADIENTLIGHT uses additive blending (48).
Each uses priority 5 and disables depth writes. The original mesh, UVs
and per-axis wrap flags stay in use. Godot samples the base level with linear
magnification and nearest minification, matching TEX1's MMAG=1, MMIN=3
and MXL=0 for these descriptors. The shafts use scene depth testing and are distinct
from the sky sun effect. Original-console pixel equivalence remains open.

The gallery retake uses HP2_PHOTO_LENS=long with 19–25 degree vertical
FOV and 70–102 m setbacks from the sampled route points. The locally
curated gallery, contact sheet and ZIP are in local/task0143-telephoto/gallery.
Set HP2_PHOTO_TAKES=1 to save candidates and HP2_PHOTO_TAKE to a single
route index to repeat a composition. Set HP2_PHOTO_SURFACE=shaft for a
fixed-camera shaft visibility comparison.
Release pixel comparisons changed 1,607 sampled pixels on Track 12
(Vulkan), 2,318 on Track 22 (D3D12), and 335 on Track 41 (Vulkan).

## ImGui effect controls

In Debug, enter `effect` in the ImGui console to open the effect window.
The grave key can close the console while the window stays open. Checkboxes
control fog, depth of field, monochrome, color filtering, motion blur, race
particles, and sun and glare. Checked options follow the authored triggers.
Particle visibility changes reuse the existing nodes and leave the race
clock and emitter state advancing.

The Debug extension and staged Release project built. Debug Vulkan tests
typed the command, hid the console, clicked the fog and particle checkboxes,
and confirmed the compositor and particle visibility changed and returned.
Track 12 restored 11 previously visible particle nodes and Track 21 restored
three, with the race advancing in both runs. The running Godot instance
locked the usual Debug staged DLL, so the smoke used an isolated staging
directory under local/task0143-effect-project.

## PS2 radial history correction

Ghidra checked US Retail `0010F280` (mask), `0010F4F0` (previous frame),
`0010F238` (alpha conversion), `0013B3F8` (Nitro), and `00112858` (order)
against Debug `0010F660`, `0010F8D0`, `0010F618`, `0013B200`,
`0010FE48` (strip order), and `00112C60`. The ELF hashes are above.
The retail RADIALBLUR bytes in GLOBAL/INGAMEB.BUN have alpha 33–63.
At Nitro's outer vertex, alpha 63 × 204 gives destination alpha 100
after the GS shift, or 100/128 historical weight.

The Forward+ pass uses the strip diagonal, GS byte modulation and signed
blend, and bilinear sampling of the previous encoded RGB across the
half-texel-inset active raster. It primes two reusable RGBA8 textures before
the effect starts and discards their validity on discontinuities. The host
normalizes fractional decay to a nominal 60 Hz NTSC or 50 Hz PAL cadence;
this is a high-refresh presentation policy. Previous HUD and mirror pixels
are absent from the Godot history. The requested attract demo look is a
Godot presentation override with the original RadialBlurOn default
center/edge bytes 0/160, and yields to any authored camera or replay blur.

Both staged configurations built. The Release Track 12 Vulkan and D3D12
smoke passed numeric probes for four corners, center, both sides of the
strip diagonal, half-texel sampling, 60/120 Hz coefficients, viewport resize
and explicit reset. The real Nitro input started and stopped the effect
on Track 12 at both 60 and 120 FPS limits; captures are in
`local/task0143-blur-60` and `local/task0143-blur-120`. These are PC captures
using the optional racer Nitro setting. Matched PCSX2 imagery was dropped at
the user's request, so original visual equivalence remains unverified.
The Debug-menu smoke required active radial blur in the Track 11 demo, then
interrupted it and entered a regular Track 44 race. The demo capture is
`local/task0143-blur-demo/manual-demo.png`. Title idle and manual launch
share the same demo state and compositor.

Godot Visual Profiler, Debug Vulkan at 960×720, fixed Track 12 camera:
post-transparent GPU P50/P95 was 0.133/0.195 ms for history capture only
(250 frames) and 0.240/0.882 ms for active blur (258 frames). The pass
measured 0.000/0.000 ms with the compositor disabled (228 frames).
Capture: `local/task0137-visual/blur-post-profile/capture.jsonl`.

Reproduce the pixel and Nitro probes with the existing postprocessing
command above. For Nitro set `HP2_MVP_TEST_NITRO=1`,
`HP2_MVP_TEST_POST_FPS=60` or `120`, and run the script with
`-- --race-event=ultimate-racer:5`.

## Viewport shutdown texture lifetime

Closing the game with Track Viewer still active reproduced invalid texture
bindings and two invalid RID frees. The compositor now stops at its source
viewport's tree exit, switches dependent materials to an ordinary fallback,
and releases its shared views on the render thread before the source
viewport frees the buffers. The queued release retains the compositor.

Debug Vulkan and D3D12 passed `tools/test_mvp_track_viewer.gd` with
`HP2_MVP_VIEWER_EXIT_ACTIVE=1`, the normal switch-and-return path, and
`tools/test_mvp_race_post.gd` with `HP2_MVP_TEST_POST=1` on Track 12.
The post test checked depth, effects, and history reset after resize.
Before/after logs are in `local/rd-lifetime`.

Forcing Godot's experimental separate rendering thread also removed the
reported texture errors. The previous Vulkan build crashed after the exit
marker. The updated Vulkan and D3D12 tests returned 0, but still logged
engine shutdown errors in this mode. The normal render thread configuration
passed without those errors.

## Scenic Drive color comparison under Windows Auto HDR

Elise Delivery was reproduced on Track 13 with both cars stopped at the
forward start line and both client areas at 1280×800. The diagnostic chose
the event's first available Godot paint and Recomp used its default paint.
Camera composition, paint, raster size, and timer differ between engines.
The regional comparisons are suitable for measuring the overall brightness
difference and the same-app A/B. Per-material equivalence remains open.

Recomp `speed.exe` SHA-256 was
`3e985020f6f67e1086080d7320b73176cd9a1bd34fcae2d093e73ceeddb9d15e`.
Its `hp2.ini` used brightness 0, contrast 100 and Gamma 100. `grade_picture`
bypasses the grade at these values. `HP2_NATIVESHOT` captures after that
point and before the window blit. Godot used the existing staged Forward+
build with extension SHA-256
`bd7a6d221a8280e2128a81670d7716c38d62f4a42ca259735458b93e1f8fdc7d`
and captured the scene viewport and the root viewport including HUD.
Windows Advanced Color was active and DisplayConfig reported 80 nits for
SDR white. Window samples used the same capture API on both engines.

The Windows Graphics page showed Auto HDR enabled for the isolated Recomp
copy. Changing only its Auto HDR switch and restarting produced these sky
samples. The window capture API returned a downsampled JPEG, so its
flat-color RGB is reported as a regional median.

| Capture | Recomp Auto HDR on | Recomp Auto HDR off | Godot Vulkan |
| --- | --- | --- | --- |
| Engine RGB | `(74, 88, 102)` | `(74, 88, 102)` | `(74, 88, 103)` |
| Window RGB | `(100, 120, 137)` | `(73, 88, 101)` | `(75, 89, 102)` |

The Recomp UI and scenery brighten together at presentation. The toggle also
moved its window's road median from `(55, 52, 43)` to `(38, 36, 31)`, shaded
woods from `(58, 58, 53)` to `(40, 42, 37)`, and mirror from `(68, 65, 51)`
to `(49, 46, 35)`. The start-line patch moved from `(255, 255, 248)` to
`(194, 193, 184)` at the 90th percentile. The corresponding Godot window
region measured `(221, 219, 208)`. Start-line material, road and mirror
sampling, HUD modulation, and Elise tail-lamp and paint differences require
matching the authored inputs and camera before changing those paths.

Godot's scene sky measured `(61, 77, 84)` before race effects and
`(74, 88, 103)` with the authored fog. Bypassing the compositor and using
`HP2_MVP_PROFILE_FOG=no-fog` each produced `(61, 77, 84)`. Switching from
FXAA to the available MSAA path kept `(74, 88, 103)`.

For comparable window images, open Windows **Settings → System → Display →
Graphics**, select Recomp's `speed.exe`, turn Auto HDR off for that app, and
restart it. Internal frame dumps are also suitable. The A/B test restored
the isolated Recomp copy's setting to On after capture. The installed
Recomp and the Godot renderer were left alone.
[Microsoft's per-app instructions](https://support.microsoft.com/en-au/windows/hardware/display-graphics/optimizations-for-windowed-games-in-windows-11).

Local evidence is under `local/race-color-parity`, including
`comparison.png`, the `grid-run` Recomp A/B captures and scripted inputs,
and the `godot-output`, `godot-fixed`, `godot-no-fog`, and `godot-no-fxaa`
frames. Launch the Recomp test copy with `HP2_DATA` pointing at the local
asset directory, an isolated `HP2_MEMCARD` and `HP2_INI`,
`HP2_KEYSCRIPT=…/keys.txt`, `HP2_NATIVESHOT=…/n%04d.bmp`, and
`HP2_NATIVESHOTN=90`. Run Godot's local `capture-window.gd` with
`HP2_MVP_TEST_PROFILE=1`, an isolated `HP2_PROFILE_DIR`,
`--resolution 1280x800 -- --race-event=ultimate-racer:1`, and
`COLOR_CAPTURE` as the PNG path prefix.

Validation used Godot 4.7.2 Forward+ and the local retail ISO. The following
checks returned exit 0 on both `vulkan` and `d3d12`:

- `tools/test_mvp_race_post.gd -- --race-track=13` with
  `HP2_MVP_TEST_POST=1` passed the forced color filter, depth, and history.
- `tools/test_mvp_race_lifecycle.gd -- --race-track=33` passed restart and
  return to menu on a second route.
- `tools/test_mvp_track_material.gd` at `3d39f6d` returned exact known
  RGB samples `(144, 123, 64)` and `(48, 72, 48)`.

# TASK-0140 - Godot race effects

- Status: active
- Scope: expose original effects from the live MVP race and render them in Godot.
- Dependencies: TASK-0124, TASK-0137 and TASK-0138.

The active Windows donor implements the AcidFX emitter scheduler and the shared
WorldEffects and FrameEffects pools in acidfxe.cpp. Wheel slip, surface emitters
and car effects flow through car.cpp, care.cpp and terraineffects.cpp. Chopper
munitions and fern collisions feed the same pools. TASK-0075, TASK-0085 and
TASK-0089 record the historical wheel, GPU and water evidence. TASK-0086 records
the original lamp markers and phases. The current donor's PAL display stage
113284/1132C4/11335C/113378/113388 identifies sun, skid, world light and both
particle passes. These PAL addresses are donor annotations. The exact PAL ELF
hash for this donor remains unresolved. The historical Debug AcidFX ELF SHA-256
is 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.

## Acceptance

- [x] Original particle births and lifetime reach Godot GPU effects.
- [ ] Original tyre marks, lamps, sun and glare display in normal races.
- [ ] Verify previews, Track 12 and Track 13 lifecycle and BUSTED.
- [ ] Build Debug and Release, check both drivers, and record measured results.

## Source and presentation coverage

| Effect | Live source | Godot consumer |
| --- | --- | --- |
| Tyre smoke, dust, water spray and debris | World surface effects use grounded contacts, slip, speed and original terrain emission channels. | Shared GPU particle nodes by definition. |
| Exhaust, sparks, damage smoke, explosion and munition trails | Vehicle effect groups, Smokeable, scenery and Chopper emitters feed AcidFX. | Same GPU path, including packed atlas frames and inherited velocity. |
| Select Car nebula | PAL1956C0 installs FrameEffects emitters. PAL1963B8 updates their positions and authored curves. | Preview viewport GPU particles. |
| Police, brake, reverse, headlamp and track flares | PAL121540 and PAL1164F0 supply lamp phase, broken lamps, marker geometry and material. | Godot light billboards with scene depth testing. |
| Sun and collision glare | Sun34202, PAL1252B0 and PAL125918. | Godot camera projection and GPU sky-depth sampling. |
| Skid marks | PAL1AB810 and PAL1AACC0 provide original trail geometry and distance fade. | Godot meshes rebuilt when the trail revision or fade changes. |

Native emission keeps separate WorldEffects and FrameEffects lists on the
shared 1000-particle and 400-group pools. MVP mode keeps native lifetime
accounting for slot reuse. The GPU owns velocity, position, atlas progression
and curves. A small history of committed steps preserves catch-up ordering.
The original GS alpha ordering and framebuffer arithmetic are not proven
equivalent on Godot. Separate RainSplash, shadow and general postprocessing
passes require their own source and coverage checks.

The light and particle shaders build camera-facing quads in view space. The
light shader also evaluates the authored marker-normal response, distance scale
and position bias. Its material uses Godot depth testing. Sun materials sample
Godot's own depth texture over the original 32 by 32 source footprint. The
sample stays the same angular size when the window changes, and offscreen
samples contribute zero. The negative-scale disc uses that depth mask in the
MVP as well. Sun visibility needs no CPU raycasts or depth readback.

## Validation

- Debug and Release staged with Godot 4.7.2, CMake 3.29.2 and Ninja 1.12.0.
  Track 12 HUD, rear view, pause and confirmation passed the Vulkan resize
  smoke at 800x600, 1120x630, 1260x540 and 540x720.
- Track 12 generated GPU particle nodes and original skid geometry on both
  drivers. Vulkan and D3D12 produced particle pixel differences during the
  turn, with the camera and World held still. The final lighting and sun
  build reported 39 changed pixels on Vulkan and 210 on D3D12. See
  `local/task0140-vulkan-check.log` and `local/task0140-d3d12-check.log`.
  Particle visibility varied by
  car and route. One Vulkan run of the earlier drive probe timed out while
  waiting for a frame with sufficient visible pixels.
- A fixed-camera Vulkan Track 12 check inserted a Godot opaque mesh over the
  sun sample area. Hiding the sun sprites then changed zero sampled pixels.
  Removing the blocker restored the flare contribution. The open view changed
  167947 pixels at the same threshold. See `local/task0140-sun-compare.log`.
- The Debug Track 13 frontend smoke reached Select Car, both pre-race pages
  and the live race. The BUSTED smoke displayed two actors and observed their
  cleanup before resuming the race. It used the existing test trigger.
- Godot Visual Profiler on the Track 13 AI route reached 3.71 km. Its last
  4769 paired reports gave P50/P95/P99 of 8.693/16.907/20.262 ms for the
  engine frame, 4.099/8.526/10.522 ms for render CPU and
  1.856/3.592/5.401 ms for GPU. The longest frame in that sample was
  26.379 ms. This run predates the final light vertex shader and does not
  establish a matched A/B or 120 Hz parity.

Still open: visual checks for individual water, spark, exhaust, explosion and
rocket effects, police lamp phases against original footage, natural BUSTED,
replay seek, restart cleanup and a matched profiler run. Use the Track 13
AI route with the same viewport and Godot remote Visual Profiler for both
samples. The raw capture is under `local/task0137-visual/task0140-final`.

## Reproduction

```powershell
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 16
$env:HP2_PS2_ASSET_ROOT = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_race_effects.gd" -- --race-track=12
```

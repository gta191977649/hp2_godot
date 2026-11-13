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
samples contribute zero. The sun shader checks clear reversed-Z depth per
disc pixel. The sky keeps its depth-free material and draws before the sun.
Transparent scenery draws over the sun, including foliage that does not
write depth. Sun visibility needs no CPU raycasts or depth readback.

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

The sun projection follow-up preserves the PAL1252B0 world anchor and fixes
the host's vertically mirrored clip-space quad. The depth lookup and the
rendered sprite now share top-left screen coordinates. Godot 4.7.2 Release
Forward+ passed the noncentral sprite and occluder check on Vulkan and D3D12
at 960x720. An occluder at the mirrored position left the flare visible.
An occluder at the projected position removed the flare, and the disc-only
pass rejected its covered pixels. Run the command below with
`tools/test_mvp_sun_alignment.gd` as the script and select each driver.

The render-order follow-up extends that probe with the authored Track 12
sky and a depth-free alpha occluder. The sun stays visible over the sky,
disappears behind the opaque blocker and is attenuated by the alpha layer.
The native reference draws the sky with Z writes disabled at PAL112E38.
Release Forward+ logs are `local/task0140-sun-order-vulkan.log` and
`local/task0140-sun-order-d3d12.log`.

```powershell
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 16
$env:HP2_PS2_ASSET_ROOT = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_race_effects.gd" -- --race-track=12
```


## PS2 GS sun compositing follow-up

Ghidra MCP decompiled Debug NFSHP2.ELF `StuffSun` (00125430),
`StuffSunGlare` (00125A70), `ePoly::Render` (0010FE48),
`CreateTextureContext` (00108CC0), `UpdateGifTags` (00105AC0),
`SortAlphaKeys` (001053B8), and `RadixSortAlphaKeys` (00104FE0).
Debug ELF SHA-256 is
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
Donor annotations identify PAL1252B0 and PAL125918 for the sun and glare.
The exact PAL ELF hash for the donor remains unresolved.

The original paths submit glare first, followed by the six track layers.
All five textures share the Sun priority-7 bucket. PAL1055B0/1051D8
sort descending by direct Z and reverse positive ties. The GPU pass
therefore visits the equal-Z layers in reverse submission order and
draws the Z=1 sun disc last within the stack.
The GS packets select B=destination for blend or B=0 for add, with A=source,
C=source alpha and D=destination in both cases. The five named sun textures
in the 30 local LOCATION texture packs share TCC RGBA, TFX MODULATE, linear
magnification and nearest minification at their single retained mip.
The packet's Q=1 and TEX1 K=0 select linear filtering at level zero.
The texture context accepts alpha >= 0 and disables depth writes.

The MVP now preserves the source alpha bytes through sampling and applies
GS modulation, byte clamping and destination-dependent blending in one
bounded Godot shader pass. It converts the Forward+ opaque scene color
to GS byte space and converts the final RGB back to linear. A Godot
sky-only viewport provides the authored sky where depth is clear. It
runs only while the sun pass is visible. Main and mirror sky materials
retain their authored transparency and normal ordering. The core change
only restores the verified publication order, keeping the existing glare
clock and all host rendering in their respective layers.

Debug and Release `stage_mvp` built with CMake 3.29.2 and Ninja 1.12.0.
The following check passed on Vulkan and D3D12 Forward+ in Godot
4.7.2.stable.official.ed1daf0bf at 960x720. The expected GS channel values
96 and 160 matched on both backends. It also verified sky contribution,
center and mirrored blockers, the per-pixel disc check and alpha foliage
order. The existing Track 12 race-effects smoke passed on Vulkan with
nine GPU particle groups, six skid meshes and 670 changed particle
pixels in the final run. A Debug Track 13 run enabled donor AI and reached a live sun
pass during the route. The frame was heavily occluded and was not used
as a visual match to the user's reference. A Track 12 race capture showed a saturated white core and visible
rays with the roadway outside the glare retaining its contrast.

```powershell
$env:HP2_PROFILE_DIR = "$PWD/local/task0140-sun-gs-profile"
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_sun_alignment.gd" -- --race-track=12
```

Use the same command with `d3d12` for the second backend. Captures and logs
are under `local/task0140-sun-gs-*`. The Godot depth aperture uses the
current frame and reversed-Z clear depth. The original reads the retained
prior Z24 tile with a threshold of two. Same-camera PS2 capture parity,
native-resolution profiler measurements of the sky target and a live
collision-glare comparison remain open.


## Police support world objects

The original draw traversal uses the WorldObject registry (donor
PAL1CDB20, WorldObject::Render PAL1CD998). Ghidra MCP confirmed Debug
WorldObject::Render (001CA968), Chopper::DrawChopper (001E6880) and
Chopper::RenderChopper (001E6A48). The Debug ELF is
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`
(SHA-256). The PAL donor ELF hash remains unresolved.

The bridge now publishes the same model, transform and visibility the
donor draw traversal consumes. Source registration assigns a port-side
identity so a recycled pool slot cannot retain the previous Godot node.
Godot uploads and shares the original meshes, selects authored animation
frames against the race clock, and reconciles instances when the source
object retires. The track renderer keeps collision spawners and rigid
animated fern placements individually hideable so the static counterpart
can disappear when the dynamic object takes its place. The police AI,
collision and munition paths stay in their source owners.

| Source object | Authored model and effect |
| --- | --- |
| Roadblock fill barrel, PAL24F0A8 | ROADBLOCK_BARREL, donor Smokeable pose and blink |
| Spike-strip segment, PAL24154C and embedded render element | SPIKEBELT_00, per-segment pose and retraction visibility |
| Helicopter, PAL2B95B8 model binding | TRACK_HELICOPTER, committed Chopper pose and AcidFX downwash routing |
| Smoke or spike bomb | SMOKEBOMB, committed Smokeable pose and source detonation group |
| Rocket | ROCKET, same WorldObject mesh path and ROCKETEXPLD routing |

Release Vulkan on the normal Track 11 Be the Cop event 5 accepted
both support requests, created SPIKEBELT_00, TRACK_HELICOPTER and an
actual SMOKEBOMB drop, then reached the police results screen. This
authored layout did not choose a fill barrel. The Debug visual fixture
used the original fill-barrel and smoke-bomb factories and the donor
smoke detonation. The normal support requests still produce the
helicopter and spike strip. The fixture uses an isolated event profile.
Release Vulkan Track 12 passed pause, resume, restart and return, with
the old race scene retired. Debug and Release staged. All four models
changed pixels on both Vulkan and D3D12 in the fixed-camera hide/show
check. See `local/task0140-support-final-vulkan.log` and
`local/task0140-support-final-d3d12.log`. The debug probe checks that the
SMOKEBOMBEXPLD group reaches a Godot GPUParticles3D node.

```powershell
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 16
$env:HP2_MVP_TEST_SUPPORT_VISUALS = '1'
$env:HP2_PROFILE_DIR = "$PWD/local/task0140-support-test-profile"
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-debug/mvp-project --rendering-driver vulkan --resolution 800x600 --script "$PWD/tools/test_mvp_police_support_visuals.gd" -- --race-event=ultimate-racer:5
```

The smoke establishes asset and lifecycle coverage on PC. Same-camera
PS2 lighting and animation comparison, a visually isolated downwash
sample, and live rocket/spike-bomb detonation remain open.

## Forward+ particle masks and vehicle outlets

The retail Track 12 and Track 21 bundles expose 40 and 26 authored emitters,
respectively, through resource chunks 35020 and 35021. The existing source
scheduler advances these emitters and publishes each birth. Track 12 fire
uses the authored Forest flame, smoke and spark groups. Track 21 uses
XE_Sandstorm_XL_01 and XE_SandDebris_XL_01. The local bundle inventory covered
30 track bundles. The smoke checks below cover the two sampled routes.
Original surface effects still come from carFrameUpdateSurfaceEffects and
the terrain table. The source decides the contact material and skid state.

Ghidra Debug ELF 00133988 and the Nitro callback at 0013C028 retain the
source Nitro route. The shipped XEM_Nitro_FlameA emitter uses FIREA_CS and
vehicle marker list 10. The US GLOBALB.BUN contains no emitter definition
for the CARFX_EXHAUST_SMOKE slot. Tailpipe smoke uses list 10 and source
engine/throttle state with a separate cosmetic Godot emitter and original
SMOKE_CS artwork. The exhaust rate and lifetime are port-side values.
The Debug ELF SHA-256 is the same as in the police support record above.
Set `racer_nitro=1` in the profile's NFSHP2_pc.ini to enable the existing
opt-in. N sends the source input sequence through the Godot InputMap.

The billboard projected size uses the magnitude of the Forward+ projection
Y coefficient. The signed coefficient could give a zero or negative size
cap. The spatial vertex path now supplies view-space position for standard
depth testing and fog. Smoke and dust preserve the atlas mask and authored
blend. The shader reads the current viewport's reversed-Z depth and applies
a 0.3 m soft intersection. Tailpipe smoke uses StandardMaterial3D's
proximity fade. The camera layer mask remains distinct for the race and rear
view. These Forward+ depth decisions are port presentation choices.

The race prepares the definition nodes, materials and textures at scene
load. A shared tailpipe node covers all outlets. The runtime test checks
instance IDs throughout activation. Replay seek clears transient state.
No engine profiler or native-resolution presentation trace was collected
for this follow-up.

### PC validation

Godot 4.7.2.stable.official.ed1daf0bf, Intel Arc B390, 960x720. The debug
camera positions itself over the source emitters. The tests isolate the
target nodes and compare viewport pixels with those nodes hidden.

| Track and effect | Vulkan Forward+ | D3D12 Forward+ |
| --- | --- | --- |
| 12, Forest flame, smoke and sparks | 96,164 changed samples, P99 8.944 ms, max 10.221 ms | 96,850, P99 9.391 ms, max 9.567 ms |
| 21, sandstorm and debris | 63,081 changed samples, P99 8.689 ms, max 9.222 ms | 63,655, P99 8.671 ms, max 8.758 ms |
| 12, Nitro and tailpipe smoke | Release pixel checks passed | Release pixel checks passed |

Debug and Release staged successfully. The final Release Vulkan frontend
smoke reached a live Quick Race and Pause Menu on Track 24 and exited 0.
The final Track 12 vehicle captures ran at 960x720 on Vulkan and 2880x1695
on D3D12. Their frame intervals are unsuitable for a backend comparison.

The frame rows each contain 100 process intervals before screenshot readback.
They do not establish uninterrupted 120 Hz display presentation. The Release
Vulkan drive triggered XETT_DIRTRD_DR and XETT_ROAD_SK plus tyre marks.
Hiding only those terrain particle nodes changed 6,033 pixels. The track
pool kept 128 node identities on Track 12 and 117 on Track 21, including
the shared tailpipe emitter. Logs and screenshots are under
`local/task0140-final-*` and `local/task0140-particle-abs-*.log`.

```powershell
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 16
$env:HP2_PROFILE_DIR = "$PWD/local/task0140-repro"
$env:HP2_MVP_TEST_TRACK_EFFECTS = '1'
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-debug/mvp-project --rendering-driver d3d12 --resolution 960x720 --script "$PWD/tools/test_mvp_particle_routes.gd" -- --race-track=21
```

Repeat with Track 12 and Vulkan for the remaining route cases. For Nitro
and exhaust, use Release, clear HP2_MVP_TEST_TRACK_EFFECTS and enable the
profile option. Use tools/test_mvp_race_effects.gd in Release on Track 12
for the terrain check. All require a configured complete PS2 disc.
Same-camera PS2 comparison, the remaining authored effects and routes,
rear-view and BUSTED smoke visibility, and a native-resolution long
frame-time run remain open.

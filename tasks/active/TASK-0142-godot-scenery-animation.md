# TASK-0142 - Godot scenery and texture animation

- Status: active
- Scope: original scenery vertex frames, rigid frames, UV scroll, texture sequences and existing track particles.
- Acceptance: verify Tropical flags, waterwheel and windmill movement, UV and frame textures, pause, and reuse of Godot resources on Forward+.
- Authority: the PAL donor code and the local US retail track assets. The interpolation and Godot material mapping are port-side.

## Source and ownership

- Donor PAL103950..1039EC groups flagged same-key solids in authored order.
  PAL10DB70..10DBCC selects the variant at 30 FPS using the 60 Hz world
  clock and original start offset. PAL10DC80 composes the rigid transform
  before drawing the selected geometry. The PAL donor ELF SHA-256 remains
  unresolved. No other ELF's addresses or hash are substituted.
- US TRACKB61..66 contains 15 authored XF_FLAG_1_00 variants and a
  15-sample rigid stream. TRACKB12/13/15/16 contains the 120-sample
  WATERWHEEL. TRACKB32/33/35/36 contains six windmill rotors with
  100, 150 or 200 rigid samples. These assets use two world ticks per rigid
  sample. B0300100 packs occur in the original location texture files.
- PAL100A50..100AB0 defines signed truncation and stepped scroll. The
  donor material path consumes descriptor offsets 79, 7C, 80 and 84.
  PAL1ABD80..1ABEE8 selects the texture frame using the pack's original real
  or world clock. The host borrows the immutable frame views at upload.
  Source texture rotation still serves the existing frontend renderer.
- Godot preloads vertex positions and rigid rotation, translation and scale
  into GPU tables. The shader interpolates matching topology, uses normalized
  quaternion interpolation for rigid frames, and keeps texture sequence
  layers discrete. One clock vector per frame drives all cameras. Source
  collision and fern trigger state remain in game_core. Resident meshes,
  texture arrays and materials remain owned by Godot.
- Track 61 has 22 authored emitters including fountain, geyser, volcano and
  waterfall effects. The existing source scheduler and Godot GPU pool handle
  them. The new Debug capture only moves the camera to the waterfall.

## Validation

Godot 4.7.2.stable.official.ed1daf0bf, Intel Arc B390, Windows Forward+.
Debug and Release stage builds passed. Release Vulkan and D3D12 fixed-camera
captures confirmed a changed intermediate pose after 0.65 logical ticks
for the flag, waterwheel and windmill. Fixed clock frames matched. Separate
checks showed UV motion on Track 12 and on the Track 32 fountain, and
texture-sequence changes on the barrier. Track 61 world-clock freeze and
resume passed on both backends. The existing Release Vulkan lifecycle smoke
passed driving, pause, resume, restart and scene retirement on Track 12.
Track 61 Vulkan Debug waterfall capture
changed 1,710 sampled pixels when only the two mist/cloud GPU nodes were
hidden. The 118-node particle pool kept the same instance IDs.

The Godot built-in Visual Profiler capture on Track 61 used Vulkan Debug
at 960x720 and 513 matched frames at the start of the race. P95 was
13.25 ms for the whole frame, 6.70 ms render CPU and 2.44 ms render GPU.
The longest frame was 16.08 ms. This sample does not reach the 8.33 ms
budget. It has no matched baseline and does not attribute the cost to
scenery animation. The waterfall diagnostic camera's separate 100-frame
process-interval sample peaked at 8.72 ms.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 16
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16
$env:HP2_PROFILE_DIR = "$PWD/local/task0142-repro"
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 800x600 --script "$PWD/tools/test_mvp_scenery_animation.gd" -- --race-track=61
```

Repeat with Tracks 12 and 32 and both drivers. The waterfall check uses
the staged Debug project, `HP2_MVP_TEST_TRACK_EFFECTS=1` and
`tools/test_mvp_particle_routes.gd -- --race-track=61`. Use the local
Visual Profiler collector `local/task0137-visual/collect_task0142.py` to
repeat the timing run. Use a configured complete US disc. Local captures
and logs are under `local/task0142-*`.

Open: same-camera PS2 comparison, live volcanic/geyser checks, replay
animation continuity checks and a matched native-resolution performance
comparison through the full Tropical route.

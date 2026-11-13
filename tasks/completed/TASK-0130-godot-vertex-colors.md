# TASK-0130 - Godot model vertex colors

- Status: completed
- Scope: pass the authored RGB555 channel to the shared Godot model mesh and use it with the existing unshaded textured material.
- Authority: the imported Windows donor's `decodeSolidGeometry` in `src/game_core/src/ecstasy/ecstasye.cpp` and the configured original US disc. The donor's original PAL ELF hash remains unresolved.

## Acceptance

- [x] Race track, sky, vehicle and frontend preview meshes use the shared color conversion.
- [x] Validate Godot mesh color data, material settings and real-asset rendering on Track 12.

Custom shaders, PS2 lighting and post-processing are outside this scope.

## Implementation and evidence

The donor decodes one RGB555 word per position in `decodeSolidGeometry`.
The source file SHA-256 is `02e7514cd0411b5ba7a06523cc81e362d339ac83d6196d98c6974ce9332bddff`.
The donor's authored-color branch in `EcstasyLitVertexColor` uses each
5-bit lane divided by 16, reflecting VIF expansion by eight and GS modulation
with 128 as unity. The Godot adapter uses that scale and saturates values
above unity for Godot's UNORM8 vertex-color channel. It marks the color as
sRGB for Forward+ and uses vertex alpha 1 so texture alpha controls opacity.
The saturation and Godot texture multiplication are port-side mappings.

## Validation

Windows Release, Godot 4.7.2 stable `ed1daf0bf`, Vulkan 1.4.359 Forward+,
Intel Arc B390, godot-cpp `e4b7c25e721ce3435a029087e3917a30aa73f06b`,
CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0, Python 3.14.4.
The US disc image SHA-256 was
`9aaa58f238dd2e17814945c0802f327ba36c94a8391d5d7ea43a03a89066cbcb`.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
$env:HP2_PS2_ASSET_ROOT = (Get-Content config/ps2_assets.local.json | ConvertFrom-Json).mvp_root
$env:HP2_PROFILE_DIR = (Resolve-Path local/task0130).Path + '/profile'
$env:HP2_COLOR_CAPTURE = (Resolve-Path local/task0130).Path + '/track12.png'
$engine = 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
& $engine --path build/godot-mvp-release/mvp-project --script (Resolve-Path local/task0130/check_colors.gd).Path -- --race-track=12
```

Build and window check passed. The local scene check inspected 2,318 distinct
meshes with 7,695 surfaces and counted 411,911 tinted vertices. All inspected
surfaces had a color per vertex, opaque vertex alpha and both material flags.
The Track 12 screenshot is `local/task0130/track12.png`. The local check script
is ignored with the capture. The Release preset registers zero CTests.
Original PS2 visual equivalence remains unverified.

The frontend smoke passed on Forward+, entering Track 12 through Quick
Race, driving, pausing and resuming. With the same asset environment and
an isolated profile, run
`& $engine --path build/godot-mvp-release/mvp-project --script (Resolve-Path tools/test_frontend_mvp.gd).Path`.
The frontend preview uses the shared converter. The surface inspection
covered the Track 12 race scene. Changes are in the working tree.

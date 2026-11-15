# TASK-0133 - Responsive Godot frontend

- Status: active
- Scope: Godot layout, video framing, previews and HUD across live window sizes.
- Acceptance: preserve authored proportions and grouping, and exercise resize in the frontend and race with original assets.
- Authority: port-side presentation policy, with original object identities taken from the donor and its loaded layout.

The final layout keeps menu controls, the race HUD, previews and text in
one centered 640×480 design canvas. The menu grid tiles outside it.
Decorative bands fill the width and menu panel video fits within a 4:3 rectangle.
The race camera keeps its vertical field of view. The authored red and
silver cinematic strips keep their 4:3 framing. Separate black fill
covers the outer edges. Title and Loading use the full-width decorative
band rule with centered foreground content.

Validation uses `tools/test_mvp_frontend_resize.gd` in the staged Godot
project. Add `-- --race-track=12` for the direct race or set
`HP2_MVP_PURSUIT=1` for the event tree. Use an isolated profile and a
writable `HP2_MVP_SCREENSHOT_DIR`. The script checks 800×600, 1120×630,
1260×540 and 540×720 in the same process, with maximize/restore at the
main or pause menu. The normal frontend smoke is a separate check.

Natural finish and results verification remain open under TASK-0129.

Both `cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16`
and the equivalent Release build passed (CMake 3.29.2, Ninja 1.12.0,
GCC 13.2.0). Godot 4.7.2 Vulkan Forward+ passed the direct HUD, pause and
restart confirmation sweep and the Track 13 menu-to-race sweep. Logs are
`local/task0133-final2-direct.log` and `local/task0133-final2-pursuit.log`.
The latter includes a Loading capture and checks that the menu video
keeps its 4:3 rectangle and playback position across the resize.
The existing Track 12 frontend smoke also passed during this task.
The Select Track preview passed the same sweep with
`HP2_MVP_RESIZE_TRACK_PICKER=1` in `local/task0133-final2-picker.log`.

## Title films and Select Car presentation

`TitleScreen::OpenCurrentMovie` follows the active bitmap from the original
movie table. The port-side fullscreen override previously forced both title
bitmaps to 640×480 for Godot. It now applies to the EA logo. Godot places the
NFS intro on the authored letterbox bitmap at y=65, approximately 640×304,
with a black matte across the live viewport. See E-0016-V01/V04 and Retail
`0016EE90` / `0016F1F0` in [Startup and original movie playback](../../docs/FRONTEND_MEDIA.md).
The evidence uses US PS2 `SLUS_203.62` (SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`).

Select Car reuses the live `XE_SelectCarNebula` definition and
`SetSelectCarLighting` presets. The 3D preview already blends the particles
and platform against transparent black. A premultiplied canvas composite
avoids weighting that result by alpha again. The selector's effect clocks,
curves, and input remain in their original owners.

The state-only `CollectFrontendCarModels` bridge previously omitted the
reflected geometry, brake meshes and road-shadow strip output. It now
borrows the existing one-view reflected pose and LOD, and publishes the
existing frontend branch of `renderCarRoadShadow` through `RaceEffectsFrame`
with its source view. Godot shares the preview depth buffer and uses the
original reflection pass's sorted-alpha and environment-map suppression.
See PAL `123D28–12405C`, `122454–122B88` and `122EE8–1233EC` in `care.cpp`.
The PAL executable identity and hash remain unresolved. Debug
`NFSHP2.ELF` (SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`)
corroborates the reflection flags at `00123EA8` and the mutable particle
basis at `00128870`. See [reflection evidence](../../reverse/task0098/compositor-correction.md)
and [particle evidence](../../reverse/task0098/frontend-effects.md).

```powershell
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
$engine = "$PWD/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
$env:HP2_MVP_PURSUIT = '1'
foreach ($case in @(@('debug', 'vulkan'), @('release', 'd3d12'))) {
    $preset, $driver = $case
    $runDir = "$PWD/local/frontend-fidelity/$preset-$driver"
    $env:HP2_PROFILE_DIR = Join-Path $runDir 'profile'
    $env:HP2_MVP_SCREENSHOT_DIR = Join-Path $runDir 'shots'
    New-Item -Force -ItemType Directory $env:HP2_MVP_SCREENSHOT_DIR | Out-Null
    & $engine --path "build/godot-mvp-$preset/mvp-project" --rendering-driver $driver --resolution 960x720 --script "$PWD/tools/test_mvp_frontend_resize.gd"
}
git diff --check
```

Both staging builds passed. Godot 4.7.2 Forward+ passed the resize test on
Debug Vulkan and Release D3D12. It checked both title bitmap dimensions
and playback continuity across four window sizes. In Select Car it found
the Elise lamp bindings, eight reflected meshes including wheels, one
shadow batch, and 168 / 160 blue nebula samples respectively. Both runs
entered Track 13. Logs and screenshots are under `local/frontend-fidelity`.
The Release Vulkan sweep also passed Select Track at five window sizes
with `HP2_MVP_RESIZE_TRACK_PICKER=1` and `HP2_MVP_PURSUIT` unset.
Matching screenshots against the original PS2 renderer and verifying a
locked-car nebula transition remain open.

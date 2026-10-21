# Host UI DPI and TrackViewer coordinates (TASK-0061)

TrackViewer shows live **camera** coordinates at top left, including viewer driving.
HP2 XYZ=Godot ZXY (HP2 Z is height); the second line labels Godot XYZ explicitly.
These are camera coordinates used for rendering/culling, not vehicle/contact points.
FPS stays top right; the main panel reserves space beneath the coordinate header.

ImGui widgets/styles/persisted positions are logical units. Physical framebuffer
triangles and scissors scale together; mouse positions use the inverse mapping.
Public viewer targets remain physical viewport pixels. Atlas glyphs bake at13*scale
pixels; revision/GPU texture changes only on DPI change. Console, FPS and every
viewer share this session. Returning to100percent does not compound style scaling.

Windows `Ps2RenderHost.get_ui_scale()` queries effective GetDpiForWindow/96.
UiDisplay polls every0.2s for monitor/settings changes and scales screen Control
text from original sizes in Debug and Release. Shared LabelSettings are duplicated;
weak ownership/removal cleanup and baseline metadata prevent retention/double scale.
Other platforms use Godot screen scale; headless is1. Process DPI awareness is untouched.

Original bitmap glyph layout/640x480 packets, world-space3D labels, camera projection,
render resolution and game units retain their existing contracts. Monitor DPI is
a host screen-UI transform, not another game-space scale.

## Validation

Godot4.4.1/GCC13.2/CMake3.29.2/Python3.14.4; native Windows UI scale reported2.0.
Native tests exercise100/150/200/100percent, stable font revisions, physical clicks,
clipping and header presence. Track11 actual windows check live coordinates, font
sizes, input targets, atlas growth/return and UI removal/re-entry. Release text
fixture checks font scaling and shared LabelSettings without ImGui. Console/viewer
regressions pass at native200percent. Forced scales are not a physical monitor-move
test; non-Windows hardware was not tested. No FPS or PS2 Verified claim.

```powershell
& local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe `
  --path local/task0061/delivery/Debug/godot `
  --script res://scripts/debug/ui_dpi_smoke.gd -- --track-load=11 --expect-debug=true
& local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe `
  --path local/task0061/delivery/Release/godot `
  --script res://scripts/runtime/ui_text_dpi_smoke.gd -- --expect-debug=false
```

Ignored screenshots: local/task0061/delivery-ui/dpi-{100,150,200}.png.
Strict source review9before/14after allows only port UI/adapters; TASK0060 renderer,
recovered logic and physics bodies remain unchanged. See
[E0061U01](../reverse/task0061/ui-dpi.md).

## Build and delivery

Launch local/task0061/delivery/Launch TrackViewer Native.cmd or Launch Race Release
Native.cmd. Each flavor has all8runtime DLLs, hashes and BUILD-INFO.json. Workspace
resource junctions mean these are local binary/config snapshots, not portable exports.

To avoid overwriting the user's running root frontend, existing build directories
now use PORT_GODOT_BIN_DIR=local/task0061/staged-{debug,release}/godot/bin.
`cmake --build --preset godot-debug -j4` and the godot-release preset produce staged
DLLs. Stage full runtime dependencies with tools/stage_godot_runtime.ps1.

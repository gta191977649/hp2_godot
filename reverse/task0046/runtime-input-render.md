# TASK-0046 host input and rendering boundary

Evidence ID: E-0046-U01 identifies port integration and host validation; it is not
independent evidence for original algorithms. E-0046-S01..S04 supply binary evidence.

Target: Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
This document describes port-side glue, not recovered camera or Godot behavior.

## Input contract

`godot/project.godot` is the sole production location for physical driving bindings.
`driving_input.gd` resolves namespaced InputMap actions, combines independent device
sources, retains held values and queues discrete commands. Keyboard echo does not
create shift events. Q/E/Space are defaults, not hardcoded controller tests. Existing
analog axes remain bound centrally. Esc is also an action. Display hints read InputMap.

The controller submits complete semantic frames through `set_driving_input`. Its front
command is resubmitted until a committed snapshot advances the step counter, then
removed once. No-step frames do not consume commands. Native Session replaces the
complete input frame so clearing the adapter cannot leave a pending native shift.
F8/focus/exit clear both physical and GUI input. Actual keyboard capture clears physical
holds and edges; intentional GUI mode/N requests survive their own widget capture.
Mouse-only widget consumption does not erase WASD. GUI and keys enter the same queue.

`Snapshot.powertrain` supplies original RPM, speed, actual gear, manual mode and
handbrake. The HUD deliberately says native speed units: this task has not proved a
HUD unit conversion. Actual Gear0/1/2 maps to R/N/1; request modes are not actual gears.

## Skid rendering contract

Canonical RenderSkids produces original quad geometry/UV/raw RGBA and texture identity.
Godot does not compute eligibility, widths, intensity, pools, skid segments or fade.
The bridge supplies an explicit tool camera view: native(x,y,z) becomes host(y,z,x),
Godot symmetric math projection becomes original row-vector clip entries, and H is
viewport_height/(2*tan(vertical_fov/2)). This is a tool camera policy, not original
HP2 camera recovery. Original RenderSkids owns render-phase LRU modifications.

`Ps2TextureLoader::load_texture_for_3d` uses the existing original archive decoder
with no vertical flip and preserved GS alpha. It has a separate cache from frontend
expanded-alpha textures. InitSkids receives original metadata and absence information.
Emitted commands require their original texture; no generated stripe texture is used.

The shader binds the existing encoded-GS host modulation implementation to independently
read Debug texture state (E-0046-S01; CreateTextureContext00108AE0). Supported slice is
MODULATE, ALPHA copy/source-over, MXL0/MMAG1/MMIN3, independent U/V repeat/clamp and bounded alpha. Nearest sampling performs signed
integer wrap or edge clamp independently; linear sampling clamps requested axes to
texel centers while preserving the repeat sampler on the other axes.
Debug AttachSolidPolys0010BEF0/AttachAlphaPolys0010BBF8 have independently
recovered draw-context selection. Original ePoly.Render3D publishes alpha test and
depth-write fields; Godot consumes them. Counterevidence: Debug does not select
writes from TextureInfo.bWriteZ50 as the old Retail helper does. RenderingOrder29
and GlossMapInAlpha74 are retained and passed into canonical original selectors.
Unsupported state throws an address-anchored gap. Original Render3D strip0,1,3,2 becomes
triangle indices0,1,3,1,2,3. Consecutive commands share surfaces without reordering.
Raster precision, Godot transparency ordering and original hardware equality remain
outside certification. No original numeric or raster Verified claim is made.

## Host validation

Pinned Godot4.4.1 command:
`local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --headless --path godot --script res://scripts/debug/driving_input_smoke.gd`
passed: W release, steering cancellation, independent keyboard/trigger release,
Q/E single edges and echo suppression, queued command consumption, Space hold/release,
mode/N requests, capture/lifecycle clears and runtime W-to-T reassignment.
Godot editor headless scan also passed after adding the host scripts.

`viewer_ui_targets()` is read-only observation of actual ImGui widget centers. The
OS harness uses those centers and guarded Win32 mouse/keyboard injection, never direct
state writes, to tolerate the added HUD rows. `--controls` adds actual mode/Q/E/N,
reverse/forward, handbrake and visible skid command checks. Runtime results and original
comparison status must be recorded by the integrating agent after the rebuilt DLL runs.

Integration result: root-owned actual-window run
`py -3 tools/test_drive_window.py --controls --surfaces --output local/task0046/window-second`
passed 69 guarded OS checks. The first attempt stopped at manual selection because
ImGui's popup auto-fit repositioned the measured first-frame option (y178 to y188).
The harness now observes a stable widget center for250ms before real clicks; semantic
queue telemetry confirmed5 requested/5 committed commands on the successful run.
This was a test-coordinate issue, not a recovered driving algorithm correction.
A subsequent artifact-only rerun should retain controls-skids.png with live W/Space/A
and the original trail/HUD before key release; it adds no new acceptance assertion.

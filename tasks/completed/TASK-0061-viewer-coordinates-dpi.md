# TASK-0061 - TrackViewer coordinates and host UI DPI

- Status: completed
- Scope: live TrackViewer camera/world position at the top; shared ImGui and
  screen-space host text adapt to the window's monitor DPI, including monitor
  changes, without changing original game text layout or 3D world scale.
- Authority: user-requested port presentation only. No new PS2 behavior.
- Acceptance: correct native/host coordinate labels during movement/rotation;
  DPI100/150/200percent layout/font/input/scissors, resize and return-to-scale;
  font atlas refreshed only on scale change; no repeated cumulative scaling;
  Debug/Release builds, relevant tests and one Track11 window check.
- Preserve all existing edits and completed TASK0060 renderer/physics behavior.

## Initial inspection / next action

Shared DebugUiSession currently builds13px default atlas once and does not accept
DPI. Viewer FPS uses same ImGui session; viewer dimensions include hardcoded pixels.
Godot receives converted triangle/clip batches and mouse positions in viewport
pixels. Audit coordinate conventions before choosing a single scaling contract;
keep event coordinates, clipping, atlas and widget sizing consistent. Inspect
host Label text paths separately; do not apply monitor DPI to 3D labels or original
bitmap glyph metrics. Root user-owned frontend process is running: stage new DLLs
in an isolated output directory rather than terminating it or overwriting its DLL.

## Completed / validation

Live camera HP2 XYZ and Godot XYZ header in TrackViewer and its real-track drive
mode. Logical ImGui coordinates with physical framebuffer/scissor conversion,
inverse mouse mapping, physical external action targets and revisioned DPI atlas.
Windows GetDpiForWindow effective scale, shared0.2s monitor/settings poll; host
Control fonts adapt in Debug/Release with baseline/shared-resource/lifetime guards.
Original game glyph metrics, world labels, projection/render resolution unchanged.
See docs/UI_DPI.md and E0061U01. Strict successor9before/14after PASS; TASK0060
renderer and all recovered/physics bodies unchanged.

Debug/Release builds PASS with Godot4.4.1/GCC13.2/CMake3.29.2/Python3.14.4.
Debug CTest3/3 (debug_console,ui_dpi_source_review_rules,track_memory_source_review_rules)
PASS; Release2/2 (debug_console,ui_dpi_source_review_rules) PASS. New review suite
17cases. Actual Track11 DPI100/150/200/100 window checks PASS: live axis mapping,
font/target/capture, stable revisions, atlas growth and return, node remove/re-entry.
Atlas sizes512x128,512x256,512x256,512x128. Windows native scale reports2.0.
Release text/shared LabelSettings/lifecycle PASS. Existing console/viewer lifecycle
GPU regressions PASS at native200percent. Release Track11 native driving/main+6
capture/restart/failure/return/re-entry PASS. No all-track sweep, FPS or PS2 Verified.

Actual physical multi-monitor movement/non-Windows hardware unavailable; forced
scales exercise the same scale update/atlas path. High-DPI small-window panel/input
mismatch discovered and fixed by reserving header height in window constraints.
Legacy smoke expected8physical-pixel margins/hover positions; adapted to new DPI
contract and reran PASS. Initial unit failed at200percent before containment fix;
initial lexical source inventory misread #if defined as a function, switched to
#ifdef and confirmed exact reviewed delta. No suppressed runtime failure.

Exact build commands: cmake --preset godot-{debug,release}
-DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0061/staged-{debug,release}/godot/bin;
cmake --build --preset godot-{debug,release} -j4. Independent stage avoids the user's
running root DLL. Full runtime staging and hashes in local/task0061/delivery;
8DLLs/flavor, BUILD-INFO.json and manifests. Resource directories are workspace
junctions, not standalone exports. No user process terminated, no commit/staging.

Logs local/task0061/{build-debug,build-release,debug-tests,release-tests,
delivery-ui,delivery-release-text,viewer-regression-final,console-regression,
race-release,source-review}.log. Screenshots local/task0061/delivery-ui/dpi-*.png.
Continuation: launch local/task0061/delivery/Launch TrackViewer Native.cmd;
optional physical monitor move and verify automatic scale updates with steady
font revision between changes. Keep remaining effect gaps in TASK0052/0053.

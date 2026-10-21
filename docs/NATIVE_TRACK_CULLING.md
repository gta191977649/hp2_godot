# Original track visibility integration (TASK-0058)

Godot-managed HLOD is retired at the user's request. `original` is the only
production backend. Both startup `--track-lod-backend=godot_hlod` and API
configuration fail explicitly. No generated LOD is enabled. Previous experiment
is archived, not silently represented as a successful completed implementation.

## Shared pipeline

TrackViewer, Test drive and race_scene use the same Ps2TrackRenderer and Runtime.
Each normal main/capture frame calls Runtime::prepare_frame -> original eDisplayFrame
-> GrandSceneryCullInfo section selection/TreeCull/DrawAScenery -> StuffScenery
-> eModel/strip queues. Selection is independent for each scheduled capture view.
Godot submits/rasterizes the selected geometry with the common shaders and caches;
no nonzero engine visibility range is assigned to select LOD. Engine raster clipping
and ordinary frustum handling remain host rendering details, not a second LOD manager.

Recovered rules and address/version evidence: reverse/task0058/native-culling.md
(E0058R01), E0051S01, E0052E03 and docs/TRACK_CULLING_AND_GENERATED_LOD.md.
The normal branch uses original logical focal H, near/far adjustment, VisibleZone
shortlist, section hierarchy, instance flags/AABB/forward-radius rejection,
integer projected-size cutoff2, and first qualifying nonnull of three model slots.
Single-model scenery still has its own FarClipSize cutoff. No-zone fallback can
select a broad loaded set by original design. Resource residence is not drawing.

Show LOD remains an explicit browse diagnostic exception. It bypasses instance
level selection to show available authored levels together, with all host ranges0.
Disable it for normal visibility/performance assessment; driving suppresses it.
Static LOD helpers and historical tests remain solely for diagnostic/evidence use.

## Diagnostics and representative acceptance

get_lod_state reports backend=original, host_lod_selection=false, debug_all_lods,
and native_culling_views keyed by main0/capture1..6. Per-view counters include zone,
selected_sections, considered_instances, selected_instances, surface_commands and
native_selection. get_render_controls retains loaded counts, depth and timing; get_state exposes
original_focal_h.
Counters describe each most recently submitted view, not an aggregate GPU count.

Only Track11 was exercised, per user instruction. At the first actual-window test
view:128 loaded sections/8407 instances ->14selected sections ->203considered
instances ->62selected instances/990surface commands. Camera offsets50/100 give
131/121considered and54/52selected. This demonstrates culling, not FPS gain.
At native driving view main plus six captures all report native_selection=true;
selected counts102/31/37/19/54/72/49 are independent. These are fixed test views,
not every point on the track and not original hardware verification.

Debug and Release builds pass. Tests: native threshold/zone/tree/unit and Track11
indexed-vs-linear reference frames;6targeted CTests (runtime/static_lods/UI/both
source reviews/progress); actual-window native culling/Show LOD/clear/reload;
TrackViewer/Test drive/return/clear; Release race movement/capture/focus/failure/
restart/frontend/re-entry. No all30 sweep. Logs and screenshots are ignored under
local/task0058. Original-track screenshot native-track11-start.png was inspected.

Reproduce from workspace root:

```powershell
cmake --build build/godot-debug -j 6
cmake --build build/godot-release -j 6
build/godot-debug/track_runtime_tests.exe --assets 11
local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path godot --audio-driver Dummy --script res://scripts/runtime/track_native_culling_smoke.gd
local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path godot --audio-driver Dummy res://scenes/debug/track_viewer.tscn -- --track-load=11 --native-track-smoke
```

Local launchers: local/task0058/delivery/Launch TrackViewer Native.cmd and
Launch Race Release Native.cmd. Projects snapshot DLL/config and junction shared
workspace resource directories; they are not portable exported distributions.
Previous task56 launch shortcuts forward to these native entries. GPU CTests remain
opt-in OFF normally. Changes are uncommitted; previous unrelated edits preserved.

## Verification boundary

Recovered finite ordinary-float culling is not instruction-level EE/VU emulation.
No PS2/PCSX2 matched-frame oracle was run; Reconstructed, never Verified. Host free
camera and HD aspect mapping remain explicit port adapters. This task does not claim
all original camera gameflow, streaming or every exceptional override is recovered.
No recovered physics or prior recovered culling body was changed; nine port-source
body replacements were explicitly reviewed against frozen TASK0056 identities.

## Delivery sidecar correction

Initial task58 snapshot staging omitted FFmpeg DLLs, causing Windows Error126.
Workspace tests had not exercised the incomplete delivery bin directory. Fixed by
staging every DLL from the matching built runtime directory with
`tools/stage_godot_runtime.ps1`; this includes swresample and the remaining FFmpeg
transitive libraries. Per-flavor runtime manifests live beside delivery projects.
No extension binary/recovered behavior change or rebuild required.

Delivery correction final: direct task58 Debug viewer/Test drive PASS and direct
task58 Release RACE_RUNTIME_SMOKE_OK PASS, including exit/re-entry. Logs
local/task0058/delivery-debug-check.log and delivery-release-check.log contain no
extension/dependency errors. Both manifests have8DLLs and matching extension pins.
Packaging follow-up closed; no all-track test, native-code change or commit.

## TASK-0059 camera-movement disappearance correction

Movement plus rotation on Track11 exposed an unsupported AcidFX alpha bound, not
native culling stopping. The exception formerly escaped and unloaded the track.
The per-quad host validation boundary now reports/omits unsupported blend-bound
quads, retains supported particles/scenery and compacts node/stream/alpha ranks.
Atlas checks use the active UV frame footprint, not all unrelated animation cells.
See reverse/task0059/particle-boundary.md (E0059P01). High-alpha GS compositing is
still a recovery gap; no clamped-alpha substitute or complete-particle claim.

TrackViewer shows omitted counts/reasons; Reset Camera/R restores track start and
forward instead of world origin. Native culling, original particle simulation/RNG,
materials, physics and effects toggles are unchanged. Truly far/off-track cameras
can still select no surfaces normally; returning restores drawing.

Latest launchers are local/task0059/delivery/Launch TrackViewer Native.cmd and
Launch Race Release Native.cmd; previous task58 links forward there. Close old
windows before relaunching. Both runtime bins include all8DLLs. Direct delivery
Debug motion/rotation/reset and Release native-culling/particle-gap/clear/reload
checks passed, only Track11. Numeric/unit and source-review tests passed6/6.
Source 3before/4after delta preserves recovered bodies. No new FPS claim.

## TASK-0112 camera/widescreen checkpoint, 2026-11-06

The shared World now uses its actual source Player/DriveCameraMover/Camera,
with V POV selection, held B rear view and the existing temporary mouse adapter.
F11 toggles fullscreen; World projection uses the explicit Hor+4:3 reference.
Original Flailer records have committed read-only frame transport, including
authored countdown animation. Original HUD/pause/results producers remain
unfinished; the temporary text UI is still present. Historical host-camera
descriptions below describe the TASK-0107 delivery, not the current camera owner.
See `reverse/task0112/presentation.md` and active TASK-0112 for exact limits.

## TASK-0111 runtime-policy update, 2026-11-06

The source owner remains shared. The user now prioritizes minimal driving:
normal host launches use original no-cops/zero-traffic FreeRun configuration,
constructing one source player. Incomplete skid generation/display is disabled
before entry with console WARNINGs. --world-strict retains the historical13-car
scope and original guard boundaries described below. Existing counts/frontiers
below remain historical strict results, not current minimal-mode limitations.
See tasks/completed/TASK-0111-minimal-driving-warnings.md for acceptance and commands.

# TASK-0107 guarded World debug delivery

Delivered2026-11-03. Build `.\build.ps1 --run`, enter `trackviewer` in F8,
choose car/skin/track and press **Drive on track**. CarViewer **Test drive**, F8
`driving` and direct race_scene use the same source World. P pauses, period
single-steps, R restarts and Esc returns. This debug build keeps original FreeRun
traffic and police (13 participants); it is not a complete production race.

The reached shared-skid1A88C8 consumer freezes the last complete frame after a
short drive/turn. It is an expected first-delivery boundary, never bypassed by
disabling effects or switching to NativeBackend. Native original numerical
limits and explicit PC stack operands remain unverified. Full GameFlow/results/
save are excluded from this task. Dynamic prop projection is implemented/tested
with declared transport records; actual converted prop rendering was not reached
in the short guarded window drives. Per-face original car LOD/capture equivalence
and original wheel blur opacity are not claimed.

Native and normal Godot ALL builds pass. Current World/InputMap host tests pass,
as do retained authored roster and generator/naming contracts. Actual Python
window cases cover motion/guard/restart/return, real focus/F8 and3loading cancels.
Headless30restart+10viewer routes and actual Vulkan30restarts pass. Closed source
Memory returns to0 regions/bytes/allocations. GPU Ready nodes889, resources15,
audio players1 and texture memory110370816bytes stay fixed; buffer/video counters
vary by less than0.5KiB. External process private memory grew about65MiB net
across post-warmup GPU samples; attribution is unresolved, so no zero-leak claim.
Readiness counters do not expose internal worker queue lengths.

Actual-window combinations: ELISE/skin7/track11 and12, F50/skin2/track11;
ELISE/skin4/track11 for lifecycle (original dark Gunmetal). Native configuration
checks also cover ELISE tracks11/12/13 both directions; this does not establish
all car/track or continuous original-game behavior. No Verified promotion.
Historical embedded single-car Godot scripts are research history, not current
World acceptance. See E-0107-V01 integration-evidence.json for exact checkpoint
DLL hashes, commands, logs, own-viewport screenshots and limitations.

# Port World presentation contract

The central `world_debug_entry.gd` routes TrackViewer Drive on track, CarViewer
Test drive, F8 driving and direct `race_scene.tscn` to one WorldSession adapter.
The router retains choices and viewer camera context, with track11 as the initial
host debug default. `world_audio.gd` accepts committed shared stereo PCM only.
The scene polls loading, publishes step0 before running, pauses for focus/console,
and makes reached Blocked/Failed terminal until restart/return. Render passes do
not invoke World ticks. The UI calls `commit_presentation(step)` only after actor,
scenery and participant-shadow bindings succeed, then advances its display step
and consumes PCM. Binding rejection restores previous host poses/operands; the
native adapter keeps the previous particle/PCM publication. Display keys use session generation plus source address.

`Runtime::set_world_objects` atomically replaces small per-instance presentation
operands; source resources and game Memory remain unchanged. Section number and
record index bind an original static instance. `scenery_flags` is a complete
sparse override-set against the immutable asset baseline: absent records revert
to that baseline. Each row may also carry the exact source-published transform,
including backwards-track basis changes; the renderer never recreates the source
rotation algorithm. Removing a dynamic object does not clear a source hide flag.
Visible dynamic objects suppress their old static draw and publish the actual
source matrix and exact WorldObject model hash. Inactive mapped objects have no
pose publication and leave actual static flags/pose operands intact. Dynamic
presentation clears static direction/hide/depth-bias flags so reflection views
cannot add static +2Z bias to a committed WorldObject matrix. Unknown source associations fail explicitly. Visible bindings reject unavailable
hashes, non-affine/singular matrices and world-baked geometry. Disabled unmapped
objects are omitted only after explicit source/visibility/identity/type checks;
no pose is invented for an inactive mapped object.

Current-pose dynamic boxes and radii conservatively enclose transformed immutable
local solid bounds. Dynamic presentation uses the existing native scenery
frustum/radius/pixel classifier with one exact source model and zero model
threshold, and enters the queue before original strip/texture/alpha ordering.
This is an explicitly port-owned debug culling operand policy, not recovered
Smokeable rendering or proof of original dynamic visibility/LOD equivalence.
It bypasses the old static tree bounds. Source static matrix overrides likewise
use current-pose conservative boxes and enter the separate presentation pass;
static descriptor LOD/flags/facade behavior remains in the native classifier. Original classifier arithmetic is intact.
World operands require original scenery LOD mode; HLOD combinations fail clearly.
Immutable matrix baking and instance batching are disabled while sparse operands
are present, so main and reflection commands consume the committed matrices.

Each actual car has a separate shadow renderer keyed by generation/address.
Shared reflection cameras retain the player's capture origin. Host debug layers
are player1 and other actor geometry0x7f, showing other cars from the same
committed pose in shared captures while excluding the player's own mesh.
Captures reuse main-view vehicle LOD/shading; independent original per-face
vehicle classification is not implemented or claimed. No hidden/visible car
flag values are inferred in the absence of source-published display metadata.

Validation: isolated C++ syntax checks passed for runtime, scenery, tests and
Godot track bridge using configured compiler commands. Project-scene headless
parsing passed before the final C++ bridge rebuild. Shared builds, GPU checks,
source asset combinations, lifecycle resource baselines and dynamic original
render equivalence require separate results from the coordinating task. Nothing
here is Verified. The smoke harness labels injected terminal faults as port tests. It compares
source Memory footprints at step0 and after original cleanup, and checks the
host PCM queue is drained. This does not cover GPU cache or async worker resource
baselines. The running progress report is saved after every completed restart
and viewer roundtrip so coordinated DLL replacement cannot erase the checkpoint.
Wheel tire/brake matrices and angular velocity are source-derived. Blur opacity
retains the explicitly host research preview formula inherited from NativeBackend
(abs(angular_velocity)*0.02 clamped to1 then multiplied by90); it is not asserted
to be recovered World rendering or original wheel blur opacity.

Subsequent smoke runs also observe host node/resource/audio-player counts after
three paused settle frames per Ready. They record loading flags and, for real GPU
windows, RenderingServer texture/buffer/video memory usage. Summaries retain
ranges and trends after excluding the first three warmup samples; exact resource
counts are not asserted without a demonstrated steady state. Headless GPU scope
is explicitly uncovered. Native worker queue/thread/handle counts need external
process observation. Use `--world-debug-restarts-smoke` for30 restarts without
viewer routes; supply `--world-debug-report=<absolute local JSON path>` to isolate
parallel reports from the default `local/task0107/world-debug-smoke.json`.

## TASK-0111 minimal driving policy, 2026-11-06

The World host now defaults to **Minimal FreeRun**: one actual source player,
original no-police/zero-traffic setup producers, no AI/traffic/cop participants.
Incomplete skid trails are disabled before execution; wheel forces/contact,
physics, collision, original input and routes continue. Engine audio remains
active. F8 shows each missing function as a WARNING with original address,
reason, action and repeat counts. Warnings survive a hidden console and worker
threads. --world-strict retains the prior strict13-car diagnostic path.

This is a user-authorized PC preset, not original equivalence or a completed
Championship race. Mandatory simulation gaps still freeze the last complete
frame and produce a warning. P pauses, period steps, R creates a fresh source
World and Esc returns. See TASK-0111 and reverse/task0111 for tested scope.

# TASK-0046 player controls and skid marks

TASK-0107 supersedes the historical embedded single-car entry: CarViewer Test
drive, TrackViewer Drive on track, F8 driving and the direct race scene now use
one actual source World. The behavior authority remains Debug NFSHP2.ELF,
SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
The synthetic NativeBackend survives as a C++ research fixture only. Historical
plane/embedded-UI acceptance scripts below are not World acceptance; no current
World build claims their results. Source shared-skid publication1A88C8 is an
explicit current boundary. See TASK0107_WORLD_DEBUG_UI.md and DEVELOPMENT.md.
Implementation and original numerical certification remain separate.

## Controls and input ownership

Default keyboard bindings live in the Godot project InputMap, under hp2_drive_*.
The single port-side driving_input.gd adapter resolves physical events into a
semantic frame. Scenes and C++ do not interpret Q/E/Space as driving controls.
Future device mappings can bind the same actions without changing vehicle code.

|Action|Default|Behavior|
|---|---|---|
|Throttle|W|Forward pedal through original RealDriver logic|
|Brake/reverse|S|Original brake/reverse routing, without an added speed threshold|
|Steer left/right|A/D|Original player steering input/filter and vehicle chain|
|Shift down/up|Q/E|One request per press, manual transmission|
|Handbrake|Space held|Original EBrake input|
|Transmission|GUI Automatic/Manual|Original SetManualTransmission request|
|Neutral|GUI N, manual mode|Original desired gear NEUTRAL; Q can also reach N|
|Cursor|Esc / world click|Release / recapture, retaining the existing lifecycle|

Automatic is the initial transmission mode. R is selected through original S
routing; there is no direct R shortcut. W while reversing follows the original
braking/forward transition. Raw actual Gear0/1/2 map to R/N/1; AUTOMATIC-1 and
SPORT_SHIFT-2 are requests, not displayable forward gear numbers.

Queued commands are consumed only after a completed native step. Focus/F8,
keyboard capture and exit clear pending requests and held controls. Preview Q/E
bindings retain their own context. This task does not add a remapping settings UI.

HUD values are stored CarState/RealDriver values from the completed update: RPM,
actual gear, native speed/forward speed, transmission mode and handbrake state.
Speed is explicitly labelled in native units until a display-unit conversion is
independently evidenced. RPM is not estimated from throttle.

## Canonical chain and boundaries

Semantic frame -> Session/NativeBackend -> RealDriver::Drive and original gear
handlers -> PhysicsCar::Move -> Engine/DriveTrain/Wheel -> CarState publication.
Required REAL_CONTROL burnout handling resides in PhysicsCar::Move itself.
Original controller/world services remain explicit host bindings; unsupported
recovered paths throw hp2::NotImplemented instead of quietly substituting behavior.

Wheel::MaybeMakeSkids -> SkidMaker -> SkidSet/SkidSegment generates original
segments. RenderSkids/ePoly submit geometry, UVs, raw colours and texture identity
to the Godot rendering boundary. There is no Godot vehicle physics or independent
stripe generator. Original limits are eight segments per set and 128 sets; visible
rendering updates the original LRU order. Fade is view-distance based, not an added
timer. The single clan and elevation queries belong to the synthetic test world.
Original textures resolve through the configured ZZDATA resource entry point.

See reverse/task0046 for instruction evidence, exact recovery scope and tests.
The source-review chain retains frozen TASK-0044/0045 pins and appends TASK-0046
before/after identities; passing source checks is not PS2 numerical certification.

## Validation and continuation

Validation is in tasks/completed/TASK-0046-player-controls-skids.md. Reproduce
native checks with ctest --preset native-debug -R '^driving_' --output-on-failure;
host checks use the godot-debug preset. The owned-window acceptance driver is
tools/test_drive_window.py --controls --surfaces; it sends real input only to its
own foreground PID and retains captures under ignored local/.

Native builds passed. The full Godot CTest run passed75/76, then the sole historical
owner-count assertion was corrected and its targeted check passed1/1. All76 distinct
checks passed across these runs. Final real-window acceptance passed69 checks;
controls-skids.png was visually inspected for textured marks and the actual-state HUD.

Original comparison probe: py -3 -m tools.oracle probe --output
local/task0046/pcsx2-probe.json. The 2026-10-17 attempt timed out. A running,
synchronizable matching Debug scene and Debug-addressed oracle mapping remain
required before a bounded numerical comparison can establish Verified status.

## TASK-0083 collision and tire sound

Both driving hosts now play original impact, static wall/wood scrape and tire
skid/squall/burnout samples. No separate drift key is required: recovered slip,
traction, terrain and throttle state select the native voices; skid-mark geometry
is independent. H remains the port-side horn/flash binding. Pause and restart
silence/release audio. See reverse/task0083 for scope, tests and original runtime
comparison limits; rolling/damaged-tire and spatial services are not complete.

## TASK-0084 Test drive camera

TASK-0112 (2026-11-06) binds the actual source Player/DriveCameraMover/Camera
to WorldSession. V selects an eligible original POV after the original initial
one-second input gate; hold B for rear view. Right-drag or click the world to
capture the mouse for temporary observation; idle returns to the source camera.
Escape releases capture first; otherwise it retains current debug navigation.
F11 toggles host fullscreen. World uses explicit Hor+ projection relative to4:3,
preserving vertical coverage at16:9/21:9; original camera fields are unchanged.
The standalone research adapter retains its historical KEEP_WIDTH default.
Dashboard GPU presentation remains warned/disabled in minimal mode and guarded
in strict mode. Full original race HUD/pause/results remain active work. See
`reverse/task0112/presentation.md` and TASK-0112; no Verified claim.

Test drive defaults to the recovered fullscreen exterior camera (player
settings POV1), with the original normal/look-back transition. TASK0088 corrects
V to cycle the available original driving POVs, including forward-facing bumper.
Hold B to look back; release B to return to the selected POV. These keys are
port-side bindings of separate original actions. Mouse movement temporarily orbits the
current native target; after1.5seconds idle it smoothly returns to the selected
original view. Focus/UI suspension clears the temporary mouse override.

The latest user request supersedes TASK0084's Godot-default projection exception:
each selected POV uses original horizontal FOV (bumper/in-car90 degrees,
outside71.52099609375 degrees) and recovered near plane. Exiting restores the
viewer's prior projection and pose. Evidence and limitations: reverse/task0088
and reverse/task0084/camera.md and modes.md. Mouse
sensitivity, idle delay and return rate are host policy, not recovered PS2 tuning.

TASK-0111 control policy (2026-11-06): the minimal-driving PC preset selects manual transmission through recovered RealDriver::SetManualTransmission so Q/E operate gears. Strict mode retains original setup. Ghidra inspection of Debug001770D8/0013BC90/0013BD70 confirms automatic-mode callbacks reroute to Player preview/freeze, whose dependencies remain unrecovered. This is an explicit PC preset, not original FreeRun parity. Pause changes its label to Resume while paused.


TASK-0100 terminal diagnostics update2026-11-06: runtime missing-feature
WARNING messages are mirrored directly to stderr with full ELF address, reason,
action and repeat count, while the retained F8 console receives the same
messages. build --run uses the pinned console executable with inherited streams.
Distinct messages are not truncated; identical repeats use first/powers-of-two
summaries with exact counters retained. ABI boundary exceptions and host error
logs also go to stderr. Mandatory gaps still stop; logging does not suppress
exceptions or skip simulation.

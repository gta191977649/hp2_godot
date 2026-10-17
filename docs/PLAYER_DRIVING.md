# TASK-0046 player controls and skid marks

Test drive remains embedded in CarViewer. Its behavior target is Debug NFSHP2.ELF,
SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
This is a synthetic-plane vehicle test, not a recovered race/track environment.
Implementation and original numerical certification are reported separately.

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

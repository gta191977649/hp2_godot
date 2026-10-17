# CarViewer Test drive (TASK-0038)

TASK-0046 player controls, centralized Godot actions, transmission HUD and original
skid boundaries are documented in [PLAYER_DRIVING.md](PLAYER_DRIVING.md). This supersedes
historical brake-only/raw-input descriptions below for the Debug Test drive path.

Test drive is a third-person driving mode inside CarViewer. Load an original car and
click **Test drive**: the camera starts above/behind the vehicle, the mouse is captured,
and WASD drives immediately. There are no Run/Pause/Step/Reset/Record/Replay buttons.
The supported 911TURBO synthetic-plane loop has native acceptance tests; original
numerical equivalence is **not verified**.

## Open and configure

```powershell
cmake --build --preset godot-debug -j6
& .\local\tools\godot-4.4.1\Godot_v4.4.1-stable_win64_console.exe --path godot res://scenes/debug/car_viewer.tscn
```

Or open `carviewer` in F8, select the car/skin, close F8 and click Test drive. The legacy
`drivinglab` command routes to this same mode. Debug console OFF still supports the
CarViewer panel; Release rejects debug viewer entry.

Original assets resolve through `HP2_PS2_ASSET_ROOT` or ignored
`config/ps2_assets.local.json`. Since TASK-0044, driving resolves the executable through
`HP2_PS2_DEBUG_ELF` or ignored `config/ps2_debug_elf.local.json` (copy the tracked example).
Only Debug NFSHP2.ELF with SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b` is accepted.
There is no fallback to the Retail executable/configuration. Executable revision/date
remains independently unknown. Original ELF/BUN/geometry remain external to Git.
The ELF supplies static data and audited getter words, never executed host code.

See [DEBUG_DRIVING_AUDIT.md](DEBUG_DRIVING_AUDIT.md) for the canonical class call chain,
all 151 former projection dispositions, Debug algorithm changes and verification limits.

## Drive and look

- W accelerates; S applies the footbrake; A/D steer. S does not invent reverse operation.
  Gamepad left X and right/left triggers remain supported with the existing tool dead zone.
- Move the captured mouse to orbit around the vehicle. After1second idle, the camera
  smoothly returns behind the vehicle heading. Esc releases the pointer without pausing;
  clicking outside the panel captures it again.
- F8 and loss of application focus temporarily suspend simulation and clear controls.
  Closing F8/regaining focus automatically resumes, with no accumulated elapsed time.
  Pointer capture is restored only if it was wanted before suspension.
- Hovering the panel does not block WASD. Actual UI keyboard capture submits zero controls
  while simulation continues. Native errors stop stepping and display their reason without
  retrying or disguising the error as a user pause.
- The only mode button is **Return to CarViewer**. It restores the selected model/skin,
  preview settings, camera and preview wheel bindings. Re-entering is immediately drivable.
  No new function-key shortcuts exist; preview camera controls do not compete with driving.

## Camera and lifecycle ownership

The RefCounted tool controller owns Off/Starting/Driving/Suspended/Error states. Entry
requires a successful original config and complete first native snapshot; startup failure
leaves preview intact. Asynchronous LOD preparation completes before mouse capture/drive
activation. `release_camera()` only resets camera/input; it cannot permanently pause a drive.
Session pause/step/record/replay remain internal test capabilities, not product controls.

The third-person camera is an explicit port-side tool, not recovered GTA/HP2 camera code.
Source+X forward becomes host body basis+Z; the camera uses that confirmed axis, not an
assumed Godot -Z forward. Target is the vehicle's local bounds center transformed by its
body. Distance is max(4,1.5*vehicle length), initial pitch15degrees. TASK0050 aligns the
projection with original OUTSIDE bAngle0x32DC (71.520996 degrees horizontally), retaining
horizontal FOV across viewport aspects; the follow pose remains tool behavior. Relative mouse
sensitivity is0.15degrees/pixel; pitch clamps5..65degrees; yaw wraps around the vehicle.
Idle recenter uses1-exp(-4*dt) with shortest wrapped yaw and default pitch. Heading is
projected onto the horizontal plane, retaining the previous valid heading if necessary.
These are documented tool choices and do not change recovered dynamics or wheel matrices.

## Grip and material testing (TASK-0045)

Test drive now exposes Uniform material selection and world-anchored Zones for ROAD, DIRT,
GRASS, SAND and SNOW, with a collapsed read-only four-wheel grip panel. Material changes
commit on the next successful step without resetting the car. See [DRIVING_GRIP.md](DRIVING_GRIP.md)
for original coefficient/field evidence, the synthetic layout and test commands.

## Native implementation and limits

NativeBackend owns one bounded SHA-bound logical Memory image and original car config.
Recovered constructor/reset, component graph, engine/transmission, steering, tire/contact,
aero/body derivatives, five-pass solver, integration, publication and clock quantization
operate on that state. Candidate steps commit only finite complete publications. No Godot
physics, invented acceleration/grip or guessed missing original branch is substituted.

The fixture supplies identity orientation, zero initial motion, geometric wheel/radius
ground placement, original forward automatic operation, Race4+A758zero and synthetic flat
contact with original material1/SKID_ROAD by default; TASK-0045 adds the five audited surfaces. Original tracks/broadphase, obstacles/car collisions,
AI/police/race orchestration, full damage/audio and original camera are excluded. Gear/reverse/
handbrake controls are not exposed. Unsupported native paths still throw hp2::NotImplemented.

Current Debug evidence lives in `reverse/task0044/` and its SHA-specific function records.
Historical Retail evidence remains in `reverse/driving/`:
I-series initialization/scheduling, E-series drivetrain/mover/EE ISA seam, T-series tire/math,
B-series body/publication, C02 roughness. E-0038-U01 anchors positive steering as left
(A-D/negative stickX); E-0038-W01 shares original00120EF8 render-basis composition between
preview and driving, with distinct tire/blur versus brake matrices. Raw physics is unchanged.

## Validation and continuation

```powershell
ctest --preset godot-debug -R 'driving_.*host|debug_console$|debug_viewers_host|car_render_host' --output-on-failure
# Uses a dedicated owned window and real Win32 SendInput; never targets other apps.
py -3 tools/test_drive_window.py
```

Host acceptance clicks the real ImGui entry/return buttons, sends Godot keyboard/mouse
input, checks native motion/steering/braking/releases, rear camera/idle/wrapped angles,
keyboard capture, Esc/click, temporary suspension/resume with elapsed discard, terminal
errors and repeated preview restoration. Native backend tests retain deterministic
trajectory/reset/replay checks independently of the simplified UI.

The Windows test verifies 23 interactions (including foreground readiness) with actual OS keys, relative mouse, F8 and an
owned focus-probe window, then closes only its own test windows. Reports/screenshots stay
ignored under `local/task0038/third-person-os/`; a short-lived topmost flag keeps the owned
test window isolated from other running previews. Every input checks foreground PID first.
The OS harness requires Windows and Pillow for its screenshot. Headless tests check logical
capture intent; only windowed tests establish actual OS mouse capture.

TASK-0044 supersedes the Retail driving baseline and retains deferred Debug numerical comparison
(see DEBUG_DRIVING_AUDIT.md; the Retail oracle hooks must not be reused). Prerequisites: enter an original driving scene,
pause and run `py -3 -m tools.pcsx2_mcp`, then match owned function entry/return captures under
equal initial/input/contact conditions. Custom buildd75a0ad has JSON/TCP21512/PINE28011;
itsPC+4 step is not branch/delay-slot exact. No function is promoted to Verified by these
port usability tests. Original runtime state/settings are untouched by this tool redesign.

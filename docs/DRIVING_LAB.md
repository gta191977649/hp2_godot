# CarViewer Test drive (TASK-0038)

Test drive is a third-person driving mode inside CarViewer. Load an original car and
click **Test drive**: the camera starts above/behind the vehicle, the mouse is captured,
and WASD drives immediately. There are no Run/Pause/Step/Reset/Record/Replay buttons.
The supported911TURBO synthetic-plane loop has native acceptance tests; original
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
`config/ps2_assets.local.json`. The executable resolves through `HP2_PS2_ELF` or ignored
`config/ps2_elf.local.json` (see the tracked example). Only SLUS_203.62 with SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2` is accepted;
executable revision remains unknown. Original ELF/BUN/geometry remain external to Git.
The ELF supplies static data and audited getter words, never executed host code.

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
body. Distance is max(4,1.5*vehicle length), initial pitch15degrees, FOV60. Relative mouse
sensitivity is0.15degrees/pixel; pitch clamps5..65degrees; yaw wraps around the vehicle.
Idle recenter uses1-exp(-4*dt) with shortest wrapped yaw and default pitch. Heading is
projected onto the horizontal plane, retaining the previous valid heading if necessary.
These are documented tool choices and do not change recovered dynamics or wheel matrices.

## Native implementation and limits

NativeBackend owns one bounded SHA-bound logical Memory image and original car config.
Recovered constructor/reset, component graph, engine/transmission, steering, tire/contact,
aero/body derivatives, five-pass solver, integration, publication and clock quantization
operate on that state. Candidate steps commit only finite complete publications. No Godot
physics, invented acceleration/grip or guessed missing original branch is substituted.

The fixture supplies identity orientation, zero initial motion, geometric wheel/radius
ground placement, original forward automatic operation, Race4+A758zero and synthetic flat
contact with original material1/SKID_ROAD. Original tracks/broadphase, obstacles/car collisions,
AI/police/race orchestration, full damage/audio and original camera are excluded. Gear/reverse/
handbrake controls are not exposed. Unsupported native paths still throw hp2::NotImplemented.

Evidence ledgers live in `reverse/driving/` and the binary's driving function records:
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

The Windows test verifies22 interactions with actual OS keys, relative mouse, F8 and an
owned focus-probe window, then closes only its own test windows. Reports/screenshots stay
ignored under `local/task0038/third-person-os/`; a short-lived topmost flag keeps the owned
test window isolated from other running previews. Every input checks foreground PID first.
The OS harness requires Windows and Pillow for its screenshot. Headless tests check logical
capture intent; only windowed tests establish actual OS mouse capture.

TASK-0038 still has deferred original numerical comparison: enter an original driving scene,
pause and run `py -3 -m tools.pcsx2_mcp`, then match owned function entry/return captures under
equal initial/input/contact conditions. Custom buildd75a0ad has JSON/TCP21512/PINE28011;
itsPC+4 step is not branch/delay-slot exact. No function is promoted to Verified by these
port usability tests. Original runtime state/settings are untouched by this tool redesign.

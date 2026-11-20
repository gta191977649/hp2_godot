# TASK-0141 - Godot single-player race integration

- Status: active
- Scope: all authored single-player race entries and their Godot lifecycle.
- Dependencies: TASK-0123, TASK-0124, TASK-0125, TASK-0129 and TASK-0140.

## Acceptance

- [x] Track 33 enters through the original menu and the diagnostic entry.
- [ ] All authored campaign events and single-player modes pass their race lifecycle.
- [ ] Every selectable track variant and allowed direction has recorded coverage.
- [x] Debug and Release build and the Track 33 Forward+ Vulkan and D3D12 checks pass.

## Provenance

The Windows donor owns the active race flow. Its PAL comments are source
annotations with unresolved ELF revision and SHA-256. The former adapter
reported PAL185528 when it rejected tracks outside 12 and 13. The donor
comment at PAL185528..185558 documents the race-aborted result cell.
That address does not identify an original track admission rule.

The reverse scenery filter comes from the donor annotation at PAL10F928.
The donor applies reverse placement transforms during scenery loading. The
presentation adapter uses the committed direction to choose the matching
single-player placement mask. The replay restore epoch is port-side state.

## Validation

Windows x64, MinGW GCC 13.2.0, CMake 3.29.2, Ninja 1.12.0,
Godot 4.7.2 (ed1daf0bf), godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b.
Forward+ used an Intel Arc B390 on Vulkan 1.4.359 and D3D12 12_0.

| Path | Observed result |
| --- | --- |
| Track 33 World Racing Quick Race on Vulkan | A dedicated test profile set the donor's `track_selection=33`. The original menus started the race. Driving, pause and resume, restart to generation 2, and return to the main menu passed. |
| Track 33 diagnostic on D3D12 | Driving, pause and resume, restart to generation 2, and scene retirement on quit passed. Both Track 33 runs published 9,255 placements and 14 vehicles. |
| Track 33 instant replay on Debug Vulkan | The original Pause Menu restored a World snapshot and the bridge published replay epoch 1. |
| Track 33 career command | `--race-event=ultimate-racer:2` selected the authored Track 33 event on a fresh profile and the player responded to normal driving input. |
| All 64 campaign entries | The Release command-line driving matrix passed 64/64 after the diagnostic code moved into the MVP adapter. Earlier probes timed out for Ultimate Racer 17 on Track 46. The harness now tries normal car recovery if the vehicle stalls and isolates the process environment for each run. |
| All 30 authored scenario IDs | Headless Release diagnostic runs published a scene and exited 0. Covers the forward Quick Race load only. |
| Championship Track 12 and Ultimate Racer Track 13 | First event in each tree reached the live race through the authored menus. Driving, restart and return to the event tree passed. |
| Ultimate Racer Track 13 result | A fresh Debug profile ran the complete route using the donor AI test driver. The original reward, Race Results and End Game Menu appeared, followed by the event tree. The profile wrote a nonzero event time where the backup had zero. |
| Challenge modes 3 through 7 on Track 11 | Menu selection of Single Race, Knockout, Lap Knockout, Free Run, and Be the Cop passed driving, pause, restart and quit. |
| Reverse Track 11 | A fresh profile saw the authored reward lock. An isolated Debug profile using the donor test override selected reverse through the original picker and passed driving, restart and return. |
| Invalid input | Catalogue hole 54, malformed and out-of-range IDs, and a missing source path returned exit 1 with the requested ID and stage. |

The catalogue contains 30 scenario IDs, 64 events across two trees, 133
authored event legs, and 260 custom-mode, track-variant and direction
combinations before progression locks. Generate it from the original
resources with the following commands.

```powershell
# Set HP2_PS2_ASSET_ROOT to a complete local PS2 disc or archive.
cmake --build --preset godot-mvp-release --target stage_mvp mvp_race_catalog --parallel 8
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8
& ./build/godot-mvp-release/mvp_race_catalog.exe $env:HP2_PS2_ASSET_ROOT > local/task0141-catalog.csv
$engine = "$PWD/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
$project = "$PWD/build/godot-mvp-release/mvp-project"
$smoke = "$PWD/tools/test_mvp_race_lifecycle.gd"
$env:HP2_PROFILE_DIR = "$PWD/local/task0141-diagnostic-profile"
& $engine --path $project --rendering-driver d3d12 --resolution 960x720 --script $smoke -- --race-track=33
$env:HP2_PROFILE_DIR = "$PWD/local/task0141-menu-profile"
New-Item -ItemType Directory -Force $env:HP2_PROFILE_DIR | Out-Null
Set-Content "$env:HP2_PROFILE_DIR/config.ini" "format=1","track_selection=33"
& $engine --path $project --rendering-driver vulkan --resolution 960x720 --script $smoke
py -3 reverse/scripts/generate_progress.py
py -3 reverse/scripts/generate_progress.py --check
git -c core.safecrlf=false diff --check
```

Use `HP2_MVP_CHALLENGE_MODE=3..7`, `HP2_MVP_EXPECTED_TRACK`, and
`HP2_MVP_PICKER_DIRECTION=1` to exercise a Challenge menu selection.
`HP2_MVP_CHECK_REPLAY=1` uses the ordinary replay menu after driving.
Debug builds accept `HP2_MVP_TEST_UNLOCK_RACES=1` to exercise authored
entries with the donor's testing override and an isolated test profile.
Local evidence is under `local/task0141-*`. Windows held the staged Release
DLL during an overlapping smoke, so the final DLL was also tested from a
separate local project copy.

To test a locked event directly, use `./build.ps1 --race-event=ultimate-racer:2`
or `./build.ps1 --race-event=championship:1`. The command selects the first
authored allowed car and skin and skips the startup and pre-race menus.
It uses a fresh diagnostic profile inside the staged project's `profiles/`.
Quick Race and campaign diagnostic selection, validation and test unlocks
live in `src/mvp/assets/race_diagnostics.cpp`. The core exposes a one-shot
host callback after the authored frontend banks become ready.
Run `./tools/test_mvp_campaign_matrix.ps1 -Jobs 5` in PowerShell 7 after
building `stage_mvp` and `mvp_race_catalog`. The script reads the catalogue,
launches each authored event in a separate headless Godot process, sends
normal acceleration input, and records movement, exit codes and logs. It
retries failed entries once serially and retains both attempts in the CSV.
The passing report is `local/campaign-matrix-d5d503e8-97ec-4bf8-bcaf-2c4279eaedf2/results.csv`.

## GitHub issue #1 gamepad input

Ordinary launches now read Godot's mapped gamepad state. A button press or
deliberate stick movement selects the active P1 gamepad, including devices
connected during a race. Keyboard input remains available with a gamepad
connected. The frontend and replay forward their respective inputs to the
existing controller scanner. The keyboard defaults use arrow keys in menus
and races, Backspace for Back, and Esc for pause and replay exit. Name entry
and credits reuse the menu actions. The Track Viewer keeps its free-camera
WASD shortcuts. This change is in the Godot host.

The Windows checks used Godot 4.7.2. The tracked menu smoke injects Godot
events on an enumerated virtual Xbox 360 controller. A separate ViGEm run
sends operating-system gamepad input. A physical wireless DualShock 3 and
the reporter's Bluetooth setup were unavailable for validation.

- `cmake --build --preset godot-mvp-release --target stage_mvp --parallel 6`
  and the corresponding Debug build passed.
- `test_mvp_controller_menus.gd` passed duplicate binding checks, title and
  main menus, selectors, options, credits, event entry into Track 12, pause
  options and confirmations, and replay transport on both input methods.
  The gamepad boot run created and saved `aB` using the on-screen keyboard.
  `test_mvp_boot_profile.gd` also passed its keyboard name and save flow.
- `test_mvp_race_lifecycle.gd` passed Track 33 driving, pause, resume, restart,
  and scene teardown with keyboard input and an injected gamepad variant.
  The OS-visible race smoke passed acceleration, steering, braking, pause
  and resume, hot-plug, switching between two controllers, keyboard input
  while both were connected, and reconnect.
- `test_mvp_results_navigation.gd` passed the Track 13 results, lap times,
  and exit confirmation on the Debug fixture with both input methods.
- `test_mvp_graphic_menu.gd` passed the settings and confirmation checks
  with each input method on Windows Vulkan. The Be the Cop gamepad smoke
  also passed Race Objective and police controls navigation in the pause menu.
- `test_mvp_native_debug.gd -- --race-track=33 --debug` passed headless
  and on Vulkan. An earlier Vulkan attempt saw mouse orbit active at the
  camera restoration assertion.

Run the tracked menu smoke with a connected controller on a staged project
with local game assets. Set `HP2_MVP_MENU_KEYBOARD=1` for the keyboard run.
Set `HP2_MVP_CONTROLLER_BOOT=1` only on a disposable staged project with no
profiles and without `HP2_MVP_TEST_PROFILE`. With a regular test profile, use
`HP2_MVP_TEST_PROFILE=1`. Local logs and OS input fixtures are under
`local/issue-1`.

```powershell
& $engine --path $project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_controller_menus.gd"
py -3 local/issue-1/controller_smoke.py
```

## Controls settings hub

Options opens a three-row Controls hub with a live device name and separate
Keyboard Settings and Controller Settings pages. The host borrows the HP2
Options art and fonts, uses the original highlight and back stack, and
aligns the headings, text columns and selection rails with the movie
panel. Godot clips the
background video to the authored movie panel while preserving its aspect
ratio. The keyboard page captures one physical key per driving action and
confirms conflicts before swapping. Menu shortcuts stay available during
normal navigation. The controller page opens the existing layout picker and
adds adjustable left-stick dead zone and sensitivity.

The alignment smoke in `tools/test_mvp_controls_settings.gd` checks the
movie edges against the row rails, confines the tab rails to the selected
tab, and checks the steering arrow against its value. The Release project was
checked at 960×720 with Vulkan and 1280×720 with Direct3D 12. Debug and
Release use the `stage_mvp` build target. A separate Display Options
pointer-scroll test reports row `52e039f9` missing, also reproduced by
entering Display Options directly without visiting Controls.

The Godot Options entry displays the host's Controls label while that
page is live. It restores the authored label and language ID on close.
The submenu rows use plain labels and the HP2 selection highlight.

Keyboard Settings uses three HP2-styled tabs. Their focus rails fit the
selected tab, and the binding list uses full-width rails and the original
Display Options scroll arrows. Cop Mode presents its shared Siren key as
a read-only row. The native gray footer describes the selected option.
The scroll arrows convey overflow without navigation text.
Key capture feedback stays in the active value, and a save failure opens
the native one-button dialog. Borrowed art is restored when leaving.

Display Options supplies seven fixed `BOTTOM_MASK` pieces in addition to
its item masks. The Controls pages now hide those fixed pieces and render
an authored `MI_1_MASK` copy for each port-side row. The native
`FlailerSelectionSet` clears the selected mask, and the same row geometry
drives the mask, caption and scrolling. The port exposes the existing
`OptionsScreenBase::PositionHighlightBars` placement for these rows. The
stock Options caller uses the same placement and offsets. The restored
footer uses `FlailerAnimatedBlurb`, with its original artwork, typography,
and position. The legacy Display Options presentation data is restored on
page exit. The Controls smoke checks the device-row shade, scrolling
geometry, footer bounds and Display Options restoration.

Controls takes the row pitch, mask height and text offsets from the
authored Options assets. The front-end smoke compares the mask height
and pitch with the live Audio Options page. Keyboard Settings uses five
visible binding rows so it can retain the authored gaps, the category
tabs and the reset row within the movie panel.

Godot previously bypassed the native description fade and slide.
The existing `FlailerAnimatedBlurb` now runs its PAL249348 animation
under the external frontend presenter. Its single-text animation group
can update the original Flailer coordinates, and Controls clips the
moving text to the authored footer. The original alpha, duration and
cubic slide come from the implementation documented in
[Frontend scroll evidence](../../docs/FRONTEND_SCROLL_BACKGROUND.md).
The smoke observes the slide settling in both Audio Options and Controls.

The Options extension previously repositioned only the rows after
Graphics. Its host layout now uses one pitch and mask height for every
enabled entry. It removes the borrowed fixed gap masks while active and
restores the borrowed objects on exit. The full set of eight Options
entries fits inside the authored video panel.

The released NTSC Options branch hides Languages. The Godot host instead
uses the native Language Options screen when the mounted assets contain
multiple selectable language packs. The availability check requires the
authored text and speech resources and shares the original speech-file
mapping from PAL table 2B6228. The native language loader and the profile
serializer still own the language value. The host calls the ordinary
profile autosave after a successful selection when enabled. See
[Language Options evidence](../../docs/FRONTEND_MENU_LANGUAGE.md) for
the original callbacks and source identities.

The Windows language smoke on the US asset image checked equal Options
mask spacing and changed English to French and back. It checked that the
Options strings refreshed, the reopened language list selected the
current language, and both changes reached the saved profile.

Release and Debug staged with `cmake --build --preset godot-mvp-{release,debug} --target stage_mvp`
using the respective presets. `tools/test_mvp_controls_settings.gd` passed
at 960×720 and 1120×630 with the virtual Xbox controller and mouse.
The reload pass verified both reset dialogs, and
`HP2_MVP_CONTROLS_SAVE_FAILURE=1` verified that a failed save opens the
message and leaves the live binding intact. Visiting Display Options
after Controls verified restoration of the borrowed artwork. Captures
are under `local/controls-settings/restyle-4x3` and `restyle-16x9`.

The reference was `recomp/runtime/pcsettings.cpp` and
`recomp/runtime/pcplatform.cpp` in the local `nfshp2-port-win` checkout.
The Godot host keeps physical keys and stick tuning in `launcher.cfg`.
The existing controller layout chooser keeps saving presets to the active
HP2 profile. Loading missing settings uses the default Godot bindings and
the previous stick response.

The game core needed two port-side integration hooks. The existing Options
row can open registered host pages through the original menu owner. The
sample's optional layout override lets keyboard actions use the existing
default layout even when the controller uses a different layout. The
scanner releases mappings that change and retains edge state for shared
bindings. No authored table, profile format, race rule, or original
equivalence status changed. This was checked with a real race using Layout C,
with the keyboard throttle and brake bindings swapped.

### Verification

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8
& $engine --path $project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_controls_settings.gd"
& $engine --path $project --rendering-driver vulkan --resolution 1280x720 --script "$PWD/tools/test_mvp_controls_settings.gd"
& $engine --path $project --rendering-driver vulkan --script "$PWD/tools/test_mvp_control_layout_race.gd"
```

Use disposable staged projects and local game assets. For the menu smoke,
start with a fresh launcher configuration and set `HP2_MVP_TEST_PROFILE=1`.
Set `HP2_MVP_CONTROLS_RELOAD=1` on the next launch to verify and restore
the defaults. For the race smoke, use a connected gamepad and a disposable
ordinary profile with its P1 layout set to C. Copy the swapped Up/Down bindings,
10% dead zone and 110% sensitivity from the first menu smoke. Clear
`HP2_MVP_TEST_PROFILE` for the live race so it loads that profile.

Windows Release and Debug staging passed. The race smoke verified
Layout C's Cross brake and Square throttle, the remapped keyboard actions,
stick tuning, and release of throttle and steering while switching devices.
The keyboard Track 33 build passed race pause, resume, restart, and quit.
The keyboard frontend and replay regression passed. The Vulkan controls
smoke passed at 960 × 720 and 1280 × 720. It checked video coverage and
aspect, header alignment, focus, keyboard and controller navigation, mouse
adjustment, reserved keys, conflict cancel and swap, and both resets after
reloading settings. The OS-visible virtual Xbox 360 controller passed
disconnect and reconnect in the hub and the Track 33 race. A physical
wireless DualShock 3 was unavailable. Logs and screenshots are in
`local/controls-settings`.

## Be the Cop follow-up

Ultimate Racer events 5, 9, 13, 18 and 27 use authored event kind 6. The
donor also sets the same pursuit flag for custom Be the Cop. The flag selects
the police HUD and results through the existing Player and EndGame owners.
The Godot keyboard shortcuts J and N send the existing Triangle+R2 and
Triangle+L2 scanner chords. The modifier precedes the primary edge.

Run the dedicated smoke with a fresh profile and the local game source.

```powershell
& $engine --path $project --rendering-driver vulkan --resolution 800x600 --script "$PWD/tools/test_mvp_be_the_cop.gd" -- --race-event=ultimate-racer:5
```

The smoke checks the authored timer strip, the player's officer binding,
target acquisition, both support requests, and Be The Cop Results 1P.
Set `HP2_MVP_POLICE_HUD_ONLY=1` to check the police role and HUD at entry
without waiting for a result.

Release confirmed the officer binding and authored timer strip in all
five campaign events. Event 5 acquired a target using H, consumed one
roadblock and one helicopter request, and reached Be The Cop Results 1P
after the donor ended the run. Logs are `local/task0141-police-hud-*.log`
and `local/task0141-police-event5-both.log`. Custom Be the Cop passed the
officer binding and HUD checks through the normal menu, drove, restarted
and returned to the menu in `local/task0141-police-custom.log`.
Debug and Release staged successfully. The police results smoke covered the timeout
result. Arresting every opponent and the corresponding victory bonus
remain open.

## PC keyboard defaults

The supported driving and police actions use the supplied PC defaults.
Nitro uses Shift in Be the Cop, and N calls the helicopter. The PS2 player
scanner exposes roadblock and helicopter requests, so there is no Call
Backup action to bind. Keyboard capture and menu navigation share the
arrow keys by context. Old custom bindings still load when their keys do
not conflict with the new defaults.

Keyboard Settings and Controller Settings each offer Restore Defaults.
Keyboard reset saves the PC bindings to the launcher configuration.
Controller reset restores Layout A, vibration on, 8% dead zone and 100%
sensitivity for P1. It applies the layout to the live input router and
uses the normal profile autosave when enabled.
The reload smoke started with swapped Up/Down keys, Layout C, vibration
off and 10%/110% stick tuning. It verified both restores, the controller
Cancel path and Layout A with vibration on in the saved profile.

The staged Release race reported throttle for Up, brake for Down and the
correct signed steering for Left and Right. Be the Cop event 5 acquired a
target with H and consumed both J and N support requests. Debug and Release
staged with `cmake --build --preset godot-mvp-{debug,release} --target stage_mvp`.

## Keyboard categories and camera input

The Keyboard Settings page opens on Frontend. Left and Right switch tabs
while the tab bar has focus, Down enters the selected list, and tabs also
accept pointer clicks. Each group retains its scroll position. The
keyboard-wide Restore Defaults stays below the list and saves through
the existing launcher configuration. The device row appears only on the
Controls hub. Cop Mode shows the binding for the shared
Horn / Siren action, which is editable in Race Mode.

Delete opens native frontend help and removes a character in the name
editor. The regular controller menu uses Y for Help and B for Back.

Space applies the handbrake. Tab cycles the HUD using the existing PS2
D-pad Up action. Menu and race actions can share keys by context, while
shared pause and replay actions are checked across both contexts. The
old W and S mouse camera entries have been removed. Only mouse motion
engages free orbit, with the existing idle blend back to the driving view.

Q requests Look Ahead (Zone Zoom) and E requests the 360° Camera
(Zone Freeze). The existing scanner table registers PS2 events 0x60 and
0x5f for these operations. The recovered player handler calls
`carFramePlayerStartState` with state 2 or 1 through the existing
RequestZone implementation (PAL 0x00177c88–0x00178354). The Godot host
passes a port-side keyboard edge to `joystickGenerateEvents` so the
registered callbacks, native eligibility, Joylog and replay use the same
route as pad input. The original player code remains untouched. The
PAL address comes from the existing donor annotations. The staged
runtime smoke uses the US `SLUS_203.62` asset revision (ELF SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`).
This integration does not establish original runtime equivalence.

### Independent driving shortcuts

The Godot keyboard had synthesized Triangle for Shift, J and N, so the
source scanner could also cycle the camera. A and Z could request zone
cameras or police support through the native automatic transmission
fallback. Godot now queues each dedicated camera, Nitro and police support
edge through the existing typed scanner/replay path. The bindings use
native events 0x5d, 0x5f, 0x60, 0x7f, 0x80 and 0x81. A and Z are forwarded
only when the live driver can shift. The original gamepad scanner, native
driver callbacks and PAL Nitro eligibility at 0x0013c25c–0x0013c260 remain
the source of truth. Godot exposes Nitro under Cop Mode and disables the
donor's optional PC racer Nitro override when it loads the profile config.

Windows checks passed `HP2_MVP_TEST_RACE_SHORTCUTS=1` with
`tools/test_mvp_race_lifecycle.gd -- --race-track=33`. Shift produced no
camera, handbrake or Nitro action, A/Z produced no zone request, and C
changed the camera. `tools/test_mvp_be_the_cop.gd -- --race-event=ultimate-racer:5`
verified Shift and virtual Xbox R3 Nitro, C, J and N, and no automatic
A/Z support requests before reaching the results. The matching Nitro
postprocessing and particle probes passed on that Cop event. The
Controls smoke confirmed Nitro is in Cop Mode.

### Grouped controls and panel verification

- Release and Debug staged with `cmake --build --preset godot-mvp-release --target stage_mvp --parallel 6`
  and the equivalent Debug preset.
- `tools/test_mvp_controls_settings.gd` passed first-run and reload
  checks at 960×720 and 1120×630. It covered device names and hot-plug,
  keyboard, pad and pointer navigation, per-tab scroll, conflicts,
  persisted overrides, both resets and the panel's video coverage.
- `tools/test_mvp_controller_menus.gd` passed frontend, pause and replay
  navigation with a Windows virtual Xbox 360 controller.
  `tools/test_mvp_control_layout_race.gd` passed live keyboard/pad
  switching, Layout C, and both stick tuning checks.
- `tools/test_mvp_race_lifecycle.gd -- --race-track=33` passed the keyboard
  race lifecycle. `tools/test_mvp_be_the_cop.gd -- --race-event=ultimate-racer:5`
  reached the police results after acquiring a target and using both
  support keys.
- `HP2_MVP_TEST_ORBIT_INPUT=1` with
  `tools/test_mvp_orbit_camera.gd -- --race-track=33` verified mouse-only
  engagement, idle return, interruption by mouse motion, Q and E,
  the Tab HUD shortcut, Space handbrake and cursor release on pause.
  The extended camera geometry smoke varied with the Quick Race vehicle
  and track fixture. No camera collision or interpolation code changed.
- A physical wireless PS3 controller was unavailable. Local logs and
  screen captures are under `local/controls-settings`.

## Frontend list scrolling

Keyboard Settings, the Jukebox playlist and the profile picker previously
replaced rows immediately when their selection crossed the visible window.
They now share a port-side clipped viewport driven by the frontend clock and
the existing Flailer cubic. Each label, value and selection marker follows
the current scroll position. The underlying input and profile operations
still commit immediately. The authored Flailer scrollers keep their timing.
The Jukebox's native path retains its original five-slot behavior.

Build with `cmake --build --preset godot-mvp-release --target stage_mvp` and
the corresponding Debug preset. `tools/test_mvp_controls_settings.gd` and
`tools/test_mvp_boot_profile.gd` sample row motion in Keyboard Settings and
the profile picker. `HP2_MVP_TEST_SCROLL=1` enables Audio Options and Jukebox
sampling in `tools/test_mvp_menu_navigation.gd`. The keyboard motion checks
also passed in both directions at 30 FPS.

## Menu video backdrops

Frontend movies use one aspect-preserving cover calculation clipped to
their active Flailer movie quad. Menu videos extend to all four edges of
their panel. The surrounding grid and menu content stay visible.
Startup logos and the intro use their authored display rectangles.
The YUV shader follows frontend canvas fades.

Builds with `cmake --build --preset godot-mvp-release --target stage_mvp`
and the corresponding Debug preset passed. The Release Godot project
passed `tools/test_mvp_menu_navigation.gd` on Direct3D 12 and
`tools/test_mvp_controls_settings.gd` on Vulkan. The video sweeps in
`tools/test_mvp_frontend_resize.gd` verified the main menu and startup
videos at 800×600, 1120×630, 1260×540 and 540×720 with
`HP2_MVP_VIDEO_RESIZE_ONLY=1`. The menu navigation script checked the
Hot Pursuit, World Racing, Main Menu and Options video panels.
The reviewed Options capture is
`local/controls-settings/panel-final-4x3/options-video-panel.png`.

## Godot gamepad takeover and vibration

The host samples the mapped driving axes through Godot and transfers input
after a fresh crossing of the steering or pedal dead zone plus five percentage
points. The native PS2 layout still owns steering, pedals, and button actions.
The host forwards the existing native two-motor output to the selected pad and
stops it on device, focus, and race lifecycle changes. This required no new
game-core changes.

`cmake --build build/godot-mvp-release --target stage_mvp -j 8` and
`cmake --build build/godot-mvp-debug --target hp2_blackbox -j 8` passed.
`tools/test_mvp_control_layout_race.gd` ran through one World Racing race
on Vulkan with a ViGEm Xbox controller, layout A, and a disposable profile.
It verified steering in both directions, keyboard takeover, both pedal inputs,
handbrake, horn, look back, camera, HUD, pause/resume and both shifts. ViGEm
reported strong-motor output from the original pause options preview followed
by zero. Select did not produce the expected recovery timestamp. Testing
ended at the user's request. Driving vibration, other layouts and modes, and
physical DualShock 3 behavior remain unverified in this run. Local trace
`local/rumble-validation/single-race-final.log`.

## Issue #4 analog trigger layout

Options → Controls → Controller Settings → Controller Layout offers Layout E.
R2 is analog throttle and L2 is analog brake/reverse. The right stick
remains available. Cross and Square shift up and down in manual mode.
The native automatic routing and Triangle chords use those face buttons
for zone cameras or police support. Restore Defaults selects Layout A.

The Godot host already forwards both trigger pressure bytes to the native
scanner. The existing profile layout field and original menu provide the
selection and persistence. Game-core integration adds port-side layout ID
34 to the scanner and profile validator and reuses the Layout A controller
art in the picker. The new rows send direct pedal events so the original
face-button throttle amplification does not saturate a partial trigger pull.

The existing mapping reference is the final PAL scanner table at
`0x002DC500` and device table at `0x002DE558`. The exact donor ELF SHA-256
remains unresolved. Layout E is a port addition, informed by Silent's
trigger layout patch as used by `nfshp2-port-win` at
`8ae05937be6865dde1e45c108cecd79f247d6b7a`. It has no original-game
equivalence claim.

### Verification

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
cmake --build --preset godot-mvp-debug --target hp2_blackbox --parallel 8
$env:HP2_MVP_CONTROLS_TRIGGERS = '1'
& $engine --path $project --rendering-driver vulkan --script "$PWD/tools/test_mvp_controls_settings.gd"
$env:HP2_MVP_CONTROLS_RELOAD = '1'
& $engine --path $project --rendering-driver vulkan --script "$PWD/tools/test_mvp_controls_settings.gd"
$env:HP2_MVP_TEST_PAD_LAYOUT = '4'
$env:HP2_MVP_SKIP_PAD_RECOVERY = '1'
& $engine --path $raceProject --rendering-driver vulkan --script "$PWD/tools/test_mvp_control_layout_race.gd"
```

Use a disposable project with one named profile for the two menu launches.
The race smoke uses a separate disposable profile with Layout E and the
swapped keyboard bindings and stick settings described above. The A and C
regressions use the same smoke with layout indices `0` and `2`. Recovery
is skipped because the prior TASK-0141 fixture already recorded a failing
Select recovery check.

Windows Release staging and the Debug extension build passed. Debug staging
could not replace the DLL in the shared staged project. Both Layout E menu
runs passed with a ViGEm Xbox 360 controller. The picker screenshot shows the
pedal, shift, and camera glyphs. The second launch loaded the saved Layout E
and Restore Defaults wrote Layout A.

The Track 24 race passed partial and full trigger pulls, release, reverse
selection, keyboard/controller takeover, both zone camera chords and manual
shifts. The Layout A and C regression runs passed. A local scanner smoke
checked both police support chords and confirmed that pedal pulls do not
produce their events. Live police support activation and a physical controller
remain unverified for Layout E. Logs and the picker capture are under
`local/layout-e-*`.

## Remaining work

Complete each event to its donor-defined result. Check multi-leg
continuation, persisted progression, and the remaining track direction
and custom mode combinations. The fresh-profile Track 13 run recorded a
result in the save, with 1,307,560 ticks against a 1,200,000-tick objective.
The subsequent normal menu probe could not start the locked Track 33
event with that result. The diagnostic covered its launch and driving.
PC coverage has not established PS2 original equivalence.

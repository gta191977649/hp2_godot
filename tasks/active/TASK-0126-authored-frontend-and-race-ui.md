# TASK-0126 - Authored frontend and race UI

- Status: active
- Depends on TASK-0123, TASK-0124 and TASK-0125.
- Scope: present the donor's Flailer fonts, bitmaps, event tree and in-race UI in Godot. Keep input, screen flow, HUD values and authored layout in game_core.

## Police radio subtitles

The port-side Display row belongs immediately after Zone Cams in the
frontend and below HUD Hotkey in Pause > Options > Display. Both screens
use their authored Flailer text and native selection. The pause screen
owns its menu and input handling in game core, so its callback installs
the same port-side row and uses the existing host preference callback.
The pause list uses the frontend's existing cubic scroll animation.
When the selection reaches Subtitle the list slides by one row. A clip
window keeps the moving labels and values inside the authored list
above the divider. Moving back to HUD restores the first row.
The toggle is saved in launcher.cfg under
`accessibility/police_subtitles` and defaults to Off.

The original `spchMakeSampleRequests` arguments carry the event ID and
selected offset. SED already assembles the order and adds 48 bytes
when submitting the PCM payload. The caption snapshot keeps the
original VAG header offset for the text lookup. The shared queue
acknowledgement publishes playback start, and the native voice manager
publishes EOF using the observed PLAY sequence before its STOP changes
the source sequence. These are port-side readouts of existing audio
operations. They preserve the original audio scheduling and callback
order.

The English transcript comes from MTA HP2. The add-on matches its
offsets against the English DAT hash. Godot uses the original
`0x6486623B` Complete value's font texture and tint, including its
baked edge. The Time Left label (`0x0390D3E1`) supplies the line height.
Centered lines wrap upward from the visible right gauge face, excluding
transparent texture padding. Original source is US PS2 `SLUS_203.62`
with ELF SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.

Windows Debug `stage_mvp` and `./build.ps1 --mvp --jobs 8` passed.
`tools/test_mvp_police_subtitles.gd` verified Display selection rails
and global persistence. On Track 13 it exercised original radio
events 96, 100 and 101 through XAudio2 and Godot, including pause,
resume, natural completion and the Off case. The alignment smoke runs
use `--race-event=ultimate-racer:1` at 1000×750 and 1280×720. The Release
export staged the transcript and passed the headless extension probe.
Logs and captures are in `local/subtitles-040856`. The clean 16:9
capture is `sample-completion/race-police-subtitle.png` in that directory.

The Pause Display smoke check reused `tools/test_mvp_police_subtitles.gd`
on Track 13 with `HP2_MVP_TEST_MENU_ALIGNMENT=1`. It checked scroll
placement, both persisted values, reopening Display, and race resume.
Debug `stage_mvp` and the Windows Release build and extension probe
passed. See `pause-smooth.log` and `pause-smooth-release-extension.log`
in the same log directory. The in-motion capture and intermediate row
position check are under `pause-smooth-motion` and
`pause-smooth-motion.log`.

## Live leaderboard vehicle swatches

Retail US PS2 `SLUS_203.62` SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
In `RenderHud__6Player`, `0017FE60` calls `GetSkinInfo` and
`0017FE6C` calls `ChangeTexture__18FlailerBitmapChunkUi` at
`00141350` with the word at SkinInfo+0x30. The older Debug
`RenderHud` at `0017D738` names the same field `ColourSwatchHash`.
The core had passed the hash to `SetColor`, which produced unrelated
RGBA values. Each existing `LIVE_SPLIT_SWATCH` now switches to the
authored texture for its current ranked vehicle, with the normal
Flailer cache reused until the hash changes. The row's status panel
and player highlight retain their authored treatments.

Debug `stage_mvp` built. Release linked successfully, and the running
Godot instance held the standard staged Release DLL. The verification
project under `local/hud-rank-swatches/project` uses the newly linked
DLL, confirmed by SHA-256. Godot 4.7.2 Forward+ passed
`tools/test_mvp_frontend_resize.gd -- --race-track=12` with an isolated
profile on Vulkan and D3D12, including the HUD, pause and restart.
The Vulkan HUD capture shows green, silver, yellow and black swatches.
The D3D12 capture shows yellow, silver, green and blue swatches.
See the two `direct-hud-800x600.png` captures under
`local/hud-rank-swatches`. `git diff --check` passed.

## Acceptance

- [x] Render original atlas glyphs and bitmaps through Godot in the title, main menu, mode selection, event tree, event details and car selection.
- [x] Enter Track 12 through Quick Race and Championship. Present the starting grid, race HUD and pause menu, then resume or return to the frontend.
- [x] Keep resources local and record donor and asset provenance.
- [ ] Finish migration of authored visual animations to Godot and validate the latest changes when requested.
- [x] Present the live rear view with original textures and centered resize behavior.
- [ ] Cover race results, replay presentation, remaining screens and unverified locales. Exact GS blend and sampler parity remains open.
- [x] Enter Track 12 and Track 13 with Godot mouse input from the normal frontend.
- [x] Dismiss the Debug console from smoke scripts and verify that the open console blocks clicks through to Pause Menu.
- [ ] Finish mouse verification for settings, replay, results and locked events.
- [ ] Confirm Windows Auto HDR activation with Xbox Game Bar on an HDR-enabled display.

## Evidence

The rotating minimap uses the reflected world Y axis and aligns local
positive Y with the player forward direction. The resulting source angle
is map rotation minus player heading minus a quarter turn. The texture and
markers consume the same angle. The original mode table is documented in
TASK-0131.

The Godot rear view follows the donor layout-2 gate and the attached
camera on view 3. PAL 10EC70 and 10EDC8 provide the 256 by 64 target,
mirrored sampling, frame and mask textures, and the destination rectangle
on the 512-wide source canvas. The donor `src/ecstasy/ecstasye.cpp` SHA-256
is `f3480f34b044ed251efd492644b091a0ca95b506318838bf083c477fe5d4232d`.
The Godot host shares the live 3D world with a SubViewport and applies
the source camera's per-view vehicle visibility. The centered HUD canvas
owns the mirror quad and composes the original textures. Matching PAL
hardware capture and the cost of the additional render pass remain open.

The Godot host maps Esc to the original controller Start action during a
race. Frontend Esc keeps its exit action. The MVP's default map uses the
player car heading with the existing TrackMap projection. The split and
special pursuit on/off presets use the authored rotating local view when
enabled. Saved single-player fixed modes and the Off setting retain their
behavior. These are port-side controls and display policy.
Godot 4.7.2 Debug and Release staged with `cmake --build --preset
godot-mvp-debug --target stage_mvp --parallel 8` and the corresponding
Release preset. The Debug `tools/test_mvp_frontend_resize.gd` probe with
`--race-track=12` opened Pause Menu on Esc and reached the restart
confirmation. See `local/task0126-escape-direct/smoke.log`. The donor
TrackMap source and heading projection evidence are recorded in TASK-0131.

- E-0126-F01. The Windows donor `indep/src/flailerdb.cpp` SHA-256 is
  `b21a0d21dd328d8b2774e88641c09cc5451f5df4f2957a2253b611afcca56ac5`.
  Its PAL `1422F0`, `1428F0` and `145950` comments identify text layout,
  bitmap layout and texture-batch traversal. The donor's exact PAL ELF SHA-256
  is unresolved. `vulkanColor` in donor `src/ecstasy/ecstasye.cpp`
  (SHA-256 `f3480f34b044ed251efd492644b091a0ca95b506318838bf083c477fe5d4232d`)
  consumes low-to-high RGBA with 128 as the modulation unit. The old Godot
  code reversed red and blue. The source menu selection tint is (128,64,0).
- E-0126-A01. The tested US retail ISO SHA-256 is
  `9aaa58f238dd2e17814945c0802f327ba36c94a8391d5d7ea43a03a89066cbcb`.
  The local US retail `SLUS_203.62` SHA-256 is
  `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
  The separately extracted `FRONTEND/FRONTB.BUN` SHA-256 is
  `869d11291994090a8a1730dcab399248b3e97ab4f2088df647017b755d5d714c`.
  Its two `EVENT_NUMBER` records (0x16B7D629) in the leaf template specify
  centered text (format 0x04), with Y anchors 199 and 179. The port now
  retains the authored panel-relative Y anchor and omits the leading
  blank on single digits so the visible digits center at the leaf X.
- E-0126-H01. Donor `feanim.cpp` SHA-256
  `228c3e5c337589ab8a840dae8160f7c78d8cc3893c0a4c050f6a38d84b665783`
  supplies the echo's selected label, authored tint, sine-out scale and
  alpha curves, 0.5 second duration and 4 second restart. The port publishes
  the static echo layout and timing operands. Godot owns the echo node
  and Tween, and retires them when the request disappears.

## Validation and continuation

Windows Release built with CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0,
Godot 4.7.2 and godot-cpp `e4b7c25e721ce3435a029087e3917a30aa73f06b`.
Forward+ smoke passed for Quick Race (`local/task0126-isolated.log`),
Championship (`local/task0126-final-campaign.log`) and quit-to-menu
(`local/task0126-final-return.log`). Direct Track 12 published generation
1 in `local/task0126-direct.log`. These runs precede the last echo and
number alignment edits. The user requested no further runtime tests.
The final echo and alignment changes compiled and staged successfully
with `cmake --build --preset godot-mvp-release --target stage_mvp --parallel 12`
(`local/task0126-animation-build.log`).
The extracted resource directory failed common sound bank startup, so
the successful runs used the full ISO. Original console parity is unverified.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
$env:HP2_PS2_ASSET_ROOT = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
$env:HP2_PROFILE_DIR = "$PWD/local/task0126-followup-profile"
$env:HP2_MVP_SCREENSHOT_DIR = "$PWD/local/task0126-followup-shots"
$env:HP2_MVP_CHAMPIONSHIP = '1'
New-Item -ItemType Directory -Force $env:HP2_MVP_SCREENSHOT_DIR | Out-Null
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-release/mvp-project --resolution 960x720 --script "$PWD/tools/test_frontend_mvp.gd"
```

Next action for TASK-0126 is to port the menu group slide, arrow pulse
and background transitions from the donor's animation owners into
Godot while preserving their authored inputs. Check the echo on selection
and the event-number anchors when runtime testing resumes.

## Mouse input and rendering backend

Godot consumes Flailer geometry and source identities for pointer hit testing.
The source UI tick validates the queued screen generation and runs existing
selection, menu, popup, campaign, name entry and replay callbacks. The Godot
pointer Control uses the default arrow and draws no buttons. Mouse clicks use
authored hit regions, with the existing title-screen shortcut. The live race
HUD is informational. The open ImGui console captures pointer and keyboard
events before the frontend controls. The smoke scripts read the actual console
state and use the grave key to dismiss it before sending game input.
`build.ps1 --rendering-driver d3d12` selects the SDR D3D12 Forward+ path. Vulkan
remains the default. No Windows display settings were changed.

- Built and staged Debug and Release using `cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16` and `cmake --build --preset godot-mvp-release --target stage_mvp --parallel 16` with Godot 4.7.2.
- Vulkan Godot mouse probes reached the live races from Quick Race and the first Hot Pursuit event. The Track 13 probe visited the event tree, selected the Elise and an automatic transmission, then advanced both pre-race screens. Logs are `local/task0126-mouse-quick/smoke.log` and `local/task0126-mouse-pursuit/click.log`.
- The Release keyboard frontend smoke passed when run by itself at 960x720 with `--script "$PWD/tools/test_frontend_mvp.gd"`. See `local/task0126-mouse-keyboard/serial.log`.
- The direct Track 12 keyboard resize probe passed on Vulkan and D3D12 Forward+ on Intel Arc B390. It checked 800x600, 1120x630, 1260x540 and 540x720 with the HUD and rear view, Pause Menu and restart confirmation. The Debug `HP2_MVP_MOUSE_PAUSE=1` run passed after checking that Resume clicks are captured by the open console, opening the Restart Race modal, clicking outside, cancelling and resuming. Logs and captures are under `local/task0126-mouse-keyboard`, `local/task0126-mouse-d3d12` and `local/task0126-mouse-pause-final`.
- D3D12 Forward+ reported feature level 12_0 and played the menu videos. The Windows Auto HDR indicator and a complete D3D12 Track 13 race were not verified. Actual HDR activation is a Windows decision.

```powershell
$env:HP2_PS2_ASSET_ROOT = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
$env:HP2_PROFILE_DIR = "$PWD/local/task0126-mouse-followup/profile"
$env:HP2_MVP_SCREENSHOT_DIR = "$PWD/local/task0126-mouse-followup"
$env:HP2_MVP_MOUSE_PAUSE = '1'
New-Item -ItemType Directory -Force $env:HP2_MVP_SCREENSHOT_DIR | Out-Null
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-debug/mvp-project --rendering-driver vulkan --resolution 800x600 --script "$PWD/tools/test_mvp_frontend_resize.gd" -- --race-track=12
```

Continue by exercising settings and post-race pages with mouse events and
checking the Game Bar Auto HDR indicator after `./build.ps1 --run --rendering-driver d3d12`.

## Godot InputMap

Project input actions supply the existing PS2 scanner. Esc and Backspace
share the Start action in races. Outside a race, Backspace sends menu
cancel and Esc exits. Debug Console capture precedes gameplay input.

Debug and Release staged with
`cmake --build --preset godot-mvp-<configuration> --target stage_mvp --parallel 16`.
With Godot 4.7.2 Vulkan Forward+, the Track 12 command above passed in both
configurations with `HP2_MVP_PAUSE_BINDINGS=1` and `HP2_MVP_MOUSE_PAUSE=1`.
The probe checked Esc, Backspace, held and overlapping keys, temporary
rebinding to P, cancellation of the restart confirmation, and resume.
Debug also checked Console capture of Backspace. Logs are
`local/task0126-inputmap-debug.log` and `local/task0126-inputmap-release.log`.
The Release frontend smoke passed from title to Quick Race, pause and resume.
The Debug frontend smoke also passed with `HP2_MVP_PAUSE_BINDINGS=1`,
which checks that Backspace returns from World Racing mode selection to
the main menu before reentering Quick Race. Both ran
`tools/test_frontend_mvp.gd` on Vulkan at 960x720 with isolated profiles.
Logs are `local/task0126-inputmap-frontend.log` and
`local/task0126-inputmap-frontend-debug.log`.

The follow-up removes all supplemental Godot buttons and checks the pointer
Control for an empty child list and the default arrow. Godot 4.7.2 Debug
Vulkan passed the Track 12 mouse pause, restart dialog, cancel and resume
smoke with the command above. Debug and Release staged with
`cmake --build --preset godot-mvp-<configuration> --target stage_mvp --parallel 16`.
The Release runtime probe exited with Windows access violation
`0xc0000005` before producing Godot logs. The Release script parse check
passed with `--headless --script tools/test_mvp_frontend_resize.gd --check-only`.
See `local/task0126-authored-pointer-debug.log` for the Debug run.

## Frontend animation cadence

The Godot state-only frontend used the World scheduler even when no World
was loaded, so FRED generated at most 60 new layouts per second. The live
frontend now updates and republishes on each Godot render callback, using
the elapsed time in the existing 1/3600-second native units with carried
rounding error. Long stalls are capped to the native timestep range.
Loading, World simulation and Joylog retain their existing scheduling.
Select Car's 40-second orbit timeout now counts 60 Hz elapsed-time frames,
so the increased callback count does not shorten it.

Debug staging passed with
`cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16`.
Godot 4.7.2 with `--max-fps 120 --disable-vsync --resolution 960x720`,
`HP2_MVP_FRONTEND_RATE=1` and `HP2_MVP_CHAMPIONSHIP=1` ran
`tools/test_frontend_mvp.gd` against the staged Debug project using an
isolated profile. Event Tree measured 107.8 FRED updates/s and progressed
through event details and Select Car.
The same smoke entered Track 12, paused, resumed, and passed at World step 84.

## Windows replay mouse controls

Godot subdivides the authored `REPLAYBOTTOM_NEW` bitmap in
`GLOBAL/INGAMEB.BUN` into hit regions for the original replay controls. It
routes clicks through the existing input dispatcher to `ReplayScreen::JoyHandler`
events 99 through 107. The one-player cover still masks player selection.
Hidden controls retain the authored toggle icon as a restore target.

The pointer follows the authored `TIME_BAR` and `TIME_MARKER`. While held,
the latest mouse position seeks the recorded scene. A backward move restores
the nearest retained snapshot through coordinator action 14. Forward moves
reuse the current playback state when it is closer to the target than the
retained snapshot. Release restores the prior playback mode, with playback
paused at the endpoint. The port-side input and catch-up are required because
the source events expose discrete marker jumps. Snapshot restoration, input
replay, timing, and scene epochs come from the existing core owners.

- Windows Debug and Release passed
  `cmake --build --preset godot-mvp-<configuration> --target stage_mvp --parallel 8`.
  The Debug smoke ran from `local/task0126-replay-debug-project` while
  the existing game held the staged DLL open. Staging passed after it closed.
- Set `HP2_MVP_TEST_PROFILE=1`, `HP2_MVP_CHECK_REPLAY=1`, and
  `HP2_MVP_MOUSE_REPLAY=1`. Run Godot 4.7.2 with
  `--resolution 800x600 --script tools/test_mvp_race_lifecycle.gd -- --race-track=12`.
  Release Vulkan and Debug D3D12 reached multiple replay positions before
  mouse-up and completed `HP2_MVP_REPLAY_MOUSE_PASS`. The smoke covers playback,
  camera, marker buttons, help, hiding and restoring the controls, focus loss,
  resizing to 1120×630, grabbing the playhead at the endpoint, keyboard pause,
  synthetic controller Start, and restoring the live race on exit. Debug also
  covers console capture. See `local/task0126-replay-live-final-release.log`
  and `local/task0126-replay-live-final-debug.log`.
- The forced Quick Race Results fixture passed page navigation and omitted
  the Replay row after the post-race snapshot restore. Results replay entry
  and two-player view selection remain unverified. See
  `local/task0126-replay-live-final-results.log`. The controller check used
  synthetic input. Original-game equivalence remains unverified.

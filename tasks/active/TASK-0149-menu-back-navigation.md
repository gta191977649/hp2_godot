# TASK-0149 — Pause and results menu Back navigation

## Escape shortcut

Removed the port's global Escape quit action. Escape still routes through
the race pause binding and commits an active name edit. The main-menu Exit
item still ends the session. On a Debug assertion, closing the window
ends the failed process with status 1.

The navigation smoke presses Escape at the title and main menu, then
continues through the frontend. Its Track 12 invocation opens and resumes
the pause menu with Escape. The test uses the Release staged project and
`tools/test_mvp_menu_navigation.gd`, with `-- --race-track=12` for the pause
run. Debug and Release are built with
`cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8`
and its Release equivalent. Logs under `local/esc-navigation` include a
headless Debug assertion check with Escape followed by close notification.
Both staging builds passed. The Release Vulkan frontend and Track 12
pause runs reported `MENU_NAVIGATION_PASS`. The Debug assertion check
survived Escape and exited with status 1 after the close notification.

## Back navigation

Backspace on the results menus was routed as the race coordinator's Start
input. The race scene remains alive during results, so the Godot host also
filtered the menu Triangle input in that state. The same filter affected
pause confirmations on the root Pause Menu.

The Godot host now sends the existing menu Back input for results, pre-race
information, pause submenus and pause confirmations. The original menu manager decides
whether to pop its history, consume Back within the current screen or cancel
a dialog. The host suppresses a Start edge until the key is released so that
holding Back across a submenu return leaves the race paused. Root pause and
Instant Replay continue to use Start. No game core changes were needed.

The original menu history callback is recorded in [FRONTEND_BACK.md](../../docs/FRONTEND_BACK.md)
at `0016C670` in the US PS2 `SLUS_203.62` ELF, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
The port result screen registrations and history are in
`src/game_core/indep/src/frontendscreens/endgamescreens.cpp`. Both Lap Times
registrations use the same callback. Validation below exercises the Godot port.

## Validation

- Before the fix, Track 13's End Game Menu stayed on screen after Backspace
  when the regression expected Best Total Times (`before.log`).
- Both Vulkan and D3D12 passed `test_mvp_results_navigation.gd` on Track 13.
  The test went forward and backward between Race Results, Best Total Times
  and End Game Menu. It reopened Lap Times 1P for Backspace, Escape and
  controller B, then cancelled the exit confirmation with Backspace.
  Track 33 also passed the same sequence on Vulkan.
- Vulkan passed `test_mvp_menu_navigation.gd` in a standard race and a police
  event. The test checked Pause Options, audio, display, restart and quit
  confirmations, the police objective and controls, native Debug Menu,
  Instant Replay and race resume.
- The Vulkan frontend pass covered both mode selectors, Challenge and Be
  The Cop setup, car and track selection, the event tree and its details,
  Options, audio and Jukebox, controller settings, display, HUD layout for
  both players, display calibration, idle Load/Save, NameEntry, and the EA,
  BBG, intro, making-of and port Credits entries. It also returned from Pre
  Race Stats to the Championship event briefing, then continued into the race.
- The separate name editing smoke passed. A dedicated credits text smoke
  failed its scroll checkpoint for the full email. The navigation
  test checks the Port Credits Back transition separately.
- The Release D3D12 race lifecycle smoke passed Track 13 drive, pause,
  resume, confirmed restart and confirmed quit with scene retirement.
- The Release Vulkan Challenge lifecycle passed car and track confirmation,
  pre-race continuation, restart and return to the main menu on Track 33.
- The existing native frontend selection in `build/native-debug` passed
  17 tests and skipped four asset-gated tests. The command was
  `ctest --test-dir build/native-debug --output-on-failure -R 'frontend|fmenus|endgame'`.
- `cmake --build --preset godot-mvp-debug --target hp2_track12 --parallel 8`
  and `cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8`
  passed. The standard Debug staging copy encountered a DLL held open by
  another session. The Debug tests used the freshly built DLL in an isolated
  staged project under `local/menu-back/project`.

Run with Godot 4.7.2, a retail ISO in `HP2_PS2_ASSET_ROOT`, and
`HP2_MVP_TEST_PROFILE=1` for a fresh diagnostic profile per run. The Debug results test also needs
`HP2_MVP_TEST_RESULTS_MENU=1`. Its fixture calls the existing finish entry
point once after 120 race steps to exercise the original results builders
without a full race. Remove the flag for other tests.

```powershell
godot --path local/menu-back/project --rendering-driver vulkan --script "$PWD/tools/test_mvp_results_navigation.gd" -- --race-track=13
godot --path local/menu-back/project --rendering-driver d3d12 --script "$PWD/tools/test_mvp_results_navigation.gd" -- --race-track=13
godot --path local/menu-back/project --rendering-driver vulkan --script "$PWD/tools/test_mvp_results_navigation.gd" -- --race-track=33
godot --path local/menu-back/project --rendering-driver vulkan --script "$PWD/tools/test_mvp_menu_navigation.gd"
godot --path local/menu-back/project --rendering-driver vulkan --script "$PWD/tools/test_mvp_menu_navigation.gd" -- --race-track=13
godot --path local/menu-back/project --rendering-driver vulkan --script "$PWD/tools/test_mvp_menu_navigation.gd" -- --race-event=ultimate-racer:5 --debug
godot --path build/godot-mvp-release/mvp-project --rendering-driver d3d12 --script "$PWD/tools/test_mvp_race_lifecycle.gd" -- --race-track=13
```

Logs are under `local/menu-back`. The US 60 Hz frontend hides Language
Options. Split-screen results, Best Lap Times, device-specific controller
calibration, reward and unlock pages, and save/load operations in flight
still need separate runtime fixtures.

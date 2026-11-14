# TASK-0145 - Debug menu and attract demo

- Status: active
- Scope: HP2 frontend Debug submenu, existing Track Viewer entry, Car Viewer placeholder, and original attract demo in Godot.
- Acceptance: Debug and Release builds, frontend smoke, menu input and popup, manual and idle demo, completion and interruption, and isolated profile checks.

The Windows donor implements the PAL `FrontendDemoMode` launch policy and
`RaceCoordinator` demo states. Its exact source ELF hash is unresolved as
documented in `docs/REVERSE_NOTES.md`. Debug frontend entries are port code.

## Implementation

- The optional `ApplicationRuntime` frontend extension registers the Debug
  page and supplies one row before Exit. The MVP adapter owns the page and
  defers tool actions until the active screen and pointer epoch are checked.
- The Debug page borrows the original Options art (screen `0x95AD096B`), small
  font, 37-pixel row spacing, video and highlight. It restores the borrowed
  text, visibility and rail positions on exit. Car Viewer uses the authored
  one-button popup (`0x74D2DFA2`).
- The manual action enters `FrontendDemoMode` after the original idle
  comparison. The existing source selector, demo modes, World, camera and
  release logic stay authoritative. The PAL AI demo coordinator range is
  `0x14A8C0..0x14AA38`, with joystick event 22 dispatched by `0x14A7F8`.
  The host supplies keyboard and gamepad buttons through the source scanner
  and disables the rear-view display during demo states.

## PC validation

Godot 4.7.2, Vulkan Forward+, 960x720, isolated profiles under `local`.
Both `cmake --build --preset godot-mvp-{debug,release} --target stage_mvp
--parallel 16` builds and both runs of `tools/test_frontend_mvp.gd` passed.
`./build.ps1 --run --mvp --debug --jobs 16` opened the Title sequence.

`tools/test_mvp_debug_menu.gd` covered the Debug position, Options artwork,
Car Viewer modal using Godot gamepad events, Track Viewer and its return
button, mouse launch of the demo, Enter interruption, scene cleanup and
profile hash stability. Debug and Release both entered the source Track 11
AI demo, publishing 14 vehicles. Release exposes four main-menu entries.
After interrupting the demo, Debug entered a regular Track 41 race.
The unattended Release run reached the demo. The natural return to Title
was not observed before testing stopped at the user's request.

Reproduce with the pinned Godot console executable, `--path
build/godot-mvp-<config>/mvp-project --resolution 960x720 --script
"$PWD/tools/test_mvp_debug_menu.gd"` and an isolated `HP2_PROFILE_DIR`.
Set `HP2_MVP_DEMO_TEST=release` for the Release menu or
`HP2_MVP_DEMO_TEST=idle` for the unattended title and demo.
PS2 equivalence, optional recorded playback assets and audio capture have
not been independently verified.

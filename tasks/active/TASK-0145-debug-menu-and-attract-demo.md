# TASK-0145 - Debug menu and attract demo

- Status: active
- Scope: HP2 frontend Debug submenu, existing Track Viewer entry, Car Viewer placeholder, and original attract demo in Godot.
- Acceptance: Debug and Release builds, frontend smoke, menu input and popup, manual and idle demo, completion and interruption, and isolated profile checks.

The Windows donor implements the PAL `FrontendDemoMode` launch policy and
`RaceCoordinator` demo states. Its exact source ELF hash is unresolved as
documented in `docs/REVERSE_NOTES.md`. Debug frontend entries are port code.

## Implementation

**Debug → Demo Scene** opens a separate submenu. **Select Track** uses
the authored left and right arrows and supports the matching keyboard
or controller input. **Start Scene** launches the displayed track.
Automatic retains the original sequence. The menu offers the authored
tracks eligible for the live AI demo and keeps the choice for the
frontend session. The frame-time graph stays off the submenu so it
does not cover the selector.

The port-side frontend extension registers a second screen through the
existing runtime owner so the submenu participates in the normal menu
stack. The port-side `FrontendDemoMode::RequestDemoMode` accepts a requested
track and validates it before handing it to the existing
`FrontendPlayerManager::StartDemoMode`. This boundary is needed because
the original request selected the track internally and also owns
recording lifetime checks. The original idle path, replay ownership
and demo race setup remain intact. The new explicit choice uses the
live AI path. The PAL donor identity remains as noted above.

Windows staging used `cmake --build --preset godot-mvp-debug --target
stage_mvp --parallel 8` and the corresponding Release preset.
`tools/test_mvp_debug_menu.gd` with `HP2_MVP_DEMO_TEST=demo-track`
opens the submenu, checks Back, selects Track 12 using the direction
controls, starts the AI demo and returns to a normal race. The named
profile hash is checked before and after. See
`local/debug-demo-submenu/run.log`.

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

## Debug pointer and menu SFX follow-up

The startup ImGui console captured mouse events before Godot's frontend
Control received them. With the console dismissed, a mouse click reached
Debug through `QueueFrontendPointer` and the shared FRED `JoyHandler`.
The console now opens on explicit input, with the frame graph still visible.
The Debug smoke test clicks the row in frontend design coordinates.

Menu SFX route through `FlailerSoundState::Play` to the original bank voice
in category 3. The default 0.31 category gain multiplies the sample descriptor
gain before the native pan matrix reaches XAudio2. For MenuSelect this is
about 0.115 (-18.8 dB) before panning at default settings. The US Retail ELF
evidence for that gain and the authored sample amplitudes is in
[TASK-0016](../../docs/SOUND_RECOVERY.md). Code inspection supports the
authored gain, so no volume boost was applied. Physical output level and
an original PS2 loudness comparison remain unmeasured.

`cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16`
passed. Godot 4.7.2 ran `tools/test_mvp_debug_menu.gd` on the staged Debug
project with `HP2_MVP_DEBUG_MODE=1` and an isolated profile. It passed mouse
entry, the Car Viewer popup, Track Viewer return, manual demo and regular
race reentry. The existing native build passed `frontend_actions`,
`frontend_jukebox` and `frontend_audio` via CTest.

## Mouse and popup lifetime regression

The earlier default-closed console change still allowed the opened console
to consume every mouse event. Its mouse capture now follows its window bounds.
Track Viewer and effect controls retain their own mouse handling. Frontend
clicks continue to enter through the existing Godot Control and FRED input.

The source ticks both popup slots even after dismissal. An inactive slot
kept its Continue label and kept publishing an external highlight despite
the original screen being hidden. The integration guard in
`FlailerPopup::Tick` preserves native timing and permits external publication
only for active slots. Godot also filters any highlight whose source screen
is absent from the visible layout. Its existing retirement path kills the
Tween, detaches the node, and queues it for deletion.

Debug staging passed. With Godot 4.7.2 at 960x720 and an isolated profile,
`HP2_MVP_DEBUG_MODE=1` and `HP2_MVP_DEMO_TEST=pointer`, the Debug menu smoke
clicked Debug, Car Viewer and Continue through Godot input. It verified that
the Continue node existed with the popup open and was absent for five frames
after dismissal, with Car Viewer still selected. This used injected Godot
mouse events. The full smoke reached Track Viewer and did not complete its
return-button step in this run.

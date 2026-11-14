# Development

Read [AGENTS.md](../AGENTS.md) for current source and evidence rules and
[TASK-0123](../tasks/active/TASK-0123-native-source-migration.md) for the
migration acceptance checks.

```powershell
cmake --preset godot-mvp-release
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
./build.ps1 --run
py -3 reverse/scripts/generate_progress.py --check
```

The active build stages the Godot project and its race GDExtension.
To build and open the Debug MVP in the pinned Godot Editor, run
`./build.ps1 --mvp --debug --editor --jobs 16`. For an existing build,
open `build/godot-mvp-debug/mvp-project/project.godot` in the Project Manager.
The staged `override.cfg` resolves the local asset configuration and Debug
window title for editor launches. Edit project sources under `godot_mvp`.
Press F6 on `native_game.tscn` or F5 for the project, then start
**Debugger → Profiler** and **Debugger → Visual Profiler** in the editor.
Press the grave/tilde key in the game to give keyboard input back to the frontend.

Use `./build.ps1 --run` for Release or `./build.ps1 --debug` for Debug. Both
start in windowed mode with race FXAA and VSync. Frame pacing follows the
display refresh rate with no software FPS cap. Add `--fullscreen` to launch
in native fullscreen.

Use `--race-track=<id>` for direct single-player diagnostics. The default launch enters
the frontend and needs the shared FFmpeg 8 SDK described in the README.
The original game data is read locally. The former Debug and Retail
reconstruction tasks and symbol records are historical references. Use their
addresses only with their recorded ELF identities.

See TASK-0124 for the current Godot entry checks.
See [TASK-0141](../tasks/active/TASK-0141-godot-single-player-races.md)
for the race catalogue and lifecycle smoke commands.
Use `./tools/test_mvp_campaign_matrix.ps1` in PowerShell 7 to start and
drive every campaign entry using its authored event ID. Build the staged
project and `mvp_race_catalog` first.

The Godot launcher reads `mvp_root` from `config/ps2_assets.local.json`
when present and otherwise uses `root`. `HP2_PS2_ASSET_ROOT` overrides
both. This lets extraction tools retain their configured loose asset
tree while Godot uses a complete disc or archive. The Godot host
reports missing Common sound bank files by name for loose sources.

Debug links the repository's pinned Dear ImGui and ImGui Console
sources through the MVP target. Open the console with the grave/tilde key
and enter `ver` to see the running Godot Engine version. The overlay uses
the DPI reported by Windows for the active game window. A fatal MVP assertion stops the
session and opens the Debug console. Esc exits the failed session with
status 1. Release reports the failure and exits immediately.
MinGW Debug builds the MVP native targets with `-O2` and debug symbols.
The game core keeps its strict floating-point options.

For the first Ultimate Racer roundtrip, run the Godot frontend smoke script
against the staged project with `HP2_MVP_PURSUIT=1`. Set `HP2_MVP_RETURN=1`
to check the pause and quit path. To drive through to the results, set
`HP2_ROUTE_CAPTURE` to a local Track 13 road course capture (see TASK-0129).
The route driver lives in `tools`; it reads telemetry and sends ordinary
key events. Neither the game nor its build loads that script.
For a Debug Track 13 run with the original AI driving the player car, set
`HP2_MVP_PURSUIT_AI=1` before launching. The takeover runs once at race start.
When the frontend smoke script also has `HP2_ROUTE_CAPTURE`, it reports
progress without sending driving input. Clear the variable for manual play.
Per-section opaque scenery instancing is enabled by default. Set
`HP2_MVP_SCENERY_BATCHING=0` to reproduce the individual placement path
and `HP2_MVP_AUDIO_TRACE=1` to enable synchronous audio voice diagnostics.
See TASK-0137 for the Track 13 Visual Profiler comparison.

For the arrest cinematic smoke, set `HP2_MVP_TEST_ARREST_NIS=1` and
`HP2_MVP_PURSUIT=1`, then run `tools/test_frontend_mvp.gd` against the
staged Debug project with an isolated profile and screenshot directory.
The test invokes the donor's arrest playback and captures two poses and
cleanup. Normal play uses the donor pursuit trigger.

Run `tools/test_mvp_frontend_resize.gd` against the staged project for
live window checks. Use `HP2_MVP_PURSUIT=1` for the event menu path, or pass
`-- --race-track=12` for the HUD and pause confirmation. The script
captures each ratio in `HP2_MVP_SCREENSHOT_DIR`.

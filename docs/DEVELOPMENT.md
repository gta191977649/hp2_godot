# Development

Read [AGENT.md](../AGENT.md) for current source and evidence rules and
[TASK-0123](../tasks/active/TASK-0123-native-source-migration.md) for the
migration acceptance checks.

```powershell
cmake --preset godot-mvp-release
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
./build.ps1 --run
py -3 reverse/scripts/generate_progress.py --check
```

The active build stages the Godot project and its race GDExtension.
Use `--race-track=12` for direct diagnostics. The default launch enters
the frontend and needs the shared FFmpeg 8 SDK described in the README.
The original game data is read locally. The former Debug and Retail
reconstruction tasks and symbol records are historical references. Use their
addresses only with their recorded ELF identities.

See TASK-0124 for the current Godot entry checks.

The Godot launcher reads `mvp_root` from `config/ps2_assets.local.json`
when present and otherwise uses `root`. `HP2_PS2_ASSET_ROOT` overrides
both. This lets extraction tools retain their configured loose asset
tree while Godot uses a complete disc or archive. The Godot host
reports missing Common sound bank files by name for loose sources.

Debug links the repository's pinned Dear ImGui and ImGui Console
sources through the MVP target. The overlay uses the DPI reported by
Windows for the active game window. A fatal MVP assertion stops the
session and opens the Debug console. Esc exits the failed session with
status 1. Release reports the failure and exits immediately.

For the first Ultimate Racer roundtrip, run the Godot frontend smoke script
against the staged project with `HP2_MVP_PURSUIT=1`. Set `HP2_MVP_RETURN=1`
to check the pause and quit path. To drive through to the results, set
`HP2_ROUTE_CAPTURE` to a local Track 13 road course capture (see TASK-0129).
The route driver lives in `tools`; it reads telemetry and sends ordinary
key events. Neither the game nor its build loads that script.

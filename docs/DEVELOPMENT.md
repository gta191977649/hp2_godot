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

The active build stages the Godot project and the Track 12 GDExtension.
Use `--race-track=12` for direct diagnostics. The default launch enters
the frontend and needs the FFmpeg executable described in the README.
The original game data is read locally. The former Debug and Retail
reconstruction tasks and symbol records are historical references. Use their
addresses only with their recorded ELF identities.

See TASK-0124 for the current Godot entry checks.

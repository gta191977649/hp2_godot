# PS2 PC Port

Project initialization for a native C++17 decompilation port, hosted by Godot.

Build from the project root in PowerShell with `./build.ps1`. Use
`./build.ps1 --run` to build and start the frontend, or
`./build.ps1 --clean --run` for a clean rebuild and launch. The default preset is
`godot-debug`; `--preset native-debug` builds only the independent core.
See `./build.ps1 --help` for presets and parallel-build options.
Two police helper local bodies now have evidence-backed native implementations and PC tests; original equivalence remains unverified. No proprietary assets are included. See [whole-binary recovery](docs/FULL_RECOVERY.md) for scope and gaps. Static binary
assessment results are in [the assessment report](docs/assessment/BINARY_REPORT.md).
The existing `task.txt` and `eval.txt` are preserved user request documents;
`eval.txt` supplied the assessment scope; its results are recorded under
`docs/assessment/` and do not constitute a reconstructed game.

The native core builds and tests independently of Godot:

```powershell
cmake --preset native-debug
cmake --build --preset native-debug
ctest --preset native-debug
```

The Godot 4.4.1 project starts the recovered front-end movie/title sequence.
Menus, localisation, background videos, menu sounds and frontend music read the
user-configured original ZZDATA at runtime. Run `./tools/run_frontend.ps1 -Build`;
see [media](docs/FRONTEND_MEDIA.md) and [sound](docs/SOUND_RECOVERY.md) for evidence
and remaining gaps. The separate `main.tscn` arithmetic scaffold remains infrastructure.
See [development](docs/DEVELOPMENT.md) for dependency checkout, Windows builds,
and the Godot smoke command. See [progress](docs/PROGRESS.md) for actual checks.

Start future sessions with [AGENTS.md](AGENTS.md) and the selected task.
[Architecture](docs/ARCHITECTURE.md) describes layer ownership;
[reverse notes](docs/REVERSE_NOTES.md) defines provenance and evidence records;
[roadmap](docs/ROADMAP.md) defines milestone gates.

`src/game_core/` holds recovered logic by subsystem; `src/platform/` holds native
service contracts and `godot_bridge/`; `reverse/` holds binary-specific knowledge;
`godot/` holds the host project; `tools/` holds future asset utilities;
`third_party/` holds pinned dependencies; `tasks/` holds durable work items.
Private inputs belong outside Git, preferably under ignored `private/`.

The initial target is Windows x86-64. Linux library mapping is provided but has
not been validated; other platforms need explicit build and loader support.

The evidence-backed main-loop structural shell and separate Godot observer are documented in [main-loop structure](docs/MAIN_LOOP_STRUCTURE.md). Run `./tools/run_main_loop_observer.ps1 -Build`. Module internals are placeholders; this is not a runnable original game.

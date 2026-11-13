# HP2 Godot MVP integration

**Keep original game logic and state in `src/game_core`. Let Godot manage
presentation components and their lifetimes.** Follow the
[MVP architecture diagram](docs/ARCHITECTURE.md) when integrating new features.

## Ownership and data flow

- `src/game_core` owns original game logic and state. Keep menu decisions,
  race rules, physics, AI, saves, audio scheduling, and effect triggers in
  C/C++ with no Godot dependencies. Preserve the source behavior, clocks,
  and resource identifiers.
- `src/mvp/bridge` carries committed state and ordered events from
  `ApplicationRuntime`. Reuse its interfaces. Include the scene generation,
  logical step, and stable source IDs where needed to identify updates and
  retire objects. Read borrowed resources synchronously within their source
  lifetime. Godot owns uploaded resources and retires stale nodes and events
  on restart, replay seek, scene change, or return to the menu.
- `src/mvp/systems` and `godot_mvp` own presentation and host input. Use
  Godot controls for pointer hit testing and send actions to the original
  handlers. Godot manages scenes, visual entities, presentation streaming,
  UI drawing and layout, Tweens, audio playback and mixing, particles, and
  render resources. Face billboards in shaders and
  sample Godot's scene depth for sun occlusion. Preserve the authored UI and
  video aspect ratios. Fit backgrounds and curtains to cover the viewport.
- Use Godot's built-in Visual Profiler to investigate rendering performance.

## Game Core Change Rules

- Preserve the donor and reverse-engineered implementations under
  `src/game_core` by default. Change them only to correct original-game
  behavior supported by evidence or to provide a required integration
  capability that existing interfaces cannot supply. For each core change,
  record why it is needed, its evidence or integration dependency, and how
  it was checked in the task record.
- Reuse existing interfaces first. Keep new boundaries minimal and
  Godot-independent in C/C++. Expose only the committed state, original
  input entry point, or generic lifecycle callback that the host needs.
  Mark port-side interfaces explicitly. Keep `main.cpp` focused on
  runtime orchestration.
- Put diagnostic event selection, test unlocks, forced triggers, debug
  UI, and diagnostic state collection in `src/mvp/assets`. The launcher
  parses command-line arguments and forwards the request to the MVP
  adapter. Keep C++ tests and AI driving helpers in `src/mvp/tests` and
  smoke and batch validation scripts in `tools`.
- Require explicit activation for debug behavior. Diagnostics that change
  progression or race state must use an isolated test profile by default.
  Preserve original identifiers and provenance when correcting recovered
  behavior. Unimplemented original paths must throw `hp2::NotImplemented`.

## Original-game evidence

Use recovered names verbatim and `Sub_<ADDR>` when only an address is known.
Clearly identify port-side interfaces. Record the source address, ELF version,
and SHA-256 for recovered behavior. Leave unresolved behavior explicit and
throw `hp2::NotImplemented` for unimplemented original paths. Mark original
equivalence as verified only after comparison with the original game. Keep
proprietary assets, ELF and ISO files, analysis databases, and secrets out of Git.

## Workflow

Check `git status --short` and read the relevant task and architecture before
editing. Keep changes scoped to the task and preserve existing work. Review
the diff and run the relevant builds or smoke checks. Record the commands,
results, and remaining gaps in the task record.

See [Decisions](docs/DECISIONS.md),
[Development](docs/DEVELOPMENT.md), [Reverse notes](docs/REVERSE_NOTES.md),
and [Progress](docs/PROGRESS.md). Record session notes in the
[progress log](docs/PROGRESS_LOG.md). Regenerate progress tables with
`py -3 reverse/scripts/generate_progress.py` when tracker inputs change.

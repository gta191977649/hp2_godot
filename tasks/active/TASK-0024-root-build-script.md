# TASK-0024 - Root build and run script

- Status: active
- Objective: provide a project-root build script supporting `--clean` and `--run`.
- Scope: port-side build infrastructure only; preserve user files and existing presets.
- Original PS2 addresses/provenance: none; this is explicitly port-side tooling.
- Dependencies: existing CMake presets, local pinned Godot4.4.1 for run mode.
- Current phase: TASK-0022 checkpoint is durable; this is the current implementation task.

## Acceptance criteria

- [ ] Root build.ps1 accepts GNU-style `--clean` and `--run`.
- [ ] Default build is godot-debug; `--run` builds successfully before launching the frontend.
- [ ] Clean uses the generator's clean target and confines the build directory to the preset.
- [ ] Help, unknown options, invalid presets/jobs and native run are handled explicitly.
- [ ] Exact validation commands and prerequisites are documented.

## Validation plan

Syntax/help/options checks, native-debug clean rebuild, default Godot build, and
build/run smoke using the pinned engine. Avoid an unnecessary clean rebuild of all
godot-cpp bindings; the same CMake clean operation is validated with native-debug.

## Findings and continuation

The existing tools/run_frontend.ps1 is a launcher; its `-Build` only supports
godot-debug and uses PowerShell-style switches. The new root entry point should
reuse the configured presets and existing frontend scene. No repository-wide cleanup.

## Related commits

None.

# AGENT.md — source, naming and evidence

The Windows source distribution at `D:/dev/hp2_ps2_rev` is the sole game
implementation reference for the Godot MVP (TASK-0123). The imported source
lives under `src/game_core` in its original directory layout. The previous
Debug/US Retail implementations and legacy RenderBridge are retired. Do not
restore a second gameplay implementation or port behavior from the retired
runtime into the donor without checking the donor and the original evidence.

## Ownership

- `hp2::ApplicationRuntime` owns startup, input scanning, World stepping,
  resources, frontend flow, audio, replay, profiles and shutdown. Keep this
  code free of Godot.
- The Entity State Bridge validates and publishes value snapshots from the
  committed World. The bridge carries source identity, lifetime and visual
  inputs. It has no game rules, GPU objects or authority to change simulation.
- Godot is the sole owner of scene instances, UI presentation, meshes, textures, cameras,
  materials, shaders, draw submission, render targets, the rendering pipeline
  and post-processing. Build these in `src/mvp/presentation`, `src/mvp/systems`,
  `src/mvp/assets` or `godot_mvp`. The Godot scene tree owns visual nodes,
  and Godot resource references own meshes, textures and materials.
  Keep host bindings in `src/mvp/godot`.
- The frontend presents its menus, movies and transitions with Godot
  components. Keep menu actions, event eligibility and progression with
  the source owners. Movie requests carry the resource path, request ID and
  playback completion. FFmpeg decodes MV30 into an encoded stream for
  Godot playback.
  Simulation must never depend on Godot node state or use Godot physics.
- Keep source camera decisions, asset decoding, authored visibility, effect
  state and decoded assets with their donor owners. Copy borrowed assets
  synchronously while the relevant World and resource owners are live.
  The Godot entry advances the donor simulation without creating, converting
  or submitting Ecstasy draw commands. Do not build a donor graphics device.

## Godot launch eligibility

Enable a race in the Godot project after its scene consumes committed World
state and the playable Godot path passes validation. The project opens the Godot frontend by default. Start simulation only
after the original frontend requests an eligible race or the user selects
the diagnostic race explicitly. Use committed World state and Godot-owned scenes for presentation. Report
unavailable integration at the point of entry with the stage, source identity
and missing capability, then stop the requested path in Debug and Release.

## Names and evidence

- Use `hp2` as the only project-owned named C++ namespace. Anonymous scopes,
  namespace aliases for external libraries and the donor's existing C ABI
  remain valid. Godot and third-party code retain their own namespaces.
- Use the donor's source identifiers. For new recovered work, preserve an
  original identifier verbatim, or use `Sub_<ADDR>`, `Type_<ADDR>` or
  `Field_0x<off>` when the original name is unknown. Document host adapters
  as port-side and keep them outside the recovered game code where practical.
- The source distribution records PAL addresses and has no Git metadata or
  documented original ELF hash. Preserve those comments and source hashes.
  Do not treat them as addresses in the previously authorized Debug or Retail
  ELF. Existing records for those ELF hashes are historical evidence.
- Confirm claims about the original against an identified ELF or original game
  data. Separate observations, hypotheses and open questions. Original
  verification needs a scoped comparison against the matching original.
- Preserve the donor's explicit failures. New untranslated recovered paths
  must throw `hp2::NotImplemented` with the source address. Catch exceptions at
  the GDExtension boundary and report them to Godot.
- Ghidra access is read-only. Keep proprietary binaries, assets, databases
  and save files outside tracked source. Resolve assets from
  `HP2_PS2_ASSET_ROOT` or the local asset configuration.
- Write repository documentation and comments in English. Track new work in
  the active task and progress log. Preserve existing historical evidence IDs.

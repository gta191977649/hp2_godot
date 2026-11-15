# TASK-0150 Portable MVP launch, profiles, and Windows export

## Changes and evidence

- Godot resolves the launch root from the exported executable or the staged
  project. `launcher.cfg` stores the selected ISO, and `profiles` stores
  native saves. The asset-selection UI is a Godot control. The port-side
  GDExtension validator uses the existing ISO and ZDIR readers.
- `GameFileSystem::DiscBootExecutable` provides the structural disc identity
  check required by the launcher. It reads the root `SYSTEM.CNF` `BOOT2`
  assignment. The GDExtension then requires the root `SLUS_203.62` ELF and
  readable game archive. This is a PC integration capability.
- `MemcardScreens::LoadProfile` consults the old folder only when the new
  primary and backup are absent. It validates before publishing and uses
  the existing no-replace write. This is a PC integration capability.
- `NameEntry` uses the original commit path and requests the existing
  profile autosave after the editor closes. The port needs this timing
  because the same screen remains active after confirmation. Original US
  PS2 `SLUS_203.62` evidence covers `0024D870`, `0024D088` and `0024D370`.
  ELF SHA-256 is `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
  [Frontend back behavior](../../docs/FRONTEND_BACK.md)
- The retired native program's argument parser and presentation clock
  had no MVP callers. They were removed with the stale generated Windows
  frontend packages. The reverse-engineering evidence remains available.
- Windows Release exports use a Godot 4.7.2 template built for size, with
  full LTO and an explicit module list. Debug exports use the official
  template. The Windows Release extension uses the matching Release
  `godot-cpp` allocator ABI. Stock export tests initially detected a heap
  error when the Debug extension allocator ran against the Release engine.

## Validation

- `cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8`
  built the staged Debug project.
- `tools/test_mvp_portable_launch.gd` ran in a disposable staged project
  against the correct US PS2 ISO, a different PS2 ISO, and a malformed
  file, then repeated startup with saved configuration and `--select-iso`. The test
  feeds Godot's Window drop signal and verifies the rejected disc leaves
  the configuration and the picker intact.
- `HP2_MVP_NAME_TEST=rename` and `HP2_MVP_NAME_TEST=reload` exercised the
  frontend with a migrated copy of an existing profile. The test compared
  records, event progression, rewards, and points across the rename,
  verified the save while NameEntry was still active, and verified it after
  restart. The original profile hash stayed the same.
- An official 4.7.2 Release export produced a 109,268,480-byte engine
  executable. The final trimmed EXE is 48,765,440 bytes, a 55.37% reduction.
  The complete release directory totals 174,244,059 bytes, including the
  GDExtension, PCK, FFmpeg and notices.
- `build.ps1 --preset godot-mvp-debug --jobs 6` exported the Debug package.
  Its standalone executable booted from a different working directory,
  created the portable profile, reached Title Screen, and exited with code 0.
- `build.ps1 --mvp --jobs 8` exported the final Release package. SCons
  reused the compiled template on the packaging run. Standalone launches
  from a different working directory displayed the picker, decoded the
  intro, and published Track 33 in both Vulkan and D3D12 Forward+.
  Each process exited with code 0.

Local validation logs are in `local/portable-launch`. The real Windows
drag gesture and file chooser UI are outside the scripted signal test.

## ISO picker retry after launch failure

The picker defers its drop and file-selected handlers until the Windows
callback returns. The Godot bridge reports initial runtime failures back
to the picker and frees the failed presentation before the next attempt.
The launcher writes the validated ISO path after startup succeeds.

The staging test rejected an unrelated PS2 disc, malformed image, and a
synthetic image with matching SYSTEM.CNF, ELF magic and ZDIR structure but
missing startup data. It then started the retail ISO, checked persistence
and reopened the picker. Isolated Debug and trimmed Release exports
repeated drop, rejection and successful retry through the Windows
templates with Godot's drop signal. Both build commands exported the final
packages. Logs are in `local/rd-lifetime`.

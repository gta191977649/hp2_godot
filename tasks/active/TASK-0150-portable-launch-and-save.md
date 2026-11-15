# TASK-0150 Portable MVP launch, profiles, and Windows export

## Profile selection and creation

Normal Windows launches enumerate `profiles` beside the executable.
Staged projects use the project directory as the launch root. The list
includes the legacy `profile.json` and separate `profile-<id>.json` files.
Startup loads a single valid entry or opens the shared profile picker.
With no entries, it opens the first-run creation flow. An unreadable
profile appears as unavailable and keeps its file. The picker excludes
`*.backup.json`, which remains available for manual recovery.

Options > Save/Load opens the same picker and offers direct creation.
Manual and automatic saves use the active filename and its own backup.
The switch prompt offers Save and Continue, Discard, and Cancel when the
live serialized profile differs from its last successful load or write.
New profiles start from the runtime's initial defaults. Each creation
publishes a unique file, then activates it. Cancelling the switch prompt
or the name editor keeps the live profile and its current changes.

The shared picker reuses the Display screen's rows, selection rails and
scroll arrows. The unsaved-changes prompt uses the authored three-choice
car-selection dialog. Profile enumeration, identity, detached name
validation and transition policy are port-side integrations in
`game_core`. Godot continues to present the Flailer commands and send
input through the existing bridge. The new picker and switch policies are
PC behavior.

## Original startup presentation

Ghidra MCP confirmed the authored startup order in US PS2 `SLUS_203.62`
`0016F950`, the `BootAutoload` constructor and callbacks at
`002478C0`, `002479D8`, and `00247AB0`, and registration of resource
`FA2AC5C6` at `00247DE8`. The status popup `0CE3F7CE` and status
strings `24DE5` and `24DD9` come from `00245E30`, `00247320`, and
`00247378`. The check and load timing use the source clock as observed in
`00246C00`. ELF SHA-256 is
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.

Required first-run naming is a PC integration. It keeps the original
startup stack and borrows the original `NameEntry` editor so both
controller selection and Godot keyboard input reach the same draft.
`0024DBD8` and `0024D088` establish the editor and its commit path.
The startup editor supports cancellation back to the creation prompt.
The name is written through the existing atomic no-replace file owner.
Existing, corrupt, and competing files retain their data. The explicit
`HP2_MVP_TEST_PROFILE=1` retains disposable profiles for older smoke scripts.

Run `tools/test_mvp_boot_profile.gd` against a disposable staged project
with a distinct Godot user directory. Set
`HP2_MVP_BOOT_PROFILE_PROJECT` to its absolute project path,
`HP2_MVP_BOOT_PROFILE` to the scenario, and
`HP2_MVP_TEST_MENU_ALIGNMENT=1` for authored pointer bounds.
Use `create`, `load`, `corrupt`, `retry`, `collision`, `mouse`, `options`, or `scroll`.
The `collision` scenario also takes a valid JSON file through
`HP2_MVP_BOOT_PROFILE_SEED`. For `options`, seed `profile.json` and
`profile-second.json` with valid profiles named Arc and Zed and Autosave
enabled. For `scroll`, seed eight valid profiles. Set
`HP2_MVP_SCREENSHOT_DIR` to capture the HP2 screens.

The multi-profile implementation passed Debug and Release builds using
`cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8`
and the equivalent Release preset. Release Vulkan passed creation,
legacy load, unreadable-file protection, blocked-directory retry,
concurrent-file protection, Options switching and creation, and the
eight-profile scroll check. The Options check verified all three
unsaved-change outcomes and compared the active file's backup hash.
Captured frames were checked for row alignment and dialog text fit.
`./build.ps1 --mvp --jobs 8` exported Windows Release. Disposable copies
launched from a separate working directory and verified startup with
zero, one, and multiple profiles. `tools/test_mvp_portable_launch.gd`
also passed with `--select-iso` and verified the generated profile file.
Logs and captures are in
`local/multi-profile-031639`.

Earlier first-run validation used Debug and Release `stage_mvp`. Release Vulkan passed
creation, restart, corrupt input, blocked-directory retry, and competing
save checks. It prompted for a name with valid JSON only in the old
user-data location and a backup and binary save beside the absent
primary. Debug D3D12 passed the on-screen keyboard confirmation check.
The Windows Release export built with `./build.ps1 --mvp --jobs 10`.
Two disposable copies of that export launched from a different working
directory. The copy with the primary profile reached the title and kept
the file hash. The empty copy stayed on the creation flow. Neither wrote
a profile to the working directory or the old user-data location.

## Earlier portable launch work

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

## Blackbox extension name

The native Windows extension, Godot descriptor, and entry symbol use the
`hp2_blackbox` name. Packaging removes the old DLL from an existing output
folder after export succeeds. The first Release cleanup encountered a
temporary access denial. Removing the old generated file and rerunning
the build completed the migration.

`build.ps1 --mvp --jobs 8` and
`build.ps1 --mvp --preset godot-mvp-debug --jobs 8` exported both packages.
Each package contained `hp2_blackbox.dll` and reached Title Screen from a
temporary directory with the retail ISO. Logs are in `local/blackbox-rename`.

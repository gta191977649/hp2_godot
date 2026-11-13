# TASK-0141 - Godot single-player race integration

- Status: active
- Scope: all authored single-player race entries and their Godot lifecycle.
- Dependencies: TASK-0123, TASK-0124, TASK-0125, TASK-0129 and TASK-0140.

## Acceptance

- [x] Track 33 enters through the original menu and the diagnostic entry.
- [ ] All authored campaign events and single-player modes pass their race lifecycle.
- [ ] Every selectable track variant and allowed direction has recorded coverage.
- [x] Debug and Release build and the Track 33 Forward+ Vulkan and D3D12 checks pass.

## Provenance

The Windows donor owns the active race flow. Its PAL comments are source
annotations with unresolved ELF revision and SHA-256. The former adapter
reported PAL185528 when it rejected tracks outside 12 and 13. The donor
comment at PAL185528..185558 documents the race-aborted result cell.
That address does not identify an original track admission rule.

The reverse scenery filter comes from the donor annotation at PAL10F928.
The donor applies reverse placement transforms during scenery loading. The
presentation adapter uses the committed direction to choose the matching
single-player placement mask. The replay restore epoch is port-side state.

## Validation

Windows x64, MinGW GCC 13.2.0, CMake 3.29.2, Ninja 1.12.0,
Godot 4.7.2 (ed1daf0bf), godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b.
Forward+ used an Intel Arc B390 on Vulkan 1.4.359 and D3D12 12_0.

| Path | Observed result |
| --- | --- |
| Track 33 World Racing Quick Race on Vulkan | A dedicated test profile set the donor's `track_selection=33`. The original menus started the race. Driving, pause and resume, restart to generation 2, and return to the main menu passed. |
| Track 33 diagnostic on D3D12 | Driving, pause and resume, restart to generation 2, and scene retirement on quit passed. Both Track 33 runs published 9,255 placements and 14 vehicles. |
| Track 33 instant replay on Debug Vulkan | The original Pause Menu restored a World snapshot and the bridge published replay epoch 1. |
| Track 33 career command | `--race-event=ultimate-racer:2` selected the authored Track 33 event on a fresh profile and the player responded to normal driving input. |
| All 64 campaign entries | The Release command-line driving matrix passed 64/64 after the diagnostic code moved into the MVP adapter. Earlier probes timed out for Ultimate Racer 17 on Track 46. The harness now tries normal car recovery if the vehicle stalls and isolates the process environment for each run. |
| All 30 authored scenario IDs | Headless Release diagnostic runs published a scene and exited 0. Covers the forward Quick Race load only. |
| Championship Track 12 and Ultimate Racer Track 13 | First event in each tree reached the live race through the authored menus. Driving, restart and return to the event tree passed. |
| Ultimate Racer Track 13 result | A fresh Debug profile ran the complete route using the donor AI test driver. The original reward, Race Results and End Game Menu appeared, followed by the event tree. The profile wrote a nonzero event time where the backup had zero. |
| Challenge modes 3 through 7 on Track 11 | Menu selection of Single Race, Knockout, Lap Knockout, Free Run, and Be the Cop passed driving, pause, restart and quit. |
| Reverse Track 11 | A fresh profile saw the authored reward lock. An isolated Debug profile using the donor test override selected reverse through the original picker and passed driving, restart and return. |
| Invalid input | Catalogue hole 54, malformed and out-of-range IDs, and a missing source path returned exit 1 with the requested ID and stage. |

The catalogue contains 30 scenario IDs, 64 events across two trees, 133
authored event legs, and 260 custom-mode, track-variant and direction
combinations before progression locks. Generate it from the original
resources with the following commands.

```powershell
# Set HP2_PS2_ASSET_ROOT to a complete local PS2 disc or archive.
cmake --build --preset godot-mvp-release --target stage_mvp mvp_race_catalog --parallel 8
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8
& ./build/godot-mvp-release/mvp_race_catalog.exe $env:HP2_PS2_ASSET_ROOT > local/task0141-catalog.csv
$engine = "$PWD/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
$project = "$PWD/build/godot-mvp-release/mvp-project"
$smoke = "$PWD/tools/test_mvp_race_lifecycle.gd"
$env:HP2_PROFILE_DIR = "$PWD/local/task0141-diagnostic-profile"
& $engine --path $project --rendering-driver d3d12 --resolution 960x720 --script $smoke -- --race-track=33
$env:HP2_PROFILE_DIR = "$PWD/local/task0141-menu-profile"
New-Item -ItemType Directory -Force $env:HP2_PROFILE_DIR | Out-Null
Set-Content "$env:HP2_PROFILE_DIR/config.ini" "format=1","track_selection=33"
& $engine --path $project --rendering-driver vulkan --resolution 960x720 --script $smoke
py -3 reverse/scripts/generate_progress.py
py -3 reverse/scripts/generate_progress.py --check
git -c core.safecrlf=false diff --check
```

Use `HP2_MVP_CHALLENGE_MODE=3..7`, `HP2_MVP_EXPECTED_TRACK`, and
`HP2_MVP_PICKER_DIRECTION=1` to exercise a Challenge menu selection.
`HP2_MVP_CHECK_REPLAY=1` uses the ordinary replay menu after driving.
Debug builds accept `HP2_MVP_TEST_UNLOCK_RACES=1` to exercise authored
entries with the donor's testing override and an isolated test profile.
Local evidence is under `local/task0141-*`. Windows held the staged Release
DLL during an overlapping smoke, so the final DLL was also tested from a
separate local project copy.

To test a locked event directly, use `./build.ps1 --race-event=ultimate-racer:2`
or `./build.ps1 --race-event=championship:1`. The command selects the first
authored allowed car and skin and skips the startup and pre-race menus.
It uses a fresh diagnostic profile unless `HP2_PROFILE_DIR` is specified.
Quick Race and campaign diagnostic selection, validation and test unlocks
live in `src/mvp/assets/race_diagnostics.cpp`. The core exposes a one-shot
host callback after the authored frontend banks become ready.
Run `./tools/test_mvp_campaign_matrix.ps1 -Jobs 5` in PowerShell 7 after
building `stage_mvp` and `mvp_race_catalog`. The script reads the catalogue,
launches each authored event in a separate headless Godot process, sends
normal acceleration input, and records movement, exit codes and logs. It
retries failed entries once serially and retains both attempts in the CSV.
The passing report is `local/campaign-matrix-d5d503e8-97ec-4bf8-bcaf-2c4279eaedf2/results.csv`.

## Be the Cop follow-up

Ultimate Racer events 5, 9, 13, 18 and 27 use authored event kind 6. The
donor also sets the same pursuit flag for custom Be the Cop. The flag selects
the police HUD and results through the existing Player and EndGame owners.
The Godot keyboard shortcuts R and T send the existing Triangle+R2 and
Triangle+L2 scanner chords. The modifier precedes the primary edge.

Run the dedicated smoke with a fresh profile and the local game source.

```powershell
& $engine --path $project --rendering-driver vulkan --resolution 800x600 --script "$PWD/tools/test_mvp_be_the_cop.gd" -- --race-event=ultimate-racer:5
```

The smoke checks the authored timer strip, the player's officer binding,
target acquisition, both support requests, and Be The Cop Results 1P.
Set `HP2_MVP_POLICE_HUD_ONLY=1` to check the police role and HUD at entry
without waiting for a result.

Release confirmed the officer binding and authored timer strip in all
five campaign events. Event 5 acquired a target using H, consumed one
roadblock and one helicopter request, and reached Be The Cop Results 1P
after the donor ended the run. Logs are `local/task0141-police-hud-*.log`
and `local/task0141-police-event5-both.log`. Custom Be the Cop passed the
officer binding and HUD checks through the normal menu, drove, restarted
and returned to the menu in `local/task0141-police-custom.log`.
Debug and Release staged successfully. The police results smoke covered the timeout
result. Arresting every opponent and the corresponding victory bonus
remain open.

## Remaining work

Complete each event to its donor-defined result. Check multi-leg
continuation, persisted progression, and the remaining track direction
and custom mode combinations. The fresh-profile Track 13 run recorded a
result in the save, with 1,307,560 ticks against a 1,200,000-tick objective.
The subsequent normal menu probe could not start the locked Track 33
event with that result. The diagnostic covered its launch and driving.
PC coverage has not established PS2 original equivalence.

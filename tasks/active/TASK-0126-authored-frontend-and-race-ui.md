# TASK-0126 - Authored frontend and race UI

- Status: active
- Depends on TASK-0123, TASK-0124 and TASK-0125.
- Scope: present the donor's Flailer fonts, bitmaps, event tree and in-race UI in Godot. Keep input, screen flow, HUD values and authored layout in game_core.

## Acceptance

- [x] Render original atlas glyphs and bitmaps through Godot in the title, main menu, mode selection, event tree, event details and car selection.
- [x] Enter Track 12 through Quick Race and Championship. Present the starting grid, race HUD and pause menu, then resume or return to the frontend.
- [x] Keep resources local and record donor and asset provenance.
- [ ] Finish migration of authored visual animations to Godot and validate the latest changes when requested.
- [ ] Cover race results, replay presentation, remaining screens and unverified locales. Rear-view target and GS blend/sampler parity remain open.

## Evidence

- E-0126-F01. The Windows donor `indep/src/flailerdb.cpp` SHA-256 is
  `b21a0d21dd328d8b2774e88641c09cc5451f5df4f2957a2253b611afcca56ac5`.
  Its PAL `1422F0`, `1428F0` and `145950` comments identify text layout,
  bitmap layout and texture-batch traversal. The donor's exact PAL ELF SHA-256
  is unresolved. `vulkanColor` in donor `src/ecstasy/ecstasye.cpp`
  (SHA-256 `f3480f34b044ed251efd492644b091a0ca95b506318838bf083c477fe5d4232d`)
  consumes low-to-high RGBA with 128 as the modulation unit. The old Godot
  code reversed red and blue. The source menu selection tint is (128,64,0).
- E-0126-A01. The tested US retail ISO SHA-256 is
  `9aaa58f238dd2e17814945c0802f327ba36c94a8391d5d7ea43a03a89066cbcb`.
  The local US retail `SLUS_203.62` SHA-256 is
  `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
  The separately extracted `FRONTEND/FRONTB.BUN` SHA-256 is
  `869d11291994090a8a1730dcab399248b3e97ab4f2088df647017b755d5d714c`.
  Its two `EVENT_NUMBER` records (0x16B7D629) in the leaf template specify
  centered text (format 0x04), with Y anchors 199 and 179. The port now
  retains the authored panel-relative Y anchor and omits the leading
  blank on single digits so the visible digits center at the leaf X.
- E-0126-H01. Donor `feanim.cpp` SHA-256
  `228c3e5c337589ab8a840dae8160f7c78d8cc3893c0a4c050f6a38d84b665783`
  supplies the echo's selected label, authored tint, sine-out scale and
  alpha curves, 0.5 second duration and 4 second restart. The port publishes
  the static echo layout and timing operands. Godot owns the echo node
  and Tween, and retires them when the request disappears.

## Validation and continuation

Windows Release built with CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0,
Godot 4.7.2 and godot-cpp `e4b7c25e721ce3435a029087e3917a30aa73f06b`.
Forward+ smoke passed for Quick Race (`local/task0126-isolated.log`),
Championship (`local/task0126-final-campaign.log`) and quit-to-menu
(`local/task0126-final-return.log`). Direct Track 12 published generation
1 in `local/task0126-direct.log`. These runs precede the last echo and
number alignment edits. The user requested no further runtime tests.
The final echo and alignment changes compiled and staged successfully
with `cmake --build --preset godot-mvp-release --target stage_mvp --parallel 12`
(`local/task0126-animation-build.log`).
The extracted resource directory failed common sound bank startup, so
the successful runs used the full ISO. Original console parity is unverified.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
$env:HP2_PS2_ASSET_ROOT = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
$env:HP2_PROFILE_DIR = "$PWD/local/task0126-followup-profile"
$env:HP2_MVP_SCREENSHOT_DIR = "$PWD/local/task0126-followup-shots"
$env:HP2_MVP_CHAMPIONSHIP = '1'
New-Item -ItemType Directory -Force $env:HP2_MVP_SCREENSHOT_DIR | Out-Null
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-release/mvp-project --resolution 960x720 --script "$PWD/tools/test_frontend_mvp.gd"
```

Next action for TASK-0126 is to port the menu group slide, arrow pulse
and background transitions from the donor's animation owners into
Godot while preserving their authored inputs. Check the echo on selection
and the event-number anchors when runtime testing resumes.

# TASK-0147 — Port credits and native race debug

- Scope: add The Porting of HP2 below The Making of HP2, and make the
  explicit Godot launch flag enable the donor's native race debug tools.
- Content: `src/mvp/assets/port_credit.hpp`; edit and rebuild. The address
  and URL are display text.

## Integration and evidence

- The US retail `FRONTB.BUN` (SHA-256
  `869d11291994090a8a1730dcab399248b3e97ab4f2088df647017b755d5d714c`)
  supplies Credits art `FD8A3B6D`, row four `52E039F6`, reward row
  `52E039F7`, and the EA scroll art `39C4C62C`. The US retail
  `FE_ENGLISH.BIN` (SHA-256
  `8a05ce92e8a7d7a19e24e816c7ae90c7c71cbdd374c27bbe8db44a7b13af62d3`)
  maps row four to The Making of HP2 (101112) and the reward row to 101111.
  The US retail ELF is `SLUS_203.62`, SHA-256
  `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
- The core supplies an optional port content hook because the original Credits
  callback owns the menu ring, authored reward gate and scrolling clock. The
  extra row and logical screen borrow the existing art; they release or
  restore borrowed objects on exit. The original atlas lacks `@` and `_`.
  The core publishes visible fallback text and clip coordinates synchronously;
  Godot owns and retires those labels.
- The launch adapter calls the existing `applicationConfigureTesting` and
  forwards normal logical controller samples. The existing native focus owner
  now publishes its existing caption as soon as it accepts the opening chord,
  so the host can display focus while neutral input is still required. The
  focus, camera operations and pause actions remain in the donor. This is an
  integration change to the donor's PC debug path; no recovered address or
  original-game equivalence claim is added.

## Validation

Use Godot 4.7.2 Forward+ and an isolated `HP2_PROFILE_DIR`. Stage with
`cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8`
and the corresponding Release preset. Run the staged project with
`--resolution 960x720 --script "$PWD/tools/test_mvp_port_credits.gd"`;
repeat in Debug with `HP2_MVP_TEST_UNLOCK_RACES=1` for the reward row.
Run `tools/test_mvp_native_debug.gd` with `-- --race-track=33 --debug`
and with `-- --race-track=33` for the disabled case.

Both builds staged successfully. The Credits smoke passed in Release with
the reward locked and in Debug with the existing diagnostic unlock. It
covered keyboard, injected joypad navigation and pause, mouse entry, return to
EA Credits, scroll restart and fallback label cleanup. Screenshots confirmed
both headings, the full email and project names, and the background change.

The native Debug smoke passed in Debug and Release on Track 33 with the
keyboard chord. It checked camera motion, gating until neutral, camera
restoration, resumed driving, standalone Start on release and entry to the
authored pause Debug Menu. Release also passed the disabled case. The stock
`test_frontend_mvp.gd` passed with both builds;
`test_mvp_debug_menu.gd` passed the pointer, popup, Track Viewer round trip
with native debug still enabled, manual demo and normal race reentry;
`test_mvp_race_lifecycle.gd` passed
the Release Track 33 restart and teardown run. Logs and screenshots are in
`local/port-native-checks`. Physical gamepad camera input and PS2 comparison
remain unverified.

## Credits colors and library notices

The port credit headings use the authored Impact face in light blue; body
text stays white. The small core change adds the committed Flailer color to
the existing fallback text view so Godot can match the body and heading
colors when a character is absent from the original atlas.

`stage_mvp` copies the third-party notice and license texts beside the
project. The four staging FFmpeg DLLs were checked through their respective
`*_license()` and `*_configuration()` exports: all reported LGPL 3.0 or later
with version 3 enabled and GPL and nonfree options off. The SDK archive hash,
matching source revision and build configuration are recorded in the notice.
The corresponding-source bundle for a public binary distribution of the
third-party SDK is a separate artifact and was not produced.

Release staging and the Godot 4.7.2 Credits smoke passed. Debug compiled;
its normal staging DLL was held open by a running game, so the new DLL
passed the same smoke in an isolated project with the reward row unlocked.
Screenshots in `local/port-native-checks/credits-licenses-*` verify the
colored headings, white body and scrolling library section.

Changed all three scrolling headings to yellow in `port_credit.hpp`.
Release `stage_mvp` and `test_mvp_port_credits.gd` passed on Godot 4.7.2;
`local/port-native-checks/credits-yellow/port-credits-projects.png`
shows the yellow headings and white body text.

The two project acknowledgments use 80% of the normal body size. The
port content interface carries a per-line scale so Flailer can apply it to
both the rendered text and its scroll spacing, including the Godot fallback
for the underscore. The original credits keep their authored scales.
Release `stage_mvp` and `test_mvp_port_credits.gd` passed; verified both
lines in `local/port-native-checks/credits-small/port-credits-projects.png`.

Matched both project lines to the original small font. The retail atlas
has a hyphen but no underscore; a port-only flag lets the Credits renderer
draw that same atlas glyph at the baseline for underscores and use its
advance when centering the text. This keeps the rest of the line in the
original font while preserving the underscore in the displayed name.

The port scroller's opening title and section headings now use the gold
from US retail EA Credits language IDs 501000 and 501002
(`$RGB(134,96,48)`). Only The Porting of HP2 uses `FONT_IMPACTBIG`;
section headings use the upright `FONT_IMPACTNORMAL`.
Release `stage_mvp` and the focused Godot capture passed. The screenshot
`local/port-native-checks/credits-ea-capture/port-credits-projects.png`
shows the title, headers, both project lines and the baseline underscore.
The broader Credits smoke did not complete in the final run.

## Pause Debug Back

Reproduced Backspace leaving the race stuck on `Debug Menu` in the Release
Godot host with `-- --race-track=33 --debug`. The host suppressed Triangle
on race screens and sent Backspace as Start; the authored submenu exits
through `LeaveScreen` on Triangle. The Godot host now routes Back to that
existing action on pause submenus and suppresses the held Start alias until
release so it does not resume the race on the parent menu. No core changes.

Release `stage_mvp` and `test_mvp_native_debug.gd` passed. Backspace, injected
controller B, Escape and right-click each returned from Debug to the Pause
Menu while the race stayed paused. A fresh Escape resumed the race.
The existing frontend resize smoke with `HP2_MVP_PAUSE_BINDINGS=1` also
passed on Track 33 without native debug, including pause remapping and
the restart confirmation.

# TASK-0094 - Display, HUD and calibration

Retail SLUS_203.62 US PS2, revision unknown, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only Ghidra MCP captures in ignored local/task0091/display-agent and
local/task0091/0025a540.listing.json. No database mutations. Not Verified.

## Confirmed: E-0094-D01

001EE6C0 snapshots profile chunks144[8],14C[8],10D7C[30],1144C[30];
001EE878 compares all four. Display001F33B8 copies eight settings bytes.
001F3A28 modifies mirror/units/map/leaderboard/big-air/Zone and routes items7/8/9
to P1 HUD/P2 HUD/calibration. In frontend state3 the map-style sequence is
0,1,2,3,4,6.001F3730 derives secondary fields3/4 as7/8 unless primary6,
when both become6. Unknown current values select first table entry.
Leaderboard toggles2 and0. Back copies settings before conditional Autosave.

## Confirmed: E-0094-D02

001F46D8 changes seven HUD values in a48-byte snapshot;001F4178 commits the
whole snapshot on destruction. P1/P2 profile offsets are10D7C/1144C.
Bits40,10,100 in word10 are independent toggles; five-position byte6 controls
bit4 (clear for mode4). Word2C is the final toggle.001F40E0 anchors arrows to
selected value center;00215EE8 blinks from persistent phase003319C8,600deg/s,
using003319C4/0032F1B4 constants and original bSin. Text/selection ghosts use
original authored widget hashes; no Godot menu theme substitutes.

## Confirmed: E-0094-D03

001F4B20 adjusts signed GS offset X by4 and Y by1, lower bounds-650/-50,
with no upper clamps. Lower-bound failed adjustments suppress sound; successful
adjustments sound1. Select restores entry cache and explicitly pops history;
Back retains changes. Offset persistence is profile144/148. Host rendering
translation is a separately documented port policy, not original GS equivalence.

## Confirmed: E-0094-D04

Retail0025A5E0 quantizes radians via signed CVT.W.S and low16 bits;
0025A540 evaluates a quadrant-reduced odd polynomial, not a sine lookup table.
Constants0032F1B4..0032F1DB raw words:
38C90FDB,4096CBE4,40C90FDB,3FC90FDB,40490FDB,3E2AAAAB,3C088889,
39500D01,3638EF1D,4622F983. Native frontend overload preserves float operation
order. Exceptional conversions explicitly throw at0025A5F0; FCR31 behavior and
PS2 arithmetic equivalence remain unverified. Debug driving overload is separate.

## Validation and open work

Native build targets frontend_display_tests/frontend_runtime_tests/
frontend_audio_tests/frontend_jukebox_tests passed. CTest frontend_display,
frontend_audio,frontend_jukebox,frontend_runtime:4/4,38.97s on2026-10-25.
Display tests cover original map cycles/unknown fallback, asymmetric calibration
limits/reset, HUD mask isolation, separate player commits, route reentry and host
frame offset request. bSin tests cover quantization/cardinal inputs. No PCSX2
comparison; hardware GS/FPU state is not reproduced or claimed.

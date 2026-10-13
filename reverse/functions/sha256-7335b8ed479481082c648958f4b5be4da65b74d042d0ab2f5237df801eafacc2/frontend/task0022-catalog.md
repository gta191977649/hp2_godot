# TASK-0022 name catalogs and timer formatting

Binary SLUS_203.62; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only GhidraMCP, 2026-10-13. Static evidence only; no Verified claim.

## Confirmed: track names (E-0022-C01)

0x00153608 binds chunk 0x00034201 payload as a table of 0xA4-byte rows and clears track ID 54.
0x00153420 accepts IDs 1..count, indexes (id-1)*0xA4, and rejects a zero first byte.
The player's GLOBAL/GLOBALB.BUN contains this chunk at offset 657920, size 10824 (66 rows).
Track localized IDs are row+0x48 and row+0x4C; callers choose the variant.
The port-side adapter parses these IDs at runtime and does not check in original text.
Raw evidence: ignored local/task0022/00153608-catalog.json, 00153420-catalog.json.

## Confirmed: car names (E-0022-C02)

0x00187F98 binds chunk 0x34600 with 16-byte alignment; it contains 64 records of stride0x560.
0x00188D08 initializes each record+0x558 from 0x00187E80(index).
That function scans 50 seven-u32 rows at 0x0030D578..0x0030DAEF, matching row+0 against index,
and uses the first row when no match exists. 0x00187EB8 localizes row+0x14.
The adapter includes the recovered ID pairs, not copied localized text. This default is recovered,
not an invented fallback. Raw: ignored local/task0022/0030D578-catalog.json,
00187EB8-catalog.json, 00187E80-catalog.json, 00188D08-catalog.json.

## Confirmed: timer format (E-0022-C03)

0x001A3B98 handles tick count 0 or INT_MAX as zero components. Otherwise it temporarily negates
negative counts, splits them through 0x001A3B28, and restores the sign. Ticks use 4000 units/second:
hours=(ticks/4000)/3600, minutes=((ticks/4000)%3600)/60, seconds=(ticks/4000)%60,
milliseconds=(ticks%4000)/4. It passes milliseconds/10 to 0x001A3838 (two fractional digits).

0x001A3838 selects language-specific templates at 0x002B5AF8..0x002B5B98. English templates:
hours: '--h:--:--'; minutes: '--:--.--'; seconds only: '   --.--'; zero seconds: '    0.--'.
Languages 1/3 use colon separators for minutes/seconds, and 2/4/5 use apostrophe/quote separators.
The hour path omits fractional digits. 0x001A37B8 fills fixed-width decimal digits from right to
left, with leading spaces unless the padding argument is nonzero. Flag1 prefixes '-' and flag2
prefixes '+'. With either sign flag and a leading hours/minutes component >9, the formatter shifts
the content right one byte before replacing the first character. Raw evidence under ignored
local/task0022/001A3B98-catalog.json, 001A3838-catalog.json, 001A3B28-catalog.json,
001A37B8-catalog.json. Timer integration remains with the detail implementation.

## Unknowns and continuation

No 3D preview implementation or original-runtime comparison was done here.
Continue TASK-0022 by wiring EventNameCatalog to event detail text, and test authentic asset rows.

## Port validation

Passed with g++13.2.0 (MinGW-W64 x86_64-ucrt-posix-seh), 2026-10-13:

```
C:/Strawberry/c/bin/g++.exe -std=c++17 -Isrc src/tests/event_name_catalog_tests.cpp src/game_core/port/assets/elab_chunks.cpp src/platform/ps2_asset_root.cpp -static-libgcc -static-libstdc++ -o local/task0022/event_name_catalog_tests.exe
./local/task0022/event_name_catalog_tests.exe
```

Output: `PASS: authentic track IDs, car variants, cleared track and fallback`.
The first compile exposed wrong throw-address argument types; fixed to literal address strings
before the passing command above. No original hardware/PCSX2 comparison was performed.


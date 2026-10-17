# TASK-0038 tire/control-force closure

Target: SLUS_203.62; executable revision unknown; SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.

This pass implements the full001A4930 and001A4EE0 ordinary finite dependency algorithms, contact preparation and native wheel matrix publication against the shared bounded `port::driving::Memory` arena. It does not claim all PS2 numerical corner cases, original road collision or full-game equivalence. Synthetic environment geometry is confined to ContactProvider; material coefficients, roughness, force laws, polynomial/trig tables and operation order remain original evidence.

| Evidence | ELF entry | Scope |
|---|---|---|
| E-0038-T01 | 0025A540 | Unsigned-angle polynomial |
| E-0038-T02 | 0025A5E0 | Float angle conversion |
| E-0038-T03 | 0025A748 | Piecewise angle table |
| E-0038-T04 | 0025A890 | Float-vector angle |
| E-0038-T05 | 001A4818 | Conditional finite double mix |
| E-0038-T06 | 001A4D28 | Tire table dispatch |
| E-0038-T07 | 001A4930 | Wheel solver initialization |
| E-0038-T08 | 001A4EE0 | Wheel solver finalization |
| E-0038-T09 | 0011B248 | Four-vector matrix multiplication |
| E-0038-T10 | 0011B6E0 | Render polynomial family |
| E-0038-T11 | 001C0D78 | Native wheel matrix publication |
| E-0038-T11 | 0011B7D8 | Planar rotation matrix helper |
| E-0038-T11 | 0011B950 | Second rotation matrix helper |
| E-0038-T11 | 0011BA18 | Rotation composition wrapper |
| E-0038-T12 | 001A4170 | Wheel/contact preparation |
| E-0038-T13 | 001A4580 | Contact wrapper |
| E-0038-T13 | 001A45A8 | Projected contact displacement |
| E-0038-T14 | 0025A300 | Fixed-point division |
| E-0038-T14 | 0025AA78 | Fixed-point angle interpolation |
| E-0038-T14 | 0025AB50 | Fixed-point heading |
| E-0038-T15 | 001A46B0 | Wheel derivative callback |
| E-0038-T15 | 001A63C0 | Controller derivative callback |
| E-0038-T16 | 001A57E0 | Original static tire table initialization |
| E-0038-T17 | 0025AFA8 | Mutable math-global initializer |

## Required original data and integration

Call Sub_001A57E0 after static ELF mapping to initialize the eight original tire table headers; mapping alone leaves00310F48 zero. Trig tables are already initialized in ELF storage.

Map the player's authorized original ELF data through the SHA-bound runtime loader; no table assets are checked in. Required ranges include00318C00 material rows (0x90 stride),0031FDC0 angle table,00320850/00320958 fixed-angle tables, tire table00310F48 and pointer family003110B0, globals through GP003358F0, math-global00320A60 and00153498 noise data002F94C8. The checked-in sources read logical addresses; they do not hardcode local proprietary file paths.

Add `src/game_core/Sub_001A4930.cpp` to game_core and `src/tests/driving_tire_tests.cpp` as the `driving_tire` test. Dependencies are the shared Memory implementation, existing Sub_00138F38/Sub_001A4E88/Sub_0018BCE0, new table/engine Sub_0018F808 and contact Sub_00152070. Root owns backend, CMake, symbols/progress/task integration.

## Counterevidence and limits

-001A4930 material multiplier and001A4EE0 cap have distinct original multiply groupings. They are not algebraically flattened.
-001A46B0 extra drag is rate*(field16C*global), correcting decompiler rate*field16C*global grouping. The lock zero store precedes the later slip correction, which may therefore produce a nonzero derivative even when lock is set.
-001A4170 decompiler register signature mislabels the first arguments: listing proves three FP arguments followed by wheel/car/unused/steering integer arguments. Wheel rendering has a distinct0011B6E0 polynomial, not the0025A540 polynomial family or host sine.
-001A4EE0 mode3 reciprocal sqrt has an explicit addressed exceptional-operand throw for zero/nonpositive or nonfinite squared length; no normalization substitute is fabricated. Whole exceptional VU reciprocal/saturation behavior remains unknown.
-001A4818 uses finite host double intermediates following observed software-library conversion/subtract/multiply/add/convert calls. Exact exceptional software-library classes and bit rounding are unverified. CVT.W.S is the shared explicit finite truncation projection and needs scoped original comparison.
-001A4170 original stack output W is uninitialized in the shown instruction range. Its+0x130 W is deliberately preserved in the port; XYZ calculations are recovered, but no arbitrary original W value is asserted. Original discarded temporary lanes are not promoted as established state.
-001A3F00 is confirmed empty;001A51B4's condition has no consumer branch. No force or animation behavior is invented for these instructions.

## Evidence capture

```powershell
py -3 reverse/scripts/inspect_driving_tire.py --elf D:/ps2_game/SLUS_203.62
```

Passed:60 read-only SHA-bound queries, reusing saved captures where identical. Original raw code/data stays under ignored `local/task0038/tire/`.001A46B0 and001A63C0 require raw-memory decoding because Ghidra has missing/inaccurate instruction/function metadata; Capstone5.0.7 decode is derived only from Ghidra-provided bytes and does not mutate the project. Local `.capstone.txt` output is not checked in.

## Validation

Standalone GCC13.2.0 command:

```powershell
& C:/Strawberry/c/bin/c++.exe -std=c++17 -O0 -ffp-contract=off -Wall -Wextra -Wno-misleading-indentation -Isrc src/game_core/Sub_001A4930.cpp src/game_core/Sub_00138F38.cpp src/game_core/Sub_001A4E88.cpp src/game_core/Sub_0018BCE0.cpp src/game_core/Sub_0018F808.cpp src/game_core/Sub_00152070.cpp src/game_core/port/driving/memory.cpp src/game_core/Sub_00137100.cpp src/tests/driving_tire_tests.cpp -static -static-libgcc -static-libstdc++ -o local/task0038/tire/driving_tire_tests.exe
& ./local/task0038/tire/driving_tire_tests.exe
```

Passed after correcting three test-fixture mistakes: the finite-double midpoint input, GP-relative fixture values, and delta-W expectation. These were fixture values, not algorithm changes. Output:`driving_tire_tests passed (instruction fixtures, no original equivalence)`.

Coverage: float/fixed angle axes and saturation, two polynomial families, alias-safe matrix composition, table branch and family interpolation, sliding/non-sliding forces, negative-request adjustment, final magnitude cap and all four basis-mode publications with an oblique normal, plus degenerate normalization rejection, synthetic plane contact through complete4170/4580, tangent sentinel, finite double mixture, derivative grouping and zero-then-correction ordering. No host test establishes original equivalence. Full backend simulation and original capture/compare are root-owned pending integration; no commit created.

Final strict source and test translation-unit compilation with `-Wall -Wextra -Werror` also passed.

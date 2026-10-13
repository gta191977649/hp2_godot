# Structure: sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/Profile_002FC050

- Binary ID / SHA-256 / version: sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2 / SLUS_203.62 / USA-NTSC inferred
- Original type name / recovered type name: unknown / Profile_002FC050 (address-anchored, AGENT.md R2)
- Size / alignment / packing / byte order: 0x114E0 = 70,880 bytes / unknown / unknown / little-endian
- Confidence: high for the offsets listed, none for anything else
- Status: Partially identified
- PC declaration and layout test location: src/game_core/memorycard/include/memorycard.h; src/tests/memorycard_tests.cpp
- Users: 0x00245738, 0x00245D30, 0x00245780, 0x00185568, 0x001F5D00, 0x001F5D28, 0x001F5D50, 0x001FAF60, 0x001FE988, 0x002010B8

| Offset | Width | Original/recovered field | Type hypothesis | Evidence | Confidence |
|---|---|---|---|---|---|
| +0x0000 | 4 | unknown / size word | u32, always 0x000114E0 | FUN_00245738 writes it before every store | high |
| +0x0094 | ? | unknown | ? | FUN_00245D30 passes 0x002FC0E4 to FUN_001EF120 | address only |
| +0x0144 | ? | unknown | ? | FUN_00245D30 passes 0x002FC194 to FUN_001F4B10 | address only |
| +0x0164 | 4 | unknown / current event-tree type | s32, 0 Championship, 1 Hot Pursuit | FUN_001F5D50 `sw a1,0x0(a0)`; FUN_001F5D28 selects on it | high |
| +0x0168 | 4 | unknown / start node, tree type 1 | s32 event number | FUN_001F5D00 `lw v0,0x4(a0)` | high |
| +0x016C | 4 | unknown / start node, tree type 0 | s32 event number | FUN_001F5D00 `lw v0,0x8(a0)` | high |
| +0x0170 | 4 | unknown / event-tree view state | s32, FUN_001FAF60's state word | FUN_001F96B8 `piVar16[0x1d] = DAT_002FC1C0` | high |
| +0x0174 | 4 | unknown | ? | FUN_00245D30 `DAT_002FC1C4 = DAT_0031C1C8` | address only |
| +0x0178 | 8 | unknown | two words | FUN_00245D30 feeds both to FUN_00154308 / FUN_00155000 | address only |
| +0x0188 | 0x100 | unknown / Championship event block | packed event slices | FUN_00245D30 `FUN_001FE988(uGpffffBEC4, 0x2FC1D8, 0x100, 4)` | high |
| +0x0288 | 0x100 | unknown / Hot Pursuit event block | packed event slices | FUN_00245D30 `FUN_001FE988(uGpffffBEC8, 0x2FC2D8, 0x100, 4)` | high |
| +0x0388 | 4 | unknown / shared-slice cache | runtime only, cleared on load | FUN_00245D30 `DAT_002FC3D8 = 0`; FUN_001FE988 reuses it for event 0x20 | high |
| +0x1072C | 4 | unknown / selected mode | s32, 8 Championship, 9 Hot Pursuit | FUN_00185568 reads it; it is DAT_0030C77C | high |
| +0x114DC | 4 | unknown / checksum | s32 sum of the preceding 0x4537 words | FUN_00245738 writes it, FUN_00245780 verifies it | high |
| everything else | 0x10E30 bytes in total | **unidentified** | unknown | none | none |

## Confirmed layout facts

The block is one contiguous global at ELF **0x002FC050**, and it is the save payload itself:
`FUN_00245D30` copies a loaded `settings.db` onto it verbatim with
`HP2_CopyBytesForward(0x2FC050, **(screen+0x10), 0x114E0)`, and `FUN_00245738` returns the same
address as the store source. There is no staging buffer, no serialiser and no field-by-field
pack step in either direction, so the on-card byte order **is** the in-memory layout.

Three independent arithmetic identities fix the base address, and each was derived from a
different function: `0x002FC050 + 0x1072C == 0x0030C77C`, the tree selector `FUN_00185568`
reads; `0x002FC050 + 0x188 == 0x002FC1D8` and `+ 0x288 == 0x002FC2D8`, the two event blocks;
and `0x002FC050 + 0x114DC == 0x0030D52C`, the checksum `FUN_00245738` writes.

The payload is self-describing and carries no separate header: its first word holds its own
byte length and its last word holds a checksum over everything before it.

## Hypotheses and unresolved questions

- **The overwhelming majority of the block is unidentified.** 0x10E30 of its 0x114E0 bytes have
  no reading instruction behind them in this session's search, and none is modelled. The port's
  type is an opaque byte array with named accessors precisely so that nothing implies a field
  that was not read out of code.
- `+0x0388` is written at load time and used by `FUN_001FE988` as a per-pass cache, so it is
  almost certainly scratch that happens to live inside the saved region rather than saved state.
  That it is nevertheless written to the card follows from the copy being verbatim, and is a
  consequence, not an observation.
- Whether `+0x0164`'s two slots are indexed by tree type or by something that merely correlates
  with it rests on one caller; see 0x001F5D00's record for the quirk in the writer.

## Verification

`src/tests/memorycard_tests.cpp` asserts the offsets against a synthetic 0x114E0 block, the
checksum's exact span (flipping the last covered word changes it, writing the checksum word does
not), signed wrap-around in the accumulator, and a store/load/store round trip. Those are
port-side tests: they establish that the port reproduces what was read out of the ELF, and
nothing about equivalence with the original (AGENT.md R1). No real save file was read, written
or examined; every fixture is synthetic. PCSX2 comparison: not performed (TASK-0023).

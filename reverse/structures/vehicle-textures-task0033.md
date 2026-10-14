# Vehicle texture archive observations — TASK-0033

Evidence E-0033-T02, original configured `CARS/TEXTURES.BIN`, inspected 2026-10-14.
Archive SHA-256: `e0a863495e126b9a2290bbfa636bfaf900b42f4aeb4aeecd7e81f75f1718c5a1`.
ELF identity: SLUS_203.62, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`, revision unknown.

## Observed bytes

The 15,761,976-byte archive contains 783 entries, starting at file offset 0x6C,
each 0xA4 bytes. Hashes at +0x20 are unique in this file. The data pool starts at
aligned file offset 0x1F680. Table stride/hash offset and pool alignment agree
with ELF 001AC3F0 and 001AC608 (E-0033-T01).

| Entry name | Entry offset | Hash | Image offset | CLUT offset |
|---|---|---|---|---|
| `4DR_SEDAN02_SKIN1` | 0x1B4 | 0x857C3BFA | 0x20700 | 0x22700 |
| `911TURBO_SKIN1` | 0x125C | 0x1568FC4B | 0x49300 | 0x5D300 |
| `BMWM5COP_BRAKELIGHT` | 0x3598 | 0x1F51C325 | 0x1B4180 | 0x1B45C0 |
| `DASHWINDOW` | 0xCADC | 0x310960D7 | 0x5BB800 | no CLUT |

753 entries declare bit depth 4 / PSM 20 / CLUT 64 bytes; 29 declare bit depth 8 /
PSM 19 / CLUT 1024 bytes. `DASHWINDOW` declares 32-bit / PSM 0 / CLUT size zero
and is unsupported by the existing indexed decoder. It fails explicitly if requested.

## Port implementation and validation

`port::vehicle::Textures` lazily opens the original archives through injected
`port::assets::Source`. It decodes only requested hashes with unchanged
`Hp2TpkArchive::decode(header, false)` and returns raw header metadata plus RGBA.
No export, disk cache, guessed hash, substitute image or broad bundle fallback.
The false argument preserves the established mesh-facing row convention, a port
display choice pending vehicle VU/UV recovery. It does not prove original texture orientation.

`vehicle_texture_tests` passed against the three configured families above and
synthetic cases for missing hash, malformed archive, invalid shifts, unsupported
CLUT, short image size and out-of-range placement. Empty requests avoid file I/O;
duplicate requests produce one decode; reset forces reopening.

Standalone validation (GCC from PATH, C++17): compile the test with textures.cpp,
catalog.cpp, frame.cpp, hp2_tpk_texture.cpp, elab_chunks.cpp and ps2_asset_root.cpp;
execute `local/task0033/vehicle_texture_tests.exe` from the repository root.

## Unknowns

The header's `texture_fx`, `is_any_semitransparency`, `alpha_bits` and `alpha_fix`
bytes are preserved but do not define a recovered host blending or culling policy.
Pixel swizzle/CLUT expansion reuse the pre-existing TASK-0012 implementation;
this task does not independently establish that entire algorithm from ELF.
No PS2/PCSX2 visual comparison has occurred; nothing is Verified.

## E-0033-T04 — original shared GLOBAL textures

Vehicle model binding exposed references absent from the car archive:
`CARBOTTOM` (0xBD1F0E0A) and the static `WINDOW` selection (0x834B8017).
Both are present in the player's original `GLOBAL/GLOBALB.BUN` texture table.
This archive's SHA-256 is
`6fad4bcf13ad8936b240596c5a7d782ccbc255f5d9fb202df785f2b6b76a2153`.
Its 116-entry TPK table starts at 0x3DEC; data pool base is 0x8880.
`CARBOTTOM`'s entry is 0x430C (name 0x4314, hash 0x432C), 256x64 P4/PSM20.

Original lookup 001ABF48 traverses a circular list of loaded packs, invoking
001AC3F0 (or 001AC468 according to its third argument) and moving a successful
pack to the front. After exhausting that list it also checks 001057D8; optional
default-hash behavior is visible but is outside this adapter. There is therefore
no fixed recovered car-before-global priority. The decompile is retained under
ignored `local/task0033/001ABF48-sharedlookup.json`.

The bounded viewer adapter searches precisely the car and shared GLOBAL texture
tables and requires each requested hash to identify exactly one entry across
them. The two original tables have zero overlapping hashes, so priority is not
needed for these resources. A collision fails explicitly rather than asserting
an unrecovered pack-residency order. Each returned texture exposes its original
archive path. Neither default-hash substitution nor unrelated bundle search is
implemented.

Native CTest checks `vehicle_model` and `vehicle_textures` passed after adding
shared lookup (2.74s and 0.98s respectively). The model check loads all three
acceptance vehicle families with their original resources. Synthetic tests cover
shared-only requests and duplicate-hash ambiguity; missing hashes still fail.

## E-0033-T05 — raw texture byte +0x74

Fresh read-only Ghidra listing of original 0010DD58 confirms instruction
0010DEBC is `lb v0,0x74(a1)`, a signed one-byte load from the resolved texture
record. At 0010DE98 a temporary flag is initialized to 4; 0010DEDC clears that
flag when the byte is zero. The flag is subsequently combined with the strip
render flags. Its original field name remains unknown.

`TpkTextureHeader::Field_0x74` preserves exactly the original byte from the
0xA4-byte disk record. It is not expanded to a guessed integer, replaced by a
neighboring alpha field, or interpreted during pixel decode. Signedness is
irrelevant to the recovered zero/nonzero condition; preserving uint8 keeps
the exact disk value. The texture adapter returns it unchanged with the header.

The native texture fixture sets +0x74 to 0x81 and checks exact preservation;
the second shared-pack fixture checks zero. Original listing and decompile
captures are ignored under `local/task0033/0010DD58-field74-listing.txt` and
`0010DD58-texture74.json`. The full renderer, blend policy and field name are
not claimed by this metadata addition.

## TASK-0034 correction: image origin

E-0034-U01 retires the provisional false-flip vehicle display convention above. The legacy
decoder already reverses rows internally; vehicle loading now requests the optional reversal
so that source row zero corresponds to the original T=0 edge. CPU UV accessors are unchanged.
The default frontend texture convention remains unchanged. See vehicle/0011c268.md.

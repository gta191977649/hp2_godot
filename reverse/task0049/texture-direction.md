# E-0049-T01: TrackViewer source-row direction

Status: implemented and synthetic native tests passed; not original-runtime Verified.
Retail target: SLUS_203.62, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.

## Observations

- A fresh read-only Ghidra query of Retail0011C268 confirms eStripData::GetST0
  reads the S/T words returned by0011C090 and copies them directly. It does not
  subtract T from1 or otherwise invert the value. Raw response stays ignored at
  local/task0049/retail-getst0.json. E-0034-U01 additionally records the prior
  instruction/VU audit and source-row correction for the car adapter.
- The current shared indexed decoder explicitly iterates source rows from
  height-1 down to0 before producing RGBA, regardless of its optional public
  flip_vertical argument. Its subsequent flip_vertical=true operation restores
  source row-zero order. This is observable host decoder behavior, not an
  inferred PS2 asset transformation.
- decode_linear32 copies consecutive RGBA bytes directly and retains source
  row order. Applying an additional row flip to it would introduce a new bug.
- Track load_scene previously used indexed decode(false,true) for the base
  image and each original mip, while the existing corrected car adapter uses
  decode(true,true). Thus indexed track pixels were reversed before upload
  despite the original VIF ST values remaining unchanged.

## Correction and boundaries

The new port::track::decode_texture_level orientation adapter requests
decode(true,true) for indexed entries and decode_linear32 for direct32. Both
base and original per-level mip call sites use this single adapter. Existing
mip offsets, dimensions, per-level swizzle metadata, raw alpha, UV coordinates,
material/terrain identifiers and shared decoder/car behavior are unchanged.

The historical shared header comment suggesting flip_vertical=false for3D is
not an original-game authority and is superseded for the track adapter by the
observed decoder behavior and explicit source-row tests. No texture name,
filename, material label, or apparent screenshot colour determines direction.

## Validation

Added src/tests/track_texture_tests.cpp with asymmetric4x4 source rows and an
asymmetric original2x2 mip for PSMT4, PSMT8 and supported unswizzled PSMCT32.
Every pixel/channel is compared against explicit source-row order, including
raw alpha128,255,0,64. PSMT8 uses the physical CLUT slot ordering expected by the
existing decoder. Indexed legacy decode(false,true) is independently checked
to retain its existing reversed-row convention; the shared decoder was not edited.

Private standalone command, avoiding shared build outputs:

`g++ -std=c++17 -Isrc src/tests/track_texture_tests.cpp src/game_core/port/assets/hp2_tpk_texture.cpp src/game_core/port/assets/elab_chunks.cpp -o local/task0049/track_texture_tests.exe`

`local/task0049/track_texture_tests.exe` passed:4-bit,8-bit,linear32,base and mip.
This fixture does not independently certify the legacy swizzler or GS sampling;
row correction occurs after that existing decode stage. Actual window captures
and original PS2 comparison are separate validation scopes owned by the root task.

## Source-integrity delta

Existing TASK0048 audited load_scene changes only its two decode expressions
(base and mip) to the named adapter; assets.cpp also adds its header include.
Do not refresh immutable TASK0048 pins. Record these exact changes in the
TASK0049 successor review. No original recovered function body is modified.

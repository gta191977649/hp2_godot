# Static base material and color investigation

- Binary SLUS_203.62, revision unknown; sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- E-0033-B12, Ghidra decompile00188180,00116A30,00116828,001169F8,00115978,00104BD0; originalGLOBAL/GLOBALB.BUN;2026-10-14.
- Status: observations only; no complete base-color shader reconstructed; not Verified.

## Confirmed

Skin catalog chunk34601 uses unaligned payload rows stride0x40, unlike aligned car catalog34600. 00188180 matches car index and skin byte, returns first row on permitted miss. Row+0x2c is used by0011E860 to look up a material registry entry through00116A30. Renderer0010CAA0 then substitutes material-table keyd6d6080a with that selected object for body, mirrors and other attachments. This is distinct from texture-page substitution.

Original skin1 observations: car39 rowfile0x9e518 is named Arctic Silver and materialhash93c0eaf6; car31 row0x9d998 is Police and materialhashd22ff4b6; car51 row0x9edd8 is Metal and materialhash93c0eaf6. WithinGLOBAL chunk3401e, records observed at0x20+index*0x140 carry hash+8, name+0xc and parameter floats+0x20. MetPaintSilver atfile0x7a0 has hash93c0eaf6; RegPaintBlack at0x660 has d22ff4b6. These are named original material descriptors, not invented RGB color names.

001169F8 computes two four-vector parameter blocks via00116828 from descriptor+0x20 and+0x70. The expressions include multiple coefficients, PS2 float/int bit manipulations and alpha-related128 scales; these cannot safely be treated as a single constant albedo tint. 00115978 builds a lighting context with original frontend/environment-specific vectors and alpha128; opaque mesh submission also receives it. Port-side raw texture display omits this material/context computation.

## Bounded gap

It is not established that multiplying raw textures by a constant material RGB or by packed vertex fields reproduces original base color. The VU path and finalGS texture function, per-vertex color expansion/modulation, alpha and pass selection must be traced together. No guessed tint, conventional Godot lighting or flat shader substitution is justified by the above data. The inspection pass remains explicitly port-side and the full static shading acceptance criterion remains incomplete.

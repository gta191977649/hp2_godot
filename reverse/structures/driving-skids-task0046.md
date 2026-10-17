# Debug skid layouts (TASK-0046)

- Evidence: E-0046-S01/S03, read-only Ghidra data types and bounded function listings.
- Binary: Debug NFSHP2.ELF SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
- No native host `sizeof`, inheritance adjustment or pointer width is asserted.

## Confirmed Debug fields

`SkidSegment` extent0x10: Position float[3] at0; DeltaPosition signed char[3] at0xC; Intensity uint8 at0xF. Delta decode uses1/64. `SkidMaker` extent4: pSkidSet pointer0.

`SkidSet` extent0x110: inherited Next0/Prev4; LastNormal0x10; LastSegmentLength0x20; pCar0x24; pClan0x28; pClanNode0x2C; pSkidMaker0x30; TheTerrainType0x34; Position0x40; BBoxMax0x50; BBoxMin0x60; SkidSegments[8]0x70; NumSkidSegments0xF0; BBoxCentre0x100. Original InitSkids allocates128 slots, alignment16, overflow disabled.

`Wheel` uses TheSkidMaker0x180, Terrain0x150, Slipping0x154, TouchingGround0x155, ZForce0x158, TractionUsage0x168, Velocity0xF0 and WorldMatrix0x70. `WheelInfo` Width0x10 and WheelNumber0x14. `Car` BlownTireBits0x848 and TheOverrideRenderingPosition.VisibleViewBits0x514.

`TerrainTypeInfo` record stride0x90, SkidTextureName0x10, SkidWidthMultiplier0x14;33 records at003102E8.

`Clan` has SkidSetList0x8, BBoxMin0x30 and BBoxMax0x40. The current synthetic fixture explicitly uses host arena list/bounds addresses in place of whole original Clan/GetClan routing; pClan in the bound skid image is a service handle and is not claimed to point to a reconstructed original Clan. Clan streaming and spatial eviction are unresolved.

`TextureInfo` exact Debug fields: RenderingOrder signed char0x29, ApplyAlphaSorting0x4D, TextureFx0x4E, IsAnySemitransparencyFlag0x4F, bWriteZ0x50, GlossMapInAlpha0x74, AlphaBits0x76, AlphaFix0x77. Actual Debug AttachSolidPolys/AttachAlphaPolys inspect RenderingOrder29 for normal depth-context selection, not bWriteZ50; do not infer control flow from field names.

`eView` H0x8, NisPlaying0x6, ID0, pCamera0x38. H originates from CalculateH(horizontal FOV); tool camera projection and position are explicit host inputs.

## Limits

All fields describe this Debug identity only. Retail evidence remains separate. Ordinary finite native fixtures and source review do not establish PS2 numerical equivalence; no structure is Verified.

# E0056T01 - Original model texture-table default binding

Authority: Retail SLUS_203.62 SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Debug NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b supplies verbatim names and comparison only.
Fresh read-only Ghidra exports under ignored local/task0056, 2026-10-19.

## Confirmed

- eSolid::FixTextureTable Retail0010BDB8 / Debug0010C1B0 iterates the texture
  table (+B8, count signed short+AC) and calls GetTextureInfo(NameHash,1,0).
- GetTextureInfo Retail001ABF48 / Debug001A95C8 searches loaded packs, then
  eGetRenderTargetTextureInfo; if missing and the second argument is nonzero,
  recursively requests0x19006835 with(0,0). If that fails it calls bBreak.
- GLOBAL/GLOBALB.BUN header0x4C24 contains actual DEFAULTTEXTURE,
  NameHash19006835,32x32. This is original data, not generated placeholder pixels.
- Track61..66 decoded original model tables contain ff91e772 in flower/person
  models (Track61 probe names TROPIC_D_REDFLOWERSPRIT, TROPIC_D_WOMANTAN).
  The hash is absent from the selected/shared packs and the checked uncompressed
  non-car BUN/BIN resources. The original fallback rule therefore matters when
  all authored levels are submitted instead of a small inspection view.
- Retail eGetRenderTargetTextureInfo001057D8 searches22 targets;
  eGetRenderTarget00105848 uses2CFD50/strideB0; target GetTextureInfo00105AA0
  uses2C8690/strideA4. Static TextureInfos are initially zero, so initial RAM
  hashes alone are NOT evidence of runtime target names.
- Retail SetScreenBuffers001064C8 establishes15 literal TARGET_* names plus
  seven EnvmapTargetNames pointers at2C8670. Debug SetScreenBuffers00106320
  and UpdateGifTags001058D8 show NameHash=bStringHash(Name). Fresh Retail name
  bytes and seven pointer dereferences yield all22 names; none hashes toff91e772.
  Exact names/hashes are retained in local/task0056/render-target-names.json.

## Bounded implementation

Original names stay in original declaring scopes: eSolid::FixTextureTable in
src/ecstasy/ecstasye.cpp; free GetTextureInfo in indep/src/texture.cpp (existing
E-0042-S001 TU/class ownership). Explicit immutable loaded/target-name sets
replace process-global TexturePack pointers. This typed value overload restores
resident-name resolution/default selection, NOT pack LRU mutation, UsedFlag,
streaming/unloaded lookup, original ABI, full target allocation or bBreak handling.
Missing DEFAULTTEXTURE raises NotImplemented at001ABF48; an actual static-scenery
render-target binding remains an explicit gap at001057D8, never a default guess.

The port loader retains requested->resolved aliases in Scene::texture_fixups and
the original bundle unchanged, while its render-surface carrier stores the bound
TextureInfo hash. Default pixels/mips pass through the existing original decoder
only when required. Existing shader, alpha, depth, queues and material code consume
the resolved header; particles/screen textures do not receive this model-only fixup.
This applies to both LOD backends so they share actual resource binding.

## Status

Reconstructed bounded selection, not Verified. Pack paging/order and original
hardware/runtime equivalence remain outside the evidence. The failed initial
all30 host run (24success/6unresolved-texture failures) is retained as counterevidence
in local/task0056/catalog-host.{log,json}; final corrected results are separate.

Provenance closure: local/task0056/elf-provenance.json independently SHA256-checks
both authorized external files and compares fresh Ghidra instruction bytes at both
paired entries (112/260 bytes). All four match their authorized files exactly.
Corrected all30 host load/view/clear passes, including61..66 using the original
fallback. Nodes return to6 after every clear; initial failed run is retained.
Track61 real GPU probe validates an actual32x32 uploaded default and captures it.
These establish byte provenance and supported PC behavior, not original runtime equivalence.

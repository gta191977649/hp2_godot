# Vehicle paint binding and strip submission

- Binary SLUS_203.62, revision unknown, sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- E-0033-B07: read-only Ghidra decompile00188790,0011FE60,0011E860,0010C7F0,0010C680,0010C878; memory002B4380;2026-10-14.
- E-0033-B08: read-only decompile0010DD58,00104A20,00104DF0,0010D390;2026-10-14.
- Private raw evidence ignored local/task0033/*-material.json and002B4380-material-names.json.
- Native slices remain unverified against original runtime.

## Confirmed paint behavior (B07)

00188790 formats exact names002B4380 `%s_SKIN1`,002B4390 `%s_SKIN1B`,002B43A0 `%s_SKIN%d`,002B43B0 `%s_SKIN%dB`. The selected skin index comes from configuration byte+5; its original default is not established. The viewer explicitly selects1 as a port-side default.

0011FE60 writes source/target pairs SKIN1->selectedSKIN and SKIN1B->selectedSKINB into constructor patch tables. 0011E860 binds the first3 body states to a12-entry patch table; final body uses a2-entry table with the same two paint mappings. Before binding, body slots2 and3 in non-type2 cars call0010C7F0 to replace an actual solid texture-table entry with SKIN1B when its key was SKIN1. Therefore the port binding flattens that sequence: normal body LOD0/1 maps SKIN1->SKINn, LOD2/3 maps SKIN1->SKINnB; every LOD maps SKIN1B->SKINnB. Type2 does not perform this pre-rewrite and always maps SKIN1->SKINn.

0010C680 resolves each target texture through001ABF48, matches source keys against the solid texture table at+0xb8 (count+0xac), and records matched index per geometry binding. 0010C878 writes the resolved object into that exact index. Native binding stores hash pairs; host applies them once to original strip texture hashes before lookup. It does not mutate original assets.

Other constructor table entries concern light state/damage;00120130 subsequently updates lighting mappings from live/environment state. E-0033-B09 now implements the explicit null-car/no-environment-override lights-off selection for four mandatory placeholders; see00120130.md. E-0033-B10 additionally implements all six null-car window targets from0011FF68. Active lamp and dynamic damage substitutions remain outside the static slice. Do not claim raw-texture inspection reproduces their shading.

## Confirmed submission behavior (B08)

0010DD58 accesses a strip's material object through signed byte at strip+0 and solid texture table+0xb8, entry stride8. It reads flags at strip+0xe. Bit0x400 excludes the strip. Texture+0x4d selects immediate versus deferred queue; this alone does not establish the blend equation. Texture+0x74 contributes flag4. Some caller bits clear0x100 or0x80; a null lighting-data pointer sets clear mask0x1c0. 00104A20 enqueues a16-byte command with the final flags and strip address/count.

## Unknown

The downstream VU/GS program's material lighting, blend equations, vertex-color modulation and winding/cull flags are not recovered here. Texture alpha metadata is not proof of material translucency. A port unlit inspection material is explicitly host debug visualization, not recovered equivalent shading. CarPartCuller diagnostic remains unbound; 0011D980 call relation alone does not establish class identity.

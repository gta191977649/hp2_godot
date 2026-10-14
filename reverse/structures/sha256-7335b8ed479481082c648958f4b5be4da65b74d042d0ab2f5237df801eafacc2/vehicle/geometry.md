# Vehicle geometry container and packet layout

- Task/date: TASK-0033 / 2026-10-14.
- Binary: SLUS_203.62, executable revision unknown.
- SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- No original-device or PCSX2 comparison; not Verified.

## Evidence

E-0033-G01: read-only Ghidra decompilation of 001036C8, 00103FF0, 0010BDB8 and 0010C458. Raw outputs retained in ignored local/task0033/<address>.txt. 001036C8 was requested at an interior instruction 0010379C; returned function entry is 001036C8.

E-0033-G02: read-only Ghidra decompilation of 0011BF98, 0011C090, 0011C180, 0011C248, 0011C268, 0011C2B8, 0011C3E0. Raw outputs retained under the same ignored directory.

E-0033-G03: byte-level inspection of configured original CARS/911TURBO/GEOMETRY.BIN (529472 bytes), CARS/BMWM5COP/GEOMETRY.BIN (471696 bytes) and CARS/4DR_SEDAN/GEOMETRY.BIN (52432 bytes), accessed through tools/ps2_assets.py.

## Confirmed layout

001036C8 accepts chunk 0x80034000, then traverses child containers. Inside each solid it recognizes 0x34003 (header), 0x34004 (strip table), 0x34005 (packet stream), 0x34006 (texture reference table), 0x3401D (another hash/reference table), and further optional chunks. Header/table/stream pointers align `(chunk + 0x17) & ~0xf`; 0x34006 starts at chunk+8. 00103FF0 relocates each strip+8 by adding the packet stream base; entries have stride0x40 and count is header+0x34 signed16. 0010BDB8 resolves each 8-byte texture reference through 001ABF48. 0010C458 resolves header+0xae entries of the 0x3401D table through00116A30.

Original bytes establish header hash+8, NUL-terminated name+0x10, bounding values+0x40/+0x50, and raw float16 matrix+0x60. These metadata are preserved without applying the matrix or claiming its instance-transform meaning. Strip byte0 selects texture reference (corroborated by0010C158). Asset bytes contain material-table indices at strip+0x1f including0xff; the port preserves this reference without inferring render flags.

The first 4DR_SEDAN strip bounds are exactly0.01 beyond the decoded position extrema in each component: min(.596857667,-.958752871,.751681387), max(.726295233,.959563673,.882787466), versus decoded min(.606857657,-.948752880,.761681378), max(.716295242,.949563682,.872787476). This is direct original-asset evidence that positions already occupy the strip-bound coordinate frame. It does not identify how a caller transforms that frame.

The packet count is byte+4 (0011C248). Positions are groups of four X values, four Y values, four Z values starting+0x20. The partial final group inserts a command word before its packed remainder when a previous group exists (0011BF98). UV values follow the position segment, with an extra command for a position remainder. Pairs are stored U0,V0,V1,U1 with intervening command words; 0011C090 determines exact offsets. 0011C180 identifies the following 16-bit color stream; raw16-bit colors are emitted without claiming a complete shading interpretation. 0011C3E0 moves input integer bits16..20 into output low5, bits8..12 into next5 and bits0..4 into next5; the original input integer color-channel convention was not established by that packer alone. The original VIF V4_5 command unpacks low5 to X/R, next5 to Y/G, next5 to Z/B and high1 to W/A, and the base VU path does not swap these channels.

In 4DR_SEDAN the first body strip table begins0x230 and its packet stream begins0xc00. Strip+0x1d agrees with packet count. Strip+0x1e agrees with popcount of packet+0xc..+0x18 across inspected strips, corroborating the topology-control interpretation independently from the VU analysis in E-0033-T03 (reverse/structures/vehicle-vu-adc-task0033.md). The decoded ADC bit suppresses the triangle ending at its vertex while retaining the previous two positions. The native presentation expands the chronological strip into a triangle list, alternating face order by the global vertex index; suppressed vertices advance that index too. This is a port representation of the original stream, not an original index buffer.

## Native projection and validation

port::assets::decode_vehicle_geometry is a host reader, not the original001036C8 ABI or resource-manager implementation. Type_0011BF98 groups address-anchored packet accessors without asserting an original class identity. Checked arithmetic, finite checks and exceptions are port safety additions.

Standalone g++13.2.0 (MinGW-W64 r8), `g++ -std=c++17 -Wall -Wextra -Isrc src/game_core/Type_0011BF98.cpp src/port/assets/vehicle_geometry.cpp src/platform/ps2_asset_root.cpp src/tests/vehicle_geometry_tests.cpp -o local/task0033/geometry_test.exe`, compiled source/accessors and passed one-/four-vertex position and UV cases, out-of-range indices, missing/truncated containers and all three authorized originals. Parsed totals:911TURBO16 solids/14893 vertices; BMWM5COP13/13304; 4DR_SEDAN5/1628. Every emitted strip triangle count matches the original strip+0x1e count across all three full files. Synthetic strip tests cover exact two-triangle indices, invalid ADC mode and triangle count mismatch. These counts cover all asset solids including damage/effect/LOD variants and are not the visible-car selection.

## Unknowns

Normals, complete shading, cull/blend flags, skinning, coordinate mapping into the host, header matrix semantics and runtime instance transforms require separate original consumer evidence. The reader emits original-position geometry, UV, packed color, decoded ADC triangle selection and raw flags; it explicitly reports the missing full material/lighting/culling passes. Optional animated/skinned data are not interpreted. The inspection backend may disable culling as an explicitly port-side viewing choice; this must not be described as original culling behavior.

## Runtime test root behavior

vehicle_geometry_tests resolves the configured original asset root through platform/ps2_asset_root.hpp when no explicit input paths are supplied. It runs all three original geometry inputs whenever configured; an invalid configured root or a missing required file fails. Only a genuinely unconfigured root skips original-file checks. Each packet is copied into its own bounded byte view before component reads, so valid bytes belonging to a following packet cannot satisfy a truncated packet read. A synthetic short-packet test verifies this boundary.

## E-0033-G04: narrow fully-in-view culling baseline

Read-only original ELF inspection followed0010DC80 ->0010D390 ->0010CF18/0010CF50/0010D050 ->0010CDD8. Detailed function records retain source addresses and raw private capture paths. 0010D390 sends fully-inside solid classification2 directly to0010CF18, which installs the complete strip range without inspecting normal cones or triangle orientation. The later0010DD58 skip of strip+0xe bit0x400 remains in force. This supports retaining both faces for the fully-in-view static exterior inspection baseline; it is not a universal statement about every original pass.

For partial bounds or special dispatch mode2, header ushort+0x32 bit0x1000 selects0010D050; strip ushort+0xe bit0x1000 then enables its normal-cone predicate. Original asset scan:911TURBO_A has header0xD180 and12 flagged strips (0x5080); BMWM5COP_A has header0xD180 and10; no other solids in the three sample containers have flagged cone strips. In mode2 packed angle+0x36 is unsigned and scaled0x3BB40000; mode1/3 uses a signed value unscaled. This listing-confirmed difference and table-based angle helper0025A748 remain untranslated.

Original VU path24C..2C3 transforms vertices and emits ADC from the recovered RNEXT pipeline without a face determinant predicate in that bounded path. The following4B4/504 routines test CLIP flags and repair clipped triangles; their predicates compare positions against clip volumes, not a recovered generic winding sign. An extended read-only decoder probe retained in ignored local/task0033/vu_cull_probe.py removes the original900-instruction inventory limit to inspect these later original MPG uploads. This static check does not establish an exhaustive absence of backface tests in every overlay/pass.

Host triangle-list alternation remains a presentation convention for consistent normals. Because the inspection material retains both faces, no unverified clockwise/counterclockwise rejection is applied. Partial-frustum cone rejection and original clipping precision remain explicit gaps; do not promote the full original culling subsystem to recovered or Verified on this baseline.

Additional catalog coverage (2026-10-14): ignored catalog_model_probe.cpp used port::vehicle::Catalog and Model::load_id for the first original skin of every available catalog row. All62 available car IDs passed geometry, part, material and texture binding; IDs45/46 had no BIN under the catalog's explicit availability check. This is not all-skin or GPU validation. Across unique bound solids per loaded model, raw render-flag frequencies were0000:1508,4080:3675,4100:6,4180:18534,5080:237,5180:31,8000:48,C080:16,C180:1680. No0x40 material route was encountered. Ignored raw output is local/task0033/catalog_model_probe.txt; source and assets remain outside tracked evidence.


## E-0033-G05 host framebuffer counterevidence and pixel check

A real Compatibility GPU oracle initially produced RGB(31,25,25) for native VU fixture
RGBA(67,28,19,128) and textureRGB(64,128,192), whose GS MODULATE result is(33,28,28).
This retires the assumption that passing encoded ALBEDO through Godot is an identity.
Pinned Godot4.4.1 [tonemap_inc.glsl](https://raw.githubusercontent.com/godotengine/godot/4.4.1-stable/drivers/gles3/shaders/tonemap_inc.glsl)
uses a cubic input decoder and a power output encoder which are not inverses.
The port adapter in car_base_shader.hpp now inverts that cubic with bounded Newton
iterations after computing the output encoder's inverse. This is host color transport,
not an invented HP2 gamma curve. Environment exposure/tonemap remain identity.

Optional car_material_gpu CTest uses shaders actually uploaded by Ps2CarViewer, then
sets independent original-instruction-oracle fixture inputs. Final GPU bytes now match
copy(33,28,28), source-over(16,14,14), alpha0 discard, strict ATST6 boundary48 discard
and49 accept. Tolerance is one framebuffer byte for host conversion/blend rounding;
observed fixture copy/source-over match exactly. It does not establish original-runtime
frame equivalence or all-GPU equivalence. Source/overlapping state and filtering limits
remain separately recorded. Run PORT_ENABLE_GPU_TESTS=ON with a real Compatibility
renderer; a headless dummy renderer is insufficient.

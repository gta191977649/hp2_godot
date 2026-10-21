# TASK0052 postprocessing reconstruction

Status: Reconstructed, not Verified. Retail executable is the format and rendering authority. Debug supplies names, type fields and source ownership. GPU probes below validate the PC adapter only.

## E-0052-P01: depth and fog

Retail SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`; Debug SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`. Read-only Ghidra captures remain outside version control in `local/task0052`.

| Original function | Retail | Debug |
|---|---|---|
| RemapLinearIntoClut8 | 001189F8 | 00118DC0 |
| eAttachDepthIntoAlpha | 00118A60 | 00118E28 |
| CreateFogPalette | 00119308 | 001196D0 |
| eAttachFog | 00119768 | 00119B30 |
| MapScreenZToPaletteIndex | 00119B58 | 00119F20 |
| MapPaletteIndexToScreenZ | 00119B70 | 00119F38 |
| MapScreenZToViewZ | 00119B90 | 00119F58 |

The original declaring TU is `src/ecstasy/eenveffectse.cpp`, obtained from Debug ECOFF source ownership (E-0042-S001). Typed host values are explicitly not the original eView/eDmaChain ABI. Remap swaps the 8..15 and 16..23 slots in every32-entry block. The fog power operation is integer/float reinterpretation affine power, not std::pow: Debug00119904..00119964 MAX.S clamps to bits3C23D70A, converts float bits as integer, applies power and one-bit bias, and reinterprets back.

Retail global base0032FA98 independently matches Debug00326B00: FogMode0, DrawFog1, FogPower4, DrawDepthOfField1, FocusAlphaMin0, FocusAlphaMax128, FocalDistance0, DepthOfField0, JitterBlend64, dofNumPasses2, dofOffsetX/Y4008, transitionSlots24. Retail GP003358F0 is established by startup00100058, not inferred from the Debug GP.

Normal fog iterates i0..255, clamps/interpolates from WeatherQuery.FogStart to255, applies the affine bit power, then writes index255-i using WeatherQuery.FogColour packedBBGGRR and trunc(power*Fog*128) alpha. Night reverses the interpolation and has RGB0; DEPTH emits gray indexRGB withalpha128; SINGLE writes one selected slot. Modes use ALPHA selectors respectively (0,1,0,1),(1,0,0,2),(0,1,0,1),(0,1,0,1), FIX128.

Depth is not a world-distance fog approximation. Retail0010AC50 creates normal geometry GIF register list412: ST2,RGBAQ1,XYZF2=4. PACKED XYZF2 Z extracts lane bits4..27, so GSZ24=(floatBits(screenZ)>>4)&FFFFFF. Independent Roadshine reconstruction corroborates this correction (TASK0052 D05). The three-copy depth-into-alpha route reinterprets PSMZ16 and writes alpha only: its semantic result is min(255,GSZ24>>8). This agrees with original MapScreenZToPaletteIndex's MFC1/SRL12/AND255 for the nonclamped range. It supersedes the older TASK0051 assumption that XYZ2 and XYZF2 packed Z share the same bit placement.

## E-0052-P02: original focus lookup and passes

| Original function | Retail | Debug |
|---|---|---|
| CreateFocusPalette | 00119C20 | 00119FE8 |
| eAttachDepthOfField | 00119F70 | 0011A338 |

MapPaletteIndexToScreenZ bit-casts 40000010|(i<<12). The original inverse projection uses Z24 range40000010..40800000. Reversed256 distance slots identify focal +/- DOF/2 band; outside alpha128 and inside alpha0 with24 transition slots. Zero global focal/DOF uses camera fields; both effective values0 disable the effect.

Stage5 order is DepthIntoAlpha, Fog, FocusMask, FocusClear, FocusJitter passes, FocusComposite. FocusMask retains RGB and replaces destination alpha. Half target256x224 is cleared black. Two direct full-to-half source draws use TEX1=61, additive FIX64 and source offsets fixed16 (0,0),(32,32), i.e.0 or2 source pixels. Composite blends the half image into full using destination alpha. The original target is512x448, backing texture512x512. 4008 is GS XYOFFSET2048.5, not a UV translation. The adapter preserves original sample extents while projecting the final output to the host viewport; raster/filtering is a host approximation.

## E-0052-P03: screen filters and previous frame

| Original function | Retail | Debug |
|---|---|---|
| DisplayRadialBlur | 0010F280 | 0010F660 |
| DisplayBlendPrevious | 0010F4F0 | 0010F8D0 |
| DisplayFilter | 0010F6A8 | 0010FA88 |

Original owner TU is src/ecstasy/ecstasye.cpp. Stage7 order is Filter, RadialBlur, BlendPrevious, gated by DrawScreenFilters. Filter uses hash02C8711C and per-view eFilter color/blend/TFX; static filters start inactive. Radial mask hashAB9A6E21 is drawn as four mirrored quadrants, centeralpha0 and staticedge192; RadialBlurOn Debug10F508 setsedge160 unless POSTRACE. Previous frame samples MainDisp0 using halftexel512-backing UVs and ALPHA(0,1,1,1), which uses destination alpha. No speed-based blur trigger is invented. Native quad descriptions and host pingpong history composition exist; original event triggering remains explicit input.

## Host contract and validation

TrackPostprocess owns colour/depth SubViewports sharing World3D and cloned source cameras, a layer1 CanvasLayer, two history targets, and cached native LUT images. It restores the original source-camera mask on release. Layer20 is reserved exclusively for depth proxies; colour excludes it. Source camera renders no3D while active. Native frame output may be passed directly, avoiding a second native render session.

A late hint_depth_texture probe was rejected: transparent depth_draw_always road was absent from its copy (opaque viewZ2; transparent clear/far). Depth capture instead clones visible writing MeshInstance3D geometry with original shader vertex transformation, fragment discard, render priority and sorting fields. The depth variant replaces only final color/blend output with encoded raw Z24 RGB and A1; no-write passes discard. Existing hp2_framebuffer_rgb preserves encoded bytes. Unknown writing shader materials are rejected explicitly. Camera override makes clear pixels black/rawZ0. Colour and depth remain separate targets; alpha is not used to encode depth because spatial blend_mix would corrupt encoded RGB.

Private native tests cover CLUT permutation, all256 inverse slot mappings, depth alpha saturation, four fog modes, focus gates/band and pass ordering, screen quads and previous halftexel coordinates. GPU independent test `local/task0052/post_proxy_probe.gd` on Intel Arc/Godot4.7.2 Compatibility: transparent red colour128, raw Z24 RGB(119,254,1)=01FE77 atviewZ2, fog blue alpha64 producesRGB(64,0,127), matching the byte GS equation. This is a PC adapter check, not original matched-runtime verification.

Known integration gaps: original screen-stage event triggering is not yet routed from the full Nitro/zone/replay systems into this host. Fog, focus, black-and-white attachment and conditional screen-stage composition are reconstructed. Original stage5 calls black-and-white after focus. Nondefault DOF jitter table extent beyond2 is rejected. No arbitrary modern grading LUT, Gaussian blur or inferred weather constants are used.


## E-0052-P04: actual activation providers

Camera constructor Debug001566E0/Retail00156BE0 writes CurrentKey.FocalDistance/DepthOfField (+74/+78) both0. Debug DriveCameraMover::Update0015C2B0 writes15/65 only when !PerfectFocus and !Camera::StopUpdating. Actual Player camera type1 call at Debug0017545C passes constructor last argument perfect_focus=1; consequently the current normal player branch retains zero focus and the original DOF checkbox alone does not activate it. Other original camera movers have distinct focus inputs (OrbitCar radius/8; Bumper distance/7.5; TrackFly distance/30); these are not substituted into the normal player camera. Debug camera source captures are local/task0052/camera-*.json and player-camera-construction.json. Full Retail correspondence of the larger mover/player bodies is not established by these Debug-only behavioral observations.

RealDriver::Drive Debug0013B200 (RadialBlurOn callsite0013B2E8), corroborated by Retail0013B3F8, activates radial blur only on NitroActive rising edge, calls RadialBlurAlpha(player,0,0.8), and deactivates on falling edge. RadialBlurAlpha0010F618 truncates float*255 into uint8 (edge204 for0.8), not float*128. Additional providers include ZonePreviewCameraMover, Player::RequestZone and ReplayScreen::ChangeCameraMover. No speed threshold is inferred. These original events remain unavailable in the currently restored normal non-nitro driving slice; inactive screen state is therefore explicit rather than pretending the effect was triggered.

BlackAndWhite attachment Retail0011A9F0 has a128page PSMCT16 source/target copy with FRAME mask7FFF and eight-texel even-to-odd stripe shifts, followed by PSMT8H sampling through the identity gray CLUT. Proving its RGB meaning requires the CT16/CT32 physical word mapping, not an assumed luminance dot product. Retail global override0032FB00 is0. The previously rejected nonzero path is now closed by E-0052-P05 below; no modern grayscale weights are introduced.

Host cache contract: source shaders and per-node writing-material layouts are immutable resources. Shader programs are cached by original Shader ID, not per-material instances. mark_tracked_material seals static parameter snapshots; notify_material_parameter forwards known mutations without Godot getters. mark_material_changed is the full synchronization fallback. Unsealed vehicle materials retain full parameter comparison. Wholly nonwriting nodes do not allocate depth proxies. Layer20 is reserved for depth capture. Shader compilation cost is paid per unique program.

Additional GPU focus fixture (post_focus_probe.gd): constant fogRGB(64,0,127), two FIX64 accumulation draws yield blue126; full focus shader output isRGB(64,0,126), proving original per-pass integer truncation is retained in this PC fixture.


E-0052-P03 triangulation correction: Debug ePoly constructor0010FD48 sets UVs(0,0),(1,0),(1,1),(0,1), neutral RGBA128. ePoly::Render0010FE48 emits indices0,1,3,2 as a triangle strip. Consequently RadialMask center vertex2 Gouraud weight is max(0,u+v-1), not min(u,v). The external shader uses the original diagonal. Native Quad vertices retain original ePoly indexing; consumers must use that strip order.

Corrected strip-diagonal GPU radial/history fixture: edgepixel(5,5) givespreviousgreen192; centerpixel(128,128) givesRGB(127,1,0), consistent with center texel not lying exactly at geometric midpoint and interpolated alpha1. The previous183 result used the rejected fan diagonal and is not a valid original-raster reference.


## E-0052-P05: black-and-white physical-bit route

Original eAttachBlackAndWhite Retail0011A9F0/Debug0011ADB8 is restored in its declaring eenveffectse.cpp. The active-player attachment retains the optional Retail global override and per-view mode gate, runs after focus, and has an explicit MainDisp-alpha dependency. SetupBlackAndWhite returns the original identity256 gray palette with alpha128; this exact linear LUT is uploaded by the host.

Primary hardware specification: Sony Computer Entertainment, GS User Manual v6.0, section3.9.5 (page72) specifies FRAME masks before format conversion; section8.3.2 (pages165-166) locates the low/high16-bit halves at adjacent eight-texel stripes. An archived copy is available at [Sony manual mirror](https://github.com/ninjadynamics/PS2Docs/blob/main/GS_Users_Manual.pdf), SHA256 `daf72c776f6e27721149f28dc46ad47443e560b22a04f3d8ff4bcf74f254bf12`. It remains outside Git in local/task0052. This is the hardware specification used to interpret original game packets; emulator implementations are not reconstruction authority.

The original first pass reads MainDraw0 PSMCT16 even8-wide stripes and writes the corresponding MainDisp0 odd8-wide stripes. FRAME mask00007FFF converts to CT16 mask01FF, preserving destination bits0..8 and copying source bits9..15. Physical low-half RG therefore supplies G bits1..7 to the high-half BA destination's A bits1..7, retaining MainDisp A bit0. The source/destination CT16 coordinates address identical physical words. This applies across all128 pages. Final PSMT8H sampling accesses MainDisp0 alpha through the original gray palette, with neutral vertexRGBA128 and no enabled blend on PRIM116. Thus output gray index is `(current_green & 254) | (main_display_alpha & 1)`; output alpha128. This is a bit-exact semantic projection of the specified mask/halfword/lookup route, not a modern luminance conversion.

A separate agent independently checked the original packet mask and the primary mask conversion; the physical stripe mapping is specified by the primary table above. Native tests use distinct green and historical-alpha low bits and verify stage order. GPU `post_bw_history_probe.gd` uses currentgreen170 and previousalpha129 and obtainsRGB171/alpha128 in a transparent history SubViewport. Host history stores exact byte alpha using canvas blend_disabled; the final main-window blit forces opacity. Main-window swapchain alpha is not used as evidence for intermediate target alpha.

Sampler audit: original eRenderTarget::UpdateGifTags Debug001058D8 sets target TextureInfo MMag1, MMin2, MXL0 and TextureFx0. DisplayBlendPrevious halftexel UV range has a slightly magnifying footprint and therefore uses linear MMAG1. The host history blend uses linear sampling; BW's previous-alpha LSB uses exact pixel texelFetch because the original bit-preserving first pass addresses physical words. FocusComposite retains TEX1=61 from jitter; there is no intervening reset in Retail00119F70. Its native Pass metadata now records61 too.


## E-0052-P06: host ownership and reproducible GPU regression

TrackPostprocess acquires a per-camera main-thread ownership entry before saving a nonzero camera mask. A second visible compositor fails explicitly without restoring another owner's mask. Hidden owners release before creating capture nodes; only the registered owner restores its saved mask. An exception during update releases partially created nodes and restores ownership/mask. ObjectID validation guards availability/depth texture access after owner or source camera deletion. This is host resource policy, not invented original behavior.

Reproducible numeric GPU check: `Godot_v4.7.2-stable_win64_console.exe --path godot --rendering-method gl_compatibility res://scenes/track_postprocess_gpu_smoke.tscn`. Passed on Intel Arc B390 with `TRACK_POSTPROCESS_GPU_OK`. It loads the production external shader source and checks actual intermediate RGBA8 targets for Fog(64,0,127,64), Focus(64,0,126,128), Filter(64,0,127,64), BW(171,171,171,128), and radial/high-destination-alpha history. This does not depend on original assets and does not establish original raster equivalence.

Actual-asset ownership regression: same command with `res://scenes/track_postprocess_lifecycle_gpu_smoke.tscn`; requires original configured asset root and latest module build. It loads Track11 into two runtime renderers and checks hidden consumers, rejection of a second visible compositor, transfer after hide and exact mask restoration on clear. Status at authoring: pending the root agent's serialized full extension build. Headless/missing original asset configuration returns77 rather than claiming a GPU pass.

Additional asset observations: GLOBAL/INGAMEB.BUN header0D3114 is RADIALBLUR hashAB9A6E21,64x64 PSMCT32, MXL0/MMAG1/MMIN3. Its four quadrants magnify the64x64 source, so linear MMAG1 applies. Original FilterGrey hash02C8711C (both original eFixUpTables bodies) has no raw occurrence in the provided BIN/BUN files. The native pointer/resource gate remains explicit; GREYMAP and SOLIDGREY are not substitutes. Debug DrawScreenFilters003267AC is initialized1, independently read from the executable.


Depth-alpha saturation independent check: Retail00118A60's post-copy clear packet uses FRAME RGB maskFFFFFF and RGBAQ3F800000FF000000, writing destination alpha255. The final packet restores the original ZBUF base/format with write disabled, sets TEST50000 (ZTE enabled, GEQUAL), and emits XYZ2 ZFFFF. Only original storedZ<=FFFF can overwrite that alpha; nearer storedZ>FFFF retains255. Another agent independently read and corroborated these raw packet fields. Thus near saturation follows explicit clear/depth-test control, rather than assuming that the MapScreenZToPaletteIndex bit extraction itself saturates. That bit extraction wraps by construction; it is separately used for LUT slot mapping/inverse construction. BelowFFFF the CT16 byte extraction is a distinct subpacket mapping. Pass records describe semantic attachment kinds, not a replacement implementation of every GS DMA subpacket or original VRAM allocation address.


Post-target alpha dependency clarification (TASK0052): MainDisp RGB is the preceding displayed frame, but MainDisp alpha is scratch modified by the current stage5 DepthIntoAlpha. When enabled, BW preserves the current clamped-depth alpha LSB, not the preceding frame alpha LSB. MainDraw alpha initially receives the wrapped depth byte; Fog replaces it with palette alpha. FocusMask replaces it again with focus-palette alpha before jitter; final Focus output alpha is the halfimage focus alpha. DisplayBlendPrevious samples preceding RGB with current MainDisp scratch alpha. The production GPU fixture distinguishes these resources: Focus expects alpha128 despite Fog alpha64; BW with green170 and preceding alpha128 returns171 when current depth alpha255 is enabled, but170 when depth transfer is disabled. Earlier standalone history-only examples omit the stage5 override and do not establish full-frame target dependencies.

Host mesh cache update: mark_tracked_mesh explicitly seals mesh/visibility/transform/material-layout/sorting state until mark_mesh_changed. Material parameter notifications remain independent of mesh state. Unsealed vehicles and particles retain dynamic checks. Discovery excludes nested Viewports, including the six environment-capture scene copies, and repeats on node-count changes, new registrations or deleted cached sources. This is a host optimization, not original culling behavior. Writing screen-space particles retain their original custom DEPTH and encode particle_z directly into RGB; deriving raw Z from their packed screen-space VERTEX would be invalid. Nonwriting particles never enter the depth capture.

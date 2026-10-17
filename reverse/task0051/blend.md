# Track effects and ordering

## Artifact identities and scope

Retail `SLUS_203.62`, SHA256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`, is format/render authority. Debug `NFSHP2.ELF`, SHA256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`, supplies names and declaring-owner correspondence. Ghidra access was read-only. Native scalar tests are not original-hardware numerical or visual verification.

## B01: original static command selection

Retail eModel::RenderStrips `0010DD58`, Debug `0010E120`: strip flag400 suppresses submission. Target alpha suppression removes texture+4D nonzero; caller8 removes +4D zero. Disable environment/caller1 clears100; caller2 clears80; absent light context clears1C0. Texture+74 adds4; partial visibility value1 adds200. Effective flags go unchanged to AddData00104A20/VIF dispatcher0010A438. Material index is strip+1F with Default fallback, independently of80. Typed eModel::RenderStrips implements this bounded selection slice, not animations, texture replacement tables, DMA allocation or the whole method ABI.

eDisplayFrame's typed overload is a host-owned frame interface for the track command stage. It calls original-owner scenery culling then the eModel selection slice. Vehicles, UI and full frame target ownership remain outside this overload. Its resident texture-chain projection follows00104DF0 first-use append order,00109358 order buckets and image-parent grouping. The parent pointer+8C corresponds to ImageParentHash+40; each later sibling is inserted immediately after the first chain, so sibling order reverses. Pertexture command order remains submission order. Runtime owns the persistent first-use chain list; the overload's null cache argument is a single-frame test adapter. Streaming page transfers/alternating upload batches are not emulated.

## B02: alpha40 and second packet

Retail VU instruction base `002A6960`, PC0C3..138=`002A6F78..002A7320`. Dispatcher tests40 at PC61 and calls0C3, kicks base geometry, then tests40 again and XGKICKs state320 and geometry. PC129..12C stores only RGBA.w. Original uploaded material vectors350..353 and light vectors35C..362 yield:

`p = affine_bit_power(max(dot(reflected,light_direction0),material0.z),material3.x,material3.y)`

`alpha = trunc(clamp(material3.z + material3.w*p,0,255))`.

The affine power is the existing scalar ITOF0/FTOI0 bit-pattern operation, not host pow(). Reflection is existing VU002A7F90; vectors must share original local coordinates. Other light RGB terms are calculated but only alpha is written by this routine. The all30 native asset scan (`local/task0051/material-alpha40-all-tracks.log`, player assets not redistributed) finds every40 surface using Road hash001C3745. GLOBALB registry parameters12..17=(8,1.600000023841858,1,1,20,128) produce M3=(8,-7452439552,20,108), M0.z=.1. At unit dot1 the actual affine-bit operation yields1.59997558594 and C3 alpha192; a normalized source-alpha clamp at128 is incorrect even with ideal unit vectors.

Retail CPU initializer0010AC50 uploads eight vectors to320. Packet writes TEX0_1 from original specular-falloff texture (fallback `0x660013300`), TEX1_1=`0x60`, CLAMP_1=0, MIPTBP1_1=0, MIPTBP2_1=0, ALPHA_1=`0x4000000058`; trailing padding is zero. It leaves TEST and ZBUF inherited. Debug eFixUpTables00103E40 resolves pTextureInfoSpecularFalloff with GetTextureInfo hash `0184D1F8`; Retail00103E68 independently resolves the same hash into uGpffff9de0 used by0010AC50. Do not substitute an arbitrary reflection texture.

ALPHA58 is `(Cs-0)*Ad/128+Cd`, using destination alpha. Consecutive fusion is not generally proven: different texture alpha can pass a second-pass alpha test where the first pass failed. The destination could therefore belong to earlier geometry. An exact adapter must preserve destination alpha (including values above128), TEST/depth outcomes and order, or report a concrete unsupported path. Native blend_channel is an unclamped floating channel reference; integer raster rounding and GS COLCLAMP are not claimed.

## B03: actual track evidence

Track11 W_RDWATER_MASK hash0CCA6E99 has nine strips4041; TUNNELROAD_MASK DB00D526 has six strips4041. They exercise40 even without80 or100. Both header ALPHA48; tunnel mask BiasLevel1, order4; water mask BiasLevel0, order3. Water underlays8BBF7D4E/79940F30 use ALPHA44/order2/depth1. These bindings follow strip hashes and header bytes, not names. Sampled mask packed vertex alpha bits are1; generic RGB5A1 alpha0/128 preservation is still necessary, but not established as the cause of these particular masks.

## B04: depth bias and sorting

Retail AttachAlphaPolys0010B838 and AttachSolidPolys0010BB20 select matrix packs by texture+75:1 selects1,2 selects2,other selects0. Both use +50 for depth-write policy and +4F/+74 for TEST selection. Debug alpha attachment has a different depth policy; it must not replace Retail behavior.

CreateViewMatricies Retail0010E638, Debug0010EA18 constructs three screen depth ranges. Retail002C9E20 and Debug002C14E0 contain identical 48 bytes:

`1000004000f00f4040000040f0ff0f40f00f0040f0ff0f4010000040000080404000004000208040000090400000f040`.

For selected pair(lo,hi), native positive camera z projects to `(-hi*n+lo*f)/(f-n) + f*n*(hi-lo)/((f-n)*z)`. This is original screen-depth float space, not a host normalized depth or world offset. SetupMatrixPacks Debug00105668 copies shared WorldView/WorldClip, selecting corresponding VUClipScreen. Host depth conversion requires the original VU output representation; screen_depth deliberately does not invent that conversion.

Expanded original VU packet decode (the older diagnostic stopped after1024 instructions): ordinary projection PC1DF..2C3 (`002A7860..002A7F88`) uses floating matrix/divide and SQI.xyz output, with no FTOI.z. Clip classification PC4CC..503 only writes packet control/w. Generated clipped vertices PC593..598 (`002A9620..002A9648`) likewise MULq.xyz then SQI. Sprite output PC197..19A FTOI4.xy explicitly excludes z. Native screen_depth_to_gs therefore retains float z bits, masking low24 for ZBufferMode31 (PSMZ24), all32 for30. normalized_gpu_depth is an explicitly port-owned double ratio; a float32 host depth attachment can lose raw low bits. Integer depth storage with explicit GEQUAL retains original integer comparison precision.

Retail0010CBF8 builds alpha depth from strip-entry AABB midpoint(+10,+20), local-world then camera-worldview, taking z. RadixSortAlphaKeys001051D8 sorts float bits, grouping RenderingOrder signed byte+29 clamped to0..7 (negative maps7); SortAlphaKeys001055B0 reverses the resulting array into linked-list traversal. Final order is ascending order bucket then descending depth. Equal nonnegative keys reverse input traversal; negative ties undergo the radix sign reversal then final reversal. Native helper preserves these distinctions including signed zero/raw bit keys. Input must be original traversal order.

## Checks and outstanding proof

Private standalone track_effects_tests passed GS0A/44/48/58, above-unity alpha, alpha sort ties/order, screen-depth endpoints and C3 upper clamp; ecstasye.cpp private syntax check passed. Shared builds are parent-owned. Existing audited body changed: hp2_tpk_texture.cpp parse_header only (+75 extraction); original decoder row behavior unchanged. New eModel overload/eDisplayFrame slice and new port effects functions require successor source-audit records. Header fixture test checks +75 separately. No original visual or numerical Verified claim.

Separate flag100 dispatch calls VU42B (`002A8AD8`), existing environment reference. Its runtime environment texture/matrix/second-context target selection is not established by40 evidence. Water interaction, animated textures and frame target effects require their own evidence; this document does not label them recovered.


## B05: per-draw context and bounded fusion

Retail eModel::IsLit0010CB00 / Debug0010CEF8 tests original eSolid StripFlags+32 &1C0. The shared geometry carrier now parses that original aggregate, not a guessed OR of decoded surfaces. The typed overload retains the original no-argument placeholder separately. Scenery supplies has_light_context; identity/world geometry uses the world context, a nonidentity model without IsLit receives no context and RenderStrips clears1C0.

Original SPECULARFALLOFF hash0184D1F8 resides in each TEXnnTRACK.BIN, not GLOBALB. Native source-pixel decoding finds all30 corresponding specular base images 256x512 with alpha exactly127 at every pixel. Packet320 TEX1=60 samples base level only.

For base TEST5000B (GEQUAL0), base alpha never fails, and identical depth/coverage proves second-pass Ad equals base post-modulation As. For TEST5030D, the same proof holds if every sampled base mip alpha is the same constant as every sampled specular alpha: unchanged vertex alpha and MODULATE/TCC make alpha-test outcomes identical. Native fusion_40 requires an explicit all-mip alpha range proof; it does not infer constancy from a header flag.

With baseALPHA48 and original COLCLAMP1 both increments are nonnegative: sequential saturation equals clamp(Cd+floor(Cs_base*As_base/128)+floor(Cs_spec*As_base/128),0,255). With baseALPHA0A it equals clamp(Cs_base+floor(Cs_spec*As_base/128),0,255). The first is an additive host operation, the second a copy operation. Native commands retain both original passes; this is a bounded adapter fusion. Final framebuffer alpha is specular post-texture As; ignoring it requires a frame-wide proof that no later unfused Ad reader exists. Native tests exhaust all256 destination values and representative source alpha0,1,127,128,192,255; they also reject varying-alpha5030D fusion.

Existing audited bodies changed for this task: hp2_tpk_texture.cpp parse_header (+75 BiasLevel,+40 ImageParentHash), port/assets/vehicle_geometry.cpp solid (+32 StripFlags). Header layout changes require a fresh common-library rebuild; stale libraries can misread adjacent fields. Shader snippets remain port adapters and do not claim PS2 filtering/raster equivalence.

All30 all-original-mip alpha range scan completed (`local/task0051/alpha40-all-mip-ranges.log`):18 unique state/range tuples, every raw strip4041, FIX0, ApplyAlphaSorting0, baseALPHA0A or48. All ATST5030D base textures have alpha127..127 over base and all original mip levels, matching SPECULARFALLOFF127..127. All other cases select ATST5000B. Consequently every shipped40 surface satisfies fusion_40's coverage predicate; no material-name or track-number special case is needed. Test coverage equality holds for constant alpha under filtering and wrapping too.

The fusion proof concerns original integer RGB units. Host sRGB conversion, linear framebuffer blending, filtering and raster interpolation remain separate port approximations. Copy fusion can perform both original byte-domain terms inside a fragment then convert once. Additive fusion still needs the same byte-domain destination semantics as preexisting ALPHA48; a linear host additive framebuffer is not original byte-equivalent. Do not promote these PC checks to original Verified rendering.

Host depth integration uses original pack coefficients(A,B), original pack0 coefficients(A0,B0) and actual host perspective coefficients(Ah,Bh): screen_z=A+B/view_z; q_eff=(screen_z-A0)/B0; host_depth=Ah+Bh*q_eff. Pack0 aligns existing car perspective depth, while selected bias pack preserves native screen-depth ordering without moving world geometry. This is a named host projection; original GS integer depth remains separately available via screen_depth_to_gs.

## B06: explicit environment dependency gap

Actual static scenery100 dependencies were separately audited: FIRETRUCK_00 has30 strips100 and aggregate4100, with four scenery LOD references per track11..16. XB_GASTRKTANK_L1A_00 has15 strips100 and aggregate4100, with one or two references per track21..26. TRACK_HELICOPTER has64 strips100/aggregateC180 but no scenery LOD references in any of30 tracks. Logs `local/task0051/raw100-all-tracks.txt` and `raw100-scenery-references.txt`.

Debug EnableEnvMap00326790 and EnvironmentMappingEnable003276F4 both initialize1. Scenery caller flags0/4 do not disable100. Thus environment cannot be declared disabled as an original rule. These effects are an explicit remaining renderer dependency gap; base-pass display is separately identified, without binding an invented texture.

The original dependency is six-view capture: Debug eDisplayFrame00112C60 calls UpdateEnvironmentMapCameras00123D20, culls/stuffs views4..9, attaches their solid geometry, then eCreateEnvMapPrims00118BD8 composites into target0E. eEnvMap::UpdateCameras001179C0 creates look-at-derived orientation and hack_man_matrix, chooses alternating EnableEnvMapFaces[eFrameCounter&1], near.5, FOV75E4 and global far range. CreateEnvMap00118518 uses curved ReflectionSamples grids for five faces; CreateBackEnvMap00118860 maps backface2 through four BackReflectionSamples grids. A generic cube map or a static texture would not establish original equivalence. Production extension of this separate capture is outside the approved mask/tunnel closure; retain the gap explicitly.

Dispatcher100 chooses packet318 or31C depending effective4, not320 or328. Both packets use context2 TEX1=61, TEX0=620213000 normally (PSMCT16 256x256 target); ALPHA default1000000048, special display mode4000000048. Packet31C can use dynamic global blend fields. Packet328 instead uses globaltextureB9AB56E4; it is not evidence for the100 target. pass_sequence preserves this unresolved-state distinction and does not invent environment alpha or texture.

Normal target initializer Retail001064C8 sets all five initialized ZBufferMode+38 fields31. Race depth coefficients therefore select the Z24 row explicitly. All30 TRACKB surface texture references use only Bias0/1, zeroBias2 (`local/task0051/used-bias-all-tracks.txt`); shared-geometry references require the same guard. Stable host inverse projection uses q=1/z+((Ap-A0)+(Bp-B0)/z)/B0 to avoid cancellation near screen-z2; raw GS output remains a separate helper.

## B07: plain source-over alpha bound and freeze validation

Independent all30 native audit (`local/task0051/alpha44_probe.cpp/.log`) inspected23690 non40 ALPHA44 strip occurrences across all loaded geometry/LODs. There are zero lit80 surfaces with effective4 (raw flag4 or texture GlossMapInAlpha+74). The maximum texture alpha over every base and original mip is127; zero alpha-bound failures. Prelit packed V4_5 alpha is0/128. Lit80 without4 uses ambient.w plus directional alpha contributions: elSetupLightContext initializes output to zero, writes only light RGB and sets ambient.w128 unconditionally, so [3,7,11,15]=[0,0,0,128] independent of instance transform/query. The audit also checks world and representative nonidentity contexts per track; zero failures. Hence all audited ALPHA44 post-texture source alpha is at most127, even though40 Road alpha can reach192. The source-over bound is specific to this observed route and does not justify clamping other materials or future data.

Plain ALPHA48 needs no destination alpha: precompute floor(Cs*As/128) in GS units and use host additive coefficient1, preserving source alpha above128. The host framebuffer color-space/raster caveats still apply. Core helpers retain original raw ALPHA registers and separate additive48 from destination-alpha58.

Final private checks: track_effects_tests passed after original Road192 regression, rawZ24 conversion, stable pack0-aligned host depth, copy/add fusion gates and exhaustive representative-destination fusion comparisons; track_texture_tests passed4/8/32 base+mips with BiasLevel+75 and ImageParentHash+40 parser checks; ecstasye.cpp and vehicle_geometry.cpp standalone syntax checks passed. No shared build was performed by this worker. Native frame/render-interface integration, shared tests and actual window verification remain parent-owned. No hardware Verified label is assigned.

The actual geometry reader always invokes original eStripDataE::GetElementRGBA and marks has_color=true for every decoded vertex. A missing-color default is not a recovered asset branch; real-track upload must reject it rather than assume alpha255 or128. For prelit data, VIF V4_5 expansion retains RGB five-bit components shifted3 and W bit15 expanded to0/128 (existing E0033 packed-alpha evidence). Uploading alpha128 for every colored vertex discards real bit15=0 values. The revised alpha44 proof assumes the correct0/128 upload, not a forced255 fallback.

Owner audit closure: ScenerySectionHeader::DrawAScenery constructs the original instance/facade matrix; GrandSceneryCullInfo::StuffScenery selects per-model light context and submits one model draw; eModel::Render (Retail0010DC80/Debug0010E048) expands supported static nonanimated model surfaces, dispatches eModel::RenderStrips and records raw submission ordinal/depth. eDisplayFrame then owns attachment ordering. The Render slice is not an empty forwarding alias. Original animation/replacement and full DMA ownership remain outside the represented static branch. Eight paired function records document these typed slices under the two binary track directories.

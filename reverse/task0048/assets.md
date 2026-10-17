# E-0048-A01..A03: Retail track asset adapter

Status: observed asset bytes and read-only static analysis; not original-runtime verified.
Retail SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Debug SHA-256: 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Raw read-only MCP captures are ignored under local/task0048.

## E-0048-A01: catalogue and bundle framing

Retail 00153608 accepts chunk 34201, stores payload and divides size by A4.
Retail 00153420 accepts one-based indices and rejects rows whose Name is empty.
The loader clears row 54. GLOBAL/GLOBALB.BUN contains 66 A4-byte rows, 30 named
rows (11..16,21..26,31..36,41..46,61..66); all named rows carry TrackNumber at41
equal to their one-based row. Names, Location and time at44 match the independently
inspected Debug TrackInfo prefix; the full Retail structure is not reinterpreted
as the different A0-byte Debug type. The adapter keeps field reads explicit.

Existing E-0038-TM01 records Debug BeginGameFlowLoadingTrackSpecific00168540
TRACKA/TRACKB file strings and CalculateTrackNumberToLoad0019B6E8's SameAs/time
selection. The catalogue exposes each actual time-specific record directly, so
resource_number uses its TrackNumber rather than SameAsTrackNumber. The named
Retail records have corresponding original TEX<number>LOCATION/TRACK packages.
Every shipped TRACKA file is present with zero bytes. TRACKB files are COMP
framed, version1001; the bounded decoder translates the existing E-0047-002
Debug LZDecompress001B1658 interpretation and checks expected output size.
All30 selected location/track archive pairs combined with GLOBALB,INGAMEB,DYNTEX
have no duplicate texture hashes (complete original table scan). Therefore fixed
host scan order does not select among conflicting loaded definitions in this
asset set. The adapter rejects duplicate hashes rather than inventing priority.

All 30 original TRACKB bundles passed existing geometry packet decoding and both
track texture archive decoding in the initial standalone integration sweep.
That sweep predates added scenery/shared-archive checks and is not evidence of
their successful validation. Track11 has1104 solids and352 decoded track textures.

## E-0048-A02: scenery instance projection and explicit version adapter

Debug LoaderScenery0014E920 establishes section80034100, info34102 stride28 hex,
instance34103 aligned16 and stride30 hex. Imported Debug SceneryInfo has three
NameHash values at0, facade bytes20 hex and Radius24 hex. SceneryInstance has
info indexC, flagsE, position10, nine signed rotation halfwords1C.
DrawAScenery0014FA68 converts each rotation halfword with1/16384 and position
directly; flag200 passes a null matrix. Asset sections independently satisfy
those same strides and index bounds. The adapter reads each field rather than
casting either PS2 layout to a host structure. This is a bounded cross-version
adapter; full Retail function correspondence is not established by the Debug
type or by these byte checks.

The viewer chooses the first nonzero model hash (inspection policy), retaining
flags and facade metadata. Original camera LOD/culling and facade billboarding
are not claimed. Solid transform metadata is not the instance placement.
Road sections often use world-space vertices with identity/null instances.
Track11 contains8407 instances; six reference four absent hashes D35FF1A6,
907921ED,C360AF35,94E9AAFB. None match GLOBAL/INGAMEB.BUN solid hashes. Missing
models and textures remain explicit diagnostic gaps, never replacement meshes.

## E-0048-A03: track texture state and missing mip closure

Retail00108CC0 emits TEX1 from header6E MXL shifted2,6F MMAG shifted5,70 MMIN
shifted6,71 L shifted19,72 signed K shifted32. It also emits MIPTBP pointers from
56/5A/5E/62/66/6A and widths58/5C/60/64/68/6C. Track11 location records use
MXL0..6, MMAG1, MMIN3, L0, K0. The car adapter's MXL0 restriction therefore
cannot gate a track inspection viewer.

The track state adapter preserves actual TEX1 and marks nonzero MXL as requiring
base-level inspection. No generated mipmap substitutes for original level data.
Original image_size includes lower levels:256x256 PSMT4 MXL3 has43520 bytes,
equal to32768+8192+2048+512;128x128 MXL4 has10912 bytes. GS pointer alignment
differs from those concatenated byte offsets at small levels, so decoding them
requires the original upload layout.00109510 queues a transfer; its producer is
not enough to establish the transfer consumer's mip packing. This remains open.

Packed vertex RGB follows the existing VU evidence: five bits per channel times8,
alpha128. Strip flag80 requires the separate original world lighting/material
chain; prelit color alone is not a replacement for it. Debug world setup00107758
and light loader00115348 were captured for continuation, not implemented here.

All60 original TEX files contain only ALPHA0A(copy),44(source-over),48(additive),
with FIX0.48 selects(Cs-0)*As/128+Cd. The adapter exposes additive independently
of the inherited source-over boolean; the host must choose additive blending.
Across those files MMIN also has4/5,L1,K4016/K4; raw TEX1 remains retained.

## E-0048-A04: resolved original mip source packing

The earlier A03 upload-packing gap was resolved by a focused read-only query of
Debug eTextureDownloader::FillVramWithTextures001097A0, retained at
local/task0048/0x001097a0.json. It initializes source from ImageData, loops
NumMipMapLevels+1, uses BaseImageSize>>(level*2) bytes, shifts Width/Height by
level, and advances the source by that byte count. ImageInfos[level] supplies
each level's TextureBufferWidth and Is32BitSwizzelled; the latter changes transfer
mode to PSMCT32 and width/height scale. Source bytes are consecutive, while GS
destination pointers may include alignment gaps. The preceding hypothesis that
GS offsets might directly index source bytes is retired.

Imported Debug TextureInfo is A4 bytes and places BaseImageSize44, ImageInfos52
(seven4-byte entries), NumMipMapLevels6E. TextureImageInfo has signed pointer
offset0, buffer width2, swizzle3. These exact imported types and the consumer
capture are retained separately under local/task0048. Retail emitter00108CC0's
MIPTBP loads independently confirm the same descriptor offsets. The new host
adapter explicitly reads these fields and invokes the established indexed decoder
on each original source slice, reducing width/height/shifts and selecting that
level's swizzle bit. It returns levels1..MXL separately, with original raw GS alpha.
No generated downsampling or fabricated tail levels are supplied. Bitmaps remain
external runtime data and are never checked in.

This recovers original mip source pixels through the established decoder, not
PS2 GS filtering/LOD hardware equivalence. Cross-version structure evidence is
bounded to these fields and does not imply that unrelated Debug/Retail layouts
are interchangeable. Tests must validate all30 decoded chains after this change;
the earlier base-only sweep does not count as that validation.

## E-0048-A05: facade matrix

Original Debug0014F9F8 and Retail0014FDA8 both compute bATan(object.x-camera.x,
object.y-camera.y), rotate original identity around Z, copy object position and
force matrix.w1. Implementation is in the original scenery.cpp free function,
using existing recovered bATan/eRotateZ and an explicit Debug math image argument.
The known Debug/Retail bATan near-zero difference is an inspection adapter limit.
No host atan2 or engine billboard replaces the original chain. Per-function
record track/0014f9f8.md retains signature and version separation.

## E-0048-A06: original world-identity light context

Source ownership was independently extracted from the authorized Debug ECOFF
using the existing recover_debug_source_ownership.extract routine and identity
checks. lighting-source-ownership.json records exact source markers and offsets:
vAABB/vAABBTree methods belong to indep/src/volumetree.cpp; QueryLightDatabase
and elSetupLightContext belong to src/ecstasy/elighte.cpp. Numerical functions
are restored there; port::track retains only byte validation and host ownership.
Existing Retail no-argument vAABB throwing entries are retained as separate
placeholders; a typed Debug Contains overload does not silently replace them.

Debug eView::SetupWorldLightContext00107758 calls elSetupLightContext with the
identity world matrix. The query location is therefore the world origin, not
each scenery instance. Full independent byte scans of all30 supplied bundles
found one containing global ambient at origin in every case, and no point light.
The parser still retains all nodes/lights and validates every stored reference,
node count and graph cycle before querying the original first-containing-leaf
order. Volume containment uses strict boundaries. eLight size60 hex and all used
field offsets are corroborated by the imported type and original consumers.

Retail00115978 independently corroborates the in-game directional/global-ambient
branch of Debug00115D88. Retail global stores00117230..264 initialize direction
from GP003358F0-7EFC/-7EF8 (bitsBF3D70A4,3F0CCCCD), Y0, colour RGB80.
Retail static distance0032FA40 is bits461C4000 (10000). These are the same
defaults as Debug00117490 and are independent of track/time SunPosition, which
is separately loaded by Debug LoaderSun001250B0 from chunk34202. The three
direction components are multiplied by distance and normalized in original
operation order. AmbientColourIG is initialized zero; zero query ambient uses
the original RGB85 fallback. The two supported queried point lights preserve the
original zero blue channel, rather than silently correcting it.

Original light query uses squared Size/FarStart attenuation and nearest base
ambient, with explicit additive influence weighting and ten-light limits. The
largest-finite nearest-distance initializer was read at Debug00326ACC
(bits7F7FFFFF). The host context exports the28 VU colour/direction scalars needed
by the existing recovered material shader. Materials use the original GLOBALB
3401E registry and its evidenced Default fallback via the established reader.

This is the bounded initialized in-game world-identity context. Frontend light
selection, runtime debug overrides, arbitrary per-object context setup and
camera environment-map matrices remain outside this interface. Normalization
uses the existing finite scalar host convention; VU exceptional arithmetic and
original-runtime numerical equivalence remain unverified.

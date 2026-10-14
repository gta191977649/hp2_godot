# Vehicle static GS texture state — TASK-0033

Evidence E-0033-T06, 2026-10-14. Target SLUS_203.62 SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`;
executable revision unknown. Original C++ class and function identifiers unknown.
This is a static control/state slice, not a complete renderer or Verified behavior.

## Original state generator: 00108CC0

Read-only Ghidra decompile retained under ignored `local/task0033/00108CC0-alpha.c`.
The function emits six packed-GIF A+D register writes followed by texture-coordinate
data. The texture record is the 0xA4-byte record used by the existing TPK loader.

| Register | Original construction / source bytes |
|---|---|
| TEX0_1, 0x06 | TCC bit34 forced to 1; TFX = signed byte record+0x4E shifted35 |
| TEX1_1, 0x14 | MXL=+0x6E, MMAG=+0x6F, MMIN=+0x70, L=+0x71, signed16 K=+0x72 |
| ALPHA_1, 0x42 | signed byte +0x76 in low bits; signed byte +0x77 shifted32 |
| CLAMP_1, 0x08 | WMS = (+0x4C XOR1)&1; WMT = ((+0x4C>>1) XOR1)&1 |

The CLAMP construction above applies when `iGpffff9edc` is zero. A nonzero
global selects `uGpffff9ee0` instead; that global override is not instantiated
by this viewer's scoped texture-state service and is not silently emulated.

Original 0010DED0 is `lb s4,0x4d(a1)`. The signed byte zero/nonzero condition
partitions immediate/deferred material queues, also seen in 00104DF0. It is
independent of byte +0x4F. `Field_0x4c`, `Field_0x4d`, `Field_0x6e` through
`Field_0x72` preserve the raw fields without inventing original field names.

## Original asset observations

Archives and hashes are recorded in `vehicle-textures-task0033.md`.

- All 783 car textures have TFX0, MXL0, MMAG1, MMIN3, L0, K0, and +0x4C=0.
- 115 of 116 shared GLOBAL textures have the same TEX1 values; one unrelated
  entry has MXL2. All shared entries have +0x4C=0 and TFX0.
- Thus the relevant static resources clamp both axes, magnify with linear
  filtering, and minify with nearest filtering at their sole mip level.
- Car ALPHA values are 0x0A or 0x44, FIX0. GLOBAL also contains 0x48 entries.
- `WINDOW` uses +0x4D=1, ALPHA0x44, FIX0. `CARBOTTOM` and the three families'
  body skins, static tires and off-state lights use +0x4D=0 and ALPHA0x0A.
- The maximum raw palette alpha of every indexed ALPHA0x44 entry is <=127 in these two
  archives. WINDOW's maximum is 127. This bounds texture alpha only; it does
  not independently bound the VU-generated vertex alpha or the final product.

The GS architectural MODULATE function with TCC1 multiplies texture and vertex
RGB and alpha in GS integer units with denominator128. ALPHA register0x0A
selects A=zero,B=zero,D=Cs, giving direct source RGB. Register0x44 selects
A=Cs,B=Cd,C=As,D=Cd, giving `((Cs-Cd)*As >> 7)+Cd`. Register0x48 gives
`Cs*As/128+Cd` and is explicitly unsupported by the static helper, not replaced
by source-over. The operand selectors are architectural decoding, while the
actual register values and their material binding come from original bytes.

## TEST and depth-write state

Original 001064C8 initializes normal player/framebuffer view targets with
view+0x40=0 (ZMSK), +0x44=2 (ZTST), +0x48=6 (ATST).
Original 00105AC0 constructs four state packets referenced at view+0x7C..0x88.
0010BB20 and 0010B838 derive state-index bit0 from texture byte+0x4F OR byte+0x74
being nonzero. Bit1 comes from byte+0x50 and can be masked off by global state.
It selects depth writes independently of bit0 (E-0033-T07 correction below).

| State bit0 | TEST_1 | Alpha test | Z test |
|---|---|---|---|
| 0 | 0x5030D | alpha > 0x30 | GEQUAL |
| 1 | 0x5000B | alpha >= 0 | GEQUAL |

The exact original construction is
`TEST0=(view+0x48 <<1) | (view+0x44 <<17) | 0x10301`,
`TEST1=(view+0x44 <<17) | 0x1000B`.
ATE=1, AFAIL=KEEP, DATE=0 and ZTE=1 in both. Architectural ATST6 is GREATER;
ATST5 is GEQUAL. The original greater-or-equal Z convention must not be copied
blindly onto a host projection with a different depth mapping.

The +0x4D sorting partition does not determine depth writes. In particular a
body skin with +0x74=1 can select the second TEST state despite sorting into
the immediate queue. The helper describes these normal-target defaults only;
environment-map and modified runtime target states are outside its scope.

The initial E-0033-T06 table incorrectly assigned depth writes to bit0=0 and
masked them for bit0=1. This interpretation is retired, not retained as a
candidate: E-0033-T07 follows the actual packet pointers and establishes bit1.

## Global raster state: 001085C0

The original frame setup writes COLCLAMP register0x46=1, DITHER0x45=0,
PABE0x49=0 and FBA1/FBA2 registers0x4A/0x4B=0. RGB results therefore use color
clamping in that frame context. This does not erase intermediate fixed-point
rounding or turn a modern GPU into a bit-exact GS implementation.

## Native implementation and alpha preservation

`src/port/vehicle/texture_state.hpp` contains a port-side projection of the
supported static state, exposing copy/source-over, partition, raw ALPHA/TEX1,
filtering, wrap and normal-target TEST/depth predicates. Unsupported TFX,
ALPHA/FIX and nonzero mip/LOD configurations throw address-bearing
`hp2::NotImplemented` at 00108CC0. Its integer RGB blend reference retains the
GS denominator128 and arithmetic-shift rounding and returns an unclamped value.

The existing frontend decoder expands palette alpha for ordinary RGBA display.
Recovering GS alpha from that expanded byte can lose information, so
`Hp2TpkArchive::decode` and `decode_by_name` gain optional
`preserve_gs_alpha=false`. The default branch is unchanged. Vehicle textures
pass true and retain each original palette alpha byte in the bitmap alpha
channel. RGB decode, palette swizzle and mesh-facing row order are unchanged.

Native fixture tests cover raw +0x4D, CLAMP/TEX1, unsupported states, TEST alpha
thresholds, depth contexts, RGB blending at alpha0/64/128/255, negative blend
rounding, raw palette alpha0/64/128/255 and unchanged frontend alpha expansion.
Configured WINDOW and CARBOTTOM assert their original equations/partition.
`vehicle_textures` passed after these changes (0.63s). The existing
`hp2_tpk_texture` executable reported SKIPPED because its separately generated
Python-reference file/environment were absent; its CTest success is not claimed
as a fresh cross-reference comparison. The new synthetic default-alpha checks ran.

## Architecture references and remaining limits

GS register bitfield names and standard operators are architectural decoding
aids from the primary PCSX2 implementation, not alternate HP2 behavior:
[GSRegs](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/GSRegs.h),
[GSState](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/GSState.cpp).
Original Ghidra captures for the state functions are ignored under
`local/task0033/*-alpha.c`; no third-party runtime code is incorporated.

Host derivative-based min/mag selection is a presentation adapter; the original
automatic GS Q/LOD computation is not thereby reproduced. Filtering/subpixel
precision, integer framebuffer conversion and comparison with PS2/PCSX2 remain
unverified. The helper does not clamp source-over alpha to1: final As>128 must
be handled explicitly by the host or rejected rather than silently approximated.

## Complete supported static alpha-range audit

The final archive-wide pass examined all899 original texture headers:783 car
and116 GLOBAL. Its raw `(ALPHA selectors, byte+0x74)` counts are:

| ALPHA | Byte+0x74 | Header count |
|---|---|---|
| 0x0A | 0 | 324 |
| 0x0A | 1 | 398 |
| 0x44 | 0 | 173 |
| 0x48 | 0 | 4 |

All173 source-over headers have byte+0x74=0 and FIX0. There is no header
counterexample. Car textures contribute124 of these and GLOBAL contributes49.
One car entry is the excluded 32-bit/no-CLUT `DASHWINDOW`, whose pixels are not
decoded by the vehicle service. It is not assigned an artificial alpha bound
from an absent palette and cannot be among successful static exterior loads.

For the172 indexed source-over entries, read the original CLUT bytes at
`aligned_data_base + palette_placement + 3`, stride4, for exactly palette_size
bytes. This bounds every possible decoded pixel, including unused CLUT colors.
Car entries have maximum alpha109 (42 textures) or127 (81 textures). GLOBAL
entry maxima are127 (40),64 (2),95 (2), and one each of73,85,87,107,126.
Thus every supported source-over texture has raw GS Ta<=127. No expanded
frontend alpha, guessed reverse conversion or image-export step enters this audit.

The independent full catalog geometry inventory found bit4 clear for all228
successfully loaded static car/skin associations; see
`reverse/structures/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/base_material.md`.
Together with source-over header+0x74=0, the initialized frontend lighting
equation produces Va128 on the lit route for every camera/normal. The unlit
packed-alpha route produces0 or128. Therefore supported static exterior
source-over bindings satisfy `As=floor(Ta*Va/128)<=127`, independently of camera.

The generic algebraic flag4 material branch can produce As>128 on other inputs;
that remains an explicit unsupported backend case. It is not a missing branch
required by this measured static exterior corpus. This range result does not
claim host/GS fixed-point rounding, depth, filtering or framebuffer equivalence.

## E-0033-T07 — corrected state-index/depth-write projection

The GPU inspection exposed dark interior triangles drawing through paint.
Re-reading original00105AC0 found an error in the initial native projection:
the decompile constructs state2/3 before state0/1. Following construction order
without following the actual object pointers confused TEST bit0 with ZMASK bit1.
The old claim is retired above and corrected in the function record.

At function entry the original assigns packet pointers:
view+0x7C=allocation+0x80, +0x80=allocation+0x160,
+0x84=allocation+0x240, +0x88=allocation+0x320.
It first builds packets through view+0x84/+0x88, using
`uVar24=ZBUF_base_and_PSM|(view+0x40 <<32)` for context1.
After ORing bit32 into `uVar22`, it builds view+0x7C/+0x80 using that forced
mask value instead. The constructor's assignments, not their order in the
decompile, establish the corrected state table:

| State index | View pointer | TEST, normal defaults | Context1 ZMSK | Context2 ZMSK |
|---|---|---|---|---|
| 0 | +0x7C | 0x5030D | 1 | 1 |
| 1 | +0x80 | 0x5000B | 1 | 1 |
| 2 | +0x84 | 0x5030D | view+0x40, normally0 | 1 |
| 3 | +0x88 | 0x5000B | view+0x40, normally0 | 1 |

0010BB20/0010B838 compute index bit1 as2 when raw texture byte+0x50 is
nonzero. They clear that bit when getter001033C0 returns zero, and retain it
when nonzero. The getter returns `uGpffff9e88`; setter001033C8 replaces it
with a boolean argument and returns the previous value. Original frame
function00112858 explicitly toggles it between0 and1 for different phases;
the late frontend rendering loops re-enable1 after the depth-disabled2D work.
This is a phase gate, not a field+0x74 test and not a reason to fabricate a
new depth prepass.

All899 original headers use raw byte+0x50 as0 or1. The722 copy-equation
headers (ALPHA0x0A) have byte+0x50=1. The173 source-over0x44 and4 additive0x48
headers have byte+0x50=0. In particular `911TURBO_SKIN1` has both+0x74=1 and
+0x50=1: it selects TEST1 with depth writes enabled in context1 when the
original phase gate is enabled. WINDOW keeps depth writes masked.

`TpkTextureHeader::Field_0x50` now preserves the raw byte. The native helper
exposes `depth_route_enabled` explicitly; its viewer default selects the
original depth-enabled normal route. It computes writes from+0x50, the phase
gate and context independently of TEST selection. This does not claim the
entire original frame scheduler or all target state transitions are implemented.

Native tests exercise all four TEST/depth combinations, the global disabled
route, context2 masking, exact preservation of raw0x81/zero, and authentic paint,
CARBOTTOM and WINDOW states. `vehicle_textures` passed in0.62s after the fix.
The GPU capture/visual regression is owned by the bridge integration checks.

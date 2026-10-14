# Vehicle VU ADC projection — TASK-0033

Evidence E-0033-T03, 2026-10-14. Original SLUS_203.62 SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`;
executable revision unknown. Status: recovered control slice, not Verified.

## Original-byte observations

Read-only Ghidra `memory/read`, hexadecimal output, captured `.vutext` at
002A5910 for 20,624 bytes in chunks of at most 4,096 bytes. Retained under
ignored `local/task0033/vutext.json`. The DMA/VIF packet at 002A6940 contains
MPG uploads; VU PC 000 starts at ELF 002A6960. Subsequent upload commands
interrupt the otherwise sequential address mapping.

The lower instruction control slice at VU PC 1AF (ELF 002A76E0) is not an
original EE/C++ function. Original names and ABI are unknown. The native
`port::vehicle::adc_002A76E0` is explicitly a scalar port-side projection.

| VU PC | ELF location | Observed operation |
|---|---|---|
| 03D | 002A6B48 | XTOP selects the packet base in VI12 |
| 046–047 | 002A6B90–002A6B98 | Calls 1AF; delay slot chooses renderer entry 24C in VI5 |
| 1B0–1B2 | 002A76E8–002A76F8 | Loads header x/y into VI1/VI2 and control xyzw into VF23 |
| 1BE | 002A7758 | Doubles VI2 (the mode byte) |
| 1C3–1C6 | 002A7780–002A7798 | Jumps to 1DD-2*mode, delay slot RINIT VF23.x |
| 1C7–1DD | 002A77A0–002A7850 | Unrolled renderer calls, final tail jump |
| 1D0 / 1D8 | 002A77E8 / 002A7828 | Reseeds R from VF23.z / VF23.w in call delay slots |
| 259,264,26C,... | 002A7C38,002A7C90,002A7CD0,... | RNEXT writes only VF16..19.w |
| 26F,279,283,28D,... | 002A7CE8,002A7D38,002A7D88,002A7DD8,... | SQI copies the corresponding output vertex to GIF memory |

The renderer transforms xyz while preserving the generated w at its output.
The initial renderer path 1DF and the textured path 24C produce the same ADC
source/iteration sequence. Pipeline output contains an extra trailing vertex;
the uploaded header count limits consumed vertices. The native helper returns
exactly that count, and does not emulate projection, clipping or rasterization.

## Symbolic provenance and scalar projection

Track each VF16..19.w as `(R seed component, RNEXT iteration)` through the
original lower instructions, delay slots, renderer entry changes and SQI stores.
No geometry, pixel color or outside game's behavior is used to derive this table.

| Mode | Output provenance up to header count |
|---|---|
| 0,1,2 | VF23.x seed, iterations 1 through count |
| 3 | VF23.w seed, iterations 1 through count |
| 4 | VF23.x iterations 1..7, then VF23.w iterations 1..13 |
| 5 | VF23.x iterations 1..11, then VF23.w iterations 1..13 |
| 6 | VF23.x iterations 1..15, then VF23.w iterations 1..13 |
| 7 | VF23.z iterations 1..19, then VF23.w iterations 1..13 |

Short final groups are limited by count; mode must equal `(count-1)/4` with
count in 1..32. The w reseed happens before all earlier results reach SQI;
thus the second segment has 13 vertices, not the plausible but incorrect 16.

RINIT keeps the seed's low 23 bits and fixes exponent 0x3F800000. Each RNEXT
advances its LFSR using old bits 4 XOR 22 as feedback into bit zero. Output
word w bit 15 is the packed-GIF XYZ2/XYZF2 ADC draw-disable bit. Exponent bits
therefore do not affect this projection.

For the original 4DR_SEDAN first three packets, the emitted ADC bit strings are:

| Packet offset | Count/mode | ADC bits | Drawing triangles |
|---|---|---|---|
| 0xC00 | 16 / 3 | `1100110011001100` | 8 |
| 0xDC0 | 8 / 1 | `11001100` | 4 |
| 0xEC0 | 28 / 6 | `1100110011001100110011000000` | 16 |

These counts agree with each strip entry's original byte +0x1E. Wider original
container checks are recorded by the geometry tests, not assumed from these samples.

## Architecture decoding aids and boundary

Instruction decoding is an analysis tool, not HP2 evidence from a third-party
reconstruction. Opcode tables and standard RINIT/RNEXT / packed GIF / strip
queue semantics were checked against the primary PCSX2 implementation:
[VU instruction tables](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/DebugTools/DisVUmicro.h),
[VU architectural operations](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/VUops.cpp),
[packed GIF registers](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/GSRegs.h),
[GS primitive assembly](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/GSState.cpp).
These inform how to read original instructions and registers; every HP2 seed,
branch, schedule and output-register attribution above comes from the original
Ghidra bytes. Downloaded architectural reference files and the probe are ignored
under `local/task0033`; none is incorporated into runtime source.

Packed-GIF ADC suppresses drawing while keeping the incoming vertex in the
strip's chronological queue. An enabled incoming vertex i>=2 therefore draws
the chronological triple i-2,i-1,i. The GS does not require a host-facing
triangle-strip winding alternation to choose these three vertices. Original
backface policy remains separate; the debug viewer must not infer it from ADC.

The original GIF primitive is independently anchored by EE function 0010AC50
(read-only Ghidra decompile retained as `0010AC50-texture-constants.json`).
It uploads seven constant vectors to VU address 294 using VIF word 0x6C070294.
The first GIF tag uses primitive value 4, ORed with 0x48 and optionally 0x10,
shifted into word y at bit 15 and combined with 0x30004000. Its register list
is 0x412. VU PC 1AF loads address 294.yzw, and PC 1B9 writes these fields into
the output tag while x receives header count/EOP. Primitive value 4 is the
GS triangle strip, independently corroborating consecutive triple assembly.
The EE initializer also has a conditional value-2 line-strip path controlled
by `iGpffff9e38`; the global's original meaning remains unknown. This slice
describes the value-4 mesh path and does not silently implement that alternate
path as triangles.

The user's other extractor provided an initial lead to this feature; its mask
formula and its reset-face assembly are not used as original-game evidence or
copied into runtime code.

## Limits and reproduction

The ignored `vu_probe.py` reads only the retained Ghidra memory result, parses
the original MPG uploads, decodes architecture instructions and traces the
scalar register provenance. Run `py -3 local/task0033/vu_probe.py adc` to inspect
all eight modes for both renderer paths. This is static symbolic data-flow
inspection, not a run of the original game or a validated general VU emulator.
Lighting, material blending, clipping and culling remain outside this slice.

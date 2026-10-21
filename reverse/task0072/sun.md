# E-0072-S01 - Sun counter transport

## Confirmed binary evidence

Read-only Ghidra rereads: Retail SLUS_203.62 StuffSun001252B0, SHA256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2,
revision unknown; Debug NFSHP2.ELF StuffSun00125430, SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b,
revision unknown. Raw decompilation/listing/references/call graph are ignored at
local/task0072/ghidra/{SLUS_203.62-001252b0,NFSHP2.ELF-00125430}.json.

The previous transfer counts clipped32x32 24-bit depths strictly below2. Coverage
is count*(1/1024). For each nonzero-intensity layer, alpha is negative intensity
times -128, or coverage*intensity*128; truncate to integer and retain its low byte.
Count changes alpha, not quad geometry or submission eligibility. Preserve layer
order, screen gates, original512x448 projection and scheduling after consumption.
The recovered CPU StuffSun implementation is unchanged.

## Port implementation and checks

An independently switched GPU adapter reduces the same original depth-proxy
samples into2x2 RGBA8 (R low byte, G high byte). Two count textures preserve the
previous completed transfer; an initial zero texture supplies original count0.
Advance only when a scheduled producer completes frame_post_draw. Offscreen and
disabled updates retain the last completed texture; clear/reload restarts at0.
The production vertex shader uses byte-valued alpha from sun_counter.gdshaderinc.
Original source-over eligibility is checked for all1025 counts; an unsupported
host blend domain retains the reference CPU route, with a diagnostic reason.

Normal GPU mode performs no CPU texture mapping. Optional profiler asynchronous
readback is diagnostic only, with producer frame, age, renderer identity, globally
unique Sun lifetime generation and texture identity. It never supplies gameplay.
--track-sun-gpu-compare=true deliberately blocks to compare the32x32 reference;
exclude it from performance measurements. CPU-unavailable counts are null.

Vulkan Forward+ on Intel Arc B390, Godot4.7.2 ed1daf0bf: shared production shader
passed16467 synthetic GPU checks (all1025 counts,16 intensity values, clipping,
thresholds and fixture buffer scheduling). Production fixture passed30 frames
and720 exact checks: native packet digest, colour, native-depth and final raw
image hashes, including first frame, movement to original road occlusion,
offscreen, disable/resume and clear/reload. Private captures and manifests are in
local/task0072/parity. These are port tests, not original-runtime verification.

## Unresolved

No PS2/PCSX2 runtime comparison was performed. The inherited TASK0071 per-draw
COLCLAMP and reflection precision gaps remain. Timing telemetry labels producer
submission frames; asynchronous CPU availability occurs later. No claim of a
whole-frame GPU time is made by adding viewport timestamps.

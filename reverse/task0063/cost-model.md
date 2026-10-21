# E0063R01 - Original rendering versus host transport costs

Authority: Retail SLUS_203.62 SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
Debug NFSHP2.ELF names/layout corroboration SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
This is reconstructed behavior, not a matched-console verification.

## Established observations

E0058R01 records the original zone/section/tree/instance/projected-size selection,
including the <2 pixel cutoff, first qualifying original model slot and logical
ScreenWidth512 focal input. Keep this native selection for each capture view.
Show LOD is an explicit diagnostic bypass, not a performance configuration.

E-0052-P01 records Retail00118A60 depth-to-alpha,00119308 fog palette and00119768
fog attachment. Fresh read-only Debug00118E28/00119B30/0011A338 decompilation in
this chat corroborated existing ZBufferAddress/FrameAddress transfers, DrawFog
and palette use. No Ghidra writes, function renames or saved analysis edits.
Original fog reuses GS depth; host encoded-depth geometry rerender is an adapter
cost, not an original extra world pass. A1080p target has9.04 times the pixel
area of512x448; this is an area ratio, not a measured FPS ratio.

E-0052-P02 describes the original half-size Focus intermediate and two jitter
source draws. Host fused arithmetic currently repeats source evaluations. A new
RGBA8 intermediate is not automatically equivalent: see E0063H01 counterexample.

E-0052-E07 records bounded original AcidFX pools and emitter distance, clip and
projected-radius gates. Original update/RNG order remains in acidfxe.cpp; host
transaction copies and Godot upload/setter traffic are separate costs.

Original Sun consumes the previous completed32x32 transfer, clipping the count
rectangle and counting 24-bit samples<2. The optional host reduction preserves
this completion cadence and predicate; it is not a new original-game algorithm.

## Hypotheses to measure

Props toggle affects native-selected draw preparation, main/depth/capture work
and transparent coverage. Its FPS delta does not identify a single bottleneck.
Fog checkbox may leave depth capture scheduled because depth-alpha and other
consumers remain active. Particle permission disables presentation after native
simulation stepping; it is not a simulation-off benchmark.

Current host CPU costs, transparent fill, high-resolution postprocess traffic and
GPU/CPU readback synchronization require separate measurements. Draw budget
<1000 and P95<=16.67ms are user goals, not universal GPU or engine limits.

## Limits

No new original streaming, fog, particle or culling reconstruction is asserted.
Public Godot depth-texture reuse is not accepted: the existing transparent-road
GPU counterexample remains. Any modern backend or submission batching candidate
must preserve native command order, material/pass identity and pixel evidence.

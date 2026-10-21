# E0059P01 - Camera movement exposes unsupported particle blend

Retail SLUS_203.62 SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
Debug NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Read-only Ghidra RenderEffects00128DA0/00128EF8 refreshed: hermite RGBA is clamped
0..255 and packed; TFX/ALPHA direct texture attachment remains original. No native
simulation or recovered packet function changed. Raw analysis stays local/task0059.

Track11 camera crossing zones and rotating reproduced a host NotImplemented guard.
First original witness: DUSTA_CS hash2747776789, vertex alpha192, whole-atlas max89,
UV0.25/0. Original entire-atlas bound includes pixels outside this animation frame.
UV-footprint bound allows that frame. A later genuine unsupported frame has alpha199,
tile max89, RGB142/142/164 at UV0/0. Its effective alpha can exceed128: normalized
blend support remains a recovery gap, not a reason to invent attenuation/clamping.

Destructive amplification: TrackParticleRenderer guard escaped update_view; outer
failure cleanup unloaded all scene resources and changed ok=false. asset_viewer
then stopped invoking update_view. This explains permanent total disappearance.

Port fix: retain the original-address NotImplemented throw for unsupported quads,
catch/report only that alpha validation boundary, count omitted quads, deduplicate
messages per texture/frame, continue supported quads and scenery. Compact emitted
node/stream/alpha-key ranks when a group has no supported quads; do not accidentally
show an old/empty node. Native group depth, simulation timestep and RNG unchanged.
No global particle disable, wrong blend substitution or weakened native culling.
Incomplete quad rendering is explicitly reported; this is failure isolation, not
full high-alpha GS compositor recovery. Future compositor work remains TASK0053.

Port sampler bound uses actual atlas rectangle plus bilinear neighbors with original
wrap/clamp modes; Q1 particle shader fixes LOD0. Cache immutable bounds per texture
and atlas UV rectangle; clear on unload. Pure unit covers false-positive global max,
bilinear neighbor, clamp/repeat and invalid dimensions. No silent error suppression.

Expose unsupported count/messages in frame state, existing Effects reason and visible
TrackViewer recovery diagnostic. Reset Camera previously targeted world origin;
restoring current track start/forward is a port UI correction, not PS2 behavior.
PC-only tests, never Verified. No proprietary asset data is added to Git.

# TASK-0051 common GPU adapter and validation

The shared host owns async CPU loading, immutable Scene/Runtime lifetime, incremental
texture/mip/mesh upload and a cached MeshInstance/material per instance-solid-surface.
It queries the active Camera3D projection on every prepare_frame, including rotation
and viewport changes. Native coordinates map to Godot (y,z,x) at this boundary only.
No unit scale multiplier or host physics is introduced.

Every original frame command carries its own matrix and local light context. The GPU
camera position is transformed to model-local coordinates before VU reflection and
lighting. Original RGB5A1 alpha expands bit15 to0/128; absent VIF colour streams fail
instead of receiving a guessed colour. All original mip levels contribute to the
fusion coverage proof. Native pass descriptors are retained in the frame digest even
when an asset-proven pair is fused by the GPU adapter.

Copy, source-over and additive host materials enter the transparent queue to permit
ordered submission. Solid/alpha partitions use priorities-64/+64 around existing
vehicle presentation. The Godot4.4.1 Compatibility overlap probe establishes effective
sort depth as origin_depth - sorting_offset, so the adapter cancels origin depth and
assigns strictly descending native ranks. The opposite sign was rejected by the GPU
probe. This is a port submission adapter, not an assertion about original interleaved
vehicle/UI queues or streaming GS residency.

The packet digest includes track/view, sections/zone, ordered instance/solid/surface,
LOD/visibility/original ordinal, matrix/light context, material/texture binding, selected
pass descriptors, depth/test/alpha/TEX1 and MatrixPack state. It excludes GPU handles.
The runtime and viewer share the same implementation; parity tests compare native
summaries and exact GPU images after disabling unrelated debug overlay text.

B03 mask camera positions come from Track11 original first nondegenerate triangle
centroids. Native water=(-170.320435,-70.8232015,-75.7048314),
tunnel=(80.0596873,198.257009,-68.5316238). These are inspection coordinates, not spawn
or physics replacements. Six moving-camera captures and native visible-mask checks
exercise alpha rewriting, original specular binding and real topology-selected geometry.

Compatibility fusion numeric probes evaluate source/destination byte-domain results,
including additive230, copy160, saturation255 and plain additive170 at alpha192.
Native integer references and actual GPU pixels agree over the stated fixtures.
Mipmap filtering, host floating depth and raster interpolation remain port projections;
there is no original hardware/PCSX2 visual or numeric Verified comparison.

Outstanding independent effect is original context2 six-view environment capture.
Affected FIRETRUCK_00/tank base passes remain explicitly diagnosed; no substitute
texture or invented environment state is used. See blend.md B06. This does not affect
TUNNELROAD_MASK or W_RDWATER_MASK, whose actual strips are4041.

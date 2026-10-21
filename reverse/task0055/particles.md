# TASK0055 persistent particle GPU transport

## E0055P01 - Scope and source authority

This is a port-side Godot optimization. Original AcidEffectSystem simulation,
RNG, particle packets, quad order, alpha keys, material/depth rules and native
functions are unchanged. TASK0054's shared screen projection helper and
camera-relative ordinary bounds remain intact. No PS2 reconstruction is added
and no original-console Verified status is claimed.

Touched host files: track_particle_renderer.cpp/.hpp and new
particle_mesh_stream.hpp. `set_mesh_reuse(bool)` defaults false, must be selected
before a scene is loaded, and persists across clear/track switches. False keeps
the previous per-frame ArrayMesh creation/upload path. True retains one
ArrayMesh per original active-group rank, doubles quad capacity on demand,
and updates vertex/attribute regions without replacing the resource when the
packet changes within capacity. Capacity growth recreates its single surface
and immutable index template, not the ArrayMesh resource. Unused groups remain
hidden; retained capacity is released by clear.

### Pinned engine layout

Primary host-format authority is Godot4.4.1-stable's
[servers/rendering_server.cpp](https://github.com/godotengine/godot/blob/4.4.1-stable/servers/rendering_server.cpp):
`mesh_surface_make_offsets_from_format`/public format offset and stride APIs,
`_surface_set_data` color/UV packing, and `mesh_get_surface` raw-buffer readback.
The uncompressed vertex stream is float3. The separate attribute stream is
RGBA8 then float2 UV. The adapter queries actual format/offsets/strides, checks
vertex12/attribute12 bytes and offsets0/0/4, rejects compression or extra
attributes, and uses dynamic-update surface allocation. Color packing follows
the engine's double-scaled/truncated float Color channels, rather than assuming
a raw byte roundtrip. No unverified interleaved storage offsets are used.

Only vertex/attribute region updates are exposed by the pinned ArrayMesh API;
the fixed quad index template is retained. Extra capacity quads collapse all
four vertices to the first active quad's same valid position/depth, with equal
attributes. Their triangles therefore have zero raster area. This is host-only
storage padding, not a new simulated/visible quad. GPU primitive counters can
include those degenerates, so capacity overhead and performance still require
measurement. The experiment remains off by default pending acceptance.

### Proxy and lifecycle interface

The same ArrayMesh Ref is shared by source and existing depth proxy, so raw
buffer changes remain visible to both. `set_mesh_change_callback` reports actual
resource/material/surface-layout changes; root connects this to proxy cache
invalidation. Particles stay unsealed dynamic sources. Root's optimized late
geometry sync runs after current particle/material/rank updates; baseline keeps
its prior sync path. Ordinary custom camera bounds and layer masks are retained.

`stats()` provides track-lifetime mesh/surface creation totals, mesh updates,
actual two-buffer update count, uploaded bytes, retained quad capacity, active
quads and rendered-group degenerate quads. ParticleFrame provides per-frame
mesh_creations/surface_creations/mesh_updates plus retained capacity_quads.
Counters reset at clear; mesh-reuse configuration and original global RNG
lifetime behavior persist.

## Tests and current validation

`TrackParticleRenderer::test_mesh_reuse` is an asset-free engine fixture. It
compares updated vertex/attribute/index raw buffers against the engine's own
fresh original-array upload, including all256 RGBA byte values and changed
positions/UVs. It also verifies stable resource identity through capacity growth,
shrink without surface recreation, and collapsed inactive vertices. It returns
a fresh one-quad baseline and a shrunken capacity-four mesh for GPU comparison.

`particle_mesh_reuse_gpu_smoke.tscn` consumes that fixture and the exact shared
TASK0054 screen projection source. It compares full RGBA8 images for both color
and encoded-native-depth output, asserts meaningful nonempty coverage, and
checks inactive capacity adds no pixels. Root must bind static
Ps2TrackRenderer.test_particle_mesh_reuse to the fixture and register the scene.

Current validation: C++20 syntax-only compile passed with pinned Godot generated
headers and generated shader header. No shared build or GPU execution performed
by this worker before root grants the GPU slot. Engine fixture, real particle
routes, color/raw-depth parity, failure/clear/switch lifecycle and default-gate
performance acceptance remain pending; no measured speedup is asserted.

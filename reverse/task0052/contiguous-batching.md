# TASK0052 conservative ordered batching

## E0052D11: host-only eligibility plan

`platform/godot_bridge/track_command_batches.hpp` produces contiguous command spans and original vertex/index subranges without mutating the native Frame. It does not promote any command to Godot's opaque queue. Equal-depth GS ordering remains observable and the existing GPU fixture already establishes that global opaque reordering is unsafe.

A merge candidate requires the same texture hash, material hash, complete EffectState/TextureState/TestState, identity/lighting flags, bit-identical local-world matrix and bit-identical28-float light context. Different source geometry may be concatenated in the original triangle order; no source vertex position, native normal, UV or packed colour needs changing. The same local matrix also preserves the per-draw local source_camera uniform. Distinct original_order/depth values remain metadata in the subranges; no interleaving command may be crossed. Any original environment pass splits spans because baseA/envA/baseB/envB cannot be reordered into baseAB/envAB.

This is more conservative than a general MultiMesh conversion. Different transforms generally require different local source_camera and light uniforms; the current shader contract cannot represent these per instance via a single four-float custom attribute. No such conversion is claimed equivalent.

The helper bounds accumulated indices to the host signed32-bit capacity and retains command indices, first vertex/index and counts. A future GPU mesh cache must include the complete ordered geometry sequence and the uniform/pass key; native packet digests and command records must remain unchanged. Driver rendering/merged-surface pixel equivalence is required before enabling an actual host merge. The helper alone changes no displayed output.

Private standalone `track_command_batches_tests` passed geometry offsets, contiguity and splits on matrix (including signedzero), lights, depth/test, alpha register, matrix pack, environment stage, flags, material and noncontiguous alpha partition. All30 original-camera GPU profiling is parent-owned; no speedup or draw-count reduction is asserted before those measurements.

## Mesh transport cache and engine fixture

`TrackMeshBatchCache` owns merged ArrayMesh resources keyed by the exact ordered sequence of(solid,surface) references; track replacement requires clear. It copies existing source positions, UVs, colours and raw CUSTOM0 normal data, retaining primitive order through index offsetting. Surface attribute changes, unsupported formats, invalid indices and changed subrange counts fail explicitly. CUSTOM0 may arrive from Godot as PackedByteArray even though the uploader supplied PackedFloat32Array; both representations are handled by byte-preserving float32 transport.

The cache uses a caller-configurable estimated attribute-byte budget and evicts least-recently-used resources only when the cache is their sole reference. A returned mesh remains valid if its cache entry is cleared. This is host resource management, not reconstructed PS2 paging behavior.

`test_track_mesh_batch_cache` is an engine integration fixture for source/merged array equality (including signedzero/nonunit normals), ordered indices, cache hits, externally referenced resources, LRU eviction, rebuild and clear lifetime. The Godot smoke scene calls it through a test-only static binding supplied by the common renderer. Syntax-only compilation passed; actual engine execution is parent-owned and must be recorded before claiming that check passed. No GPU pixel equivalence or performance improvement is asserted solely by this array fixture.

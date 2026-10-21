# E0060M01 - Track host resource residency

## Authority and limits

TASK-0060 is a user-authorized port-side ownership change, not a recovered PS2
allocator or streaming reconstruction. Retail scenery authority remains
SLUS_203.62 SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
Debug driving authority remains NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
No recovered function body, culling threshold, pass/alpha equation or physics
implementation changes. No new binary claim; Ghidra is not needed for host ownership.

## Confirmed source observations

- Previous pump_load uploaded every nonempty decoded surface before readiness.
  Track11 has 10,369 surface records. Native selection and host node visibility
  did not limit these persistent GPU source allocations.
- Every encountered view/instance/model/surface retained its draw node, distinct
  material and cached uniforms until unload. Inactive nodes were hidden only.
- Postprocess retained depth proxies and source/depth material pairs. Its source
  material references and hidden-node mesh references prevented cache eviction.
- The merged-mesh 64 MiB cache cap only evicted cache-exclusive references. It was
  not a limit on the render graph's total retained GPU resources.
- Debug fill/edges and their CPU topology expansion were built unconditionally.
- Scene.bundle has no runtime reader after load_scene and load_track_environment
  complete. Remaining CPU geometry, textures/mips, topology and metadata ARE used
  by native selection, exact batch transport and alpha footprint validation.

## Reviewed host changes

- Upload singleton/sky source meshes only when requested. Build merged meshes
  directly from the same decoded attributes; remove intermediate source GPU
  upload/readback. Attribute layout, indices, colors and CUSTOM0 remain unchanged.
- All active view submissions remain pinned, including independently scheduled
  reflection views. Inactive draw LRU: at most256 entries,16MiB estimated geometry,
  or120 unused visual frames. These are host cache policies, not PS2 visibility.
- Eviction removes source node, reflection node, postprocess proxy/material pair,
  uniform cache and owned-node bookkeeping before trimming merged meshes. Source
  meshes retain at most256 cache-exclusive entries or120 unused visual frames;
  externally referenced meshes are never invalidated.
- Merged cache remains64MiB for evictable storage; live geometry is an explicit
  non-evictable floor, not a reason to omit selected native draws.
- Debug geometry is created on first enabled overlay and released when both
  overlays are disabled. Topology readiness enables controls before allocation.
- Release the raw decoded bundle after all decoders finish; Track11 releases
  8,995,308 retained capacity bytes. Do not discard CPU data still read at runtime.
- get_memory_state is on-demand: CPU capacity categories, source GPU/merged payload
  estimates, material/proxy/uniform counts and owned render-target sizes. Estimates
  exclude allocator/driver overhead and shared math memory; do not sum them into
  a purported exact process/GPU total.

## Measurement and acceptance

The committed one-track lifecycle script and Windows sampler execute the actual
Godot executable (never the console launcher). Track11: load,12 stationary frames,
three identical24-pose moving/rotating laps, clear, reload and final clear. Visual
time is fixed at zero for repeatable ownership/native-packet comparisons; this is
not an FPS benchmark or a full driving/particle soak. Actual viewer particle-motion
and single-car entry checks are separate.

Sample private bytes, working set, commit and total process page faults. Windows
PageFaultCount includes soft faults. PDH Memory Pages Input/sec and Page Reads/sec
are system-wide disk-paging evidence, not attribution to Godot. Record missing
counter support rather than interpreting an unavailable counter as zero.

Preliminary Debug actual-process peak:4,249.58MiB before vs1,911.15MiB after.
Before route:4,639 retained draw nodes/4,444 depth proxies; after:388 draws,
256 inactive,371 source meshes. Repeated route resource counts stable; clear
returns all track-owned accounted categories to zero. Native route hashes and
1920x1080 reloaded-view screenshot bytes match the predecessor delivery exactly.
Final isolated measurements and commands are in docs/TRACK_MEMORY.md.

## Unresolved

Driver/system allocator retention means process private bytes need not return to
startup after ownership is released. No general absence-of-leaks proof, PS2 oracle,
all-track validation, hard-fault attribution or60FPS claim. Fixed-route resource
boundedness is a tested scope, not a universal GPU-memory upper bound.

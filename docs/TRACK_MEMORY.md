# Track host memory ownership (TASK-0060)

TrackViewer, Test drive and the single-car scene use the same Ps2TrackRenderer.
This is host ownership policy, not a recovered PS2 allocator. Native zone/tree/
projected-size selection, LOD, passes, material equations and physics are unchanged.
See [E0060M01](../reverse/task0060/memory-lifecycle.md) and the strict source review.

## Changes

Track11 previously uploaded all nonempty surfaces (10,369 source records) before
camera selection. Moving also retained hidden draw nodes and depth proxies. Their
references prevented merged-mesh cache eviction.

Source GPU meshes are now lazy. Multi-surface batches copy identical decoded CPU
attributes directly into the final mesh, eliminating intermediate GPU upload and
readback. Every native-selected surface/pass still renders. Active view resources,
including independent reflection submissions, are never evicted.

Inactive draws use a host LRU:256 entries,16MiB estimated geometry,120 unused visual
frames. Eviction removes source/environment-pass nodes, depth proxies/materials and
uniform references together. Unreferenced source meshes retain at most256 entries
or120 unused frames. The merged cache keeps its64MiB evictable-storage policy;
referenced geometry is a non-evictable floor, not a total GPU-memory guarantee.

Track11's8,995,308-byte raw bundle is freed after all decoders finish. Decoded CPU
geometry, textures/mips and topology remain for native selection, exact batching
and particle alpha bounds. Collision/material debug geometry is created on demand
and released when both overlays are disabled. Flags still survive track changes.

## Accounting and reproduction

Call `Ps2TrackRenderer.get_memory_state()` on demand. It reports source capacities,
mesh/batch payload estimates, active/inactive draws, limits/evictions, cached draw
materials/uniforms, depth proxies/materials and owned render-target dimensions.
Estimates exclude allocator/driver overhead, shader binaries and shared math image;
they are not an exact process-memory total. The full accounting walk is not per frame.

Build: `cmake --build --preset godot-debug -j 4` and
`cmake --build --preset godot-release -j 4`. Tested with Godot4.4.1, GCC13.2,
CMake3.29.2, Python3.14.4, Intel Arc B390/OpenGL Compatibility32.0.101.9150.

```powershell
py -3 reverse/scripts/measure_track_memory.py `
  --engine local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64.exe `
  --project local/task0060/delivery/Debug/godot `
  --output local/task0060/recheck-debug --enforce
```

Use the Release project for the same check. The sampler measures the actual engine,
not its console wrapper: private bytes, working set, commit, total process page
faults, available physical memory and system PDH paging. PageFaultCount includes
soft faults; Pages Input/sec and Page Reads/sec are system-wide, not Godot hard-fault
attribution. Unavailable counters are not zero. Asset-dependent outputs stay ignored.

Track11 fixture: load,12 stationary frames, three identical24-pose moving/rotating
laps, clear/reload, four overlay combinations, worker load/clear and invalid-ID
failure cleanup. Visual time is frozen for ownership/native-packet comparisons;
this is not an FPS or full driving/particle soak. Viewer motion/race tests are separate.

## Results and limits

Isolated Debug route private-byte peak:4,237.52MiB before vs1,897.39MiB after,
55.2% lower. Final delivery additionally exercises overlays/cancel/failure cases.
Exact final results and commands are recorded in TASK-0060.

Before route:4,639 retained draw nodes/4,444 depth proxies. After each warmed lap:
388 draws (132 active,256 inactive),378 depth proxies/materials,371 source meshes,
641 merged entries/20,855,024 payload bytes. Counts remain stable across repeated
laps. Unload returns accounted source bytes, nodes, source meshes, batches,
draw/depth materials and targets to zero; scene node count returns to baseline6.

All24 native route hashes match TASK0059. Reloaded1920x1080 screenshot bytes match
exactly. The GPU fixture checks position/UV/color/index/CUSTOM0 transport. PC tests
do not establish PS2 Verified. No all-track sweep, universal memory cap or60FPS claim.

Private bytes remain above startup after clear. Driver/allocator retention is
possible; without allocation tracing, remaining bytes cannot all be attributed to
either retention or leakage. Large active views/Show LOD can still need substantial RAM.

## Delivery

`local/task0060/delivery/Launch TrackViewer Native.cmd` starts Debug Track11;
`Launch Race Release Native.cmd` starts the single-car Release scene. Both include
all runtime DLLs, hashes and BUILD-INFO.json. These are local binary/config snapshots
with workspace junctions for scripts/scenes/assets/shaders, not portable exports.

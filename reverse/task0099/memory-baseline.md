# TASK-0099 memory copy baseline

Port-only measurement, E-0099-M01; NOT original game behavior, running-race timing, Godot frame timing, GPU timing or1080p60 acceptance. Source tool is src/port/diagnostics/memory_copy_benchmark.cpp. Memory::footprint is read-only port accounting; it does not claim original PS2 allocation statistics.

## Protocol and results2026-10-28

Host Intel Core Ultra X7 358H,16 cores/16 logical processors. GCC13.2.0, CMake3.29.2, C++17 Release configuration(-O3 -DNDEBUG). The SHA-checked Debug runtime image is loaded BEFORE timing. Each timed iteration copies and destroys one existing port Memory image and observes a word; no physics/render/audio step is performed. Three separate executable processes each use120 warmup iterations then600 samples. P99 is nearest rank. Raw timings remain ignored in local/task0099/memory-copy-run1..3.json and hashes/build/source identities are in memory-baseline.json.

| Run | Payload bytes/copy | Regions | Mean ms | P50 ms | P99 ms | Max ms |
|---|---:|---:|---:|---:|---:|---:|
|1|2310172|2|0.558539|0.5850|0.8929|2.7976|
|2|2310172|2|0.594404|0.5911|0.9468|1.0535|
|3|2310172|2|0.599294|0.5981|0.9045|1.9030|

Payload byte count is logical copied data, NOT measured memory-bus traffic. Heap allocation counts were not instrumented. The mapped image has no owned dynamic allocations yet; live vehicle/world/particle state adds data and bookkeeping. This is therefore not a complete NativeBackend::advance or multi-car frame benchmark and must not be multiplied into a claimed car-count/FPS result. GPU timers are unavailable in this standalone tool. No rendering setting or original object count was reduced.

## Exact commands

```powershell
cmake -S . -B build/native-memory-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DPORT_BUILD_GODOT=OFF
cmake --build build/native-memory-release --target port_memory_copy_benchmark --parallel 4
& 'build/native-memory-release/port_memory_copy_benchmark.exe' . > local/task0099/memory-copy-run1.json
& 'build/native-memory-release/port_memory_copy_benchmark.exe' . > local/task0099/memory-copy-run2.json
& 'build/native-memory-release/port_memory_copy_benchmark.exe' . > local/task0099/memory-copy-run3.json
```

The target is EXCLUDE_FROM_ALL and not a timing-sensitive CTest. It uses existing configured Debug ELF resolution/SHA validation, never a proprietary path in source.

## Implications and remaining work

New registration/startup code uses one shared Memory, no per-car backend/world copies. World table rebuild performs no persistent allocation; native reentry probes retain existing mapped footprint. Startup scratch drivers are freed. Catalog validates/reserves metadata before its single arena allocation. None of this replaces the existing fixture backend's full-image transaction yet.

A dirty-page/write-barrier transaction still requires exact commit/rollback tests across byte/word/half/vector/matrix/copy/map/allocate, error paths and shared RNG/object state. Mutable ByteView/raw-pointer writes and views retained across transaction boundaries are critical coverage hazards; do not silently assume Memory::word intercepts them. Complete shared World construction/scheduling first with explicit ownership; keep single-car fixture behavior isolated. Actual gameplay allocation/upload rates, full Release1080p/VSync-off frame protocol, GPU timers and retry/RID/audio/worker lifetime acceptance remain open.

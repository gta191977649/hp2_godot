# TASK-0072: isolated CPU preparation and Godot scheduling

This is a port-side integration, not recovered PS2 scheduling. Recovered input,
physics, visibility and ordered command generation remain serial and Godot-free.
Future recovery of the original main program changes the producer/orchestration;
it must not import this scheduler into recovered functions.

## Ownership and execution

`Runtime::prepare_frame` produces the existing native frame. The bridge retains
its conservative contiguous batch policy. On the main thread it checks existing
draw/source/batch caches, deduplicates missing geometry keys and builds immutable
inputs. `platform/render/mesh_prepare.hpp` accepts standard-C++ values and shared
const presentation geometry; it has no Godot headers, RIDs, Nodes or task IDs.
Its output contains local vertex attributes, rebased indices, optional original
matrix rows and matching host bounds. It does not select instances or reorder draws.

The production snapshot aliases immutable decoded geometry with shared ownership
of its Scene. It copies each required matrix and subrange. Work never reads the
mutable Runtime, original math Memory, active Godot arrays or cache containers.
The historical GPU-array lookup remains a main-thread fixture/reference facility;
production workers use decoded CPU assets and never call surface_get_arrays.

The Godot adapter schedules whole missing meshes through native group callbacks.
Each callback owns its output slot and catches exceptions. The caller joins the
complete group before propagating errors through the existing ABI boundary.
Only then does the original loop convert packed arrays, create resources, insert
cache entries and submit states in original order. Singleton source uploads keep
their original place relative to materials and nodes. LRU access/eviction is not
reordered by completion order. A late cache miss is prepared inline and timed.

There is one in-flight view, with no cross-frame queue or added result latency.
No task survives prepare_view. Scene replacement/unload therefore occurs after
all callbacks have joined; snapshots also own their source lifetime. Diagnostics
carry renderer scene generation, view and native visual source frame. finish_view
releases temporary output after ordered submission. No main/reflection selection,
physics tick, postprocess consumption point or original math operation is moved.

## Configuration

Defaults remain0/Safe until all acceptance gates pass:

```powershell
.\build.ps1 --run --profile --render-prepare-workers=2 --render-thread=safe
.\build.ps1 --run --profile --render-prepare-workers=4 --render-thread=separate
```

0 runs exactly the same pure preparation function inline. 2/4 cap bridge group
tasks; they do not resize the global pool or promise physical cores. Zero/one
missing mesh runs inline. Cache hits never spawn tasks. Separate is an independent
engine experiment and must run standalone; the editor forces Safe.

Godot consumes its --render-thread argument before OS.get_cmdline_args. Launchers
also pass --render-thread-requested metadata. The bridge compares this request
with RenderingServer.is_on_render_thread called from the main thread: true is
Safe, false Separate. A mismatch is a startup error, not silent fallback. A direct
engine launch without metadata reports the request as project/unknown; actual
mode remains recorded. Worker values other than0/2/4 are rejected.

The per-view render_preparation_views dictionaries appear in host_stats and
Profile context. snapshot_ms, dispatch_ms, prepare_wall_ms, join_wait_ms,
packed_conversion_ms, source_upload_ms, batch_upload_ms, batch_cache_commit_ms
and late_inline_prepare_ms are CPU wall measurements. job_sum_ms is cumulative
job time and overlaps the wall/wait measurements; never add it to frame time.
Upload times describe CPU API calls, not measured GPU transfer duration. Jobs,
actual peak concurrency, cache hits, reused draws and logical payload bytes are
reported. GPU source-frame rules remain those in PROFILE_DEBUG.md.

## Native sector selection stays authoritative

The chain remains eDisplayFrame -> DoCulling -> WhatSectionsShouldWeDraw ->
CullView -> TreeCull -> DrawAScenery. FindVisibleZone chooses a zone from its
polygon and retains move-to-front ordering. The selected section list includes
original global sections plus that zone's authored list, before tree/AABB,
instance flags, projected-size cutoff and authored LOD selection. No worker
substitutes loaded geometry, Godot visibility or a whole-sector MultiMesh for
the resulting per-instance commands. See E-0072-V01..V04 and VISIBILITY_TASK0072.md.

The fixed actual driving fixture observed zone71,14 selected sections out of128
loaded; its first main-view checkpoint considered235 of8407 loaded instances,
retained106 and produced1192 surface commands. These are different stages, not
interchangeable counts. The known original no-zone fallback is preserved and must
be reported if entered. These observations do not certify every original camera
coordinator or original route membership. Native tests exercise a neighbouring
zone, switching back, excluded sectors and the no-zone fallback.

## Reproduction and acceptance

Use the isolated project/DLL, never overwrite a DLL held by an owned test process.
Set both PORT_GODOT_PROJECT_DIR and PORT_GODOT_BIN_DIR when configuring CMake.

```powershell
ctest --test-dir build/godot-debug -R 'mesh_prepare|track_runtime|native_render_workers_rules|render_worker|render_thread_mismatch' --output-on-failure
.\tools\run_task0072_worker_matrix.ps1 -ProjectPath local/task0072/candidate/godot -OutputDirectory local/task0072/workers/parity-final -Phase parity
.\tools\run_task0072_worker_matrix.ps1 -ProjectPath local/task0072/candidate/godot -OutputDirectory local/task0072/workers/profile-on -Phase performance
.\tools\run_task0072_worker_matrix.ps1 -ProjectPath local/task0072/candidate/godot -OutputDirectory local/task0072/workers/profile-off -Phase performance -WithoutProfile
```

Compare the frozen pre-split DLL to0/Safe, then compare0/Safe with each worker and
render-thread combination. Driving parity uses723 native tick invocations, seven
2880x1695 checkpoints, all submitted reflection faces/atlas and full unload/reload.
Tests require one original tick invocation per frame; legitimate scheduler holds
remain valid. Every image target and original command digest must match exactly.

Performance uses drive_spin and viewer short_spin,120 warm/600 measured frames,
three repeats, CORVETTE, Vulkan Forward+, identical Debug build and Profile layout.
Profile-off measurements are a separate protocol. Export events and cold loading
maxima remain separate from sustained percentiles. Five-minute driving rotation
checks resource/memory and timing growth. No concurrent graphical benchmark or
build is permitted during timed captures. Results and limitations belong in
PERFORMANCE_TASK0072.md. No default is promoted for speed alone; pixel/lifecycle
parity, gain above repeat variation and no repeatable P95/P99 regression are
required across the tested routes. Failure to reach16.67ms median/P95 leaves the
task active. No PC-only result promotes PS2 equivalence to Verified.


## CPU submission extension (2026-10-20)

New `--track-submit-math=true|false` is independent of worker count; default false.
Main-thread planning retains stable Draw iterators and copies selected transforms,
parent transform, camera, lighting, rank and reflection matrix. Standard-C++
`platform/render/submit_math.hpp` calculates desired values only. The shared
`PreparationWork` callback groups contiguous math slots alongside mesh misses;
0 invokes the same callback serially,2/4 cap native group concurrency. Per-view
join precedes all ordered Godot resource mutations. Duplicate targets compare
against latest committed state at each original command position. No native
selection, reflection scheduling or physics work is placed in the pool.

Cached inverse matrices require bit-identical input transforms. Unsupported
nonfinite/singular cases retain the reference bridge calculation and increment
fallback counts. Unlit commands do not need an inverse. Tests use actual pinned
godot-cpp arithmetic, strict FP options, parent scale/shear/reflection and signed
zero; they do not assert PS2 arithmetic equivalence.

Vehicle staging reads one detached versioned snapshot at the existing depth
consumer point. Generation disambiguates recycled identities; unknown uniforms,
missing publishers and shader changes retain reference getters. Depth discovery
listens for relevant source geometry changes plus child-order changes on source
ancestors. Unknown sources retain reference synchronization. Owned compositor
and reflection viewport trees are excluded. Tracked sources receive field masks
for geometry/material/transform/visibility/bounds/sorting changes; changing the
renderer owner transform invalidates tracked transforms/visibility. Material-to-
source associations propagate material replacement. Observers disconnect before
resource teardown; source IDs are resolved before use. No residency expansion.

Per-view diagnostics add draw_snapshot_ms (Draw lookup and immutable math snapshot),
mesh snapshot_ms, submit_math_entries/tasks, inverse_cache_hits and fallbacks.
prepare_wall_ms, job_sum_ms, dispatch_ms and join_wait_ms overlap; never sum them
as frame time. Main-thread Packed conversion/upload/cache commit remain separate.
View submission rows carry source_frame/scheduled_this_frame. Existing CPU rows
were already cleared each frame; stale CPU timings were not demonstrated. GPU
source attribution rules remain unchanged. Mesh-only old checkpoints had0..1
steady tasks; the new math switch explicitly covers every submitted batch.

User renewed permission for necessary Godot runs after the earlier test stop.
Use one isolated consolidated parity/performance sequence. New candidates remain
off and Separate stays off until individual parity and repeated tail gates pass.

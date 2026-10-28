# TASK-0099 - Real race World and participants

- Status: active
- Parent: TASK-0096; see parent for exact authority, constraints and acceptance.
- Dependencies: TASK-0098 native launch boundary; production frontend binding remains open.
- Scope: Original constructors/loading/time/route/reset/traffic/opponents/multi-body collision; shared-world memory/compact publication; no single-car fixture race.
- Acceptance: scope behavior/production routes/tests/evidence complete; original comparisons
  stay open until recorded. Detailed address-bound criteria added before activation.
- Current bounded implementation: first-event shared World participant registration/table and original constructors.

## Activation2026-10-28

User explicitly renewed first legitimate event full race roundtrip as the immediate milestone. TASK-0098 native launch handoff now has original-data tests; full presentation/profile integration is still open. This scheduling revision permits native World work before full frontend acceptance. Do not claim TASK-0098 complete or substitute its service fixtures for production resources.

Scope and checks: recover original World construction/participants before stepping races; first restore0014D028 BuildCarTable using the same shared logical Memory and original Car/DriverInfo links, stable category order and CarIndex stores. Continue real World/RaceCoordinator/Car initialization from E-0096-C01/ignored first-world captures. No per-car NativeBackend worlds, fixed-car scene, synthetic finish state or Godot vehicle-physics replacement.

Acceptance for current slice: original address/types/listing retained; category/order/bounds tests including real first-event driver records, shared storage and malformed-list rejection; native/Godot build, related regressions, evidence/tasks/progress updated. Full task acceptance remains real World/simulation/participants; list registration alone cannot complete it.


## Shared World and performance checkpoint2026-10-28

E-0099-W01 implements original shared-Memory BuildCarTable/GetCarByNumber/ResetTimeScale; E-0099-W02 implements DriveTarget constructor/scoped Clear and TrafficTeleporter route totals/timing. E-0099-W03 records required World ctor effect/pool/police/TrackGraph dependencies without pretending to implement them. No real World/Car simulation is running yet. The existing single-car backend remains a fixture.

E-0099-M01 adds read-only Memory footprint and a standalone Release clone/destroy baseline. Three independent120-warmup/600-sample runs of the2310172-byte imported image gave P99 0.8929/0.9468/0.9045ms; this excludes live world/simulation/rendering and heap counts/GPU timers. See reverse/task0099/memory-baseline.md. No full-frame60FPS or dirty-page replacement claim.

Final relevant build/12 regressions passed12.73s before source-review order corrections/read-only footprint extension; final validation is recorded below. Normal Godot DLL also built at that point. Source review corrected selected-ride lookup after character selection and per-generation location reads; native order probes passed after correction. Evidence/records42 scoped Debug entries and structure/version details are in task0098/first-event-launch.md and task0099/shared-world.md.

Exact next action remains original World/RaceCoordinator construction, real routes/graph/resources and Car/controller/mover initialization and ordered scheduling. The constructor dependency audit lists actual required pools; only explicitly proven original no-op functions may be empty. Production frontend/profile/record/GameFlow service providers from TASK-0098 remain unbound, and real finish/results/rewards/save plus original/performance acceptance are not complete. Continue autonomously without a confirmation gate.

Final validation2026-10-28:

- Final12-target native build command in reverse/task0098/first-event-launch.md passed after live-read ordering and read-only footprint changes.
- Final12-test command there passed12/12 in12.98s.
- Final original car identity/usage assertions: `cmake --build --preset native-debug --target race_parameters_tests --parallel 4` passed; `ctest --preset native-debug -R '^race_parameters$' --output-on-failure` passed1/1 in1.80s. All64 Debug enum/asset identities were independently audited, with no mismatch.
- Final `cmake --build --preset godot-debug --target ps2_port --parallel 4` passed including latest source order/DriveTarget/Memory changes. No window/gameplay acceptance is implied.
- `cmake -S . -B build/native-memory-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DPORT_BUILD_GODOT=OFF` and `cmake --build build/native-memory-release --target port_memory_copy_benchmark --parallel 4` passed. Three separate120/600 benchmark processes exited0; exact commands/results/source+binary hashes in memory-baseline.md/json.
- Generated progress/contracts and final whitespace/evidence checks are recorded after final metadata refresh. No commit/staging or Verified promotion.

The intended first race roundtrip remains unfinished: there is no actual complete World/controller/police/race/results/save session yet. The next implementation remains TASK-0099 original resources/routes/graph/World/Car construction; continue that chain, not further isolated frontend presentation work.

- Final `ctest --preset native-debug -R '^(progress_tables|recovery_progress_contracts)$' --output-on-failure`:2/2 passed2.20s after final metadata refresh.
- `git diff --check`:passed; existing line-ending notices only. All831 TASK-0098 and43 TASK-0099 indexed capture hashes matched files. Restricted-material scan of untracked files passed; staged files0. No commit made.

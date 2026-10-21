# TASK-0051 - Original track passes and unified runtime rendering

- Status: completed
- Approved scope: tunnel/wet-road blend correctness; one shared native frame/host path for TrackViewer and independent single-car game scene in Debug and Release.
- Exclude full StartRace/GameFlow/RaceCoordinator, AI, ranks and race rules. Original Ghidra read-only; English code/docs; preserve pre-existing changes and historical review pins.

## Acceptance

- [x] Original Retail eDisplayFrame/scenery/eModel/solid-alpha ownership, culling, sections/zones/tree/LOD and per-instance light contexts.
- [x] Original flag40 alpha and320 extra pass, BiasLevel matrix packs, material/texture/test/depth/alpha state and ordered submission.
- [x] One non-debug Ps2TrackRenderer/nativeRuntime and thin TrackViewer; all shaders centralized.
- [x] Independent race_scene.tscn --track/--car uses shared track/vehicle/driving runtime, controls, lifecycle and explicit failures in Debug/Release.
- [x] Native/GPU values, actual tunnel/puddle windows, view/packet parity, all30 assets and lifecycle regression.
- [x] Evidence, task/progress/usage, strict successor audit and full regression; no Verified from PC tests.

## Allocation

REA step6: scenery worker owns assets, culling/lighting and native runtime frames; effects worker owns original VU/material/depth/blend/sort; runtime worker owns common driving bridge and race entry; root owns sharedGPUhost, build/registration, integration and windows. Shared builds are serialized byroot.

## Final baseline closure (TASK-0052 prerequisite)

The full Debug suite ran98 tests:96 passed, two failed. The single-chain structural
check still inspected the former Lab implementation instead of its shared Runtime;
it now checks the real entry and thin subclass. The windowed race fixture exposed
startup without a presentation frame when focus was absent; runtime rendering now
runs independently of focused physics, including its initial frame. Both corrected
tests passed2/2 (28.39s). No native algorithm change or old review-pin refresh.
Release extension build passed; actual Compatibility single-car runtime passed
native commands/body presentation, controls, focus, failure, restart, frontend exit
and re-entry. Logs local/task0051/{ctest-full-debug.log,baseline-final-rerun.log,
release-runtime-final.log}. Debug and Release artifacts were isolated for testing.

Remaining original comparison and separate six-view environment capture are the
explicit documented boundaries. User-reported Roadshine strength and complete
render-stage restoration proceed as TASK-0052; this baseline is not original Verified.

# TASK-0051 - Original track passes and unified runtime rendering

- Status: active
- Approved scope: tunnel/wet-road blend correctness; one shared native frame/host path for TrackViewer and independent single-car game scene in Debug and Release.
- Exclude full StartRace/GameFlow/RaceCoordinator, AI, ranks and race rules. Original Ghidra read-only; English code/docs; preserve pre-existing changes and historical review pins.

## Acceptance

- [ ] Original Retail eDisplayFrame/scenery/eModel/solid-alpha ownership, culling, sections/zones/tree/LOD and per-instance light contexts.
- [ ] Original flag40 alpha and320 extra pass, BiasLevel matrix packs, material/texture/test/depth/alpha state and ordered submission.
- [ ] One non-debug Ps2TrackRenderer/nativeRuntime and thin TrackViewer; all shaders centralized.
- [ ] Independent race_scene.tscn --track/--car uses shared track/vehicle/driving runtime, controls, lifecycle and explicit failures in Debug/Release.
- [ ] Native/GPU values, actual tunnel/puddle windows, view/packet parity, all30 assets and lifecycle regression.
- [ ] Evidence, task/progress/usage, strict successor audit and full regression; no Verified from PC tests.

## Allocation

REA step6: scenery worker owns assets, culling/lighting and native runtime frames; effects worker owns original VU/material/depth/blend/sort; runtime worker owns common driving bridge and race entry; root owns sharedGPUhost, build/registration, integration and windows. Shared builds are serialized byroot.

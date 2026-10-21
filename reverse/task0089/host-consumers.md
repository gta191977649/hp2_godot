# TASK-0089 Godot water-effect consumers

E-0089-H01. Port-only dictionary/audio/render transport of native E0089W01..05
and original wiper animation metadata. Debug authority and resource coverage are
recorded in water-research.md and wiper-consumer.md. Not Verified.

Ps2DrivingRuntime publishes raw signed wiper state/start/end and unsigned original
world frame. Ps2VehicleRenderer validates the complete input widths and wheel
publication before applying original local wiper matrices. Legacy test snapshots
without wipers explicitly clear the optional effect. The model owns original
pixel-size/time/matrix decisions; the host owns no phase or physics integration.

SplashSoundBank loads Splash1/2/3 from the same original InGame decoder used by
collision audio. Native requests retain original sample selection and volume;
existing original gain/pitch helpers bind patch volume and sample rate. The host
consumes each committed request once, with independent one-shot player lifetimes.
Pause, failed-state silence, restart/replay/vehicle load and unload drain pending
requests and stop voices. The current listener is the explicit own-car fixture,
not restored original Sound3d/world scheduling or stereo ownership.

Validation on Godot4.7.2 Vulkan Forward+ Intel ArcB390:
- Final normal DLL build succeeded.
- water_effects_host passed4.23s and collision_audio_host passed2.28s.
- A subsequent relevant seven-case host batch passed camera, collision audio,
  water GPU4.94s, water headless4.54s, particles6.15s and driving asset20.30s.
  race_runtime_host exposed a passive external-animation binding regression;
  preserve this failure and see the final correction/check below.
- The first new host fixture incorrectly treated set_show_lods(false) on an empty
  renderer as a successful completed load. It now sets that option before loading,
  then checks the actual load and all subsequent publications. No behavior check
  was removed.

The water fixture uses actual Track12/native911TURBO plus original vehicle and
sound assets. Its test-only active-wiper snapshot injection tests the host
consumer; actual water contacts, timed scheduling, audio RNG and DriveOver births
are covered by native controlled-topology tests. It checks real wiper nodes,
original frame holds/changes, invalid-publication rollback, inactive restoration,
three decoded original patches and one-shot poll/pause/reset semantics. A single
ignored GPU screenshot local/task0089/wiper-host.png was inspected: original
blades sit on the windshield without displaced geometry. This is not an original
runtime visual comparison or a performance benchmark.

Final correction: passive external animation references retain explicit unresolved metadata/diagnostics; required active wiper animation still throws before publication. Native wiper/geometry/model3/3 passed11.87s. Rebuilt normal DLL, then ctest --preset godot-debug -R '^(race_runtime_host|water_effects_host)$' --output-on-failure passed2/2 in21.07s (race17.39s, water3.65s). This final run supersedes the failed passive-binding race load above; no unrelated test assertion was removed. Remaining original scheduling/replay/rain-screen limits are unchanged. Root-owned source diff whitespace check passed. No performance benchmark or proprietary staging.

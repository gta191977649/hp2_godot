# TASK-0083 - Recover collision and tire skid audio

Status: Completed (bounded supported scope; not Verified)
Date: 2026-10-21 Australia/Sydney

## Scope and acceptance

User adds original tire skid/drift audio. Recover its independent gates and
original samples; do not use skid-mark eligibility as an audio substitute.

Recover Debug collision event-to-sound chain, original sample selection, gain,
pitch and timing gates; connect the supported static-contact driving path to
native audio output and host one-shot playback. No synthetic replacement.
Authority NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
- [x] Read-only address-anchored evidence and asset provenance.
- [x] Native reconstructed selection/event behavior with practical tests.
- [x] Host playback and pause/restart/rollback lifecycle tests.
- [x] Strict successor preservation, builds, documentation and continuation.

Unknown dynamic car/damage/SPU services remain explicit; no Verified promotion.
Existing unrelated workspace edits preserved. Evidence local/task0083 (private)
and reverse/task0083 (public). Next: inspect collision producers and sound handlers.

## Validation recorded 2026-10-21

CMake3.29.2, Ninja1.12.0, GCC13.2.0, Python3.14.4;
Godot4.7.2 stable ed1daf0bf; godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b.
- cmake --build --preset native-debug --target static_collision_tests driving_tire_audio_tests driving_collision_audio_tests driving_backend_tests driving_collision_events_tests -j6: passed (targets built in two calls).
- ctest --preset native-debug -R '^(static_collisions|driving_backend|driving_collision_audio|driving_collision_events|driving_tire_audio)$' --output-on-failure:5/5passed25.71s.
- cmake --build --preset godot-debug --target ps2_port -j6: passed.
- ctest --preset godot-debug -R '^(driving_audio_host|collision_audio_host)$' --output-on-failure:2/2passed4.73s.
Native tests use configured original assets. Host tests use Dummy audio, not a
speaker/PS2 comparison. Teardown now releases stream references; the one-shot
fixture allows the Dummy audio service to retire playback before process exit.
Earlier short-lived fixture runs exposed retained playback instances; final tests
complete without that warning. Existing misleading-indentation warnings remain
unrelated. Strict preservation result will be recorded in source-review.md.

## Final preservation and deployment

TASK0083 strict unittest7/7passed24.728s; historical82 7/7passed21.995s;
existing_body_preservation1/1passed25.47s. Exact12replacements/26additions,
9modified/14new auxiliarypins; no original stub transition. Review SHA256
42c543a80e59d31ff8101abff7ba51d198c5b41ddef8cb479d5054516a9fc688.
Frozen82 evidence unchanged. No commit created or unrelated edits staged.

Final inspection corrected sound-update iteration from polygon map order to the
original CollisionList creation order. Native static_collisions then passed1/1
13.23s. The running user frontend locked godot/bin/ps2_port.dll, preventing final
replacement. Its window/process was preserved. Final extension successfully linked
and tested in ignored local/task0083/godot-stage/bin;2/2hosttests passed5.33s.
Fresh staging initially lacked .godot/extension_list.cfg and failed class lookup;
copying the existing res://ps2_port.gdextension registration fixed setup, then both
checks passed. This was staging setup failure, not suppressed test evidence.
CMake cache project/bin paths were restored to normal godot/godot-bin destinations.

Reproduce staged validation by copying godot excluding bin/.godot to the staging
project, retaining the original extension_list.cfg, configuring godot-debug with
PORT_GODOT_PROJECT_DIR=D:/dev/hp2_godot/local/task0083/godot-stage and
PORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0083/godot-stage/bin, then build
ps2_port and run the two host tests above. Restore both cachepaths afterward.
After closing the current frontend, install final normal runtime with:
cmake --build --preset godot-debug --target ps2_port -j6
The currently running process cannot adopt a newly built DLL automatically.

## Findings, limitations and continuation

Read reverse/task0083/{collision-audio,collision-events,tires}.md and per-entry
Debug records. Collision impacts preserve age, cumulative impulse, material,
variant and RNG rules; persistent scrape starts during contact accumulation and
fades via original service steps. Tire skid/squall/burnout reads original state,
independent of skidmark eligibility. Original22unique InGame PCM samples provide
these paths; no synthetic audio. Host stages use actual native tire driving plus
an explicitly port-side collision queue fixture; native wall tests cover producer.

Next task: scope original spatial camera/azimuth/doppler and sound service cadence,
then remaining rolling/leaves/damaged-tire/impact/thumper and multi-car/Smokeable
sound ownership. Original runtime comparison is still required before Verified.

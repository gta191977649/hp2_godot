# TASK-0082 - Recover vehicle lights, horn and engine audio

Status: Completed (bounded scope; no Verified promotion)
Date: 2026-10-21 Australia/Sydney

## Scope

User requests functional vehicle headlights, horn and engine sound. Recover
original state/gates, original sound/visual assets and the native producers;
connect host input/render/audio adapters in both current driving consumers.
Driving authority: Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Retail asset provenance remains separate. Build date/revision independently unknown.
No synthetic sound, invented lamp model or guessed RPM/audio tuning.

## Acceptance

- [x] Recover original light/horn state and input/update gates with addresses.
- [x] Implement supported visible lamps using original material/geometry/effects.
- [x] Load/decode original horn/engine samples and recover playback state rules.
- [x] Connect native state, semantic input and host playback/render ownership.
- [x] Test native state/data contracts and host start/stop/reset behavior.
- [x] Record limitations, strict preservation evidence, builds/tests and next action.

## Evidence and continuation

Read-only Ghidra and configured original assets; captures under local/task0082.
Public findings E-0082-L01/H01/A01 under reverse/task0082. Existing unrelated
workspace edits and predecessor review JSON remain preserved. No Verified promotion
without matching original-runtime evidence.

## Findings and validation

Evidence E-0082-L01/H01/A01/A02: reverse/task0082/{lights,horn,engine}.md.
Original lamp textures and horn/flash state are connected to both driving hosts.
Original12engine+horn ADPCM streams preserve sample loops and recovered mixer
pitch/gain/throttle/RPM rules.49vehicle banks decode;15traffic rows reference
missing original banks and report explicit errors. Headlight flares, special
sound effects and full original platform ABI remain outside this bounded scope.

Commands on2026-10-21 (CMake3.29.2,Ninja1.12.0,GCC13.2.0,Python3.14.4,
Godot4.7.2 ed1daf0bf; existing godot-cpp pin retained):
- cmake --build --preset native-debug --target driving_player_tests driving_backend_tests driving_audio_tests vehicle_model_tests -j6: passed.
- ctest --preset native-debug -R '^(driving_player|driving_backend|driving_audio|vehicle_model)$' --output-on-failure:4/4passed43.21s.
- cmake --build --preset godot-debug --target ps2_port -j6: passed after isolating sound header namespace exposure with forward declarations.
- ctest --preset godot-debug -R '^driving_audio_host$' --output-on-failure:1/1passed1.64s.
Host smoke uses Dummy audio and checks nonzero playing gains, horn/flash,
release, pause, restart and stream cleanup. No real-speaker/original-runtime
comparison; no Verified claim. Initial failed backend test used an exception
helper for a different exception type; replaced with real unsupported-frame
rollback/retry coverage, which passed. Strict preservation:source-review.json.
No commit created; existing unrelated changes preserved.

## Continuation

Read TASK-0082 and reverse/task0082 evidence. Next scoped recovery: original
CreateCarLightFlares00120558/eRenderLightFlare001168E8 marker, parameter and
texture dependencies and host ePoly consumer; separately bind audio stereo,
replay and damage/nitro/backfire services. Original runtime capture is required
before any Verified promotion. Missing traffic bank:__Unknown/47CB8E5A.

Strict preservation validation also passed: TASK0082 unittest7/7 (25.458s),
TASK0081 historical6/6 (16.279s), existing_body_preservation1/1 (29.27s).
Review scope21replaced/23added; frozen TASK0081 JSON unchanged.

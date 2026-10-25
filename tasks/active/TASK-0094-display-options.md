# TASK-0094 - Retail Display/HUD/calibration interfaces

- Status: active
- Parent: TASK-0091. Single current implementation scope; TASK-0092/0093 await original comparison.
- Binary: Retail SLUS_203.62 US PS2, executable revision unknown, SHA-256
  7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Evidence: E-0094-D01 onward, ignored local/task0091/display-agent.
- Scope: Display preparation/baseline, eight-byte settings and six value labels,
  P1/P2 HUD snapshots/changes/destruction commit, calibration actions and explicit
  host display-offset adaptation. Preserve conditional Autosave/history.

## Acceptance criteria

- [x] Portable Display/HUD/calibration controls and tests implemented from evidence.
- [x] Options/Display/HUD/calibration/Back routes and settings persistence connected.
- [ ] Host build, real-asset routes and rendered checks recorded.
- [ ] Original-runtime comparisons recorded; no Verified claim from PC tests.

## Evidence correction

Gameflow state3 is IN_FRONTEND (E-0091-G02), so001EE148 cycles primary map-style
byte+2. Do not force a Player count or call this option camera POV.001EE6C0
captures0x70 bytes of settings; it is not GPU preparation.001F4178 commits HUD
on destruction, not only on confirm. Calibration X/Y are GS display offsets;
host presentation must label its adaptation rather than claiming GS equivalence.

## Validation checkpoint2026-10-25

`cmake --build --preset native-debug --target frontend_display_tests frontend_runtime_tests frontend_audio_tests frontend_jukebox_tests --parallel 4`: passed.
`ctest --preset native-debug -R '^(frontend_display|frontend_audio|frontend_jukebox|frontend_runtime)$' --output-on-failure`:4/4 passed,38.97s.
GCC13.2.0, CMake3.29.2, Python3.14.4. Existing unrelated warning sites remain.
Godot build/render checks pending at this checkpoint. Original comparison open.

Godot `cmake --build --preset godot-debug --target ps2_port --parallel 4`: passed.
Godot4.7.2 Vulkan window captures: `--screen=26 --menu-events=6,4,6,wait:0.5`,
`--screen=25 --menu-events=4,4,6,wait:0.5`, `--screen=33 --menu-events=6,3,wait:0.3`,
all `--scale=1 --capture=D:/dev/hp2_godot/local/task0091/{display,hud,calibration}-live.png`.
All exited0 with FRONTEND_CAPTURE_OK; inspected original labels, selected states,
video background and calibration translation. Calibration/help overlay remains a
shared-help dependency; no claim that all calibration presentation is recovered.
Original comparison stays open. Current implementation moves to TASK-0095.

# TASK-0093 - Retail Audio Options interaction

- Status: active
- Parent: TASK-0091; follows TASK-0092 production Jukebox integration.
- Scope: seven volume sliders, original interpolation/marker geometry, output selector,
  restore defaults, Jukebox transition, Back commit and Autosave dependency.
- Binary: SLUS_203.62 US PS2; revision unknown; SHA-256
  7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Evidence: E-0093-A01 onward. Read-only MCP captures in ignored
  local/task0091/options-agent and popup-agent; no computer-use.

## Acceptance criteria

- [x] Original slider arithmetic, path interpolation and marker placement implemented.
- [x] Production volume/output/default/Jukebox/Back interactions connected.
- [x] Native boundary/route tests and Godot integration pass.
- [ ] Original-runtime comparison recorded; no Verified claim from PC tests.

## Findings and next action

001EFB80 dispatches Audio Options,0016DD38 adjusts raw amplitude by0.05,
normalizes with0.7/category-default, clamps normalized0..1 then divides back.
0016DE30 animates marker X using a0.25-second path. Retargeting retains the
current path derivative; the existing zero-velocity-only exit helper is insufficient.
Implement the bounded full scalar-path projection, then attach the seven controls.

## Validation continuation

Production integration now covers seven sliders, normalized bounds, nonzero
derivative retargets, output-mode localization, original volume feedback,
restore defaults, Jukebox exit and Back/Autosave commit. Frontend music and menu
sound gains are live; the private Godot bus consumes Mono/Stereo output requests.
Original Pro Logic II SPU encoding remains outside the host stereo adapter.

Native `cmake --build --preset native-debug --target frontend_audio_tests --parallel 4`
passed; `ctest --preset native-debug -R '^(frontend_audio|frontend_jukebox)$' --output-on-failure`
passed2/2 in5.21s. Godot-preset full build and later ps2_port target build passed.
Godot4.7.2 Vulkan capture using `--screen=28 --scale=1 --menu-events=6,wait:0.3`
was inspected (local/task0091/audio-live.png, ignored): original slider marker moved,
authored Y/scale retained, title/options/background/blurb displayed. Capture exited0
but reported4 leaked ObjectDB instances; cleanup needs investigation before calling
the Godot check complete. No original-runtime comparison or Verified promotion.

The capture cleanup warning was investigated with `--verbose`: two stopped WAV
streams/playbacks remained on the mix thread during immediate test exit. A0.1s
capture-only drain after stopping audio removes the warning. The repeated Vulkan
Audio capture exited0 without leaks/errors. Original comparison remains open;
TASK-0094 is now the implementation scope.

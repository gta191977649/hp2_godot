# TASK-0070 - Live in-game profiler panel

- Status: completed
- User request: global profile_debug.gd with realtime tracking/graphs and function hotspots in the running game, using ImGui when --profile is enabled.
- Scope: shared native ImGui frame, all scenes; selectable CPU/GPU/counter/function views, bounded scrolling history, pause/resume/reset and bounded JSON export. Function scopes report inclusive wall time, not an invented complete native call graph. No game logic or GPU timing inference changes.
- Normal runs remain panel/collector-free. GPU unavailable readings are gaps/N/A, no sum of viewport durations. Preserve frozen TASK0069 review with an exact host-only successor; explicitly retire the old track_profiler.gd auxiliary path.
- Acceptance: actual4.7.2 window panel/graphs, responsive resize/pause/export, no-track state and disabled/headless behavior; existing collector regression and strict source checks.

## Results

- Global native ImGui presentation,240-frame history, pause/frame inspection/clear/export/functions modes and16 monitors implemented. Actual input checks pass in graphical/headless runs, including frontend input gating.
- Global frontend function capture and a Running game20-frame capture pass. Game trace requires RaceScene._present_frame/Ps2DrivingRuntime.tick; full race runtime smoke also passes with profile enabled.
- Normal root Debug build delivered. Strict TASK0070 host-only successor preserves TASK0069 review JSON, explicitly retires old script auxiliary path and carries all remaining inherited pins. No original logic/shader/RNG changes.
- GPU history anomalies and complete native sampling remain out of scope; no inferred timings or FPS claim. See docs/PROFILE_DEBUG.md.

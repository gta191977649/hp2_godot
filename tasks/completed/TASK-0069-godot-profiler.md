# TASK-0069 - Godot 4.7.2 and profiler-driven render diagnosis

- Status: completed
- User authorization: upgrade the default engine directly to official 4.7.2; do not run or retain a 4.4.1 comparison workflow. Display the actual rendering API in the window title.
- Scope: runtime/build/test pins, API title, opt-in built-in render timing and custom Performance monitors, bounded current-pose profiling and evidence-based bottleneck report. Supersedes TASK-0068's engine-version constraint; its unfinished cache/source-review work remains explicit.
- Keep original game behavior, authored LOD, visibility, particle simulation/order and Compatibility default. No long FPS benchmark or track-ID optimization policy.
- Acceptance: official archive checksum/version checked; normal build uses 4.7.2; extension and targeted rendering/lifecycle smoke checks; title API matches runtime; useful actual CPU/GPU timing records from a short Track11 pose capture; unavailable metrics stay unknown.
- Runtime: official 4.7.2.stable.official.ed1daf0bf, Windows x64 archive SHA256 731980f9608d61333e5baf54a2ef17210acc7a538446c0cb9969f002aca1e953. Existing 4.4 API GDExtension has already passed the headless diagnostics smoke on 4.7.2; bindings need not be changed merely to run a newer compatible engine.

## Results

- Normal Debug build, official4.7.2 import, active OpenGL title,14 Performance monitors, host control/cache/diagnostic tests, five GPU regression tests and exact source successor pass. Current-pose/nearby cache-on/off native packets and colour/depth/final pixels match; same-runtime Sun reduction count and real-road occlusion tests pass.
- Bounded60-sample warm engine timing capture completed at actual2880x1695 and separate1080p. Primary actionable categories are scene transparency/duplicate depth and Sun synchronous waiting. Reduction4096->16bytes does not materially remove waiting. History timestamp anomalies remain unknown; no FPS acceptance is inferred.
- All30-track static census completes (60 records); no full30-track visual or PS2 verification claim. See docs/PERFORMANCE_TASK0069.md for commands, data, constraints and next optimization priorities. Broader GPU experiments remain explicitly in activeTASK0068.
- Bounded normal frontend boot selects4.7.2/OpenGL and exits0. Forced early exit reports two retained audio objects (AudioStreamWAV/AudioStreamPlaybackWAV); recorded as a shutdown-cleanup limitation rather than silently claiming warning-free exit.

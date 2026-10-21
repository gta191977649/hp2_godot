# E0070U01 - Global ImGui live profiling

User-authorized port-only successor: rename the collector to profile_debug.gd,
make it global across frontend/viewer/game scenes, and draw it with the existing
native ImGui frame. The old track_profiler.gd auxiliary path is explicitly retired.
Frozen TASK0069 review/scope JSON remains unchanged; historical validation uses
its canonical auxiliary snapshot. Every other inherited pin carries forward.

Host changes are limited to Ps2RenderHost::console_frame and a new port-only
ProfileDebugUi::draw. The latter owns bounded UI samples and draws controls,
graphs, unknown-value gaps and function rows. Frontend input is admitted to the
existing ImGui capture path while the profiler is enabled, including outside the
asset viewer. No original gameplay, physics, RNG, ordering or shader is changed.

Function scopes wrap script entrypoints and native driving tick boundaries. They
report inclusive CPU wall duration and counts/max call, not exclusive CPU time
or a full native sampling call graph. Existing native render section costs are
identified separately. Unknown GPU queries remain N/A and broken graph segments;
no inferred/summed GPU total. UI and collector overhead remain observable.

Checks: real4.7.2 OpenGL ImGui drawing, no-track global sampling, frontend and
running-game traces; actual ImGui pause/clear/export/function buttons injected
through host input; headless path; normal Debug build and strict source successor.
The game fixture validates RaceScene._present_frame and Ps2DrivingRuntime.tick
after Running, not just a loading-scene capture. Private artifacts stay in local/.

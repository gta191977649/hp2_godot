# E0069H01 - Port transport and Godot profiler integration

This is an explicitly authorized host-adapter successor to TASK0067, incorporating
unfinished TASK0068 transport work and the user's subsequent Godot4.7.2 request.
It makes no new claims about original executable behavior and does not change
recovered simulation, culling, authored LOD, visibility, RNG or native packet order.

Changes: on-demand detailed batch diagnostics; exact command-descriptor plan cache;
load-lifetime section ownership index preserving duplicate/unowned rejection;
trusted particle material notifications with first-bind/shader-change full sync;
particle staging reuse; official runtime/build/tool pins, active-driver window
title, opt-in Godot Performance monitors and engine viewport timestamp sampling.

Native header tests exercise cache misses/hits, state/order/matrix/depth/light
changes, mode changes, disabled reference planning, scene replacement, duplicate
ownership and clear. Bounded current-pose rendering tests compare encoded depth,
colour and presentation; counters/CPU wall times are not original-console evidence.
Historical TASK0063 auxiliary hashes are supplied only with explicit canonical
historical function identities by TASK0064; they are not refreshed to current
source. This successor carries those files and all inherited TASK0067 auxiliary
pins forward so a current-source edit is still checked. Prior review JSON is frozen.

The runtime upgrade retains godot-cpp's existing4.4 minimum API ABI. Actual4.7.2
extension, raw particle mesh-layout, Sun, postprocess and lifecycle tests are
required; changing the runtime does not justify claiming a console-equivalent
frame. Existing Track24 colour discrepancy remains unresolved and unrelaxed.

Engine timing limitations: CPU wall times include possible waits; viewport GPU
timestamps are delayed and may overlap. Zero/disabled results remain unknown.
Do not sum viewport durations or subtract CPU from total to invent GPU time.
Sun32x32 readback may stall on earlier work regardless of transfer size. Existing
reduction, opaque queue and MultiMesh experiments remain independent/off unless
separately shown correct and beneficial. No all-track/per-track visual fallback
is introduced by this review. See docs/PERFORMANCE_TASK0069.md for measured scope.

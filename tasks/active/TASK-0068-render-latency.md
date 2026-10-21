# TASK-0068 - Global render transport and latency

- Status: active
- Scope: approved CPU caches/diagnostics, material notifications, particle staging, consumer accounting and bounded before/after GPU capture. Official Godot4.4.1, original state/RNG/order/LOD, all-track state rules. No long FPS benchmark.
- Real exported pose: local/task0065/track-draw-616514866.json, Track11 HP2(-675.77,245.32,-44.50), actual2880x1695. Also compare1920x1080 separately. Export contains particle counters, not a full simulation checkpoint; do not claim replay of its exact transient state.
- Initial actual snapshot: main submission4.9812ms, previous Sun synchronous readback14.9857ms, native3.1946ms; these are single CPU wall-clock values, not steady-state/GPU times. Existing normal DLL saved privately in local/task0068/reference-bin before edits.
- Independent rollback switches and exact native/image tests required. Preserve historical reviews with explicit canonical auxiliary snapshots; no per-track fallback.

## Update after user-directed TASK-0069 runtime upgrade

- Engine4.4.1 constraint superseded explicitly by the user: use4.7.2 directly, no old-engine comparison workflow.
- CPU caches/diagnostic levels/particle staging/material notifications are integrated under E0069H01 and the TASK0069 strict source successor. Same4.7.2 cache-on/off pose and nearby native/pixel checks pass. All30-track CPU census completes; no30-track GPU equivalence claim.
- Profiling is now engine-driven through custom Performance monitors, viewport timestamps and --gpu-profile. Warm current-pose colour/depthGPU4.38/1.78ms and SunreadbackCPUwall5.78ms show synchronization and scene workload merit attention; shrinking transfer alone did not resolve the wait.
- Retain this task active for unresolved GPU/consumer/history experiments, native call-stack attribution and wider image coverage. History timestamp anomalies require investigation. No default opaque-queue/MultiMesh, no matrix-palette change without evidence, no blanket depth/history removal or stagedRGBA8 focus. See docs/PERFORMANCE_TASK0069.md.

# TASK0053 fixed-pose Roadshine cost isolation

## E0053P02 - Gate on/off/on observation

Actual Debug Godot4.4.1 Compatibility, IntelArcB390 driver32.0.101.9150,
1920x1080,VSyncdisabled. Independent race scene paused at original Track11
start (real car/environment capture retained), camera fixed to the earlier
water fixture(-78.8232,-72.2048,-173.3204), target(-70.8232,-75.7048,-170.3204).
Visual timestep0 holds pose/time conditions. Only Roadshine permission toggled;
other gates/distance retained. Thirty warm frames then60 measured per phase.

| Gate sequence | wall median/p95 ms | native median ms | submit median ms | scheduled draws median/p95 |
|---|---:|---:|---:|---:|
| On |24.389/27.384|1.5466|11.7333|715/731|
| Off |24.383/26.855|1.5256|11.7711|715/731|
| On again |24.916/27.779|1.5800|11.7791|715/731|

Roadshine available=true throughout, active=true/false/true as expected.
All samples retained1184 native commands,57 visible instances, zone21 and the
same13 section IDs. Packet digest was empty in update_view state, so no digest
identity claim is made. Per-target scheduled draw sum matched the global draw
monitor. The car itself is off-camera at this wet pose; its environment capture
still runs at the original start, so this is not a moving-driving acceptance.

The third On wall median drifted0.527ms from first On; the first On versus Off
median differs only0.006ms. This does not support the permission gate as the
observed dominant cost at this pose. It does not prove the whole original effect
is free: this build sampled specular_texture unconditionally, then zeroed the
specular RGB when disabled. Mandatory C3 alpha/base-mask work remained active.
Root's subsequent conditional-texture-sample optimization is a separate build
and must be measured independently. No global effect disable or shortened
visibility distance was used as a fix.

Raw scheduled GPU timer medians8.973/9.182/9.442ms show drift larger than the
On/Off difference. Some intermittent-view timers were stale or zero despite
scheduled drawing: one middle capture face retained an impossible1463ms CPU
value across phases; three other intermittent capture faces returned zero.
Aggregate timer values therefore are diagnostic only, not reliable latency
budgets. Wall clock and draw counters are the primary observations here.

Exact script/log/results/screenshots are ignored local evidence:
`local/task0053/roadshine_ab.gd`, `roadshine-ab.log`, `roadshine-ab.json`,
`roadshine-ab-{0-true,1-false,2-true}.png`.
Command:

```
local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path D:/dev/hp2_godot/godot --rendering-method gl_compatibility --script D:/dev/hp2_godot/local/task0053/roadshine_ab.gd
```

These are host performance observations, not original-console Verified results.

## E0053P03 - Vehicle environment binding cache

Code inspection found set_environment_capture cleared readiness/texture and
rebound every reflective material every frame. It also threw/caught the same
unsupported31C exception for each flagged surface on every frame. This is host
binding overhead, not original recovered vehicle physics.

The narrow host cache keys enabled/ready, texture ObjectID, load generation and
draw count. HackMan matrix finite validation and update remain live on each
valid input call, even when texture/material binding is unchanged. A binding
change evaluates the unsupported31C guard once, caches its explicit diagnostic,
and detaches those draws. next_pass is written only when changed. Supported318
texture uniforms update only for changed texture/new draw generation. Release
invalidates the cache, clears readiness and releases the held texture reference.
No original draw order, material coefficient, geometry or physics body changes.

C++ syntax check passed with current generated shader/Godot4.4.1 headers.
Existing car_surface_batch_smoke now exercises repeated unchanged capture,
changing HackMan matrix with stable texture, cached31C diagnostic, off/on,
texture replacement, malformed input and clear/reload with same texture identity.
Integrated engine execution is parent-owned and pending; no pass is claimed yet.

The complete initial gate-only script/results/log/images are preserved separately
under `local/task0053/roadshine-ab-initial/` before any subsequent build rerun,
so later output at the normal path cannot silently replace this observation.

Integrated Debug car_surface_batches lifecycle regression passed (local/task0053/native-host-regressions.log and full-debug.log), including the new binding cases. Debug/Release shared builds pass.


## E0053P04 - Final conditional-sample / binding-cache build

Same fixed wet pose,1920x1080,30warm/60samples, on/off/on and original other
effects. Actual engine command adds --audio-driver Dummy to avoid an unrelated
Windows device invalidation. Final wall medians34.973/33.445/33.791ms; p95
39.416/37.029/36.649ms. Draws715median/731p95 each. Native medians
2.3097/2.2371/2.2532ms; host submit18.8289/18.1030/17.8047ms. All gates,
asset/section/visible-instance invariants passed. GPU numeric fusion and actual
mask tests pass, including disabled Roadshine retaining base-mask blending.
The On repetition drift1.182ms limits attribution of the1.528ms first difference;
this pose does not establish Roadshine as the dominant severe stall. It does not
establish improved wall latency against E0053P02: CPU cost differs across runs.
Intermittent target CPU timers again retained impossible~2280ms readings and are
not usable frame budgets. Further CPU submission profiling remains TASK0052,
strict60FPS deferred. No hidden quality reduction or original coefficient change.
Final results/log/images at local/task0053/roadshine-ab.*, first run preserved
under roadshine-ab-initial. This is a bounded observation, not moving-route or
all-track performance acceptance.

# TASK-0072 visibility and rendering distance

The original Camera constructor defaults to near0.5/far10000 internal units.
This does not mean every object inside10000 is submitted. Original zone, tree,
instance, projected-size and authored LOD gates already drive the main host path.
There is no evidence-based global distance reduction to apply from this audit.

Fresh binary identities, addresses, limitations and private evidence locations:
[visibility evidence](../reverse/task0072/visibility.md), E-0072-V01..V04.
The Debug rear-view mirror uses near0.75/far192; sky forces100/50000. Thus a single
universal distance is incorrect. The full Track11 camera-key/coordinator chain and
all exceptional overrides are still unresolved, and no original runtime was run.

Port diagnostics now distinguish original constructor defaults, effective clip
planes and explicit force-far override. get_state.visibility_audit includes mover
type, logical ScreenWidth512 and camera_key_gameflow_recovered=false.
get_render_controls retains original_far for compatibility and adds explicit
original_default_near/original_default_far, camera_depth_source, camera_mover_type,
force_far_override, original_logical_screen_width and native_selection_bypassed.
The default values describe the bounded adapter input, not a universal PS2 claim.

For60FPS acceptance, keep Show LOD off, retain the original logical focal H,
record per-view native selection, and use identical pose/effects/override values.
Counts are latest submitted-view counters, not aggregate GPU draw counts. More
fine-grained rejection counters remain future instrumentation; no culling logic
was changed or test-equivalence claimed. Parent TASK-0072 owns builds and GPU
acceptance of the additive diagnostic fields. Static review only in this audit.

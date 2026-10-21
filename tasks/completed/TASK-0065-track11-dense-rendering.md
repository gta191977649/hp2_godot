# TASK-0065 - Track11 dense rendering

- Status: completed
- User scope: Track11 native camera position (697.04,141.24,-17.98); preserve visual state, native selection and LOD. No prolonged FPS benchmark.
- Deliver: expandable per-viewport engine counters and JSON diagnostic/camera export; conservative cached cross-matrix batches; exact Copy-cutout opaque experiment; shared depth geometry; bounded parity and backend checks.
- Constraint: local-space recovered lighting cannot be baked naively. Cross-matrix baking initially excludes lighting, alpha ordering, environment/fusion and depth-bias states; candidates remain off until pixel/depth parity.
- The screenshot does not contain camera orientation. Export captures full current pose; diagnostic sweeps must label their chosen headings instead of claiming exact screenshot reproduction.
- Check: native boundary/cache tests, Debug build, bounded Godot image/draw checks, explicit source-preservation successor. Record failures and do not promote failed candidates.

## Delivered and accepted scope

- Expandable engine viewport statistics, diagnostic JSON export and camera/control restore; no normal-use GPU readback.
- Cached raw matrix attributes reduce Track11 unlit main-view submissions and matching encoded depth. Original native command/LOD/RNG and lighting preserved. Optimized default is Track11-only with automatic baseline fallback for visual consumers/captures/nonidentity owners/HLOD.
- Final1080p seven-view default totals:2409->669,86->84,68->68,717->381,3003->689,597->317,2376->692. All colour/depth/final images and native packets equal. Exact screenshot orientation remains unknown.
- Three Track11 marker poses pass combined-cache parity. Three Track24 default fallback poses pass unchanged764/799/796draws. Viewer-to-car transition and four1080p reflection atlas/colour/depth/final checks pass after baseline fallback.
- Rejected CPU pretransform, Track24 experimental matrix batching, existing MultiMesh and opaque queue after exact image failures. Those paths are not accepted defaults; their unresolved differences are recorded in docs/PERFORMANCE_TASK0065.md.
- Mobile/Forward+ each pass seven within-backend candidate comparisons; cross-backend pixels differ, so Compatibility stays default. No timing winner or60FPS claim.
- CPU-only census covers30tracks/60rows plus14dense-heading on/off rows. Normal Debug build updates root godot/bin/ps2_port.dll.13targeted native/UI/host/source checks pass, including strict11-before/13-after successor and three fail-closed review cases. No long FPS run.
- Continuation: resolve Track24/capture matrix submission differences before broadening defaults; assess original depth integration separately. Rollback: --track-opt-world_batches=false.

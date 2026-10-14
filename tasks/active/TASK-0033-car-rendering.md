# TASK-0033 — Vehicle exterior rendering and CarViewer

- Status: active
- Target: SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Scope: runtime BIN geometry, shared textures, undamaged exterior parts and LOD; existing CarViewer integration. No physics, handling, dashboard or dynamic effects.
- Preserve the uncommitted TASK-0031/TASK-0032 work.

## Acceptance criteria

- [ ] Evidence-backed geometry, texture binding, part and LOD behavior in the correct native classes.
- [ ] Runtime loading of 911TURBO, BMWM5COP and 4DR_SEDAN from configured original assets.
- [ ] CarViewer load/clear/fixture, part visibility, LOD, wireframe and bounds controls and diagnostics.
- [ ] Native tests, Godot smoke and window checks; sequential Debug/on, Debug/off and Release checks.
- [ ] Evidence records, symbols, template coverage, viewer guide and generated progress updated.

## Findings and limits

Initial read-only planning checked the exact ELF digest. The 0011E860 allocator/initialization path is a lead for CarRenderInfo; 001AC0A8 is shared texture initialization. Diagnostic strings alone do not bind render methods. No equivalence claim is made.

## Next steps

Trace native geometry and vehicle callsites; compare accesses with original BIN bytes. Record stable E-0033 evidence IDs and implement only confirmed slices. Keep raw analysis and captures under ignored local/task0033.

## Validation

Pending. Do not mark complete until all criteria have evidence.

## Related commits

None.

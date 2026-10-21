# TASK-0057 - Track culling and generated LOD feasibility

- Status: completed
- Scope: read-only Retail rendering optimization recheck and Godot4.4.1 generated
  mesh LOD feasibility. No default rendering or physics change.
- Acceptance: address-anchored culling evidence; distinguish current backends;
  version-pinned generator/runtime availability; one-track-only feasibility and
  concrete bounded implementation recommendation. No catalogue sweep required.
- Original authority: Retail SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.

## Findings and validation

E0057R01: fresh read-only Retail00150950/00150378/0014FE18 decompilation confirms
zone/section, hierarchical AABB, exclusion/projected-size/model selection stages.
Current HLOD delegates later selection and can increase CPU candidate preparation.
E0057G01: pinned Godot4.4.1 sources confirm runtime-capable ImporterMesh generation,
ArrayMesh LOD transport and GLES3 selection; HP2 COLOR/CUSTOM0 preservation risks.
E0057G02: one Track11 only,24 real batches,5 terrain batches generated low indices;
4017->3717 triangles in sampled aggregate. Synthetic reference1LOD. No pixels/FPS
claim. Tracked probe rerun PASS GENErATOR_PROBE_OK (case-sensitive marker in log:
GENERATOR_PROBE_OK samples=24). No production function or default changed.
Documentation: docs/TRACK_CULLING_AND_GENERATED_LOD.md. Numeric results:
reverse/task0057/generated-lod-probe-track11.json. Logs ignored local/task0057.
Checks: native diagnostic target build PASS, selected asset inventory PASS,
stock Godot4.4.1 tracked headless probe PASS. No full native rebuild needed for
research-only script/docs changes. No proprietary geometry/ELF/raw decompile added.
All investigation acceptance checks met; actual generated-LOD integration is a
future opt-in implementation, not claimed delivered. No commit, no Verified.
Continuation: preserve original culling first; prototype cached single-model opaque
terrain LOD, validate seams/custom attributes/dependent passes on Track11.

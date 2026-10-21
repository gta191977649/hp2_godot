# TASK-0085 - Godot GPU particle backend

Status: Completed (implementation; extended performance matrix waived by user)
Date: 2026-10-21 Australia/Sydney

User-approved scope: retain original AcidFX emission/RNG/material rules; migrate
simulation and drawing into Godot GPUParticles3D custom shaders. No physics
replacement. GPU numeric/sorting differences are explicitly port-side. Native
backend remains default: no measured performance/default-enablement claim.

- [x] Native committed emission batches, original lifecycle/capacity/RNG tests.
- [x] GPU simulation/draw backend for world and wheel effects, lifecycle/reset.
- [x] GPU motion/state/actual-pixel fixtures and native/host regressions.
- [x] Extended 120warm/600sample/3repeat matrix stopped and waived by explicit
  user instruction to stop repeated benchmarking on2026-10-21. Not a passed gate.
- [x] Instrumentation/default decision/docs; preservation successor E-0085-S01.

Authority: Debug NFSHP2.ELF SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b for driving;
existing Retail AcidFX provenance remains separate. TASK0052/0075 remains the
recovered-code authority; new transfer/GPU interfaces are port-side.

Validation: native emission plus six driving regressions passed. Final Godot
build passed; gpu_particle_lifecycle_gpu, gpu_particle_state_gpu and
 gpu_particle_visual_gpu passed3/3 in26.25s. Flat burnout/drift actual pixels and
Track11 lifecycle checked on Vulkan Forward+ Intel ArcB390 Godot4.7.2. See
reverse/task0085/emission.md, gpu_renderer.md and docs/PERFORMANCE_TASK0085.md.
No original-game runtime reference or Verified promotion. No new original
function/stub completion. Incomplete/invalid benchmark captures remain ignored
local/task0085; no timing acceptance or speedup claimed. No commit made.

Next: TASK-0086 original vehicle flare reconstruction.

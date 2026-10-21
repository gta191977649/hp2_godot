# TASK-0086 - Original vehicle light flares

Status: Completed (PC-tested; original runtime equivalence unverified)

User-approved scope: recover original CreateCarLightFlares/Render/eRenderLightFlare/
RenderSingleFlare data and state; original lamp marker positions/normals, parameter
indices, texture,size,colour,angle/intensity/depth rules. Attach persistent billboard
instances via GPU particle management, no fabricated emission or road beams.
Connect head/brake/reverse/cop/horn/night in both driving consumers. Missing data
explicitly reports gap. Native/host visual/lifecycle tests and strict preservation;
retain address/SHA/version/evidence records; no Verified without original runtime.
Continue from reverse/task0082/lights.md and private local/task0082/lights captures.


Scope acceptance:
- [x] Original marker decoding, normals, parameter pairs and textures.
- [x] Godot-independent state/flare producer with native regression tests.
- [x] Attached persistent billboards in Test drive and standalone driving.
- [x] Day/night, horn/brake/reverse/cop, angle/distance/depth/reset checks.
- [x] Evidence/index, strict preservation, task/progress/docs updates.
No additional benchmark requested or planned. No proprietary assets/raw ELF inGit.
Debug authority SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
version remains independently unknown. Original runtime comparison unavailable.


Validation: native vehicle_flares1/1 (2.79s), vehicle_markers1/1 (2.39s),
vehicle_model1/1 (10.22s), driving_backend/player2/2 (15.89s). Final Godot build
passed. GPU flat/Track11/live-node-exit vehicle flare tests3/3 passed37.33s on
Godot4.7.2 Vulkan Forward+ Intel ArcB390. Isolated occlusion9920->0pixels; front,
night, brake, reverse and cop screenshots inspected. Actual BMWM5COP horn
activation and changing original cop phases checked in native and host tests.
Driving camera/asset/pipeline regressions passed; independent runtime regression
passed after test-only stale horn-signature/projection checks were corrected.
No production race camera change. progress_tables passed2.26s. See
 docs/VEHICLE_LIGHT_FLARES.md and reverse/task0086 for evidence and limitations.
Strict source successor E-0086-S01 passed:7/7 tests280.215s and existing_body_preservation1/1 in294.43s.
No further benchmark, no proprietary files staged, no commit made.


Final strict review SHA256
5eb78631b86cd079c7074730ebf2cec463eb8f988d520bfa6224ac50f54b9b40.
Frozen TASK-0085 review unchanged;8replacements/19additions, no original stub
transitions,7inheritedaux/13newpins. Earlier cancelled/classification runs are
superseded by these final passes. No required implementation work remains here.
Next optional work: original-runtime frame comparison and wider renderer global
alpha/DMA equivalence under TASK-0052; do not restart performance sampling without
a new user request. No commit made.

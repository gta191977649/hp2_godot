# TASK-0087 - Track12 particle and driving gaps

Status: Completed
User report: Track12 repeated non-progressing original emitter interval,
missing driving wheel-blur resource, and unsupported water/puddle terrain.

Scope: reproduce actual Track12 resources; recover/correct each reported path
from original ELF/assets. No silent skips or invented terrain/physics fallback.
No performance matrix. Debug driving SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
Retail effects SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Version independently unknown. Preserve prior dirty workspace and frozen86 review.

- [x] Reproduce Track12 emitter guard and original interval semantics.
- [x] Recover missing wheel-blur binding behavior from original resources/code.
- [x] Recover water/puddle contact dependencies; implement only evidenced slices.
- [x] Focused native/host regressions, strict successor, docs/progress updates.

Read AGENT.md before edits. Raw captures/assets stay ignored local/task0087.
Unknown dependencies remain explicit NotImplemented. No Verified claim.

Findings: E-0087-P01 preserves original zero-interval retry/RNG semantics with
an explicit throwing port budget. E-0087-B01 permits absent optional original
wheel-blur solids. E-0087-W01 retains original terrain4/5/20 and recovered
native wheel/static-contact response. Water-specific wipers/splash/replay remain
unrecovered; no broader water-effects completion claim.

Validation: focused native emitter, optional blur, terrain/static contact and
driving regressions passed. Final normal DLL build passed after the user closed
the locking game. Final track12_driving_gaps_host/gpu2/2 passed29.47s using that
DLL; no performance benchmark. Commands, versions and evidence are recorded in
reverse/task0087 and docs/PROGRESS_LOG.md. GCC13.2/CMake3.29.2/Ninja1.12,
Python3.14.4, Godot4.7.2 Vulkan Forward+ Intel ArcB390;
godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b.

E-0087-S01 passed: unit7/7 in291.455s; existing_body_preservation1/1 in293.52s. Review SHA25665248eca7c5d4f621fc9c52cb838aec312ef7f5b974de3a2c27250c1c8ea68fc. Frozen86 unchanged; no original stub transitions. Progress generator/check and progress_tables passed. Next optional action: original-runtime comparison or recover remaining water-specific effects; neither is claimed complete here.
No commit made; pre-existing workspace changes preserved.

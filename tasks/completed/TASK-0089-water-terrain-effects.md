# TASK-0089 - Original water terrain effects

Status: Completed

User requests recovery and implementation of water splash, timed wipers and
remaining water terrain effects after TASK0087 generic contacts. Debug
NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
version unknown. Begin from Car::TriggerCarTerrainEffects00133988 and
reverse/task0087/water.md. Read-only Ghidra and original assets, no invented
wiper geometry, splash texture or replay policy. Keep native logic Godot-free.

- [x] Recover water branch, dependencies and original assets; record replay and
  full-world scheduling boundaries explicitly.
- [x] Implement recovered state, particle/audio and visible wiper consumers.
- [x] Focused native/host lifecycle tests, strict review, evidence/progress.

No benchmark or Verified claim. Activated after TASK0088 completion.

Findings E0089W01..06, E-0089-H01: original DriveOver1/2 water spray groups,
body query, intensity blending and pair midpoints; native timed wipers and
Splash1/2/3 cooldown/RNG; original34012 rigid-animation matrices and optional
wiper solids. Use raw original frame/on/off state, not a generated sine phase.
Original missing-wiper texture lookup19006835 is evidence backed. No proprietary
assets/captures enter Git. MUDPUDDLE authored emitter lists remain empty.

Native water/particle/physics regression11/11 passed65.01s; final null-sound guard
water test1/1 passed31.55s. Controlled original911TURBO+GLOBAL water4/5/20 tests
cover480frames each, native deterministic physics/audio/wiper/reset and spray.
Final native wiper/geometry/model3/3 passed11.87s after fixing passive external
animation loading. Initial failure and counterevidence remain in evidence notes.
Normal DLL build and water host/GPU tests passed; independent race scene final
regression passed2/2 with water host21.07s. Strict E-0089-S01 passed7/7 unit cases and preservation1/1 (authorized environment-timeout retry349.32s). Exact commands/tool versions in
reverse/task0089/{water-research,wiper-consumer,host-consumers}.md.

Limits: current own-car listener and bounded World ordering; original replay
capture is not port input recording. Damage/nitro/flat-tire branches and separate
rain-screen RainSplash remain unrecovered. Active unresolved required animations
throw; passive external references remain explicit metadata/diagnostics.
Final review SHA256a9fdb1c94b8c4902d5c723203d11c7c17b6764b888db6a2ab32f56d8afb846ce. Frozen88 unchanged;17 replaced/12 added functions, no stub transitions. Initial preservation timeout10227.66s and unit wall15152.077s reflect environment suspension; no performance claim. Progress generation/check passed. No commit or proprietary staging. Optional next work: original runtime comparison, original World/replay ownership or separately scoped RainSplash recovery; no further benchmark without a new request.

# TASK-0071 - Vulkan Forward+ performance migration

- Status: active
- User authorization: default Vulkan + Forward+, no automatic fallback; preserve recovered PS2 effects/state/LOD/physics. Migrate global ProfileDebug. Track11 only, bounded paired measurements, not a 30-track acceptance claim.
- Baseline: existing uncommitted user work preserved under ignored local/task0071/baseline with SHA256 manifest, original DLL, Git status and source identities. Godot4.7.2.stable.official.ed1daf0bf; godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b.
- Scope: port-side backend configuration, GS colour-domain transport, viewport/depth/reflection/particle compatibility, measured host/GPU optimizations, profiling and reproducible evidence. No new recovered gameplay behavior or quality reductions.

## Acceptance

- [x] Strict Vulkan/forward_plus default and actual-runtime checks; explicit OpenGL diagnostic baseline remains possible.
- [ ] Existing supported material/effect paths work without new backend-only NotImplemented failures; meaningful GPU fixtures pass.
- [ ] Global profiler UI, scope timing and export work; driver/method metadata and unavailable timestamps are explicit.
- [ ] Track11 fixed ordinary/dense poses and short route:1080p,120 warm frames,600 samples,three repeats, VSync/cap off. Compare median/P95/P99, native/host/GPU/readback. No inferred gain.
- [ ] Native packets and scoped pixels compared; finite PS2/PCSX2 comparison documented or prerequisite blocker retained.
- [ ] Debug/Release checks, source preservation and generated progress recorded; no proprietary inputs/captures staged.

## Findings and next action

Confirmed: migrated the prior Compatibility-only vehicle blending/reflection and skid adapters to numeric-GS offscreen HDR transport. Actual Vulkan vehicle/fusion/presentation/particle/Sun/sky/postprocess/lifecycle/reflection fixtures pass in the bounded scopes recorded in docs/PERFORMANCE_TASK0071.md. Read-only Ghidra reread Retail00108CC0/0010AC50 and Debug00108AE0 against their separately hash-checked ELFs. No recovered logic changed.

Confirmed limitation: HDR does not implement per-primitive COLCLAMP; clamping on resolve cannot prove original equivalence. Dense/nearby packet digests/counts agree across backends but pixels differ. Global profile startup exported120 samples on default Vulkan/Forward+; full game/UI migration acceptance remains pending.

Next: validate --track-source-3d-skip=true (default-off candidate) in the isolated candidate project, finish paired Track11 routes without competing user GPU work, isolate pixel differences, run Release checks and obtain a matched original Track11 reference. Fixed-start timing improved32.737->24.277ms median of repeats, but baseline spread8.518ms narrowly exceeds paired median gain8.460ms: conservative performance gate fails. No accepted FPS gain or Verified claim. Existing effects gaps remain explicit.

## Build-lock correction

The interrupted workspace benchmark retained ps2_port.dll. The user's subsequent
build compiled its sources but failed linking with ld "cannot open output file
...ps2_port.dll: Permission denied". Stopped only the owned TASK0071 matrix runner
and child benchmark. `./build.ps1` then configured/linked successfully (exit0).
Bounded normal frontend launch with --gpu-profile --profiling --profile-debug
on default Vulkan/Forward+ exited0 and exported120 samples to ignored
local/task0071/build-fix-profile.json. No source compiler error remained.
The unfinished matrix case is excluded; subsequent graphical checks must use
local/task0071/candidate/godot rather than locking the user's workspace DLL.

## Related commits

None created.

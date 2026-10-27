# TASK-0098 frontend effects dependency

Authority: Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
revision/date unknown. Original behavior comes from read-only Ghidra MCP and
configured original assets. Raw captures remain ignored under
`local/task0098/frameeffects/`; hashes are in `evidence.json`.

## Confirmed E-0098-P01: mutable frontend bases

`AcidEffectSystem::DoTimeStep00128870` processes auto-emitters, then in frontend
state3 rebuilds the global emitter-definition colour and extra bases, then
integrates particles. Definitions are not immutable in this path. The existing
`ParticleSimulation` had omitted the frontend branch; updating ColourMatrix alone
would leave displayed colours unchanged.

`fDeterminant001261E0`, `fInvertMatrix00126378` and `hermite_basis00126AB8` are
implemented in `src/acidfxe.cpp`. E-0042-S001's exact ECOFF marker for fDeterminant
establishes `src\acidfxe.cpp`; the companion native header is a port declaration.
The24-term determinant, alternating cofactor forms, and three original eMulMatrix
calls retain their arithmetic ordering. Do not replace these with affine
bInvertMatrix, a host inverse, or algebraically cancel Mf/iMf. Inverse aliasing,
singular/nonfinite and exceptional host domains throw; the original frontend
Hermite call uses distinct inverse source/destination.

Each key is scaled by00324C84 bits3EAAAAAB. Extra X uses Size; extra Y uses
RelativeAngle times00324C88 bits43360B61. The original extra-control stack Z/W
lanes are unwritten. Only consumed X/Y output lanes are implemented; the port
keeps unconsumed stored lanes unchanged and asserts no original values for them.
Scratch zeroes used for the discarded computations are explicitly port-side.
Colour output implements all four initialized lanes.

`ParticleSimulation::step(...,frontend=true)` performs the rebuild between auto
emission and integration inside the candidate transaction. A later bad definition
cannot publish earlier updated bases. Existing default driving/world stepping is
unchanged. The frontend GPU branch explicitly throws until dynamic basis transfer
is bound; the supported frontend branch uses native particles.

## Confirmed E-0098-P02: original selector nebula

`CarSelector::UpdateNebula00193E60` now updates decoded native state and typed
emitter handles. It preserves the original .01 transition globals, locked/unlock-
all branches, clamp, integer65536/count step, angle+16384 rotation,15-unit ring,
Z=-1.25/W=1, null-emitter skip and shared ColourMatrix writes. Colours project
the original00196688 static stores rather than reading zero-filled BSS.

The constructor00192B78 allocates20 zeroed State1 emitters sharing the
`XE_SelectCarNebula` definition. Active auto-emitter insertion reverses their
construction traversal. UpdateNebula occurs once there and again in the final
constructor Tick. Its definition resides in the actual GLOBAL/GLOBALB.BUN
library; the initial test incorrectly expected FRONTB and failed. The corrected
test reads the reviewed original library.

ColourMatrix updates precede drawing, but ColourBasis updates occur in the fixed
FrameEffects step after render queues execute (00324A48 bits3C888889). Preserve
this presentation delay. The production constructor/render loop is not bound by
the new state/math methods, and must not move stepping before drawing.

## E-0098-P03: reached zero-speed VU normalization

The real nebula has Speed0 and nonzero MotionInherit with zero initial inherited
velocity. A native GDB trace exposed host NaNs from per-component0/0, eventually
reported by the generic CVT guard. Fresh original001284F0..00128534 instead
computes sum-of-squares, scalar SQRT, VRSQRT at00128510, then vector multiplication
at00128524. The port now mirrors reciprocal multiplication and routes the reached
zero operand through an explicitly port-side instruction carrier. No epsilon,
new particle direction, rate or RNG call is introduced.

As an ISA implementation reference only,
[PCSX2 v2.8.0 VUops.cpp](https://github.com/PCSX2/pcsx2/blob/v2.8.0/pcsx2/VUops.cpp#L893)
gives finite signed saturation for the nonzero-numerator/zero-denominator RSQRT
case. It is not original-game behavior evidence under AGENT.md R0. This narrow
port instruction decision follows the existing EE zero-denominator policy;
general VU exceptions, flags and original numerical equivalence remain open.

The native test verifies zero packed direction at birth. An initial post-step
zero-direction assertion was wrong: original nebula Gravity=-.05 changes direction
during the subsequent integration. The assertion now checks the actual birth
boundary without changing original gravity or integration.

## Confirmed E-0098-P04: shared native pool transaction and teardown

Original afxInit00126EC8 creates global particle1000 and group400 pools with
overflow disabled. Emitter600 is initial capacity, not a maximum: NewSlotPool
00251110 retains overflow/zero-fill and Malloc00251500 expands by(n>>2)+1.
No600-emitter hard cap is introduced. AcidRandomSeed00326D78 is shared.

`SharedParticleState` is port-owned shared accounting and RNG. Constructor
binding preserves sequential original seed consumption. `step_shared` stages
the simulation, subtracts other systems' occupied slots from the two original
budgets, and publishes simulation/counters/seed only on success. Copy the ledger
with all participating simulations for a larger World transaction. This interface
supports shared auto-emitter stepping; the future World dynamic-trigger path and
global cross-system definition ownership still require their explicit adapter.
No assertion of an implemented whole FrameEffects/WorldEffects lifecycle follows.

Once associated with shared accounting, the simulation rejects ordinary step
without its ledger. Unbound shared dynamic triggers/cleanup throw explicitly
instead of silently acquiring a private capacity. Shared teardown uses its
ledger; original list detach itself does not release particle/group ownership.

RemoveAutoEmitter001277E0 detaches without killing groups. KillAllActiveEffects
00127800 kills the system's whole active group list without changing RNG or
definitions. Native methods preserve this distinction; shared teardown releases
only the owning system's pool counts. Actual pointer/slot allocation and emitter
destruction are separate from stable port index storage.

Original selector destructor00193278 removes/destroys emitters, kills ALL
FrameEffects groups, dismisses popup, releases platform/swatches, preloads the
selected car twice, unloads loaded/loading cars, clears overrides, destroys
camera/main/reflection, releases cover, removes input handlers, restores menu
emitter state, stops sound and destroys alpha groups. Its complete production
ownership binding is still required.

## Validation and remaining boundaries

Tools: CMake3.29.2, GCC13.2.0, Python3.14.4; pinned godot-cpp unchanged.

- `cmake --build --preset native-debug --target frontend_effects_tests ultimate_selection_tests ultimate_selection_camera_tests ultimate_selection_widgets_tests ultimate_selection_lifecycle_tests selection_locked_tests driving_particle_tests gpu_particle_emission_tests track12_particle_tests --parallel 4`: passed.
- `ctest --preset native-debug -R '^(frontend_effects|ultimate_selection|ultimate_selection_camera|ultimate_selection_widgets|ultimate_selection_lifecycle|selection_locked|driving_particles|driving_particles_original|driving_wheel_particles_assets|gpu_particle_emission|track12_particles)$' --output-on-failure`:11/11 passed23.41s with configured original data.
- `cmake --build --preset godot-debug --target ps2_port --parallel 4`: passed.

After the final single-candidate/shared-accounting guards, effects/GPU-emission/
driving3/3 passed6.56s and the normal Godot DLL rebuilt. Final source-review1/1
passed7.50s; progress contracts2/2 passed2.71s; full diff check passed. The initial
math compile failed because cmath was included below the new bodies; its include
was moved before use. All failures described above were corrected before final
validation; original comparison and integrated acceptance remain blocked/incomplete.

Fixtures cover non-affine inverse identity, interpolation between all controls,
output aliasing, singular failure without publication, unknown extra lanes,
default-world isolation, atomic bad-definition failure, real library and nebula
control points, both original constructor colour steps, actual emission, zero-
speed birth, reversed auto order, shared1000-particle/400-group exhaustion across
two systems, release/reuse and inconsistent ownership rejection. No graphical
comparison, full selection entry, complete shared World/police/outcome/save,
runtime equivalence or performance acceptance is claimed. No Verified.

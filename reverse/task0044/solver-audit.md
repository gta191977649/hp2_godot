# TASK-0044: Debug wheel, suspension, body and integration audit

Target: `NFSHP2.ELF`, SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`. Historical comparisons use `SLUS_203.62`, SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`. The user selected Debug behavior. These findings are static recovery, **not Verified equivalence**.

## Evidence and search boundary

- **E-0044-S01**: fresh read-only Ghidra function metadata, decompilation and explicitly bounded instruction listings for the existing wheel/tire/contact, suspension, body, component integration and force-loop closure. Fifty-two records with original signatures, exact source ownership markers, direct call targets and capture sizes are in [solver-audit.json](solver-audit.json). Ownership references retain E-0042-S001 rather than treating decompiler source comments as authoritative. Raw responses remain ignored in `local/task0044/solver`.
- **E-0044-S02**: paired GP memory instructions, matched in ordered functions by instruction opcode and destination register, followed by live Ghidra memory reads of both operands. [solver-globals.json](solver-globals.json) records 84 instruction pairs and direct Debug addresses. All 71 distinct initial four-byte values in this paired set match Retail. This is a bounded scalar-data comparison; it does not establish equivalent execution or cover all static data.
- **E-0044-S03**: live Debug data types for eight classes/configurations. The JSON records verbatim field names, types, offsets and lengths, including inherited list links and vtable pointer.
- **E-0044-S04**: live Debug constructor instructions and vtable memory reads. The JSON records signed adjustors and targets for used slots. No database annotation, rename, type edit or function-boundary mutation was performed.

The Debug GP is `0x0032c9f0`. There is no universal old-to-new scalar delta: for example Retail `0x00330cd0` becomes Debug `0x00327d50`, while `0x0032e660` becomes `0x00325734`. Historical helper offset identifiers now route through the explicit audited lookup owned by root; direct canonical accesses use Debug addresses.

## Original owners, layout and dispatch

| Original owner / source | Debug entries used by driving | Debug size and operative state |
|---|---|---|
| `Wheel`, `..\indep\src\wheel.cpp` | constructor `001a16e8`; full state `001a1948`; reset `001a1b30`; load exchange `001a1c80`; contact `001a1d58/001a1d80`; derivative `001a1e88`; brake lock `001a1f50`; pilot `001a1ff0`; tire guess/lateral/modify/finalize `001a2108/001a2500/001a2660/001a26b8` | size `0x190`; WI/TI/CI/CS `+0xc/+0x10/+0x14/+0x18`; State `+0x1c`; inertia `+0x24`; matrices `+0x30/+0x70/+0xb0`; GroundForce `+0x114`; ContactNormal `+0x140`; Terrain `+0x150` |
| `Suspension`, `..\indep\src\suspension.cpp` | constructor `001a3680`; calibrate/reset `001a3768/001a3790`; auxiliaries `001a37a0`; load/force `001a3b10/001a3b50`; derivative `001a3ba0`; impulse `001a3be8` | size `0xb0`; State `+0x14`; Force `+0x1c`; Position `+0x20`; RestPosition `+0x80`; pWheel/pBody/pSteering `+0x90/+0x94/+0x98`; GroundForce `+0xa0` |
| `RigidBody`, `..\indep\src\rigidbody.cpp` | constructor `0019a2d8`; inertia/state `0019a580/0019a5b0`; force/frame/moment/impulse `0019a6b8/0019a7a8/0019a820/0019a838`; derivative/reset `0019adf8/0019b170` | size `0x180`; State `+0x10`, 32 floats; force/moment accumulators `+0xf0/+0x100`; mass/inverse `+0x118/+0x11c`; inverse tensor `+0x120`; CoG `+0x160` |
| `PhysicsSystem`, `..\indep\src\componentphysics.cpp` | constructor `00199a98`; append `00199b60`; Euler `00199bb8`; derivative `00199f98`; auxiliaries `0019a130`; force `0019a220` | size `0x10`; list `+0`; count `+8`; own vtable `+0xc` |

The above operative Wheel/Suspension/RigidBody fields retain the historical Retail offsets. This does **not** justify treating other Debug classes as Retail-compatible: engine, drivetrain and PhysicsCar layouts are independently audited by other workers.

Vtables: Wheel `0x002ada00`, Suspension `0x002ada90`, RigidBody `0x002ad1f0`, PhysicsSystem `0x002ad160`. Component slot pairs remain adjustor/target: count `+0x10/+0x14`, state pointer `+0x18/+0x1c`, derivative `+0x38/+0x3c`, auxiliaries `+0x40/+0x44`. Force dispatch is `+0x30/+0x34`; Wheel and Suspension inherit empty `PhysicsComponent::ResolveForces` (`0019a238`), while RigidBody selects its own empty `0019b2a0`. PhysicsSystem's own vtable is a different layout: Euler calls its `+0x10/+0x14` auxiliaries slot. All used captured adjustors are zero; implementations still retain explicit signed adjustment semantics.

`Suspension::GetDerivative` is exactly `001a3ba0..001a3baf`, storing two zero words and returning the output pointer. `GetPointVelocity` starts at `001a3bb0`; it must not be attributed to the derivative because a larger decompiler range contains the adjacent helper. Similarly, Retail `001a46b0` required an explicit listing range through `001a4777`; the default function query did not provide a usable body extent. No Ghidra boundaries were changed.

## Algorithm and constant findings

Forty compared function pairs have identical ordered instruction opcode/register streams after replacing numeric operands with placeholders. This narrow comparison detects relocation-compatible code shapes; it deliberately does not claim equal numeric operands, helper semantics or runtime results. Canonical math uses the separately audited Debug helpers, including Debug `bATan`'s exact zero guards rather than the former Retail small-sum threshold.

The wheel solver retains the original brake/e-brake torque order, local velocity transform, slip stores, branch comparisons, scalar multiplication grouping, five-pass force iteration, force finalization and contact-frame selection. Body force accumulation retains its 1200 force cap, then computes the moment using the original uncapped force, then applies the separate 100 moment cap. Impulse accumulation uses the capped input instead and retains its 1000/10 caps. Fourth-lane behavior is explicit: some vector operations preserve an existing W, matrix-vector transforms read Debug `bUnitVector4` (`0x00317b20`), and original uninitialized stack W is a port-supplied projection parameter rather than a claimed original constant.

`SetSliderValue(int,float,float,float)` at Debug `001a16d8..001a16e3` reserves/releases stack and returns. Canonical wheel finalization retains the call to this proven empty function. The call also existed in Retail; an early suggestion that this was a Debug-only addition was incorrect and is retired here.

Table pointers require distinguishing headers from sample arrays:

| Exact Debug name | Header or pointer array | Sample storage / call anchor |
|---|---|---|
| `AngularMomentumDamping` | header `0x00308518` | samples `AngularMomentumDampingTable` at `0x00308500`; argument formed by `0019afec/0019aff8` |
| `MotionDampingTable` | header `0x00308550` | samples `MotionDampingTableValues` at `0x00308530`; argument `0019b064/0019b06c` |
| `CorneringForce` | header `0x00308658` | samples `0x00308638`; argument `001a2534/001a253c` |
| `LoadSensitivityTable` | pointer array `0x003087c0`, seventh pointer `0x003087d8` | each word points at a Table header initialized by Debug `001a2fc0` |
| `TerrainTypeInfoTable` | rows start `0x003102e8`, stride `0x90` | static/dynamic/rolling coefficients at row `+0x20/+0x24/+0x28` |

Counterevidence: the first body migration incorrectly passed sample address `0x00308500` as a Table header. The identity-bound Debug backend rejected this on its first step. Root corrected the argument to `0x00308518` using the initializer and exact call argument listing. The sample-array symbol name alone was insufficient evidence. Other direct body/tire Table call arguments were then independently checked against their call-site address formation.

## EE numerical boundary

Debug DIV.S sites are preserved as scalar operations, including:

- wheel guess `001a2384/001a238c/001a2440/001a2444/001a2470`;
- lateral tire `001a2570/001a2590`;
- finalizer `001a27bc/001a27e8/001a2804`;
- wheel derivative `001a1ef8/001a1f38`;
- terrain projection `001a1e64`;
- body inverse mass `0019a59c`, state tensor reciprocals `0019a644/0019a648/0019a64c`;
- force iteration `00139008/0013900c/0013901c/00139020`.

These instruction addresses are not evidence that host IEEE floating point reproduces every EE exceptional case. The existing narrow port projection handles finite numerator and signed-zero denominator where explicitly used by drivetrain. General EE NaN/subnormal/overflow modes and arbitrary zero-mass/zero-radius/zero-inertia configurations remain outside the established port execution scope. Tire finalization mode 3 has `VRSQRT` at `001a2ac4`; the port rejects degenerate/nonfinite projected basis lengths with an addressed `NotImplemented`, rather than inventing a fallback basis. Original synchronized numerical traces remain required for equivalence.

## Canonical implementation and checks

The working algorithms now belong to `hp2::recovered::Wheel`, `Suspension`, `RigidBody`, `PhysicsSystem`, plus their original free helpers `GetPointVelocity` and `VectorStarMatrix`. These classes bind explicit `port::driving::ObjectView(Memory&, PS2_address)`. Encoded pointers and host arrays are explicit port interfaces; they do not assert a native host/PS2 ABI. Constructors expose `port_initialize` because host object lifetime is separate from original allocation and initialization. Historical working functions forward to these implementations. Bounded leaf projection overloads share one algorithm body in the original class; the obsolete separate composite force-loop implementation is removed by the PhysicsCar worker.

`MaybeMakeSkids` and body debug-print services remain addressed untranslated paths. Synthetic plane contact, allocator, performance counter services, fixture input and clock policies remain port services, not recovered world geometry or original startup.

Validation before final integration: `cmake --build build/native-debug --target driving_tire_tests driving_init_tests driving_solver_tests -j 4` passed. `ctest --test-dir build/native-debug -R 'driving_(tire|init|solver)$' --output-on-failure` passed 3/3 in 0.39 seconds. GNU C++ 13.2.0 (MinGW-W64 x86_64-ucrt-posix-seh r8) from `C:/Strawberry/c/bin/c++.exe`, CMake 3.29.2 and Ninja 1.12.0, native Debug build. After that run, tire fixtures were switched to canonical Wheel member calls, obsolete composite fixture assertions were replaced with canonical Memory-bound body force and bit-preserving state transfer checks, the body fixture was migrated to Debug and augmented with static Table-header/sample-pointer and untouched-W assertions, and direct table-header correction occurred during root integration. Root owns the serialized final rebuild/test pass; no later pass is claimed here. No original PS2 comparison was performed by this worker.

Continuation: run the three scoped tests serially after root finishes linking; run the identity-bound Debug backend acceptance suite, asserting initialized Table headers point to their sample arrays. Review [solver-audit.json](solver-audit.json) and [solver-globals.json](solver-globals.json) for every address used when expanding the closure. Keep TASK-0044 active until the parent task's full acceptance checks and runtime comparison or concrete blocker are recorded.

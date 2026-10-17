# Debug vehicle driving audit (TASK-0044)

Driving now targets Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
The user authorized this behavioral baseline. Its release date/revision is not independently
established. Historical Retail `SLUS_203.62` records remain separate. This is a bounded
reconstructed driving closure, **not original-runtime numerical equivalence**.

## Inventory and evidence

The exhaustive [driving ledger](../reverse/task0044/driving-audit.json) classifies all
151 former driving projection entries against baseline `8c7efde`: 130 canonical forwarders,
one transitive forwarder, nine memory/dispatch adapters, six retired composite/dispatch
entries, one synthetic-world boundary, three constant/empty compatibility leaves and one
bounded scalar packed-time projection. No entry is silently treated as a whole original
function. Supporting dependencies have 207 unique Debug address records in
[the Debug index](../reverse/task0044/debug-symbols.csv) and the main symbols index, with
185 Implemented and 22 Decompiled records; none is Verified. The narrower closure and
unsupported branches are stated per record, not implied recovered by the presence of a name.

Read-only Ghidra decompilation was checked with instructions, encoded virtual targets and
adjustments, imported field types, ECOFF source markers and actual data operands. No database
mutation was used. Detailed per-function signatures, callers/order, offsets, source owners,
listings/capture hashes, differences and exclusions are in:

- [PhysicsCar / Car / scheduling / input](../reverse/task0044/physicscar-audit.md),
  [manifest](../reverse/task0044/physicscar-manifest.json).
- [Engine / DriveTrain / steering / aero / tables](../reverse/task0044/engine-audit.md),
  [92-function inventory](../reverse/task0044/engine-audit.json),
  [source ownership](../reverse/task0044/engine-source-ownership.json).
- [Body / wheel / suspension / solver](../reverse/task0044/solver-audit.md),
  [function inventory](../reverse/task0044/solver-audit.json),
  [static-data ledger](../reverse/task0044/solver-globals.json).
- [Math and static initializers](../reverse/task0044/math-audit.json),
  [explicit global bindings](../reverse/task0044/debug-global-bindings.json).

Per-address records live under
`reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/`.
Private raw queries, ELF, runtime reports and screenshots remain outside Git under `local/`
or external source directories. Source paths and symbol names establish ownership; they do
not establish native C++ ABI or equivalence.

## One implementation and call chain

Godot CarViewer input and presentation -> port Session / NativeBackend -> canonical
`hp2::recovered` classes. The production backend directly invokes:

1. Debug static initializers, `ResetWorldTime`, synthetic fixture preparation and
   `PhysicsCar::Initialize` (explicit host binding of Debug constructor `001370C0`).
2. Per step: input stores, `PrepareRealTimestep` (`001A0C50`), `AdvanceRealTime`,
   `PrepareWorldTimestep`, `PhysicsCar::Move` (`00137D18`).
3. Original Move/substep chain, component derivatives/integration and force resolution;
   `PhysicsCar::ResolveForces`, `IterateForces` and `UpdateCar` publish original state.
4. Tool snapshot validation/publication, `AdvanceWorldTime`, then atomic state commit.

The canonical bodies live in the evidenced original source files: `physicscar.cpp`,
`car.cpp`, `carstate.cpp`, `engine.cpp`, `suspension.cpp`, `wheel.cpp`, `rigidbody.cpp`,
`componentphysics.cpp`, `aerodynamics.cpp`, `table.cpp`, `carinfo.cpp`, `timer.cpp`,
`track.cpp`, `bMath.cpp`, `bMatrix.cpp` and `ecstasy/emathe.cpp`. In particular,
`TwoWheelAckermanSteering` belongs to `suspension.cpp`, as its Debug source marker says.
The older typed compatibility functions forward to these bodies. The separate composite
solver implementation and its dependency stubs have been removed. Original classes do not
call back into a second Test drive physics implementation.

`ObjectView(Memory&, uint32_t)` is an explicitly port-side binding to logical PS2 storage.
Its inheritance, extra Memory/service arguments and `port_initialize` factories do not
claim original constructor ABI, object lifetime, allocator or host-pointer compatibility.
Original virtual dispatch reads Debug vtable words and applies the evidenced subobject
adjustments; unknown targets throw. The backend's service closures supply external seams,
not an alternative force/integration algorithm. Recovered C++ remains independent of Godot.

The class generator keeps historical Retail attributions but recognizes 14 explicitly
superseded canonical Debug owners, rather than regenerating obsolete no-argument gaps or
crediting Debug bodies to Retail addresses. Symbol updates preserve other binary rows even
where numerical addresses collide. The [migration review](../reverse/task0044/source-migration-policy.md)
pins 249 replaced before bodies and 383 after bodies, plus evidence and reasons. Its checker
has no accept-current mode. Historical preservation checks still apply to the pinned
TASK-0043 tree; intentional Debug changes are not described as unchanged Retail code.

## Material differences corrected

- DriveTrain uses Debug shift tables at `+0x4C/+0x70`, rather than Retail `+0x50/+0x74`.
  Debug clutch torque has no Retail previous-road-torque history/halving branch. The
  derivative clutch-state table is `003077C8`; leaving Retail `00310050` caused a real
  throttle override/low-speed plateau and is covered by a regression.
- Debug boost omits the Retail damaged-AI engine-scaling branch. Car damage/blown/reset/
  blinking/poof offsets were checked individually, rather than shifted uniformly.
- Damping calls pass Table headers `00308518` and `00308550`, not sample arrays
  `00308500` and `00308530`. Static initializer and table-pointer regressions cover this
  distinction; the first incorrect migration was rejected on its first native step.
- The canonical clock uses Debug globals and PAL/NTSC bounds, original quantization and
  packed-time advancement. Timer/profile counters and simulation elapsed time are separate.
- Fourth lanes preserve the actual operation's stores. Homogeneous transform addition uses
  Debug `bUnitVector4` at `00317B20`. Uninitialized stack fourth lanes remain explicit port
  projection arguments, not invented recovered constants. Debug bATan does not inherit the
  Retail tiny-magnitude guards.
- CarState directly invokes full Debug `eRotateY/eRotateZ`, including exactly one original
  matrix multiply. Brake matrix translation retains original wheel position; tire matrices
  additionally include suspension/render displacement. Yaw boost reads mutable Debug globals.
- Component initialization/order, vtable words/adjustments, force limits and moment ordering,
  five-pass solver, state transfer, steering direction and brake/publication sequencing have
  instruction fixtures. These fixtures do not establish EE floating-point equivalence.

## Explicit tool policies and incomplete game paths

The loader accepts only the full Debug hash via `HP2_PS2_DEBUG_ELF` or ignored
`config/ps2_debug_elf.local.json`. There is no Retail fallback or universal address delta.
The compatibility global helper resolves each historical operand key through an audited
switch and throws on unknown keys; it never reads a Retail global from Debug memory.

Test inputs remain tool choices: zero initial motion, identity orientation, geometric
placement on a synthetic flat plane, Race stage 4, NO_CONTROL/raw control injection and
forward automatic operation. The fixture sets CarState gear at +0x388 to raw 2 (forward), desired gear at
CarDriver +0x220 to -1 (automatic request), and sport gear at +0x224 to raw 1.
Original constructor/state restoration and subsequent automatic changes are covered by
initialization and drivetrain tests; these chosen inputs are not original race startup. This is not reconstruction
of whole Car, World or RealDriver startup. Normalized A/D steering requests use a tool
maximum of 0.5 radians converted using Debug `00325780`; this is not an original controller
curve. S is brake only. Original getters/configuration and terrain row 1 / SKID_ROAD supply
the supported contact coefficients; the plane and selected test material are not a track query.

Session pacing supplies elapsed time to the canonical clock and discards suspended elapsed
time. Host chrono is profiling only. Snapshot publication and coordinate conversion are port
interfaces. Preview and drive share the existing evidenced renderer basis; original Debug
CarState wheel matrix calculation supplies driving transforms. Blur opacity is still a tool
presentation policy (`clamp(abs(angular_velocity)*0.02,0,1)*90`), not a recovered full Render
implementation. The bounded CarRenderInfo blur helper was inspected; that does not certify
the full renderer. The follow camera, input routing and UI remain tools.

Original track/world queries, collision scene, RealDriver/controller curve, replay snapshot,
complete allocator/destructor paths, REAL_CONTROL burnout, enabled SpeedDebug restore,
unbound effects/damage/skids/debug services and unsupported EE exceptional domains are
outside the supported closure. Unimplemented recovered execution paths throw
`hp2::NotImplemented`; no guessed fallback is substituted. There is no claim that every
vehicle/game branch is now runnable.

## Validation and original-runtime blocker

See TASK-0044 and its session log for final exact build/test commands and results. Native
instruction fixtures exercise initialization, control conversion, Debug shift/derivative
operands, force/integration order, component dispatch and full pose publication. The actual
Debug-image backend exercised rest -> 11.7256 -> 32.307 m/s -> brake to zero, opposite steering
trajectories (+7.20529 / -7.20283), transactional failures and exact 132-snapshot replay.
Structural tests reject duplicate solver bodies, bypassed canonical entry points, changed
reviewed bodies and known Retail direct data operands.

Real Windows acceptance passed 23 checks with the owned-window Win32 SendInput harness:
actual ImGui entry, W motion, A/D/S/releases, rear camera, relative mouse/idle recenter,
Esc/recapture, F8/focus suspend/resume, return and repeated entry/W/return. The added check
waits for actual window focus; every send/click checks ownership. Desktop activity caused
prior guarded/partial attempts, so the passing run required an idle desktop. Temporary
camera changes were withdrawn; no physics or camera assertion was relaxed. Private evidence:
`local/task0044/window-idle/report.json`, `godot.log`, `driving.png`.

Original Debug numerical comparison remains **blocked, not passed**. On 2026-10-17,
`py -3 -m tools.oracle probe --output local/task0044/pcsx2-probe.json` returned
`{status: blocked, reason: timed out}`. No PCSX2 process/live synchronized Debug driving state
was available; the supplied PCSX2 executable alone is insufficient. No emulator/save/settings
were changed. The older `tools/driving_oracle.py` is Retail-addressed and must not be used
as a Debug oracle. Host trajectory/replay is not a substitute for original traces.

To continue certification: launch the supplied MCP PCSX2 with this exact Debug ELF, reach a
known runnable driving scene, establish PINE/MCP connectivity and pause at an owned Debug
entry. Capture same initial object/component/global state, input, time step and contact
queries for Debug `PhysicsCar::Move 00137D18` and its component calls, using instruction-safe
entry/return stepping. Port the oracle address/field map explicitly before capture; do not
reuse Retail hooks. Compare component states and published body/four-wheel matrices at equal
substep counts, recording per-field absolute/relative errors and covered branches. Original
track contacts must be captured or matched; synthetic-plane motion alone cannot certify a
real-track trajectory. Until such evidence exists, every migrated function stays below Verified.

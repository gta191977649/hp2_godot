# TASK-0038 normal mover/control closure

Evidence E-0038-E07..E11;2026-10-17 Australia/Sydney. Target SLUS_203.62,
revision unknown, ELF SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Authoritative source: original ELF through read-only Ghidra. No original numerical
comparison is claimed. Native fixtures are port-side logical-memory projections.

## Ledger and supported boundary

| ID | Entries | Result |
|---|---|---|
| E-0038-E07 |001A5D60 |16-bit steering control, config-derived distinct angles, original sine calls and two matrix stores |
| E-0038-E08 |00139490 | Scalar scale publication across engine/body/helper/four wheels |
| E-0038-E09 |001C0888/001C08D0/00135EB8/00135E18/00133260 | Car-state angle, timed irregular table evaluation and original field predicates |
| E-0038-E10 |00137D90 | Normal mode control filtering, steering dispatch, drivetrain requests, component factors, recovered substep count, external integration/normalization, launch/freeze flags and publication |
| E-0038-E11 |0025A3D0/0025A410/0025A4A8 | Shared seed transform, unsigned remainder and scalar random output |

**Retired prior boundary (E-0038-E13 below supersedes):** the initial mover projection
excluded the original `car+20==1` branch and every world mode4 branch. They throw; this is a **normal-branch projection**,
not a translation of every00137D90 path. Original mode semantics are not named.
External services must be bound explicitly, and unavailable services throw at the
original address. Physical integration/normalization/publication remain owned by
the parent TASK-0038 implementation, not substituted by mover test hooks.

The implementation is `src/game_core/Sub_00137D90.hpp/.cpp` in
`port::driving::native`; the shared `Memory` retains original offsets/pointers.
The earlier throwing whole-entry template may coexist until its replacement is
reviewed at the actual game-core seam; this record does not silently remove it.

## Evidence and counterevidence

Retained source: `local/task0038/closure/mover-compact.c` and fresh bounded listings
under `local/task0038/engine-ghidra/00137d90.listing.json`,
`00138300.listing.json` and `00138790.listing.json`. Earlier saved huge listing
previews were truncated, so fresh explicit ranges were read. Steering001A5D60
is not a defined function in the Ghidra project. Its complete read-only byte listing
is retained as `001a5d60.listing.json`; Capstone5.0.7 MIPS64 little-endian decoded
ordinary MIPS/COP1 instructions into `001a5d60.decoded.txt`. MIPS32 mode initially
stopped at the first SD; switching to64-bit decoded the complete range. No Ghidra
function boundary, annotation or data type was changed.

00137D90 decompilation conflates temporary integer/float variables. Native mover
uses scalar loads/stores for mover+70/+74, preserving actual instructions. At
00138610 the load is **controls+460** and the following signed byte is **car+84c**;
a decompile local alias must not be interpreted as wheel+460. Steering dispatch
reads a signed16-bit adjustor at table+10 and verifies target001A5D60 at+14.
001A5D60 masks incoming control to16 bits before float conversion; signed host
steering axes require an explicit adapter to this original representation.

Original RNG0025A3D0 uses `DIVU` at0025A3E4. The decompiler's signed remainder is
countered by that instruction. It returns the **previous** seed modulo divisor,
then stores the XOR/right5/left27 transform.0025A4A8 selects shared state00332068;
map its original seed bytes instead of inventing a default. Earlier frontend/audio
records for0025A3D0 already contain this unsigned correction; no duplicate third
function record was created.0025A410 multiplies the input by rawfloat30000000
before multiplying the converted integer; source grouping is preserved.

## Launch/freeze and unknown lane

Constructor00137100 sets mover+60=1. For world mode<2, normal mover preserves
that flag and zeros body+60 XYZ after every substep. Modes>=2 and!=4 run the
recovered throttle/timeout/RNG transition that clears it. Fixture mode2 can thus
exercise the original launch transition without overriding flags in the mover.
Mode choices remain explicit port-side fixture inputs, not recovered enum names.

The original freeze branch initializes only the first three stack vector lanes
before a quadword store. The fourth lane is unresolved. `DrivingServices` requires
an explicit `Field_001389A4_stack_w` value when that path is reached; absence throws.
Choosing0 in the synthetic backend is a recorded port-side projection of an
unconsumed lane, not a claim that original stack bytes were zero.

Original performance counters, generic integration0019C290, normalization0011BD90,
conditional postpasses0019D980/0018C030, final publication00139188, optional damage
001A5520 and referenced object evaluation001B50C8 are explicit service callbacks.
Missing required callbacks never silently succeed. Original helper375F0,
engine timer/control requests, steering, angle/timestamp predicates, friction
leaf00137C88 and RNG are directly recovered calls.

## Validation

Passed GCC13.2.0 ordinary finite fixture command:

```powershell
g++ -std=c++17 -ffp-contract=off -Isrc src/tests/driving_mover_tests.cpp src/game_core/Sub_00137D90.cpp src/game_core/Sub_00137100_runtime.cpp src/game_core/Sub_00137100.cpp src/game_core/Sub_0018BCE0_memory.cpp src/game_core/Sub_0018BCE0.cpp src/game_core/Sub_0018F808.cpp src/game_core/Sub_00137C88.cpp src/game_core/Sub_001A4930.cpp src/game_core/Sub_00138F38.cpp src/game_core/Sub_001A4E88.cpp src/game_core/Sub_0019D4D0.cpp src/game_core/Sub_00152070.cpp src/game_core/port/driving/memory.cpp -o local/task0038/driving_mover_tests.exe
& ./local/task0038/driving_mover_tests.exe
# DRIVING_MOVER_OK: control and service instruction fixtures, original validation deferred
```

Fixtures cover3 recovered substeps at dt0.25/subdivision0.125, callback ordering,
real RNG launch counter/flag/tick updates, throttle copy, two zero-steering matrices,
freeze XYZ and explicit W bits, absent integration throw, excluded-mode rejection
and unsigned remainder counterevidence. Synthetic exact polynomial nodes isolate
the steering stores; they are not original trig constants. Integrator callbacks
in this test only exercise the call contract and assert no driving behavior.
An initial standalone link omitted live cross-module sources and failed; adding
the actual recovered dependency files resolved it. No stubs were added.

Remaining: parent whole-scene integration and original numerical comparison,
exceptional PS2 FPU behavior, unsupported car+20==1 and diagnostic mode4 branches,
and complete per-branch native fixtures. Keep TASK-0038 active until its chosen
acceptance scope has evidence; do not mark the entire mover Verified.

## Running-context correction - E-0038-E13/E14

See task0038-running-context.md. Original Race+0 state4 is reached by00148A78 and
is supported when separate diagnostic flagA758 is0. That path skips launch/freeze;
only Race4/A758!=0 remains excluded. The earlier blanket mode4 exclusion above is
retired counterevidence, not the current supported boundary. Input table/writer
recovery also establishes controls+218 as the JOY_EVENT_BRAKE route and+21c as the
JOY_EVENT_EBRAKE route. The direct normalized tool adapter uses the former and
excludes the latter. No original numeric comparison is established.

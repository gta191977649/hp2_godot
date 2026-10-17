# TASK-0038 engine, table and transmission closure

Evidence E-0038-E01..E06;2026-10-17 Australia/Sydney. Target SLUS_203.62,
executable revision unknown, ELF SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Source authority: authorized ELF through read-only Ghidra MCP. Original numerical
comparison is deferred; no function is Verified. All host carriers are port-side.

## Evidence ledger

| ID | Entries | Instruction-derived result |
|---|---|---|
| E-0038-E01 |0018F808,00189120,00189168,0018F838,0018F9F0 | Uniform table pointer/count/bounds/scale, clamped scalar interpolation, irregular x/y interval scan and tiny-width midpoint |
| E-0038-E02 |0018AC38 | Config-selected static threshold or50-unit crossover scan; two ratio tables and terminal upper-bound store |
| E-0038-E03 |0018A7E8,0018AE70 | Engine/transmission constructors, exact pointer/store offsets and table generation |
| E-0038-E04 |0018AA28,0018AAB0,0018A988,0018A9D8 | Engine evaluation, factors, cutoff, sequential clamp, accumulator reset and derivative |
| E-0038-E05 |0018B008,0018B0D8,0018B178,0018BC18,0018BC68,0018B4A0,0018B698 | Axle scalar helpers, wheel scalar assignment, state predicates, transition machine and state derivative |
| E-0038-E06 |0018C368,0018C1C8 | Both scalar-count leaves return1 in delay slot |

Retained ignored Ghidra queries: `local/task0038/ghidra/<address>.listing.json`
and `.c` for the earlier E01..E04 entries; fresh read-only queries are under
`local/task0038/engine-ghidra/<address>.listing.json` and `.decompile.json`.
Fresh listings use explicit start/end addresses and bounded max_lines. A listing
request with start address alone initially returned one line; it was repeated
with an explicit end address. No Ghidra boundaries/annotations were changed.

0018A988 and0018B698 remain bytes in the analysis database. Their retained byte
listings establish the instruction words; the native projection decodes those
words without modifying the database.0018C368/0018C1C8 are `03e00008,24020001`.
001A4168 is the exact single wheel+20 store in the delay slot and is preserved
inline in0018B178.

## Implementation and limits

`src/game_core/Sub_0018F808.hpp/.cpp` use the shared `port::driving::Memory`.
Constructors preserve fields they do not write.0018A7E8 writes through+58;
its host region must cover at least0x5c despite the previously observed allocator
label size0x58.0018AC38 uses a local table carrier instead of persisting the
original stack storage in the logical heap. Signed integer decrement/limit
addition retain32-bit wrapping. Port bounds failures reject invalid mapped
inputs rather than manufacturing data.

Engine0018AAB0 multiply grouping follows instructions: first form engine+44 times
engine+4c, then multiply curve output.0018B0D8's front-only branch halves even if
the rear branch was skipped.0018B178 uses car+384 and exactly the two axle-specific
contact-byte gates; the first axle ratio adjustment is not silently reused for
the other.0018B4A0 records real transition fields+2c/+30/+38/+3c and coupling stores.

Original0018F878 is `cvt.w.S`. The supported native scalar projection truncates
finite signed32-range values, independently of the host fenv, and throws
`hp2::NotImplemented` for exceptional/out-of-range conversion operands. This is
a bounded CPU projection, not a complete PS2 FPU implementation. FCR31 was
previously observed as0x01000061 (E-0038-007), corroborating a truncation context.
As an architecture/tooling lead only, upstream PCSX2 v2.8.0
[FPU.cpp237..240](https://github.com/PCSX2/pcsx2/blob/v2.8.0/pcsx2/FPU.cpp#L237)
uses signed32 truncation for CVT_W and does not consult FCR31. That external code
is not original-game evidence under AGENT.md R0 and establishes no original
numeric equivalence. The ELF operation and finite scope remain recorded here;
comparison against actual original operands remains deferred.

## Validation

GCC13.2.0, ordinary finite exact instruction fixtures. Passed:

```powershell
g++ -std=c++17 -ffp-contract=off -Isrc src/tests/driving_engine_tests.cpp src/game_core/Sub_0018F808.cpp src/game_core/port/driving/memory.cpp -o local/task0038/driving_engine_tests.exe
& ./local/task0038/driving_engine_tests.exe
# DRIVING_ENGINE_OK: bounded instruction fixtures, original validation deferred
```

Fixtures cover uniform interpolation endpoints/interior, irregular second segment
and midpoint branch, static and dynamic crossover/fallback generation, pointer
stores and trailing engine field, engine factors/cutoff/clamping/derivative,
axle averages/front-only halving, wheel scalar averaging, transmission3->0->1->2
state sequence, derivative table and constant-count leaves. They are explicit
hand-computed algorithm fixtures, not engine integration or original comparisons.

## Continuation

Bind engine derivative0018A9D8 and transmission derivative0018B698 to the generic
integrator, count leaves to their object vtables, and engine evaluation0018AAB0
to existing drivetrain coupling hooks. Map original global/table bytes through
the shared memory source. Do not populate inferred constants or replace unrecovered
contact behavior. Complete parent TASK-0038 constructor/control/solver integration
before claiming a drivable fixture or original trajectory agreement.

## Later bounded exceptional operation - E-0038-E12

The all-zero wheel ratio exposed host IEEE0/0 propagation in0018B6C8. Its two
confirmed DIV.S sites now use a narrow explicit port-side EE zero-denominator
carrier before the original bound comparisons. See task0038-ee-zero-divide.md
for debugger evidence, ISA-reference decision, scope and signed-zero tests.
This does not expand the ordinary-finite scope of unrelated operations.

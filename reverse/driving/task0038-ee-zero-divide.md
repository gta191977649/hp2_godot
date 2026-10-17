# TASK-0038 zero-denominator DIV.S projection

Evidence E-0038-E12;2026-10-17 Australia/Sydney. SLUS_203.62 revision unknown,
ELF SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Original numerical validation remains deferred. No Verified promotion.

## Original instruction evidence and host counterexample

Read-only Ghidra listing E-0038-D05 establishes actual `DIV.S` instructions at
0018B794 (front ratio) and0018B8A0 (rear ratio). They divide one wheel+20 scalar
by the axle pair's sum before the original bound comparisons. There is no
zero-denominator branch in the game code, and no balanced-ratio fallback.

The real-data synthetic-plane backend exposed a host projection defect. GDB
caught the first nonfinite scalar store at wheel+108 in
`Sub_0018BCE0_memory.cpp` after Sub_0018B948 -> Sub_0018B6C8. All four source wheel+20
values were0, all contact bytes155 were0, selector car+3a4 was0 and engine+48 was
-4.47677. Host IEEE0/0 yielded NaN, making the weighted transfer NaN. It subsequently
reached wheel derivative, angular value+20, phase+1c, sine0025A5E0 and the explicit
CVT.W.S unsupported-operand guard. The final conversion error was downstream of
the original host DIV.S mismatch, not evidence that a random epsilon was needed.

Ignored reproducible debugger scripts: `local/task0038/debug-first-nan.gdb`
(break Memory::scalar setter when value!=value), `debug-split.gdb` (captures the
four source wheel values/bytes), `debug-nan.gdb` (first angular nonfinite store).
The scripts invoke only the native test executable. No emulator state was changed.

## Port-side ISA implementation decision

As an ISA implementation reference, upstream PCSX2 v2.8.0
[FPU.cpp98..102 and242..246](https://github.com/PCSX2/pcsx2/blob/v2.8.0/pcsx2/FPU.cpp#L98)
projects zero-denominator DIV.S results using sign XOR and finite magnitude
0x7f7fffff. This source is **not original-game evidence** under AGENT.md R0.
The parent TASK-0038 implementation explicitly accepted it as the narrow port-side
instruction carrier needed for the confirmed DIV.S sites and observed zero inputs.
The original ELF remains authoritative for the algorithm and comparisons; no game
constant, invented physical behavior or Verified claim follows from this decision.

`port/driving/ee_scalar.hpp::ee_div_s_zero` supports finite numerator and a signed
zero denominator only. It returns raw bits
`((numerator_bits ^ denominator_bits) & 0x80000000) | 0x7f7fffff`.
Unsupported helper operands throw at the supplied original instruction address.
Only the two reached zero-denominator ratio paths call it. Normal divisions,
all unrelated arithmetic, and exceptional NaN/infinity cases are not broadened.
This is not a complete PS2 FPU model: exception flags, denormal flushing and
instruction-level rounding are outside the helper. Original comparison is deferred.

The resulting large signed scalar is consumed by the **existing original** lower/
upper comparisons and corresponding recovered global-derived ratio selection.
There is no equal split, epsilon, friction tweak or state adjustment. Positive0/0
selects the original upper-bound branch; sign differences can select the lower
branch.

## Validation

Passed GCC13.2.0:

```powershell
cmake --build --preset native-debug --target driving_drivetrain_tests -j4
./build/native-debug/driving_drivetrain_tests.exe
# DRIVING_DRIVETRAIN_OK: instruction fixtures, no original equivalence claim
```

Tests cover all four signed-zero numerator/denominator combinations, a finite
negative numerator with negative zero denominator, rejection of nonfinite numerator,
and a zero-wheel/contact-free launch. With synthetic original-global input0.25
and transfer8 the original comparison/store graph yields wheel increments
14/2/2/14. These are explicit instruction fixtures and establish no original
trajectory agreement. Full backend relink/run remains part of the parent task.

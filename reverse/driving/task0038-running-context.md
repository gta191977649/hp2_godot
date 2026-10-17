# TASK-0038 Race running context and brake input evidence

Evidence E-0038-E13/E14;2026-10-17 Australia/Sydney. SLUS_203.62 revision unknown;
ELF SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Read-only Ghidra evidence; original numerical validation deferred. No Verified claim.

## E-0038-E13 - Race state4 and independent diagnostic flag

**Counterevidence retires the earlier blanket description of mode4 as diagnostic.**
Global003301F0 (gpA900) points to the object allocated using the original label
`Race` at00330258.00149988 allocates size0x6b4, calls constructor001475E8, then
00147960.001475E8 stores the shared pointer;00147960 calls00147EF0, which stores its
state argument at object+0. Normal setup00149988 can select3, with other setup
branches selecting1 or4. These observations do not establish original enum names.

00148A78 tests Race+0==3, calls001480C0, and if its result is4 invokes00147EF0 with
state4. Explicit instruction range00148A78..00148AD0 confirms the call and delay
slot argument4 at00148AC4. Thus state4 is an original operational state reached
by the Race update, not an unsupported debug-only context.

Within00137D90, Race state4 with separate global gpA758 (00330048) equal0 reaches
the compact00138B9C..00138BD4 branch: it stores tick-derived telemetry and seven
zero/one fields. It does **not** run the mover+60 launch/freeze logic. The separate
Race4/A758!=0 branch at001389BC performs diagnostic thresholds and position
restoration, and remains explicitly unsupported in the native mover.

The state4/A7580 scope also activates the original all-wheel controls+218 gate,
where other states use only mover+60 and the first axle. It skips the preintegration
body snapshot copy unless A758!=0. Native implementation now preserves these
conditions rather than treating every state4 as diagnostic.

The prior mode2 synthetic context initially allowed a short RNG launch, then
reasserted mover+60 after the original time window and froze under held input.
That finite result was a wrong fixture context, not a force/inertia problem.
No invented mover+60 setter was added. The backend may explicitly select the
originally demonstrated Race state4 and A7580 as its synthetic running context.
Original Race/gameflow construction and transitions remain outside DrivingLab.

A bounded Ghidra byte-pattern search of direct SW stores at offset+60 found the
PhysicsCar constructor and mover launch writes, plus unrelated subsystem matches.
Related+58/+5c searches and virtual00137450 serialization did not reveal a
justified independent single-field start setter. This is a bounded negative search,
not a claim that no indirect/adjusted setter exists. The recovered state4 path makes
such a setter unnecessary for the selected fixture.

Raw ignored queries are under `local/task0038/start-context`: mode label read,
001475E8-containing00147638 decompile,00149988-containing00149A90 decompile,
00147EF0 decompile and00148A78 listing. Earlier mover explicit listings remain in
`local/task0038/engine-ghidra/00138790.listing.json`.

Passed:

```powershell
cmake --build --preset native-debug --target driving_mover_tests -j4
./build/native-debug/driving_mover_tests.exe
# DRIVING_MOVER_OK: control and service instruction fixtures, original validation deferred
```

New fixture confirms Race4/A7580 preserves constructor mover+60=1 while retaining
the integration result, consumes no launch RNG/counter, requires no freeze W input,
and performs the original telemetry stores. Race4/A758!=0 still throws before mutation.
These callback fixtures establish only control flow, not physical equivalence.

## E-0038-E14 - JOY_EVENT_BRAKE versus JOY_EVENT_EBRAKE

The original value/name table002FA2D0 contains:

| Entry address | Value | Original identifier | Name pointer |
|---|---|---|---|
|002FA658 |0x74 |JOY_EVENT_BRAKE |002B1098 |
|002FA660 |0x75 |JOY_EVENT_BRAKE_ANALOG |002B10A8 |
|002FA668 |0x76 |JOY_EVENT_BRAKE_ANALOG_ALTERNATE |002B10C0 |
|002FA680 |0x79 |JOY_EVENT_EBRAKE |002B1120 |
|002FA688 |0x7a |JOY_EVENT_EBRAKE_ANALOG |002B1138 |

0013B080 registers74/75/76 to callback0013BD40 and79/7a to0013BDC0. The complete
0013BD40 listing shows byte inputs at+260/+261/+262, selection of the digital input
or larger analogue input, original global scale, and normalized scalar store+264.
0013BB40 loads+264 at0013BBBC; its forward/reverse routing ultimately writes the
braking scalar to controls+218 at0013BC98. Signed speed thresholds are stored at
00330120/24/28:0.5,-30,-2. The reverse routing and filtering are not implemented by
the direct normalized tool input adapter; its documented scope excludes reverse.

0013BDC0 writes its selected input to normalized+274 at0013BE10.0013BB40 loads that
field and publishes it to controls+21c at0013BBB4, except for its independent timed
special branch. Therefore +21c is linked to `JOY_EVENT_EBRAKE`, not the requested
foot brake. This is established from original identifiers and data flow, not from
an assumed rear-wheel braking convention.

Additional original data corroborates distinct use:

| Table | Per-wheel floats | Raw bits |
|---|---|---|
|002F0500, multiplied by controls+218 |1,1,0.6,0.6 |3f800000,3f800000,3f19999a,3f19999a |
|002F0510, multiplied by controls+21c |0,0,1,1 |00000000,00000000,3f800000,3f800000 |

The original mover also reduces scalar+74 toward0.5 while+21c>=0.5, and applies the
existing00137C88-related rear factors; none of that authorizes using it as foot brake.
For the accepted direct normalized tool adapter, route `Input.brake` to+218 and
hold+21c at0 while handbrake is excluded. The adapter remains port-side, without a
claim to reproduce the complete original RealDriver filtering/reverse logic.

Retained ignored data: `joy-full-table.json`, `002b1010.read.json`,
`brake-wheel-factors.json`, `0013b080.json`, `0013bd40.listing.json`,
`0013bdc0.listing.json`, `0013bb40.listing.json` and `00330120.read.json` under
`local/task0038/start-context`. No proprietary bytes were added to Git.

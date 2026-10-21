# TASK0053 skid zero-vector VU boundary

## E0053D01 - Exact-zero SkidSet::AddSegment normalization

Authority: Debug NFSHP2.ELF SHA256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`,
original `AddSegment__7SkidSetP8bVector3T1f`, entry001A8150, canonical owner
SkidSet in indep/src/skids.cpp. Retail physics is not substituted here.
Fresh read-only Ghidra listing: `local/task0053/skid-001a8150-listing.json`.

Confirmed instruction chain:

- 001A81A0 computes delta;001A81AC..B4 sums its squared XYZ components.
- 001A81C4 executes EE SQRT.S for travel length.
- 001A81C8 word4A6103BE executes VRSQRT with fs0/fsf3 and ft1/ftf0:
  numerator vf0.w=1, denominator square-root of vf1.x=squared length.
- 001A81CC waits for Q;001A81D0 copies Q using vf0.x=0.
- 001A81DC multiplies all delta lanes by Q. There is no zero-length branch.
- 001A81E4 onward retains original count/angle/merge/end/capacity decisions.

ISA interpretation uses Sony Computer Entertainment's VU User's Manual6.0,
April2002, sections2.1.1(p26),2.3(p28), and macro VRSQRT(p310):
[primary manual mirror](https://docs.alexrp.com/mips/ee_vu.pdf).
VU exponent255 represents finite values; zero-divisor results saturate to
signed maximum while execution continues. IEEE NaN/Infinity behavior is not
applicable. For numerator+1 and denominator+0 this yields positive finite MAX,
and multiplication by exact signed-zero delta components yields signed zeros.
The positive MAX value `(2-2^-23)*2^128` is exactly representable in host double.
The scoped adapter carries that value only for an exact-zero XYZ vector, then
performs the original multiplication. It introduces no epsilon, fallback axis,
skipped segment or invented eligibility branch. Nonzero components whose host
squared sum underflows to0 still throw an addressed NotImplemented boundary.
No general VU flags, underflow, rounding or numerical emulator is asserted.

The historical TASK0046 record's zero-length NotImplemented was a port recovery
limit, not evidence that the original throws. This TASK0053 amendment closes
only the exact-zero vector boundary, preserving that historical record/context.

## Regression

`driving_skid_tests` now covers:

- Repeated stationary MakeSkid contacts retain the original second endpoint,
  finite bounds and signed-zero direction. The next zero-direction contact takes
  the original angle/end branch and detaches; no artificial merge/skip is added.
- Movement after detachment starts a new set and retains unit nonzero direction
  and original travel length. Existing nonzero merge/rollover/LRU tests remain.
- Twelve stationary Wheel::MaybeMakeSkids frames execute both elevation queries
  each frame without exception; generated bounds remain finite and original
  zero-direction endpoint/end behavior remains active.
- A nonzero delta with squared-length underflow remains explicitly unsupported.

Private build/run (GCC13.2.0, current skid TU plus unchanged shared dependencies;
no shared build or GPU probe):

```
C:/Strawberry/c/bin/c++.exe -std=c++20 -O2 -Isrc -Isrc/game_core src/tests/driving_skid_tests.cpp src/game_core/indep/src/skids.cpp build/godot-debug/libgame_core.a -o local/task0053/driving_skid_zero_tests.exe
local/task0053/driving_skid_zero_tests.exe
```

Result: DRIVING_SKID_TESTS_OK. Full Debug/Release integration and actual driving
are parent-owned pending checks. PC tests do not establish original-console
Verified equivalence. Only SkidSet::AddSegment changes recovered production logic;
all other physics bodies must remain immutable under the scoped successor audit.


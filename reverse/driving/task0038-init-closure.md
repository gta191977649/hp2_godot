# TASK-0038 — 00137100 initialization closure

Target: SLUS_203.62, executable revision unknown, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Evidence E-0038-I13: read-only Ghidra MCP closure audit, 2026-10-17 Australia/Sydney. No original
equivalence test or Verified status is claimed.

## Confirmed and translated dependencies

Continuation: `Sub_00137100_runtime.hpp/.cpp` now translates the complete logical
constructor call/store graph and list/reset/late initialization dependencies
(E-0038-I14..I29). `Sub_0019DA30.hpp/.cpp` adds dynamic table headers and original
frame scheduling projections (E-0038-I30..I32). Logical arena ownership, synthetic
contact geometry, performance/replay services and otherwise uninitialized scratch
W lanes are explicit port adapters. The production class template is a separate
entry until backend integration; no original equivalence is claimed.

The remainder of the initial twelve-function audit below records the earlier
baseline. Its open-dependency table is historical; current continuation details
and retired counterevidence follow it.

`src/game_core/Sub_00137100.hpp/.cpp` adds twelve bounded initialization
dependencies under the explicitly port-side `port::driving::init` namespace.
Individual function records use E-0038-I01 through E-0038-I12.

| Entry | Translated contract |
|---|---|
| 0025AC68 | Identity matrix, all sixteen words |
| 0025AC98 | Scalar affine matrix inverse and inverse translation |
| 0019CC58 | Config scalar, reciprocal, inverse matrix publication |
| 0019C9B0 | Complete constructor stores plus the above two matrix calls |
| 001A3F10 | Complete constructor stores, preserving halfword neighbors |
| 001A5EA0 | Complete constructor stores |
| 0019C170 | Intrusive head self-links, count and table |
| 0019CC40 | Optional sixteen-byte copy |
| 001A5F88 | f12 publication and four-lane vector difference |
| 001A4348 | Raw global word publication |
| 001A4158 | f12/f13 stores |
| 001A5FB0 | Two-field clearing |

ByteView is a bounded little-endian port adapter, not a recovered native type.
Object pointers remain encoded uint32 words; resolved pointers/vectors are explicit
arguments. Unwritten bytes retain their caller-supplied values. In particular,
0019C9B0 does not initialize body+0x160 or zero the fourth lanes of the vectors at
+0x60/+0x70/+0x90/+0xA0/+0xF0/+0x100. The constructor's +0x20 matrix is initialized
fully; its separate +0x120 inverse matrix comes from config+0x10.

## Remaining whole-entry gaps

00137100 still must allocate and wire all components, append them through
0019C238, perform late configuration/initial-state calls, and establish the
native owner/lifetime model. It remains throwing. The new file's address names
the investigation's root; it intentionally does not define Sub_00137100.

| Dependency | Evidence-backed gap |
|---|---|
| 00257C80 | Original allocation/pool integration and failure contract |
| 0019C238 | Intrusive append mutates links, invokes vtable+0x14 with signed adjustor at +0x10, then adds returned count to head+8 |
| 0018A7E8 | Requires 0018F808 twice (9-point and 3-point tables), whose float conversion contract still needs audit |
| 0018AE70 | Requires 0018AC38 to generate both 9-word tables from explicit global and config data |
| 001375F0 | Uses 0019CC40/58, then global-controlled config math, 001ADFB8, four 001A5F88 calls and four 001A4348 calls; not yet integrated |
| 001ADFB8 | Float-to-int conversion and sign mask precede multiplication by the sum of helper-config+4/+8; conversion semantics need instruction/control-state audit |
| 00138CF8 | Requires 0019CC88 to synchronize body state, four 001A5FB0/001A4158 calls, and explicit global override semantics |
| 00137788 | Calls 0019C808, 00137938, then adjusted virtual vtable+0x1C (signed adjustor +0x18) |
| 0019C808 | Traverses the component list, adjusted virtual dispatch through +0x44/+0x40, with 00250F38/00250F48 timing side effects |
| 00137938 | Four-wheel checks, 001A6408, 001377D8 state classification, vector modifications, 00133070, 0019CF10 and possible list reset |
| 001377D8 | Body vector lengths, controls and vehicle state predicate 00133130 determine 0/1/2; not replaced with a rest-state guess |

The decompiler's PhysicsCar descriptions are analysis labels, not recovered names.
No physical meaning is assigned to unnamed config fields by these projections.

## Validation

Passed:

```powershell
& C:/Strawberry/c/bin/c++.exe -std=c++17 -Wall -Wextra -Werror -ffp-contract=off -Isrc src/game_core/Sub_00137100.cpp src/tests/driving_init_tests.cpp -o local/task0038/driving_init_tests.exe
& local/task0038/driving_init_tests.exe
py -3 reverse/scripts/inspect_driving_init.py --elf D:/ps2_game/SLUS_203.62
```

Compiler: GCC 13.2.0; Python 3.14.4. Capture produced 66 fresh read-only queries
under ignored `local/task0038/init-ghidra` with a SHA-bound manifest. Tests cover
exact instruction store schedules, untouched bytes, a halfword boundary, raw
vector copies, all four subtraction lanes, a non-orthonormal affine inverse and
an independent matrix multiplication check. Float projection scope is ordinary
finite arithmetic; exceptional EE/VU arithmetic and dynamic equivalence remain
unverified. Build-system integration and full suites are performed by the main
TASK-0038 session.

## Exact continuation

Continue TASK-0038: audit 0018F808/0018AC38 conversion instructions and FPU control
state, resolve the six component vtable contracts used by 0019C238/0019C808, then
translate 0019CC88 and the late initialization state calls. Preserve the throwing
00137100 production entry until the entire constructor and ownership contract
is implementable from evidence.

## Continuation: constructor, reset and scheduling closure

The new `Memory` owns checked logical regions and separate persistent component
allocations. `Memory::Scratch` scopes temporary allocations, including nested
calls, and leaves explicit static mappings intact. No original allocator or
native C++ ABI layout is claimed. Original encoded pointers and exact offsets
are retained throughout the recovered algorithms.

00137100 now performs all body/wheel/controller/engine/drivetrain construction,
eleven list insertions (32+8*2+1+1=50 derivative words), sibling wiring, helper
setup, late synchronization, two reset traversals when the state2 route applies,
the adjusted final00138E08 call and the original final stores. Required unknown
external service calls fail loudly. Native driving integration remains the root
TASK-0038 work, not established by the synthetic graph fixture.

Retired counterevidence: the earlier001ADFB8 conversion blocker was produced by
incorrect decompiler output. Its full listing uses `max.S` followed by a scalar
multiply and contains no conversion.001A5FC0 and001A33F0 also have bogus integer
casts in decompilation; their listings show max/min float instructions. These
casts were never adopted into native behavior.

001395B0 writes the irregular sample table and header00330060 dynamically;
reading that header from the uninitialized ELF region would produce invalid
runtime state.0019DA30 similarly constructs headers00310DF8/00310E30 while their
sample arrays remain original ELF data.

Scheduling evidence: a bounded Ghidra hex search for immediate `98 b3` over
00100000..002D0000, followed by aligned word/opcode decoding, identifies writes
to gp-0x4C68/00330C88 at001A35CC (initial clear),001A35E0 (delta publication)
and001A3694 (end-frame clear). Main loop0012F378 calls001A33F0 with measured
elapsed timer output scaled by0032DBC0, chooses its output or replay-scaled
0014D718 output, then publishes via001A35E0 before physics.001A35F0 consumes and
clears the delta at frame end. Pointer-based or dynamically generated writers
were not excluded by this bounded immediate search.

001A33F0 scales actual elapsed input, clamps using exact ELF words according to
00330C54, quantizes with3600, retains a4000Hz residual, and invokes the original
stream read/observation calls through explicit services. The ELF default mode1
clamp is approximately1/60..0.0500000045; mode0 is approximately0.02..0.06.
This is a recovered bounded scheduler, not an assumed fixed60Hz timestep.

Passed native fixtures:

- `driving_constructor_tests.cpp`: checked memory, scratch lifetime, exact links,
  signed adjustors/counts, unequal axle split, vector W lanes, sentinel/time
  gates and the complete synthetic constructor graph with real component resets.
- `driving_schedule_tests.cpp`: dynamic static-table headers, original constant
  words, distinct3600/4000 conversions, both clamp modes, replay service order,
  residual/counter stores and zero-delta behavior.

Standalone tests were compiled with GCC13.2.0/C++17 and `-ffp-contract=off` and
passed. Initial full-link attempts preceded arrival of independent body/tire
definitions; a strict all-source compile also reported their existing misleading
indentation warnings. Final standalone links with the complete source set passed.
Root session performs CMake integration and whole-suite validation.

After integration, `cmake --build --preset native-debug --target
driving_constructor_tests driving_schedule_tests -j4` passed.
`ctest --preset native-debug -R '^driving_(constructor|schedule)$'
--output-on-failure` passed2/2 (0.52s), including the final capacity/guard changes.

Capacity counterevidence: the initial controller logical capacity0xA0 covered
its constructor's stores but not later001A6330, which writes controller+0xA0.
An actual original-config integration attempt exposed that field aliasing the
next wheel's list link. The port-owned capacity is now0xB0 (alignment-rounded
minimum covering the recovered+0xA0 store), with a regression that writes the
field and checks the next node's link. This is a port bound, not a claim about
the original pool allocation size.

The port allocator also leaves an unmapped16-byte guard after each allocation.
This does not claim an original PS2 layout; it makes a future missed field bound
fail immediately instead of silently aliasing an adjacent component's links.

Reproduce evidence with `py -3 reverse/scripts/inspect_driving_constructor.py --elf
D:/ps2_game/SLUS_203.62`. All raw listings/data and the SHA-bound query manifest
remain ignored under `local/task0038/constructor2`.

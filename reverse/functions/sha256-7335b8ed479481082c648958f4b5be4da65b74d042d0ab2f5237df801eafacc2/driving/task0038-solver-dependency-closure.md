# TASK-0038 solver dependency closure

Target: SLUS_203.62; executable revision unknown; SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.

Read-only Ghidra capture, field projections, bounded algorithm implementation and host fixtures. This is partial recovery, not a completed drivable backend and not Verified behavior. Original source paths and physical field semantics remain unknown; new recovered functions use ELF-address identifiers and projection types live under `port::driving`.

## Recovered scope

| Evidence | ELF entries | Scope |
|---|---|---|
| E-0038-S01 | 0025AF20 | Sixteen-float transpose |
| E-0038-S02 | 0025AED0 | Matrix/vector transform with mutable global |
| E-0038-S02 | 0019D368 | Transform thunk |
| E-0038-S03 | 0019CD90 | Two independently bounded accumulations |
| E-0038-S04 | 0019CE80 | Transform/accumulation wrapper |
| E-0038-S05 | 0019CEF8 | Four-lane addition |
| E-0038-S06 | 001A5428 | Three paired scalar products |
| E-0038-S07 | 001A44A8 | Conditional four-lane offset and double-cross factor |
| E-0038-S08 | 001A6330 | Conditional request wrapper |
| E-0038-S09 | 001A6370 | Conditional wheel/body accumulation |
| E-0038-S10 | 00138F38 | Five-iteration solver orchestration |
| E-0038-S11 | 001A4778 | Two strict scalar thresholds |

## Counterevidence and unknowns

-0025AED0 input W is ignored.00320A60 is mutable, despite all-zero ELF storage.0025AFA8 sets its W to1. A transform implementation that simply consumes input W would be incorrect. Explicit context retains this requirement.
-001A44A8 decompiler prototype and001A6330 decompiler call omit the inherited A1 contract. The listing establishes that+0x70 controls both the added input and the byte store at wheel+0x155.
-0019CD90 caps force and cross independently, computes the cross from the original force reloaded after the first accumulation, and preserves A2 W in the XYZ-only cross instruction. A generic host cross() with W=0 would erase original lane behavior.
-00138F38 temporary output W comes from uninitialized stack data and only contaminates a temporary sum W never consumed by the recovered scalar computation. The port projection makes this discarded lane deterministic; no original fourth-lane value is claimed.
-001A4930(1016 bytes) and001A4EE0(1216 bytes) remain untranslated. Captured decompilations show material table00318C20/24 with0x90 stride, helpers001A4778/4818/4D28, fixed-angle and trig routines0025A890/0025A5E0, globals and VU branches. They require further listing/data audit; no approximate tire model is supplied.
-00138E08 also requires001ADD70, a body virtual call, timing bookkeeping and00139518; this group does not replace that whole entry.
-Default solver hooks throw at0018B948,001A4930,0018BAA0,001A4EE0.0018BC10 delegates the separately recovered original empty body. Separately recovered drivetrain functions may be explicitly bound by an adapter. No test fixture hooks are used by production.

## Validation

Passed with GCC13.2.0 (C:/Strawberry/c/bin/c++.exe):

```powershell
& C:/Strawberry/c/bin/c++.exe -std=c++17 -O0 -ffp-contract=off -Wall -Wextra -Werror -Isrc src/game_core/Sub_00138F38.cpp src/game_core/Sub_001A4E88.cpp src/game_core/Sub_0018BCE0.cpp src/tests/driving_solver_tests.cpp -static -static-libgcc -static-libstdc++ -o local/task0038/solver/driving_solver_tests.exe
& ./local/task0038/solver/driving_solver_tests.exe
```

Output:`driving_solver_tests passed (bounded host fixtures, not original equivalence)`. These fixtures cover in-place transpose, explicit global W, force/cross caps, late input reload aliasing, four-lane behavior, XYZ-only stores, double-cross clamp, conditional request forwarding/suppression, five iterations, both wheel-exclusion fields, dependency order and throwing missing dependencies.

Integration required: add new source to game_core and new test executable to CMake, then run native/Godot checks. No full mover, constructor or game scheduling equivalence follows from these finite fixtures. Float multiply/add order is preserved at the C++ dependency level; exact PS2/VU rounding remains unverified. No commit created.

## Reproduce evidence

Use read-only`tools.ghidra.Client` endpoint from project configuration; program`SLUS_203.62`. Query`inspect` with`action=decompile,address=<entry>` and`action=listing,address=<first>,end_address=<last>,max_lines=1000` for each bounded range in its per-function record.0025AFA8..0025AFCF establishes the global initializer;`memory read`00320A60 length16 and0032E644 length4 plus`inspect references_to` establish initial storage/write xrefs. Raw captures and hash manifest stay ignored in`local/task0038/solver/`; no proprietary listing is added to Git. SHA256 was independently checked against the authorized ELF using Python hashlib before implementation.

Continue TASK-0038: recover001A4930/001A4EE0 and their trig/table/material dependencies, bind original drivetrain hooks explicitly, recover00138E08 body/update dependencies, then collect paused instruction-boundary original traces before any Verified promotion.

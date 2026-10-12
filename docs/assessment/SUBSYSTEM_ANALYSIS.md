# Subsystem discovery and complexity

Classification is provisional and limited to the 20 selected pilot entries.
The inventory leaves 7,525 entries Unknown, including one intentionally unclassified
pilot. Confidence measures interpretation certainty, not test or verification status.
Every candidate below is unverified; no PC gameplay implementation exists.
Evidence references are defined in EVIDENCE.md.

| Subsystem | Candidates | Evidence and confidence |
|---|---|---|
| Main loop and engine core | `0x0012f000`, `0x0012f378` | Entry/bootstrap relation, registered callback, repeated timing and update orchestration; medium |
| Game states | `0x0016fa80` | Global mode comparisons and coordinated calls; low; exact state identities unknown |
| Physics and collision | `0x00131618`, `0x00131da8` | PhysicsCar/CarCollision parameter-label references, movement dispatch and dt/vector calculations; medium |
| AI and gameplay | `0x00131410` | AIDriver allocation label and controller-mode switch; medium; not proof of AI algorithm semantics |
| World and scene management | `0x0019a4b8`, `0x001044c8` | Inherited camera name, distance/corner/matrix operations; low; function-body repair required |
| Graphics and VU/GS | `0x00120ef8`, `0x00282fa0` | COP2/matrix/buffer work; named GS syscall stub; medium |
| Audio | `0x0026a310` | Audio command/end/flush diagnostic references and buffer manipulation; high for audio involvement, not protocol correctness |
| Input | `0x00281f58` | libpad alignment/port diagnostics and RPC request; high for pad subsystem, original contract unverified |
| Memory and platform services | `0x00100008`, `0x00250340`, `0x002577c0`, `0x002514f0`, `0x00285958`, `0x00250ac0` | Startup/syscalls, fill loop, allocation hypothesis, format switch, semaphores/RPC and diagnostics; low to high by function |
| Resource management | `0x001ebd20` | Inherited flight-area name, buffer magic/offset/alignment fixups; medium, damaged body |
| Unknown | `0x001796b0` plus remaining unclassified entries | Complex object/global manipulation; no supported subsystem identity |

E-0002-06 links observed parameters at `0x001316ac` to PhysicsCar and
`0x00132598` to CarCollision. AIDriver appears as an allocation label in
`0x00131410`. Audio diagnostics at `0x002c2bb8`, `0x002c2c28`, and `0x002c2cb0`
have parameter references inside `0x0026a310`. libpad alignment text at
`0x002c4960` has a reference inside `0x00281f58`. These are identification anchors,
not executable specifications. Existing comments in `0x00131618` are treated as
inherited hypotheses and not independent supporting evidence.

## Complexity axes

The full CSV separates these axes; a small function can be expensive to port.

- Code: CFG estimate <=5 low, 6–20 medium, 21–60 high, >60 very high; empty graph unknown.
- Dependencies: high for observed indirect calls, >10 callees or >50 callers;
  medium for >2 callees; otherwise low in the recovered static graph. Unresolved
  targets can increase the true dependency closure.
- Hardware: COP2/syscalls mark high coupling; MMI marks medium; absence is unknown,
  not proof of platform independence. Semantic/hardware contracts still require analysis.
- Data: unknown globally; pilot entries explicitly distinguish byte buffers from
  unrecovered layouts, vtables, requests, global state and file fixups.
- Verification: pilot-specific low through very high according to observable
  state, timing, math, side effects and trace requirements; no verification has occurred.

These thresholds are triage rules. They must not be used as a substitute for
purpose, ABI, dependency and behavior analysis.

## Highly connected dependencies

| Entry | Static distinct callers | Finding / risk |
|---|---:|---|
| `0x002577c0` | 255 | Allocation-wrapper hypothesis; ABI currently decompiles as void while callers consume a result |
| `0x00114e98` | 229 | Thunk-style label targeting `0x00257850`; target behavior and alias/caller accounting need review |
| `0x002534a8` | 109 | Shared service; purpose not yet established |
| `0x002919d0` | 104 | Shared service; purpose not yet established |
| `0x00114e20` | 100 | Shared service; purpose not yet established |
| `0x00251398` | 97 | Formatting-family relationship suggested by the large formatter probe |
| `0x00250340` | 90 | Fill helper: useful low-coupling pilot, but only one slice of the full workload |
| `0x00285958` | 89 | RPC/semaphore request service: concurrency/order propagation risk |

The candidate graphics routine `0x00120ef8` has 45 static callees and estimated
complexity 233. The main-loop candidate `0x0012f378` has 39 static callees despite
complexity 13. The unknown large routine `0x001796b0` has 35 callees and complexity
148. These illustrate why instruction count or CFG complexity alone understates scope.

## Scope still unknown

No reliable partition between game code, PS2 SDK, compiler runtime, dead/demo
paths and duplicated/thunk behavior exists yet. Library recognition can reduce
manual reconstruction but still requires contract validation. Physics must remain
original-authoritative; no Godot-physics substitution is proposed.
Rendering adaptation must address observed COP2/VU work and private resources,
not merely translate high-level draw calls. Audio/input request adapters require
IOP/RPC behavior analysis. These are feasibility risks, not implemented systems.

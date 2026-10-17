# TASK-0038 runtime MCP feasibility

Evidence ID: **E-0038-007**. Date: 2026-10-17 (Australia/Sydney).
Scope: port-side emulator instrumentation and bounded original-runtime observations.
Target reference: SLUS_203.62, executable revision unknown, ELF SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Runtime observations corroborate ELF findings; they do not expand AGENT.md R0.
No recovered function is promoted to Verified.

## Observed tooling

The user-supplied `D:/Game/PCSX2-MCP-v1.0.0-win64` contains the compiled emulator,
Node MCP bridge and the custom DebugServer patch source. The running process was
PID69952 from that exact directory. Native registered PCSX2 MCP tools were not
advertised to this agent, but the bridge protocol is directly accessible with
standard Python sockets; no installation or MCP configuration changes were needed.

- DebugServer: `127.0.0.1:21512`, newline-delimited JSON; explicit `ok` replies.
- PINE: `127.0.0.1:28011`, existing `tools.oracle.Pine` protocol. An initial version
  request timed out while running; a later request after pause succeeded.
- PINE reported `PCSX2 d75a0ad`, game ID `SLUS-20362`, CRC `1d2818af`, version `1.00`.
  This custom-build string supersedes no previous captured v2.8.2 snapshot: it is
  another running executable, with no evidence that its emulation state is the same.
- SHA-256 of supplied executable: `bbf1ac83161c8077718985f83c8e35bcc1b4af5192078b0ed1b7cbbf5a1b4ff9`.
- SHA-256 of supplied `source/DebugServer.cpp`:
  `bc2334dc79fb3a40b6a6fe23cb1c4356eee3dad2e12029234ccd9d1e4483c66f`.

Patch source audit (local line numbers): `status`308; register categories321;
`read_memory`391; set/list/remove breakpoints487/579/519; pause625; resume635;
step644; step_over679; TCP newline framing876. The MCP bridge maps directly to
these commands in `pcsx2-mcp-server/dist/debug-server-client.js`.

## Capabilities and limitations

Read registers exposes GPR, CP0, FPR, FCR, VU0f, VU0i and GS, retaining 128-bit raw
values. Status exposes alive/paused/PC/cycles. Bounded memory reads, conditional
breakpoints, watchpoints and pause/resume exist. The source's conditional-breakpoint
parser does not report a failed expression parse: do not trust `ok` alone for a
condition. Read-memory substitutes zero for invalid reads, so bound physical EE
RAM before requesting it. The adapter does this, limits responses, and rejects
short/truncated responses.

**The supplied `step` is not a reliable instruction step.** It places a temporary
breakpoint at PC+4 and resumes, regardless of branch/delay slots. `step_over` uses
PC+8 for JAL/JALR and PC+4 otherwise. Both poll for a finite duration and return
`ok` even if the CPU is still running. Neither command is used by the adapter.
Source comments and README claims of delay-slot-aware stepping are countered by
the implementation. There is no atomic capture/step command or frame/input
injection command among the inspected DebugServer handlers. The final callback
`OnBreakpointHit` only contains a future notification comment; poll status.

`tools.pcsx2_mcp.DebugServer.run_until` uses a bounded, explicit execution deadline,
refuses to take over an existing breakpoint at the same address, and finally
pauses, checks paused state, then removes only its own breakpoint. This naturally
advances original state and cannot restore it. It never writes registers/RAM or
uses savestates. A stable paused snapshot checks PC and cycle equality before and
after register/memory capture. These checks do not prove machine-wide atomicity
or original ELF hash identity.

## Bounded observations and blocker

The emulator was running initially at observed PC `00111f0c`. A pause request was
followed by stable status/register reads: PC `00111f0c`, cycles3141698594. Existing
breakpoint/watchpoint lists were empty. `00137c88`'s first32 runtime bytes matched
the existing leaf instruction prefix (`3c013f80`, etc.).

FCR31 raw value was `0x01000061`; low two bits were1. This is only a runtime register
observation. It does not establish `cvt.w.s` rounding semantics or which original
instruction configured it; those require ELF instruction/architecture evidence.

A controlled natural-entry breakpoint at `00137c88` was armed for at most3seconds.
It did not hit. Cleanup left the EE paused at `00111f0c`, cycles4032240318, with
no breakpoints. No entry/return numerical comparison was possible. Paused memory
reads showed both `00330288` (previously evidenced World slot) and `00330148`
(previous replay slot) as zero. Therefore the previous replay driving snapshot
cannot be assumed to describe this newly supplied emulator session. No further
resumes were attempted; naturally reachable driving leaf comparisons require an
active original driving scene. No UI settings/input bindings/save slots were
changed. The current emulator remains paused.

Raw captures are ignored under `local/task0038/pcsx2-mcp/`: `initial.json`,
`paused.json`, `leaf-00137c88.json` (the latter records the timeout, not a leaf hit).
No frames, saves, ROM/ELF or captured memory enter the checked-in record.

## Validation and continuation

Passed with Python3.14.4:

```powershell
py -3 -m unittest tools.tests.test_pcsx2_mcp tools.tests.test_driving_oracle
# 12 tests passed; bounds, stability, existing-breakpoint protection,
# success/failure/timeout cleanup, no flawed step use.
py -3 -m tools.pcsx2_mcp
# Stable paused register snapshot into ignored local/; no execution.
```

Continue TASK-0038 by entering an original driving scene through user-controlled
normal game flow, pausing, and confirming a nonzero evidenced World slot plus
matching code entries. Then use a natural-entry owned breakpoint at `00137c88`,
capture a0/f12..f17 and *a1, and an owned breakpoint at the captured ra to capture
f0 and *a1 after return. Save bytes only under ignored local/. Match native
finite fixtures and explicitly record raw float differences and scenario scope.
Do not invoke the supplied step command or infer whole driving equivalence from
one leaf result. Original racing setup, function hit and synchronized numerical
comparison remain unresolved.

# Risk register — TASK-0002

Likelihood is qualitative judgment (not a measured probability). Evidence IDs
refer to EVIDENCE.md. All risks remain open unless stated otherwise.

| ID | Risk / evidence | Likelihood / impact | Mitigation and exit evidence |
|---|---|---|---|
| R01 | Function discovery/body errors: 26 empty CFGs, one-byte named bodies, remote chunks (E02–05) | High / critical | Read-only boundary audit, review instructions/callers; repair only in scoped task, repeat metrics before using sizes |
| R02 | Unknown ABI/types: 7,543 unknown conventions, one __stdcall label; allocation-return mismatch (E02/E05) | High / critical | R5900 register/stack/GP/return audit; fixed-width native values and testable serialization contracts |
| R03 | Unresolved indirect dispatch: 663 jalr and 190 other indirect jr sites (E04) | High / high | Recover vtables/switches/callbacks; record dynamic targets and dependency closure; switch recovery currently disabled |
| R04 | COP2/VU/MMI precision and side effects: 7,398 COP2-related sites, 606 MMI sites, VU sections (E01/E04/E05) | High / critical | Determine scalar/vector contracts, PS2 float/packing behavior and microprogram scope; deterministic original comparisons |
| R05 | Platform/IOP concurrency: syscalls, RPC-like helper and semaphores (E04/E05) | High / high | Trace request lifecycle, completion/order/timing; explicit host adapters and failure-path tests |
| R06 | Shared-service mistakes propagate: allocation candidate 255 callers; RPC candidate 89 (E03/E05) | High / high | Prioritize shared contracts, ABI and negative paths before callers; avoid assuming SDK idiom is a proven match |
| R07 | Physics/AI state uncertain: labels and vectors without recovered layouts (E06) | High / critical | Recover one bounded state closure, then timestep/collision/AI traces; original rules remain authoritative |
| R08 | Asset scope/formats: 402 private files, ~3.10 GB, extensions only known (E01) | High / high | Inventory manifests, one representative format per class; synthetic parsers before private extraction; measure conversion effort |
| R09 | Shader/render fidelity and VU overlays not assessed (E01/E05) | High / high | Define fidelity target; capture representative frames, decode packet/VU contracts; separate gameplay correctness from pixel fidelity |
| R10 | Static decompiler output mistaken for correctness: 21 outputs, warnings in 3, bad-body probes (E05) | High / critical | Keep output availability separate from semantic status; require original comparisons before Verified |
| R11 | No original oracle or PCSX2 timings in assessment (E05) | Confirmed gap / critical | Reproducible version/settings/input/save/capture manifest; controlled normal/edge traces and explicit tolerances |
| R12 | Uncalibrated rates and unknown manual-recovery fraction | Confirmed gap / high | Timed stratified pilots and explicit exclusions; publish scenarios as assumptions, never a committed schedule |
| R13 | Existing symbol/comment/import history unknown (E02/E07) | High / medium | Preserve annotations; separate inherited claims from observed instructions; record original version and analysis settings |
| R14 | Address-space/host-width/aliasing mismatch: 32-bit ELF, mapped aliases, GP globals (E01/E02) | High / high | Explicit PS2 layouts/handles; avoid casting original addresses into native pointers; layout and alias tests |
| R15 | Proprietary inputs accidentally versioned | Medium / high | Keep binary/ISO/assets/databases/raw captures in private or ignored local storage; review Git status/diff and tracked-file policy |

The successful scaffold bridge reduces infrastructure uncertainty only. It does
not close rendering, asset, original-rule or verification risks. No score here
is a claim that a particular failure has occurred in the original game.

Immediate gate: TASK-0003 for boundary/ABI quality, then TASK-0004 for original
comparison setup. Execute the varied pilot set after those gates, starting with
shared byte-buffer behavior but continuing into hardware/state-heavy functions.

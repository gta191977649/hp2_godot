# TASK-0007 — HP2 function naming (2026-10-11, Australia/Sydney)

## Outcome

Screened all **7,544** current Ghidra functions and reviewed all **20** assessment
pilots. Applied **16 evidence-backed purpose names**; **11 default jump-wrapper
aliases** automatically display their target's new name, for **27 changed entries**.
Ghidra was saved successfully. No trustworthy original developer function name
was recovered. All original-identifier fields remain `unknown`.

The supplied ELF has no SHT_SYMTAB/SHT_DYNSYM sections or entries, no `.debug*` or
`.stab*` sections, and an empty `.mdebug.eabi64` section. A scoped recursive search
of `D:/ps2_game` for map/sym/pdb/dbg/elf/SLUS files found only the known ELF.
This is not a claim that no alternate/debug build exists elsewhere.
Diagnostic strings were inspected from `.rodata`, `.data` and `.sdata` (3,587
NUL-terminated ASCII candidates), with targeted Ghidra references for vehicle,
audio and libpad labels. Labels and inherited comments do not prove original names.

Binary: `D:/ps2_game/SLUS_203.62`; SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
USA/NTSC inferred; executable revision unknown, boot VER 1.00. Ghidra 12.0.4,
`r5900:LE:32:default`, compiler `default`, existing hp2_ps2 project/MCP connection.

## Applied names and evidence

All names below are **analysis-purpose names**, not asserted SDK/developer symbols.
Confidence is high for the described operation, not for complete ABI or behavior.

| ELF address | New name | Evidence ID |
|---|---|---|
| 0x0012F000 | HP2_InitializeAndRegisterMainCallback | E-0007-N01 |
| 0x0012F378 | HP2_MainFrameLoop | E-0007-N02 |
| 0x00131410 | HP2_SetDriverControllerMode | E-0007-N03 |
| 0x00131618 | HP2_SetCarMovementMode | E-0007-N04 |
| 0x002500B0 | HP2_RingBuffer30Push | E-0007-N05 |
| 0x00250100 | HP2_RingBuffer30Pop | E-0007-N06 |
| 0x00250288 | HP2_CopyBytesForward | E-0007-N07 |
| 0x00250340 | HP2_FillBytes | E-0007-N08 |
| 0x00250428 | HP2_FindListNodeIndexOneBased | E-0007-N09 |
| 0x00250458 | HP2_SpliceListToTail | E-0007-N10 |
| 0x002504B8 | HP2_InsertListByPredicate | E-0007-N11 |
| 0x00250550 | HP2_SortListByAdjacentSwaps | E-0007-N12 |
| 0x00253520 | HP2_StringLengthOrZero | E-0007-N13 |
| 0x0026A310 | HP2_EndSoundCommand | E-0007-N14 |
| 0x00281F58 | HP2_OpenPadPort | E-0007-N15 |
| 0x00285958 | HP2_SubmitRpcRequest | E-0007-N16 |

Evidence combines current decompilation, listing, references, ELF strings/bytes
and existing caller/callee metadata. Direct candidates had body byte counts equal
to their current envelopes, valid instruction alignment and DEFAULT symbols.
For example: AIDriver/PhysicsCar label references corroborate replacement-object
mode switches; SOUND_EndCommand corroborates command-buffer append/reset logic;
libpad port/alignment diagnostics corroborate RPC-backed port initialization.
The memory helpers were deliberately not named `memcpy`, `memset` or `strlen`:
original names and standard return/null/overlap contracts are not established.

The 11 aliases are ELF `J` instructions with NOP delay slots targeting the splice
or sort functions. Ghidra renders their target name directly after the target
becomes USER_DEFINED. Duplicate alias display names are expected; original
addresses and generated `_0xADDRESS` suffixes uniquely identify each function.
No alias was assigned a speculative independent purpose.

Shareable records:

- `reverse/naming/TASK-0007-name-map.csv`: all 27 changes, source types and evidence.
- `reverse/naming/TASK-0007-screening.csv`: one disposition per original function.
- `reverse/naming/TASK-0007-pilot-review.csv`: all 20 pilot decisions.
- `reverse/naming/TASK-0007-reviewed.json`: reviewed candidates and deferred hypotheses.
- `reverse/symbols.csv` and per-address records: 16 Decompiled analysis entries plus
  11 Identified jump aliases; no native implementation or Verified promotion.

Screening is triage, not semantic recovery of every body: 16 approved purpose
names, 28 boundary-blocked entries, 8 other deferred candidates, 197 existing
thunks, 64 preserved existing names and 7,231 unresolved functions. Syscall,
COP2, indirect-call and caller-count hints are prioritization signals only.
Existing system names were preserved; observed `SetGsCrt` symbol source is
ANALYSIS, not original ELF provenance. SDK-version/function-signature matches
were not established.

## Deferred findings

`HP2_DemoCameraCoordinator_Update` at 0x0019A4B8 remains a low-confidence existing
label: vector-distance/visibility tests and camera-object construction support a
camera-coordination hypothesis, but the body is still one byte. Its authorship is
unknown. `HP2_LoadFlightAreas` at 0x001EBD20 validates magic 0x80034800, aligns a
pointer and performs fixups; its one-byte body and missing independent resource
type evidence prevent confirming the flight-area name. Both were preserved.

The 475,968-byte envelope at 0x0016FA80 remains blocked; its mode branches are
insufficient to establish a complete game-state routine. Allocation candidate
0x002577C0 still has unresolved return ABI. Renderer, car-update, formatting and
other pilot names remain hypotheses. Eleven deferred-review records retain these
observations separately from the curated applied-symbol index.

## Private output and reproducibility

`ps2_decomp/recompiled-named/` contains 7,544 function files, registration and two
headers: **7,516 translations and the same 28 unimplemented placeholders**.
Generation exited 0; generated files total 138,489,762 bytes before supplementary
documentation. Prior TASK-0006 outputs/configs/snapshots were not overwritten.

`ps2_decomp/naming/` contains before/after function snapshots, affected symbol
metadata, ELF audit, raw probes, string references, apply journal, per-address
rollback map, saved-project response, named CSV/TOML and validation logs. All
proprietary listings/code and generated outputs remain Git-ignored.

The scripts are a recorded one-run workflow, not commands to blindly reapply to
the now-renamed project. Snapshot/name/output guards refuse overwriting existing
state. A fresh collection needs an explicit new task/run directory.

```powershell
# Original read-only collection and review preparation, before applying names:
py -3 reverse/scripts/collect_hp2_naming.py
py -3 reverse/scripts/prepare_hp2_names.py
# Synthetic safety checks (safe to repeat):
py -3 -m unittest discover -s reverse/tests -v
# Original mutation command; the journal prevents a second application:
py -3 reverse/scripts/apply_hp2_names.py --apply
# Export into a new output directory; refuses an existing recompiled-named/:
py -3 reverse/scripts/export_named_hp2.py
& local/ps2recomp-build/ps2xRecomp/ps2_recomp.exe ps2_decomp/naming/hp2-named.toml
py -3 reverse/scripts/document_hp2_names.py
py -3 reverse/scripts/validate_named_hp2.py
```

PS2Recomp is the previously built user-supplied D:/dev/PS2Recomp-0.4 tree; source
fingerprints/dependency identities are retained from TASK-0006 and checked again
in `naming/tool-identity.json`. Its module advertises 0.1.0, so the directory label
0.4 remains unverified. Native compiler is MinGW-w64 GCC 13.2.0; Python 3.14.4.
No tool source edits, instruction patches, forced runtime stubs, auto-analysis,
prototype changes or body repairs were made.

## Validation, rollback and limitations

- Passed: 11 synthetic tests for stale/custom names, invalid names/categories,
  missing evidence, duplicate addresses/names, collisions, damaged envelopes,
  journal-before-write, failures before/after write, name rollback, alias rendering
  and unrelated namespace/name changes. These test tools, not reconstructed game behavior.
- Passed: all 16 newly named bodies, registration, one jump alias, retained camera
  placeholder and largest unchanged generated body passed C++ syntax checks (20 total).
  Exact argument arrays and per-file exit codes are in `named-validation.json`.
  Each uses `g++ -std=c++20 -msse4.1 -mavx -fsyntax-only`, runtime include and Kernel
  include directories, and the named-output include directory. No full link/boot performed.
- Passed: ELF SHA-256 unchanged; all 7,544 entry/envelope bounds unchanged;
  calling conventions/namespaces unchanged; precisely 16 direct + 11 alias name
  changes; Ghidra/map/27 generated names agree; exclusions match by address.
- Observed failure: initial postcondition expected `thunk_HP2_*` rendering and
  stopped before saving. Read-only investigation confirmed actual target-name
  rendering, with no unexpected boundary/name changes. Corrected the alias model
  and synthetic test, then `apply_hp2_names.py --finalize` validated the fully
  applied journal, wrote snapshots/rollback map and saved Ghidra successfully.

To restore the recorded names, with the same ELF and connected current project:

```powershell
py -3 reverse/scripts/apply_hp2_names.py --rollback
```

Rollback preflights current names/bounds and refuses unrelated edits. It restores
direct old names and explicitly restores alias names if Ghidra rendering requires
it. **This is name rollback, not a full Ghidra database restore**: the available
symbol API cannot restore DEFAULT source type, and explicit alias restoration can
change automatic-name inheritance. Old source types are retained in the snapshot
and mapping for a manual Ghidra provenance repair. Live rollback was not executed;
the direct mutation/rollback engine is tested on synthetic state, and alias
restoration remains a documented live limitation. A rollback also requires a
separate deliberate update of published metadata/code; do not treat current named
outputs as synchronized after rolling back Ghidra.

Evidence IDs: E-0007-001 identity/ELF audit; E-0007-002 full snapshot/screening;
E-0007-N01–N16 reviewed purpose observations; E-0007-P-ADDRESS deferred probes;
E-0007-003 journal/actual alias investigation/save/rollback map;
E-0007-004 named generation; E-0007-005 synthetic and syntax checks.
Raw probe SHA-256 values are in the reviewed private plan and function records.

No original/PCSX2 comparison was performed, no recovered behavior test was
invented, and no symbol is Verified. No Git commit or staging change was made.
Next: TASK-0003 boundary/ABI audit of 0x0016FA80, 0x0019A4B8, 0x001EBD20 and
0x002577C0; then collect evidence for the next bounded batch of unresolved names.

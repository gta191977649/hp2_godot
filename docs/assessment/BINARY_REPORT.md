# Binary report — 2026-10-10

Assessment task: TASK-0002. Evidence IDs and collection procedure are in
[EVIDENCE.md](EVIDENCE.md). No game implementation or original-execution
verification was performed. This is a snapshot of existing analysis, not a claim
that Ghidra has discovered all functions.

## Input identity

| Field | Observation |
|---|---|
| Source ELF | `D:/ps2_game/SLUS_203.62`, supplied through the connected Ghidra project |
| Size | 2,321,608 bytes |
| SHA-256 | `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2` |
| ELF | ELF32, little endian, machine 8 (MIPS), entry `0x00100008` |
| Ghidra project/program | `hp2_ps2` / `/SLUS_203.62`, already open |
| Language/compiler | `r5900:LE:32:default` / `default` |
| Region | USA/NTSC inferred from local ISO filename and `SYSTEM.CNF`; no independent release matching |
| Boot configuration | `BOOT2 = cdrom0:\SLUS_203.62;1`, `VER = 1.00`, `VMODE = NTSC` |
| Exact executable build/revision | Unknown; boot VER is not proof of an executable revision |
| Product | Local ISO filename identifies Need for Speed — Hot Pursuit 2 (USA); release identity remains provisional |

The source path, ELF sections, and executable bytes agree with the open program.
Every byte of Ghidra's `.text` listing was reconstructed and compared with the
disk ELF: exact match. Loaded `.text` SHA-256 is
`f3f8392acf663a88c07c5b67b20b42c098d6db8e37e994961d7c05177428dab9`.
Other sections were not exhaustively compared, and Ghidra's original imported
whole-file hash is not exposed by the metadata resource. The recorded ELF hash
identifies the current disk file, with this explicit correspondence limitation.

## Tools and current configuration

- Ghidra 12.0.4 PUBLIC; installation metadata build 2026-Mar-03.
- Emotion Engine Reloaded installed; language-definition version 1.4.0,
  extension host compatibility 12.0.4. Its exact release/commit is unknown.
- GhidraMCP extension description 0.7.0-pre15, compatibility metadata 12.0.4.
  Read-only program, function, graph, listing, reference and decompile queries worked.
- Java Temurin 21.0.11; Python 3.14.4 for metadata aggregation.
- Native/Godot scaffold versions and validation remain recorded in PROGRESS.md;
  they were not rebuilt for this documentation/data assessment.

Current R5900 Constant Reference Analyzer and Unaligned Instruction Fix are
enabled. Global GP recovery and pointer analysis are enabled. Switch-table
recovery is disabled, T9-at-entry is assumed, and writable memory values are
trusted. These are observed current options, not proof of the original import
or analysis history. Loader is reported as ELF Loader. Previous import settings,
analysis runs, and manual annotation provenance are unknown.

## Inventory measurements

| Measurement | Result and scope |
|---|---|
| Ghidra function entries | 7,544; complete paginated export, 7,544 unique addresses |
| Symbols | 11,235 reported by program metadata; origin/quality not audited |
| Default-style function names | 7,477 (`FUN_` or `thunk_FUN_` prefix) |
| Other function names | 67; includes entry, two HP2 names and 64 possible SDK/system labels |
| Thunk-style names | 197 name-prefix matches; not an independent thunk audit |
| Call relationships | 17,162 distinct directed function pairs, depth-one caller/callee queries for all entries; no reported truncation |
| `.text` size | 1,726,724 bytes, `0x00100000–0x002a5903` |
| Decoded `.text` instructions | 400,582 existing four-byte instruction code units |
| Decoded byte fraction | 92.796%; remaining 124,396 bytes are not decoded as instructions in this listing |
| Instructions in returned CFG block intervals | 392,685 distinct physical instruction addresses; shared blocks counted once globally |
| Empty CFGs | 26; real function instruction count/complexity unknown |
| Indirect calls | 663 `jalr` sites in decoded `.text`; not a count of distinct runtime targets |
| Other indirect transfers | 190 `jr` sites whose operand is not `ra`; may include switch dispatch, tail calls or other transfers |
| Direct `jal` targets lacking a function entry | 0 among decoded `.text` direct jal targets; does not establish resolved indirect calls or references |
| Unresolved references, complete program | Unknown; no complete operand/reference or dynamic target audit |
| Decompile probes | 21 distinct entries; 21 returned output, 0 probe failures |
| Probes containing warnings | 3 functions; output availability does not mean correct semantics |
| Remaining unprobed entries | 7,523; global decompile success/failure counts unknown |

The generic decompile query initially used an unsupported address-as-name form;
it failed lookup and was retried with `address`. This is not counted as a failed
function decompilation. Probe outputs remain private; report metadata is public.

`FUNCTION_INVENTORY.csv` provides counts per entry. Its `cfg_instruction_count`
counts decoded instructions inside returned CFG block intervals, including
delay slots. It is **not** an independently verified original function size.
Block models can include instructions beyond a damaged body, and missing
instructions/boundaries can suppress counts. For example, `0x001ebd20` reports
a one-byte body but a five-instruction block: its full original size is unknown.
`0x0019a4b8` has an empty CFG despite substantial decompiler output. Min/max
address spans must not be divided by four: some bodies contain remote chunks.

## Complexity snapshot

Estimate `M = E - N + 2P`, with CFG nodes N, returned edges E, and weakly connected
components P. Calls, exceptions and unresolved jumps are not necessarily modeled.
All nonempty returned CFGs were connected; missing switch edges remain a concern.

| Estimated complexity | Function entries |
|---|---:|
| Low, M <= 5 | 6,103 |
| Medium, 6–20 | 1,224 |
| High, 21–60 | 173 |
| Very high, >60 | 18 |
| Unknown, empty CFG | 26 |

Largest observed estimates: `0x00120ef8` = 233, `0x0017f040` = 219,
`0x00298138` = 193, and `0x001796b0` / `0x002514f0` = 148.
These are prioritization signals, not estimates of engineering hours.

## Hardware, libraries and debug information

Decoded `.text` includes 7,398 COP2-related opcode sites (COP2/lqc2/sqc2), 606
MMI-major-opcode sites, 75 syscall instructions and 17 cache instructions.
There are 178 LUI constants matching mapped hardware/scratchpad address prefixes;
these are candidates, not confirmed MMIO accesses. The import exposes MMIO,
VU, GS, IOP and scratchpad memory blocks; their presence alone is loader metadata.

ELF `.vutext` contains 20,624 bytes and eight DVP overlay sections contain 12,504
bytes. VU program execution, overlay relocation/loading and IOP modules have not
been analyzed. EE instruction statistics exclude these sections. Data contains
PADMAN/STREAM module-name references; actual module execution is unverified.

Possible SDK/runtime families include named syscall/semaphore/thread/SIF/GS
stubs, byte fill `0x00250340`, formatting `0x002514f0`, RPC-like `0x00285958`,
and libpad-like `0x00281f58`. Names and idioms are candidates, not SDK-version
matches. Game-specific candidates are described in SUBSYSTEM_ANALYSIS.md.

The ELF section table has no `.symtab`, `.dynsym`, or DWARF `.debug_*` sections;
`.mdebug.eabi64` has size zero. It contains diagnostic labels and source-path
strings, which can help identification but are not debug symbols or original
function signatures. Existing HP2 names/comments may be manual annotations;
their authorship and correctness are unknown. 7,543 calling conventions are
`unknown`; one `_Exit` entry has `__stdcall`, which needs an R5900 ABI audit and
must not be accepted as evidence of the original convention.

## Asset scope and limitations

The private `D:/ps2_game/GAME` directory contains 402 files totaling 3,098,727,312
bytes: 128 `.bin`, 92 extensionless, 80 `.lzc`, 44 `.bun`, 18 `.avi`, 18 `.msb`,
10 `.pck`, 8 `.cam`, and one each `.ctb`, `.ltb`, `.jbc`, `.evt`.
Extensions do not establish formats, compression or required converters.
No asset payload was extracted or converted, and the ISO was not hashed.

No original-execution traces or PCSX2 comparisons are available for this
assessment. Function discovery, ABI/type recovery, VU/IOP coverage and comparison
setup remain blockers to a calibrated full-game schedule, not blockers to the
completed static assessment. Raw inputs, listings and decompiled source remain
outside versioned deliverables.

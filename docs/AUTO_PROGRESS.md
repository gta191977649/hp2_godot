# Automatic recovery progress (TASK-0073)

Start the save watcher once in a PowerShell terminal from the repository root:

```powershell
./tools/track_progress.ps1
```

Keep that terminal running; Ctrl+C stops it. No system service or Git hook is installed.
The watcher polls every two seconds and refreshes after a changed input stays stable
for at least one second. Build startup also refreshes once after CMake configuration.
These commands need no AI agent:

```powershell
./tools/track_progress.ps1 -Once
./tools/track_progress.ps1 -Check
./tools/track_progress.ps1 -Once -RefreshGhidra
```

The wrapper uses `-Python`, then `HP2_PROGRESS_PYTHON`, existing CMake interpreter
configuration, then a non-WindowsApps `python` command. Set an explicit Python 3.11+
interpreter if discovery fails. `py -3` is not required. Direct Python equivalents:

```text
python reverse/scripts/track_progress.py --watch
python reverse/scripts/track_progress.py --once
python reverse/scripts/track_progress.py --check
python reverse/scripts/track_progress.py --once --refresh-ghidra
```

## What is counted

`reverse/progress.json` is the deterministic report; `generate_progress.py` owns
the entire `docs/PROGRESS.md` dashboard (TASK0078). It contains only one Progress
headline, a collapsed CLASS table, and a collapsed Functions section with nested
class groups. Every `<details>` omits `open`, so all lists start collapsed. Both
outputs are checked by `progress_tables`. The generator computes from inputs;
it does not trust an older report. Historical handwritten tracker content is
preserved in `docs/PROGRESS_ARCHIVE_TASK0078.md`; session narrative stays in
PROGRESS_LOG.md, whose prose remains untouched by generation. Game code is unchanged.

Inputs are the existing template inventory, class CSV, TASK-0043 member registry,
`*-debug-canonical-owners.json`, binary/symbol registries, per-function records and
current `src/game_core` definitions. The watcher additionally watches boundary-source
directories, structure records, task files and assessment inventories used by the
existing tracker. It never regenerates class templates or overwrites evidence registries.

Functions are keyed by binary SHA-256 and entry address. Shared ownership appears
in each explicitly registered class but is deduplicated globally. Retail and Debug
remain separate. The headline means **evidenced bodies / all inventoried Retail
functions**, using the 7544-entry assessment inventory; Debug never inflates it.
CLASS percentages remain **evidenced bodies / bound methods**, not a claim of
complete class coverage. Classes with no bound methods show `unknown (0/?)`.

Functions includes the full Retail inventory, the complete 10069-address Debug
inventory retained from TASK0073's identity-checked read-only catalog, and additional
recorded/bound entries outside those inventories. The Debug address-only CSV and
its SHA/provenance JSON live at `reverse/naming/task0078-debug-function-inventory.*`;
the earlier TASK0041 symbol-name CSV supplies names without guessing new ones.
The complete inventory no longer depends on an ignored local cache or a running
Ghidra instance. E-0078-I01 records its derivation and identity limitations.

No class is inferred from a mangled name: entries without an explicit binding
appear under `Unassigned / global`. Every entry shows implementation state and
record status separately; these can disagree without changing evidence. Unknown
original identifiers fall back to address-anchored `Sub_<ADDR>` display names.
Outside-inventory entries are listed but do not enlarge the Retail headline
denominator. The compatible `functions` JSON field retains the class-bound
inventory; `inventory_functions` adds the full catalog and `overall` the headline.

| State | Requirement |
|---|---|
| Missing/unresolved | No unique address-bound definition was found |
| Throwing stub | Body consists solely of `not_implemented` |
| Unconfirmed body | A body exists but evidence/status is insufficient, or definitions are ambiguous |
| Partial | A body contains an explicit untranslated path plus other logic |
| Evidenced body | A unique bound body, matching SHA/address record with evidence IDs, and a recorded Implemented/Tested/Verified status |

The lexical scanner masks comments and literals, balances braces and recognizes
qualified out-of-line method definitions. It is deliberately conservative: unsupported
syntax, unanchored inline methods, macros, inferred owners and unresolved overloads do
not earn implementation credit. Debug methods require an explicit Debug address anchor.
The scanner does not preprocess C++; counts describe source/evidence bindings, not compiled coverage.
No schema or evidence status is promoted by deleting a stub. Invalid UTF-8 records do
not establish implementation evidence. Claim/source conflicts are listed in the JSON.

Partial detection is **direct-body only**; a body can call a dependency that still
throws. Existing implementation records can describe bounded domains. Therefore an
evidenced body is a traceable claim, not proof of complete behavior or equivalence.
Host-test and original-verification columns report existing claims separately; no
test execution or original comparison is inferred from function names or source presence.

## Ghidra and offline operation

The watcher checks MCP on startup and every ten minutes on a background worker.
Ordinary saves and builds require no network. `-Endpoint` / `--endpoint` overrides
the default `http://127.0.0.1:8080/mcp`. Explicit refresh returns exit 2 when catalog
refresh is incomplete; local progress remains available.

The client only calls resource reads, `tools/list`, `functions` with action `list`,
and `read_tool_output` with action `read` for oversized responses. It follows pagination
and rejects repeated cursors, errors, mismatched identities, empty or count-inconsistent
catalogs. Each network request has an eight-second timeout; a catalog has a 45-second
budget checked between requests. Failed refreshes retain the last complete catalog.

The current MCP program-info response omits SHA-256. The script then requires the
returned import path to match the corresponding configured original ELF
(`config/ps2_elf.local.json` or `config/ps2_debug_elf.local.json`) and hashes that file.
This binds the original import file identity; **it does not certify database memory
against the file**. That limitation is explicit in each cached catalog. A provider
with an executable SHA-256 field uses that field directly. No name-only identity is accepted.

Raw catalogs, import metadata and connection status stay under ignored `local/progress/`.
`status.json` shows the last attempt and per-program freshness/errors; successful catalogs
include bound addresses missing from the live function directory. They do not change
implementation states. No proprietary listings, ELF files or assets enter the report.

## Validation and continuation

```text
python -m unittest discover -s reverse/tests -p test_recovery_progress.py
python reverse/scripts/track_progress.py --check
ctest --preset native-debug -R "^(progress_tables|recovery_progress_contracts)$" --output-on-failure
```

TASK-0073 established this infrastructure; TASK-0078 owns its compact layout and
full catalog presentation. New ownership/address mappings belong in the
evidence-backed registries; implementers must not add semantic guesses to the scanner.
Concurrent writers use an OS lock; writes are atomic and check for intervening document
edits. Unchanged content is not rewritten. Refresh errors remain visible and the watcher
retries after another stable interval.

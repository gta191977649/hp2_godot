# Assessment evidence and reproduction

Date: 2026-10-10 Australia/Sydney. Task: TASK-0002. Binary ID:
`sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.

| ID | Observation/source | Scope and limitation |
|---|---|---|
| E-0002-01 | Get-FileHash, ELF header/section parsing, SYSTEM.CNF, local file inventory | Disk identity; region/build inference limited |
| E-0002-02 | `ghidra://programs`, `ghidra://program/SLUS_203.62/info`, paginated functions list | Existing project; not complete binary discovery |
| E-0002-03 | `analyze graph` and depth-one `call_graph` for every entry | Existing CFG/reference model; unresolved targets omitted |
| E-0002-04 | Complete paginated `.text` listing; every returned byte matched disk | EE `.text` only, not VU/IOP or execution |
| E-0002-05 | 21 distinct decompile probes | Output available; semantics/signatures unverified |
| E-0002-06 | String references, diagnostic text, and selected decompilation observations | Subsystem hypotheses; names/comments can be inherited annotations |
| E-0002-07 | Extension/application properties and current MIPS analysis options | Installed/current settings, not full analysis history |

The checked-in CSV, CALL_EDGES.csv, MEASUREMENTS.json and PILOT_SELECTION.json
are metadata-only exports. Original instructions/decompiled source are not
included. Local captures are ignored under `local/assessment/`:

- mcp-functions.json: complete 7,544-entry paginated function export.
- metrics-full.json: graph/call queries for the same ordered entries.
- text-listing.json: complete decoded/data code-unit listing of `.text`.
- pilot-decomp.json, extra-decomp.json, pad-decomp.json: private output probes.
- pilot-config.json: mirrors shareable PILOT_SELECTION.json.

To recollect in a future connected session, query `ghidra://programs` and verify
the intended program/hash first. Use read-only `batch_operations` entries:

```json
{"tool":"functions","arguments":{"action":"list","page_size":500}}
```

Continue with the returned `next_cursor` until absent; do not use the functions
resource's default 1,000-row preview as a full inventory. For every entry:

```json
{"tool":"analyze","arguments":{"action":"graph","address":"<entry>"}}
{"tool":"analyze","arguments":{"action":"call_graph","address":"<entry>","depth":1,"direction":"both","max_results":500}}
```

Capture `.text` with `inspect listing`, `address=00100000`,
`end_address=002a5903`, `max_lines=1000`, following every listing cursor.
Decompile the 20 pilot addresses plus `0x00107cd8` with `inspect decompile`,
`address=<entry>`, `timeout=30`. Do not use an address string as a function name.
Use explicit program `SLUS_203.62` for every query. Oversized batch outputs are
retrieved through `read_tool_output`, view=json, following `nextOffset` and
concatenating content before JSON parsing; inline previews are not full captures.
No rename, type mutation, analysis run or save was requested during collection.

Reaggregate captures from repository root:

```powershell
py -3 reverse/scripts/summarize_assessment.py --elf D:\ps2_game\SLUS_203.62 --captures local/assessment --output docs/assessment
```

The script asserts matching populations, unique entries, complete `.text` byte
coverage, exact disk correspondence and 20 unique pilot selections. It computes
counts from decoded instruction addresses, not bounding spans. Recollection on
a changed project will produce a new snapshot; record changes rather than
silently replacing historical conclusions. This initial script intentionally
expects 7,544 entries; adjust the assertion explicitly for a new analysis version.

METADATA validation must confirm 7,544 CSV rows, 17,162 distinct edges, 21 probed
outputs, 20 pilot IDs, consistent subsystem totals, and no fabricated addresses.
Rates in ESTIMATE_SCENARIOS.json are assumptions, not evidence IDs for observed
engineering speed. PCSX2 checks and implementation timings were not performed.

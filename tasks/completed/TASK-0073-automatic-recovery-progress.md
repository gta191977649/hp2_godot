# TASK-0073 - Automatic recovery progress

- Status: completed
- Scope: deterministic class/function progress, save-triggered watcher, build refresh and read-only Ghidra identity/catalog checks. Port-side infrastructure only.
- Dependencies: TASK-0026 tracker and existing class/member/canonical-owner registries.
- Evidence: repository inputs; Ghidra program inventory and paginated Debug function-list availability checked read-only. No new game behavior finding.

## Acceptance

- [x] SHA/address-keyed report covers registered Retail and Debug classes separately without crediting shells as implementations.
- [x] Source gaps, unconfirmed bodies and evidence-backed implementation claims remain distinct; no status promotion.
- [x] Watcher refreshes after stable saves, build entry refreshes once, generated checks detect stale output.
- [x] MCP identity checks, complete pagination and offline cache retention are tested.
- [x] Regression tests cover scanning, ambiguity, deduplication, deterministic output and safe writes.
- [x] Documentation, generated tables and reproducible continuation are current.

## Initial constraints

Existing modified/untracked files are user work and must be preserved. `py -3` is unavailable; existing CMake caches identify Python 3.14.4 at C:/Users/nurupo/AppData/Local/Python/pythoncore-3.14-64/python.exe. No Godot launches are required. Ghidra remains read-only. Versions unknown in the binary registry remain unknown.

## Next action

Run `./tools/track_progress.ps1` to start the foreground watcher, or
`./tools/track_progress.ps1 -Check` for offline validation. The validation watcher
was explicitly stopped after testing; no service or background watcher is left running.

## Findings and limits

- Report: 419 binary/class rows, 1450 unique binary/address entries, separated by Retail/Debug. Unknown totals remain unknown.
- Live read-only MCP: Retail 7544 entries (254 registered addresses absent from its function directory), Debug 10069 entries (zero registered misses). Missing entries do not invalidate existing evidence or establish boundaries.
- Oversized responses require read_tool_output chunk reads. Both catalogs passed complete pagination and program count checks; cached results and freshness are ignored local files.
- Program info has no SHA. The configured original import path is required and the original file is SHA-checked; database memory is explicitly not certified.
- Lexical qualified-definition scanning does not preprocess C++, infer inlined semantics or prove transitive completeness. Explicit direct gaps remain partial. Existing bounded implementation claims are not whole-game correctness.
- No evidence registry, game source, verification status or Ghidra database was modified. No proprietary material was added to Git.

## Validation (2026-10-20)

Python 3.14.4, CMake 3.29.2, Ninja 1.12.0 on Windows. Python commands below used
`C:/Users/nurupo/AppData/Local/Python/pythoncore-3.14-64/python.exe`; the wrapper
discovers that path from existing CMake caches.

- `python -m unittest discover -s reverse/tests -p test_recovery_progress.py`: PASS (22 behavior tests, including stale report rejection and handwritten-prose preservation).
- `cmake --preset native-debug`: PASS.
- `./build.ps1 --preset native-debug --jobs 4`: PASS, all 178 build steps; automatic progress refresh ran before compilation. Existing C++ indentation warnings remain outside this task.
- `./tools/track_progress.ps1 -Once -RefreshGhidra`: PASS, both live catalogs fresh after stored-output compatibility handling.
- `ctest --preset native-debug -R '^(progress_tables|recovery_progress_contracts)$' --output-on-failure`: PASS, 2/2.
- Actual `--watch` smoke: started owned PID 58540 with ignored logs, saved this session's PROGRESS_LOG entry, observed automatic `UPDATED docs/PROGRESS_LOG.md contents`, then stopped owned process. No generated-output feedback loop observed.
- Scoped `git diff --check`: PASS. Existing modified/untracked work preserved; nothing staged or committed.

Initial Python-launcher and sandbox temporary-directory/socket failures were environment
failures, not recovered-behavior results. Used the configured interpreter; subsequent
unsandboxed validation passed. Full native CTest and Godot launches were unnecessary
for this tooling change; no original-runtime comparison or Verified claim.

## Related commits

None.

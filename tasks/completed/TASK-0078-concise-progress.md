# TASK-0078 - Concise automatic recovery progress

- Status: completed
- Request: docs/PROGRESS.md contains only a large overall percentage/count, collapsed CLASS coverage and collapsed Functions grouped by class, listing all known classes/functions.
- Scope: deterministic report/rendering, inventory coverage, generator/watcher integration and regression tests. No game source, evidence promotion, Ghidra writes or graphical tests.
- Baseline: current dirty tree preserved; generator inputs copied under ignored local/task0078/baseline. Original progress page preserved in docs/PROGRESS_ARCHIVE_TASK0078.md, including handwritten continuation knowledge.

## Acceptance

- [x] Exactly the requested summary and two collapsed top-level sections; no tasks/logs/blockers or unrelated prose in PROGRESS.md.
- [x] All registered classes and inventoried/recorded functions visible, grouped only by explicit class bindings; unassigned entries remain separate.
- [x] Retail headline denominator comes from its full inventory; Debug stays SHA-separated. Deduplicate shared addresses globally, retain shared class memberships and unknown class totals.
- [x] Strict evidenced-body credit is unchanged; stubs/partial/unconfirmed/missing entries do not inflate completion. Existing record status remains independently visible.
- [x] Generator, save watcher, build refresh and stale-output checks produce the same compact page without feedback loops.
- [x] Focused tests, generated checks, documentation and continuation updated; original source and unrelated user work preserved.

## Continuation

Run ./tools/track_progress.ps1 for save watching, -Once for refresh or -Check for
offline validation. Every outer and nested details group starts collapsed.
TASK0077 remains incomplete after interrupted graphical controls; no further
rendering benchmarks were scheduled for this task.

## Results and validation (2026-10-21)

419 SHA/class rows; 17886 unique known SHA/address entries: 7544 inventoried
Retail, 10069 inventoried Debug, and 273 outside-inventory Retail bindings/records.
The strict evidenced-body Retail headline is 0.4% (30/7544); this differs from the
former record-coverage percentage and is not original-runtime verification.
Shared ownership duplicates display rows only, never headline credit. E-0078-I01
records the complete Debug inventory; no new behavior or class attribution.

Tools: Python3.14.4, CMake/CTest3.29.2, Ninja1.12.0, Windows PowerShell wrapper.

- py -3 -m unittest discover -s reverse/tests -p test_recovery_progress.py: PASS29/29, including full inventories, shared deduplication, Debug separation, default collapse, escaping, stale whole-page checks and save-loop exclusion.
- py -3 reverse/scripts/generate_progress.py and --check: PASS.
- ctest --preset native-debug -R '^(progress_tables|recovery_progress_contracts)$' --output-on-failure: PASS2/2.
- ./tools/track_progress.ps1 -Check: PASS through the normal wrapper.
- Dashboard HTML structural check: PASS, one heading, exactly two outer sections, all318 details groups closed by default and balanced.
- Actual watcher loop with mocked offline MCP: PASS; initial refresh plus one debounced task-save refresh, no feedback refresh; exited cleanly. Results: local/task0078/watch-smoke.json. Initial timer-based fixture did not observe the edit; retry schedules it only after the initial snapshot and preserves that failed attempt as fixture counterevidence.

No native compilation or graphical test was needed: this changes reporting only,
not game code or runtime behavior. No evidence status, Ghidra database, assets or
recovered identifiers were modified. Old progress text is preserved verbatim in
docs/PROGRESS_ARCHIVE_TASK0078.md. Nothing staged or committed.

## Related commits

None created.


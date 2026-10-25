# TASK-0097 - Ultimate Racer Debug inventory and version boundary

- Status: completed
- Parent: TASK-0096; single current implementation scope.
- Binary Debug NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
- Scope: exact call/state/ownership graph from event through selected vehicle to World,
  race/police/outcome; all32 original Hot Pursuit records and field meaning/version
  correspondence; testable portable parameter foundations only where evidence complete.
- Evidence E-0097-*; raw captures ignored local/task0096/.

## Acceptance

- [x] Binary hashes/tool versions/pins/original assets/MCP availability checked.
- [x] Debug frontend/selection/World/race/police/outcome dependency graph documented.
- [x] All32 events inventoried with confirmed fields and explicit per-field unknowns.
- [x] Existing Retail-to-Debug record/profile/ID compatibility audited, not assumed.
- [x] First bounded portable behavior implemented and tested if evidence permits.
- [x] Baseline source/performance risks and original-runtime blockers recorded.
- [x] Evidence/function/symbol records and generated progress current.

## Starting observations

Existing event confirmation throws at Retail001F6A28; selectcar.cpp contains
throwing slices and no full selector. RaceCoordinator has only an empty declaration.
Current race_scene.gd explicitly owns a single-car test fixture, not original races.
NativeBackend copies logical Memory per candidate frame; multi-car World must not
multiply that into independent car worlds. Confirm current code before reuse.

## Findings and completion2026-10-26

All phase acceptance evidence is recorded in reverse/task0097/inventory.md,
event-coverage.csv and evidence.json (130 distinct Debug entry captures; this is
an inventory, not130 implemented functions). All32 Hot Pursuit nodes and other
32 Championship records decode;7 Hot Pursuit event types identified. Common
record fields validated, incompatible KO progress size identified12vs16; no unsafe
profile adapter enabled. Source/performance risks and missing original-runtime
prerequisites recorded. Native getters/state-size/track mapping implemented;
ultimate_event and eventtree tests2/2 passed1.36s. Godot extension build result
recorded in progress log. Inventory completion is NOT whole-mode completion or
Verified. Continue TASK-0098; unresolved World/AI/Police/outcome dependencies are
explicitly assigned to later subtasks. No commit/staging.

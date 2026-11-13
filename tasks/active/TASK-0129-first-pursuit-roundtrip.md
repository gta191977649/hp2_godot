# TASK-0129 - First Hot Pursuit event roundtrip

- Status: active
- Scope: admit the donor's first Hot Pursuit event on Track 13 through the normal frontend, publish its race scene and return through donor UI.
- Acceptance: verify authored event selection, car selection, pre-race UI, driving, pause and return. Exercise the natural results path where practical. Preserve the first Championship event and the Track 12 diagnostic.

Debug Forward+ entered Track 13 from the authored event tree, published
8,282 placements and 13 vehicles, and returned to the event tree from
Pause in `local/task0129-pursuit-fixed.log`. The input-only route probe
negotiated multiple bends and reached 5,592.8 m of the 12,295 m capture
before the run ended without a finish record (`local/task0129-route.log`).
Natural finish and results-to-event-tree remain open.

To continue, run `tools/test_frontend_mvp.gd` against the staged Debug
project with `HP2_MVP_PURSUIT=1`, `HP2_MVP_DEBUG_CONSOLE=1` and
`HP2_ROUTE_CAPTURE` pointing to the local Track 13 capture documented in
TASK-0100. Use an isolated profile and screenshot directory. Require the
test's results-return marker before recording full-route success.

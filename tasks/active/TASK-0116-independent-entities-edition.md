# TASK-0116 — Independent Godot Entities Bridge edition

- Status: active
- Scope: Separate the two runnable products and migrate the currently available player and debug presentation paths.
- Dependencies: TASK-0115 Track 12 pilot, TASK-0104 frontend Controls, TASK-0110 authored race flow.
- Original behavior authority: existing Retail and Debug records remain authoritative in their respective scopes.

## Acceptance

- [ ] Independent projects, Debug and Release DLLs, application data, saves, and distribution.
- [ ] Current frontend screens and selection preview use Godot-owned visuals and native state.
- [ ] Current race entry points, authored UI, and race visuals use the entity and UI bridges.
- [ ] F8, profiling, and both asset viewers use the new edition's presentation.
- [ ] Independent startup, input, restart, settlement, save, return, and visual comparisons.

## Initial work

Keep the original project launch as the default. Add the independent project and build route, then migrate native state consumers in the order recorded in the implementation plan.

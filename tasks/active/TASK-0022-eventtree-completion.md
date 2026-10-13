# TASK-0022 - Event tree completion

- Status: active
- Objective: close the remaining open questions in docs/EVENT_TREE.md section 7 so screen 39
  behaves and looks as the original does, including its animations and its sibling documents.
- Scope: included - FUN_001F6B70 (the event detail sub-menu), the tLineSegment build and the
  connector colour rule, the tStillCameraMover easing, the state 5/6/7 transition animators, and
  the three sibling documents 17/18/19. Excluded - anything that needs save data the profile
  does not carry, and starting a race.
- Original PS2 addresses: sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2 -
  0x001F6B70, 0x001F6930, 0x001F6678, 0x001F6328, 0x001F9480, 0x001F9598, 0x001F65C8,
  0x001F8CF0, 0x001F91D0, 0x001F8820, 0x001F8E18, 0x001F91F0, 0x002494E8, 0x001FAC78,
  0x001FABF0, 0x001FAD20, 0x001FAE40, 0x001F96B8, 0x0015E6A0, 0x001DAAA0
- Dependencies: TASK-0021 (the profile; landed). GhidraMCP read-only at 127.0.0.1:8080.
- Relevant files: src/game_core/indep/src/eventtree.{hpp,cpp},
  src/port/runtime/event_tree_view.{hpp,cpp}, src/port/runtime/frontend_runtime.{hpp,cpp},
  src/tests/eventtree_tests.cpp, src/tests/frontend_runtime_tests.cpp, docs/EVENT_TREE.md
- GitHub issue: not created

## Acceptance criteria

- [x] The event detail panel shows the original's own Status and Reward text, from the
      recovered FUN_001F6B70 over document 17.
- [x] The connector colour rule is read out of FUN_001F96B8's tLineSegment build, or the port's
      rule is explicitly recorded as port-side in both the code and the doc.
- [x] The zoom easing is recovered from FUN_0015E6A0 / cameramover.cpp, or the snap is recorded
      as port-side with what was ruled out.
- [x] kPortPanelFit is gone, replaced by whatever transform makes the authored 256 px panel fit
      the recovered 130/350 lattice at zoom 0.4.
- [ ] States 5, 6 and 7 are translated, including FUN_001F9F10's intro path.
- [x] Reused, not reimplemented: the zero-velocity cubic Hermite in
      Type_002490D8::Update_00249128 and the scrolling-text Type_002491B0 that screen 39 already
      binds through EVENT_TITLE_BAR and WHITEOUT_BAR.
- [x] No new descriptive identifier for recovered code; no new silent default.

## Verification procedure

```
cmake --build --preset native-debug --parallel 4 && ctest --preset native-debug
cmake --build --preset godot-debug --parallel 4 && ctest --preset godot-debug
./tools/run_frontend.ps1 -RenderSmoke
```
Plus a screen 39 capture compared against the user's original-hardware reference screenshot.
Original comparison scope: none here; that is TASK-0023.

## Current findings

### Continuation on 2026-10-13

Scope: resume Claude's uncommitted work, recover the Status/Reward producers, document
names and detail input, and replace unsupported event-tree geometry/transition rules.
Acceptance checks: native and Godot builds/CTest, original-asset runtime assertions,
render smoke, and private visual captures. Original equivalence is not established by
these checks. Existing user modifications are preserved; no commit is requested.

Read-only Ghidra MCP confirms that 0x001F6B70 handles input, while 0x001F6770 draws
document 17 and 0x001F8890 populates the browsing overlay. 0x001F60E8 selects localized
Status IDs 0xF230..0xF234; 0x001F6220 resolves the first reward's car/track name.
0x001FAE40 enters reward preview, rather than starting a race. Detailed evidence and
remaining dependencies are being recorded under E-0022-*.

Carried from TASK-0021, which identified the targets:

- The event tree is a four-document cluster: screen 39 plus screens 17 (detail, 0xA744BA7B),
  18 (zoom transition, 0xFD297FD4) and 19 (state 5, 0xF1EED4CD). (E-0021-17)
- FUN_001F96B8 binds the scrolling-text animator the port already implements:
  `FUN_002491B0(view+0x408, screen, EVENT_TITLE_BAR, EVENT_TITLE_BAR, WHITEOUT_BAR)`. That is
  `Type_002491B0`, which exists in src/game_core/indep/src/fmenus.cpp, so the title bar should
  reuse it rather than gaining a second implementation.
- FUN_001FA9C8 drives four per-node updaters on every move: FUN_001F8820 (view+0x1C, a marquee
  that latches uGpffffB368, measures with FUN_00140518 and binds class 0xA2A16EA1),
  FUN_002494E8 (view+0x408, the localised description from record+0xB0), FUN_001F8E18
  (view+0x5C0) and FUN_001F91F0 (view+0x600). None is translated.
- FUN_001F96B8 writes a 32-float array at DAT_0031B240 indexed by event number minus one; its
  consumer was not traced.

## Unresolved questions

The six in docs/EVENT_TREE.md section 7 as updated by TASK-0021, plus its new questions 7-9.

## Next steps

1. Decompile FUN_001F6B70 and FUN_001F6930 (document 17, the detail panel).
2. Decompile the tLineSegment build inside FUN_001F96B8 for the connector rule.
3. Decompile FUN_0015E6A0 and the tStillCameraMover update for the easing and the missing
   transform behind kPortPanelFit.
4. Translate FUN_001F8820 / FUN_002494E8 / FUN_001F8E18 / FUN_001F91F0 onto the existing
   animation machinery.

## Related Git commits

None yet. Record actual commit hashes only after creation.

## Completion evidence

Not complete: full car/track 3D reward preview, race/car-selection effects, event31
completion globals, and original-execution comparison are still missing.

The Status/Reward producers are browsing-overlay functions001F8890/001F60E8/001F6220;
001F6B70 is the input handler, and document17 is drawn by001F6770. All are now documented.
The old easing target was misidentified:0015E6A0 constructs the preview camera; the tree
uses the same native Hermite interpolator as the screen-exit animation. The renderer
does not apply node+0x0C's selected multiplier; kPortPanelFit is removed as unsupported.

Implemented state1/3/4/6 transitions and state5 return, doc17 text and doc18 introduction,
car-logo cycling and track-logo transitions, original selection border pulse, authored
line widgets/colors and clipping, live title and recovered frontend document names.
State5's missing model renderer reports0x001F6440; unlocked selection reports0x001F6A28.
Unknown special nodes carry explicit diagnostics rather than a fabricated Locked state.

Validation on2026-10-13: `cmake --build --preset native-debug --parallel 4` passed;
`cmake --build --preset godot-debug --parallel 4` passed;
`ctest --preset godot-debug --output-on-failure` passed24/24. Native full rerun is
recorded in PROGRESS.md. Targeted text/catalog/animation tests pass with original assets
configured. Private DX12 browse/detail captures inspected; no original comparison.
Earlier test failures (old recovered-name count18 and a fixture assuming the saved
cursor reset on reopen) were corrected; the latter demonstrated the recovered profile
write was now taking effect. An initial DLL-link failure was due to the previous
running game holding the DLL; closing that game allowed the rebuild.

Evidence: E-0022-D01..D10, E-0022-C01/C02, E-0022-T01..T12, E-0022-G01..G07,
E-0022-R01..R05. See docs/TASK_0022_DETAIL_TEXT.md and docs/TASK_0022_INTEGRATION.md.
No commit was made. Continue TASK-0022 at001F6440/001F6A28 and the event31 gate;
TASK-0023 is required before any Verified promotion.

# TASK-0025 — Language Options screen: construction and item activation

- Status: active
- Objective: translate the Language Options screen handler's construction and item-activation
  paths so activating a language item no longer raises
  `hp2::NotImplemented: screen item activation handler not translated (ELF 0x1f1dd8)`.
- Scope: included — the class router `001F1DD8`, the two constructors `001F1A78` / `001F1D70`,
  the message handler `001F1B08` message `0x01` (activation) and message `0x0B`
  (construction) slices, the generic item enable/visibility helper `0016D2A8`, the locale
  table behind `0021E0D0`, and the runtime routing that reaches them.
  Excluded — the `0x11` (Back) slice, which TASK-0017 already recovered; the full
  profile/game reset `00183830`; the `001EE918` sub-object the normal constructor builds at
  instance+0x10; the destructor message `0x0C`; the unresolved consumer of `DAT_00331A60`.
- Original PS2 addresses: SLUS_203.62,
  sha256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`,
  revision unknown. `0x001F1DD8`, `0x001F1A78`, `0x001F1B08`, `0x001F1D70`, `0x0016D2A8`,
  `0x0016A5B0`, `0x0016FDB0`, `0x0016D3F0`, `0x00170320`, `0x0021E0D0`, `0x00183830`,
  `0x001740C0`. Data: `0x0031C168` (locale filename pointer table), `0x0031C1C8`
  (`DAT_0031C1C8`, current language), `0x00331A60` (`DAT_00331A60`), `0x002B9B98` /
  `0x002B9EB8` (class-name literals), `0x002BA520` / `0x002BA508` (vtables).
- Dependencies: TASK-0017 (Back slice and `query_back` routing), TASK-0013/0014 (screens,
  menu list, widget visibility), read-only Ghidra MCP over `tools/ghidra.py`.
- Relevant files: `src/game_core/indep/src/frontendscreens/options.{hpp,cpp}`,
  `src/game_core/indep/src/fmenus.{hpp,cpp}`, `src/port/runtime/frontend_runtime.{hpp,cpp}`,
  `src/tests/frontend_actions_tests.cpp`, `docs/FRONTEND_MENU_LANGUAGE.md`.
- GitHub issue: not created

## Acceptance criteria

- [x] `001F1B08` message `0x01` is translated from the ELF listing, including the exact
      `MENUITEM_1..MENUITEM_6` → language index mapping, the unsigned range guards, the
      `MENUITEM_7` arm, and the order `0021E0D0` → `-1` check → `00183830` → `00170320(1)`.
- [x] The `0x0B` construction slice reproduces instance field initialisation and the
      unconditional `0016D2A8(class, MENUITEM_7, screen, 2)` call, with the evidence that
      `0016FDB0` returns 0 in this build.
- [x] The locale index order is read out of the ELF pointer table rather than assumed.
- [x] Activating a language item in the host front end performs the locale change instead of
      raising `NotImplemented` at `0x1F1DD8`.
- [x] Untranslated dependencies on this path still fail loudly (`00183830`, the unresolved
      `DAT_00331A60` consumer is documented rather than guessed).
- [x] Tests cover the activation mapping, both class variants, the hidden `MENUITEM_7`, and
      the loud failures.

## Verification procedure

```
cmake --build --preset native-debug --target frontend_actions_tests --parallel 4
ctest --preset native-debug -R '^frontend_actions$' --output-on-failure
cmake --build --preset native-debug --target frontend_back_tests fmenus_menu_tests --parallel 4
ctest --preset native-debug -R '^(frontend_back|fmenus_menu)$' --output-on-failure
```

Host tests only. No original hardware or PCSX2 comparison was performed, so nothing on this
path may be marked `Verified` (AGENT.md R1).

## Current findings

Confirmed (read-only Ghidra MCP, 2026-10-13) — see `docs/FRONTEND_MENU_LANGUAGE.md` for the
full write-up and E-0025-L01..L06.

- `001F1DD8` is the class router, not a per-item handler: message `0x0B` picks the constructor
  by `strcmp` (`002535F8`) against the class-name literals, `0x0C` destroys the instance, and
  every other message is forwarded to `001F1B08`.
- `MENUITEM_1`..`MENUITEM_6` (`0x52E039F3`..`0x52E039F8`) select language 0..5;
  `MENUITEM_7` (`0x52E039F9`) sets `DAT_00331A60 = 1` and pops.
- `0016FDB0` returns 0 unconditionally, so the constructor always disables and hides
  `MENUITEM_7`; the `MENUITEM_7` activation arm is therefore unreachable from input in this
  build.
- Locale order is `ENGLISH, FRENCH, GERMAN, ITALIAN, SPANISH, SWEDISH` from the pointer table
  at `0x0031C168`; entries 6 and 7 (`FE_LONGEST.BIN`, `FE_ID.BIN`) are not offered by the menu.
- Instance+0x08 is the constructor's third argument: 0 for `Language Options Pre FE`
  (`001F1A78` directly) and 1 for `Language Options` (through `001F1D70`). Only the 0 case
  reaches the `00183830` full profile reset.

## Unresolved questions

- No reader of `DAT_00331A60` was located. Search bound: Ghidra `references_to` on
  `0x00331A60`, plus a whole-image byte search for every `lw`/`sw $rt, -0x3e90($gp)` encoding,
  which found only the two writers `001F1B08` and `001740C0`. A reader using a different
  addressing form or width would not have been found.
- `0016FDB0`'s original name and the condition it was meant to express are unknown; only its
  constant return is recovered.
- The `001EE918` sub-object built at instance+0x10 by `001F1D70` (hashes `0x03C790BB`,
  `0x718CD8C3`, `0x95AD096B`) is not translated; `0x95AD096B` is the `Options` document hash,
  so this is most likely the scrolling-hint/back-target binding, but that is a hypothesis.
- `00183830` (full profile and game reset) remains an explicit `NotImplemented` boundary, so
  the `Language Options Pre FE` activation path cannot complete.
- The Back overload `Sub_001F1B08(BackContext&, ...)` writes `*c.language` after its own
  `0021E0D0` invoke. That field is a context copy rather than the runtime's global, so it cannot
  mask the `-1` arm the way the activation bug did, but it is the same pattern. Revisit when
  `0021E0D0` is translated rather than stubbed; not changed under this task because it is
  TASK-0017's tested code.

## Next steps

1. Translate `00183830` (depends on the TASK-0021 profile work) to unblock the
   `Language Options Pre FE` activation path.
2. Locate a reader of `DAT_00331A60`, or record it as dead in this build with a wider search.
3. Translate the `001EE918` binding at instance+0x10 and the `0x0C` destructor slice.
4. Compare the locale switch against the original under TASK-0023 before any `Verified` claim.

## Related Git commits

None yet. Record actual commit hashes only after creation.

## Completion evidence

2026-10-13. CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0 (Strawberry Perl toolchain), Windows 11.

- `cmake --build --preset native-debug --parallel 4` — succeeded.
- `ctest --preset native-debug --output-on-failure` — **24/24 passed**, including
  `frontend_actions`, `fmenus_menu`, `frontend_back` and `frontend_runtime`.
- `cmake --build --preset godot-debug --parallel 4` — succeeded; relinked
  `godot/bin/ps2_port.dll`.

Resolved during the work: the Language screen does author a `MENUITEM_7` widget (seven items
total), observed through the port's own loader against the player's configured asset tree, so
the `0016D2A8` hide is real behaviour and the runtime assertions on it are not vacuous.

Remaining limits. Host tests only; no original hardware or PCSX2 comparison, so no `Verified`
status was promoted anywhere (AGENT.md R1, R11). `Language Options Pre FE` activation still
terminates in the `00183830` `NotImplemented`, which is correct behaviour for an untranslated
path but means that one of the two class variants cannot complete. The `0x0C` and `0x12` arms,
the `001EE918` sub-object and the consumer of `DAT_00331A60` are unresolved and recorded above.

The task stays **active** on that basis: the activation acceptance criteria are met with
evidence, but the scope's open dependencies are tracked here rather than being closed.

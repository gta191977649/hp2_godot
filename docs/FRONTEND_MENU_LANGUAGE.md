# TASK-0025 — The Language Options screen

2026-10-13, read-only Ghidra MCP (`tools/ghidra.py`, `http://127.0.0.1:8080/mcp`).
US PS2 `SLUS_203.62`, revision unknown, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
No Ghidra edits (AGENT.md R5). No original hardware or PCSX2 comparison was performed, so
nothing here is `Verified`.

TASK-0017 recovered this screen's Back (`0x11`) arm; see `docs/FRONTEND_BACK.md`. This document
covers the rest of it: the class router, the two constructors, and the item-activation (`0x01`)
arm that the port was raising `hp2::NotImplemented` on.

## `001F1DD8` is the class router, not the handler

`FUN_001F1DD8(message, payload)` is what `0016ADB0` registers for both Language classes. It
owns the instance lifetime and forwards everything else:

| message | behaviour |
|---|---|
| `0x0B` | `strcmp` (`002535F8`) the current class name against `"Language Options Pre FE"` (`0x002B9EB8`); on a match `operator new(0x10)` (`00257690`) then `FUN_001F1A78(mem, payload[0], 0)`. Otherwise `strcmp` against `"Language Options"` (`0x002B9B98`); on a match `operator new(0x58)` then `FUN_001F1D70(mem, payload[0], 1)`. Neither match leaves the global instance pointer untouched. |
| `0x0C` | virtual destructor through instance+0x0C with flag 3, then clears the instance pointer. Not translated. |
| any other | `FUN_001F1B08(instance, message, payload)` |

The instance pointer is the gp-relative global at `gp-0x424C`. The current class name reaches
the router through the manager at `gp-0x534C` (`DAT_003305A4`), `+0x19C -> +0x10 -> +0x08`.

Both classes register the **same** document hash `0x632B068E` and the same handler
(`registry_0016adb0.cpp` rows 43/44), so the screen identity that decides behaviour is the
class name, not the layout — the reason the port's navigation history stores class names.

## Instance layout and the one flag that matters

`FUN_001F1A78(this, screen, flag)`:

```
+0x00 = DAT_0031C1C8          ; current language, clamped to 0 when it is still -1
+0x04 = screen                ; message-0x0B payload word 0
+0x08 = flag                   ; 0 for "... Pre FE", 1 for "Language Options"
+0x0C = &DAT_002BA520          ; vtable
```

`FUN_001F1D70` is the normal-class constructor: it forwards `a0/a1/a2` unchanged to
`001F1A78`, overwrites the vtable with `&DAT_002BA508`, and then builds a `001EE918`
sub-object at `+0x10` from the hashes `0x03C790BB`, `0x718CD8C3` and `0x95AD096B`. That last
hash is the `Options` document hash, which makes a hint/back-target binding the natural reading
— but that is a **hypothesis**, and the sub-object is not translated.

`+0x08` is the whole difference between the two classes: only `0` reaches the
`FUN_00183830(0x002FC050)` full profile and game reset, on both the Back and the activation
arm. The `Pre FE` class is the one `0016F950` pushes during boot (see `screen_manager.hpp`),
i.e. the first-run language prompt, which is why it is the one that resets the profile.

## Construction always hides `MENUITEM_7`

The constructor's tail is:

```
if (FUN_0016fdb0() == 0)
    FUN_0016d2a8(current_class_name, 0x52E039F9, instance+0x04, 2);
```

**`FUN_0016FDB0` is `return 0;` — nothing else.** The branch is therefore unconditional in this
build. What the original condition was meant to express is not recovered; only the constant
return is.

`FUN_0016D2A8(class, name_hash, screen, mode)` resolves the screen group from the class name
(`0016C8C8`, `0016B340`), finds the item by name hash (`0016A5B0`, comparing item `+0x0C`), and
then, for `mode == 2`, clears the enabled bit (`00169CB0(item, 1, 0)`) **and** hides the widget
(`00144968(screen, hash, 0)`). Both lookups return 0 silently on a miss and the function
returns.

So `MENUITEM_7` is always disabled and hidden on entry. The player's own `FRONTB.BUN`
confirms this is observable rather than defensive: the Language screen authors **seven**
`MENUITEM_n` text widgets, and the port's recovered menu list builds all seven before this call
removes the last one (`frontend_runtime_tests`, `check_submenus`).

## Activation — `001F1B08` message `0x01`

The item hashes are positional, not language names: `StringHash_002534A8("MENUITEM_1")` is
`0x52E039F3`, and they run contiguously to `MENUITEM_7` at `0x52E039F9`. The same six values
carry entirely different meanings on the Options and Mode Select screens.

```
hash = event+0x00
if (hash - MENUITEM_1 <u 6)      instance+0x00 = hash - MENUITEM_1   ; jump table at 002B9EA0
if (hash <u MENUITEM_1)          return
if (MENUITEM_6 <u hash) {
    if (hash == MENUITEM_7) { DAT_00331A60 = 1; goto FUN_00170320(1); }   ; tail call
    return
}
thunk_FUN_0021E0D0(&DAT_0031C1C8, instance+0x00)
if (DAT_0031C1C8 == -1)          return
if (instance+0x08 == 0)          FUN_00183830(0x002FC050)
goto FUN_00170320(1)                                                      ; tail call
```

Three details are easy to get wrong and are load-bearing:

- The index store happens **before** the range guards, so a hash outside `MENUITEM_1..7` leaves
  `+0x00` alone and returns without doing anything.
- `0021E0D0` takes the **address** of `DAT_0031C1C8` and writes the global itself. The `-1`
  test afterwards is a re-read of what the loader left behind. A handler-side copy of
  `instance+0x00` into the global would make that arm unreachable — the port had exactly that
  bug until `frontend_actions_tests` caught it.
- Both exits are tail calls into `FUN_00170320(1)`, which is
  `FUN_0016C670(DAT_003305A4, 1, 0, -1)` with the callback disabled. The handler pops its own
  history; it does not report a permission to the manager. This matches the Back arm, which
  writes permission 0 and pops directly (`docs/FRONTEND_BACK.md`).

Compared with the Back arm, activation differs in three ways: it reads the **activated**
item's hash instead of the currently-selected one (`0016D3F0`), it has no leading
`DAT_0031C1C8 == -1` early return, and it writes no permission/sound word.

## The locale table

`FUN_0021E0D0(&language_state, requested)` returns immediately when `requested` equals the
stored value. Otherwise it unloads the previous resource (`00194A00`, `001992A8`), stores the
new index, and loads `(&PTR_s_LANGUAGES_FE_ENGLISH_BIN_0031C168)[requested]`. The pointer table
at `0x0031C168`, read verbatim:

| index | file |
|---|---|
| 0 | `LANGUAGES\FE_ENGLISH.BIN` |
| 1 | `LANGUAGES\FE_FRENCH.BIN` |
| 2 | `LANGUAGES\FE_GERMAN.BIN` |
| 3 | `LANGUAGES\FE_ITALIAN.BIN` |
| 4 | `LANGUAGES\FE_SPANISH.BIN` |
| 5 | `LANGUAGES\FE_SWEDISH.BIN` |
| 6 | `LANGUAGES\FE_LONGEST.BIN` |
| 7 | `LANGUAGES\FE_ID.BIN` |

Only 0..5 are reachable from the menu, which is exactly the six items `MENUITEM_1..MENUITEM_6`
select. Entries 6 and 7 are development locales (a longest-string layout probe and an id dump)
that no menu item selects. This is the evidence behind the order the port's
`FrontEndRuntime::apply_language` already used; it was previously asserted without a citation.

The loader also mirrors the index into `DAT_002FC1C4`, fires `0021E280`/`0021E250` when the
state's `+0x14` is set and the value actually changed, and then calls `001681B0(new, old)` and
`00141648()` — the widget invalidation that makes already-loaded screens re-resolve their
string ids.

## What the port does

- `hp2::frontend::Type_001F1A78`, `Construct_001F1A78`, `Sub_001F1B08(message, hash, ...)` in
  `src/game_core/indep/src/frontendscreens/options.cpp`, beside the existing `0x11` overload.
- `MenuList_00169F78::FindByHash_0016A5B0` and `Sub_0016D2A8` in
  `src/game_core/indep/src/fmenus.cpp`. `0016D2A8`'s original translation unit is **not**
  pinned (`docs/ORIGINAL_SOURCE_TREE.md`); it lives in `fmenus.cpp` because it only operates on
  that file's menu framework, and the port is handed the list and screen the caller already
  owns instead of re-resolving them from the class name.
- `port::frontend::ScreenActionServices` gained an `invoke(address, argument)` seam, the same
  shape `BackServices` already used, so an activation slice that calls out of the front end
  names the ELF address it is standing in for.
- `FrontEndRuntime` holds the instance and `DAT_00331A60`, constructs the instance after
  `EnsureMenu_0016A858()` (the menu list has to exist before `0016D2A8` can find an item in
  it), and routes activation on the two Language class names.

Untranslated paths still fail loudly: `00183830` throws, so `Language Options Pre FE`
activation cannot complete, and a missing `invoke` service throws rather than dropping a
locale change.

## Unresolved

- **No reader of `DAT_00331A60` was found.** Search bound: Ghidra `references_to` on
  `0x00331A60`, plus a whole-image byte search for every `lw`/`sw $rt, -0x3E90($gp)` encoding,
  which returned only the two writers `001F1B08` (this screen) and `001740C0` (another screen's
  message-1 arm, on hash `0xAEE77883`). A reader using a different addressing form or access
  width would not have been found, so "dead in this build" is a hypothesis, not a finding.
  Because the item that writes it is always hidden, the write is recovered but never exercised.
- `0016FDB0`'s original name and intent.
- The `001EE918` binding at instance `+0x10`, and the `0x0C` destructor arm.
- `00183830`, the full profile and game reset, is a TASK-0021 dependency.
- The Back overload `Sub_001F1B08(BackContext&, ...)` writes `*c.language` after its own
  `0021E0D0` invoke. That context field is a copy rather than the runtime's global, so it
  cannot mask the `-1` arm the way the activation bug did, but it is the same pattern and
  should be revisited when `0021E0D0` itself is translated rather than stubbed.

## Evidence

- `E-0025-L01`: `001F1DD8` decompile and `001F1B08`/`001F1A78`/`001F1D70` decompiles;
  class-name literals at `0x002B9B98` / `0x002B9EB8`; vtables `0x002BA520` / `0x002BA508`.
- `E-0025-L02`: `0016FDB0` decompile (`return 0`); `0016D2A8` decompile; the seven authored
  `MENUITEM_n` widgets observed in the player's own `FRONTB.BUN` through the port's loader.
- `E-0025-L03`: `0021E0D0` decompile; pointer table listing at `0x0031C168`; the eight
  filename strings at `0x002BC850`..`0x002BC920`.
- `E-0025-L04`: `0016D2A8` decompile and the `00169CB0` / `00169BD0` / `00144968` callees.
- `E-0025-L05`: `0016A5B0` and `0016B340` decompiles (circular-list scans on `+0x0C` / `+0x14`).
- `E-0025-L06`: `001F1B08` MIPS listing `0x001F1BF4`..`0x001F1D6C`, including
  `sw v0,-0x3E90(gp)` at `0x001F1D3C`; `001740C0` decompile; the byte search described above.
- `E-0025-L07`: `MENUITEM_1`..`MENUITEM_7` hashes reproduced exactly by
  `StringHash_002534A8` (`FUN_002534A8`, `h = -1`, `h = h * 0x21 + byte`).

## Tests

```
cmake --build --preset native-debug --parallel 4
ctest --preset native-debug --output-on-failure
```

24/24 passed on 2026-10-13. CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0 (Strawberry), host only.
`frontend_actions` covers the activation mapping, both class flags, `MENUITEM_7`, the
out-of-range and non-activation returns, the `-1` arm, and the missing-service failure.
`fmenus_menu` covers `0016A5B0` and all four `0016D2A8` modes including the misses.
`frontend_runtime` drives the real screen from the player's own assets: entering
Language Options, asserting the seven authored items with `MENUITEM_7` disabled and hidden,
activating a language, and checking the locale applied and the screen popped.

Host tests do not establish original equivalence (AGENT.md R1). Original comparison is
TASK-0023.

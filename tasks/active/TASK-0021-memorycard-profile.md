# TASK-0021 — `src\memorycard.cpp` and the `settings.db` profile

- Status: active
- Objective: recover the original save container and the parts of the 0x114E0 player profile
  that code provably reads, so the event tree can report real event status instead of a
  profile-less reduction.
- Scope: included — the save directory/file naming, the `settings.db` byte-stream layout
  (leading size word and trailing checksum), the load and store contracts, and every profile
  offset backed by a reading instruction. Excluded — PS2 memory-card hardware/driver
  sequencing (`sceMc*`), the load/save front-end screen's pixel accuracy, and any profile
  offset no code was observed to read.
- Original PS2 addresses: sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2 —
  0x0012DEE0, 0x0012CDE8, 0x0012CF00, 0x0012D118, 0x0012D858, 0x0012C938, 0x0012E270,
  0x0012D2A8, 0x001A3390, 0x001878B0, 0x00245700, 0x00245738, 0x00245780, 0x00245B00,
  0x00245BA0, 0x00245CA8, 0x00245D30, 0x00246C00, 0x001F5D00, 0x001F5D28, 0x001F5D50,
  0x001FDFE8, 0x00200018, 0x00200068, 0x002003F8, 0x002007D0, 0x002008B8, 0x001FFFB0,
  0x002010B8
- Dependencies: GhidraMCP read-only at 127.0.0.1:8080 (AGENT.md R5); the authorized ELF at
  D:/ps2_game/SLUS_203.62. Blocks TASK-0022 (event tree completion).
- Relevant files: src/game_core/memorycard/{include/memorycard.h,src/memorycard.cpp},
  src/game_core/indep/src/eventtree.{hpp,cpp}, src/tests/memorycard_tests.cpp,
  docs/SAVE_SYSTEM.md, reverse/functions/sha256-7335.../memorycard/
- GitHub issue: not created

## Acceptance criteria

- [x] The save directory and file names come from recovered ELF strings only, with no invented
      descriptive identifier anywhere in the module.
- [x] The `settings.db` stream layout is stated with the instruction that proves each part
      (docs/SAVE_SYSTEM.md section 3).
- [x] Every modelled profile offset cites the function that reads it; unproven bytes are
      explicitly unidentified and are not modelled (the structure record lists 0x10E30 bytes as
      unidentified, and D009 makes the rule binding).
- [x] The two **recovered** GoalType slice decoders reproduce the decompiled switches exactly,
      including both "not attempted" sentinels and the unsigned/signed split. The other two are
      not recovered and are not implemented.
- [x] Load rejects a wrong length and a wrong checksum; store stamps both fields; store then
      load then store is byte-identical (`src/tests/memorycard_tests.cpp`).
- [x] GoalType 3 and 4 decode, and the event 0x1F / 0x20 gates, raise `hp2::NotImplemented`
      naming their ELF address rather than returning a default.
- [x] No save file, memory-card image or profile dump is added to Git. `.gitignore` additionally
      refuses `settings.db`, `*.ps2`, `*.mcd` and `saves/` in case a port save root is pointed
      back inside the tree.
- [x] Selecting Championship or Ultimate Racer reaches the event tree, with the tree the player
      chose. Asserted end to end by `check_event_tree_route` in
      `src/tests/frontend_runtime_tests.cpp` and confirmed in the running app.

## Verification procedure

```
cmake --build --preset native-debug --parallel 4
ctest --preset native-debug
cmake --build --preset godot-debug --parallel 4
ctest --preset godot-debug
py -3 reverse/scripts/validate_main_loop.py --metadata-only
```

Expected: the pre-existing suites unchanged plus `memorycard` passing; `MAIN_LOOP_CATALOG_OK`
unchanged. `memorycard_tests` prints `MEMORYCARD_TESTS_OK`.
Original comparison scope: none in this task. Nothing is promoted to Verified (AGENT.md R1);
the PCSX2 comparison is TASK-0023.

## Current findings

Confirmed, read-only GhidraMCP, Ghidra 12.0.4, 2026-10-13 (Australia/Sydney), no Ghidra state
modified. See docs/SAVE_SYSTEM.md for the full write-up.

- E-0021-01 The original source file is `src\memorycard.cpp` (ELF string 0x002AD488). The
  module occupies approximately 0x0012A000..0x0012E6FF, immediately below `HP2_MainFrameLoop`
  (0x0012F378).
- E-0021-02 `FUN_0012DEE0` builds the save directory name with `"%s%s%s"` (0x0032FDC8) from a
  region prefix — `"BA"` (0x0032FDC0) or `"BE"` (0x0032FDB0) — the title id `"SLUS-20362"`
  (0x002AD6D8) or `"SLES-50731"` (0x002AD6C8), and the suffix `"HP2"` (0x0032FDB8). The region
  comes from `FUN_001A3390`, which returns `uGpffffB364`: zero selects the PAL triple.
  Resulting names: `BASLUS-20362HP2` and `BESLES-50731HP2`.
- E-0021-03 Paths are joined with `"/%s/%s"` (0x0032FD98) and enumerated with `"/%s/*"`
  (0x0032FDA0). The icon descriptor is `icon.sys` (0x002AD5E8), written by `FUN_0012D858` as
  0x3C4 bytes taken from the manager's `+0x184`; the icon itself is `nfs6.ico` (0x002AD588),
  whose 0x10-byte header `FUN_0012DEE0` reads through `FUN_00254908`. `"PS2D"` (0x0032FDA8) is
  the icon.sys magic.
- E-0021-04 There is exactly one save payload file, `settings.db` (0x002BEBE0), of 0x114E0 =
  70,880 bytes. Both `FUN_00245B00` (store) and `FUN_00245BA0` (load) pass that name and that
  length to the manager through vtable+0x2C (`FUN_0012E270`, which stores the name at
  manager+0x14, the buffer at +0x114 and the length at +0x118).
- E-0021-05 The stream is self-describing and carries no separate header.
  `FUN_00245738` writes `0x000114E0` into the first word and the checksum into the last:
  `DAT_002FC050 = 0x114E0; DAT_0030D52C = FUN_001878B0(0x2FC050);`. `0x0030D52C - 0x002FC050`
  is 0x114DC, so the checksum is the final dword of the payload.
- E-0021-06 `FUN_001878B0` sums signed 32-bit words from the buffer base. Its counter starts at
  0x4536 and the loop exits after the decrement reaches -1, so it covers 0x4537 = 17,719 words
  = 0x114DC bytes — the whole payload except the checksum word itself. `FUN_00245780` verifies
  it against `*(int *)(buffer + 0x114DC)`.
- E-0021-07 The global player profile is a single 0x114E0-byte block at **0x002FC050**.
  `FUN_00245D30` copies the loaded payload onto it verbatim
  (`HP2_CopyBytesForward(0x2FC050, **(screen+0x10), 0x114E0)`) and `FUN_00245738` hands the
  same address straight back as the store source, so no staging copy exists in either
  direction. Three independent arithmetic identities confirm the base: profile+0x1072C is
  `DAT_0030C77C` (the mode-select tree selector), profile+0x188/+0x288 are `DAT_002FC1D8` /
  `DAT_002FC2D8` (the two event blocks) and profile+0x114DC is `DAT_0030D52C` (the checksum).
- E-0021-08 Post-load fix-ups in `FUN_00245D30`, in order: clear `DAT_002FC3D8`;
  `FUN_001FE988(uGpffffBEC4, 0x2FC1D8, 0x100, 4)` and `FUN_001FE988(uGpffffBEC8, 0x2FC2D8,
  0x100, 4)` — the two event-tree slice passes the port already reproduces;
  `FUN_001EF120(0x2FC0E4)`; `DAT_002FC1C4 = DAT_0031C1C8`; two `FUN_00154308` /
  `FUN_00155000` calls over `DAT_002FC1C8`; `FUN_001F4B10(0x2FC194)`.
- E-0021-09 Event-tree profile accessors, read out of the listing rather than the decompiler
  because the R5900 argument order was lost. `FUN_001F5D00(block, type)` returns `block+8` for
  type 0, `block+4` for type 1 and **1** otherwise. `FUN_001F5D28(block, type, number)` selects
  on `block[0]`, not on its `type` argument — an original quirk that is preserved — writing
  `block+8` when `block[0] == 0` and `block+4` when `block[0] == 1`. `FUN_001F5D50(block, type)`
  is `sw a1, 0x0(a0)`: it sets `block[0]`. With `block` = 0x002FC1B4 = profile+0x164 this gives
  profile+0x164 = current tree type, profile+0x16C = last selected event number for type 0
  (Championship) and profile+0x168 = for type 1 (Hot Pursuit). This is the start node
  `EventTreeView::load` currently hardcodes to event 1; **1 is the recovered default**, so the
  existing behaviour was right for an empty profile and wrong only in its provenance.
- E-0021-10 `FUN_002010B8` decodes an event's status from its slice. Values: 0 locked,
  1 unlocked/not attempted, 2 attempted but below the goal's own floor, 3 attempted and the
  goal missed, 4 won. `iGpffffBEB0 != 0` forces 4 for every event. Event 0x1F is additionally
  gated on `DAT_0031D7F4+0x180` (tree 0) and `+0x1B8` (tree 1); event 0x20 requires events
  1..31 of both trees to be 4.
- E-0021-11 The per-GoalType slice decoders, with `row` being the event's requirement row and
  `row+4` its u32 goal threshold: `FUN_00200018` (types 0 and 2, one byte) returns 1 when the
  byte is 0, else 4 when `byte <= row+4`, else 3. `FUN_00200068` (types 1, 5, 6, s32) returns 1
  for 0 or 0x7FFFFFFF, else 4 when `value <= row+4`, else 3. `FUN_002003F8` (type 3) and
  `FUN_002007D0` (type 4) need `FUN_002001A8`, `FUN_002006A0` and `FUN_00200768`, which are not
  recovered.
- E-0021-12 The `0x34A03` table at tree+0x24 is a requirement table, not a plain link table.
  `FUN_002008B8(leaf, i)` returns `tree+0x24 + record[0x7C + 2i] * 8` for `i < record+0x3D`,
  and `FUN_001FFFB0(row, tree)` asks whether the event numbered at `row+0x02` reaches status 4
  when judged against **this row's** `row+0x04` threshold. An event is unlocked when any one of
  its rows is satisfied. That identifies four of the six bytes `docs/EVENT_TREE.md` listed as
  unknown; `row+0x00..0x01` remains unidentified.

- E-0021-22 The class `View Event Tree` registers through `FUN_0016AD88`, not `FUN_0016ADB0`:
  a tail call at 0x001FB860 whose operands are built at 0x001FB7F0..0x001FB830 -- class-name
  string 0x002BA718, handler 0x001FB218, document hash **0x147F8A59**, which is FRONTB.BUN
  screen 39's stored name hash. The port's 37-row registry covered only the `FUN_0016ADB0`
  registrations, so this row was absent and `apply_pending_screen` raised NotImplemented at
  0x0016C8C8 instead of opening the tree. A registration's document hash is the screen's stored
  name hash, **not** `StringHash_002534A8` of the class name in the same row.
- E-0021-23 Five of the six mode-select globals live inside the profile, so in the original the
  handlers' writes are save writes: `DAT_002FC06C` is profile+0x1C, `DAT_002FC1B4` +0x164,
  `DAT_002FC1C0` +0x170, `DAT_0030C77C` +0x1072C and `DAT_0030C784` +0x10734. Only
  `DAT_0031AB90` (0x0031AB90) lies outside it. `DAT_0030C784`'s readers are in the race code
  (0x001948C8 and about twenty more) and were not traced.
- E-0021-24 `FUN_001FB218` is the `View Event Tree` class dispatch. Message 0x0B allocates
  0x860 bytes from the `tEventTreeView` pool, calls `FUN_00185568(0x2FC050)` and hands both to
  `FUN_001F96B8`; 0x0C tears down through `FUN_001FA2B8(view, 3)`; 0x13 and 0x12 go to
  `FUN_001FA458` and `FUN_001FA698`; everything else to `FUN_001FAF60`. The `0x2FC050` literal
  is a fourth independent confirmation of the profile base, and it is why the tree follows the
  mode the mode-select screen wrote rather than a host-chosen one.
- E-0021-25 `FUN_00217020` and its Cops Off twin `FUN_002176D8` do not switch screens: they
  build a 0x1C-byte `tExitScreenAnimation` (pool string 0x002BC528) through `FUN_002490D8`,
  store it at instance+0x270 and let the switch happen only when it completes. The float each
  passes -- gp-0x6AC8 and gp-0x6AC4 -- is **650.0f** in both cases, the same x=650 endpoint the
  recovered exit path already uses, so it is a target position rather than a duration.

Hypotheses, explicitly not confirmed:

- H-0021-01 The manager's operation numbers requested through vtable+0xB4 (0x0012CCA8, which
  Ghidra has not defined as a function) are 1 for store, 2 for load and 3 for
  create-directory-and-icon, inferred from which starter requests which number
  (`FUN_00245B00` to 1, `FUN_00245BA0` to 2, `FUN_00245CA8` to 3). `FUN_0012C938`'s outer
  switch is on the manager's state word and its inner switch on `+0x0C`; whether the requested
  number lands in one, the other or both was not established, so the number-to-driver-function
  mapping is **not** asserted. The port does not need it: it talks to a host file, not a card.

## Unresolved questions

1. `row+0x00..0x01` of a requirement row.
2. `DAT_0031D7F4+0x180` / `+0x1B8` (the event 0x1F gate) and `iGpffffBEB0` (the unlock-all
   flag) are not traced to producers.
3. `FUN_002001A8`, `FUN_002006A0`, `FUN_00200768` — GoalType 3 and 4 slice decode.
4. The meaning of profile+0x0094, +0x0144, +0x0174 and +0x0178, beyond which function reads
   them.
5. The great majority of the 0x114E0 block. No code reading it was looked for outside the
   event tree's needs, so it stays unidentified rather than guessed.
6. Whether a real PS2 or PCSX2 accepts a `settings.db` this port writes (TASK-0023).

## Next steps

The scoped save work is complete and its event-tree consumers landed with it. What remains is
TASK-0022's, not this task's:

1. Decompile `FUN_001F6B70` (the state-2 event detail sub-menu over document 17) so "Status" and
   "Reward" can show real text, and the `tLineSegment` build inside `FUN_001F96B8` for the
   connector colour rule.
2. Decompile `FUN_0015E6A0` and the `tStillCameraMover` update in `indep\src\cameramover.cpp`,
   for the easing between the two recovered zoom levels and for whatever transform makes the
   authored 256 px panel fit a 52 px lattice -- the last port-side factor in the view.
3. Trace `DAT_0031D7F4+0x180` / `+0x1B8` and `iGpffffBEB0` before implementing either completion
   gate; both currently throw.
4. Read 0x0012CCA8 from the listing to settle H-0021-01, if the operation numbering is ever
   needed.

## Related Git commits

None yet. Record actual commit hashes only after creation.

## Completion evidence

2026-10-13 (Australia/Sydney). CMake 3.29.2, Ninja 1.12.0, MinGW GCC 13.2.0, Python 3.14.4,
Godot 4.4.1.stable.official.49a5bc7b6, Ghidra 12.0.4 over read-only GhidraMCP at 127.0.0.1:8080.

- `cmake --build --preset native-debug --parallel 4`; `ctest --preset native-debug`: 23/23
  passed (22 before this task; `memorycard` is new).
- `cmake --build --preset godot-debug --parallel 4`; `ctest --preset godot-debug`: 23/23 passed.
- `./tools/run_frontend.ps1 -RenderSmoke`:
  `NATIVE_RENDER_SMOKE_OK: screens=41 quads=1085 glyphs=7765 textures=120 empty=[36]`,
  identical to the pre-task baseline.
- `./tools/run_frontend.ps1 -RuntimeSmoke`: `FRONTEND_RUNTIME_SMOKE_OK`.
- Screen 39 captured on DX12 (Intel Arc B390) and visually inspected.
- `main_loop_catalog` unchanged; `reverse/main_loop/*.csv` untouched.

Remaining limits: nothing is `Verified` -- no PCSX2 or original-hardware comparison was
performed (TASK-0023), no real save file was read or written, and every test fixture is
synthetic. The seven unresolved questions above stand. The task stays **active** because its
own open questions 1-5 are unanswered; the scoped deliverable is done.

Not committed.

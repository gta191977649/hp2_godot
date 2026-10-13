# TASK-0021: `src\memorycard.cpp`, `settings.db` and the player profile

2026-10-13 (Australia/Sydney). Binary `SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`; Ghidra 12.0.4, GhidraMCP
**read-only** — no annotation, rename, comment, bookmark, data-type edit or function-boundary
change was made (AGENT.md R5). The original source file name is recovered:
`src\memorycard.cpp` (ELF string 0x002AD488), so the port-side module carries it.

**Nothing here is `Verified`.** No save file was read, no memory card was touched, and no PCSX2
or original-hardware comparison was performed. Every test fixture is synthetic.

## 1. Why this had to come first

`docs/EVENT_TREE.md` closed with six open questions, five of which were blocked on the same
missing thing: the game's save data. `EventTree_001FFC40::Status_002010B8` answered "no incoming
links means unlocked, everything else locked" and said in its own comment that reporting more
would be inventing save data. Reversing the save system was the only way past that, which is
why the user asked for it as a prerequisite.

## 2. The card-side layout

| Thing | Value | Evidence |
|---|---|---|
| Save directory | **`BASLUS-20362HP2`** (NTSC-U), `BESLES-50731HP2` (PAL) | `FUN_0012DEE0`, built with `"%s%s%s"` (0x0032FDC8) from `"BA"` (0x0032FDC0) / `"BE"` (0x0032FDB0), `"SLUS-20362"` (0x002AD6D8) / `"SLES-50731"` (0x002AD6C8) and `"HP2"` (0x0032FDB8) |
| Region switch | `FUN_001A3390` returns `uGpffffB364`; zero means PAL | `FUN_0012DEE0` |
| Path join / enumerate | `"/%s/%s"` (0x0032FD98) and `"/%s/*"` (0x0032FDA0) | `FUN_0012CDE8`, `FUN_0012CF00`, `FUN_0012D118`, `FUN_0012D858` |
| Payload file | **`settings.db`**, 0x114E0 = 70,880 bytes | string 0x002BEBE0; `FUN_00245B00` and `FUN_00245BA0` both pass that name and length |
| Icon descriptor | `icon.sys` (0x002AD5E8), 0x3C4 bytes from manager+0x184, magic `"PS2D"` (0x0032FDA8) | `FUN_0012D858` |
| Icon | `nfs6.ico` (0x002AD588), 0x10-byte header read through `FUN_00254908` | `FUN_0012DEE0` |

The module occupies roughly 0x0012A000..0x0012E6FF, immediately below `HP2_MainFrameLoop`
(0x0012F378).

## 3. The payload is self-describing

`FUN_00245738` is three statements, and they settle the format question completely:

```c
DAT_002fc050 = 0x114e0;                   // the first word holds the payload's own length
DAT_0030d52c = FUN_001878b0(0x2fc050);    // 0x0030D52C - 0x002FC050 == 0x114DC
return 0x2fc050;
```

So `settings.db` is **0x114E0 bytes with no separate header**: a size word at +0x0000, the
profile, and a checksum in the final dword at +0x114DC.

`FUN_001878B0` is a signed 32-bit sum of little-endian words from the buffer base. Its counter
starts at 0x4536 and the loop decrements before testing `-1 < counter`, so the body runs 0x4537
times — 17,719 words, exactly 0x114DC bytes, i.e. everything except the checksum itself. There is
no seed, no mask and no table. `FUN_00245780` verifies it on load.

The number 0x4537 falling out at exactly `checksum_offset / 4` is worth stating plainly: it was
derived from the loop's own termination condition, not assumed from the payload size, and the two
agreeing is independent corroboration that the payload size and the checksum offset are both
right.

## 4. The profile is the payload

The global player profile is one 0x114E0-byte block at **0x002FC050**, and the save path has no
serialiser at either end: `FUN_00245D30` copies a loaded payload straight onto it
(`HP2_CopyBytesForward(0x2FC050, **(screen+0x10), 0x114E0)`) and `FUN_00245738` hands the same
address back as the store source. The on-card byte order is the in-memory layout.

Three arithmetic identities, each from a different function, confirm the base:

- `0x002FC050 + 0x1072C == 0x0030C77C` — the tree selector `FUN_00185568` reads, and the exact
  global `docs/EVENT_TREE.md` §1 already named,
- `0x002FC050 + 0x188 == 0x002FC1D8` and `+ 0x288 == 0x002FC2D8` — the two event blocks,
- `0x002FC050 + 0x114DC == 0x0030D52C` — the checksum.

The offsets this task modelled are in
[the structure record](../reverse/structures/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/memorycard/Profile_002FC050.md).
**0x10E30 of the block's 0x114E0 bytes are unidentified** and are deliberately not modelled: the
port's type is an opaque byte array with named accessors, so no field is implied that was not
read out of an instruction.

### Post-load fix-ups

`FUN_00245D30` runs five of them in order after the copy: clear `DAT_002FC3D8`;
`FUN_001FE988` twice over the two event blocks (the slice passes the port already reproduced);
`FUN_001EF120(0x2FC0E4)`; `DAT_002FC1C4 = DAT_0031C1C8`; two `FUN_00154308` / `FUN_00155000`
calls over `DAT_002FC1C8`; `FUN_001F4B10(0x2FC194)`. Only the first two belong to the event tree.
The rest are other subsystems' and are not translated, so what they consume at +0x0094, +0x0144,
+0x0174 and +0x0178 stays unknown.

## 5. What this unlocked in the event tree

### The requirement table (`docs/EVENT_TREE.md` open question 4)

The `0x34A03` table at tree+0x24 is a **requirement** table, not a plain link table.
`FUN_002008B8(leaf, i)` returns `tree+0x24 + record[0x7C + 2i] * 8` for `i` under `record+0x3D`,
and `FUN_001FFFB0(row, tree)` asks whether the event named at `row+0x02` reaches status 4 **when
judged against that row's own `row+0x04` threshold**. An event unlocks when any one of its rows
is satisfied. Four of the six bytes TASK-0018 recorded as unidentified now have meanings;
`row+0x00..0x01` does not. The retired reading is kept and marked in `eventtree.hpp` per
AGENT.md R6.

### The status decode (open question 2)

`FUN_002010B8` answers five distinct values, not two:

| Value | Meaning |
|---|---|
| 0 | locked |
| 1 | unlocked, not attempted |
| 2 | attempted, below the goal's own floor |
| 3 | attempted, goal missed |
| 4 | won |

`FUN_001F75F0` maps 1, 2 and 3 to the same `_UNLOCKED` panel, which is why TASK-0018 could not
see the difference from the artwork alone. The per-GoalType slice decoders:

| GoalType | Size | Decoder | Rule |
|---|---|---|---|
| 0, 2 | 1 byte | `FUN_00200018` | `0` → 1; `value <= row+0x04` (unsigned) → 4; else 3 |
| 1, 5, 6 | 4 bytes | `FUN_00200068` | `0` or `0x7FFFFFFF` → 1; `value <= row+0x04` (signed) → 4; else 3 |
| 3 | 16 bytes | `FUN_002003F8` | **not recovered** — needs `FUN_002001A8` |
| 4 | 16 bytes | `FUN_002007D0` | **not recovered** — needs `FUN_002006A0`, `FUN_00200768` |

Two gates remain blocked on globals with no traced producer: event 0x1F on
`DAT_0031D7F4+0x180` / `+0x1B8`, and `iGpffffBEB0`, which short-circuits every event to 4.
Event 0x20 needs every event 1..31 of **both** trees, which needs the other tree's view.
All four of those raise `hp2::NotImplemented` naming their ELF address.

With an all-zero profile none of them is reached: the recovered arms answer first, and every
event but the root reports Locked — the same thing the port showed before, but now as the
recovered answer for an untouched save rather than as a shortcut.

### The start node (open question in `EventTreeView::load`)

`FUN_001F96B8` takes it from the profile: `FUN_001FA840(view, FUN_001F5D00(0x2FC1B4, type))`.
Read from the listing, because the decompiler lost the R5900 argument order:

- `FUN_001F5D00(block, type)` — type 0 reads block+8, type 1 reads block+4, anything else
  answers **1**.
- `FUN_001F5D28(block, type, number)` — selects on `block[0]`, **not** on its `type` argument,
  which it never reads even though its only caller passes the tree's type there. The quirk is
  preserved (AGENT.md R4).
- `FUN_001F5D50(block, type)` — two instructions, `sw a1,0x0(a0)`: it sets `block[0]`.

So profile+0x164 is the current tree type, +0x16C the last node for type 0 and +0x168 for
type 1. The port starting on event 1 turns out to have been right all along — 1 is
`FUN_001F5D00`'s own fallback — but for the wrong reason, and that is now fixed.

## 6. The port side

`src/game_core/memorycard/{include/memorycard.h,src/memorycard.cpp}` is the recovered half. It
touches no file, no card and no Godot header: it takes and returns `std::vector<std::uint8_t>`.
The `src\` root is recovered; the `include/` and `src/` split mirrors the existing `bSound/`
pair and is inferred, which the file header says.

`src/platform/ps2_profile_store.{hpp,cpp}` is the host half and the only place that opens a
file.

- **Reading** is opt-in and off by default. `HP2_PS2_PROFILE_PATH`, then a `profile_path` key in
  the gitignored `config/ps2_assets.local.json`, then the port's own previously written save.
  Nothing configured means a zeroed profile, which is what `FUN_00245700` hands the original's
  own driver before a load — so the front end behaves exactly as it did before this task.
- **Writing** goes to `<save root>/BASLUS-20362HP2/settings.db`, where the save root is
  `HP2_PORT_SAVE_ROOT`, then a `port_save_root` config key, then a per-user host location.
  It never writes to a memory card and never overwrites a configured read path; the one case
  where those could collide is checked and refused. See `docs/DECISIONS.md` D009.

The PS2 driver sequencing inside `src\memorycard.cpp` — `FUN_0012C938` and the `sceMc*`
wrappers below it — is deliberately not translated. A host file is not a memory card.

## 7. Open questions

1. `row+0x00..0x01` of a requirement row.
2. The producers of `DAT_0031D7F4+0x180` / `+0x1B8` and of `iGpffffBEB0`.
3. `FUN_002001A8`, `FUN_002006A0`, `FUN_00200768` — GoalType 3 and 4 slice decode.
4. profile+0x0094, +0x0144, +0x0174 and +0x0178 beyond which function reads them.
5. The other 0x10E30 bytes of the profile. Nothing outside the event tree's needs was searched
   for, so they stay unidentified rather than guessed.
6. The manager's operation numbers. H-0021-01 records what was observed — `FUN_00245B00`
   requests 1, `FUN_00245BA0` requests 2, `FUN_00245CA8` requests 3 — without asserting the
   number-to-driver-function mapping, because the dispatcher at 0x0012CCA8 is raw bytes Ghidra
   has not made a function and `FUN_0012C938` switches on a state word and separately on +0x0C.
7. Whether a real PS2 or PCSX2 accepts a `settings.db` this port writes. That is TASK-0023.

## 8. Evidence

`E-0021-01` .. `E-0021-21`, recorded per function under
`reverse/functions/sha256-7335.../{memorycard,frontend}/` and in `reverse/symbols.csv`, with the
findings listed in `tasks/active/TASK-0021-memorycard-profile.md`.

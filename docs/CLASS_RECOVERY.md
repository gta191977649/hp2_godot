# TASK-0027 — Recovering the binary's classes and their member functions

2026-10-13. US PS2 `SLUS_203.62`, revision unknown, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`. Ghidra MCP read-only
(`tools/ghidra.py`); the rest is offline over the local disassembly corpus. No Ghidra edits
(AGENT.md R5). No original comparison, so nothing here is `Verified` (AGENT.md R1).

The question was whether class names had been identified, and whether each class's functions
could be listed with its progress. They can — through one mechanism, not the obvious ones. This
is the write-up; the table itself is generated into
[PROGRESS.md](PROGRESS.md#classes) from `reverse/classes.csv`.

## What does not work, and why it is worth knowing

**Symbols.** The ELF has no `.symtab`, no `.dynsym`, no `.debug*`, and an empty
`.mdebug.eabi64` (TASK-0007, confirmed again here). Nothing to read.

**RTTI.** `analyze list_rtti` finds no game classes. Legacy C++ runtime type-info text does
exist at `0x002C7090` (`__class_type_info`, `__si_type_info`, `bad_typeid`, …), so the runtime
was linked in, but no game-class type-info record carrying a name was located. An empty tool
result is not proof of absence; this stays an open question rather than a finding.

**The `Class::Method()` label block.** `.rodata` holds 115 qualified-name strings across 48
class names — `World::DoTimestep - GlobalCopThink`, `PhysicsCar::ResolveForces()`,
`CarList::Sort()`. These are real, verbatim names. They are also **dead data**: the
instrumentation that passed them to a profiler was compiled out of the retail build.

That was established two independent ways, which matters because a single negative result from
one tool would not be enough:

1. Ghidra `references_to` on each string: 7 of 115 have any reference.
2. A scan of every instruction word in the whole local disassembly corpus for the `lui`/`addiu`
   or `lui`/`ori` pair that materializes each string's address — the complete test, since that
   is the only way MIPS forms a 32-bit constant. Same 7, and `memory search` finds no pointer
   to any of them anywhere in the image either.

So those 48 class names are recovered, but their methods cannot be bound to addresses by any
evidence in this binary. The seven that do bind:

| Qualified name | Function | Why it survived |
|---|---|---|
| `DemoDiscManager::Init()` | `0x0012E6C8` | live self-diagnostic, printed through `0x00251150` |
| `DemoDiscManager::SetEndReason()` | `0x0012EB00` | same |
| `DemoDiscManager::EndDemo()` | `0x0012EB00` | same function, second label |
| `SlotPool::CleanupExpandedSlotPools()` | `0x00257F28` | same |
| `SlotPoolManager::SlotPoolManager()` | `0x002584B0` | same |
| `SlotPoolManager::DeleteSlotPool` | `0x00258548`, `0x002584B0` | two sites; not uniquely bound |
| `bFile::Filename` | `0x00253828` | same |

This reproduces TASK-0019's five mappings from a different direction and adds two it had
rejected or not reached. Note the binding must be done on the **full string start**, not the
offset of the class name inside it: a label such as `WARNING: DemoDiscManager::SetEndReason()
…` is materialized at the start of the whole C string, so matching at `DemoDiscManager` finds
nothing. That one detail is the difference between 1 hit and 7.

## What does work: the named allocator

`FUN_002577C0(size, label)` is an allocator the game calls with the **name of the object it is
about to build**. `docs/POLICE_AI_MAP.md` had already found one instance of this at
`0x00131410` without generalising it: a `CopDriver` label at `0x002AE4E8`, a 0x4C-byte
allocation, then the constructor `0x001A1CE8`, which writes vtable `0x002B5998`.

That shape generalises. Sweeping every call site of `0x002577C0` across the corpus:

- **579 call sites**, 350 with a resolvable label, **201 distinct labels**.
- 26 of those labels are original `__FILE__` paths (`..\indep\src\car.cpp`) rather than class
  names — those belong to `docs/ORIGINAL_SOURCE_TREE.md` and are excluded here by shape.
- **175 class names**, each with the object size from the allocation's first argument.

Reading the argument requires honouring the branch delay slot: the compiler routinely puts the
low half of the label address in the slot after the `jal`, which executes before the call.

## From a name to its functions

The chain is: label → allocation size → the call after the allocation → the vtable that call
stores → the vtable's slots.

The vtable layout is the old GCC/ARM one, read straight off recovered code rather than assumed.
`001F1DD8` message `0x0C` calls `(**(code **)(vtable + 0xC))(this + *(short *)(vtable + 8), 3)`,
and `001F1B08` message `0x12` does the same at `+0x10`/`+0x14`. So entries are 8 bytes — an
`int16` `this` delta then the function pointer — with slot *k* at `vtable + 8 + 8k` and its
pointer at `vtable + 0xC + 8k`. The two words at `+0` and `+4` are the header. A scan that
starts at `+4` finds nothing but zero, which is the trap.

Candidate vtables come from the corpus: a `.rodata` constant that a function materializes **and
then stores into memory**. 393 candidates, of which **252 confirm** as runs of at least two
consecutive slots holding a 4-aligned `.text` address, giving **1577 member slots** over 791
distinct functions.

The confirmation is self-validating in a useful way: across all 1577 accepted slots, every
`this` delta is 0, −8 or −256. Random data would spread those across the full 16 bits, so the
absence of a single arbitrary value is strong evidence that the runs really are vtables. That
constraint is now part of the acceptance test rather than a lucky observation.

Four independent cross-checks against findings the repository already had, all from different
tasks:

| Check | Expected | Recovered here |
|---|---|---|
| Audio Options vtable | `0x002BA558` → handler `0x001EFB80` (TASK-0017, E-0017-B01) | matches |
| Controller vtable | `0x002BE5B8` → includes the *uncreated* `0x002402C0` | matches |
| Jukebox vtable | `0x002BA538` → includes the *uncreated* `0x001F0D10` | matches |
| CopDriver / CopMover | ctors `0x001A1CE8` / `0x001A0DD8` (TASK-0008, E-0008-02) | matches |

The two "uncreated" entries are the ones TASK-0017 had to read as raw listings because Ghidra
had never created functions there. That is why the slot test is "4-aligned inside `.text`" and
not "a known Ghidra function": whether a function object exists is a property of that database,
not of the binary. Each member records which it is.

## Names this gives the port

Several classes the port currently refers to by address now have their original names. These are
`Recovered` under AGENT.md R2 and should replace the address-anchored names, which is tracked as
debt rather than done here:

| Recovered name | Size | What the port calls it |
|---|---|---|
| `AudioOptionsScreen` | 792 | the Audio Options screen (`001F0278` / `001EF3C8`) |
| `tDisplayOptionsScreen` | 616 | the Display Options screen (`001F3EC0` / `001F33B8`) |
| `tOptionsScreen` | 524 | the Options screen (`001EEF00`) |
| `JoystickPreconfig` | 644 | the Controller screen (`00240060` / `002400C0`) |
| `JukeBoxMusicScreen` | 576 | the Jukebox screen (`001F1220` / `001F02D8`) |
| `tCustomizeHUDScreen` | 580 | not yet reached |
| `NameEntry` | 776 | `NameEntry`, which the registry's class-name literal already gave |
| `tEventTreeView` | 2144 | the event tree view (TASK-0021/0022) |
| `tEventGroup`, `tEventLeaf` | 40, 96 | the event tree's node records |
| `Autosaver`, `BootMemcardLoader` | 64, 88 | the autosave and boot-load paths |
| `CopsOnModeSelect` | 632 | the Cops On mode select (`002174A0`) |

## Limits

- **A class is only here if its objects go through the named allocator.** The Language Options
  screen does not: `001F1DD8` builds it with `FUN_00257690(0x10, 0x10)`, which takes no label.
  Its class name is therefore **unknown**, and so are the names of every other class built that
  way. 175 is a floor on the class count, not the count.
- **Only virtual members are listed.** Nothing in a stripped image ties a non-virtual function
  to its class. A class's function count is a floor; 0 means nothing is bound yet, not that the
  class has no methods.
- **The call after an allocation is not always the constructor.** An argument can be computed by
  a call in between — `tEventTreeView`'s next call is `0x00185568`, which reads the profile's
  selected mode. So a call is only recorded as the constructor when it is independently seen
  storing a vtable; the raw first-call candidate is kept in a separate column.
- **4 of the 64 attributed vtables belong to more than one class.** That is a shared base class
  (each derived constructor writes the base vtable) or label variants of one class
  (`DriveCameraMover(DCC)` and friends). Those classes' member lists include inherited slots, so
  their counts are attributed, not exclusive.
- 29 single-slot candidates are left unconfirmed rather than claimed: one slot alone cannot be
  distinguished from a stored pointer that happens to be followed by something `.text`-shaped.
- Object sizes are the allocation request, which can exceed `sizeof` if a call site over-allocates.
- None of this establishes behaviour. A class with 40 identified members has 40 addresses, not
  40 understood functions.

## Reproducing

```bash
py -3 reverse/scripts/recover_classes.py --bind          # name -> address, offline
py -3 reverse/scripts/recover_classes.py --candidates    # vtable candidates, offline
py -3 reverse/scripts/recover_classes.py --vtables       # confirm against the binary
py -3 reverse/scripts/recover_classes.py --allocations   # allocator labels
py -3 reverse/scripts/recover_classes.py --emit          # write reverse/classes.csv
py -3 reverse/scripts/generate_progress.py               # render the table into PROGRESS.md
```

`--vtables` reads `.rodata` through the read-only Ghidra client once and caches it under the
gitignored `local/classes/`, because raw bytes of the original image must never enter Git
(AGENT.md R7). Only derived addresses reach `reverse/`. Every other pass is offline over
`ps2_decomp/`, which is also gitignored.

One defect worth recording, because it silently weakens any future corpus scan: the corpus
prints instruction words without leading zeros, so a fixed eight-digit match drops **every**
R-type, `j` and `jal` instruction (all of which are below `0x10000000`). That both hides calls
and disables register invalidation on R-type writes, which lets a stale `lui` pair with an
unrelated low half. The regex takes a variable width for that reason.

## Evidence

- `E-0027-C01`: `reverse/classes.csv` — 175 class names with sizes, constructors, vtables and
  members. Produced by `reverse/scripts/recover_classes.py --emit`.
- `E-0027-C02`: `reverse/classes/allocations.json` — 579 `FUN_002577C0` call sites with labels,
  sizes and the following call.
- `E-0027-C03`: `reverse/classes/vtables.json` — 252 confirmed vtables with slot deltas, member
  addresses and Ghidra-created state, plus the 29 unconfirmed single-slot candidates.
- `E-0027-C04`: `reverse/classes/name-binding.json` — the 115 qualified labels, the 7 that bind
  and the 91 with no reference anywhere in the image.
- `E-0027-C05`: vtable layout read from `001F1DD8` message `0x0C` and `001F1B08` message `0x12`
  (see `docs/FRONTEND_MENU_LANGUAGE.md`); `memory list_blocks` section ranges.

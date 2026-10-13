# TASK-0027 — Recover the binary's classes and their member functions

- Status: active (175 class names and 791 member functions recovered and tracked; the classes
  built through the unlabelled allocator, and every non-virtual member, remain unidentified)
- Objective: answer "which classes does this binary have, which functions are inside each, and
  how far has each got" from evidence, and keep the answer in `docs/PROGRESS.md` as a generated
  table rather than prose.
- Scope: included — class-name recovery, class → constructor → vtable → member binding, the
  per-class progress table, and the tooling that regenerates it. Excluded — renaming the port's
  existing address-anchored identifiers to the recovered class names (tracked below as debt),
  recovering non-virtual members, and any behavioural work on the classes found.
- Original PS2 addresses: SLUS_203.62,
  sha256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
  `0x002577C0` (named allocator), `0x00257690` (plain allocator, no label), `0x00251150`
  (diagnostic print), `0x00131410` (the allocation site TASK-0008 found first), and the 252
  confirmed vtables listed in `reverse/classes/vtables.json`. Data: `.rodata`
  `0x002AB000..0x002C72BD`, the qualified-label block around `0x002AC920..0x002B0000`, the class
  label block around `0x002AE478..0x002AE5A0`, RTTI runtime text at `0x002C7090`.
- Dependencies: TASK-0007 (no symbol table), TASK-0019 (qualified-name inventory and the five
  original method mappings), TASK-0008 (the `CopDriver` allocation site), TASK-0017 (the vtables
  and uncreated entries used as cross-checks), TASK-0026 (the generated-table mechanism).
- Relevant files: `reverse/scripts/recover_classes.py`, `reverse/classes.csv`,
  `reverse/classes/`, `docs/CLASS_RECOVERY.md`, `docs/PROGRESS.md`,
  `reverse/scripts/generate_progress.py`.
- GitHub issue: not created

## Acceptance criteria

- [x] Every class name the binary gives up is recovered verbatim, with its provenance recorded.
- [x] Each class's member functions are identified where the binary allows it, by address.
- [x] Per-class progress is generated from records, not typed, and appears in `docs/PROGRESS.md`.
- [x] The recovery is cross-checked against findings from other tasks that were reached by
      different means.
- [x] What the method cannot see is stated where a reader would otherwise assume coverage.
- [ ] The port's address-anchored names are replaced by the recovered class names (AGENT.md R2).
      Not started; see next steps.

## Verification procedure

```
py -3 reverse/scripts/recover_classes.py --bind
py -3 reverse/scripts/recover_classes.py --candidates
py -3 reverse/scripts/recover_classes.py --vtables
py -3 reverse/scripts/recover_classes.py --allocations
py -3 reverse/scripts/recover_classes.py --emit
py -3 reverse/scripts/generate_progress.py --check
ctest --preset native-debug --output-on-failure
```

`--vtables` reads `.rodata` once through the read-only Ghidra client and caches it under the
gitignored `local/classes/`; every other pass is offline. No Ghidra state is modified.

## Current findings

Full write-up with the evidence chain: `docs/CLASS_RECOVERY.md` (E-0027-C01..C05).

- **Class names come from the named allocator**, `FUN_002577C0(size, label)`, which the game
  calls with the name of the object it is about to build. 579 call sites, 201 distinct labels,
  **175 of them class names** (26 are original `__FILE__` paths and belong to
  `docs/ORIGINAL_SOURCE_TREE.md`). TASK-0008 had found one instance of this shape at
  `0x00131410` without generalising it.
- **Members come from vtables.** 393 candidates (a `.rodata` constant a function materializes
  *and stores*), **252 confirmed**, 1577 slots, **791 distinct member functions**. Layout is the
  old GCC/ARM one: 8-byte entries, pointer at `vtable + 0xC + 8k`, `int16` `this` delta at
  `+8 + 8k`, header at `+0`/`+4`.
- **The `Class::Method()` label block is dead data.** 115 qualified names across 48 class names
  exist in `.rodata`, but only 7 are referenced by any code; the profiler instrumentation that
  used them was compiled out. Established twice over: Ghidra `references_to`, and a scan of
  every instruction word in the corpus for the `lui`/`addiu`|`ori` pair that materializes each
  address. `memory search` finds no pointer to them either.
- Binding must use the **full string start**, not the offset of the class name inside the
  string. That alone is the difference between 1 and 7 bindings.
- The slot test is self-validating: across all 1577 accepted slots every `this` delta is 0, −8
  or −256, where random data would spread over 16 bits. The constraint is now part of the test.
- Four cross-checks against other tasks all match: the Audio (`0x002BA558`), Controller
  (`0x002BE5B8`) and Jukebox (`0x002BA538`) vtables including the two entries Ghidra never
  created, and the `CopDriver`/`CopMover` constructors from TASK-0008.

## Unresolved questions

- **Classes built through the unlabelled allocator have no recoverable name.** `FUN_00257690`
  takes no label, and the Language Options screen is built with it, so its class name is
  unknown. 175 is a floor on the class count.
- **Non-virtual members cannot be attributed.** Nothing in a stripped image ties a non-virtual
  function to its class, so every per-class function count is a floor.
- 4 of the 64 attributed vtables belong to more than one class — a shared base, or label
  variants of one class. Those member lists include inherited slots.
- 29 single-slot vtable candidates are unconfirmed and deliberately not claimed.
- Whether game-class RTTI records exist is still open. The C++ runtime type-info text is present
  at `0x002C7090` but `analyze list_rtti` recognises no game classes; an empty tool result is
  not proof of absence, and a targeted search of that region has not been done.
- The qualified-label block names 48 classes whose methods can never be bound from this binary.
  A debug or prototype build, if one existed, would bind them — out of scope and not assumed.
- Object sizes are allocation requests and may exceed `sizeof`.

## Next steps

1. Rename the port's address-anchored screen identifiers to the recovered class names
   (`AudioOptionsScreen`, `tDisplayOptionsScreen`, `tOptionsScreen`, `JoystickPreconfig`,
   `JukeBoxMusicScreen`, `tEventTreeView`, `tEventGroup`, `tEventLeaf`, `CopsOnModeSelect`),
   updating `reverse/symbols.csv` and keeping the addresses in the doc comments (AGENT.md R2,
   and the R8 debt list).
2. Search the `0x002C7090` RTTI region for game-class type-info records; if any carry names,
   they would bind classes the allocator sweep cannot see.
3. Attribute non-virtual members by call-graph locality as a *hypothesis* layer, kept strictly
   separate from the evidenced table — or decide not to, and record that.
4. Fold the per-class member addresses into `reverse/symbols.csv` as `Identified` rows so the
   function tracker and the class tracker stop disagreeing about what is known.

## Related Git commits

None yet. Record actual commit hashes only after creation.

## Completion evidence

2026-10-13. CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0 (Strawberry), Windows 11; Ghidra MCP
read-only over `tools/ghidra.py`, program `SLUS_203.62`.

- All five `recover_classes.py` passes ran; outputs are `reverse/classes.csv` and
  `reverse/classes/{allocations,vtables,name-binding,vtable-candidates}.json`.
- `py -3 reverse/scripts/generate_progress.py --check` — `PASS generated regions are current`.
- `ctest --preset native-debug --output-on-failure` — **25/25 passed** (unchanged; this task
  adds no runtime code).

The task stays **active**: the recovery and the tracker are delivered with evidence, but the
identifier retrofit in step 1 is an acceptance criterion that is not met, and the limits above
are real gaps rather than finished work. Nothing here establishes behaviour — a class with 40
identified members has 40 addresses, not 40 understood functions — and no status was promoted to
`Verified` (AGENT.md R1, R11).

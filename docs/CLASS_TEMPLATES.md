# C++ class templates and continuation — TASK-0028 / TASK-0029

Target: US PS2 `SLUS_203.62`, executable revision unknown (SYSTEM.CNF VER 1.00),
SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
No class layout, behaviour or original equivalence is established by these templates.

## Coverage

The inventory covers **190 class spellings**: 152 valid identifiers from TASK-0027's
175 allocator labels, plus 38 additional names from the qualified diagnostic catalogue.
The other **23 allocation labels** contain spaces/parentheses and remain catalogue-only,
as requested. They are not sanitized into invented C++ names.

There are **609 member attributions over 370 distinct addresses** in this coverage:
596 attributions from valid allocator-labelled classes, five confirmed original-method
mappings, and six additional title/leaf entries plus two existing police helpers.
Shared vtable targets appear in multiple classes without asserting inheritance or exclusive
ownership. Of the 609 attributions, **603 are throwing templates** and six preserve existing
native projections (two police local bodies and four title slices). Twelve templates have
separate existing partial projections which remain available to their current callers.
**134 classes have no bound members** and therefore contain no speculative functions.

This is narrower than all 702 allocator-label attributions: the 23 catalogue-only labels are
excluded from C++ generation. The additional diagnostic classes are outside `classes.csv`'s
allocator-only table, so its headline class count intentionally remains 175.

- [Template inventory](../reverse/classes/template-inventory.json): class, namespace, header,
  source, member address, spelling, evidence record, signature, coverage and preserved projections.
- [Placement/member policy](../reverse/classes/template-policy.json): explicitly retained module
  destinations and separately evidenced non-virtual members.
- [Audit evidence](../reverse/classes/template-audit.json): fresh read-only Ghidra observations.

## Directory and interface rules

Existing classes `CopDriver`, `CopMover`, `TitleScreen` and `tEventLeaf` keep their source
files, storage and bodies. Screen shells live beside their existing partial projections in
`options` / `joystickconfiguration`, and event-node/view shells beside `eventtree`. This
preserves this port's established module placement; a module's original source-path string
does **not** prove that every new class was originally defined in that translation unit.

TASK-0029 supersedes the initial `port/class_templates` placement. Of those 179 class pairs,
170 now live directly under `src/game_core`; nine use directories confirmed through fresh
Ghidra evidence. See [CLASS_SOURCE_PATHS.md](CLASS_SOURCE_PATHS.md) for the complete sweep,
accepted paths and unresolved classes. Root placement and per-class basenames/header
extensions are port-side choices. Empty shells also compile via their `.cpp`.

Unknown member names use `Sub_<ADDR>`. The five uniquely bound original method names keep
their exact spelling. Every new method has an explicitly **port-side void/no-argument
placeholder**: neither a Ghidra `undefined8` signature nor an analysis-purpose prototype is
adopted as a verified original ABI. Constructor/destructor entries use ordinary named methods,
not native C++ lifetime hooks. Instantiating an empty shell performs no original initialization.
Invoking an unresolved member calls `hp2::not_implemented` at its own address and throws.

Existing free functions and utility projections keep their typed host interfaces and bodies.
They do not become class members merely because their address occurs in a screen's vtable.
A complete class-entry placeholder can coexist with such a partial helper; the inventory links
them and does not claim that the helper implements the whole original entry.

## Naming audit

- `Type_0016F000` becomes `TitleScreen`: 0016F440 passes label 002B2F68 to the named allocator
  and feeds the returned object directly to 0016F000 (E-0028-C02).
- `EventLeaf_001F6FA0` becomes `tEventLeaf`: 001F96B8 allocates with label 002BA878 and
  directly calls 001F6FA0 (E-0028-C03). `EventTree_001FFC40` remains the data-source projection;
  it is **not** renamed to the different `tEventTreeView` object.
- The two police helper methods and four attributed partial utility functions lose invented
  descriptive spellings in favour of `Sub_<ADDR>`; title methods follow the same rule. Callers,
  tests and current symbol/source links are updated together. Their bodies are preserved.
- Fresh diagnostic bytes and decompile confirm 002584B0 as `SlotPoolManager::NewSlotPool`.
  The same warning mentions the constructor as a prerequisite, not as the current entry's
  identity. TASK-0019's detailed interpretation wins over TASK-0027's conflicting summary.
  `bFile::Filename` remains a field expression; no callable template is created for it.
- `tExitScreenAnimation` / `tMenuAnimElement` have recovered labels but no accepted binding to
  the existing address-named utility classes. They get unbound shells; no speculative rename.

## Safe regeneration and future implementation

```powershell
py -3 reverse/scripts/generate_class_templates.py --write
py -3 reverse/scripts/recover_classes.py --emit
py -3 reverse/scripts/generate_class_templates.py --check
py -3 reverse/scripts/generate_progress.py
cmake --preset native-debug
cmake --build --preset native-debug --parallel 8
ctest --preset native-debug
```

`--write` appends missing declarations/definitions and creates missing files. It preserves
existing method bodies, fields and knowledge records, including a record not yet indexed in
`symbols.csv`. It never removes stale members or rewrites a recovered body. A declaration
without a definition fails for manual review. Changes to generated inventory, source list and
contract tests are expected; those are derived artifacts.

To implement an entry, select it from the inventory, read its function record and audit it
read-only in Ghidra. Replace the placeholder signature/body **in place**, preserve its address
and update evidence/tests. Run `--write` to refresh coverage. The generator stops including a
changed body in the stub exception tests and does not promote its recovery status. Behavioural
tests and status decisions must be documented separately. The generator recognizes current
address-named out-of-line methods and the five explicitly bound original methods; new original
spellings or inline definitions require updating the evidence/tooling explicitly.

Class templates introduce no Godot dependency or runtime integration. They are compiled into
`game_core`; current runtime callers continue to use their preserved host projections.
Native tests validate compilation, coverage, preservation and exceptions, not PS2 equivalence.

# TASK-0043 - Complete class mapping templates and confirmed identifiers

- Status: completed
- Request: recheck all supported class-member mappings, fill missing CPP/HPP templates and restore confirmed original function names throughout the project.
- Defaults confirmed by user: missing behavior throws hp2::NotImplemented; unmapped Debug methods remain in a per-class pending catalogue, not compiled Retail entries.
- Scope: existing SHA-bound TASK-0041/0042 mappings, read-only Ghidra follow-up for conflicts/TitleStringId_001F96B8, exact class/member ownership, source references, generators and evidence records. User additionally requests field/struct/class/type/constant naming restoration with entity-specific evidence. No behavior or ABI invention.
- Retail SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Debug SHA-256: 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
- Startup: worktree clean; read binding AGENT.md and current progress, architecture, decisions, development and reverse notes. TASK-0042 completed.

## Acceptance

- [x] Account for every supported class method: existing implementation, throwing template, added template, ownership conflict or explicit unrepresentable entry.
- [x] Catalogue Debug-only/unmapped methods per class without inventing Retail addresses.
- [x] Audit all address-suffixed identifiers by entity type; adopt only confirmed original spellings and maintain address/evidence provenance.
- [x] Trace TitleStringId_001F96B8 independently; preserve its identifier if evidence remains insufficient.
- [x] Preserve existing bodies, throwing gaps, verification statuses and independent host interfaces; update generators and durable evidence.
- [x] Native/Godot builds, relevant suites, idempotence and generated/diff checks pass.

## Validation and findings

The bounded naming/template pass is complete; no game behavior was added.

- 1,025 adopted Retail mappings checked; 822 class-method rows in 250 original declaring scopes accounted for: 650 added whole-entry throwing templates, 169 existing throwing entries, three explicit nested/template exclusions.
- Generated total: 372 classes and 1,203 member attributions. The 4,999 unmapped Debug methods are catalogue-only. No Retail address was invented.
- 42 existing callable identifiers restored, plus EventRecord_001FFF60 -> EventInfo and 11 independently proven fields. Host signatures/storage remain projections. Identical offsets in other types retain their names.
- E-0043-T01 retains TitleStringId_001F96B8 because it is an extracted constructor branch, not a separately named original function. No guessed replacement.
- 25 wrongly attributed duplicate sole-throw templates removed; exact original bodies and canonical same-address throwing targets are pinned and audited.
- 1,411 address/offset identifiers and declared types inventoried across source/Godot scripts with lexical categories and bounded dispositions. Constants, aliases and fields do not inherit a name from a containing function address.
- All 1,602 existing symbol rows/statuses preserved in original order. No verification promotion or Ghidra mutation. No commit made.

### Exact validation commands and results

Tools: Python 3.14.4, CMake 3.29.2, MinGW GCC 13.2.0; configured pinned Godot bindings and read-only Ghidra MCP.

```powershell
py -3 reverse/scripts/audit_title_projection.py
py -3 reverse/scripts/trace_event_type_fields.py
py -3 reverse/scripts/adopt_task0043_identifiers.py --write
py -3 reverse/scripts/refresh_task0043_projection_bindings.py --write
py -3 reverse/scripts/complete_class_members.py --write --registry-only
py -3 reverse/scripts/generate_class_templates.py --write
py -3 reverse/scripts/complete_class_members.py --write
py -3 reverse/scripts/collect_class_source_paths.py --report
py -3 reverse/scripts/audit_task0043_identifiers.py --write
py -3 reverse/scripts/audit_task0043_preservation.py --renames reverse/naming/task0043-identifier-renames.json --report reverse/naming/task0043-preservation-audit.json
py -3 -m unittest discover -s reverse/tests -v
py -3 -m unittest discover -s reverse/scripts/tests -p test_complete_members.py -v
cmake --build --preset native-debug --parallel 4
ctest --preset native-debug --output-on-failure
cmake --build --preset godot-debug --parallel 4
ctest --preset godot-debug --output-on-failure
py -3 reverse/scripts/generate_progress.py
py -3 reverse/scripts/generate_progress.py --check
py -3 reverse/scripts/generate_class_templates.py --check
py -3 reverse/scripts/complete_class_members.py
py -3 reverse/scripts/adopt_task0043_identifiers.py --check
git diff --check
```

- Live evidence scripts passed: seven Title queries and 11 field bridges. Original responses remain ignored in local/task0043/title.
- Native build passed; CTest 61/61 passed, 57.82s.
- Godot build passed; CTest 67/67 passed, 73.58s, including the six Godot host suites.
- Python discovery 54/54 passed; separate complete-member safety tests 7/7 passed.
- Preservation passed: 1,795 existing bodies retained, 25 pinned duplicate gap removals, one separately recorded regenerated test main, 650 added sole-throw bodies, zero unexplained production changes or new nonstub bodies.
- Repeated identifier adoption/current-binding generation/member registry/class generation/member coverage produced no byte differences across 2,637 source/evidence files. Logs remain ignored in local/task0043.
- Final generation and whitespace checks passed; untracked restricted-suffix scan found no binary/raw asset files.

Intermediate checks caught overlapping no-argument Wheel::SetState placeholders, a field receiver outside the initially reviewed file list and legacy namespace/path test assumptions. Corrected by preserving whole-entry overload addresses, binding field edits to exact receivers, updating source reports and retaining fixture compatibility. Final suites above all passed. An initial Windows symbols.csv write failed transiently before mutation; rerun and full idempotence/status checks succeeded.

No PS2/PCSX2 or visual equivalence was tested because this pass adds names/gaps only. Existing runtime behavior tests and original-body protection were retained. Unverified Retail ABI, native class/field layouts, constructors/operators and the 4,999 unmapped Debug methods remain explicit unknowns.

## Continuation

Read docs/CLASS_MAPPING_COMPLETION.md and the TASK-0043 member/identifier/field registries. Continue unknown names only with new independent caller/table/type/field evidence. Use TASK-0042 adopted mappings and source ownership together with the canonical class registry. Do not run one-time TASK-0042 migrations on the current tree. Original addresses and exact reference names remain separate from host projection signatures.

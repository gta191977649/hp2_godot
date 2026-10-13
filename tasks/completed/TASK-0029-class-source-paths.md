# TASK-0029 — Root-level class files and evidenced original directories

- Status: completed
- Objective: move all 179 class pairs currently under `src/game_core/port/class_templates` into `src/game_core`, then relocate individual classes only where read-only Ghidra evidence confirms an original directory.
- Scope: file placement, includes/build lists/generator policy, class-to-source-path evidence and validation. Preserve all implementations, identifiers and template coverage. No behavioural reconstruction or Ghidra changes.
- Original PS2 addresses: class/member catalogue and source-path strings in SLUS_203.62, executable revision unknown; SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
- Dependencies: TASK-0028, TASK-0027, TASK-0020; reverse-engineer-anything workflow; Ghidra MCP.
- Relevant files: `src/game_core`, `reverse/classes/template-policy.json`, source-path evidence ledger, template generator, `docs/CLASS_TEMPLATES.md`.
- GitHub issue: not created

## Acceptance criteria

- [x] All former class_templates pairs are outside that directory, with no overwrite/lost body.
- [x] Unknown-path classes remain directly under game_core, explicitly marked as user-chosen port placement.
- [x] A bounded Ghidra source-path sweep is recorded for every moved class; confirmed bindings carry exact paths, addresses and evidence, and uncertain associations remain unknown.
- [x] Confirmed original directories are mirrored without guessed middleware roots or sanitizing original identifiers.
- [x] Generator, includes, symbols/inventory/build lists and tests match the final placement and cannot regenerate the old directory.
- [x] Relevant builds/tests, durable progress and continuation are recorded.

## Current findings

The user explicitly supersedes TASK-0028's unknown-path placement: root-level class pairs are
now preferred regardless of whether an original path is known. This changes file placement,
not provenance. All existing dirty/untracked TASK-0028 work must be retained.

Final placement: 179 class pairs moved, 170 root-level unknowns, eight confirmed indep/src
pairs and bFile under src. Eleven pre-existing module classes unchanged. Coverage remains
190 classes, 609 member attributions and 603 throwing templates. All 358 original standalone
files retain their contents except self-includes, provenance comments and line endings.

E-0029-S01 records 298 path occurrences / 54 distinct paths and 323 references. Nine focused
bindings are E-0029-P01..P09 in the source-path decisions/results ledgers. Four new path-only
Identified records/index rows preserve additional initializer/assertion evidence. No existing
function status, identifier, method signature or body changed.

## Hypotheses

None adopted. A source-path string near a class label does not bind the class to that file.
Shared/inherited member targets and inline-header diagnostics require separate treatment.

## Unresolved questions

170 scoped classes remain unresolved by this bounded evidence. Unreferenced profiler strings,
generic inline-header diagnostics and shared targets are not exclusive class-file attribution.
Original declaration headers and the prefix before bFile's src directory remain unknown.

## Verification procedure

Read-only Ghidra memory search/read, source-path references and focused listing/decompile;
template generator --check, native configure/build/CTest and Godot link validation if needed.

## Next steps

Task complete. Read docs/CLASS_SOURCE_PATHS.md and reverse/classes/source-path-bindings.json
before another path investigation. To reproduce the sweep, use
`py -3 reverse/scripts/collect_class_source_paths.py --collect --report` with read-only Ghidra
and the private instruction corpus. Do not move an unknown root-level class until a new
reviewed decision exists. Regenerate class metadata and progress after any relocation.

## Related Git commits

None created.

## Completion evidence

2026-10-13 Australia/Sydney. CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0, Python 3.14.4;
registered ELF SHA-256 and godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b confirmed.

- Passed: `py -3 reverse/scripts/collect_class_source_paths.py --collect --report` and focused
  read-only Ghidra decompile/listing/memory/reference checks; no database changes.
- Passed: `py -3 reverse/scripts/generate_class_templates.py --write` / `--check`.
- Passed: `py -3 -m unittest discover -s reverse/tests -p test_class_templates.py`, six tests.
- Passed: `cmake --preset native-debug`; `cmake --build --preset native-debug --parallel 8`;
  `ctest --preset native-debug`, 28/28 tests.
- Passed: `cmake --preset godot-debug -DCMAKE_PROJECT_PS2PCPort_INCLUDE=D:/dev/hp2_godot/local/task0029/isolated_output.cmake`
  and `cmake --build --preset godot-debug --parallel 8`; DLL linked under local/task0029/godot-bin.
  Restored normal configuration using `cmake --preset godot-debug -U CMAKE_PROJECT_PS2PCPort_INCLUDE`.
- Passed: `ctest --preset godot-debug -R 'class_template_'`, 3/3 tests.
- Passed: preservation hash audit of all 358 files (only includes/provenance/line endings differ),
  generated progress checks and `git diff --check`. No old class_templates source/include links
  remain in current code, inventory, policy, build list or symbols index.
- Optional cleanup blocked: empty-directory removal was rejected as "blocked by policy".
  Safe validated move-only commands completed all file relocations; the leftover directory is
  empty and contains no code. No approval was requested or bypass attempted.

No original-runtime comparison, behavioural implementation, API expansion, proprietary file
addition or commit. The isolated Godot link did not replace the running frontend's DLL.

# TASK-0045 append-only source review

This is port-side source integrity tooling, not original-game behavioral evidence. It does not promote any implementation to Verified.

## Frozen predecessor

Before TASK-0045 production edits, `historical.collect_current()` captured all 2,577 recognized function bodies under `src/` into ignored `local/task0045/pre-source.json`. The durable `pre-source-identities.json` retains only the 2,362 production function identities: source path, declarator name, signature-token SHA-256, body-token SHA-256 and sole-throw classification. Test bodies remain outside production preservation, matching the existing checker scope. Raw source tokens are not copied into the durable inventory.

The inventory-list SHA-256 is `423f0738818d3be6467627a1185dfa9d8a05c1c800a79ef5675ea44512070ec9` and is fixed in the checker. The unchanged TASK-0044 review has raw captured SHA-256 `457d0b769f8a02bc81a3871b300ce3fbba053cfcb0752f66c5bbebec72a37d6f`. Its LF-normalized byte SHA-256, `8d5ed24700c986fe51d464c56380a27a40fcc66bf2359e412b4a2f941cbbab1e`, is fixed in the checker. Only CRLF/LF conversion is normalized; reformatting or changing any predecessor pin fails. These anchors do not depend on ignored files in later sessions.

## Separate successor review

`reverse/scripts/audit_task0044_migration.py` keeps the original `check_review()` behavior unchanged. Its current-tree entry point now validates two distinct stages when the TASK-0045 baseline/amendment files exist:

1. Validate baseline commit8c7efde plus every unchanged TASK-0044 before/after permission against the frozen predecessor inventory. The remaining body-hash multiset follows TASK-0044's historical move-preservation policy.
2. Validate the current production functions against that frozen inventory using the separate `source-review.json` permissions. Remaining functions must retain their full identity. An unreviewed path/name/signature change fails even if its body is unchanged. New throwing stubs also require explicit permissions.

A replacement requires the exact old identity in `before` and exact new identity in `after`. An addition requires its exact `after` identity. A deliberate removal requires its exact `before` identity and an explicit reason. Every permission needs evidence and a reason; duplicates, unmatched permissions, missing replacements, new unreviewed functions and wrong predecessor/target fingerprints fail. Neither a missing successor file nor changing the frozen baseline can disable the chain. Deleting both successor files cannot fall back to the historical check; older trees retain their earlier checker version.

The checker has no write, refresh, or accept-current mode. The TASK-0044 review is never regenerated to accommodate TASK-0045 changes. Successor permissions are added only after the integrating owner identifies the changed functions and reviews the reason/evidence. This review covers recognized outer C++ function tokens; it is not a claim to parse the complete C++ language or validate binary numerical equivalence.

## Validation

`py -3 -m unittest discover -s reverse/tests -p test_task0045_source_review.py`: 13 passed. Tests cover original API compatibility, exact successor acceptance, attempts to refresh old pins, unmatched changes, missing replacements, unreviewed additions/stubs, signature/path drift, missing evidence, duplicate permissions, explicit removal, predecessor-file changes, frozen-inventory changes and missing successor files, including deletion of both files.

`py -3 -m unittest discover -s reverse/tests -p test_task0044_migration.py`: 12 passed unchanged.

Final source review contains 11 exact before identities and 17 exact after identities. Approved changes are limited to port surface selection/catalogue/observation, Session recording/replay settings, backend input/publication, synthetic PlaneContact material selection, tool UI/bridge presentation, and the diagnostic-only Wheel literal below. No other production function changed. The final UI pins were held until the integrating owner reported a successful Godot compile, the driving_asset surface/restore scenario and 31 real-window checks. Those host checks do not establish original-game numerical equivalence.

`py -3 reverse/scripts/audit_task0044_migration.py` passes both the frozen TASK-0044 stage and the strict TASK-0045 successor stage with no unreviewed functions or mismatched permissions. The private current-chain report is `local/task0045/final-source-review-check.json`.

`py -3 -m unittest discover -s reverse/tests -p 'test_task004[345]*.py'`: all 42 tests passed (TASK-0043 identifier/preservation, TASK-0044 migration and TASK-0045 successor reviews). No native build or generator write was performed by this worker.


First explicitly reviewed amendment: `Wheel::FinalizeGroundForce` changes only the diagnostic address literal from Retail001A52EC to Debug001A2AC4 (E-0045-T02, `tire-audit.md`). Full before/after body-token comparison confirms no other token changed. The original TASK-0044 pin remains present in its immutable predecessor review.

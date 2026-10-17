# TASK-0044 source change review policy

The TASK-0043 preservation claim remains bound to commits 19a8de9 through 8c7efde and its original rename/removal permissions. TASK-0044 intentionally changes driving behavior authority from Retail to Debug NFSHP2.ELF SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b. A Debug reconstruction is not token preservation and is not evidence that the previous Retail implementation was equivalent.

`source-migration-review.json` explicitly pins the intentionally replaced before bodies and reconstructed after bodies against baseline8c7efde. Each row identifies path, declarator, signature-token SHA256, body-token SHA256, evidence and reason. The checker removes only exact one-to-one permissions and applies the existing multiset preservation check to everything else. It rejects missing replacements, additional unreviewed algorithms, altered baseline hashes, signature changes, duplicate permissions and missing evidence/reasons. The checker has no refresh/write/accept-current option. A later algorithm edit requires an explicit updated review; merely running the checker cannot accept it.

`projection-retirements.json` separately records all 151 historical TASK-0043 projection signature bindings from the fixed8c7efde tree. Those signatures remain inspectable historical evidence. Historical partition tests validate the old token hashes against that tree. Current replacements are checked by the TASK-0044 before/after review and by behavioral tests, rather than being falsely called unchanged projections. This also covers deliberate retirement of the earlier composite force solver and its throwing dependency shims.

`reverse/classes/task0044-debug-canonical-owners.json` identifies 14 Debug canonical ObjectView owners. The existing generator retains their Retail method attributions and record links in the historical inventory, with explicit superseded coverage. It does not regenerate their obsolete void/no-argument stubs, does not generate default-construction tests for them, and does not bind their Debug implementations to Retail addresses. The registry's full Debug method-address lists establish declaring-owner inventory only; they do not claim every listed method is implemented. Port binding inheritance is explicitly distinct from original PS2 ABI.

The source checks are lexical integrity/review controls. They establish neither instruction-level numerical equivalence nor correct original behavior. Per-function source/listing evidence and original synchronized-runtime comparison remain separate requirements. No status is promoted to Verified by these checks.


## Recorded source-check validation

Python 3.14.4:

- `py -3 reverse/scripts/generate_class_templates.py --check`: passed, 372 historical classes/1203 member attributions/23 allocation labels. Debug owner supersession is explicit; no Debug body is credited to a Retail signature.
- `py -3 -m unittest discover -s reverse/tests -p test_class_templates.py`:10 passed.
- `py -3 -m unittest discover -s reverse/tests -p test_projection_partitions.py`:8 passed, preserving the 151 historical partition signatures and 12 helper chunks at fixed8c7efde.
- `py -3 -m unittest discover -s reverse/tests -p test_task0044_migration.py`:12 passed, including changed-body/signature/baseline/missing-replacement rejection, canonical-owner opt-out and same-address cross-binary symbol collision cases, historical member-coverage supersession and strict unmigrated-entry checking.
- `py -3 -m unittest discover -s reverse/tests -p test_task0043_preservation.py`:11 passed.
- `py -3 reverse/scripts/audit_task0043_preservation.py --renames reverse/naming/task0043-identifier-renames.json`: passed both historical 19a8de9-to8c7efde preservation and the current 249-before/383-after reviewed migration. No unreviewed production body remained. The private full report is `local/task0044/historical-and-migration-check-final.json`.
- `git diff --check`: passed at this checkpoint.

Later reviewed amendments are restricted to documented Debug diagnostic instruction anchors and the homogeneous transform addend 00317B20. Their exact new hashes are checked; the four prior reviewed hashes remain as amendment metadata. These source checks do not promote behavioral verification status.

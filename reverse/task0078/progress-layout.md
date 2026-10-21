# TASK-0078 progress layout and inventory provenance

## Confirmed port-side findings

The user requests only a large progress headline and default-collapsed CLASS and
Functions lists, with functions grouped by explicit class. The generator now owns
the entire dashboard; the former mixed manual/generated page is retained verbatim
in docs/PROGRESS_ARCHIVE_TASK0078.md. Logs and task continuation remain outside
the dashboard. All details elements, including nested groups, omit open.

The headline uses existing conservative evidenced-body classification over the
full Retail inventory. Class percentages use explicitly bound methods only.
Shared addresses count once globally and remain in each registered owner group.
Record status, source classification and unknown totals are not conflated. No
original implementation or verification status is promoted by the new layout.

## E-0078-I01 - Complete Debug address inventory

The retained TASK0073 local/progress/NFSHP2.ELF.json reports complete=True and
binary_id=sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Its 10069 entry addresses are unique. The earlier TASK0041 named export has only
8715 rows; using it alone would omit unnamed functions. TASK0078 preserves only
sorted addresses in reverse/naming/task0078-debug-function-inventory.csv and
source digest/identity method/count in the adjacent JSON. No new name, class
ownership, decompilation, original behavior or function equivalence is asserted.

The source catalog was collected read-only in TASK0073. Its identity method binds
the configured original import file by SHA, not a certification of Ghidra database
memory against the file. The inventory remains metadata, not runtime proof or a
change of the Retail target. Missing original names display Sub_<ADDR>; existing
TASK0041 symbol names are retained verbatim. No new live Ghidra call was required.

## Hypotheses and unresolved questions

No new behavior hypothesis. Source scanning remains conservative and does not
preprocess C++ or prove transitive completeness. Outside-inventory registered
addresses remain visible, flagged, and excluded from the headline denominator.

## Validation

See TASK-0078 for exact commands/results. No graphical test, native game build,
original ELF change or proprietary asset write is needed for this tooling change.

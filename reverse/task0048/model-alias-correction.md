# E-0048-D05 - Original BaseModelName and shared geometry correction

## Confirmed defect and evidence

The initial TASK-0048 port passed the selected911TURBO tests because code and base model
are equal there. StaticCollisions::configure incorrectly selected `<Config.code>_CV`,
and the bridge initially assembled `CARS/<code>/GEOMETRY.BIN`. Neither assumption is
valid for all original catalogue rows.

Read-only Ghidra Debug CarCollisionBody constructor001D4A80 explicitly builds its volume
name from `car->pCarTypeInfo->BaseModelName` plus `_CV`. Imported CarTypeInfo declares
BaseModelName as char[32] at0x20. Raw constructor capture is ignored at
`local/task0048/driving/001d4a80-alias.json`. Existing read_car_render_config independently
reads prefix from row+20; port::vehicle::Catalog retains that prefix and the original
resource text at row+60. No suffix-removal heuristic is required or permitted.

The configured original64-row table has21 code/base-model aliases, including
SPIDERHP -> CHALLENGE and911TURBOHP ->911TURBO. All21 are HP aliases; no literal FHP code
was present in this bounded table scan. This is an asset observation, not a statement
about other releases. The code fix uses the field rather than any alias-name pattern.

## Correction and interface

`base_model_name(Config)` reads only raw bytes20..3f, rejecting empty or unterminated
data. StaticCollisions::configure now accepts Config itself, preventing accidental
display-code selection at its public host seam. NativeBackend passes the selected
configuration. The bridge resolves the selected available catalogue row and reads its
validated resource_path, retaining the existing explicit BIN adapter.

The original Rect fallback remains valid when the **correct base-model volume** is
actually absent; it is no longer accidentally reached because an alias hash is wrong.
No original physics algorithm, coefficient or scene-spawn rule changes.

## Tests and immutable review

static_collision_tests now counts all21 actual catalogue aliases, explicitly reports
unavailable geometry, and checks every available alias. It compares each native prefix
with the original catalogue, selects the actual base-model CV, and
compares every CG-adjusted vertex plus edge/face counts. These checks would reject the
old silent Rect fallback. Unterminated BaseModelName is also rejected. Existing native
wall/corner/rollback tests remain in place. Syntax checks passed; the root performs the
integrated build/run after this correction.

The initial source-review.json remains byte-for-byte unchanged apart from permitted
LF normalization, pinned SHA-256
`d156e2a9a349870ad8a4b2ccf802fb87c91b66cc23c635c32cc52f9d8384a1a0`.
The independent source-review-alias.json pins only three replaced identities and four
corrected/added identities. The audit derives the exact initially reviewed state from
the original frozen baseline and then applies this explicit D05 successor; it does not
refresh any baseline or accept current source. Negative tests protect the initial review
fingerprint and amendment predecessor as well as the existing identity checks.

This is a confirmed port bug correction with bounded native acceptance tests, not
original runtime verification. No entry is marked Verified.

Imported CarTypeInfo raw response SHA-256: `7835111277f7c9499533f77f8ecd8c7e480e9ada53e679121ef278ed71cc9279`
(private `local/task0048/driving/type-CarTypeInfo-alias.json`).


Availability correction: the current configured ZZDATA lacks
`CARS/DIABLO/GEOMETRY.BIN`, so DIABLOHP is one of21 declared aliases but is correctly
unavailable in the catalogue. The regression separately counts declared/tested/unavailable
rows rather than falsely requiring21 decoded volumes. Twenty available aliases reached
the exact CV/CG comparisons before the initial count-only assertion exposed this test bug.
The test now reports this missing resource and requires at least two available shared-model
aliases; no production fallback or availability policy was changed. Syntax check passed.

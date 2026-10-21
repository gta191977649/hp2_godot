# TASK-0081 exact lifecycle source successor

E-0081-L01 permits exactly two existing function replacements:
NativeBackend::initialize and NativeBackend::advance. It also permits exactly
five new function identities: port-side begin_movement_state and
finish_movement_state slices, partial Car::BeginMovement/FinishMovement entries
with explicit untranslated-path throws, and the recovered CarState::ResetCar leaf.
See [lifecycle evidence and boundaries](lifecycle.md). These intentional behavior
changes are not a preservation or original-equivalence claim for those bodies.

Debug authority is NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
build date/revision remains independently unknown. No reset-to-spawn substitute
or blanket driving implementation credit is permitted.

TASK0079's immutable review derives the baseline identities and inherited
auxiliary hashes. Its default live check delegates to this successor; explicit
historical identity/auxiliary snapshots still run the original TASK0079 rules.
TASK0079's tests retain their frozen historical scope; TASK0081 tests own the live
state and reject unrelated functions, changed signatures/stub classifications,
scope widening, predecessor changes and auxiliary mutations.

Only the predecessor dispatcher/test and named Car header/backend test pins may
change. The new frame header, player test, lifecycle evidence, successor checker,
negative tests and this note receive explicit whole-file pins. Frozen historical
review JSON is never rewritten. Every other inherited identity/auxiliary pin stays
active.

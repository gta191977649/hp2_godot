# E-0048-R01 - Explicit TASK-0048 source amendment

This is an intentional reconstruction and port integration review, not a claim that
TASK-0048 preserved every earlier body or established original numerical equivalence.
The root requested a separate successor review. Existing TASK-0044/0045/0046 baseline
inventories and review pins are unchanged. The checker has no refresh/accept-current mode.

## Frozen predecessor

No pre-TASK-0048 snapshot taken after implementation is used as historical truth.
The baseline is derived deterministically from the immutable TASK-0046 pre-source
inventory: remove each exact approved TASK-0046 before identity, add its exact after
identity, then sort by path/name/signature/body hashes. The resulting 2,424 identities
pass the entire existing predecessor review chain before being frozen.

Frozen inventory SHA-256:
`0c782ccd13ae38018406c7ad9cdb9388a2c8dc3dbbb3a632dbe3721987587dc7`.
TASK-0046 review normalized-LF SHA-256:
`c93f5f8eedf077e0205c55eafcff6da98cdb1381b207539fbaa3896f885ebeab`.
These independent constants are checked in audit_task0048_review.py. Prior auditors
accept the explicitly frozen identity list when called by their successor; their
historical documents and matching policy are not rewritten.

## Reviewed changes

Every changed/added recognized production function is pinned by full path, name,
signature-token hash and body-token hash in source-review.json. Removing any approval,
changing a signature/body, moving a function or adding an unreviewed stub fails. Tests
remain outside the production identity multiset under the pre-existing policy.

- **Asset and rendering boundary (E-0048-A01..A06):** original catalogue/resource routing,
  COMP decoding, native geometry/scenery ownership, GS texture-state/mip descriptors,
  original facade matrix and world light volume lookup. The existing parse_header body
  adds original BaseImageSize+44 and per-level ImageInfos swizzle fields needed by the
  confirmed texture producer; it does not invent host mip generation. See assets.md
  and render-review.md. Original algorithms remain in their corresponding owners;
  Godot resource upload/batching remains a port adapter.
- **Topology and collision geometry (E-0048-T01..T06):** typed overloads restore packed
  polygons, original tree lookup/elevation, segment/cylinder predicates, original car
  collision-volume parsing, wafer/Voronoi features, original dynamic backup/interpolation
  and World collision ordering. Existing unrelated legacy throwing overloads remain
  present. See topology-evidence.md, collision-geometry-evidence.md and
  static-collision-evidence.md; host storage has no original ABI claim.
- **Driving integration (E-0048-D01..D04):** explicit 33-terrain semantic mapping, original
  marker/path/spline spawn and quantized five-probe Car placement, real per-wheel/skid
  contact, full material publication, original contact time/crowbar/Resolve and scaled
  impulse. NativeBackend's port fixture now runs the selected immutable environment and
  transactional static collision state. Synthetic input/control/replay remains available.
  No second tire, grip, acceleration or collision response model was added. See
  driving-integration.md and collision-impulse.md.
- **Tool UI and host lifecycle (E-0048-R01):** requested catalogue selection/loading,
  native real-track drive entry/return, identity/terrain diagnostics, original render
  resource upload and thin native/Godot bindings. Boundaries retain existing exception
  containment. Repeated opaque scenery may be batched without changing material math.
  These are port behavior and ownership decisions, not recovered original frontend UI.
- **Read-only surface observation (E-0048-H02):** observe_grip and the host/UI expose
  stored Wheel+130 contact point and Wheel+140 plane after the completed native step.
  Height is the stored point Z; the plane's fourth coefficient is not relabelled as
  height. No face ID, new topology query or grip coefficient is fabricated.

The review deliberately includes port observer/helper functions as well as canonical
recovered functions. It is therefore stricter than merely checking that old physics
functions have not disappeared. Scope claims remain bounded: one playerless native car
against static track polygons; excluded water, dynamic objects and non-null damage/
animated/player/police effects must still report their missing dependencies.

## Validation and certification

Negative review tests cover every identity dimension, mandatory evidence/reason fields,
missing baseline/review, predecessor fingerprint mutation and frozen inventory mutation.
`audit_task0048_review.py` checks original historical preservation through all successor
amendments and then compares its exact reviewed source identities with current code.
The root active task records native/build/host/window test commands and results.

Final review checkpoint: **17 before / 156 after** production identities, all predecessor
amendments and the TASK-0048 amendment passed `py -3 reverse/scripts/audit_task0048_review.py`.
Python negative suites passed: TASK-0048 six tests, TASK-0046 four tests and TASK-0045
thirteen tests (23 total). `git diff --check` passed. Later source changes require an
explicit additional reviewed identity amendment; this checkpoint is not auto-refreshed.

Passing these checks proves reviewed source continuity and port acceptance only.
No entry is promoted to Verified without a scoped original-game comparison.


## D05 scoped successor

The initial review above remains unchanged. A later confirmed BaseModelName/resource-path
alias defect is covered by [model-alias-correction.md](model-alias-correction.md) and the
independent source-review-alias.json (**3 before / 4 after**). The checker pins the initial
review's normalized-LF SHA-256 and validates both stages separately. Final strict suites
passed eight TASK0048, four TASK0046 and thirteen TASK0045 tests (25 total), plus the full
source audit and diff whitespace check. Root-controlled native regression counts all21
actual shipped aliases and verifies the available geometry (20 in the configured tree;
DIABLOHP is explicitly unavailable); its runtime result is recorded by the root task.

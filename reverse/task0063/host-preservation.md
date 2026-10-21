# TASK-0063 host rendering preservation successor

## E0063H01: explicit user-authorized port optimization

The user requested an overhead analysis with original-game reverse evidence and Godot rendering optimizations, then authorized implementation of the plan. This successor reviews only the resulting port-side transport, measurements and cache candidates. It also closes the previously unreviewed TASK-0062 request to add a default-enabled Render props UI checkbox hiding trees and props. These are host controls, not recovered game behavior.

TASK-0061 review and scope are frozen predecessors. Their LF-normalized hashes are pinned in the successor checker; no historical hash or body identity is refreshed. The baseline is derived from the complete approved historical chain without consulting current source. The source-review identities pin the exact observed before and after bodies, including signatures and throwing-stub classification. A changed body outside those slots fails even if its file is otherwise eligible. Recovered translation-unit bodies and physics/driving/skids remain immutable.

The reviewed changes are:

- Main Godot renderer: startup performance controls, diagnostic timing/state, props visibility presentation and resource/setter reuse. The main-view native selection, alpha keys and original packet order remain the authority.
- Postprocess host: independently selected texture parameter cache and consumer/target diagnostics. Full encoded-depth geometry capture and history ping-pong remain. No fused DOF arithmetic or recovered fog/focus function is changed.
- Particle host: independently selected transport cache, unchanged-value material setters, layout query caching per mesh surface, omission of unused CPU indices in the mesh-reuse candidate, diagnostics and raw-buffer test fixture extension. Constructor bodies may appear as `impl_` in the legacy source inventory parser; these are initializer-list constructor owners, not recovered original functions.
- Particle port allocation adapter: mutable-only transaction staging, with unchanged recovered TriggerEmitter/DoTimeStep calls/order and a retained whole-state reference branch. The recovered src/acidfxe.cpp body is not approved for change.
- Sun host: port readback candidate/retained reference path, resource ownership, counters and scheduled viewport diagnostics. Original sun/occlusion math is not approved for modification.
- TASK-0062 host props controls: viewer UI/snapshot, shared presentation settings and final props-visibility adapter. These do not reconstruct or alter original visibility selection.

These scope reasons authorize port optimization only. Source identity checks do not prove runtime parity, measured performance or original-console equivalence. GPU colour/raw-depth checks, native particle-state tests and repeatable real-window benchmarks are independent required evidence. Default selection of experimental candidates must follow those results.

## Staged DOF and bypass limitations

No staged DOF or fog-only history bypass is included. A CPU arithmetic counterexample establishes that an RGBA8 half target can quantize fractional interpolated focus alpha earlier than the current fused shader: interpolate .49 and 1.49 at weight .02; rounding the final fused value yields byte 1, while rounding each half value first yields byte 0. This does not establish original GS raster behavior, but rejects assuming exact host parity from an extra RGBA8 stage. History availability/current scratch-alpha transitions and external Sun/particle depth consumers also preclude an unproven compositor bypass.

## Inventory coverage

The inherited inventory checks recognized C++ function signatures, bodies and stub flags. It is not a complete semantic verifier: shader strings, class data members, free data and non-C++ scripts require ordinary source review and runtime checks. This successor does not weaken that coverage or license changes outside the exact listed identities.

## E0063O01: explicit experimental opaque-queue extension

The root implementation deliberately adds `eligible_track_opaque_queue` in the host-only `track_command_batches.hpp` and switches only the main renderer's existing experimental opaque shader flag to this predicate. Copy blend, depth writing, GEQUAL depth test and alpha-test ALWAYS or GREATER are required. Alpha-sort, environment pass, alpha rewrite, second pass, matrix packing and additive blending remain excluded. The original material alpha-test fragment discard is retained. The opaque-queue option remains experimental/off pending exact colour/native-depth GPU parity.

The separate `eligible_track_opaque_geometry` occluder predicate still requires alpha-test ALWAYS (`alpha_test == 1`). It is not broadened to GREATER cutouts, because their holes cannot be represented by solid occluders. Neither this evidence nor the successor permits changing that predicate or recovered rendering/physics bodies.

The native host fixture checks Copy cutout acceptance and rejection of SourceOver, alpha-sort, environment, no depth write, matrix pack and unsupported alpha/depth tests. The inherited identity inventory excludes src/tests; the exact reviewed fixture therefore has its LF-normalized file fingerprint pinned separately in auxiliary_sources. The helper and changed main submission function have exact signature/body identities in the ordinary review. This evidence describes inspected code and scope, not executed tests or GPU equivalence.

## E0063D01: bounded-parity host cache defaults

The final user instruction stopped long performance testing and limited completion to static code tests and draw-call statistics. Based on the root's completed 180-step exact particle-state comparisons, raw mesh-buffer comparisons and 15 image-parity cases, the root enabled the host particle transport cache, mutable particle-state staging and postprocess consumer cache by default. Each still has an independent startup false override retaining its reference path. Sun readback and opaque-queue experiments remain off.

This selection removes statically identifiable redundant CPU work while retaining bounded tested semantics; it is not an FPS-based promotion, a long-run performance acceptance result or a claim of original-console equivalence. Class data-member initializers are outside the inherited lexical function inventory. The two exact host source files therefore have independent LF-normalized full-file hashes under auxiliary_sources, with exact scope entries, to cover these defaults and other non-function content. No historical input, recovered function identity or frozen predecessor hash is updated by this amendment.

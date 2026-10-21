# TASK0052 CPU frame preparation

## E0052D06: equivalent command transport and lookup acceleration

The original owner entry points and their arithmetic/candidate order remain intact. `eDisplayFrame` sorts indices into the immutable generated command array rather than moving complete matrix/light/effect payloads repeatedly through `stable_sort`. Its comparison, stable ties, texture-chain history, image-parent sibling ordering and alpha radix projection are unchanged. The final ordered array copies each selected payload once. No host render queue exclusion is introduced.

`ScenerySectionHeader::TreeCull` uses a fixed64-slot host array for the original64-entry LIFO. Initial root, child insertion order, overflow point, traversal counter and rejection rules are unchanged. `GrandSceneryCullInfo::WhatSectionsShouldWeDraw` and `CullView` use the already-owned ResourceIndex sections map when supplied; missing-index guards validate the immutable source identity. The no-index path retains the linear source lookup and remains available as a test oracle.

Lighting already occurs once in `StuffScenery` before surface expansion. It is deliberately unchanged: no scene-address cache, mutable shared cache or approximate light query was added. `Runtime::prepare_frame` still commits zone and texture-chain ordering only after successful original frame preparation.

The runtime asset test compares every meaningful DrawCommand field and nested EffectState/TextureState/TestState value plus frame metadata, in output order, across indexed and linear calls over three successive views per track. Float matrices, light contexts and depth use exact equality. This validates acceleration equivalence rather than a visual digest alone. Timing repeats twenty preparations per loaded track and excludes asset loading.

Private validation results are recorded in `local/task0052/runtime-perf-all30.log`; timing configuration uses O2 for current ecstasye/scenery/test translation units and unchanged dependencies from the announced-stable Debug archives. These timings are not a Release GPU frame benchmark and not original-console verification.

Validation completed: all30 catalog entries passed three-view ordered full-command indexed/linear equivalence and twenty-repeat preparation loops (exit0). Track11 measured1.1054ms; median across tracks2.0366ms. Camera-at-world-zero whole-track cases include substantial outliers (Track42:36.4238ms), so this is not a claim that every scene prepares below2ms. Parent-owned integrated Release/GPU profiling remains the performance acceptance authority. Scoped syntax compilation and diff whitespace checks passed.

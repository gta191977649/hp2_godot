# TASK-0076 - Retain track after render gaps and disable costly capture by default

- Status: completed
- Request: track suddenly disappears while driving; optimize/disable expensive vehicle reflections.
- Scope: port-side runtime render-failure lifetime, diagnostic/retry behavior and default dynamic environment capture permission. Recovered physics/selection/particle algorithms remain unchanged.
- Preserve pre-existing dirty tree, including TASK0075 and wireframe changes.

## Acceptance

- [x] Dynamic environment reflection capture defaults off; explicit enabling remains available and restoring defaults keeps capture off.
- [x] A runtime render/visual-readback exception reports a gap without unloading a loaded track or its collision environment; subsequent valid rendering can recover. Actual load failures still clear failed content.
- [x] Native settings, headless/lifecycle and actual Vulkan fault-injection checks pass; source review, progress and continuation updated.

## Findings

The screenshot does not establish the exact throwing site. Source inspection confirms
update_view and complete_visual_readback currently release_display/reset scene and
collision environment on any caught render exception, explaining a mechanism that
leaves the independently owned car visible against an empty world. Original app
logs contain no attributable renderer exception. Do not claim exact field reproduction.

## Outcome and continuation

Default-off capture, render gap retention and normal Debug build delivered.
Native/settings, lifecycle/headless, Vulkan capture and source review passed.
See reverse/task0076/source-review.md for exact commands and remaining limits.
No original behavior changes/Verified claims. Related commits: none.
If the problem returns, continue TASK-0076 using the new render gap warning and
render_error/readback_error; the screenshot's precise original exception is unknown.


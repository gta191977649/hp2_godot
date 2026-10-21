# TASK-0056 - Original scenery resources with Godot HLOD selection

- Status: deferred
- Scope: user-authorized host LOD replacement; default original retained. One shared renderer for viewer, drive and race. No proprietary material in Git.
- Authority: original resources and existing Retail evidence describe input only. Godot HLOD selection is explicitly port-side, not recovered behavior.

## Acceptance

- [ ] Immutable static LOD descriptors, distance intervals and original section selection without native per-instance LOD selection.
- [ ] Persistent independent HLOD levels, common anchor, matching depth/extra passes, separate capture views; no cross-anchor batching or MultiMesh.
- [ ] Load-time API/CLI validation, diagnostics, isolated launchers and lifecycle cleanup.
- [ ] All30 asset checks; interval and GPU boundary/FOV/view tests; visual and actual-driving regressions.
- [ ] Debug/Release builds, performance comparison and window inspection; report regressions honestly.
- [ ] Source preservation review, architecture/evidence/usage/progress updates. No original Verified claim.

## Initial evidence and decisions

Read AGENT.md, task55 completion, task53 followups and current shared renderer/runtime.
E-0051-S01/S02: three original model slots, signed thresholds, radius+4; not all objects have three models.
Godot4.4.1 renderer_scene_cull.cpp uses transformed AABB center for range distance
(lines1545,1918,2828), rather than the simplified documentation's origin wording.
All levels/passes must therefore have custom bounds centered at the same source instance anchor.
Range endpoints use inclusive engine comparisons; adjacent begin uses nextafter(end,+infinity).
No alpha fades/margins. No new mesh simplification, material sharing or native physics change.

## Continuation

Implement port-only descriptor/interval tests, runtime alternate frame assembly,
shared-host persistent range configuration and proxy propagation. Preserve default55 binaries.

## Implemented and bounded validation

- Shared core static descriptors/intervals and original section/tree leaf order;
  all levels submitted, no original per-instance LOD selection in godot_hlod mode.
- Main-level resources warmed during load, persistent capture pools on first use,
  common range centers and matching depth/extra-pass ranges; FOV return-cache fix.
- Load-time APIs configure_lod_backend/get_lod_state/get_lod_ranges and CLI; default
  original, experimental instancing/occlusion/opaque conflicts rejected atomically.
- Necessary dependency E0056T01: added original eSolid::FixTextureTable0010BDB8 and
  free GetTextureInfo001ABF48 bounded resident/default lookup. Debug named pairs
  0010C1B0/001A95C8, exact authorized ELF bytes compared. Original default pixels,
  no invented replacement.4 records/symbol rows; no prior recovered body changed.
- Initial all30 host scan exposed61..66 missing ff91e772; corrected loader applies
  original19006835 default rule. Final all30 host checks pass, nodes return to6;
  missing model references remain explicit existing gaps with no lower-level fallback.
- Native unit compares selected original near/far level's mesh/matrix/light/material
  to its entry in the all-level stream, accounting only for engine clipping bit200.
- Debug107/107 non-GPU and Release101/101 PASS after texture closure. Four final
  targeted unit/source-preservation/progress checks PASS after prewarm-clock fix.
- Actual engine near/far/near, real Track11 camera/FOV/depth proxies, cancel/clear/
  invalid/reload PASS; seven independent driving/capture views, native controls,
  restart/frontend/re-entry PASS. Viewer/Test drive and mask/pipeline image checks
  PASS; final-clock common-renderer GPU comparison PASS. Screenshots under local/task0056.
- Source review11before/19after PASS (2 added bounded recovered functions, remaining
  deltas port-side); task55 approval/scope pinned, native physics immutable.

## Final source freeze / exact continuation

Debug DLL d4c07ce0f6721e336222ed0e2ef427f2b9920a916688a0de0f50f51e51f5423a;
Release08060227aa4e189ba8fdd281a098a0478d6631b1119dbe560e811573d72059c0.
Ignored local/task0056/delivery snapshots contain these binaries and workspace
resource junctions. Default original; no commit/staging. Final code is frozen.

Running root exec74553: local/task0056/final-regression-performance.ps1 performs
final original-mode14-GPU regression, then exclusive Release1080p fresh-process
original vs godot_hlod measurements. Fixed routes11/21/42 and real native driving11,
60warm/300samples,2 fresh repeats each. Output local/task0056/performance/<backend>/
<fixed|driving>/track-<id>-repeat-<n>/{result.json,run.log,captures}.
Do not launch builds/tests/other render windows during measured runs. After finish,
compare wall median/P95/P99, CPU, actual engine draw/primitive/memory statistics,
loading stalls and exact pose/input/effect permissions. No native packet identity
is expected across different LOD backends. GPU timers may be unavailable. This is
selected-route experimentation, not an all30 sustained60FPS acceptance gate.

Then finalize results/docs/BUILD-INFO, restore normal GPU-test opt-in OFF, regenerate
progress and inspect diff/restricted material. Original remains default even if some
routes improve; report regressions and existing original recovery gaps honestly.

## User-requested Show LOD debug increment

Port-only TrackViewer checkbox, default off. Both backends submit all available
original levels when enabled, without HLOD distance filtering. Original section
selection and engine frustum culling remain. All levels overlap intentionally.
Disabled during driving; remembered within the viewer session. No physics change.
Prior benchmark process is no longer running; preserve its partial results.
Acceptance: UI disabled/on/off test, Debug/Release build, host toggle/range smoke.

Show LOD validation: Debug/Release build PASS; targeted4/4 CTest PASS;
actual GPU original/HLOD off/on/off/on and clear PASS (show-lod-smoke.log).
Source delta review updated explicitly:13 before/22 after, preserved native bodies.
Prior final-source hashes above are superseded by this user-requested increment.

Full TrackViewer entry smoke PASS: TRACK_VIEWER_SMOKE_OK and driving projection
check (local/task0056/show-lod-viewer.log). User narrowed further validation to
one representative track; no new all30 sweep for this increment.

User LOD coverage check: Track11 only, native --lod-inventory probe PASS.
E0056H02 records6922 single-reference/1483 two-reference non-sky instances; all
24 multi-reference tuples are vegetation-named. No runtime/render behavior changed.

## Superseded by user direction

TASK0058 removes Godot-managed LOD. Remaining experiment acceptance/benchmark
work is cancelled by that direction, not passed. Frozen source review remains
provenance for the successor. No further HLOD rollout without a new request.

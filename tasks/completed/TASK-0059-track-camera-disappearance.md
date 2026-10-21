# TASK-0059 - Track disappears after rapid/distant camera movement

- Status: completed
- Scope: reproduce Track11 camera movement, distinguish empty native selection,
  host cached visibility, postprocess ownership and destructive exception handling.
  Fix demonstrated root cause, preserve original culling/physics and user edits.
- Acceptance: reproduce with diagnostics; independent regression; near/far/return
  and rapid movement in one track; Debug/Release build and actual delivery check;
  evidence/source preservation review if native/adapter bodies change; docs.
- No all-track sweep or FPS target. No silent recovered fallback or Verified claim.

## Confirmed cause / implemented correction

Reproduced only Track11 as requested, after user clarified simultaneous motion and
rotation. DUSTA_CS high-alpha guard escaped host update and released all scenery;
UI then stopped updating because ok=false. Atlas-wide alpha max also falsely
rejected an active animation cell. E0059P01 records raw and corrected witnesses.

Implemented per-cell sampler footprint bound/cache and explicit per-quad recovery
boundary. Original NotImplemented throw remains; unsupported quads are counted,
reported and omitted. Supported packets/scenery continue, alpha/stream/node ranks
compact together. This fixes total disappearance, not the untranslated high-alpha
GS compositor. Native simulation/RNG/physics/culling and user effects settings are
unchanged. Reset Camera returns current track start instead of world origin.

## Validation

Debug/Release native builds PASS (GCC13.2/CMake3.29.2/Godot4.4.1).
Pure sampler bounds unit and targeted CTest6/6 PASS: particle_alpha_bounds,
debug_console, track_runtime, track_environment, track_postprocess,
particle_boundary_source_review_rules. Strict successor audit3before/4after PASS.
Actual Debug delivery reproducer now passes16motion/rotation frames including
3gap frames, supported particles remain visible, then Reset restores track view.
Release delivery native/camera-motion/gap/clear/reload PASS, gap exercised1frame.
Both delivery folders stage all8runtime DLLs; extension SHA pins and per-flavor
runtime manifests in ignored local/task0059/delivery. Prior58shortcuts forward59.
Inspected fixed-gap.png: original scenery still visible with explicit warning.
No all30 sweep, original Verified or FPS claim. No commit/staging.

Initial test-only failures retained: Reset tested through wrong direct UI handler
then standalone SceneTree couldn't resolve DebugTools compile identifier; corrected
to root autoload request. First Release render passed but screenshot path assumed
workspace layout; fixed user://default/--native-capture-dir and reran cleanly.
Logs local/task0059/{zone-probe,diagnostic-probe,rgb-probe,motion-fixed-v3,
delivery-debug-motion,delivery-release-motion-final,targeted-final-tests}.log.

Continuation: run local/task0059/delivery/Launch TrackViewer Native.cmd with old
windows closed. Future correct high-alpha GS compositing belongs to existing
TASK0053 effects gaps; don't present isolated omissions as full particle recovery.

# TASK-0046 - Player transmission controls and skid marks

- Status: completed
- Certification: original Debug numerical comparison blocked; not Verified.
- Related: TASK-0038, TASK-0044, TASK-0045.
- Target: Debug NFSHP2.ELF SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
- Preserve the existing uncommitted TASK-0044/0045 changes and frozen reviews.
- Scope: original RealDriver/DriveTrain input and transmission closure, original skid
  generation/rendering closure, centralized Godot driving actions and read-only HUD.
- Evidence: read-only Ghidra and configured original assets only; no Verified claim
  without a bounded original runtime comparison.

## Acceptance

- [x] Original-class initialization, Drive, manual/automatic shift, neutral, reverse,
  handbrake and required player-physics dependencies recovered and tested.
- [x] Godot InputMap plus one semantic adapter; WASD, Q down/E up, Space handbrake,
  automatic/manual and manual neutral GUI; edge events consumed once and cleared safely.
- [x] HUD reads actual gear, engine RPM, speed and handbrake; no inferred physics.
- [x] Wheel/SkidMaker/SkidSet/SkidSegment/RenderSkids restored with original material,
  geometry, segment/pool limits, lifetime, visibility and render-state evidence.
- [x] Godot renders original commands and original textures; no substitute stripe algorithm.
- [x] Native/Godot/input-remap/generator/single-chain/source-review checks pass.
- [x] Real-window keyboard, GUI, reverse, handbrake, skid and lifecycle checks recorded.
- [x] Original numerical comparison attempted; blockers and coverage reported separately.
- [x] Evidence, architecture, usage and generated progress updated.

## Work allocation and continuation

REA step 6: controls46 owns original player/native orchestration; skids46 owns original
skid generation/render commands; runtime46 owns host input/bridge/UI. Root owns integration,
append-only review, validation and durable status. No proprietary artifacts enter Git.
Pre-edit production identities are frozen in reverse/task0046/pre-source-identities.json.
Unknown recovered paths must still throw hp2::NotImplemented with the Debug address.

## Findings and implementation

Read PLAYER_DRIVING.md, reverse/task0046/controls-audit.md, physics-player-audit.md,
skid-audit.md and runtime-input-render.md. E-0046-C01/P01/S01..S04/U01 distinguish
original behavior from host services. Twenty-one control queries and32 skid/dependency
entries retain addresses, types/signatures, source attribution and instruction hashes.
Raw code/listings/assets/screenshots remain ignored. The Debug index now has258 unique
addresses (226 Implemented/32 Decompiled); no Verified entries.

One semantic adapter and project InputMap drive original RealDriver, DriveTrain and
PhysicsCar. Default automatic, Q/E shift, manual N, S original brake/reverse, Space
handbrake are usable. Powertrain reads actual completed state; speed remains explicitly
native units because a HUD conversion was not proved. No Run/Pause UI or new function key.
The original REAL_CONTROL burnout dependency is recovered in PhysicsCar::Move.

Skid generation, compressed segment geometry,8/128 limits, material width, two endpoint
elevations, minimum intensity, pixel fade and render-driven LRU are canonical. Debug
draw-context evidence disproved reuse of Retail bWriteZ50: original selectors now supply
depth/alpha state. Independent texture U/V wrap is supported at the host sampler boundary.
Source review explicitly covers25 old identities and81 replacements/additions, after the
unchanged TASK0044/45 pins. New owner registry prevents regenerated noarg duplicates.

## Validation

- `cmake --build --preset native-debug -j4`: passed, all native targets.
- `cmake --build --preset godot-debug -j4`: passed. A late rebuild initially encountered
  a separate project frontend locking ps2_port.dll; closed that exact project runtime
  gracefully and reran successfully. No unrelated process was terminated.
- `ctest --preset native-debug -R '^(driving_player|driving_skids|driving_mover)$' --output-on-failure`:
  final3/3 passed in0.55s. Earlier material/player/skid/backend integration4/4 passed in41.52s.
- `ctest --preset godot-debug --output-on-failure`:75/76 passed in118.25s. Sole failure
  was the historical owner-count assertion14 after adding a separate7-owner successor.
  Updated the test to retain exact14 original owners, require disjoint successor and exact
  union, without weakening provenance checks. Targeted `ctest --preset godot-debug -R
  '^debug_driving_migration_rules$' --output-on-failure` then passed1/1. All76 distinct
  checks passed across full run and correction; not an uninterrupted76/76 run.
- Final full run includes original-class/native driving, Godot asset/input/viewer hosts,
  InputMap W-to-T and E-to-R rebinding, generator, strict source reviews and progress.
- `py -3 tools/test_drive_window.py --controls --surfaces --output local/task0046/window-final`:
  **69 actual OS checks passed**, including real GUI Automatic/Manual/N, actual Q/E,
  S reverse/W forward transition, Space hold/release, original skid mesh commands,
  material changes, mixed-wheel zones, camera, Esc/F8/focus and repeated return/entry.
  Root visually inspected controls-skids.png: original textured marks behind the wheels,
  actual Gear1/RPM1250/HandbrakeON visible. Captures remain ignored.
- Earlier OS attempts exposed harness issues: first popup auto-fit moved item centers;
  wait for stable observed centers. Later screenshot run read the observer JSON between
  writes; retry incomplete reads rather than calling a widget disappeared. Second and
  final complete runs each passed69; no physics behavior changed for these fixes.
- `py -3 reverse/scripts/generate_class_templates.py --write`,
  `py -3 reverse/scripts/complete_class_members.py --write`, source-review chain and
  `git diff --check` passed. `py -3 reverse/scripts/generate_progress.py` and its
  `--check` passed after moving this task to completed; predecessor review hashes
  remain unchanged. New-file restricted-extension inspection found no proprietary artifacts.

Tools: GCC13.2.0 MinGW, CMake3.29.2, Ninja1.12.0, Python3.14.4, Godot4.4.1
49a5bc7b6, pinned godot-cpp/ImGui retained. Real window uses Compatibility/OpenGL,
Intel Arc B390 driver32.0.101.9150. No commit made.

## Limits and reproducible continuation

`py -3 -m tools.oracle probe --output local/task0046/pcsx2-probe.json` returned
blocked/timed out. A runnable synchronized matching Debug scenario remains missing.
Do not reuse the Retail oracle to certify Debug. Continue TASK-0038 with Debug-addressed
Move00137D18 and component entry/return captures under equal initial state/input/time/
contacts, then record errors and coverage before any Verified promotion.

Implemented acceptance covers the supported finite playerless synthetic scene, not a
whole Car/World/Player constructor or original race. One synthetic clan replaces world
spatial routing at an explicit service boundary. Zero-length AddSegment VRSQRT and
unsupported numerical/render domains still throw. Full PS2 DMA/GS, transparency ordering
and raster precision are not certified. Original game replay is distinct from Session
input recording. These limits are preserved in the individual records and usage guide.

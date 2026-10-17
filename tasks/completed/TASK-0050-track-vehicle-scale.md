# TASK-0050 - Track/vehicle scale mismatch investigation and correction

- Status: completed (bounded unit audit and projection correction)
- Request: check visibly mismatched track/car proportions, including Lotus Elise near roadside signs.
- Preserve all existing dirty work. Use original Retail asset/render evidence and explicitly separate Debug vehicle physics. Ghidra read-only.

## Acceptance

- [x] Measure vehicle render dimensions versus original wheel/CV/config units.
- [x] Measure track geometry and scenery transforms versus original topology/path units.
- [x] Identify and correct supported unit/transform defects without empirical scene resizing.
- [x] Record regression tests and actual windows using the reported vehicle/track.
- [x] Preserve historical review pins, update evidence and documentation; no Verified promotion.

## Allocation

REA step6 independent read-only questions: track rendering transform/packet scale; vehicle visual/physics units; topology/path units. Root owns integration/camera checks, any confirmed fix, serialized builds and final acceptance. Do not change scales before establishing source evidence.


## Findings

- E-0050-V01/V02 `reverse/task0050/vehicle-scale.md`: Elise and911TURBO visual bounds,
  original CVs, wheel configuration, actual original car render matrix and unit pose.
- E-0050-R01 `reverse/task0050/render-scale.md`: fresh Retail GetXYZ, signed1/16384
  instance basis, static model draw chain, original sign geometry and scene AABBs.
- E-0050-U01/U02 `reverse/task0050/topology-units.md`: Track11/21 markers, paths and
  topology agree; original outside FOV is a horizontal angle, not a vertical angle.
- E-0050-H01 `reverse/task0050/host-units.md`: all30-track sign bounds and actual host
  scales; both the vehicle and track root are(1,1,1) during real driving.

No supported global geometry scale error was found. Elise body3.83825x1.76197 and
originalCV3.88961x1.80214 agree, as do wheel centers and original direct-float render
accessors. A road vertex matches independent packed topology within0.008874nativeunits.
All1388 inspected ordinary curve signs across30 tracks fit original instance bounds.
These bounded checks do not establish full original-game visual equivalence.

A concrete common display-policy discrepancy was found: the old tool used60 degrees
vertical FOV; Debug DriveCameraMover::Update0015C2B0 assigns OUTSIDE0x32DC, while original
CalculateH001042E8 uses screen width. The projection adapter now uses KEEP_WIDTH and
71.52099609375 horizontal degrees. Save/restore includes aspect policy. No geometry,
physics, scene matrix, alpha path or C++function body changed; TASK0049 audit remains
current with immutable historical pins. Follow distance/spring/wall response remain
tool behavior; the projection fix is not asserted as the sole cause of the report.

## Validation

- Private original-byte/geometry audit across30 bundles: PASS1388 ordinary sign instances,
  zero outward overflow against independent stored original scene-space AABBs.
- Runtime original Elise/Track11 Compatibility window: PASS body/track unit scales,
  native body dimensions, fixed horizontal projection; capture inspected at
  local/task0050/elise-track11-projection-fixed.png.
- Actual engine projection matrices: PASS4:3,16:9 and portrait. Failed entry leaves
  previous projection unchanged; synthetic and real-track driving restore FOV, aspect
  and projection onreturn. Headless driving and track lifecycle smoke PASS.
- `cmake --build --preset godot-debug -j 6`: PASS (no C++ rebuild required).
- `python reverse/scripts/audit_task0049_review.py`: PASS; no successor required for
  GDScript-only production changes. Full regression result is recorded below.

## Remaining boundary

Original follow-camera pose is not restored, and the exact screenshot object/pose is
unidentified. No matched original/port frame certifies complete proportion parity.
Do not add empirical scale factors to satisfy an unmeasured visual impression. A
future camera recovery must retain original per-car POV records, lag/spring dynamics,
lookback, road noise and wall/elevation constraints rather than only copying Lag.

Final full regression:87/87 PASS,152.07seconds; source/class/progress checks passed. Original complete follow camera and matched-frame parity remain explicitly unrecovered.

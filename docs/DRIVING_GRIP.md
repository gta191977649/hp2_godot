# Grip and material tests (TASK-0045)

The existing CarViewer Test drive uses the one canonical Debug PhysicsCar/Wheel chain.
This task exposes its original terrain inputs and stored wheel diagnostics; it does not
introduce a grip model. Behavior authority remains NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b. Neither host fixtures
nor visual tests establish original-runtime numerical equivalence.

## Use

Load a car in CarViewer and click Test drive. Driving still starts immediately with WASD
and the existing rear/orbit/recenter camera. Esc releases the cursor for the panel; clicking
the world recaptures it. F8/focus suspension and returning to preview retain their prior
behavior. No new keys, Run/Pause buttons or physics tuning controls were added.

The driving panel contains **Surface mode** and **Material**. Uniform defaults to ROAD;
select ROAD, DIRT, GRASS, SAND or SNOW to change all wheel queries on the next successful
simulation step. Position, velocity and gear are not reset. Changing settings while suspended
waits for resumed stepping. The committed snapshot drives both selection display and colors;
failed candidates cannot publish a partial surface or diagnostic update.

Zones uses the confirmed native forward coordinate X (host world Z). The first region extends
to X<25, then boundaries25/50/75/100 select ROAD/DIRT/GRASS/SAND/SNOW respectively. A boundary
belongs to the following region; the first/last extend outward. Every wheel resolves its own
world query point, so front and rear wheels can receive different materials. The plane, its
height/normal, boundaries, colors and labels are explicit tools, not original track topology
or recovered terrain textures. Native descriptor arrays supply the world-anchored shader and
labels; following the vehicle does not move the boundaries.

Expand **Wheel grip (stored values)** for four read-only wheel rows. Horizontal scrolling
keeps the original values accessible without a permanent large overlay. Fields include
contact, terrain name/ID, slipping, ZForce, CurrentSlip, TractionUsage, TractionBoost,
CatchupFrictionMultiplier, road noise, Slip XY, GroundForce XY, TireInfo static/dynamic
coefficients and terrain static/dynamic/rolling multipliers. Values are original stored
scalars, not an invented grip percentage, inferred combined coefficient or recomputed force.
SI units are not established here. Preview floor material and tool labels are restored/removed
on exit, including repeated entry.

## Original data and algorithms

The complete 33-row terrain ledger is [terrain-rows.json](../reverse/task0045/terrain-rows.json).
It preserves original names, enum values, row addresses, original scalar bits and hashes;
full raw captures stay ignored. The first five supported test choices are:

| Original name / ID | Static multiplier | Dynamic multiplier | Rolling multiplier | RoadNoiseScale |
|---|---|---|---|---|
| ROAD / 1 | 1 | 1 | 1 | 0 |
| DIRT / 2 | 0.97 | 0.957875 | 1 | 0.05 |
| GRASS / 6 | 0.96 | 0.948 | 1.05 | 0.05 |
| SAND / 7 | 0.96 | 0.95 | 2 | 0.05 |
| SNOW / 8 | 0.95 | 0.938125 | 1 | 0.01 |

Displayed decimals are summaries of original float32 values; runtime reads the original bits
from the SHA-bound image. Small differences are retained; snow is not assigned an invented
ice coefficient. All33 types are catalogued, but water/walls/objects/car and other deferred
materials are not enabled as ordinary ground in this bounded test.

TerrainTypeInfoTable starts003102E8 with stride0x90; static/dynamic/rolling fields are
+0x20/+0x24/+0x28. Road-noise spacing/scale are +0x18/+0x1C. Original Wheel GetLateralForce
001A2500 uses CorneringForce or the load-sensitive curve family according to the original
switch. GuessGroundForce001A2108 selects static/sliding behavior, FinalizeGroundForce001A26B8
limits force and publishes TractionUsage, and derivative001A1E88 uses rolling resistance.
Suspension load exchange, iterative force resolution, traction/catchup multipliers and yaw
assist remain on that same original chain. New material choices feed the existing original
roughness query; the tool does not generate substitute noise or flatten it.

The raw observer reads after Move/UpdateCar from the candidate Memory used for the successful
pose snapshot. It checks finite publication values but never clamps original fields or
modifies physical state. Original tire coefficients and material multipliers are exposed
separately. Session records each successful frame's surface settings alongside controls/time;
replay uses the recorded setting even under different render pacing.

See [tire audit](../reverse/task0045/tire-audit.json) and [the audit report](../reverse/task0045/tire-audit.md) for
instruction/type/source/boost provenance and bounded unresolved paths. The only recovered-body
change in this pass corrects the exceptional VRSQRT diagnostic address from the historical
Retail address to Debug001A2AC4; no supported tire arithmetic was replaced. The existing road
noise projection's CAR32 bound remains a documented deferred issue outside the five surfaces.

## Validation and limits

Native fixtures cover material friction/rolling/noise, loads, slip threshold equality/nextafter,
both lateral models, curve-family saturation, friction/traction/catchup multipliers, brake
lock/airborne behavior and read-only observation. The surface suite checks boundary sides,
negative coordinates, per-wheel queries, equal-state/input/time material trajectories,
continuous cross-region driving, queued settings, transactional rejection and exact replay
including all diagnostics. Qualitative comparisons do not prescribe a desired amount of slip.

```powershell
cmake --build --preset godot-debug -j6
ctest --preset godot-debug -R 'driving_(tire|surfaces|asset_host|lab_host)$' --output-on-failure
py -3 tools/test_drive_window.py --surfaces --output local/task0045/window
```

The window test uses actual ImGui clicks and Win32 keys in its own foreground-PID-guarded
window. Use an idle desktop. Reports/screenshots remain ignored under local/task0045.
The completed task/log record exact commands, pass counts and any blocked acceptance.

Original Debug comparison was attempted with
`py -3 -m tools.oracle probe --output local/task0045/pcsx2-probe.json`; it returned
blocked/timed out, with no active PCSX2 process/runnable matching Debug driving state.
No emulator/save/settings were changed. Full original track queries, collisions, effects,
water/object behavior and exceptional EE domains remain separate. Existing Retail oracle
addresses cannot certify Debug. Continue certification using exact Debug Move00137D18 and
wheel/component entry/return hooks, equal state/input/time/material/contact, and per-field
errors/coverage. No Verified promotion is justified by the tests in this task.

# TASK-0122 - Donor vehicle physics and collision integration

- Status: active
- Scope: compare the native Windows donor driving and collision subsystems
  with the production MVP World, then integrate compatible missing behavior.
- Dependencies: TASK-0044, TASK-0099, TASK-0120 and TASK-0121.
- Evidence authority: the existing authorized Debug `NFSHP2.ELF`, SHA-256
  `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
  The donor source is a lead for locating and understanding original code.

## Acceptance

- [x] Map donor vehicle and collision ownership to the live MVP call chain.
- [x] Integrate the original fracture path and test child collision lifetime.
- [x] Build the production MVP and run the Championship control harness.
- [ ] Exercise an authored fracture in the production MVP and capture matched
  original and native dynamics and contact traces.
- [ ] Integrate any remaining compatible missing paths discovered in that
  comparison. A complete donor backend replacement has not been performed.

## Source intake

`D:/dev/hp2_ps2_rev` is a Windows source distribution without Git metadata.
Its `README.md` identifies Visual Studio v143, Vulkan, XAudio2 and SDL2.
The package's native C++ types, pointers and allocators serve its own World.
MVP uses Debug logical addresses, virtual descriptors, pools and replay
storage. The production entry remains `WorldSession::step` through the
recovered World. The bridge consumes committed state for presentation.

| Scope | Donor source lead | MVP source owner |
| --- | --- | --- |
| Input and requested gear | `indep/src/realdriver.cpp`, `indep/src/engine.cpp` | `realdriver.cpp`, `engine.cpp`, `WorldSession::apply_input` |
| Chassis and substeps | `indep/src/physicscar.cpp`, `rigidbody.cpp`, `componentphysics.cpp`, `aerodynamics.cpp` | Existing corresponding Debug modules and PhysicsCar vtable dispatch |
| Four wheel contacts | `indep/src/wheel.cpp`, `suspension.cpp` | Existing Debug modules and shared topology contact provider |
| Body collision and response | `indep/src/world.cpp`, `collisionbody.cpp`, `contact.cpp`, `collision.cpp`, `voronoiclip.cpp` | Debug World collision step and existing CollisionBody, Contact and VClip owners |
| Breakable collision response | `indep/src/smokeable.cpp` `collisionBreakableFracture` and `indep/src/world.cpp` | Debug `Smokeable::Break` and `World::DoTimestepCollisions`, now connected |

Donor source file SHA-256 values for the reviewed excerpts:

| File under `indep/src/` | SHA-256 |
| --- | --- |
| `engine.cpp` | `95624c075b8d9c6de66db870186d3867979483f46512df475c65b70cef39bb71` |
| `physicscar.cpp` | `e078003486deb7f17177d13adb8a5af99d6bcb94754d49f5aca46c96e5879195` |
| `smokeable.cpp` | `cdaeae562d9008086afaeFa8e17e20eddc1db06db213d1e3209f1958b36fa228` |
| `world.cpp` | `21e449f4179ff26e10b46f9fbd5c394479dfe62098b8a9fff3a7dc1141161de0` |
| `contact.cpp` | `b0f241f723248ce2a390d1548bd7ababf76a42d793edb821c51997dc3319d243` |

## Confirmed

- E-0122-C01. Debug `0021C050` creates basic Smokeable fragments from the
  parent's resolved child rows, with sequential suffix hashes, the parent
  display transform and a null velocity input. The parent destructor runs
  after all children are awake. Debug World `0014D630` adds the children to
  its original eight-body recheck array. The MVP implementation uses the
  existing factory, placement, wake, destructor and witness paths.
- E-0122-P01. The donor retains previous clutch slip RPM and adjusts
  stiffness after a sign change. Debug `00189AF0` lacks that branch. The
  donor selects a 50/50 split when axle speeds sum to zero. Debug `0018993C`
  uses the EE division result followed by limited-slip comparisons. The
  donor also refreshes angular velocity after orthonormalization, while the
  Debug `00138794–001387F8` call sequence has no corresponding refresh. The
  three donor behaviors were left out of the Debug runtime.

## Open evidence

The donor source marks PAL addresses. Its exact original ELF identity was
not established by the package. The connected Ghidra project contains the
authorized Debug and US Retail programs. Full donor ownership would also
require mapping or replacing car and track construction, participant and
collision registration, replay, resources and game effects. Directly
inserting the donor native types into the Debug memory model would not
preserve either implementation's runtime contracts.

The regression uses synthetic child records with the actual Debug factory,
body slots and WorldObject lifetime. It proves ordered fragment creation,
seeded model hashes, pose, awake status, zero initial linear speed, capacity
failure before creation, parent retirement and cleanup. The normal
Championship smoke does not demonstrate an authored fracture. Original
vehicle and collision trajectories remain unverified.

## Validation and continuation

Windows Release, CMake 3.29.2, Ninja 1.12.0, MinGW-w64 GCC 13.2.0,
Python 3.14.4, Godot 4.7.2 `ed1daf0bf` and godot-cpp
`e4b7c25e721ce3435a029087e3917a30aa73f06b`.

```powershell
cmake --build --preset godot-release --target smokeable_conversion_tests smokeable_moveability_tests shared_collision_foundation_tests driving_drivetrain_tests driving_solver_tests driving_mover_tests event_world_tests --parallel 8
ctest --test-dir build/godot-release --output-on-failure -j 4 -R '^(smokeable_(conversion|moveability)_original|shared_collision_foundation|driving_(drivetrain|solver|mover)|event_world_original)$'
cmake --build --preset godot-mvp-release --target ps2_port --parallel 8
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship --launch-only --controls-smoke --result-timeout 70 --output local/task0122/mvp-controls
py -3 reverse/scripts/generate_progress.py
py -3 reverse/scripts/generate_progress.py --check
```

Seven native tests passed. The production Championship window passed 31
assertions. MVP DLL SHA-256
`dadcb85c6c2ddc764f5ddde9cb50d850737c43ac0bfc25d6c0fcc99d307f3a54`.

Continue TASK-0122 by locating a Track 12 placement with resolved Smokeable
child records and loaded `_CHILDn` models, then exercising a hit through
the MVP World and the hash-matched Debug ELF at `0014D630` and `0021C050`.
Record the parent, returned child IDs, subsequent witness contacts, and
published poses. Use TASK-0121 for the matched player launch and tire trace.
No related commit has been made.

# TASK-0121 - MVP vehicle dynamics fidelity

- Status: active
- Scope: player input, engine and drivetrain, wheels and suspension, rigid body,
  contact and collision in the Godot MVP race World.
- Authority: the existing Debug `NFSHP2.ELF` driving and World authorization.
  Use `nfshp2-port-win` as a Retail address and host integration lead.
- Dependencies: TASK-0044, TASK-0046, TASK-0099, TASK-0118 and TASK-0120.

## Acceptance

- [x] Trace the MVP race from host input to committed vehicle state and record
  the relevant Debug and Retail static evidence.
- [x] Fix the confirmed input and initialization discrepancies, with targeted
  native regressions and the physical-input portion of the MVP window check.
- [x] Pass the complete MVP window harness on the final build.
- [ ] Match original runtime input, gear, RPM, wheel/contact state and chassis
  motion on the same car, track and schedule, including launch, braking,
  steering, handbrake and contact transitions.

## Confirmed

The reference repository `nfshp2-port-win` at
`ce4e85522c9d2b6ea05fe5aab058a4bff87fdc50` generates C++ from the Retail ELF.
Its checked-in `recomp/symbols/retail_symbols.json` supplies comparison leads.
`recomp/runtime/pad.cpp` and `window.cpp` supply host-side PS2 pad bytes.
They do not establish the original pad calibration or physics. The local
Debug and Retail inputs were individually SHA-256 checked.

| Evidence | Finding |
| --- | --- |
| E-0121-I01 | Debug `0013B3B8`, `0013B7B0`, `0013BB48`, `0013BBC8` and `0011D6A8` define the scanner byte selection, inversion and pressure scaling. World previously wrote normalized floats directly. The port now encodes its semantic actions as centered steering and pressure bytes and calls the four recovered callbacks. Godot deadzones and the normalized-to-byte conversion are PC mappings. |
| E-0121-P01 | Debug `00138C80`, stores at `00138D58–00138D88`, restores engine speed, clutch and gears. The port also cleared `DriveTrain+0x4C`, which is the first Debug shift-up table slot. Removed that store and added a regression to the constructor fixture. The first slot is outside the normal automatic forward-gear table lookup, so this finding alone does not explain the reported feel. |
| E-0121-V01 | Debug `00189AF0` clamps engine-to-differential speed difference and applies clutch stiffness without the history branch. Retail `SLUS_203.62` `0018B948` reads/writes `+0x4C` and can halve stiffness after a sign change when both magnitudes exceed 2000. The Debug runtime retains its own algorithm and layout. Retail SHA-256 is `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`. |
| E-0121-W01 | Debug `0014D9A8` and `0014D588` connect World to dynamic collision bodies. `001D4E28` reaches `00131B90` and the mover dispatch. `00137D18` sets steering, engine throttle and desired gear before `00199BB8` runs Euler substeps. `00138D90` runs body, aero and suspension resolution, followed by `00138EC0` with five wheel/clutch force passes. `001A37A0`, `001A1C80`, `001A1D80`, `001A2108`, `001A1E88`, `001A26B8` and `0019B170` cover the inspected suspension, contact, tire and rigid-body nodes. World collision scheduling at `0014D630` is separate from this wheel force loop. `00139110` publishes the car and wheel state read by the MVP presentation. |

Read-only Ghidra MCP operations used the authorized Debug `NFSHP2.ELF`,
SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
The build is user-labelled 2002-07-08. The embedded revision/date remains
unconfirmed. Retail was inspected separately by program name. Listings were
used to resolve decompiler artifacts such as the apparent float-to-integer
conversion in the suspension progression calculation at `001A3A64` and the
skid maxima at `00139388` and `00139394`.

## Hypotheses and open evidence

- Debug/Retail differences and host input quantization can affect feel. Their
  relative impact is unmeasured. Full-scale controls already produced unit
  magnitudes in the old adapter, so the callback change alone may be subtle.
- The existing minimal FreeRun PC preset starts in manual transmission.
  The authored event uses its selected transmission. Match this setting,
  controller bytes, initial state and elapsed-time schedule in any comparison.
- PCSX2 2.8.2 booted the Debug ELF override with the private A1.56 disc. A
  paused PINE capture read Debug code words at `00100008`, `00137D18`,
  `00189AF0` with an empty World pointer. The subsequent running instance
  initialized the DualShock 2 but did not yield a synchronized driving World
  capture during this session. PINE memory reads alone cannot define an exact
  input and step schedule. No original vehicle behavior received Verified status.
- PS2 hardware polling/calibration, exact EE/VU arithmetic and unsupported
  contact paths still need evidence. No handling tune was selected.

## Validation and continuation

Windows Release, CMake 3.29.2, Ninja 1.12.0, MinGW-w64 GCC 13.2.0,
Python 3.14.4 and Godot 4.7.2 `ed1daf0bf`.

```powershell
cmake --build --preset godot-mvp-release --target ps2_port --parallel 10
cmake --build --preset godot-release --target driving_drivetrain_tests driving_solver_tests driving_mover_tests driving_player_tests driving_constructor_tests real_controller_tests event_world_tests minimal_driving_tests car_construction_tests shared_collision_foundation_tests --parallel 8
ctest --test-dir build/godot-release --output-on-failure -j 4 -R '^(driving_(constructor|drivetrain|solver|mover|player)|real_driver_frame_original|event_world_original|minimal_driving_original|car_construction_original|shared_collision_foundation)$'
ctest --test-dir build/godot-release --output-on-failure -R '^(progress_tables|debug_driving_migration_rules|debug_driving_single_chain)$'
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --headless --path build/godot-mvp-release/project --script res://scripts/debug/driving_input_smoke.gd
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship --launch-only --controls-smoke --result-timeout 70 --output local/task0121/mvp-controls-final
py -3 -m tools.oracle capture --port 28012 --address 0x100008 --address 0x137d18 --address 0x189af0 --address 0x3272f0 --output local/task0121/pcsx2-debug-identity.json
```

The Release native set passed ten checks. `progress_tables` and the Godot
headless input smoke passed. The first window run passed 31 control and
lifecycle assertions and exited 1 for two `reflection.gdshaderinc:43`
errors. The final window run exited 0 with 32 assertions, including physical
W, S, Space and Q/E. Final MVP DLL SHA-256 is
`a57db05ae2ca4b4661e874f3e57030bc0d2279781f704877d062dc90df963fb9`.
This task made no shader edits. Two older structural checks failed.
`debug_driving_migration_rules` expects a registry without `tCopManager`,
although the existing generator loads TASK-0107's amendment.
`debug_driving_single_chain` expects one `SkidMaker::MakeSkid` and image
loading in the Godot bridge. HEAD already has two overloads and loads the
image in WorldSession.

Continue TASK-0121 by booting the SHA-matched Debug ELF with the same private
disc and keyboard profile. Enter a matching Track 12 TS50 race, align initial
car, transmission and time schedule, then capture the player at Debug
`PhysicsCar::Move` entry and `UpdateCar` exit. Compare per-step input, gear,
RPM, clutch, suspension height, contact/terrain, wheel speed, and body pose
against the committed native state. Cover a launch, a gear change, a brake to
reverse transition and a contact transition before making a parity claim.
`tools/pcsx2_mcp.py` documents the paused EE and instruction breakpoint
requirements. Use the Debug address map and keep raw captures under `local/`.

No related commit has been made.

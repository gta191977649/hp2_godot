# TASK-0079 - Current driving chain audit

Date: 2026-10-21 Australia/Sydney. Driving authority is Debug NFSHP2.ELF,
SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
User-labelled build date2002-07-08 is not independently confirmed. Retail
addresses and records remain separate. No Verified promotion.

Program-info readback selected NFSHP2.ELF, r5900:LE:32:default/default compiler.
The file at its reported import path was SHA-256 checked against the authority
above (local/task0079/program-info.json and identity.json, ignored). This binds
the import file, not every byte of mutable database memory.

## Confirmed source audit

The requested percentage is a conservative evidence-binding count, not an
execution trace or percentage of working vehicle physics. Engine/DriveTrain
already have many canonical definitions; grouped address comments, overloads
and inline class definitions are not uniquely credited by the current parser.
This task does not loosen that parser or manufacture implementation credit.

| Class / chain | Connected source behavior | Unused or missing boundary |
|---|---|---|
| CarDriver | Inline SetEBrake00136C30 and SetDesiredGear00136C58; RealDriver supplies original control fields consumed by PhysicsCar | Untranslated wrappers are not proof that their field-level behavior is absent. AI target/speed interfaces are outside the playerless tool. |
| RealDriver | NativeBackend calls Drive0013B200 before Move; steering0013B438, pedals/reverse0013B948, shift0013BC90/0013BD70 are implemented | Non-null Player nitro/radial-blur/event dependencies still throw. No nitro bypass added. |
| Engine | ComputeAuxiliaries00188B38, derivative00188B88, total torque00188C60, throttle/braking, nitro timer and state accessors exist in engine.cpp | Original allocation/destruction and DoSnapshot ownership are not used by the Memory fixture. A timer implementation does not establish complete Player nitro support. |
| DriveTrain | Differential, clutch guess/modify/finalize, shift requests, auxiliaries00189648 and derivative00189840 run through PhysicsCar | Original allocation/destruction and replay snapshots are outside fixture ownership. Do not call those merely to raise coverage. |
| CarState | Yaw, current wheel contact/terrain and wheel matrices are already connected | BeginTimestep001BD938 was missing from each successful frame. ResetCarForSound remains an explicit sound-path gap. |

Source scope: native_backend.cpp, realdriver.cpp, physicscar.cpp, engine.cpp,
carstate.cpp and CarDriver.hpp. Historical detailed evidence remains in
reverse/task0044, reverse/task0046 and docs/DRIVING_GAPS.md. This table is a
source audit, not a new whole-entry binary certification.

## E-0079-S01 - Per-frame terrain reset

Live read-only Ghidra decompile/listing001BD938, references_to001BD938,
decompile00131D78 and listing00131E18..00131E54 confirm the complete reset and
its Car::BeginMovement tail call. Exact four stores and exclusions are recorded
in the per-function record. Raw captures/query hashes: local/task0079/manifest.json
(ignored). No Ghidra annotations, renames or saves.

Before this change, SetWheelTerrain's maximum at State+324..330 persisted
across frames, including a snow-to-road transition. Now one reset occurs before
the native Move/collision chain. Collision retries and solver substeps share
the accumulation window. Failed candidate frames do not publish the reset.
This is a bounded CarState binding, not a reconstructed whole BeginMovement.

## Acceptance and limitations

Synthetic tests check exact write extent and descending-frame lifecycle with
multiple contacts. The original-data backend regression checks snow-to-road
transition and rejected-frame rollback. Exact commands/results are recorded in
TASK-0079 and docs/PROGRESS_LOG.md. No new GPU behavior or Godot API.

Remaining work: recover Player dependencies before enabling nitro; sound reset
when connecting original sound; replay snapshots when original replay ownership
is implemented. Matched Debug original-runtime traces are still required for
numerical equivalence. Host tests cannot establish that equivalence.

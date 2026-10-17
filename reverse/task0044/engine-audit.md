# TASK-0044 engine, drivetrain, table, steering and aerodynamics audit

Behavior target: **Debug NFSHP2.ELF**, SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`. Revision/date unknown. These findings do not change historical Retail records. Status: reconstructed, **not Verified**.

## Evidence and method

E-0044-E01 consists of 92 complete Debug function decompilations and address-bounded instruction listings, 184 read-only Ghidra queries, plus program identity and named-global lookups. Private captures and per-query SHA-256 values are in ignored `local/task0044/engine/manifest.json`. The durable `engine-audit.json` inventory includes all 92 entry addresses, original mangled/reference names, full capture hashes, and audit exclusions. Ghidra program selection was explicitly `NFSHP2.ELF` on every inspect query; no database mutation was requested. Source catalogue E-0041-I01 establishes names, not behavioral equivalence. Instruction listings resolve ambiguous decompiler expressions. Imported type offsets were corroborated against the actual accesses below.

## Canonical ownership and APIs

| Original owner/source | Debug entries recovered for driving | Explicit host binding |
|---|---|---|
| Engine, `indep/src/engine.cpp` | ctor00188998, ComputeAuxiliaries00188B38, GetDerivative00188B88, GetTorque00188BD8, GetTotalEngineTorque00188C60, GetEngineTorque00188D20, DecrementNitro00188DA0; inline access/state methods0018A348..0018A4A0 | ObjectView(Memory&, logical PS2 address), float/scalar arguments and logical output pointers |
| DriveTrain, `indep/src/engine.cpp` | ctor00189020, differential001891B0/00189320/00189508, slippage00189280, ComputeAuxiliaries00189648, derivative00189840, clutch00189870/00189AF0/00189BD0/00189D40, Idling00189D48, ClutchMatched00189D98, gear00189DD0/00189E10; inline0018A1B0..0018A2F0 | Same address binding; typed static projections retain one canonical scalar algorithm for isolated testing |
| CalculateShiftPoints, `indep/src/engine.cpp` | free function00188DE8 | Memory plus configuration/output addresses and detune float |
| Table, `indep/src/table.cpp`; inline definitions emitted into `indep/src/carinfo.cpp` | ctor0018D810, GetValue0018D840, InverseLookup0018D8D8; SetMinMax001872D0, CalcIndexMultiplier00187318 | Address view over pointer/count/min/max/index multiplier |
| Graph, `indep/src/table.cpp` | ctor0018D9E8, GetValue0018D9F8, GetInverse0018DAF8 | Address view over point pointer/count |
| Aerodynamics, `indep/src/aerodynamics.cpp` | ResolveForces001AB3F0, GetDownforceAtSpeed001AB638 | Address view, optional logical scalar output pointer |
| TwoWheelAckermanSteering, `indep/src/suspension.cpp` | ctor001A3F30, SetSteering001A3540, empty ComputeAuxiliaries001A3EC8 | Address view and unsigned16 angle |

Companion C++ header placement and ObjectView inheritance are port-side. The original constructor entry is separated from the host object's lifetime using its Debug address-named method. No native host ABI or implicit ownership is asserted. Original allocation/deallocation and replay snapshots are outside the driving behavior scope; missing replay paths continue to throw. Empty original methods FinalizeClutchTorque00189D40, PrintCurrentState0018A160 and steering ComputeAuxiliaries001A3EC8 are evidenced empty returns, not fabricated fallbacks.

## Confirmed layout and algorithm changes

**DriveTrain differs materially from Retail.** Debug ctor00189044/0018904C forms ShiftUpRPM at `this+0x4C` and ShiftDownRPM at `this+0x70`. SetDesiredGear00189E70/00189EB0 loads those same bases. The nine-float tables end at0x94. Retail's former torque-history slot at0x4C and shift tables0x50/0x74 must not be copied into Debug. Debug GuessClutchTorque00189AF0 clamps the engine-minus-differential angular velocity to ±ClutchLimiter, multiplies by ClutchStiffness and State.clutch, and applies that torque. It has **no Retail sign reversal/history/halved-stiffness branch and no history write**. The ctor does not clear0x4C after calculating shift points. Canonical instance loading reads Debug tables and omits the old history load/store; legacy typed projection field spellings remain compatibility storage only.

DriveTrain's remaining fields are State.clutch0xC, DTI0x10, Engine0x14, four wheel pointers0x18, CarState0x28, DesiredGear0x2C, CurrentGear0x30, PreviousGear0x34, CurrentClutchState0x38, Shifting0x3C, DifferentialAngularVelocity0x40, GearHoldTime0x44, SportGear0x48. ApplyClutchTorque preserves the observed asymmetric front/rear angular-velocity numerators, individual brake locks0x10C and contact bytes0x155; it reads CarState.BurnOutState0x3A4. Differential acceleration reads wheel AppliedTorque0x108, GroundForce.x0x114, MomentOfInertia0x24 and WI.Radius0xC. Gear changes preserve strict comparison boundaries and original 32-bit wrapping upper-bound addition.

Engine's Debug storage is State.w0xC, EI0x10, CarState0x14, Table TorqueCurve0x18, Table EngineBraking0x2C, IsHP word0x40, Throttle0x44, RoadTorque0x48, ThrottleOverride0x4C, NitroFlag word0x50, NitroBoostTimer0x54 and CatchupTorqueMultiplier0x58. Constructor sets vtable0x2AC3F8; drivetrain sets0x2AC3B0. The total-torque listing 00188C84/00188C8C explicitly multiplies Throttle*ThrottleOverride first, then torque, despite decompilation printing a left-associated expression. The reconstructed grouping preserves this counterevidence. Engine braking and DriverMode0x39C scaling are preserved. ComputeAuxiliaries clamps to idle/max RPM in angular units then clears RoadTorque.

Table storage is pointer0,count4,min8,max0xC,multiplier0x10. Conversion uses the existing bounded EE CVT.W.S contract; unsupported nonfinite inputs throw rather than inventing indices. InverseLookup has an unusual exact-first-value return: it returns the table value itself when within its first epsilon; this is retained. Graph's inverse exhausted/fewer-than-two fallback returns first **y**, while normal inverse interpolation returns x. These behaviors were not normalized. Table finite interpolation, Graph boundary/epsilon paths and shift-point sampling otherwise match the reviewed Retail shapes with Debug addresses.

Steering ctor stores CTI0, SteeringMatrix pointer4=self+0x10, vtable8=0x2ADAD8. It does not initialize the two matrices. SetSteering initializes matrices at0x10 and0x50; wheelbase is CTI[0x120]-CTI[0x180], track width CTI[0x124]-CTI[0x144]. Negative angle changes the inner/outer assignment. This owner is established by fresh ECOFF definition evidence: translation unit `..\indep\src\suspension.cpp`, FDR 65, symbol 273815, symbol-file offset0x5CC914, path marker0x5CC908, MAP releaseSuspension.obj line3773. Original header placement is not inferred from this source marker.

Aerodynamics fields remain AI0, body4, Boost8, Draft0xC, Throttle0x10, GroundEffect0x14. Body velocity0x90, forward matrix vector0x20, vertical orientation0x48 are actual accesses. Its scaled-speed direction, fourth-power forward projection, off-throttle center displacement, ground-effect branches and front/rear force positions are retained. Listing 001AB558/001AB568 squares twice. Downforce force application delegates to the one canonical body implementation through compatibility adapters.

## Required Debug globals/static dependencies

| Purpose | Debug address |
|---|---|
| Engine angular/RPM factors, driver braking and timer scale | 00325420..00325430 |
| Drivetrain differential/slip/auxiliary/idle/gear thresholds | 00325434..00325490; individual accesses in captured listings |
| MaxRPMEffect / TorqueHPMultiplier | 00327918 / 0032791C |
| ShiftUpDetune / ShiftDownDetune / GearShiftMethod | 00327924 / 00327928 / 0032792C |
| LIMITEDSLIP / ClutchLimiter / match threshold / MaxDownShiftOver | 00327930 / 00327934 / 00327938 / 0032793C |
| Derivative clutch-state lookup table | 003077C8, indexed by CurrentClutchState |
| WorldTimer.PackedTime / WorldTimeElapsed | 00327CF8 / 00327D08 |
| Table inverse first/subsequent epsilons; Graph forward/inverse epsilons | 003254D8 / 003254DC / 003254E0 / 003254E4 |
| Steering scale, pi, two-pi, sine phase | 00325780 / 00325784 / 00325788 / 0032578C |
| Aero OffThrottleDragFactor / CenterHeight | 00327ED0 / 00327ED4 |
| Aero speed epsilon / front fraction / rear fraction | 003257E0 / 003257E4 / 003257E8 |
| FrontDownforceLocation / RearDownforceLocation | 00308CC0 / 00308CD0 |

The observed gp accesses corroborate gp=0x0032C9F0, e.g. EngineBraking factor `-0x75C8(gp)` resolves00325428 and ClutchLimiter `-0x50BC(gp)` resolves00327934. No values are copied from Retail's global addresses. Original constants/tables must come from the SHA-bound Debug image loader; these are required dependencies, not arbitrary port tuning defaults.

## Validation and unresolved limits

The implementation has synthetic regression fixtures for actual Debug constructor vtables, distinct0x4C/0x70 shift table reads, terminal shift entry, history-free clutch torque, total torque/braking, Table interpolation and unusual inverse behavior. Existing scalar boundary fixtures remain useful through forwarding adapters. Validation: `cmake --build build/native-debug --target driving_drivetrain_tests -j 4` passed; `build/native-debug/driving_drivetrain_tests.exe` passed with `DRIVING_DRIVETRAIN_OK`. C++ compiler: MinGW-W64 GCC 13.2.0 (r8); Python 3.14.4. These are PC instruction fixtures, not original-runtime equivalence.

Original EE exceptional scalar behavior is only projected where an existing explicit contract exists. ApplyClutchTorque's DIV.S sites are Debug 0018993C and00189A48; rest-state zero denominators follow the existing finite EE saturation policy. This remains a port projection with its documented domain, not a full EE emulator. Host tests and source/listing inspection do not establish synchronized numerical equivalence. Replay, original pool allocators/destructors and unrelated Average/Linear methods are excluded, preserving their throwing boundaries. Original-game runtime comparison remains the integrating task's required verification step or explicit blocker.


Counterevidence during integration: the first migration retained unsigned literal `0x310050u` in DriveTrain::GetDerivative, because its suffix escaped a token substitution. Debug 00189844/00189848 establishes `0x003077C8`; this operand was corrected and a mapped-table regression added. The other engine/gear scalar operands were enumerated and checked against captured Debug accesses; no Retail literal remains in the canonical engine body.

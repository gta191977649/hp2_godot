# E-0046-P01 - REAL_CONTROL burnout dependency

Target: Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Read-only live Ghidra decompilation of PhysicsCar::Move 00137D18 and listing
00137F50..00138298, 2026-10-17. Source ../indep/src/physicscar.cpp is established
by E-0044-P01. Raw results stay ignored in local/task0046/root.

## Confirmed

The earlier blanket car+20==1 exclusion blocks REAL_CONTROL, not an unrelated mode.
Listing calls IsJoystickTypeWheel 0011D720 with RealDriver.Port at +234; true skips
burnout processing. The host provides a device-classification boundary, explicitly
non-wheel for this keyboard/ordinary-stick tool; missing service throws.

CarState imported type and instruction offsets independently agree: BurnOutState
3a4, BurnOutTime3a8, BurnOutTraction3ac, BurnOutAllow3b0. Existing state fields
ForwardSpeed380 and DriverMode39c are used. Wheel AppliedTorque108 and Terrain150
are instruction-confirmed. Static friction is TerrainTypeInfoTable3102e8+90*ID+20.

Debug gp32c9f0 operands establish BurnOutTorqueValue327084, clip327088,
cancel torque32708c, max speed327090, traction327094, minimum time327098,
allow time32709c, yaw maximum3270a0 and yaw cancellation3270a4. Original elapsed
WorldTimeElapsed327d08, not the caller dt, decrements burnout timers.

Existing burnout cancels for negative remaining time or yaw beyond its threshold.
All four torques must be strictly below cancellation torque to accumulate allow
time; equality of allow time does not cancel. The successful accumulator branch
does not clear itself (00138050 branch to0013805c); preserve that subtlety.

New burnout requires DriverMode1, zero steering, positive speed below maximum,
and yaw below maximum. Rear torque has priority over front torque. Rear-left
greater than rear-right selects wheel2/state1; ties select wheel3/state2. Front
right greater than left selects wheel1/state1; ties select wheel0/state2. State
selection precedes blinking rejection. Clipped torque interpolation preserves
DIV, subtraction, multiplication and addition order at001381b4..00138288. Rear
material static multiplier unequal to1 scales the original traction value by1.25.

## Implementation and validation

The body resides solely in PhysicsCar::Move, followed by its existing yaw/force
chain. No new burnout helper algorithm exists in the host. driving_mover_tests
uses explicitly synthetic constants to test side selection, interpolation order,
strict allow threshold/equality, and wheel-controller bypass. These are instruction
fixtures, not original trajectory samples. Original comparison remains unperformed.

## Limits

IsJoystickTypeWheel's device-service dependency is explicit; full original joystick
hardware discovery is outside this port boundary. Race diagnostic restore remains
unimplemented and throws. This recovery does not establish numerical PS2 equivalence.

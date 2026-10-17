# TASK-0038 — explicit grounded synthetic scenario

Evidence E-0038-I33. Target SLUS_203.62, revision unknown, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
The player's original GLOBAL/GLOBALB.BUN config row39 names911TURBO. Data was
resolved through `tools.ps2_assets.path`, not a hardcoded asset location.

This is an explicitly port-side initial condition for a synthetic horizontal
plane and identity body orientation. It is not the original spawn algorithm or
a guessed equilibrium solution. The original constructor and state classifier
can suppress movement in state2, so starting above the plane and expecting a
fall is not a justified setup strategy.

## Confirmed equations

001375F0/001A5F88 compute controller+0x80 as wheel-config XYZ/W minus
body+0x160, where the latter copies config+0x110.001A5FC0 transforms that vector
and adds body position, queries the plane via001A4580, adds wheel-config+0xC,
and clamps to controller-config+0x18/+0x14. With identity orientation and a
horizontal plane, the unclamped controller+0x14 is therefore:

`plane_height + wheel_config_0xC - (body_center_z + wheel_config_0x8 - config_0x118)`.

A zero-displacement center coordinate for each wheel is consequently:

`plane_height + wheel_config_0xC - (wheel_config_0x8 - config_0x118)`.

## Original911TURBO row observations

All four wheel config+8 values are approximately0.11; config+0x118 is
approximately0.24, so the controller local Z is approximately-0.13.

| Wheels | Wheel config+0xC | Controller min/max | Zero-displacement center Z |
|---|---|---|---|
| 0/1 | 0.3190000057 | -0.125 / 0.1199999973 | 0.4490000010 |
| 2/3 | 0.3170000017 | -0.1150000021 / 0.1150000021 | 0.4469999969 |

For plane height0, all four unclamped displacements are within their original
limits when center Z lies in the intersection approximately[0.3319999948,
0.5619999990]. Selecting the minimum of the four zero-displacement center
coordinates, approximately0.4469999969, puts both front displacements near0.002
and rear displacements near0. This is a reproducible geometric port setup,
not a claim that all spring/damper forces are at equilibrium.

The values above were derived from original config bytes and the recovered
equation; native source should calculate from its loaded config instead of
hardcoding this911TURBO-specific coordinate. All proposed displacements must
be checked against their original min/max constraints; unsupported geometry
must be rejected rather than supplied a guessed rest height.

Private numeric projection: `local/task0038/constructor2/911TURBO-ground-projection.json`.
Original-plane versus synthetic-plane behavior and the original spawn procedure
remain unverified. No original equivalence or Verified status is asserted.

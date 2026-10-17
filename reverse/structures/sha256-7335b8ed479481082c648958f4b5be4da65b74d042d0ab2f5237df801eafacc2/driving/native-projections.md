# TASK-0038 native dependency projections

Target: SLUS_203.62; executable revision unknown; SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Evidence E-0038-I01..I13, D01..D07, S01..S11. All observations are read-only
Ghidra instruction/data observations. No original C++ type layout or identifiers
are recovered by these port-side containers; no Verified status is claimed.

## Confirmed field access groups

| Port projection | Original access contracts |
|---|---|
| init::ByteView | Little-endian bounded image; pointers remain encoded32-bit words. Constructors write only evidenced offsets; no blanket zero initialization. |
| DrivetrainProjection | this+0x2C/+30/+34/+38/+48 are integer accesses; +0xC/+40/+44/+4C and threshold tables+50/+74 are float accesses. Resolved config, engine, car and four wheel objects are explicit inputs. |
| DrivetrainWheelProjection | +20/+24/+108/+114/+160 float; +10C integer gate; +155 signed byte tested nonzero. config pointer at+C resolves another+C scalar. |
| SolverBodyProjection | +10 vector, +20 matrix, +F0/+100 four-lane accumulation, +118/+148 scalars. Original caps affect XYZ length, then scale four lanes. |
| SolverWheelProjection | Scalar basis groups+B0..B8/+C0..C8; +114/+118 iterates; +154 and+10C exclusion gates. +155 byte is explicitly truncated on store. |
| SolverSuspensionProjection | +70 integer forwarded into wheel contact byte; +80/+84 solver terms; +90/+94 resolve wheel/body. |
| SolverMathContext | Explicit mutable00320A60 four-lane global; transform uses its W rather than input W. |

Names describe port projections only. Read individual function records for exact
store order, signedness, branches, dynamic global addresses and untouched lanes.
The original function pointer graph and allocator lifetime model remain incomplete.

## Hypotheses

Physical interpretation of most config scalars remains unbound. No guessed
friction, mass units, gear enumeration, spring constant or rest state is supplied.

## Unresolved

The three projection groups are tested independently and are not a production
driving backend. Integrating them requires complete constructor wiring, resolved
wheel/table dependencies, coherent shared state and original scheduling/contact
contracts. Original aliasing, invalid-memory cases and exceptional EE/VU rounding
remain outside the tested scope. Native projection tests cannot verify PS2 parity.

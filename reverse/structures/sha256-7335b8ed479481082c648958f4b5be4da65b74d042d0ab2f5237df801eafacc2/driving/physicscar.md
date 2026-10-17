# PhysicsCar dependency projection - TASK-0038

Binary: SLUS_203.62; executable revision unknown; SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Read-only Ghidra MCP, 2026-10-17 Australia/Sydney. Original identifier unknown.
No original behavior equivalence established. Native ordinary-float tests are not PS2 FPU verification.

Evidence E-0038-001/002/004. Offsets refer to PS2 objects, not native C++ layouts.

## Confirmed static relationships

| Owner offset | Observed relationship |
|---|---|
| car+0xC | config row passed to00137100 |
| car+0x28 / +0x2C / +0x30 | controller / its shared controls / mover |
| mover+0x18 | object constructed via0019C9B0 |
| mover+0x20..2C | four001A5EA0 objects |
| mover+0x30..3C | four001A3F10 objects |
| mover+0x40 / +0x44 / +0x48 |0018A7E8 /0018AE70 / helper objects |
| mover+0x4C / +0x50 / +0x54 | config / car+0x40 / car |

Config is the64-row,0x560-stride original chunk0x34600. The new reader preserves an entire row as opaque bytes, rather than assigning unproven handling field names. Existing E-0033-B04 locates911TURBO at row39.

## Runtime observations (not behavioral verification)

PCSX2 v2.8.2, game ID SLUS-20362, CRC1d2818af, reported game version1.00. The reported version does not prove executable revision. On2026-10-17 two repeated paused captures of a JAGUAR vehicle were identical within each capture pair. Its controller table was002BE170, mover table002AEAC8 and mover update target00137D90. Mover config and car pointers agreed with the static relationships above.

Before/after snapshots are18 bounded read windows each; window lengths are tooling bounds, not proven allocation sizes. No save slot was written. Private files: local/task0038/pcsx2/paused.json and after-throttle.json. The first sample was a replay context (nonzero330148); no claim of a clean race initialization is made.

An OS-keyboard Cross input produced controls+0x214 float1.0. During a roughly3-second non-atomic sample sequence, wheel0+0x1C changed from24.179752 to162.728592, wheel0+0x20 from0 to69.500526 and car+0x3C4 from0.0588864 to23.647615. These are observations at wall-clock sample points, not confirmed physical units or frame-aligned input/output pairs. No native trajectory comparison was performed. SDL Pad1 mappings were restored and compared equal to the pre-session configuration.

## Unresolved

Original timestep scheduling, complete initialization, the wheel/contact query contract at00152070, road table00318C14/00318C20, body derivatives, full wheel-force iteration, shifting and original keyboard/controller filtering remain unrecovered. A synthetic plane visual must not be interpreted as a working contact provider.

## Reproduce

Pause an already-running matching game and run `py -3 -m tools.driving_oracle`. Running captures are rejected. Two identical paused captures corroborate stable reads; they do not establish an instruction boundary or deterministic frame stepping.

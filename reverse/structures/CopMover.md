# CopMover â€” partial PS2 layout, TASK-0009

Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
USA/NTSC inferred; executable revision unknown; boot VER 1.00.
`CopMover` is surviving allocation/diagnostic text, not a recovered RTTI definition.
Allocated extent 0x80; little endian, 32-bit address words. Alignment/inheritance
unknown. Status Identified partial layout; no original-memory verification.

| Offset | Width | Observed use | Evidence |
|---|---|---|---|
| 0x00 | unknown | unknown | none |
| 0x04 | 4 | vtable address 0x002B5A08 | E-0008-02 |
| 0x08 | 4 | car address | E-0008-02, E-0009-01 |
| 0x0C | unknown | unknown | none |
| 0x10 | 16 | copied block from car+0x70 | E-0008-02 |
| 0x20, 0x24 | 4 each | zero-initialized words, purpose unknown | E-0008-02 |
| 0x28, 0x2A | 2 each | direction-like values copied from car+0x3A4 | E-0008-02/03 |
| 0x2C | 4 | movement scalar copied from car+0x3C0 | E-0008-02 |
| 0x30 | 4 | requested speed-like scalar written by Think | E-0008-03 |
| 0x34 | 4 | global-derived word, purpose unknown | E-0008-02 |
| 0x38 | 4 | initialized to float 40, physical meaning unknown | E-0008-02 |
| 0x3C | 4 | type-dependent coefficient bits | E-0009-01 |
| 0x40 | 4 | zero-initialized word, purpose unknown | E-0008-02 |
| 0x44â€“0x4F | unknown | unknown | none |
| 0x50, 0x60 | 16 each | copied/vector-operation blocks; component meaning uncertain | E-0008-02 |
| 0x70, 0x74 | 4 each | zero-initialized words, purpose uncertain | E-0008-02/03 |
| 0x78â€“0x7F | unknown | unknown | none |

Native `hp2::recovered::CopMover` is an existing-storage view. Its one recovered
local method corresponds to 0x001A0EA0; exact coefficient bits and unknown-type
no-write behavior are supported by all disk jump-table entries and return paths.
This does not implement constructor, Move, destructor, wheel or terrain behavior.
Move 0x001A1030 has a one-byte body; vtable pointer 0x001A0E88 has no function.
No original class hierarchy, method signature or physical coefficient units claimed.


## TASK-0028 continuation

The existing storage view is preserved. Its local method is now `Sub_001A0EA0`,
per AGENT.md R2; the old descriptive helper spelling was port analysis wording.
Missing attributed entries are throwing templates in the same class. See the template
inventory for current source links. Original layout/equivalence remains unverified.

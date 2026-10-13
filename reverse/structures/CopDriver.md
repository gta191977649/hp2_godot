# CopDriver â€” partial PS2 layout, TASK-0009

Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
USA/NTSC inferred; executable revision unknown; boot VER 1.00.
`CopDriver` is surviving allocation/diagnostic text, not a recovered RTTI definition.
Allocated extent 0x4C; little endian, 32-bit address words. Alignment/inheritance
unknown. Status Identified partial layout; no original-memory verification.

| Offset | Width | Observed use | Evidence |
|---|---|---|---|
| 0x00 | 4 | car address used by Think and low-speed helper | E-0008-03, E-0009-02 |
| 0x04 | 4 | subordinate route object address | E-0008-02 |
| 0x08 | 4 | vtable address 0x002B5998 | E-0008-02 |
| 0x0Câ€“0x23 | unknown | unknown | none |
| 0x24 | 4 | numeric controller state | E-0008-03, E-0009-02 |
| 0x28 | 4 | saved prior state | E-0008-03 |
| 0x2C | 4 | unknown, initialized to 1 | E-0008-02 |
| 0x30 | 4 | float low-speed timer; assembly LWC1/ADD.S/SWC1 | E-0009-02 |
| 0x34 | 4 | float timer in state 4 | E-0008-03 |
| 0x38 | 4 | global-derived value, purpose unknown | E-0008-02 |
| 0x3C | 4 | pursuit-record index, initialized -1 | E-0008-02/03 |
| 0x40, 0x44 | 4 each | aim-point components, type interpretation provisional | E-0008-03/05 |
| 0x48 | 4 | type-derived numeric selection | E-0008-02 |

Native `hp2::recovered::CopDriver` is a storage view, not the original class layout
or native vtable. Its one recovered local method corresponds to 0x001A2A90;
adapter `PoliceMemory::query_00231258` must implement the external call and side
effects. Object construction, Think, inheritance and other methods are unresolved.
No semantic labels assigned to numeric states. Tests exercise finite normal
float cases; PS2 exceptional/denormal handling and original oracle remain gaps.


## TASK-0028 continuation

The existing storage view is preserved. Its local method is now `Sub_001A2A90`,
per AGENT.md R2; the old descriptive helper spelling was port analysis wording.
Missing attributed entries are throwing templates in the same class. See the template
inventory for current source links. Original layout/equivalence remains unverified.

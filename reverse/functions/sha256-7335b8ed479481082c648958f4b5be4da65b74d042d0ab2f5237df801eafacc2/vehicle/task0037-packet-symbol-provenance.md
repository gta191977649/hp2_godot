# TASK-0037 - Packet accessor symbol provenance

- Binary: SLUS_203.62; executable revision unknown.
- SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Evidence: E-0037-N01 (read-only Ghidra symbols/get, function list, accessor/caller
  decompilation and listing); E-0037-N02 (original string memory search/read and references).
- Investigation: 2026-10-15. Original runtime comparison is irrelevant to identifier spelling;
  no behavior or Verified status was changed.
- Raw outputs: ignored local/task0037/*.json. No database annotations/renames were made.

## What Type_0011BF98 is

It is a native grouping created by the port for packet access projections, named after
original function address0011BF98. It is NOT a recovered ELF class symbol. Neither an
original class name nor original C++ membership, constructor, vtable, layout, source file
or exclusive ownership has been established. std::vector/std::array and checked byte reads
are the port interface, not the original ABI. CarViewer reaches it through
port::assets::decode_vehicle_geometry, then uses its position/UV/color/count results.

The name does not mean an original constructor exists at0011BF98. That original routine
calculates three component addresses; its wrapper0011C2B8 reads their values. Calling the
port C180 method returns raw color, whereas original0011C180 computes a color address.
Do not silently reinterpret the port signatures as original function prototypes.

## Confirmed accessor roles and direct consumers (N01)

| ELF address | Confirmed bounded role | Direct original caller evidence |
|---|---|---|
|0011BF98|Calculate packed X/Y/Z component addresses|0011C2B8 reads values;0011C338 writes values|
|0011C090|Calculate packed U/V component addresses|0011C268 reads;0011C398 writes|
|0011C180|Calculate packed16-bit color element address|00104100 and0011C3E0|
|0011C248|Return byte at packet+4, used as vertex count|0010C158,0010D778 and other packet helpers|
|0011C268|Read U/V values through0011C090|0010C158 rescales UVs after texture allocation dimensions|
|0011C2B8|Read X/Y/Z values through0011BF98|00104100 callback transformation;0010E178 debug geometry construction|

All six symbols/get results have `source=DEFAULT`, `namespace=Global`, generated
`FUN_<address>` names and no original spelling. The original-name audit E-0019-N02 agrees.
The focused imported-symbol search for VIF/packet/strip/geometry/solid/poly/prim/vertex
returned no matching imported symbol. This is a bounded lookup, NOT a claim that every
ELF symbol/name has been eliminated everywhere in the binary.

Caller listing0010C318 loads the packet pointer from strip+8 into s1;0010C328 and
0010C350 pass it as a0 to count/UV helpers.00104100 traverses64-byte strip records,
loads strip+8, reads positions/colors, invokes a callback, and writes updated positions
through0011C338.001B4B50 reaches that traversal with callback001B48B0. Neighbor0011C328
writes a masked byte into packet+0x10. The examined0011BF98..0011C43F instruction range
contains only packet access/helper logic; it exposes no class-name/assert/function-name
string loads or allocation constructor chain. This does not prove the original functions
were free functions: a packet pointer could also be a C++ receiver.

## Naming leads checked, and why they are not replacements (N02)

- `eStripEntry` exists verbatim at002AC298. Exact references_to returned none. Nearby
  `ObjectName`/`NumVerts` strings are used by debug consumer0010E178, but there is no
  direct naming bridge from the eStripEntry literal to these accessor entries. The64-byte
  strip record and its +8 packet target are distinct proven objects. eStripEntry remains
  a related lead, not a confirmed name for the packet or its functions.
- `DuplicatedSolid` (002ABD18), `DuplicatedStripEntryData` (002ABD28) and
  `DuplicatedStripEntryTable` (002ABD48) are original allocation labels in00103BF0.
  The data/table allocations are immediately copied byte buffers, then00103FF0 relocates
  packet pointers. No constructor at0011BF98 follows these allocations. These labels do
  not establish a C++ class identity for the helper grouping.
- `eVertexAnimatorSlotPool` at002AC168 and `ePolySlotPool` at002AC180 reference startup
  sites0010A060/0010A07C, not the packet helper cluster. Proximity does not assign ownership.
- `Packet` search hits002C2FAC/002C4CED are audio/TTY messages. Other `Strip` hits include
  GS LINESTRIP/TRISTRIP names, spike strips and FlailerStripSlotPool. None provides an
  original spelling for the six addresses. VIF search finds command decoder diagnostics;
  `geometry.cpp` had no literal match in this query. No source path is confirmed.

## Result and unresolved requirements

No recovered-verbatim class or function spelling was established for this group. Retain
Type_0011BF98 and Sub_<ADDR>; do not invent GetPosition/GetUV/VertexPacket/eStripEntry
member names. Function purpose is useful explanatory prose, not original symbol evidence.
The accompanying reverse/naming/task0037-packet-symbol-provenance.csv is the compact ledger.

To advance exact naming, obtain matching authorized unstripped/debug ELF, link map or
original diagnostic evidence that actually binds a spelling to one of these addresses.
A different build/platform name would still require explicit correspondence, not automatic
reuse. Original C++ class membership and source path stay unknown. No REA/third-party
analogy, generated decompiler name or allocation-buffer label was promoted into an identifier.

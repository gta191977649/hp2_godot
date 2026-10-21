# E-0086-M01: original vehicle position markers and shared flare textures

TASK-0086,2026-10-21. Confirmed static evidence, implementation PC-tested;
original runtime equivalence is not Verified. Authority: Debug NFSHP2.ELF
SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
All Ghidra operations were read-only. Raw captures remain ignored in
`local/task0086/marker-loader.json`, `marker-find.json`, `marker-type.json`,
`cartype-type.json`, plus the existing TASK0082 constructor capture.

## Confirmed bindings

- `LoaderPositionMarkers` Debug001A5670 accepts chunk0x34013, aligns payload
  start to16bytes, computes record count by integer division by32, and appends
  every record to PositionMarkerList in file order. Serialized Next/Prev words
  are overwritten by runtime list insertion and are not position data.
- Imported `ePositionMarker` is32bytes: Next+0,Prev+4,NameHash+8,TypeHash+12,
  float Position[3]+16, signed Param+28. The adapter retains the original Param
  bit pattern; it does not reinterpret raw link words as host pointers.
- `eFindPositionMarker`001A5758 with name_hash0 selects all matching TypeHash
  entries, continuing after the previous marker. Both hashes0 return null.
- `CreateCarLightFlares`00120558 selects TypeHash=bStringHash of
  CarTypeInfo.BaseModelName. Imported CarTypeInfo places that string at+0x20;
  the existing port CarRenderConfig.prefix already reads that exact field.
  Name classification, Param byte1 normal selection and subtraction of pivot
  belong to the native flare module, not the asset decoder.
- CarRenderInfo constructor0011EA88 takes PivotPosition from Body[0].Solid's
  PivotMatrix.v3.xyz, with zero only for absent Body0. Imported eSolid confirms
  PivotMatrix+0x60; the existing VehicleSolid.transform decoder already reads
  those64bytes. Model.marker_pivot returns elements12..14 unchanged.
- LightFlareTextureHashes002CD730 contains8868656F,88D8E8A2,3394FE62.
  Native eLightUpdateTextures00116880 resolves these three hashes with
  GetTextureInfo(...,0,0). Original GLOBAL/GLOBALB.BUN contains all three;
  the checked CARS/TEXTURES.BIN and GLOBAL/INGAMEB.BUN contain none.
  Model preloads original textures through the existing validated TPK service,
  without adding archive priority or replacement textures. Absent hashes
  produce explicit diagnostic gaps.

## Implementation and fixtures

`port::render::VehiclePositionMarker` is a port-side value projection, not a
PS2 ABI object. VehicleGeometry.position_markers retains all decoded records;
Model.position_markers selects only the original BaseModelName TypeHash. The
decoder traverses ordinary chunk containers and preserves file order, including
the marker chunk that precedes the vehicle geometry container at file offset0.
Model cache accounting includes marker storage. Original geometry, positions,
physics and texture state remain unchanged.

Original asset observations:

| Geometry | Marker count | TypeHash |
| --- | --- | --- |
| 911TURBO |30|5A7E6906|
| BMWM5COP |36|6C411C29|
| 4DR_SEDAN |4|F27F6A53|

Passing actual asset/native marker classification counts by original type0..8:
911TURBO `(2,4,2,0,0,0,0,1,1)`, BMWM5COP
`(4,2,2,0,2,2,4,1,1)`,4DR_SEDAN `(2,2,0,0,0,0,0,0,0)`.
The test checks exact pivot subtraction and Param-selected original normal for
every cooked marker. Nonlamp helper markers remain in raw geometry and are
ignored by original flare classification, rather than becoming fabricated lamps.

`src/tests/vehicle_marker_tests.cpp` checks synthetic alignment/stride/order,
ignored link words, Param preservation, nested containers, malformed/truncated
streams and non-finite positions. With private assets configured it checks
these three original fixtures, selected model hashes and exact Body0 pivot.
Synthetic fixtures carry no original-game provenance. Missing private assets
skip only original-asset cases and report that fact explicitly.

Validation2026-10-21, GCC13.2.0/CMake3.29.2/Ninja1.12.1:

```
cmake --build --preset native-debug --target vehicle_marker_tests -j4
ctest --preset native-debug -R '^vehicle_markers$' --output-on-failure
build/native-debug/vehicle_marker_tests.exe
cmake --build --preset native-debug --target vehicle_model_tests -j4
ctest --preset native-debug -R '^vehicle_model$' --output-on-failure
```

All passed: marker CTest1/1 in2.39seconds, direct original asset/classification
run `VEHICLE_MARKERS_OK`; existing vehicle model regression1/1 in10.22seconds.
Marker tests assert all three original flare textures are decoded from
GLOBAL/GLOBALB.BUN. Existing unrelated misleading-indentation warnings remain.
An initial configure attempt preceded root test registration and reported an
unknown target; a later build was blocked by the concurrent new flare module's
C++20 bit_cast use, which its owner corrected for the project's C++17 baseline.
No unresolved compile/test failure remains in this asset slice.

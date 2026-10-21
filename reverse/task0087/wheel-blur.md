# E-0087-B01: optional original wheel-blur geometry

TASK-0087,2026-10-21. Debug NFSHP2.ELF SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b,
version independently unknown. Static original behavior confirmed; original
runtime equivalence not Verified. Ghidra operations were read-only.

## Cause and original behavior

The port Model::apply_driving_pose rejected every positive wheel blur alpha
whose model had no WheelBlur part. That was a port-side precondition, not an
original-game resource requirement. It stopped all wheel/body publication for
legitimate shipped vehicles with no optional blur solid.

CarRenderInfo constructor Debug0011EA88 first initializes both TireBlur models
using `<BaseModelName>_WHEEL_BLUR`. Each absent solid gets its corresponding
`_WHEEL_BLUR_F`/`_WHEEL_BLUR_R` retry. The constructor permits a null result after
that retry. Existing port Sub_0011E860 binding names already represent this
lookup correctly; no new names or replacement geometry are needed.

CarRenderInfo::Render001210F0 computes per-wheel blur opacity and conditionally
calls eModel::Render for TireBlur[0] on wheels0/1 and TireBlur[1] on wheels2/3.
The blur opacity does not imply an asset exists. eModel::Render0010E048 begins
with `if(Solid != null)` and otherwise returns without strip submission. This
proves the optional absence behavior; it is not a guessed fallback.

Raw captures: ignored `local/task0087/wheel-blur-emodel-render.json` and
`wheel-blur-car-render-tail.txt`; the constructor capture is retained from
TASK0082 at `local/task0082/lights/0011ea88.json`.

## Asset inspection

A bounded header-only scan traversed43 shipped CARS/*/GEOMETRY.BIN chunk trees,
reading original solid names from aligned0x34003 records.16 contain no
WHEEL_BLUR solid:4DR_SEDAN,4DR_SEDAN02,AMBULANCE,BUS,COUPE,CROWNVIC,FIRETRUCK,
HATCHBACK,HATCHBACK02,MINIVAN,PANELVAN,PARCELVAN,PICKUP,SUV,TAXI,TAXI02.
The scan used original asset data under the configured private asset root;
no asset bytes were copied into Git.

Original F/R-only resources exist for CLK_GTR,LAMBOCOP,MCLAREN,MCLARENLM,
MURCIELAGO. The existing lookup fallback must continue to bind those, while
ordinary shared resources such as911TURBO remain unchanged. The user's actual
selected car was not supplied; no claim is made that it was CROWNVIC. Track
selection itself does not change which vehicle geometry file has these solids.

## Port change and validation

Model now reports original optional solid absence at load time, including both
attempted original binding names, and exposes front/rear availability. Driving
snapshot opacity no longer requires that optional asset. Full wheel snapshot
validation still occurs before any display mutation. No fake blur mesh, material,
texture or substitute physics is introduced. Preview's explicit force-blur
control still reports unavailable four-wheel geometry.

`src/tests/vehicle_blur_absence_tests.cpp` covers absent4DR_SEDAN/CROWNVIC,
present911TURBO and F/R-onlyMCLAREN, each with near-only and full LOD loading.
Positive alpha90 must keep four original tires updating and select existing
blur parts, with zero invented parts for missing resources. Invalid snapshot
NaN must reject atomically; publication must perform no asset IO/decoding.
Validation2026-10-21, GCC13.2.0/CMake3.29.2/Ninja1.12.1:

```
cmake --build --preset native-debug --target vehicle_blur_absence_tests water_contact_tests -j4
cmake --build --preset native-debug --target vehicle_blur_absence_tests vehicle_model_tests -j4
ctest --preset native-debug -R '^vehicle_(blur_absence|model)$' --output-on-failure
build/native-debug/vehicle_blur_absence_tests.exe
```

Builds passed. Combined CTest2/2 passed: blur absence1.46seconds, existing model
regression14.68seconds. Direct run reported all eight original fixture/LOD cases
and `VEHICLE_BLUR_ABSENCE_OK`. Missing private assets return77, registered as
CTest skip, instead of falsely claiming original asset validation. The water
test target was built for its owning agent; its results are recorded separately.
No original runtime or performance equivalence claim is made.

# TASK-0086 original vehicle light flares

Original lamp markers, flare parameters and textures now drive attached sprites
in Test drive and standalone driving. This restores the original glow sprites;
it does not add projected road beams. Native particle backend selection does not
disable these fixed attachments.

Noncop horn input flashes the original headlights. For an original COP-class car
such as BMWM5COP, horn press toggles the already recovered FAST police-light state
and siren mode; horn release retains that state. Existing brake, reverse gear,
track night codes and original police phase tables select their respective flares.
This reuses the recovered input callback, rather than adding a new cop-state setter.

## Evidence and ownership

Debug NFSHP2.ELF SHA256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`,
version independently unknown. See [native evidence](../reverse/task0086/flares.md)
and [marker evidence](../reverse/task0086/markers.md).

Model decoding retains original 32-byte ePositionMarker records. BaseModelName
selects the marker type hash; Body0 PivotMatrix supplies the pivot subtraction;
Param byte1 selects the original normal. The three original flare textures load
from GLOBAL/GLOBALB.BUN through the existing texture service. Runtime resources
come from the configured, SHA-checked Debug ELF; parameter tables are not copied
into source as substitute assets.

The Godot-independent vehicle_flares module reconstructs the lamp intensity slice,
parameter selection, original bit-float angular power approximation, size/colour,
distance-scale clamps, camera-facing corners and ZBias displacement. Snapshot
flare_intensities publishes twelve original lamp channels. The renderer derives
eView.H from the actual camera projection at the original logical512-pixel width.
CarTimebaseStart uses the existing single-player adapter's explicit zero origin;
this does not reconstruct the full original render-object allocation/history.

The particle presentation layer reuses a static quad mesh, materials by texture,
and persistent attached MeshInstance3D nodes. Instance colours and transforms
update without rebuilding quad vertex buffers. Original texture blend/depth state
uses the existing GS shader adapter. Flares remain depth-tested and do not write
depth for the three recovered textures. Unsupported material/mip/depth-writing
routes report explicit dependency gaps. Alpha sorting is reproduced among the
flare packets; complete original DMA/global alpha-list equivalence remains part
of the wider renderer's existing validation boundary. No original runtime frame
comparison was available, so this is not Verified.

## Validation

```powershell
cmake --build build/native-debug --target vehicle_flare_tests vehicle_marker_tests -j 4
ctest --test-dir build/native-debug -R '^vehicle_(flares|markers)$' --output-on-failure
cmake --build build/godot-debug --target ps2_port -j 4
ctest --test-dir build/godot-debug -R '^vehicle_flares_(gpu|track_gpu|live_exit_gpu)$' --output-on-failure
```

Native tests cover original resource pointers, marker classification and normals,
pivot subtraction, bit-float power, angle/absolute flags, colour/size/scale/ZBias,
day/night, brake, reverse, broken lamps and police phases. Real BMWM5COP native
input tests toggle lights with horn, observe red/blue phases, and switch them off
with a second press. Three original model fixtures and all three texture decodes
pass; existing vehicle_model, driving_backend and driving_player regressions pass.

Vulkan Forward+ checks use the production renderer on flat ground and Track11:
front/night/brake/reverse, actual cop horn activation and changing phases, red/
blue/white marker presentation, near/far/side observation, reset, vehicle reload,
unload and quitting with live attachments. In the isolated pixel test, an opaque
occluder reduces9920 flare pixels to0. Raw screenshots stay ignored in local/.
ObjectID-checked cleanup handles Godot deleting child nodes before extension
wrapper destruction. A rejected first Track11 fixture assumed size was monotonic
while changing observation angle; the native formula was retained and the test
was corrected to validate finite original-clamped scale.

No further performance matrix was run after the user's request to stop repeated
benchmarking. GPU particles remain optional and native remains the default.


Final strict preservation:7/7 scope tests passed280.215s; preservation CTest1/1
passed294.43s. TASK0086 source-review SHA256
5eb78631b86cd079c7074730ebf2cec463eb8f988d520bfa6224ac50f54b9b40.
Source changes preserve all unrelated recovered functions and stub classifications.
Final host suite3/3 passed37.33s; original runtime equivalence remains unverified.

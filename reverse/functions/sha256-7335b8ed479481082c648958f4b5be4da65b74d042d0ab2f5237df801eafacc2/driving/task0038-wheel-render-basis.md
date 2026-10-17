# TASK-0038 - Native wheel matrices require original render basis composition

- Evidence: E-0038-W01, extends E-0033-B02 and E-0035-W01.
- Target: SLUS_203.62; executable revision unknown; SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
- Status: implemented and host-regression tested; not Verified.
- Scope: reported driving/preview wheel orientation mismatch; no physics, scene, camera or original controller change.
- Sources: read-only Ghidra listing00122E70..00123000 and00123290..00123400; refs to0011BA18/0011B248; prior001C0D78 writer and00120EF8 records. Raw evidence/hash manifest under ignored `local/task0038/wheel-render-basis/`.

## Confirmed cause

001C0D78 publishes two raw four-matrix groups into live car state. It applies spin/front steering/position; render-only side orientation is not part of this physics publication. The port NativeBackend correctly publishes those raw matrices. Before this correction, Model::apply_driving_pose copied them directly to mesh transforms while preview's existing attachment slice already included the render-only00120EF8 composition. Thus driving skipped a known original rendering stage.

00122E80 creates identity and00122E90 calls0011BA18 with0x8000, producing the source-Z half turn. First tire matrix is composed through0011B248 at00122EC0; instances1/2 use the raw input directly at00122F24/00122F7C; instance3 again composes the half turn at00122FD4. The shared blur layer follows the same instance orientation (prior E-0035-W01). Brake submission separately initializes identity00123294, writes+1 at matrix lanes0/10 and-1 at lane5 (001232B0/B4/BC), and precomposes this source-Y reflection for instances0/3 at001232CC/00123328. Other instances use their original matrices directly00123388/001233D4. Original0011B248 multiplication order preserves the input translation when the render-basis matrix is the first argument.

This is not a guessed per-side rotation: both preview and driving now call the same address-anchored render slice, with the original two distinct tire/blur versus brake basis rules. Raw native wheel snapshots and physics matrices are unchanged.

## Implementation

`CarRenderInfo::Sub_00120EF8(const port::CarAttachmentMatrices&)` owns this bounded render-stage composition. Existing preview helper `port::Sub_001C0D78(config,phases,steering)` constructs the same raw matrices as before, then delegates to this shared slice. `Model::apply_driving_pose` validates the entire raw publication and applies that slice once before replacing mesh transforms. Repeating a publication recomputes from raw input, so transformations cannot accumulate.

A related preview group-selection defect was exposed by the new all-layer parity fixture: Model::advance_wheels assigned the blur matrix to brake parts, although the helper already produced the correct distinct brake reflection. Group selection now consumes matrices.brake for brakes and matrices.blur for blur. Tire preview calculations and their existing expected nonzero-phase matrices are preserved.

No Godot coordinate conversion or Ps2VehicleRenderer patch is needed; source-to-host matrix conversion continues through the existing shared renderer. The raw input contract is explicit in model.hpp and CarRenderInfo.hpp.

## Validation

```powershell
cmake --build --preset native-debug --target vehicle_model_tests car_render_binding_tests -j6
ctest --preset native-debug -R '^(vehicle_model|car_render_bindings)$' --output-on-failure
```

Passed2/2,7.60s. The first parity attempt correctly failed due to the existing brake/blur group-selection defect described above; correcting that original-evidence inconsistency made all layers pass.

New checks cover all four raw-zero-spin driving mesh bases versus static preview, independent nonzero-spin/steering matrix examples (catching multiplication on the wrong side), unchanged wheel-center translations, preserved raw input arrays, distinct original brake reflection, repeated publication without accumulated correction, and existing invalid-publication atomicity. Existing rolling preview tests still pass. Resource-backed vehicle_model uses the player's original911TURBO geometry/configuration.

Godot relink/host and visual validation are coordinated by the root agent. No original-runtime numerical equivalence or Verified promotion is claimed. Continue TASK-0038 with the user's DrivingLab visual checks and existing original comparison prerequisites. No commit created.

# TASK-0050: vehicle scale investigation

## Result and scope

The inspected Elise render geometry, original configuration wheel locations, original collision volume and current native body transform agree in source units. There is no evidence for enlarging the vehicle to compensate for the reported track/sign appearance. No production source was changed by this investigation. This is a bounded static and host-data measurement, not original-runtime Verified equivalence; it does not by itself identify the track-side or perspective cause.

## E-0050-V01: original asset measurements

Read-only inputs:

- `GLOBAL/GLOBALB.BUN`: SHA-256 `6fad4bcf13ad8936b240596c5a7d782ccbc255f5d9fb202df785f2b6b76a2153`.
- `CARS/ELISE/GEOMETRY.BIN`: SHA-256 `fb0ebdf13140d04ed4f6515753d136d42491ca9441b87bcc3ad76ecb77d4932e`.
- Control `CARS/911TURBO/GEOMETRY.BIN`: SHA-256 `72d99dad3e95f6ba4d8e01e31d54cb8c6a1bf9a51198fb43090a10e571f86ddd`.

Original catalogue entries41 `ELISE` and42 `ELISEHP` both resolve prefix `ELISE` and the same `CARS/ELISE/GEOMETRY.BIN`. Measurements use the existing original packet float decoder, LOD0 body binding and original `_CV` loader. Collision-volume coordinates are corrected once for configuration COG by the original `FindPipelinePolyhedron` path (Debug00213C98); this translation does not alter extents. Bounds below include all decoded surface vertices of the bound body solid, not unrelated scenery or LOD spacing. Source axes remain longitudinal X, lateral Y, vertical Z. Values are source units; no external real-world dimensions or assumed meter conversion were used.

| Asset / configuration | X extent | Y extent | Z extent |
| --- | ---: | ---: | ---: |
| ELISE_A rendered body LOD0 | 3.83825 | 1.76197 | 0.981344 |
| ELISE_CV original collision volume | 3.88961 | 1.80214 | 0.875796 |
| 911TURBO_A rendered body LOD0 | 4.41594 | 1.90367 | 1.12096 |
| 911TURBO_CV original collision volume | 4.46280 | 1.90551 | 1.10942 |

Elise visual body ranges are X[-1.94479,1.89346], Y[-0.880985,0.880985], Z[0.00210024,0.983444]. Original COG is[-0.2,0,0.28]. The CV uses a simplified envelope and need not reproduce body bounds exactly. Its length differs by approximately1.34% and width by2.28%; this excludes a large uniform vehicle scaling discrepancy in this measured pair.

Elise configuration wheel vectors at row+0x120+i*0x20 have positions:

| Wheel | X | Y | Z | Radius field+0xc |
| --- | ---: | ---: | ---: | ---: |
| 0 | 1.108 | 0.637 | 0.145 | 0.299 |
| 1 | 1.108 | -0.637 | 0.145 | 0.299 |
| 2 | -1.223 | -0.666 | 0.145 | 0.317 |
| 3 | -1.223 | 0.666 | 0.145 | 0.317 |

The longitudinal center separation is2.331; front/rear lateral center separations are1.274/1.332. Renderer static tire translations reproduce these four XYZ vectors exactly. Basis diagonals are(-1,-1,1) for wheels0/3 and(1,1,1) for1/2: mirroring/half-turn changes orientation, with no scale. `ELISE_TIRE_FRONT_A` packet-space X/Z diameters are0.607779/0.611198, close to the configuration front diameter0.598; decorative mesh extents and physical tire radius are separate original inputs. Rear geometry lookup can use a fallback binding; the probe does not assert an independently named rear solid exists.

A private executable also initialized the actual native Debug driving backend for ELISE, ELISEHP and911TURBO with their original geometry. Each published body rotation basis had lengths[1,1,1]. This confirms the current publication does not scale these bodies. Private probe source/output: `local/task0050/vehicle_scale_probe.cpp` and `.txt`, compiled separately against existing libraries; no shared build was run.

## E-0050-V02: original ordinary car render transform

Authority: Retail `SLUS_203.62`, SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`. Fresh read-only Ghidra queries record:

1. `00124060` ordinary car render caller passes position at Car+0x50 and rotation at Car+0x90 into `00120EF8`. Its alternate selected pose uses+0x500/+0x4c0. Wheel matrix construction is called separately at00135FB8.
2. `00120EF8` copies all four rows of the supplied rotation matrix, multiplies by global002E0270 through0011B248, then overwrites translation XYZ with the supplied position and W with1. It multiplies this resulting model matrix by the view matrix for clipping/submission. There is no extra scalar conversion in this ordinary body-model preparation.
3. Direct references to002E0270 are its render read00121128, initialization0011DBF0 and a data reference. Initializer0011DB90 calls0011B1B0 with002E0270. The latter writes exact identity: diagonal float1, all off-diagonal and translation entries0. Initial ELF memory at002E0270 is zero because this is runtime-initialized storage; initial file bytes alone would be misleading.
4. `0011B248` performs matrix multiplication via the original VU instructions. `0010CCB0` consumes the already composed matrix to clip solid bounds, without installing a car-specific unit conversion.
5. Retail `eStripData::GetXYZ0011C2B8` copies the three float words obtained from0011BF98 directly. No position divisor or multiplier appears in that accessor.

Raw query records: `local/task0050/retail-carrender.json`, `retail-carrender-caller-b.json`, `retail-car-axis-matrix-xrefs.json`, `retail-car-axis-matrix-init.json`, `retail-car-axis-matrix-constructor.json`, `retail-matrix-multiply.json`, `retail-solid-matrices.json`, `retail-getxyz.json`, `retail-vertex-pointers.json`. These names are ignored local captures; durable observations and exact addresses are retained here.

Debug `NFSHP2.ELF`, SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`, remains the vehicle-driving authority. Fresh Debug Wheel constructor001A16E8 confirms the WheelInfo pointer is retained as WI. Existing E-0044-S01/S03 establishes WheelInfo/config layouts and the physical contact chain; this investigation does not reinterpret a Retail struct as Debug. An initial lead querying Debug001A3F10 resolves TwoWheelAckermanSteering and is explicitly not used as wheel evidence;001A3F10 is the historical Retail wheel constructor address.

## Current adapter comparison and remaining checks

The vehicle adapter uploads the decoded vertex XYZ with an orientation-preserving axis permutation `(x,y,z)->(y,z,x)`. Body parts use identity local transforms; driving publishes the native pose through the same permutation. Wheel local poses preserve the original source translations and orthonormal basis. The solid-header matrix is separately retained metadata and has not been added as a second vehicle transform. These are current source observations, consistent with the measurements above; no new source behavior is proposed.

Perspective remains a possible contributor to a single screenshot: a foreground sign can project larger than a farther car. Camera field of view and follow distance are port presentation choices, not evidence of an original unit conversion. This investigation has not measured screenshot sign/car depth and cannot assign perspective as the cause. Track instance placement and geometry transforms must be audited separately before any correction is selected.

## Host projection regression checks

After the separately owned E-0050-U02 projection correction, private Godot4.4.1 headless runs passed both original-asset driving and TrackViewer smoke tests. `driving_projection_checks.gd` measures the actual active camera projection horizontal angle from `m00`, then tests engine Camera3D projection matrices in attached SubViewports sized800x600,1280x720 and600x800. Every horizontal angle must equal `0x32DC * 360 / 65536 = 71.52099609375` degrees; vertical angles must follow the aspect-derived tangent relation. This tests the engine aspect policy rather than simply repeating a camera field assertion.

The existing original-asset driving lifecycle test also verifies failed startup preserves the preview's FOV, keep-aspect and projection coefficients, and returning restores all three. TrackViewer tests the same successful round trip. Private captures: `local/task0050/driving-projection-smoke.log` (`DRIVING_PROJECTION_OK`, `DRIVING_ASSET_SMOKE_OK`) and `local/task0050/track-projection-smoke.log` (`DRIVING_PROJECTION_OK`, `TRACK_VIEWER_SMOKE_OK`). No shared build or C++ source change was needed. The TASK0049 exact C++ source review still passes without a successor, baseline refresh or CLI change.

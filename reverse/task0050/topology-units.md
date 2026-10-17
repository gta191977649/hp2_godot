# TASK-0050: topology, start and path coordinate units

## E-0050-U01 — read-only cross-check

Scope: inspect original bytes and existing recovered bodies; no source changes or builds. Authority is Retail SLUS_203.62 SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2 for track assets and Debug NFSHP2.ELF SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b for the existing driving/skid chain. This is a coordinate consistency check, not original-runtime visual verification.

The established Retail TrackPolygon decoder001514A8 uses signed XY/8, commonZ+signedRelativeZ/256, and multiplies decoded Z by4 only for flags4. This is packing precision, not a world-to-physics scaling stage (E-0048-T01). Marker00153608 stores float XY directly; TrackPathManager path points likewise store float XY directly. Current TrackEnvironment passes marker XY directly to the original elevation query and path projection. No factor128 or4 intervenes.

An independent byte-only Python inspection of uncompressed original TRACKB11 and TRACKB21 read chunk34146 start markers,34149 path points and34132 polygons. Ignored numeric output: local/task0050/topology-units.json. Convex XY containment below is an inspection calculation, not a substitute implementation of original query ordering.

| Asset | Forward start XY | Nearest stored path point XY | Eligible containing polygon | Terrain/flags | Four perimeter lengths |
| --- | --- | --- | --- | --- | --- |
| TRACKB11 | -98.080917,-732.541687 | point82: -98.602310,-730.119751 | 8874 | 1/16 | 10.101064,5.099020,9.953027,5.003124 |
| TRACKB21 | 636.895325,-1264.051514 | point0: 610.917847,-1288.590576 | 14135 | 1/16 | 10.111205,8.055472,9.934751,8.047710 |

TRACKB11 polygon8874 vertices are (-100.125,-734.75,-89.601563),(-90.25,-736.875,-89.617188),(-89.25,-731.875,-89.617188),(-99,-729.875,-89.601563). TRACKB21 polygon14135 vertices are (636,-1261.875,-83.457031),(629.125,-1269.25,-82.695313),(635.125,-1274.625,-82.695313),(641.875,-1267.375,-83.453125). Polygon edge lengths do not establish complete road width; do not interpret them as lane widths.

These original starts lie within original decoded road polygons in the same coordinates as original path points. A uniform track-only scaling would invalidate that alignment. The renderer worker independently found Track11 render road RD_SECTION70_CHOP1 within0.008874 of topology vertices, and RDDRT_SECTION70_CHOP1 within0.021290; both use original instance flags0x200 identity placement. Thus matching material overlays are not the only evidence: independent float render geometry and separately packed topology agree numerically near the start.

## Skid coordinate precision is not world scale

Existing recovered Debug SkidSegment::SetPoints001A7E98 stores the center point as three unchanged floats and compresses only the width/delta vector by trunc(delta*64) to signed bytes. GetPoints001A7F08 reconstructs that delta with1/64; GetEndPoints001A7F80 adds/subtracts it from the float center. SkidSet::Render001A83B8 submits these endpoints plus the original small Z offset. There is no center-point world scale128 in this chain (E-0046-S01/S02). Renderer fixed-point projection constants, if present, cannot be assumed to change native world units without tracing their inverse/projection context.

## Conclusion and limits

Evidence does not support correcting topology, paths, starting positions or all track coordinates with a guessed uniform scale. Existing native911TURBO body bounds measured by the parallel vehicle probe are4.41594 by1.90367, versus collision-volume bounds4.4628 by1.90551; that comparison belongs to the car/renderer investigation. The retained track measurements only establish consistency among track coordinate representations. They do not prove final wheel mesh transforms, camera perspective or the complete rendering transform chain correct. No repair is proposed from this topology slice alone.

## E-0050-U02 — original driving camera differs from the tool camera

Read-only Debug Ghidra queries captured in ignored local/task0050/debug-camera-0015BD10.json (lowercase filename),0015BFE0,0015C2B0,001042E8 and debug-type-POV/CameraInfo/CarTypeInfo/POVTypes.json. No camera implementation was changed.

DriveCameraMover constructor0015BD10 selects CarTypeInfo.Camera, falling back to CarTypeInfoArray[0].Camera if either full-screen or split-screen POV count is zero. GetPOV0015BFE0 selects the split-screen array for EVIEWMODE_TWOH and the full-screen array otherwise. Debug metadata identifies Camera at CarTypeInfo+0x3C0, full-screen POV array at Camera+0x10,32-byte POV records: Type+0,Lag+4,Height+8,LatOffset+12,Angle ushort+16,MaxAngle ushort+18. POVTypes explicitly names BUMPER0,INCAR1,OUTSIDE2.

Direct original GLOBAL/GLOBALB.BUN byte records, consistent with the established CarTypeInfo asset layout, gave the following identical full-screen values for911TURBO at file0x93D30 andELISE at0x947F0 (both full/split counts3/3):

| POV | Type | Lag | Height | LatOffset | Angle | MaxAngle |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | BUMPER | -0.716 | 0.79 | 0 | 1400 | 0 |
| 1 | OUTSIDE | 5.8 | 1.7 | 0 | 600 | 9158 |
| 2 | OUTSIDE | 4.7 | 1.7 | 0 | 800 | 9158 |

Raw output retained at local/task0050/camera-pov-records.txt. These are source offset parameters, not a claim that original final camera distance is simply Lag: Update0015C2B0 also applies acceleration lag, car yaw/pitch response, lookback interpolation, road noise, and wall/elevation constraints. It reads Lag/Height/LatOffset from the selected POV unless original UseTweakables overrides them. Original camera pose is not determined from render bounds.

Update0015C2B0 assigns CurrentKey.FieldOfView=0x4000 for BUMPER/INCAR and0x32DC for OUTSIDE, provided Camera_StopUpdating is zero. With65536 angle units per revolution, these are90 and71.52099609375 degrees. CalculateH001042E8 computes `(ScreenWidth >> 1) / (sin(FOV/2)/cos(FOV/2))`, establishing horizontal FOV. Existing E-0046-S02 CreateViewMatricies0010EA18 passes CurrentKey.FieldOfView to this calculation.

The port tool godot/scripts/debug/driving_lab.gd explicitly uses camera.fov60, default pitch15 degrees, target at car render-bounds center, and distance=max(4,bounds.size.z*1.5). These are tool choices, already documented in its source as not the original camera. At a16:9 viewport and vertical-FOV convention,60 degrees vertical corresponds to91.49284452 horizontal; the original OUTSIDE71.52099609 horizontal corresponds to44.10559670 vertical at the same aspect. These conversions are geometric comparisons, not evidence of the original PS2 display aspect or a proposed replacement value.

A shared camera can change perceived track/car proportions across all tracks: projected relative sizes depend on depth and camera position, and a wider FOV with a different follow pose changes the perspective. The measured camera-policy difference is therefore a concrete common presentation discriminator. Without matched original and port frames with the same car, position, POV, viewport and aspect, it remains unproven that this difference explains the user's full observation. Do not rescale assets or apply a guessed camera correction on this evidence alone.


### Implemented projection correction

The observations above describe the pre-fix tool policy. TASK0050 now uses the exact
OUTSIDE0x32DC angle as a horizontal Godot FOV (KEEP_WIDTH); it saves/restores the prior
aspect policy with FOV. This is an evidenced projection adapter, not a guessed scale
factor or full DriveCameraMover implementation. Existing bounds-based follow position,
pitch/orbit and recentering remain tool behavior. Active camera and SubViewport engine
matrix tests confirm horizontal FOV for4:3,16:9 and portrait. No matched original frame
is available to assign this correction as the sole cause of the user's observation.

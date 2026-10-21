# TASK-0072 visibility audit

## Identity and evidence

Read-only Ghidra MCP through tools/ghidra.py, 2026-10-19. No database writes.
Retail SLUS_203.62 SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Debug NFSHP2.ELF SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Executable revisions otherwise unknown. Fresh configured ELF hashes match; private
identity and complete query/result wrappers are in local/task0072/ghidra/.
This is static evidence and a source audit, not original-runtime verification.

- E-0072-V01: Debug Camera constructor001566E0 and memory00324F90
  bytes00401c46: CurrentKey.NearZ=.5 and FarZ=10000.
- E-0072-V02: Retail CreateViewMatricies0010E638 / Debug0010EA18,
  Debug CalculateViewMatricies00106F18 and its callers, eDisplayFrame00112C60.
  Main view IDs1/2 rescale near by baseline/current half-FOV tangent unless mover
  type1 or5; far preserves far-minus-near span. Nonzero force arguments apply
  afterwards. Screen far has its own override, separate from view.FarZ.
  Ordinary first frame stage passes zero forces, sky stage passes near100 and
  far from00324A44 (bytes00504347=50000), later stage can use EnvMapScreenZ
  as force_screen_far_z. These stages are not one global distance setting.
- E-0072-V03: Retail DrawAScenery0014FE18 / Debug0014FA68,
  Retail WhatSections00150950 / Debug001505A0,
  Retail TreeCull00150378 / Debug0014FFC8. Fresh reread agrees with prior
  E0058R01/E-0051-S01 bounded native selection. No missing ordinary cull
  condition identified in the audited source.
- E-0072-V04: Debug named Camera Update methods listed in camera-names.json;
  fresh camera-<address>.json captures cover SetCameraMatrix00156868 and the
  fifteen Update entries001572C8..0015E950 returned in that bounded range.
  RearViewMirrorCameraMover00158AB0 writes CurrentKey.FarZ192/NearZ.75.
  TrackCarCameraMover00159040 writes NearZ.5. DriveCameraMover0015C2B0
  writes either NearZ from003250EC (cdcccc3e=.4000000059604645) or.5.
  SetCameraMatrix reads current/previous clipping keys to derive velocity;
  it does not establish every writer. No named SetNear/SetFar result was found
  in the bounded function-name search; inlined writes remain possible.

## Confirmed source integration

Ps2TrackRenderer main adapter uses viewID1, selected mover type, host horizontal
FOV converted to original angle, constructor input.5/10000 and explicit Debug
force-far override. Effective near/far are then applied to host projection and
FrameView. CalculateH uses original ScreenWidth512, independent of output pixels.
Native selection is zone shortlist -> section tree/AABB -> instance AABB/flags ->
forward-radius -> integer projected-size cutoff2 -> first eligible nonnull of three
model slots with each FarClipSize. Radius is authored radius+4. FarClipSize is a
projected-size threshold, not a world distance. The no-zone fallback is broad by
original design; loaded geometry is not submitted geometry. Show LOD explicitly
bypasses instance selection and must be off in performance acceptance.

Scheduled reflection captures have independent FrameView clip matrices/H and
near/far, while particles use the current frame near/far and native projection.
Sky uses its distinct supported depth100..50000. Existing environment-camera
carrier defaults.5/10000 are not proof of every original capture key writer.
Current diagnostics expose zone/selected_sections/considered_instances/
selected_instances/surface_commands independently for each submitted view.
They do not yet count individual AABB/flags/size/LOD rejections.

## Inferences and open questions

No evidence licenses shortening global far distance to achieve60FPS. Most excess
work should first be measured after the already-present selection and GPU sync.
The source audit is not an original matched-frame command oracle.

Full active Track11 camera/coordinator selection, all CameraKey writers (including
inlined/cinematic keys), Retail correspondence of newly examined Debug mirror
behavior and every reflection camera override remain unresolved. Default10000
must not be labeled the actual universal original gameplay render distance.
The port free-browse camera is an adapter, not original camera gameflow.
No recovered behavior is changed by this audit; no Verified claim.

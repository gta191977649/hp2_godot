# E0058R01 - Native visibility integration, no host LOD management

Retail SLUS_203.62 SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Debug NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b supplies names/types.
Fresh read-only Retail CreateViewMatricies0010E638, ClassifyAABB0010CCB0,
DoCulling00150D98, eCreateView001070C0 (caller at0010712C) cached privately
local/task0058. Prior fresh WhatSections00150950/TreeCull00150378/
DrawAScenery0014FE18 evidence E0057R01 remains authoritative.
Debug paired CreateViewMatricies0010EA18 confirms source name/signature and
CurrentKey NearZ/FarZ/FieldOfView; constructor Camera001566E0 sets near0.5,
FarZ from00324F90. Existing E0052E03 records far10000 and mover adjustment.
Retail GP003358F0 signed9F60/9F64 resolves0032F850/854: fresh memory reads
both0000003F =0.5, initial ClipVolumeScaleX/Y. No write operations performed.

Original chain preserved: zone->loaded-section shortlist->64-entry spatial tree
AABB classify->instance AABB/flags/forward-radius->integer projected size<2
reject->first nonnull qualifying original model->StuffScenery/eModel queues.
Three slots do not imply three distinct models. Nozone fallback is broad by design.
View H comes from original CalculateH with logical ScreenWidth512, not1080p width.
Native classes remain engine-independent. Host converts Camera3D camera/world clip
matrix with native XYZ/hostYZX axes and matching homogeneous[-w,+w] planes.
No empirical distance shortening, generated mesh LOD or physics substitution.

User revoked host LOD policy. Constructor/configuration accept original only;
old godot_hlod fails explicitly. Default main and scheduled independent capture
frames call Runtime::prepare_frame and original culling. Renderer no longer applies
nonzero engine LOD visibility ranges. Show LOD is retained as explicitly requested
browse diagnostic: all authored levels, ranges0, intentional selection bypass.
Descriptor/range helpers remain for diagnostics/historical unit evidence, not a
production alternate backend. Removed obsolete HLOD host CTest registrations.
Culling diagnostics per view expose zone, selected sections, considered instances,
selected instances, surface commands, native-selection flag. Resident meshes are
not actual draws. Existing caches and material identity stay intact.

Limits: finite ordinary-float projection, not instruction-level VU/EE bit emulation;
no matched PS2/PCSX2 frame oracle and no Verified claim. Tool free camera/aspect
conversion is port-side; full original camera gameflow and every camera override
are not declared recovered by this integration. The default supported view/depth
branch is established; no unknown exceptional branch is filled with guessed values.

Fresh Debug00324F90 bytes00401C46 decode float10000; Retail ClipVolumeScale
bytes and paired constructor defaults saved local/task0058/view-constants.json.
Native culling bodies unchanged; this increment closes host bypass/integration.

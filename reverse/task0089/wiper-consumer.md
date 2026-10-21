# TASK0089 original rigid wiper consumer

Authority: Debug NFSHP2.ELF SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`,
version independently unknown. E0089W02/E0089W04 in water-research.md.
Private complete captures: local/task0089/wipers/render-complete-code.txt,
0010deb0.txt, 0010df98.txt, 0010e048.txt, 001a4cf8.txt and eSolid-type.json.
Original assets remain external. No original-runtime equivalence or Verified claim.

Confirmed: Render001210F0 draws two optional Wipers eModels only with nonnull
car, integer GetPixelSize(view, position,3.0)>40, and state!=0 or
OnTime<=WorldTimeFrames<OffTime. A missing solid/rigid animation is skipped.
Each start word becomes (OnTime&0x3fff)|0xc000. GetRigidAnimationTime0010DEB0
uses age=(WorldTimeFrames-start)&0x3fff and loops age modulo NumFrames*scale.
GetMatrixAtTime001A4CF8 selects floor(age/scale)%NumFrames, with no interpolation.
Render first multiplies its body matrix by Solid.PivotMatrix; Setup0010DF98
then multiplies that matrix by the authored animation frame using bMulMatrix002541C0.

LoaderSolidList00103588 aligns child34012 payload to16. Header is48 bytes:
NumFrames signed16 at0, TriggerableFlag signed8 at2, FrameRateScale signed8
at3, NameHash at4, runtime label pointer at8, Name32 at16, matrices at48,
64 bytes each. eSolid.Name is24 bytes at0x10; ExternalRigidAnimationNameHash
is0x28, PivotMatrix0x60. External animation references use original solid hashes.

Port projection: VehicleSolid retains matrices separately from geometry.
The decoder resolves same-bundle external hashes even when geometry filtering
omits the referenced solid. An unresolved external binding or cycle remains explicitly flagged in solid metadata
and geometry gaps/model diagnostic, allowing passive geometry to load. An active
wiper consumer requiring that animation throws NotImplemented before publication;
it does not fabricate motion. Nonpositive count/rate and
nonfinite/truncated frame data are explicitly rejected. Global/external bundle
binding and original label-frame APIs remain outside this slice.
Model includes original constructor offsets190/1a8 WIPER_LEFT/RIGHT resources,
keeps them hidden without live state, and uses apply_wiper_state(WiperSnapshot).
Part local transform is PivotMatrix*frame; the renderer retains body/world ownership.
Model.select uses the existing renderer projected size and strict >40 gate.
Constructor0011EA88 attaches the first two SkinReplacementTextureTable pairs
and replaces light material D6D6080A. Newly loaded911TURBO wipers exposed
textureECE04847 absent from bound CARS/GLOBAL packs. Counterevidence to a
strict missing-texture failure: FixTextureTable0010C1B0 calls
GetTextureInfo001A95C8(hash,1,0); after pack/render-target lookup fails it
returns original default texture19006835, or breaks if that is also absent.
The bounded wiper consumer loads that actual original default and maps missing
wiper texture references only. Other existing vehicle texture policy is unchanged.
Private captures0010c1b0/001a95c8 under local/task0089/wipers.
Wipers have no generated geometry, phase accumulator or per-frame asset IO.

Original asset inventory:911TURBO/CROWNVIC left+right41frames/scale2;
ELISE right-only61frames/scale2; BMWM5COP no wiper solids. Missing optional
resources do not hide body geometry. vehicle_wiper_tests covers this inventory,
On/Off boundaries, explicit state, pixel40/41, hold2frames, loop82/122,
14bit wrap, exact authored-matrix composition and no per-frame IO.
Synthetic tests cover layout, bounds, exceptional divisors and unbound/cyclic hashes.
Validation results are recorded by the integrating task after builds complete.

Validation,2026-10-21: cmake --build build/native-debug --target
vehicle_wiper_tests vehicle_geometry_tests vehicle_model_tests -j6 passed.
ctest --test-dir build/native-debug -R '^(vehicle_wipers|vehicle_geometry|vehicle_model)$'
--output-on-failure passed3/3 in9.95s. CMake3.29.2; native compiler C:/Strawberry/c/bin/c++.exe, GCC13.2.0 MinGW-W64. PC checks establish bounded behavior, not PS2 equivalence.
Integration owns host lifecycle and visual checks. No benchmark was run.

Integration regression: the shared geometry decoder is also used for passive
track solids. Immediate resolution of every external animation incorrectly
blocked race scene loading although ELISE itself has no external rigid hashes.
Resolution now preserves explicit unresolved metadata and diagnostics for passive
solids. Only active wiper animation consumption requires that dependency to be
bound and throws before publication if absent/cyclic. A test mutates an actual
911TURBO wiper external hash: inactive model loads; activating rejects without
changing selected parts. Rebuilt the same three native targets;3/3 passed11.87s.
Host race lifecycle rerun belongs to the integrating agent.

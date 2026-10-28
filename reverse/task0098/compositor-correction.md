# TASK-0098 compositor counterevidence

E-0098-F03, Debug NFSHP2.ELF SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
revision/date unknown. Fresh read-only MCP captures and their manifest are ignored
under `local/task0098/compositor/`. This corrects earlier E-0098-F01 prose; it does
not implement the compositor or establish original-runtime equivalence.

## Confirmed draw/flush order

| Stage | Submission | Flush |
|---|---|---|
|0|Main cars, views1/2, main1/reflection0/solid1/alpha0|Solid and alpha lists|
|1|Sky with global ZWrite0, then ZWrite1 and view0 layers24..20|Both lists after sky, then after each layer|
|2|View1..3 platform callbacks, FrameEffects before WorldEffects|Solid list only; alpha remains pending|
|4|Reflected cars, then main-car alpha|Solid list only; alpha remains pending|
|5|No replacement flattened-image pass|Accumulated alpha list|
|7|View0 layers15..0,19..16,28..25|Both lists per layer|

`eDisplayFrame00112C60` stage4 call passes reflected solid1/alpha1, but
`RenderFrontEndCars00123EA8` intrinsically sets0x11 on reflection:
ERF_DISABLE_ENVMAP1 and ERF_IGNORE_ALPHA16. `RenderStrips0010E120` suppresses
ApplyAlphaSorting strips for bit16. Therefore earlier wording "reflection
solid+alpha" and treating all alpha as flushed in stage4 are superseded. Stage5
must preserve a shared sort domain for pending platform/effect/main-car alpha.
Reflection is gated by EVIEWMODE_ONE and matching view ID and is separate from
the environment-map material next_pass.

SetupRenderTargetPtrs00112C00 requires initialized active targets; target1 is
conditional on QUINCUNX and distinct active pointers. Runtime table002C7410 is
zero-filled in the ELF. AttachSolidPolys0010BEF0/AttachAlphaPolys0010BBF8 choose
contexts from original texture fields and global ZWrite. Alpha defaults to
MatrixPack1 in frontend; solids use MatrixPack0. Alpha sort derives from transformed
AABB depth; Flailer ePoly0010FE48 uses layer-depth table002C1530. Extra stage0 car
pass/target6/blend-back has its own NumPasses/view/fullheight/multisample gates.

## Current backend boundaries

Frame stage1/7 ranges alone do not carry native depth/target/context/sort state.
The single-canvas frontend and ordinary vehicle materials cannot yet execute the
original shared-depth/shared-alpha domain. apply_frontend_pose does not yet
consume Reflection's intrinsic flags. The Retail texture-state projection is not
by itself a Debug context adapter. These are required implementations before
the formal selection route can be enabled without presenting unsupported output.

One investigation note called all loader requests Release-disabled based on an
error string. Source inspection counters that inference: the shared
Ps2VehicleRenderer base's requests_enabled() returns true; viewer subclasses may
override it. Production should use that shared backend rather than inherit a
viewer restriction. This correction asserts no complete production binding.

Unknowns: final target initialization, full clipping/blend behavior and original
runtime output. No silent viewport sandwich or guessed sort/depth replacement.

## E-0098-RB01 bounded reflection correction,2026-10-28

The intrinsic0x11 flags now reach the direct host vehicle adapter: envmap
next-pass disabled and ApplyAlphaSorting strips make no colour/depth contribution.
See reflection-binding.md for fresh type/listing evidence and GPU tests. This
supersedes only the earlier sentence about ignored Reflection flags; shared
physical targets, pending-alpha executor and actual page binding remain open.
The strict production roundtrip test still FAILS at001F6A28.

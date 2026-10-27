# TASK-0098 selection page ownership checkpoint

Authority: Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
revision/date unknown. Readonly Ghidra captures in ignored `local/task0098/page-ownership`
and `page-services`; SHA-indexed in evidence.json. Retail assets remain separately
adapted, not a claim of Debug/Retail layout or whole-version equivalence.

## Confirmed original behavior

E-0098-O01: VisibleCarSet001920C0 independently copies nodes in forward order,
shares CarStatus pointers, copies skin/thumbnail/type/unlock/index, maps the source
current node (first-node fallback), and copies counts without sorting. The temporary
source list may die after selector construction. AnimationGroup00241E50, reached
from CarLockedDisplayer00196908, restores saved integer group X/Y, including the
loss of fractional authored coordinates; it does not restore visibility or alpha.

E-0098-O02: UpdateInfoBarAtTop001953B0 uses Control.SpeedLimiter at car+0x320,
with metric multiplier003255F8 (bits3FCE147B), then CVT.W.S. Existing finite host
conversion is retained; original FCSR rounding is not verified. HighPerformance
uses speed/BHP plus and acceleration minus suffixes. COP/cloaked hides eight
widgets, including three accessory hashes. GetCarSpec0To60/BHP001860E8/00186128
use numeric language IDs. Getters precede label translations. Class is signed+1.
Counter always formats NumberUnlocked/TotalNumCars, then hides for UseTreeEvent.
Flailer SetString00140C10 compares the full incoming string before bounded copy,
sets dirty5/LanguageID0 only on change, and reserves the final NUL byte. Repeated
long input remains a change even when the truncated result equals current text.

E-0098-O03: CarSelector00192B78/00193278 compose the single-player constructor
and explicit normal close in port::selection::SelectionPage. The owner binds
existing recovered state, copied car list, three cached alpha groups, locked
display, main/reflected render states, camera,22 input registrations, load leases,
cover/platform model leases, spotlight buffer extents and20 nebula registrations.
Input registrations receive the real native callback; cancellation leases remove
it before state disposal. Constructor calls UpdateNebula once and Tick once.
Tick never advances particles: after_display uses original fixed timestep00324A48
and shared FrameEffects pools/RNG after drawing. The caller initializes camera
BSS projections once per fresh Debug memory image, not on every page reentry.

Explicit close removes page emitters, kills ALL groups in that FrameEffects
system, dismisses PopupID, releases platform, closes swatches, preloads TWICE,
unloads current load leases, clears both shared OverrideModel pointers, destroys
camera/main/reflection/cover leases, unregisters input in forward order, releases
spotlight buffers, stops sound12, restores locked coordinates, then restores
snap/orbit/fade groups and releases copied nodes. SetEmitterState0016B670 is a
confirmed JR RA/NOP. FrontEndRenderingCar OverRideAlpha0011EA78/RestoreAlpha0011EA80
are also empty; no speculative alpha restoration is added.

E-0098-O04: FrontendDatabase wrapper00182430 tail-jumps to00192580, rather than
being empty. PreloadCurrentSelectedCars rebuilds each active player's VisibleCarSet;
only changed types unload the previous type, update CurrentPreloadedCarTypes,
set loader InFrontEndFlag/RoundRobinFlag1, load(type,-1,0,0), BeginLoading. Calling
it twice is normally idempotent but both call sites remain. Unload001926E8 visits
both slots regardless of current NumPlayerCars, unloads non-NONE then stores NONE.

## Port lifetime policy and tested scope

Points and original spotlight palette extent require explicit bindings (zero is
valid but never inferred). UseTreeEvent derives from original GameType8/9.
Required host PageLease resources are synchronous ownership/cancellation seams,
not original PS2 objects. Their destructors must cancel pending work, fence rendering
where needed and never throw; referenced services/screen/catalog/pools outlive the
page. No asynchronous model job receives a page pointer. Actual host backend
generation cancellation is still unbound; the page test uses service fixtures.
Normal callers invoke close; destruction without close is an abnormal port rollback,
which releases owned handles and emitters without successful-exit preload/audio.
A failing close enters a port closing state: Tick/input and repeated close are
rejected, then abnormal disposal releases remaining leases; it does not report a
successful original exit. Partial construction restores cached groups and clears shared overrides before
resource destruction. This is port exception safety, not recovered PS2 exception
behavior. Recovered constructor/destructor records remain Decompiled/partial.

ParticleSimulation add_auto_emitter binds original AddAutoEmitter001275F8 to stable
host handles/shared RNG. Detached unreferenced slots can be reused; detached slots
with live groups cannot alias new emitters. Teardown trims dead trailing storage
without removing unrelated registrations or resetting RNG. Original initial emitter
pool600 is not a hard cap (602 registrations tested). Appending auto emitters after
unbound dynamic-copy storage explicitly throws001275F8; general mixed registry
support is not claimed. Shared dynamic TriggerEffect remains an explicit gap.

selection_page tests delayed first load, switch while loading, explicit exit with
pending request, both input ports, after-display shared pools, unrelated registration,
30 close/reentry cycles and injected failure after registration. It does not test
actual GPU uploads, window output, native speech/help ownership, races or saves.
Existing widget/locked/effect tests add copy, formatting/order/truncation, fractional
coordinate restoration and registration limit/reuse checks.

## Still unresolved

Formal event entry remains throwing. Required production loader/model/input leases,
original help and speech services, localized rectangle rendering, shared-depth
GPU compositor and valid StartRace handoff must be bound before enabling it.
The whole loading lifecycle, full World, police/rules/results/progression/save,
original-runtime comparisons and1920x1080 performance acceptance remain incomplete.
No Verified promotion, staging or commit.

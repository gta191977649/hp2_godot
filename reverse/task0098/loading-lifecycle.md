# TASK-0098 selector Tick and loading dependency

Authority: Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
revision/date unknown. Retail asset/profile adapters remain separate.
Original queries use read-only Ghidra MCP; ignored captures are indexed in
`evidence.json`. PC fixtures are not original-runtime equivalence.

## Confirmed: E-0098-T01

`CarSelector::Tick001947D0` now has a native orchestration projection in
`indep/src/frontendscreens/selectcar.cpp`, with explicit required original
loader/palette/lighting/locked-display/nebula services. The function is not an
implementation of those services, the whole constructor, or the production page.

- Poll LoadingCar first. A completed request sets LoadedCar and clears LoadingCar.
  Only current identity immediately gets override/reinitialization. Superseded
  loads must be unloaded, rather than rendered as the newly selected identity.
- At full overlay and stationary thumbnail spline, unload old LoadedCar, clear
  it, load current LoadingCar and begin loading, in that order. Loader requests
  are original `RideInfo(type,-1,0,0)`, distinct from the chosen display skin.
- Different identity raises overlay by7; only strict overshoot clamps to130 and
  resets override. Matching identity shows at130 and decreases by3, clamped at0.
  Completion at full overlay consequently calls ShowLoadedCar twice (each call
  reinitializes main and reflection). This apparent duplication is retained.
- Orbit timeout is a strict signed comparison after32-bit wrapping subtraction.
  Timeout changes help/camera/speech before transition interpolation. Car rotation
  wraps16bits; manual orbit suppresses rotation outside choose mode. Camera stats
  use original globals; per-Tick mode increments use003255D4/D8, not elapsed time.
- Main and reflection pose update precedes alpha groups. Lighting reads current
  selected swatch's ImageData[3] indexed palette sample and uses the original
  RGB weighting and fourth power. Silhouette admission instead uses LoadedCar.
  Required palette service returns absent for an unavailable original texture;
  it must not substitute a generated colour. Then locked update, arrow predicates,
  swatches, thumbnails and nebula retain original order.
- Fresh listing00193AE0 is exactly `JR RA` with `NOP` delay slot. UpdateSpotlight
  is a confirmed original no-op; there is no missing spotlight update service.
- Original static initializer00196688 writes camera vectors after ELF mapping.
  `port::selection::initialize_camera_offsets` projects only those nine stores
  using00325608..00325618. Call once on a fresh selection Memory image, never
  every Tick. Zero-filled BSS is not an initialized C++ runtime. Other original
  initializer work (nebula colours, registration) is outside this helper.

## Native regression scope

`ultimate_selection_lifecycle_tests` loads the configured SHA-checked Debug ELF;
it explicitly reports SKIP when that prerequisite is absent. Fixtures check delayed
load, strict overlay boundary, initial duplicate reinit, request order, a superseded
completion followed by another request, moving thumbnail unload gate, strict orbit
timeout/manual rotation, transition clamp,16-bit rotation wrap, static camera offset
stores, palette lighting, partly-visible/cloaked model branches, and missing-service
failure. No synthetic race or profile outcome is produced.

Actual GPU/worker cleanup, screen reentry, resource baselines and save semantics
are not certified by these native callback fixtures. Required production services
must bind owning generations to prevent callbacks touching a destroyed selector.

## Remaining integration and next action

Complete the original constructor/destructor, FrameEffects ownership/RNG/pools,
help/speech/locked text rendering and stage0/1/2/4/7 composition. Bind the reviewed
Debug/Retail semantic profile adapter, then real event entry and valid launch.
Production event confirmation must keep its explicit gap until those dependencies
exist; do not replace selection with a model viewer or launch the single-car lab.

Independent World dependency audit reconfirmed common load00168258 ->00167F58
->00167DC0, distinct from track-specific loading. Common load excludes eliminated
drivers, separates undamaged and vertex-damaged car passes and temporarily uses
traffic skin-1/instance0. Completion duplicates instance damage/SCUFFS before specific
loading. Specific unload00168700 waits rendering, tears down coordinator/track/world
systems and waits async work; common unload00168340 separately releases INGAME and
CarLoader/duplicates before frontend loading. These are investigation-only findings,
not TASK-0099 implementations. Retain both ownership scopes in retry/return tests.

Original comparison remains blocked: no running PCSX2 process was found and
DebugServer21512/PINE28011 connection probes timed out. A runnable matching Debug
scenario and Debug-addressed paused capture protocol are required. Existing Retail
`tools/driving_oracle.py` cannot establish Debug equivalence. No Verified promotion,
integrated selection/career claim or1080p60 performance acceptance.

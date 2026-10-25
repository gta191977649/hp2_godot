# TASK-0098 selection core checkpoint

Behavior authority: Debug NFSHP2.ELF, SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b,
revision unknown. Read-only Ghidra MCP; no computer-use or database mutations.
Retail asset/correspondence authority: SLUS_203.62, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Raw captures stay in ignored local/task0096; evidence.json records capture hashes.

## E-0098-S01: car availability and visible list

Confirmed original functions: MakeEventCarsAvailable001822F0;
SetCarAvailability00185AB0, FlushAvailableCars00185AF8, GetCarStatus00185B28,
BuildRideInfo00185B88, AssignCarOfType00185C38, GetSelectedRideInfo00185CC8,
AddAllSkins00185CD8; GetNumAvailableSkins00186668/GetAvailableSkin00186688;
VisibleCar ctor00191F38, SetCurrentSkinNumber00191FB8, NextSkin00192038,
PrevSkin00192068; VisibleCarSet ctor001921D0, GetNextCar00192090,
GetPrevCar001920A8, CarListSort00192410, SortVisibleCarList00192518.

- Both players'64 statuses first become LOCKED_INVISIBLE. Only the referenced
  event cars become UNLOCKED_FULLY_VISIBLE. Packed skin zero means all; dashboard
  is bit30. Existing skin/dashboard bytes survive FlushAvailableCars itself.
- Police skins are language0 ->1, otherwise2. Normal skins use signed count/modulo.
  AddAllSkins writes only ten entries but keeps incrementing the original count.
  Host consumers explicitly reject out-of-storage access rather than clamp count.
- BuildRideInfo substitutes first allowed skin if absent, including the original
  stored slot0 at zero count; dashboard also requires a nonempty geometry name.
- Visible list excludes invisible entries unless the original override is set;
  locked entries require the real EasterEggs service. Missing service throws.
- Current game-specific car/skin is restored if present; otherwise first sorted
  car. Missing requested skin preserves current SkinIndex. Cars do not wrap;
  skins do. Original zero-count PrevSkin yields -1; caller consumption is guarded.
- Comparator uses uint32 arithmetic, unlock/COP/performance/class keys followed
  by translated full name. It returns1 for an already ordered pair, including
  complete ties. Typed Sort00196960 jumps to bList::Sort00249958, which repeatedly
  swaps adjacent unordered pairs. Equal ties preserve insertion order.
- Original Debug bStringHash0024C8B0 and existing Retail002534A8 implement identical
  unsigned-byte uint32 hash(-1; multiply33+byte); thumbnail hash reuses that helper.

Host ownership is a vector of owned stable VisibleCar nodes, allocated at page load,
not each input/frame. Sorting moves ownership pointers and retains current identity.
No intrusive-list ABI or original allocator/lifetime equivalence is asserted.

## E-0098-S02: popup state and selected-car commit

Confirmed: ctor defaults00192B78 (state projection only), AdvanceState001945F0,
DevanceState00194610, SummonPopup00194628, ScreenCallback00194E08,
AreAllSelectorsReady00196348, SetSelectedCars00196198,
MaybeContinueToNextScreen001965C0 and outer screen callback001963E8.

- Defaults SELECT/AUTO/CLASSIC/PopupID-1 do not read profile transmission/handling.
- MENUITEM_SELECT requires hash52E039F3, stateSELECT, desired mode not ORBIT,
  and unlocked/override. It opens transmission before handling.
- Popup documents41C2FDC3/6344DA01/6344DA02 depend on parent document; flags3,
  dismiss6/cancel0, original button/message IDs and restored start index retained.
- POPUP_DISMISS updates settings, checks continuation BEFORE advancing. Single
  player readiness is HANDLING, two player READY. Handling must still be chosen
  before single-player continuation. Cancel steps back without clearing choices;
  dismiss6 does not advance. Zero selectors returns ready in the original.
- SetSelectedCars commits the currently selected type and actual skin through
  AssignCarOfType(use_dash=1), maps AUTO/MANUAL/SPORT to0/1/2 and CLASSIC/EXTREME to0/1.
  Outer screen Back commits too, including returning from orbit; no rollback.

Current native projection implements confirm/popup state only. It does NOT claim
the full constructor, callback sound/Noise publication, real popup ownership,
MaybeContinue launch, orbit/held input, loader, presentation or outer Back dispatch.
Other ScreenCallback messages throw at00194E08. Production event confirmation
remains its existing explicit gap until these dependencies are connected. This
prevents a partly implemented screen or fixture from being presented as a real race.

## E-0098-S03: language/version adapter

Confirmed imported CarLanguageInfo fields, FindCarLanguageInfo00186050,
GetFullName00186088, InitCarInfo00186EB8 and DetermineAvailableCarSkins00186E58.
Debug table00304DB8..00305267 is50 rows, stride24. Init binds by loop identity,
not row.Type. DetermineAvailableCarSkins only calls the skin getter for nonempty
geometry; it does not mutate the lists in this version.

Fresh Retail00187E80/00187EB8 and table0030D578 captured separately. Retail has50
rows of stride28, an extra word at0xC. Debug FullName/ShortName offsets10/14hex
correspond to Retail14/18hex. All50 car identities and the five common numeric
language IDs match after explicitly dropping the extra Retail word. Never overlay
these structs. Static decoded IDs are source constants, not proprietary strings.

Actual Retail English language assets resolve all64 full-name bindings (including
original fallback row), and acceleration/BHP text for non-COP cars. An initial
overbroad asset check failed on43004, then44004. Original001953B0 computes displayed
top speed numerically and suppresses specs for COP cars: those unavailable IDs are
not required by that branch. Tests now follow those recovered access conditions;
no missing text was invented and no fallback added. Other locales need validation.

## E-0098-S04: presentation investigation; NOT implemented

Read-only captures establish authored camera, light, animation and loading dependencies.
See local/task0096/selectcar-agent and selection-media-agent. Camera Update0015A668
uses FOV4500bangle, fixed.03 mode step per call and cosine interpolation. Actual
Tick stats distance is12.9300003/16.5 (1P/2P), overriding initialization7.9495.
Camera::SetCameraMatrix00156868 consumes WORLD->VIEW, so its inverse determines
eye/target; matrix translation is NOT the camera world position. Coordinate/Quaternion
dependencies must be implemented/reused from reviewed original code before output.

Full CarLoader/LZC/rendering/reflection/CARCOVER/PLATFORM, AcidFX nebula,
light/swatch/thumbnail/locked-state help, speech and animation/async host lifecycle
are still required. No generic viewer substitution, guessed camera or BGM change.
SelectCarScreen ctor stops background movie, not BGM. Speech fades and selector
sound shutdown have separate original calls; complete audio-bank binding remains open.

## Validation and remaining work

Native core and original-data tests are implemented in ultimate_selection_tests.cpp.
They cover locks/list ties/bounds/reordering identity/skin wrapping,1P and2P readiness,
popup cancellation/ignored dismissal, default settings, commit, missing-service gaps,
all64 original event restrictions and reviewed language ID correspondence.

No Verified, original visual/runtime comparison, performance target or whole-screen
completion is claimed. TASK-0098 remains active. Next: exact camera/math and model
loading/presentation closure, then event entry integration and stale-load/lifetime tests.

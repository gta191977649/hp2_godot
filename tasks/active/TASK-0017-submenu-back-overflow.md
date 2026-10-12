# TASK-0017 — Submenu Back and overflowing menu content

Status: active. User asks original recovery and implementation of Backspace/previous
screen permissions and Audio Options overflow, scrolling and clipping. Preserve all
prior work. Binary SLUS_203.62 US PS2, revision unknown, SHA256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.

Scope: existing reachable frontend submenu handlers; recover permission and special
return actions, class-aware history; recover authored scrolling viewport/update/render
behavior with Audio as concrete regression. No blanket permission, guessed viewport
or invented smooth animation. Unknown reached paths raise hp2::NotImplemented.

Acceptance: read-only Ghidra address anchored records; portable C++ behavior tests;
Godot traversal/Back and Audio overflow integration tests; actual DX12 screenshot;
record original-comparison and other unimplemented action limitations explicitly.
REA skill authorizes independent parallel workers: Back permission vs overflow.
Root owns bridge/GDScript/CMake/shared docs/index integration. No commit requested.

Initial finding E-0017-H01: fresh0016C4C0 pushes current descriptor+8 class-name
string into12x0x20-byte manager history via0016B418;0016C670 pops that string
via0016B438 and reopens class through0016C4C0 without pushing. Bridge previously
stored layout indices, losing shared-layout class identity. Root changes host
history to class strings. Permission/event defaults are explicit0016C670 fields
(+8=1,+12=sound6); only specific handler may alter these, so unopened recovered
handlers must not be assumed. Raw decompiles ignored in local/task0017.

### Validation before architecture checkpoint

Native andGodot build/CTest18/18PASS after Back/Audio/Display slices.
SubmenuSmokePASS: 8 actualBackspace routes including released/repeatedkey rejection,
class history, language enum1 directpop, Audio/Display clip/scroll/selector andends.
Rapid-retarget position/velocity testsPASS. ActualDX12 Audio/selector/Display
captures inspected underignoredlocal/shots/task0017-*.png. LatestWindows export
succeeded after Display addition; earlier exportedSubmenuSmokePASS/exit0 predates
Display. Active task remainspartial for data-dependentJukebox/editor/settings/card
services andDisplay snapshot entry. No originalcomparison orVerified claim.

User architecture question: inspected currentdependencies andRenderingServer use;
checkpoint inARCHITECTURE.md. CurrentGDExtension exists, directnativeRenderingServer
backend/RHI doesnot. User proposes renderer-onlyGodot target; no refactor executed.
Next TASK17: record finalintegrated findings/status/progress and validate latest
export; future architecture implementation needs its own scopedtask.

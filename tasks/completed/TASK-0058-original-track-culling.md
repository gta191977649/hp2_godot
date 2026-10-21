# TASK-0058 - Original track visibility and culling integration

- Status: completed
- Scope: user supersedes TASK0056 host-managed HLOD; production track frames must
  use original section/tree/instance/model selection for every view. Keep Show LOD
  as explicit browse-only diagnostic exception. No generated LOD or physics change.
- Authority: Retail7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
  Debug815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b names.
- Acceptance: trace original view/depth/focal setup and culling; close proven
  discrepancies; disable host HLOD selection/ranges; common viewer/drive/race and
  capture integration; useful culling diagnostics; representative Track11 native
  boundary/zone and GPU lifecycle/driving checks; Debug/Release build; docs/audit.
- Verification: PC checks cannot establish PS2 Verified. Unknown original paths
  remain explicit gaps; do not claim a complete world/gameflow recovery.

## Results

PASS: native selection integrated for every normal main/capture frame; host HLOD
configuration rejected; Show LOD remains explicit debug exception, engine ranges0.
Fresh Retail view/classify/culling plus paired Debug ctor/focal data inspected.
Original finite-float culling bodies were already restored and remain unchanged;
no invented additional original algorithm. Nine port-body replacements reviewed
against frozen TASK0056 identities. Source audit PASS; unchanged physics/native code.

Validation (one Track11 only): Debug/Release build PASS; native unit boundary tests
and --assets11 indexed/linear references PASS; targeted6/6 CTest PASS; actual GPU
native culling/Show LOD/clear/failure/reload PASS; viewer/Test drive/return PASS;
Release race movement and seven independent native views/focus/failure/restart/
frontend/re-entry PASS. Logs local/task0058/native-track11.log, native-gpu-final.log,
viewer-drive-gpu.log, race-native-gpu.log; inspected native-track11-start.png.
Commands, exact scope and limits in docs/NATIVE_TRACK_CULLING.md; binary pins in
local/task0058/delivery/BUILD-INFO.json. Toolchain unchanged Godot4.4.1/GCC13.2/
CMake3.29.2/Python3.14.4. No all30 sweep, FPS promise or PS2 Verified claim.

TASK0056 deferred to backlog by user revocation; previous launchers forward to58.
No commit/staging; external assets remain outside Git. No camera gameflow/streaming
completeness claim. Follow-up: use Launch TrackViewer Native.cmd with Show LOD off;
compare against original PS2 execution if Verified equivalence is requested.

Final Release targeted runtime/UI/source/progress4/4 PASS; final source audit
9before/9after PASS, git diff --check PASS. No owned render test process remains.

## Delivery dependency correction

User reported Error126 in task58 Debug snapshot. Snapshot staging copied the
extension DLL/config but omitted FFmpeg sidecars. Reopen packaging validation;
reconstructed/native source unaffected. Copy full built runtime bin and validate
actual delivery paths, not workspace/staged-build substitutes.

Packaging correction applied: all8 runtime DLLs staged per flavor by new
tools/stage_godot_runtime.ps1; extension hashes unchanged, manifests recorded.
Actual delivery Debug viewer/Test drive PASS. Direct Release startup/captures
PASS, lifecycle check finishing. No build needed for sidecar-only correction.

Delivery correction final: direct task58 Debug viewer/Test drive PASS and direct
task58 Release RACE_RUNTIME_SMOKE_OK PASS, including exit/re-entry. Logs
local/task0058/delivery-debug-check.log and delivery-release-check.log contain no
extension/dependency errors. Both manifests have8DLLs and matching extension pins.
Packaging follow-up closed; no all-track test, native-code change or commit.

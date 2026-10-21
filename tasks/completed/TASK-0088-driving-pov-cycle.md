# TASK-0088 - Original driving POV cycle and per-view projection

Status: Completed

Scope: correct V from latched lookback to original available driving POV cycle,
including forward-facing bumper view and original per-view FOV. This user request
supersedes TASK0084 Godot-default projection exception for switched driving views.
Preserve temporary mouse/idle return. Debug NFSHP2.ELF SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b,
version unknown. Read-only Ghidra and original assets only; not Verified.

- [x] Recover bumper/outside pose, original POV cycling and projection dependencies.
- [x] Implement native producer and host switch/projection, retain mouse overlay.
- [x] Bind B as held original lookback; release restores selected POV, with
  focus/UI/pause input cleanup and original transition, not a latched toggle.
- [x] Focused native/host tests, strict source successor, records/progress.

No benchmark. Preserve unrelated dirty files and immutable TASK0087 review.
Findings: E-0088-C01/H01, reverse/task0088/camera.md and host-camera.md.
No-dash fixture cycles original authored fullscreen1->2->0->1. Bumper uses
body-relative original mount and90degree horizontal FOV; outside uses0x32DC
(71.52099609375degrees). Native producer publishes original near/own-car/dash
flags; unsupported host dashboard rendering throws explicitly at0015D89C.
Mouse idle return, B release, consumed-input/focus cleanup and preview restoration
are covered. No original whole-entry stub transitions or physics replacement.

Native driving_camera1/1 passed4.18s (all64configs, authored bumper, B/release,
reverse and topology). Godot six host regressions passed42.05s; final bridge
guard rebuild succeeded and latest-DLL camera_host passed3.92s. Versions and
exact commands are in evidence records. No benchmark or original equivalence.
Strict E-0088-S01 passed:7/7 unit264.287s; preservation1/1 in261.06s. Review SHA256583208d733c67735396dc33277e9445be9eb51755084374f84742885e9303123; frozen87 unchanged, no stub transitions. No commit. Next: TASK0089 water effects; original camera equivalence remains unverified.

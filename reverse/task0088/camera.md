# TASK-0088 original driving POV cycle, bumper and projection

Evidence E-0088-C01. Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
version/build independently unknown. Read-only Ghidra captures are private under
local/task0088/camera; camera-captures.json records their hashes. Original camera
rows come from the separately authorized Retail GLOBAL/GLOBALB.BUN. Implemented
bounded native projection, not Verified against original execution.

## Confirmed correction to TASK0084 interpretation

POV_CHANGE and LOOKBACK are different original events. Player::CycleDriveCameraPOVIndex
00176EE0 tests up to four cyclic indices. A non-null POV passes immediately when
SkipFE326E9C is enabled; otherwise RideInfo.HasDash+6 selects the eligibility gate:
no dash accepts Type0 BUMPER or Type2 OUTSIDE; dash accepts Type1 INCAR. Four
unsuccessful candidates select index0. The host V request now advances this cycle.
B press/release supplies the original DesiredLookBackParam1/0 state independently
of the selected POV; it does not cycle or latch an unrelated front-of-car view.
TASK0084's original pose/lag recovery remains useful, while its V lookback binding
and Godot-default FOV exception are superseded by this user correction.

The original64-car asset table contains44 explicit three-POV rows (0,2,2), six
four-POV rows (0,2,2,1: SPIDER/F50/BARCHETTA and their HP variants), and14 rows with
zero camera counts. Constructor0015BD10 selects CarTypeInfoArray[0].Camera when
either full/split count is zero; row0 is CORVETTE and supplies (0,2,2). Thus the
explicit no-dash host fixture cycles default1 -> close-outside2 -> bumper0 ->1
for every catalog row. A genuine RideInfo, if supplied, uses its original HasDash
byte. No original Player/RideInfo allocation ABI is invented for the fixture.

## Original pose and projection

Update0015C2B0 BUMPER0/INCAR1 uses transpose(State.BodyMatrix), standard eased
LookBackAngle, authored POV Lag/Height/LatOffset, then the authored Angle rotation.
The selected view follows full body orientation rather than velocity-blended
outside heading. It computes the inverse camera pose, raises it to topology
elevation+3250E8 when necessary, then applies eRotateX4000 and eRotateYC000 before
publishing the camera matrix. Returning the inverse matrix's forward/up axes to
Godot preserves original orientation without substituting a look-at-car formula.
A bumper view faces forward from its authored mount; full held lookback faces
rearward from that selected POV and release returns to its forward orientation.
Original outside reverse-velocity reflection remains independent of lookback.

Original FOV fields: BUMPER0 and INCAR1 use4000 (90degrees), OUTSIDE2 uses32DC
(71.52099609375degrees), measured across original screen width. NearZ is.5 except
INCAR1, which reads3250EC. CarNotToRender is the followed car for the non-outside
branch and null for outside. RenderDash is enabled for INCAR only when the desired
lookback state is not1. The native frame exposes fov_bangle, near_z, hide_car,
render_dash, pov_index and pov_type rather than silently applying Godot defaults.
The host per-view projection/mask adapter is documented separately.

Authored Angle is consumed by BUMPER/INCAR. The original OUTSIDE branch still
rebuilds IsoProjection independently and discards its final Angle scratch rotation;
TASK0084's assembly counterevidence remains valid. Switching outside distance
reads the selected row (5.8vs4.7 for911TURBO), rather than using one fixed radius.

## Bounded implementation

DrivingCamera::cycle_pov and update in original cameramover.cpp use port-owned
camera state, private copied Memory and original table/helper arithmetic. Old
whole-entry placeholders remain unchanged. Headercamera.hpp describes native
output; bridge cycle_driving_camera_pov/update_driving_camera commit candidate
camera state only on success and do not change physical car state.
The no-dash native fixture makes bumper/outside available. Native in-car pose and
metadata have tests, but dashboard rendering remains unbound: bridge requests
with render_dash=true throw at original assignment0015D89C instead of silently
showing an incomplete dash view. Null topology remains an explicitly unbounded
laboratory world; original topology elevation is used whenever a track exists.

No pursuit/birdseye camera, active camera shake, full original render-camera
scheduler/DOF or original display/GS emulation is introduced. Temporary mouse view
and idle return remain explicit requested host controls. User B is a held host
binding to original lookback semantics, not a recovered PS2 keyboard assignment.

## Validation

cmake --build --preset native-debug --target driving_camera_tests -j4 passed;
ctest --preset native-debug -R '^driving_camera$' --output-on-failure passed1/1
in4.18s on2026-10-21 (GCC13.2.0,CMake3.29.2,Ninja1.12.0,Python3.14.4).
Tests retain TASK0084 stationary/lag/wall/reset/invalid-time and240native motion
coverage, then check every64-car no-dash POV cycle/FOV, close distance, authored
forward bumper mount, B held/released bumper direction and index preservation,
HasDash in-car metadata/near plane, original reverse velocity reflection and
bumper topology floor clearance. PC tests establish regression contracts only;
no original numerical, visual or timing equivalence was measured. No benchmarks.

# TASK0052 original LOD focal scale

## E0052D10: CalculateH and global screen width

Read-only Retail CreateViewMatricies0010E638 / Debug0010EA18 assigns `view->H=CalculateH(camera.CurrentKey.FieldOfView)`. This precedes target aspect/scissor matrix construction. It does not derive H from the host viewport or from the selected render target width.

CalculateH Retail00104448 / Debug001042E8 halves the original16-bit bAngle, evaluates bSin/bCos, and returns `(ScreenWidth>>1)/(sine/cosine)`, with10 when the tangent is exactlyzero. RetailbCos0025A610 is a direct tailcall bSin(angle+0x4000), preserving16-bit argument wrap. The reconstruction uses existing original bSin and explicit Debug logical memory for the global, not host trig.

RetailScreenWidth0032F6DC and Debug00326760 initially contain512; the following ScreenHeight values are448. eSetDisplaySystem Retail001030E8 / Debug00103108 writes ScreenWidth512 in BOTH branches (height448 for mode1,512 otherwise). Thus the supported original video paths always use halfwidth256 for native pixel-size/LOD policy, including the64-square environment capture views. Target aspect/scissor matrices separately adapt raster projection.

A1920-pixel host viewport would incorrectly multiply native H by3.75 if its width replaced512. The renderer must preserve original H for track and car LOD while using the actual host projection for displayed image geometry. This is a restoration of original selection thresholds, not an arbitrary shorter draw distance. `port/track/projection.hpp::original_focal_h` forwards to the original owner free function `hp2::recovered::CalculateH` in `src/ecstasy/ecstasye.cpp`; the explicit Memory argument replaces original globals without asserting original ABI.

Fresh evidence under `local/task0052`: `lod-0010EA18.c`, `lod-0010E638.c`, `lod-calculateh-debug.c`, `lod-calculateh-retail.c`, `lod-display-system.c`, `lod-display-system-retail.c`, `lod-bcos-retail.c`, `lod-screen-0032F6DC.json`, `lod-screen-00326760.json`. Parent-owned GPU/frontend wiring consumes the same original focal value for main and environment views.

PC numeric and image tests do not imply original-console Verified status.

Private current-TU probe with the original Debug image passed zero-FOV fallback10,90-degree focal near256 (original polynomial bSin approximation), outside0x32DC H355.469 and environment0x75E4 H31.9234. Doubling the logical original ScreenWidth doubles H exactly. Output `local/task0052/focal-h.log`. The runtime asset test adds the two endpoint checks without changing synthetic FrameView default focal1 or existing LOD fixtures.

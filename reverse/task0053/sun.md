# TASK0053 Sun regression investigation

## E-0053-S01: layer-safe host visibility

Authority: Retail SLUS_203.62 SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; Debug naming reference NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b. Executable revisions unknown. Native StuffSun001252B0/00125430 and original game data retain the previously recorded TASK0052 evidence; no native Sun behavior change is inferred from this port defect.

Actual Track11 SunChunkInfoE world position is native(-3900,650,1300). Four active additive layers use intensities1,.5,.7,.32 and sizes60,320,800,800; all four positive intensities depend on the original cached32x32 uncovered-depth count. The original StuffSun transforms this world point with world-screen/world-view matrices using W1; no camera-relative direction substitution is made.

Before correction, the actual Sun-facing GPU fixture at free-camera native(-98,-732,300), aimed at the original asset point, produced four native quads but zero uncovered samples after15 completed frames. Sun enabled/disabled images were identical; no shader error or loading error occurred. Ignored log local/task0053/sun-facing-before.log and enabled/disabled images reproduce the defect. CPU projection is inside the target; this evidence isolates coverage/depth or host submission rather than authorizing forced visibility.

The parent draw-budget probe identifies the host defect: RenderingServer.instance_set_ignore_culling(true) bypasses layer masks as well as geometric culling. Main/environment sky nodes therefore leaked into unrelated views including the raw depth proxy viewport, corrupting what must be encoded GS depth with sky colour. The proper fix is layer-respecting ordinary culling plus a host-only conservative camera-frustum custom AABB for screen-space submissions; it is not a change to original Sun visibility, alpha, depth thresholds or asset position. Implementation and post-fix GPU validation pending.

The regression fixture also checks actual original road occlusion by placing a free test camera5m below the recovered start road and retaining the same original Sun point. This changes only the test camera; no synthetic Sun or occluding game geometry is substituted. Original-runtime image equivalence remains unverified.

## Implementation and validation

The TASK0053 host correction removes `instance_set_ignore_culling(true)` from Sun and particle submissions. Both now use the shared `camera_visibility_bounds.hpp` helper to provide conservative camera-frustum custom bounds while retaining ordinary layer filtering. Particle update receives the actual source `Camera3D*` as an explicit optional host parameter; enabled rendering without it fails explicitly. The parent applies the same correction to Sky and preserves custom bounds when cloning writing particles into the raw-depth viewport. No recovered native function body, Sun data, coverage threshold or RNG behavior changes.

Validation: C++17 syntax checking of `track_sun_renderer.cpp` and `track_particle_renderer.cpp` passed with the project Godot headers. Parent shared Debug build and promotion passed (`local/task0053/build-debug-layer-fix.log`). The actual-window command below passed on Godot4.4.1, Compatibility/OpenGL3.3, Intel Arc B390, exit0:

```powershell
& local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path D:/dev/hp2_godot/godot res://scenes/track_sun_gpu_smoke.tscn
```

`local/task0053/sun-facing-after.log` reports `TRACK_SUN_GPU_OK`. After15 completed frames at the asset-facing camera, the original32x32 uncovered count is1024, all four native quads are submitted, and enabling Sun changes the captured pixels. Visual inspection of `sun-facing-enabled.png` confirms the disc and flares at the original projected asset point. With the test camera5m below the recovered start road, the same original Sun point yields zero uncovered samples. Thus the regression checks both visible and actual-geometry-occluded conditions, together with the Sun permission gate; it does not force cached coverage. The pre-fix log and separate before-images remain retained in ignored local evidence. Original-console matched-frame equivalence remains unverified.

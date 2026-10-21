# TASK0054 world effect anchor regression fixtures

## E-0054-T01: moving-camera GPU versus CPU projection

These are port regression tests, not original-console equivalence evidence. The existing actual Track11 Sun/forest-fire fixtures now exercise three camera poses with independent yaw, pitch and translation while presentation time is zero. No native data, simulation arithmetic or production renderer is modified by this test scope.

Sun expected pixels come from Camera3D.unproject_position of the actual original Sun world point, mapped native(-3900,650,1300) to host(650,1300,-3900). Strong disc pixels above0.90 in each RGB component form the measured centroid; it must be within6pixels of the projected anchor. The poses deliberately place the Sun away from the vertical/horizontal viewport center, making the previously observed vertical mirror distinguishable. Existing real-road occlusion and permission tests remain.

Particle expected rectangles come directly from actual visible MeshInstance3D packet vertices whose shader contains particle_z. Original fixed16 GS XY values convert to original top-origin512x448 coordinates by subtracting(1792,1824), then scale to the actual viewport. Every original four-vertex quad contributes its rectangle, expanded2pixels for raster boundaries. Particle-on versus particle-off captures at frozen time must change at least4pixels, and no more than1percent of meaningful changed pixels (RGB delta at least0.025) may fall outside all expected rectangles. Three translated/yawed/pitched camera poses prevent a centered or mirrored shader from passing accidentally. Sun is disabled during particle comparisons to isolate this contribution.

Validation: Godot4.4.1 --headless --path D:/dev/hp2_godot/godot --script res://scripts/runtime/track_sun_gpu_smoke.gd --check-only and corresponding track_particles_gpu_smoke.gd command both passed, exit0. Actual-window execution is pending the parent-owned production build and GPU slot. No Verified claim.

## Stronger asymmetric production-shader transport control

The broad union of actual forest-fire packet rectangles also admitted the TASK0053 vertically mirrored shader because real overlapping particles cover a large region. This counterevidence is retained: those moving-camera checks alone do not discriminate the defect.

An additional explicitly synthetic host-only transport fixture now duplicates the actual live production ShaderMaterial, including its original texture and depth coefficients, into a separate256x256 own-world SubViewport. A test-only GS packet occupies UV rectangle(0.30,0.20)..(0.40,0.30), fixed16 GS XY and native depth0xffffff. It does not invoke or replace original simulation. Nonzero GPU pixels must lie inside that asymmetric expected rectangle with2pixel raster allowance. A vertical mirror occupies a disjoint rectangle and fails.

Current corrected DLL actual-window run passed: PARTICLE_TRANSPORT pixels359 outside0. Real moving-camera checks also passed: poses0/1/2 changed33811/15425/40072 pixels, outside0. Log local/task0054/particle-transport-fixed.log, image particle-asymmetric-transport.png. The third pose uses camera(169,-46,-639), positive pitch0.03: the initial negative-pitch pose was actually occluded and produced no contribution, so its camera was corrected without weakening the nonempty-pixel condition. Frozen TASK0053 DLL negative control fails as required: PARTICLE_TRANSPORT pixels359 outside359 (local/task0054/particle-negative-control.log). The fixed DLL was restored and rerun successfully. Sun negative control also fails (expected516.78,316.73 vs observed517.01,223.76), local/task0054/negative-control.log. No original equivalence claim.

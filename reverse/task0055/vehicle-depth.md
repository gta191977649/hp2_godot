# TASK0055 closed vehicle depth-uniform polling contract

## E0055V01

Host-only optimization metadata; no recovered C++/physics, draw order, camera,
lighting coefficients or shader numeric behavior changes. Applies only to the
BASE ShaderMaterials created by Ps2VehicleRenderer. Baseline proxy polling stays
unchanged when root's resource_reuse gate is false.

A complete writer audit of ps2_vehicle_renderer.cpp found:

| Writer stage | BASE material mutations |
|---|---|
| rebuild_display | Shader, priority, texture, base_lit, flag_0x4, first four material vectors and seven initial light vectors are initialized for the new resource. |
| update_view | source_camera every update; light_35c,35d,35e,35f,360,361,362 when basis/ambient state changes. These existing computations/writes remain untouched. |
| set_environment_capture | next_pass link only; texture/HackMan reflection updates belong to the separate reflection material, which the raw-depth proxy does not follow. |
| skin/car load, inspection change | Rebuilds BASE material resources, rather than mutating initialized static coefficients. |
| comparison LOD creation | Duplicates initialized BASE materials once, then follows the same camera/light writer path. |
| wireframe/bounds visibility | Separate material overlays or geometry visibility, not BASE uniform writes. |

Each new BASE material stores immutable metadata:

- hp2_depth_uniform_contract: integer1.
- hp2_depth_dynamic_uniforms: PackedStringArray containing source_camera and
  light_35c,light_35d,light_35e,light_35f,light_360,light_361,light_362.

The list is allocated only during material construction. Comparison-LOD copies
explicitly receive the same contract/list, without relying on unspecified
Resource metadata duplication. Reflection, wireframe and unknown materials are
not tagged by this declaration. Root's optimized depth-proxy consumer must do a
full initial sync and resync on dirty/static invalidation or shader replacement;
only stable tagged resources may poll the declared eight values. Untagged or
unrecognized contracts retain complete uniform polling. External static uniform
mutations must mark the proxy dirty or revoke this closed-writer contract.

The primary host API reason is Godot4.4.1-stable
[ShaderMaterial::set_shader_parameter](https://github.com/godotengine/godot/blob/4.4.1-stable/scene/resources/material.cpp):
parameter updates reach RenderingServer but do not emit Resource changed, so a
blanket changed-signal cache is insufficient. This explicit audited writer set
retains camera/light liveness while reducing repeated static-uniform engine
calls. Actual speedup is not established by metadata alone.

Validation: C++20 syntax-only compile passed with pinned generated shader and
Godot headers. No shared build or GPU run by this worker. Root owns consumer
integration and full color/raw-depth comparisons during camera/basis movement,
skin/inspection rebuild, comparison LOD and unknown-material fallback tests.
No original-console Verified promotion.

# TASK-0082: vehicle lamp texture states

Evidence E-0082-L01. Debug `NFSHP2.ELF`, SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`. Read-only Ghidra captures remain ignored under `local/task0082/lights/`; `manifest.json` records capture hashes. Status: implemented bounded slice, PC tests passed; original runtime equivalence **not verified**.

## Confirmed behavior

- `CarRenderInfo::UpdateLightStateTextures` Debug `0x00120350`: signed track TimeOfDay codes 2 or 3 enable both headlights and brake textures. Car.HeadlightsState +0x830 enables both headlights; otherwise nonzero CopLights +0x808 chooses complementary sides using `(CarTimebaseStart + WorldTimeFrames) & 15`. The original table at 0x002D7AE8 contains eight one bytes then eight zero bytes. Global ForceHeadlightsOn 0x00326C80 and ForceBrakelightsOn 0x00326C84 override their respective states.
- Brake textures also enable in frontend or when driver.Brake +0x218 strictly exceeds the float at 0x00324B68 (raw CD CC 4C 3D, 0.05f). The driver pointer is Car+0x28. Equality does not enable brakes.
- Original state replacement preserves damaged texture entries and reconnects Body LODs 0/1/2 only after a hash change. This port binds only existing undamaged exterior surfaces; it does not claim the full original allocation, damage preservation or reconnect machinery.
- `GetUsedCarTextureInfo` Debug 0x00186940 identifies `_HEADLIGHT_LEFT`, `_HEADLIGHT_RIGHT`, `_BRAKELIGHT_LEFT`, `_BRAKELIGHT_RIGHT` source slots and `_HEADLIGHT`, `_HEADLIGHT_ON`, `_BRAKELIGHT`, `_BRAKELIGHT_ON` target suffixes, with the original texture prefix. Reverse lights are a distinct original state, not silently aliased to brakes.
- `CarRenderInfo::CreateCarLightFlares` 0x00120558 uses original ePositionMarker names, positions relative to PivotPosition, Param-selected normals and flare types. `CarRenderInfo::Render` 0x001210F0 computes per-marker head/brake/reverse/cop flare intensity and broken-lamp gates, then calls `eRenderLightFlare` 0x001168E8.
- 0x001168E8 selects up to two `LightFlareParameterIndicies[type]` entries. `RenderSingleFlare` 0x00116510 reads original texture ID, size/scale bounds, Power, ZBias and colour bounds, constructs camera-facing ePoly vertices and calls ePoly::Render3D. The inspected complete vehicle Render function has no identified projected-road-headlight call. This is evidence about that function only, not proof that every original rendering subsystem lacks road illumination.

## Implementation

`port::driving::vehicle_light_state` in src/care.cpp publishes the supported selector through port/driving/vehicle_lights.hpp. The driving session passes original track time and exposes three booleans under snapshot.lights. `port::vehicle::Model` records original body light slot roles before static constructor replacements, preloads existing ON/OFF assets and returns their hashes through driving_texture. Unsupported texture pairs with differing UV or GS pass state throw NotImplemented. No per-frame decoding or invented lamp geometry is used.

PS2VehicleRenderer changes base_texture and flag_0x4 only when a selected original hash changes. Dynamic depth submissions consume these changed material parameters as well. Its public visual state exposes selected light booleans and active_light_textures for integration checks. Existing uploaded textures are retained after staged model promotion.

## Validation and limitations

2026-10-21: `cmake --build --preset native-debug --target vehicle_model_tests -j 4` passed. `ctest --preset native-debug -R '^vehicle_model$' --output-on-failure` passed 1/1 in 14.31s. Native tests exercise day/night, strict brake threshold, force/frontend, callback state, cop phase boundaries, original asset hashes and unchanged geometry/UV. Fixture light-slot counts: 911TURBO 41 head / 8 brake; BMWM5COP 10 / 6; 4DR_SEDAN 0 / 0 (the latter fixture has no recognized original mapped slots). Initial asset assertion incorrectly required lights on that traffic fixture and was corrected after inspecting its surfaces.

Remaining: original damage texture state, reverse texture state, marker-driven glow/flare sprites and any separately evidenced road illumination. No ePositionMarker/flare-parameter/flare-texture loader or eLightFlare renderer currently exists in src; implementing original flares therefore requires recovering those data bindings plus the ePoly consumer, rather than adding arbitrary Godot cones. Native tests establish selector and asset binding behavior, not PS2 frame equivalence. Godot extension build and scene smoke are recorded by TASK-0082 integration owner.

Continuation: TASK-0082 or a scoped follow-up should recover the original ePositionMarker and LightFlareParameterIndicies loading paths, retain asset-provided normals/sizes/colours, then bind 0x001168E8/0x00116510 and the per-marker intensity slice from 0x001210F0. Keep raw captures ignored and compare against an original frame before marking verified.

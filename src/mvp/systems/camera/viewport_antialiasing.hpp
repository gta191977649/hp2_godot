#pragma once

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/viewport.hpp>

namespace hp2 {

inline godot::String graphics_antialiasing()
{
    return godot::ProjectSettings::get_singleton()->get_setting("hp2/graphics_aa", "");
}

inline bool uses_resolved_viewport_color(const godot::String& mode)
{
    return mode == "fxaa" || mode == "smaa" || mode == "taa" ||
        mode == "ssaa" || mode == "fsr2" || mode == "metalfx";
}

inline void configure_viewport_antialiasing(godot::Viewport& view, const godot::String& mode)
{
    using V = godot::Viewport;
    const auto msaa = mode == "msaa2" ? V::MSAA_2X :
        mode == "msaa4" ? V::MSAA_4X : mode == "msaa8" ? V::MSAA_8X : V::MSAA_DISABLED;
    view.set_msaa_3d(msaa);
    // The runtime is 4.7.2. The pinned GDExtension headers predate the SMAA enum.
    constexpr auto smaa = static_cast<V::ScreenSpaceAA>(2);
    view.set_screen_space_aa(mode == "fxaa" ? V::SCREEN_SPACE_AA_FXAA :
        mode == "smaa" ? smaa : V::SCREEN_SPACE_AA_DISABLED);
    view.set_use_taa(mode == "taa");
    if (mode == "ssaa") {
        view.set_scaling_3d_mode(V::SCALING_3D_MODE_BILINEAR);
        view.set_scaling_3d_scale(1.5f);
    } else {
        view.set_scaling_3d_scale(1.0f);
        view.set_scaling_3d_mode(mode == "fsr2" ? V::SCALING_3D_MODE_FSR2 :
            mode == "metalfx" ? V::SCALING_3D_MODE_METALFX_TEMPORAL : V::SCALING_3D_MODE_BILINEAR);
    }
}

}

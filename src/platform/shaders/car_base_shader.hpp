#pragma once
#include "hp2_embedded_shaders.hpp"
#include <string>
namespace port::godot_bridge {
// Host shader specialization only. Original coefficient/light data comes from
// CarRenderInfo's native resource/pose pipeline and the bounded shared PS2 slices.
inline void replace_shader_token(std::string& code,const std::string& token,const std::string& value) {
    const auto at=code.find(token);if(at!=std::string::npos)code.replace(at,token.size(),value);
}
inline std::string car_base_shader_source(const std::string& modes,const std::string& fragment,
                                         const std::string& hint="repeat_disable") {
    std::string code=embedded_shaders::car_base;
    replace_shader_token(code,"@RENDER_MODES@",modes);
    replace_shader_token(code,"@SAMPLER_HINT@",hint);
    replace_shader_token(code,"@FRAGMENT_SOURCE@",fragment);
    return code;
}
inline std::string car_fragment_source(bool inspection,int alpha_test,bool blend,bool deferred) {
    std::string code=inspection?"vec4 combined=texture(base_texture,UV)*255.0;\n":embedded_shaders::car_modulate;
    if(alpha_test==6)code+="if(combined.a<=48.0){discard;}\n";
    code+="ALBEDO=hp2_framebuffer_rgb(combined.rgb/255.0);\n";
    if(blend)code+="ALPHA=combined.a/128.0;\n";
    else if(deferred)code+="ALPHA=1.0;\n";
    return code;
}
}

#include "track_shader.hpp"
#include "car_base_shader.hpp"
#include <stdexcept>
namespace port::godot_bridge {
namespace {
std::string sample_case(int level,bool linear,const port::track::TextureState& state){
    const auto sampler=level?"mip_"+std::to_string(level):std::string("base_texture");
    std::string code="vec2 size=vec2(textureSize("+sampler+",0));vec2 p=uv;";
    if(linear){if(!state.repeat_u)code+="p.x=clamp(p.x,0.5/size.x,1.0-0.5/size.x);";if(!state.repeat_v)code+="p.y=clamp(p.y,0.5/size.y,1.0-0.5/size.y);";return code+"return textureLod("+sampler+",p,0.0);";}
    code+="ivec2 c=ivec2(floor(p*size));";
    code+=state.repeat_u?"c.x=((c.x%int(size.x))+int(size.x))%int(size.x);":"c.x=clamp(c.x,0,int(size.x)-1);";
    code+=state.repeat_v?"c.y=((c.y%int(size.y))+int(size.y))%int(size.y);":"c.y=clamp(c.y,0,int(size.y)-1);";
    return code+"return texelFetch("+sampler+",c,0);";
}
}
std::string track_shader_source(const hp2::assets::TpkTextureHeader& header,const port::track::EffectState& effect,port::track::Fusion40 fusion){
    using port::track::Fusion40;
    if(effect.matrix_pack==2)throw std::invalid_argument("Track BiasLevel2 requires a separate host depth range; no shipped track surface uses it");
    if(effect.rewrite_alpha&&fusion==Fusion40::None)throw std::invalid_argument("Track strip40 needs a proven two-pass fusion or explicit destination-alpha rendering");
    if(!effect.rewrite_alpha&&fusion!=Fusion40::None)throw std::invalid_argument("Track fusion requested for a non40 strip");
    const auto& state=effect.texture;const int maximum=state.maximum_mip_level,mm=header.Field_0x70;
    if(maximum>6||mm>5||header.Field_0x71>3)throw std::invalid_argument("Track shader texture LOD metadata exceeds recovered fields");
    std::string declarations,cases;
    for(int level=1;level<=maximum;++level)declarations+="uniform sampler2D mip_"+std::to_string(level)+" : filter_linear, repeat_enable;\n";
    for(int level=0;level<=maximum;++level)cases+="if(level=="+std::to_string(level)+"){if(linear){"+sample_case(level,true,state)+"}else{"+sample_case(level,false,state)+"}}\n";
    std::string minify;
    if(mm<2)minify="return sample_original(0,uv,"+std::string(mm==1?"true":"false")+");";
    else{minify="lod=clamp(lod,0.0,"+std::to_string(maximum)+".0);";const std::string linear=mm>=4?"true":"false";if(mm==3||mm==5)minify+="int first=int(floor(lod));return mix(sample_original(first,uv,"+linear+"),sample_original(min(first+1,"+std::to_string(maximum)+"),uv,"+linear+"),fract(lod));";else minify+="return sample_original(int(floor(lod+0.5)),uv,"+linear+");";}
    std::string sampling=embedded_shaders::track_sampling;
    replace_shader_token(sampling,"@MIP_DECLARATIONS@",declarations);replace_shader_token(sampling,"@SAMPLE_CASES@",cases);
    replace_shader_token(sampling,"@LOD_SCALE@",std::to_string(float(1u<<header.Field_0x71)));
    replace_shader_token(sampling,"@LOD_BIAS@",std::to_string(float(static_cast<std::int16_t>(std::uint16_t(header.Field_0x72)<<4))/256.f));
    replace_shader_token(sampling,"@MINIFY_SOURCE@",minify);
    std::string modes="fog_disabled, shadows_disabled, ";modes+=effect.test.write_depth?"depth_draw_always":"depth_draw_never";
    const bool additive=fusion==Fusion40::Additive||state.additive;
    modes+=additive?", blend_add":", blend_mix";
    std::string fragment=embedded_shaders::track_fragment;
    replace_shader_token(fragment,"@TRACK_ALPHA_TEST@",effect.test.alpha_test==6?"if(combined.a<=48.0)discard;":"");
    std::string fusion_source;
    if(fusion!=Fusion40::None){fusion_source="vec4 specular=clamp(floor(textureLod(specular_texture,UV,0.0)*255.0*gs_rgba/128.0),vec4(0.0),vec4(255.0));\n";
        fusion_source+="combined.rgb=vec3("+std::string(fusion==Fusion40::Additive?"hp2_track_fused40_increment":"hp2_track_fused40_copy")+"(ivec3(combined.rgb),ivec3(specular.rgb),int(combined.a)));\ncombined.rgb=clamp(combined.rgb,vec3(0.0),vec3(255.0));\n";}
    if(fusion==Fusion40::None&&state.additive)fusion_source="combined.rgb=clamp(floor(combined.rgb*combined.a/128.0),vec3(0.0),vec3(255.0));\n";
    replace_shader_token(fragment,"@TRACK_FUSION@",fusion_source);
    std::string alpha="ALPHA=1.0;";
    if(fusion==Fusion40::None&&!state.additive&&state.blend==port::vehicle::BlendMode::SourceOver)alpha="ALPHA=combined.a/128.0;";
    replace_shader_token(fragment,"@TRACK_ALPHA_OUTPUT@",alpha);
    replace_shader_token(fragment,"@TRACK_DEPTH_OUTPUT@",effect.matrix_pack?"DEPTH=hp2_track_project_host_depth(-VERTEX.z,track_host_depth_ab);":"");
    auto code=car_base_shader_source(modes,fragment,"filter_linear, repeat_enable");
    replace_shader_token(code,"uniform bool base_lit = true;",std::string("uniform bool base_lit = ")+((effect.flags&0x80)?"true;":"false;"));
    replace_shader_token(code,"uniform bool flag_0x4 = false;",std::string("uniform bool flag_0x4 = ")+((effect.flags&4)?"true;":"false;"));
    std::string helpers="float hp2_affine_bit_power(float v,float s,float b){return vu_bit_affine(v,s,b);}\n";
    helpers+=embedded_shaders::track_effects;helpers+=sampling;
    if(fusion!=Fusion40::None)helpers+="uniform sampler2D specular_texture : filter_linear, repeat_enable;\n";
    if(effect.matrix_pack){auto depth=std::string(embedded_shaders::track_depth);const auto range=effect.matrix_pack==1?"vec2(intBitsToFloat(0x40000040),intBitsToFloat(0x40802000))":"vec2(intBitsToFloat(0x40900000),intBitsToFloat(0x40f00000))";replace_shader_token(depth,"@TRACK_DEPTH_RANGE@",range);helpers+=depth;}
    code.insert(code.find("void vertex(){"),helpers);
    if(effect.rewrite_alpha)replace_shader_token(code,"    if(reflection_pass){",std::string(embedded_shaders::track_alpha_vertex)+"\n    if(reflection_pass){");
    if(effect.matrix_pack)replace_shader_token(code,"    gs_color_w =", "    track_host_depth_ab=vec2(-PROJECTION_MATRIX[2][2],PROJECTION_MATRIX[3][2]);\n    if(CLIP_SPACE_FAR<0.0){track_host_depth_ab.x=track_host_depth_ab.x*0.5+0.5;track_host_depth_ab.y*=0.5;}\n    gs_color_w =");
    return code;
}
}

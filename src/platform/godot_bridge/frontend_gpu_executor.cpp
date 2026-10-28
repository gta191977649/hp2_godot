#include "frontend_gpu_executor.hpp"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/classes/rd_shader_source.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_vertex_attribute.hpp>
#include <godot_cpp/classes/rd_sampler_state.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/rd_pipeline_rasterization_state.hpp>
#include <godot_cpp/classes/rd_pipeline_multisample_state.hpp>
#include <godot_cpp/classes/rd_pipeline_depth_stencil_state.hpp>
#include <godot_cpp/classes/rd_pipeline_color_blend_state.hpp>
#include <godot_cpp/classes/rd_pipeline_color_blend_state_attachment.hpp>
#include <cstring>
#include <map>
#include <stdexcept>
namespace port::godot_bridge {
namespace {
using RD=godot::RenderingDevice;
const char* vertex_source=R"glsl(#version 450
layout(location=0) in vec4 position;
layout(location=1) in vec2 uv;
layout(location=2) in vec4 colour;
layout(location=0) out vec2 texcoord;
layout(location=1) noperspective out vec4 rgba;
void main(){gl_Position=position;texcoord=uv;rgba=colour;}
)glsl";
const char* fragment_source=R"glsl(#version 450
layout(set=0,binding=0) uniform sampler2D image;
layout(push_constant,std430) uniform Params {vec4 values;} params;
layout(location=0) in vec2 texcoord;
layout(location=1) noperspective in vec4 rgba;
layout(location=0) out vec4 result;
void main(){
 vec4 texel=texture(image,texcoord);
 if(params.values.x>0.5){
  vec4 combined=clamp(floor((texel*255.0)*rgba/128.0),vec4(0),vec4(255));
  if(combined.a<=params.values.y)discard;
  result=vec4(combined.rgb/255.0,combined.a/128.0);
 }else{result=texel*rgba;if(result.a<=params.values.y)discard;}
}
)glsl";
godot::PackedByteArray bytes(const void* data,std::size_t size){
    godot::PackedByteArray out;out.resize(size);if(size)std::memcpy(out.ptrw(),data,size);return out;
}
void require(godot::RID rid,const char* what){if(!rid.is_valid())throw std::runtime_error(what);}
}
struct FrontendGpuExecutor::Impl {
    RD* rd=nullptr;godot::RID shader,colour,depth,framebuffer,vertex_buffer;
    int width=0,height=0;std::size_t capacity=0;std::int64_t format=0;
    std::map<int,godot::RID> pipelines,samplers;
    struct Texture {godot::RID rid;int width=0,height=0;std::uint64_t revision=0;const void* identity=nullptr;};
    std::map<port::render::TextureId,Texture> textures;
    ~Impl(){if(rd){
        for(auto& entry:pipelines)rd->free_rid(entry.second);
        if(framebuffer.is_valid())rd->free_rid(framebuffer);
        for(auto& entry:textures)if(entry.second.rid.is_valid())rd->free_rid(entry.second.rid);
        for(auto& entry:samplers)rd->free_rid(entry.second);
        if(colour.is_valid())rd->free_rid(colour);if(depth.is_valid())rd->free_rid(depth);
        if(vertex_buffer.is_valid())rd->free_rid(vertex_buffer);if(shader.is_valid())rd->free_rid(shader);
        memdelete(rd);rd=nullptr;
    }}
    void initialize(){
        rd=godot::RenderingServer::get_singleton()->create_local_rendering_device();
        if(!rd)throw std::runtime_error("Frontend shared target requires a Vulkan RenderingDevice");
        godot::Ref<godot::RDShaderSource> source;source.instantiate();
        source->set_stage_source(RD::SHADER_STAGE_VERTEX,vertex_source);
        source->set_stage_source(RD::SHADER_STAGE_FRAGMENT,fragment_source);
        auto spirv=rd->shader_compile_spirv_from_source(source);
        for(auto stage:{RD::SHADER_STAGE_VERTEX,RD::SHADER_STAGE_FRAGMENT})
            if(!spirv->get_stage_compile_error(stage).is_empty())throw std::runtime_error(spirv->get_stage_compile_error(stage).utf8().get_data());
        shader=rd->shader_create_from_spirv(spirv);require(shader,"Frontend GPU shader creation failed");
        godot::TypedArray<godot::RDVertexAttribute> attrs;
        for(int i=0;i<3;++i){godot::Ref<godot::RDVertexAttribute> a;a.instantiate();a->set_location(i);
            a->set_format(i==1?RD::DATA_FORMAT_R32G32_SFLOAT:RD::DATA_FORMAT_R32G32B32A32_SFLOAT);
            a->set_offset(i==0?0:i==1?16:24);a->set_stride(sizeof(port::render::FrontendVertex));attrs.push_back(a);}
        format=rd->vertex_format_create(attrs);
    }
    void target(int w,int h){
        if(w==width&&h==height)return;
        for(auto& entry:pipelines)rd->free_rid(entry.second);pipelines.clear();
        if(framebuffer.is_valid())rd->free_rid(framebuffer);
        if(colour.is_valid())rd->free_rid(colour);if(depth.is_valid())rd->free_rid(depth);
        godot::Ref<godot::RDTextureFormat> f;f.instantiate();f->set_width(w);f->set_height(h);
        f->set_format(RD::DATA_FORMAT_R8G8B8A8_UNORM);
        f->set_usage_bits(RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT|RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT|RD::TEXTURE_USAGE_SAMPLING_BIT);
        godot::Ref<godot::RDTextureView> v;v.instantiate();colour=rd->texture_create(f,v);require(colour,"Frontend colour attachment failed");
        f.instantiate();f->set_width(w);f->set_height(h);f->set_format(RD::DATA_FORMAT_D32_SFLOAT);
        f->set_usage_bits(RD::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        depth=rd->texture_create(f,v);require(depth,"Frontend depth attachment failed");
        godot::TypedArray<godot::RID> attachments;attachments.push_back(colour);attachments.push_back(depth);
        framebuffer=rd->framebuffer_create(attachments);require(framebuffer,"Frontend shared framebuffer failed");width=w;height=h;
    }
    godot::RID texture(port::render::TextureId id,const port::render::FrontendPacket& packet){
        static const auto white=[](){auto b=std::make_shared<port::render::Bitmap>();b->width=b->height=1;b->rgba={255,255,255,255};return b;}();
        const auto found=packet.textures.find(id);auto bitmap=id==0?white:found==packet.textures.end()?nullptr:found->second;
        if(!bitmap)throw std::runtime_error("Frontend packet references a missing original texture");
        auto& cached=textures[id];
        if(cached.rid.is_valid()&&cached.width==bitmap->width&&cached.height==bitmap->height){
            if(cached.revision!=bitmap->revision||cached.identity!=bitmap.get()){
                rd->texture_update(cached.rid,0,bytes(bitmap->rgba.data(),bitmap->rgba.size()));cached.revision=bitmap->revision;cached.identity=bitmap.get();}
            return cached.rid;
        }
        if(cached.rid.is_valid())rd->free_rid(cached.rid);
        godot::Ref<godot::RDTextureFormat> f;f.instantiate();f->set_width(bitmap->width);f->set_height(bitmap->height);
        f->set_format(RD::DATA_FORMAT_R8G8B8A8_UNORM);f->set_usage_bits(RD::TEXTURE_USAGE_SAMPLING_BIT|RD::TEXTURE_USAGE_CAN_UPDATE_BIT);
        godot::Ref<godot::RDTextureView> view;view.instantiate();godot::TypedArray<godot::PackedByteArray> data;data.push_back(bytes(bitmap->rgba.data(),bitmap->rgba.size()));
        cached={rd->texture_create(f,view,data),bitmap->width,bitmap->height,bitmap->revision,bitmap.get()};require(cached.rid,"Frontend original texture upload failed");return cached.rid;
    }
    godot::RID pipeline(const port::render::FrontendDraw& draw){
        const int key=draw.blend|(draw.depth_write?4:0);auto found=pipelines.find(key);if(found!=pipelines.end())return found->second;
        godot::Ref<godot::RDPipelineRasterizationState> raster;raster.instantiate();raster->set_cull_mode(RD::POLYGON_CULL_DISABLED);
        godot::Ref<godot::RDPipelineMultisampleState> ms;ms.instantiate();
        godot::Ref<godot::RDPipelineDepthStencilState> z;z.instantiate();z->set_enable_depth_test(true);z->set_enable_depth_write(draw.depth_write);z->set_depth_compare_operator(RD::COMPARE_OP_GREATER_OR_EQUAL);
        godot::Ref<godot::RDPipelineColorBlendStateAttachment> a;a.instantiate();
        if(draw.blend){a->set_enable_blend(true);a->set_src_color_blend_factor(RD::BLEND_FACTOR_SRC_ALPHA);
            a->set_dst_color_blend_factor(draw.blend==2?RD::BLEND_FACTOR_ONE:RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
            a->set_src_alpha_blend_factor(RD::BLEND_FACTOR_ONE);a->set_dst_alpha_blend_factor(RD::BLEND_FACTOR_ZERO);}
        godot::Ref<godot::RDPipelineColorBlendState> blend;blend.instantiate();godot::TypedArray<godot::RDPipelineColorBlendStateAttachment> as;as.push_back(a);blend->set_attachments(as);
        auto rid=rd->render_pipeline_create(shader,rd->framebuffer_get_format(framebuffer),format,RD::RENDER_PRIMITIVE_TRIANGLES,raster,ms,z,blend);require(rid,"Frontend pipeline creation failed");pipelines.emplace(key,rid);return rid;
    }
};
FrontendGpuExecutor::FrontendGpuExecutor():p_(std::make_unique<Impl>()){}
FrontendGpuExecutor::~FrontendGpuExecutor()=default;
godot::Ref<godot::Image> FrontendGpuExecutor::render(const port::render::FrontendPacket& packet){
    auto& p=*p_;if(!p.rd)p.initialize();p.target(packet.width,packet.height);
    const auto size=packet.vertices.size()*sizeof(port::render::FrontendVertex);
    if(size>p.capacity){if(p.vertex_buffer.is_valid())p.rd->free_rid(p.vertex_buffer);p.capacity=std::max<std::size_t>(size,4096);p.vertex_buffer=p.rd->vertex_buffer_create(p.capacity);require(p.vertex_buffer,"Frontend vertex storage failed");}
    if(size)p.rd->buffer_update(p.vertex_buffer,0,size,bytes(packet.vertices.data(),size));
    struct Ready {godot::RID pipeline,uniform,vertices;std::array<float,4> params;};std::vector<Ready> ready;
    for(const auto& draw:packet.draws){if(!draw.count)continue;if(draw.first+draw.count>packet.vertices.size())throw std::runtime_error("Frontend draw exceeds vertex packet");
        auto& sampler=p.samplers[draw.repeat];if(!sampler.is_valid()){godot::Ref<godot::RDSamplerState> s;s.instantiate();s->set_mag_filter(RD::SAMPLER_FILTER_LINEAR);s->set_min_filter(RD::SAMPLER_FILTER_NEAREST);
            s->set_repeat_u((draw.repeat&1)?RD::SAMPLER_REPEAT_MODE_REPEAT:RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE);s->set_repeat_v((draw.repeat&2)?RD::SAMPLER_REPEAT_MODE_REPEAT:RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE);sampler=p.rd->sampler_create(s);}
        godot::Ref<godot::RDUniform> u;u.instantiate();u->set_uniform_type(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);u->set_binding(0);u->add_id(sampler);u->add_id(p.texture(draw.texture,packet));
        godot::TypedArray<godot::RDUniform> us;us.push_back(u);auto uniform=p.rd->uniform_set_create(us,p.shader,0);require(uniform,"Frontend sampler binding failed");
        godot::TypedArray<godot::RID> buffers;godot::PackedInt64Array offsets;for(int i=0;i<3;++i){buffers.push_back(p.vertex_buffer);offsets.push_back(draw.first*sizeof(port::render::FrontendVertex));}
        auto va=p.rd->vertex_array_create(draw.count,p.format,buffers,offsets);require(va,"Frontend vertex view failed");
        ready.push_back({p.pipeline(draw),uniform,va,{{draw.raw_gs?1.f:0.f,draw.alpha_threshold,0,0}}});
    }
    godot::PackedColorArray clear;clear.push_back({0,0,0,1});
    const auto list=p.rd->draw_list_begin(p.framebuffer,RD::DRAW_CLEAR_COLOR_ALL|RD::DRAW_CLEAR_DEPTH,clear,0);
    for(const auto& draw:ready){p.rd->draw_list_bind_render_pipeline(list,draw.pipeline);p.rd->draw_list_bind_uniform_set(list,draw.uniform,0);p.rd->draw_list_bind_vertex_array(list,draw.vertices);p.rd->draw_list_set_push_constant(list,bytes(draw.params.data(),16),16);p.rd->draw_list_draw(list,false,1);}
    p.rd->draw_list_end();p.rd->submit();p.rd->sync();auto pixels=p.rd->texture_get_data(p.colour,0);
    for(const auto& draw:ready){p.rd->free_rid(draw.uniform);p.rd->free_rid(draw.vertices);}
    return godot::Image::create_from_data(p.width,p.height,false,godot::Image::FORMAT_RGBA8,pixels);
}
}

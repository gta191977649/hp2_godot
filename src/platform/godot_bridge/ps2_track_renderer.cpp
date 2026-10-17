#include "ps2_track_renderer.hpp"
#include "boundary.hpp"
#include "game_core/port/track/runtime.hpp"
#include "platform/shaders/track_shader.hpp"
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <sstream>
#include <iomanip>
#include "game_core/port/track/debug_geometry.hpp"
#include "game_core/port/driving/surface.hpp"
#include "game_core/indep/src/scenery.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/port/driving/track_environment.hpp"
#include "platform/ps2_asset_root.hpp"
#include "port/vehicle/texture_state.hpp"
#include "game_core/port/track/texture_state.hpp"
#include "platform/shaders/car_base_shader.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <future>
#include <filesystem>
#include <map>
#include <tuple>
#include <stdexcept>
namespace port::godot_bridge {
namespace {
godot::String text(const std::string& s){return godot::String::utf8(s.c_str());}
godot::Vector3 host(const std::array<float,3>& p){return {p[1],p[2],p[0]};}
port::assets::DirectorySource source(){
    const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");
    const auto root=hp2::platform::resolve_zzdata_root(start.utf8().get_data());
    if(!root)throw std::runtime_error("Original ZZDATA root is not configured");
    return port::assets::DirectorySource(*root);
}
godot::Transform3D host_matrix(const std::array<float,16>& m){
    godot::Basis b;b.set_column(0,host({{m[4],m[5],m[6]}}));b.set_column(1,host({{m[8],m[9],m[10]}}));b.set_column(2,host({{m[0],m[1],m[2]}}));return {b,host({{m[12],m[13],m[14]}})};
}
struct Loaded {std::shared_ptr<const port::track::Scene> scene;std::shared_ptr<port::driving::Memory> math;std::shared_ptr<const port::driving::TrackEnvironment> environment;std::unique_ptr<port::track::DebugGeometry> debug;std::string error,debug_error;};
}
struct Ps2TrackRenderer::Impl {
    std::vector<port::track::Entry> catalog;
    std::future<Loaded> pending;
    std::shared_ptr<const port::track::Scene> scene;
    std::map<std::uint32_t,godot::Ref<godot::ImageTexture>> textures;
    std::map<std::uint32_t,std::vector<godot::Ref<godot::ImageTexture>>> mipmaps;
    std::map<std::string,godot::Ref<godot::Shader>> shaders;
    std::map<std::tuple<std::uint32_t,std::uint32_t,std::uint16_t>,godot::Ref<godot::ShaderMaterial>> materials;
    std::vector<godot::GeometryInstance3D*> nodes;
    // Port-only topology inspection; flags survive scene replacement, resources do not.
    std::unique_ptr<port::track::DebugGeometry> debug;
    godot::MeshInstance3D *debug_fill=nullptr,*debug_edges=nullptr;
    godot::Ref<godot::ShaderMaterial> debug_material;
    bool render_collision=false,render_material=false;
    std::string debug_error;

    port::track::Runtime runtime;
    port::track::Frame frame;
    std::string packet_digest;
    struct Draw {godot::MeshInstance3D* node=nullptr;godot::Ref<godot::ShaderMaterial> material;};
    std::map<std::tuple<std::size_t,std::size_t,std::size_t>,Draw> draws;
    std::map<std::pair<std::size_t,std::size_t>,godot::Ref<godot::ArrayMesh>> meshes;
    std::map<std::uint32_t,std::array<unsigned,2>> alpha_ranges;
    std::map<std::uint32_t,const port::vehicle::Texture*> texture_info;
    std::shared_ptr<port::driving::Memory> math;
    std::shared_ptr<const port::driving::TrackEnvironment> environment;
    std::size_t batched_instances=0;
    std::size_t instance_cursor=0;
    godot::AABB bounds;
    bool has_bounds=false,loading=false,ok=false;
    int number=-1;
    std::size_t texture_cursor=0,solid_cursor=0,surface_cursor=0,triangles=0;
    std::string error,diagnostic,phase="Empty";
};
Ps2TrackRenderer::Ps2TrackRenderer():impl_(std::make_unique<Impl>()){}
Ps2TrackRenderer::~Ps2TrackRenderer()=default;
void Ps2TrackRenderer::_bind_methods(){
    godot::ClassDB::bind_method(godot::D_METHOD("get_catalog"),&Ps2TrackRenderer::get_catalog);
    godot::ClassDB::bind_method(godot::D_METHOD("request_track","number"),&Ps2TrackRenderer::request_track);
    godot::ClassDB::bind_method(godot::D_METHOD("load_track","relative_path"),&Ps2TrackRenderer::load_track);
    godot::ClassDB::bind_method(godot::D_METHOD("pump_load"),&Ps2TrackRenderer::pump_load);
    godot::ClassDB::bind_method(godot::D_METHOD("get_state"),&Ps2TrackRenderer::get_state);
    godot::ClassDB::bind_method(godot::D_METHOD("frame_summary"),&Ps2TrackRenderer::frame_summary);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_track"),&Ps2TrackRenderer::clear_track);
    godot::ClassDB::bind_method(godot::D_METHOD("set_render_collision","enabled"),&Ps2TrackRenderer::set_render_collision);
    godot::ClassDB::bind_method(godot::D_METHOD("set_render_material","enabled"),&Ps2TrackRenderer::set_render_material);
    godot::ClassDB::bind_method(godot::D_METHOD("update_view","camera"),&Ps2TrackRenderer::update_view);
}
void Ps2TrackRenderer::release_display(){
    impl_->runtime.unload();impl_->frame={};impl_->packet_digest.clear();impl_->draws.clear();
    impl_->debug_fill=impl_->debug_edges=nullptr;impl_->debug_material.unref();impl_->debug.reset();impl_->debug_error.clear();
    for(auto* node:impl_->nodes){remove_child(node);memdelete(node);}impl_->nodes.clear();
    impl_->textures.clear();impl_->meshes.clear();impl_->mipmaps.clear();impl_->shaders.clear();impl_->materials.clear();
    impl_->alpha_ranges.clear();impl_->texture_info.clear();impl_->bounds={};impl_->has_bounds=false;
}
void Ps2TrackRenderer::clear_track(){
    // One owned worker is joined before clearing; it never touches Godot objects.
    const auto failure=catch_all([&]{
        if(impl_->pending.valid())impl_->pending.get();
        release_display();impl_->scene.reset();impl_->environment.reset();impl_->loading=false;impl_->ok=false;impl_->number=-1;
        impl_->error.clear();impl_->diagnostic.clear();impl_->phase="Empty";
    });
    if(!failure.empty()){impl_->error=failure;impl_->phase="Failed";impl_->ok=false;impl_->loading=false;}
}
godot::Array Ps2TrackRenderer::get_catalog(){
    godot::Array result;
    const auto failure=catch_all([&]{if(!requests_enabled())throw std::logic_error("TrackViewer is disabled in Release builds");auto assets=source();impl_->catalog=port::track::load_catalog(assets);
        for(const auto& e:impl_->catalog){godot::Dictionary t;t["id"]=e.number;t["name"]=text(e.name);t["location"]=text(e.location);
            t["path"]=text("TRACKS/TRACKB"+std::to_string(e.number)+".LZC");
            bool available=true;std::string missing;
            for(std::size_t i=0;i<e.resources.size();++i){const auto& r=e.resources[i];std::error_code error;
                const bool present=i==0?std::filesystem::is_regular_file(std::filesystem::path(assets.root())/r,error):assets.probe(r);
                if(!present){available=false;missing+="Missing "+r+"\n";}}

            t["available"]=available;t["error"]=text(missing);result.push_back(t);}
    });
    if(!failure.empty())impl_->error=failure;return result;
}
godot::Dictionary Ps2TrackRenderer::load_track(const godot::String& path){
    const auto value=std::string(path.utf8().get_data());
    if(impl_->catalog.empty())get_catalog();
    for(const auto& entry:impl_->catalog){const auto suffix=std::to_string(entry.number);
        if(value=="TRACKS/TRACKB"+suffix+".LZC"||value=="TRACKS/TRACKA"+suffix+".BUN")return request_track(entry.number);}
    clear_track();impl_->error="Expected a catalogue TRACKS/TRACKB<number>.LZC or TRACKA<number>.BUN path";impl_->phase="Failed";return get_state();
}
godot::Dictionary Ps2TrackRenderer::request_track(int number){
    clear_track();
    const auto failure=catch_all([&]{if(!requests_enabled())throw std::logic_error("TrackViewer is disabled in Release builds");if(impl_->catalog.empty())get_catalog();
        auto entry=std::find_if(impl_->catalog.begin(),impl_->catalog.end(),[&](const auto& t){return t.number==number;});
        if(entry==impl_->catalog.end())throw std::invalid_argument("Unknown original track number");
        auto assets=source();const auto root=assets.root();const auto selected=*entry;
        impl_->number=number;impl_->loading=true;impl_->phase="Reading and decoding original track";
        impl_->texture_cursor=impl_->solid_cursor=impl_->surface_cursor=impl_->triangles=impl_->instance_cursor=0;
        const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");const auto project=std::string(start.utf8().get_data());const auto math=impl_->math;
        impl_->pending=std::async(std::launch::async,[root,selected,project,math]{Loaded out;try{
            port::assets::DirectorySource assets(root);out.scene=std::make_shared<const port::track::Scene>(port::track::load_scene(assets,selected));
            out.environment=port::driving::load_track_environment(*out.scene);
            out.math=math?math:std::make_shared<port::driving::Memory>(port::driving::read_debug_image(port::driving::debug_image_path(project)));
            out.debug_error=catch_all([&]{out.debug=std::make_unique<port::track::DebugGeometry>(port::track::build_debug_geometry(out.environment->topology,port::driving::read_track_material_catalog(*out.math)));});
        }catch(const std::exception& e){out.error=e.what();}return out;});
    });
    if(!failure.empty()){impl_->error=failure;impl_->loading=false;impl_->phase="Failed";}return get_state();
}
godot::Dictionary Ps2TrackRenderer::pump_load(){
    if(!impl_->loading)return get_state();
    const auto failure=catch_all([&]{
        if(impl_->pending.valid()){
            if(impl_->pending.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
            auto loaded=impl_->pending.get();if(!loaded.error.empty())throw std::runtime_error(loaded.error);
            impl_->debug=std::move(loaded.debug);impl_->debug_error=std::move(loaded.debug_error);
            impl_->environment=std::move(loaded.environment);impl_->math=std::move(loaded.math);impl_->scene=std::move(loaded.scene);if(!impl_->scene)throw std::runtime_error("Track loader returned no scene");
            impl_->runtime.load(impl_->scene);
            for(const auto& gap:impl_->scene->gaps)impl_->diagnostic+=gap+"\n";
            impl_->phase="Uploading original track";
        }
        const auto start=std::chrono::steady_clock::now();
        const auto yield=[&]{return std::chrono::steady_clock::now()-start>=std::chrono::milliseconds(3);};
        const auto& scene=*impl_->scene;
        while(impl_->texture_cursor<scene.textures.size()){
            const auto& t=scene.textures[impl_->texture_cursor++];
            impl_->texture_info.emplace(t.header.tex_hash,&t);
            auto& range=impl_->alpha_ranges[t.header.tex_hash];range={255,0};
            const auto include_alpha=[&](const auto& bitmap){for(std::size_t i=3;i<bitmap.rgba.size();i+=4){range[0]=std::min(range[0],unsigned(bitmap.rgba[i]));range[1]=std::max(range[1],unsigned(bitmap.rgba[i]));}};
            include_alpha(t.bitmap);
            const auto original_mips=scene.mipmaps.find(t.header.tex_hash);if(original_mips!=scene.mipmaps.end())for(const auto& mip:original_mips->second)include_alpha(mip);
            godot::PackedByteArray bytes;bytes.resize(t.bitmap.rgba.size());
            std::memcpy(bytes.ptrw(),t.bitmap.rgba.data(),t.bitmap.rgba.size());
            auto image=godot::Image::create_from_data(t.bitmap.width,t.bitmap.height,false,godot::Image::FORMAT_RGBA8,bytes);
            if(image.is_null())throw std::runtime_error("Track texture upload failed");impl_->textures.emplace(t.header.tex_hash,godot::ImageTexture::create_from_image(image));
            const auto mips=scene.mipmaps.find(t.header.tex_hash);
            if(mips!=scene.mipmaps.end())for(const auto& mip:mips->second){
                godot::PackedByteArray pixels;pixels.resize(mip.rgba.size());std::memcpy(pixels.ptrw(),mip.rgba.data(),mip.rgba.size());
                const auto level=godot::Image::create_from_data(mip.width,mip.height,false,godot::Image::FORMAT_RGBA8,pixels);
                if(level.is_null())throw std::runtime_error("Original track mip upload failed");impl_->mipmaps[t.header.tex_hash].push_back(godot::ImageTexture::create_from_image(level));
            }
            if(yield())return;
        }
        while(impl_->solid_cursor<scene.geometry.solids.size()){
            const auto& solid=scene.geometry.solids[impl_->solid_cursor];
            while(impl_->surface_cursor<solid.surfaces.size()){
                const auto surface_index=impl_->surface_cursor++;
                const auto& surface=solid.surfaces[surface_index];if(surface.indices.empty())continue;
                godot::PackedVector3Array positions;godot::PackedVector2Array uv;godot::PackedColorArray colors;godot::PackedInt32Array indices;godot::PackedFloat32Array normals;normals.resize(surface.vertices.size()*4);std::size_t vi=0;
                for(const auto& v:surface.vertices){const float raw[4]{v.normal[0],v.normal[1],v.normal[2],0};std::memcpy(normals.ptrw()+vi++*4,raw,sizeof(raw));positions.push_back(host(v.position));uv.push_back({v.uv[0],v.uv[1]});const auto c=v.raw_color;
                    if(!v.has_color)throw std::runtime_error("Original track VIF colour stream is absent");
                    colors.push_back(godot::Color(float((c&31)*8)/255.f,float(((c>>5)&31)*8)/255.f,float(((c>>10)&31)*8)/255.f,float((c&0x8000)?128:0)/255.f));}
                for(auto i:surface.indices)indices.push_back(int32_t(i));
                godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);arrays[godot::Mesh::ARRAY_VERTEX]=positions;arrays[godot::Mesh::ARRAY_TEX_UV]=uv;arrays[godot::Mesh::ARRAY_COLOR]=colors;arrays[godot::Mesh::ARRAY_INDEX]=indices;arrays[godot::Mesh::ARRAY_CUSTOM0]=normals;
                godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();
                mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays,{}, {},uint64_t(godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT)<<godot::Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT);
                if(mesh->get_surface_count()!=1)throw std::runtime_error("Track mesh upload rejected");
                impl_->meshes.emplace(std::make_pair(impl_->solid_cursor,surface_index),mesh);impl_->triangles+=surface.indices.size()/3;
                if(yield())return;
            }impl_->surface_cursor=0;++impl_->solid_cursor;
        }
        while(impl_->instance_cursor<scene.instances.size()){
            const auto& instance=scene.instances[impl_->instance_cursor++];
            const godot::AABB bounds(host(instance.bbox_min),host(instance.bbox_max)-host(instance.bbox_min));
            impl_->bounds=impl_->has_bounds?impl_->bounds.merge(bounds):bounds;impl_->has_bounds=true;
            if(yield())return;
        }
        if(!impl_->has_bounds)throw std::runtime_error("Original track contains no supported render surfaces");
        const auto debug_failure=catch_all([&]{build_debug_display();});
        if(!debug_failure.empty())impl_->debug_error=debug_failure;
        impl_->loading=false;impl_->ok=true;impl_->phase="Loaded";apply_debug_display();
    });
    if(!failure.empty()){release_display();impl_->scene.reset();impl_->environment.reset();impl_->loading=false;impl_->ok=false;impl_->phase="Failed";impl_->error=failure;}
    return get_state();
}
// Tool overlay only: source topology vertices and physics remain unchanged. The
// 0.015 host-up offset prevents coplanar flicker; normal depth occlusion stays on.
void Ps2TrackRenderer::build_debug_display(){
    if(!impl_->debug||!impl_->debug_error.empty())return;
    if(impl_->debug->triangles.empty()){impl_->debug_error="Track topology has no wheel-contact polygons";return;}
    const auto make_mesh=[&](bool edges){
        godot::PackedVector3Array positions;godot::PackedColorArray colors;
        const auto append=[&](const auto& primitive){for(const auto& vertex:primitive.vertices){positions.push_back(host(vertex)+godot::Vector3(0,0.015f,0));colors.push_back(godot::Color(primitive.color[0],primitive.color[1],primitive.color[2],1));}};
        if(edges)for(const auto& edge:impl_->debug->edges)append(edge);
        else for(const auto& triangle:impl_->debug->triangles)append(triangle);
        godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);arrays[godot::Mesh::ARRAY_VERTEX]=positions;arrays[godot::Mesh::ARRAY_COLOR]=colors;
        godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();mesh->add_surface_from_arrays(edges?godot::Mesh::PRIMITIVE_LINES:godot::Mesh::PRIMITIVE_TRIANGLES,arrays);
        if(mesh->get_surface_count()!=1)throw std::runtime_error("Track debug mesh upload rejected");
        auto* node=memnew(godot::MeshInstance3D);node->set_mesh(mesh);node->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);node->set_visible(false);add_child(node);impl_->nodes.push_back(node);return node;
    };
    godot::Ref<godot::Shader> fill;fill.instantiate();fill->set_code(embedded_shaders::track_debug_fill);
    impl_->debug_material.instantiate();impl_->debug_material->set_shader(fill);impl_->debug_material->set_render_priority(3);
    impl_->debug_fill=make_mesh(false);impl_->debug_fill->set_material_override(impl_->debug_material);
    godot::Ref<godot::Shader> edge_shader;edge_shader.instantiate();edge_shader->set_code(embedded_shaders::track_debug_edges);
    godot::Ref<godot::ShaderMaterial> edge_material;edge_material.instantiate();edge_material->set_shader(edge_shader);edge_material->set_render_priority(4);
    impl_->debug_edges=make_mesh(true);impl_->debug_edges->set_material_override(edge_material);
}
void Ps2TrackRenderer::apply_debug_display(){
    const bool ready=impl_->ok&&impl_->debug_error.empty();
    if(impl_->debug_fill)impl_->debug_fill->set_visible(ready&&(impl_->render_collision||impl_->render_material));
    if(impl_->debug_edges)impl_->debug_edges->set_visible(ready&&impl_->render_collision);
    if(impl_->debug_material.is_valid())impl_->debug_material->set_shader_parameter("material_mode",impl_->render_material);
}
void Ps2TrackRenderer::set_render_collision(bool enabled){const auto failure=catch_all([&]{if(!requests_enabled())throw std::logic_error("TrackViewer is disabled in Release builds");impl_->render_collision=enabled;apply_debug_display();});if(!failure.empty())impl_->debug_error=failure;}
void Ps2TrackRenderer::set_render_material(bool enabled){const auto failure=catch_all([&]{if(!requests_enabled())throw std::logic_error("TrackViewer is disabled in Release builds");impl_->render_material=enabled;apply_debug_display();});if(!failure.empty())impl_->debug_error=failure;}

godot::Dictionary Ps2TrackRenderer::update_view(const godot::Vector3& camera_position){
    const auto failure=catch_all([&]{
        if(!impl_->ok||!impl_->math)return;
        auto* viewport=get_viewport();auto* camera=viewport?viewport->get_camera_3d():nullptr;
        if(!camera)throw std::runtime_error("Shared track rendering requires an active Camera3D");
        port::track::FrameView view;view.position={float(camera_position.z),float(camera_position.x),float(camera_position.y)};
        const auto direction=-camera->get_global_transform().basis.get_column(2);
        view.direction={float(direction.z),float(direction.x),float(direction.y)};
        const auto projection=camera->get_camera_projection();
        const auto clip=projection*godot::Projection(camera->get_global_transform().affine_inverse());
        const int columns[]{2,0,1,3};for(int c=0;c<4;++c)for(int r=0;r<4;++r)view.clip_matrix[c*4+r]=clip[columns[c]][r];
        view.focal_h=float(viewport->get_visible_rect().size.x)*.5f*projection[0][0];
        auto frame=impl_->runtime.prepare_frame(view,*impl_->math);
        for(auto& entry:impl_->draws)entry.second.node->set_visible(false);
        // FNV byte digest includes ordered native command state, never GPU handles.
        std::uint64_t digest=14695981039346656037ull;
        const auto hash=[&](const auto& value){const auto* bytes=reinterpret_cast<const unsigned char*>(&value);for(std::size_t i=0;i<sizeof(value);++i){digest^=bytes[i];digest*=1099511628211ull;}};
        hash(impl_->number);hash(view.position);hash(view.direction);hash(view.clip_matrix);hash(view.focal_h);hash(frame.zone);for(auto section:frame.sections)hash(section);
        std::size_t rank=0;impl_->triangles=0;
        for(const auto& command:frame.commands){
            const auto& scene=*impl_->scene;const auto& surface=scene.geometry.solids.at(command.solid).surfaces.at(command.surface);
            hash(command.instance);hash(command.solid);hash(command.surface);hash(command.lod);hash(command.visibility);hash(command.original_order);hash(command.matrix);hash(command.light_context);hash(command.identity);hash(command.has_light_context);hash(command.depth);
            hash(command.effect.flags);hash(command.effect.alpha_pass);hash(command.effect.rewrite_alpha);hash(command.effect.matrix_pack);hash(command.effect.rendering_order);hash(command.effect.texture.alpha_register);hash(command.effect.texture.tex1_register);hash(command.effect.test.test_register);hash(command.effect.test.write_depth);hash(surface.texture_hash);hash(surface.material_hash);
            const auto native_passes=port::track::pass_sequence(command.effect,surface.texture_hash);for(const auto& pass:native_passes){hash(pass.kind);hash(pass.gs_context);hash(pass.alpha_register);hash(pass.texture_hash);hash(pass.state_available);}
            const auto material_data=scene.materials.find(surface.material_hash);if(material_data!=scene.materials.end())hash(material_data->second.blocks);
            const auto mesh=impl_->meshes.find({command.solid,command.surface});if(mesh==impl_->meshes.end())throw std::runtime_error("Native track frame references an absent GPU surface");
            const auto key=std::make_tuple(command.instance,command.solid,command.surface);auto found=impl_->draws.find(key);
            if(found==impl_->draws.end()){
                const auto& header=impl_->texture_info.at(surface.texture_hash)->header;
                auto fusion=port::track::Fusion40::None;
                if(command.effect.second_pass_320){const auto a=impl_->alpha_ranges.at(surface.texture_hash),b=impl_->alpha_ranges.at(0x0184d1f8);fusion=port::track::fusion_40(command.effect,a[0],a[1],b[0],b[1]);if(fusion==port::track::Fusion40::None)throw std::runtime_error("Track flag40 destination-alpha pass exceeds proven host fusion coverage");}
                if(command.effect.environment_pass){
                    const auto missing=catch_all([&]{hp2::not_implemented("0x0010AC50","Retail track context2 dynamic six-view environment capture");});
                    const auto gap=std::string("Environment reflection gap: ")+scene.geometry.solids.at(command.solid).name+"; base pass only. "+missing+"\n";
                    if(impl_->diagnostic.find(gap)==std::string::npos)impl_->diagnostic+=gap;
                }
                const auto code=track_shader_source(header,command.effect,fusion);
                auto shader=impl_->shaders.find(code);if(shader==impl_->shaders.end()){godot::Ref<godot::Shader> value;value.instantiate();value->set_code(text(code));shader=impl_->shaders.emplace(code,value).first;}
                Impl::Draw draw;draw.material.instantiate();draw.material->set_shader(shader->second);
                draw.material->set_shader_parameter("base_texture",impl_->textures.at(surface.texture_hash));
                draw.material->set_shader_parameter("base_lit",bool(command.effect.flags&0x80));draw.material->set_shader_parameter("flag_0x4",bool(command.effect.flags&4));
                for(int level=1;level<=header.Field_0x6e;++level)draw.material->set_shader_parameter(text("mip_"+std::to_string(level)),impl_->mipmaps.at(surface.texture_hash).at(level-1));
                if(command.effect.second_pass_320)draw.material->set_shader_parameter("specular_texture",impl_->textures.at(0x0184d1f8));
                const auto material=scene.materials.find(surface.material_hash);
                if(command.effect.flags&0x1c0){if(material==scene.materials.end())throw std::runtime_error("Native track frame has unresolved lighting material");const auto& block=material->second.blocks;for(int i=0;i<4;++i)draw.material->set_shader_parameter(text("material_"+std::to_string(350+i)),godot::Vector4(block[i*4],block[i*4+1],block[i*4+2],block[i*4+3]));}
                draw.material->set_render_priority(command.effect.alpha_pass?64:-64);
                draw.node=memnew(godot::MeshInstance3D);draw.node->set_meta("track_texture_hash",int64_t(surface.texture_hash));draw.node->set_meta("track_effect_flags",command.effect.flags);draw.node->set_meta("track_matrix_pack",command.effect.matrix_pack);draw.node->set_mesh(mesh->second);draw.node->set_material_override(draw.material);draw.node->set_sorting_use_aabb_center(false);draw.node->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);add_child(draw.node);impl_->nodes.push_back(draw.node);found=impl_->draws.emplace(key,std::move(draw)).first;
            }
            auto& draw=found->second;const auto transform=command.identity?godot::Transform3D():host_matrix(command.matrix);draw.node->set_transform(transform);
            const auto local=transform.affine_inverse().xform(camera_position);
            draw.material->set_shader_parameter("source_camera",godot::Vector3(local.z,local.x,local.y));
            const char* names[]{"35c","35d","35e","35f","360","361","362"};for(int i=0;i<7;++i)draw.material->set_shader_parameter(text(std::string("light_")+names[i]),godot::Vector4(command.light_context[i*4],command.light_context[i*4+1],command.light_context[i*4+2],command.light_context[i*4+3]));
            draw.material->set_shader_parameter("depth_near",camera->get_near());draw.material->set_shader_parameter("depth_far",camera->get_far());
            // Godot4.4.1 GPU ordering probe confirms depth - sorting_offset.
            // Cancel host origin depth and assign a strict descending native order.
            const float origin_depth=(draw.node->get_global_position()-camera->get_global_position()).dot(direction);
            draw.node->set_sorting_offset(origin_depth-float(frame.commands.size()-rank)*4096.f);
            draw.node->set_visible(true);++rank;impl_->triangles+=surface.indices.size()/3;
        }
        std::ostringstream label;label<<std::hex<<std::setfill('0')<<std::setw(16)<<digest;impl_->packet_digest=label.str();impl_->frame=std::move(frame);
    });
    if(!failure.empty()){release_display();impl_->scene.reset();impl_->environment.reset();impl_->ok=false;impl_->loading=false;impl_->phase="Failed";impl_->error=failure;}
    return get_state();
}

godot::Dictionary Ps2TrackRenderer::get_state()const{
    godot::Dictionary result;const auto failure=catch_all([&]{result["ok"]=impl_->ok;result["loading"]=impl_->loading;result["track_id"]=impl_->number;
    result["render_collision"]=impl_->render_collision;result["render_material"]=impl_->render_material;
    result["debug_ready"]=impl_->ok&&impl_->debug_fill&&impl_->debug_error.empty();
    result["debug_diagnostic"]=text(impl_->debug_error);
    result["debug_fill_visible"]=impl_->debug_fill&&impl_->debug_fill->is_visible();
    result["debug_edges_visible"]=impl_->debug_edges&&impl_->debug_edges->is_visible();
    godot::Array legend;
    if(impl_->debug)for(const auto& row:impl_->debug->legend){godot::Dictionary item;item["id"]=row.id;item["name"]=text(row.name);item["color"]=godot::Color(row.color[0],row.color[1],row.color[2],1);item["polygon_count"]=int64_t(row.polygon_count);item["known"]=row.known;legend.push_back(item);}
    result["material_legend"]=legend;
    result["debug_triangles"]=int64_t(impl_->debug?impl_->debug->triangles.size():0);
    result["debug_edges"]=int64_t(impl_->debug?impl_->debug->edges.size():0);
    result["packet_digest"]=text(impl_->packet_digest);result["commands"]=int64_t(impl_->frame.commands.size());result["zone"]=impl_->frame.zone;result["visible_instances"]=int64_t(impl_->frame.visible_instances);
    godot::Array sections;for(auto section:impl_->frame.sections)sections.push_back(section);result["sections"]=sections;
    result["phase"]=text(impl_->phase);result["error"]=text(impl_->error);result["diagnostic"]=text(impl_->diagnostic);
    result["bounds"]=impl_->bounds;result["triangles"]=int64_t(impl_->triangles);
    result["render_nodes"]=int64_t(impl_->nodes.size());result["batched_instances"]=int64_t(impl_->batched_instances);
    result["summary"]=text("Original track: "+std::to_string(impl_->triangles)+" triangles / "+std::to_string(impl_->textures.size())+" textures");
    if(impl_->environment){result["start_position"]=host(impl_->environment->start);result["start_forward"]=godot::Vector3(impl_->environment->direction[1],0,impl_->environment->direction[0]);}
    if(impl_->scene)result["track_name"]=text(impl_->scene->entry.name);
    });
    if(!failure.empty()){result["ok"]=false;result["error"]=text(failure);}return result;
}
}

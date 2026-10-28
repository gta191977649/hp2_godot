#include "game_core/port/track/environment.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include "ps2_vehicle_renderer.hpp"
#include "particle_billboard_renderer.hpp"
#include "game_core/port/driving/original_image.hpp"
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include "vehicle_depth_staging.hpp"
#include <godot_cpp/classes/os.hpp>
#include "game_core/port/track/reflection.hpp"
#include "boundary.hpp"
#include "platform/shaders/car_base_shader.hpp"
#include "platform/ps2_asset_root.hpp"
#include "port/vehicle/model.hpp"
#include "port/vehicle/loader.hpp"
#include "game_core/CarRenderInfo.hpp"
#include <chrono>
#include "port/vehicle/base_material.hpp"
#include "port/vehicle/texture_state.hpp"
#include "game_core/port/frontend/selection_render.hpp"
#include "game_core/src/ecstasy/elighte.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <cstring>
#include <map>
#include <sstream>
#include <limits>
#include <stdexcept>

namespace port::godot_bridge {
namespace {
godot::Vector3 host(const std::array<float,3>& p){return {p[1],p[2],p[0]};}
std::array<float,3> source(const godot::Vector3& p){return {{p.z,p.x,p.y}};}
godot::Transform3D host_matrix(const std::array<float,16>& m) {
    // Port-side orientation-preserving basis permutation; source rows become columns.
    godot::Basis b;
    b.set_column(0,host({{m[4],m[5],m[6]}}));
    b.set_column(1,host({{m[8],m[9],m[10]}}));
    b.set_column(2,host({{m[0],m[1],m[2]}}));
    return {b,host({{m[12],m[13],m[14]}})};
}
std::array<float,16> source_matrix(const godot::Transform3D& m) {
    std::array<float,16> result{};
    for(int row=0;row<3;++row){const auto v=source(m.basis.get_column((row+2)%3));for(int axis=0;axis<3;++axis)result[row*4+axis]=v[axis];}
    const auto translation=source(m.origin);for(int axis=0;axis<3;++axis)result[12+axis]=translation[axis];
    result[15]=1;return result;
}
godot::AABB part_bounds(godot::MeshInstance3D* node){
    auto bounds=node->get_aabb();
    for(int i=0;i<node->get_child_count();++i)
        if(auto* batch=godot::Object::cast_to<godot::MeshInstance3D>(node->get_child(i)))
            bounds=bounds.merge(batch->get_transform().xform(batch->get_aabb()));
    return bounds;
}
godot::String string(const std::string& s){return godot::String::utf8(s.c_str());}
port::assets::DirectorySource assets() {
    const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");
    const auto root=hp2::platform::resolve_zzdata_root(start.utf8().get_data());
    if(!root)throw std::runtime_error("Original ZZDATA root is not configured");
    return port::assets::DirectorySource(*root);
}
}
namespace {
std::unique_ptr<port::vehicle::Loader>& loader_slot(){static std::unique_ptr<port::vehicle::Loader> instance;return instance;}
port::vehicle::Loader& loader(){auto& instance=loader_slot();if(!instance)instance=std::make_unique<port::vehicle::Loader>();return *instance;}
struct GpuCache {
    struct Image {godot::Ref<godot::ImageTexture> value;std::size_t bytes;std::uint64_t age;unsigned max_alpha;};
    struct Mesh {std::vector<godot::Ref<godot::ArrayMesh>> value;std::size_t bytes;std::uint64_t age;};
    std::map<std::uint32_t,Image> images;std::map<std::string,Mesh> meshes;std::map<std::string,godot::Ref<godot::Shader>> shaders;
    std::string root;std::size_t bytes=0;std::uint64_t clock=0,hits=0;
    void trim(){while(bytes>128u*1024u*1024u&&(!images.empty()||!meshes.empty())){
        auto image=images.end();for(auto it=images.begin();it!=images.end();++it)if(image==images.end()||it->second.age<image->second.age)image=it;
        auto mesh=meshes.end();for(auto it=meshes.begin();it!=meshes.end();++it)if(mesh==meshes.end()||it->second.age<mesh->second.age)mesh=it;
        if(image!=images.end()&&(mesh==meshes.end()||image->second.age<mesh->second.age)){bytes-=image->second.bytes;images.erase(image);}else{bytes-=mesh->second.bytes;meshes.erase(mesh);}
    }}
};
std::string mesh_key(const port::render::VehicleSolid& solid){
    std::string key;for(const auto& surface:solid.surfaces){
        key.append(reinterpret_cast<const char*>(&surface.render_flags),sizeof(surface.render_flags));
        const auto count=surface.vertices.size();key.append(reinterpret_cast<const char*>(&count),sizeof(count));
        for(const auto& v:surface.vertices){key.append(reinterpret_cast<const char*>(v.position.data()),sizeof(float)*3);key.append(reinterpret_cast<const char*>(v.uv.data()),sizeof(float)*2);key.append(reinterpret_cast<const char*>(v.normal.data()),sizeof(float)*3);key.append(reinterpret_cast<const char*>(&v.raw_color),sizeof(v.raw_color));}
        const auto indices=surface.indices.size();key.append(reinterpret_cast<const char*>(&indices),sizeof(indices));key.append(reinterpret_cast<const char*>(surface.indices.data()),indices*sizeof(std::uint32_t));
    }return key;
}
}
struct Ps2VehicleRenderer::Impl {
    std::uint32_t requested_frontend_solid=0;
    std::uint32_t frontend_flags=0;
    std::optional<hp2::debug::SelectCarLights> frontend_lights;
    std::array<float,16> frontend_view{};
    std::uint64_t frontend_light_revision=0;
    struct Draw {
        godot::Ref<godot::ShaderMaterial> material;
        std::size_t part{},surface{};
        bool source_over=false,flag_0x4=false;
        unsigned maximum_texture_alpha=0;
        godot::MeshInstance3D* node=nullptr;
        bool bounded_alpha=false;
        bool lighting_valid=false;
        std::uint64_t frontend_light_revision=0;
        godot::Basis lighting_basis;
        std::array<float,28> lights{};
        godot::Ref<godot::ShaderMaterial> reflection;
        std::size_t batch=0,batch_surface=0;
        vehicle_depth_staging::Owner depth_parameters;
        std::uint32_t bound_texture=0;
        bool apply_alpha_sorting=false,frontend_flags_bound=false;
    };
    struct Texture {godot::Ref<godot::ImageTexture> image;const port::vehicle::Texture* data;unsigned max_alpha;};
    std::map<std::uint32_t,Texture> uploaded;
    bool capture_requested=false,capture_ready=false,capture_enabled=false;
    godot::Ref<godot::Texture2D> capture_texture;
    std::array<float,16> capture_hack{};
    std::string capture_diagnostic;
    bool capture_binding_valid=false,capture_bound_ready=false,capture_bound_enabled=false;
    std::uint64_t capture_bound_texture=0,capture_bound_generation=0;
    std::size_t capture_bound_draw_count=0;
    std::string capture_binding_error;
    std::shared_ptr<GpuCache> gpu=std::make_shared<GpuCache>();
    std::size_t texture_cursor=0,part_cursor=0,surface_cursor=0;
    std::vector<godot::Ref<godot::ArrayMesh>> building_meshes;
    std::vector<godot::Ref<godot::ShaderMaterial>> building_materials;
    std::string building_key;bool cached_mesh=false,first_bounds=true;
    bool driving_pose=false,vehicle_depth_staging=false;
    port::driving::VehicleLightState lights;
    port::driving::VehicleFlareState flare_state;bool flare_state_available=false;
    std::optional<port::driving::VehicleFlareResources> flare_resources;
    std::unique_ptr<ParticleBillboardRenderer> flare_renderer;
    std::vector<port::driving::VehicleFlareBillboard> flare_billboards;
    std::string flare_error;
    double wheel_speed=80;
    bool show_lods=false,wheel_blur=false,loading=false;std::uint64_t generation=0,completed=0;
    std::optional<port::vehicle::Loader::Ticket> ticket;
    std::unique_ptr<Impl> staging;
    std::vector<godot::MeshInstance3D*> compare_nodes;
    std::vector<int> node_lods;std::vector<std::size_t> node_parts;
    godot::Array labels;
    std::vector<godot::Label3D*> tags;
    double cpu_ms=0,read_ms=0,geometry_decode_ms=0,texture_decode_ms=0,binding_ms=0,upload_ms=0,max_upload_slice_ms=0;std::uint64_t file_reads=0;
    std::chrono::steady_clock::time_point upload_start;
    std::uint64_t geometry_reads=0,geometry_hits=0,texture_decodes=0,texture_hits=0;
    std::map<int,std::array<float,3>> previews;std::map<int,std::string> preview_errors;
    std::unique_ptr<port::vehicle::Model> model;
    std::vector<godot::MeshInstance3D*> nodes;
    std::vector<Draw> draws;
    godot::MeshInstance3D* bounds_node=nullptr;
    godot::Ref<godot::ShaderMaterial> wire;
    godot::AABB bounds;
    bool wireframe=false,show_bounds=false,inspection=false,display_ready=false;
    int lod=0,projected_size=0,triangles=0;
    int requested_car=-1,requested_skin=-1;
    std::string error,requested_path;
};
void Ps2VehicleRenderer::shutdown_loader(){loader_slot().reset();}
Ps2VehicleRenderer::Ps2VehicleRenderer():impl_(std::make_unique<Impl>()){
    const auto failure=catch_all([&]{const auto args=godot::OS::get_singleton()->get_cmdline_user_args();
        for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-vehicle-depth-staging=")){
            const auto value=args[i].trim_prefix("--track-vehicle-depth-staging=");
            if(value!="true"&&value!="false")throw std::invalid_argument("Vehicle depth staging requires true or false");
            impl_->vehicle_depth_staging=value=="true";
        }
    });if(!failure.empty())impl_->error=failure;
}
Ps2VehicleRenderer::~Ps2VehicleRenderer(){cancel_load();}
void Ps2VehicleRenderer::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("apply_driving_snapshot","snapshot"),&Ps2VehicleRenderer::apply_driving_snapshot);
    godot::ClassDB::bind_method(godot::D_METHOD("request_car","relative_path","car_id","skin_id"),&Ps2VehicleRenderer::request_car);
    godot::ClassDB::bind_method(godot::D_METHOD("request_frontend_solid","hash"),&Ps2VehicleRenderer::request_frontend_solid);
    godot::ClassDB::bind_method(godot::D_METHOD("pump_load"),&Ps2VehicleRenderer::pump_load);
    godot::ClassDB::bind_method(godot::D_METHOD("set_show_lods","enabled"),&Ps2VehicleRenderer::set_show_lods);
    godot::ClassDB::bind_method(godot::D_METHOD("set_wheel_blur","enabled"),&Ps2VehicleRenderer::set_wheel_blur);
    godot::ClassDB::bind_method(godot::D_METHOD("set_wheel_speed","radians_per_second"),&Ps2VehicleRenderer::set_wheel_speed);
    godot::ClassDB::bind_method(godot::D_METHOD("advance_wheels","delta"),&Ps2VehicleRenderer::advance_wheels);
    godot::ClassDB::bind_method(godot::D_METHOD("load_car","relative_path"),&Ps2VehicleRenderer::load_car);
    godot::ClassDB::bind_method(godot::D_METHOD("get_catalog"),&Ps2VehicleRenderer::get_catalog);
    godot::ClassDB::bind_method(godot::D_METHOD("load_car_id","car_id","skin_id"),&Ps2VehicleRenderer::load_car_id);
    godot::ClassDB::bind_method(godot::D_METHOD("set_skin","skin_id"),&Ps2VehicleRenderer::set_skin);
    godot::ClassDB::bind_method(godot::D_METHOD("set_inspection","enabled"),&Ps2VehicleRenderer::set_inspection);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_car"),&Ps2VehicleRenderer::clear_car);
    godot::ClassDB::bind_method(godot::D_METHOD("update_view","position","forward","projection_scale","forced_lod"),&Ps2VehicleRenderer::update_view);
    godot::ClassDB::bind_method(godot::D_METHOD("get_state"),&Ps2VehicleRenderer::get_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_visual_state"),&Ps2VehicleRenderer::get_visual_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_visual_texture","hash"),&Ps2VehicleRenderer::get_visual_texture);
    godot::ClassDB::bind_method(godot::D_METHOD("set_environment_capture","capture"),&Ps2VehicleRenderer::set_environment_capture);
    godot::ClassDB::bind_method(godot::D_METHOD("set_part_visible","name","visible"),&Ps2VehicleRenderer::set_part_visible);
    godot::ClassDB::bind_method(godot::D_METHOD("set_wireframe","enabled"),&Ps2VehicleRenderer::set_wireframe);
    godot::ClassDB::bind_method(godot::D_METHOD("set_bounds","enabled"),&Ps2VehicleRenderer::set_bounds);
}
void Ps2VehicleRenderer::release_display() {
    impl_->display_ready=false;impl_->driving_pose=false;
    impl_->frontend_flags=0;
    impl_->lights={};impl_->flare_state={};impl_->flare_state_available=false;impl_->flare_renderer.reset();impl_->flare_billboards.clear();impl_->flare_error.clear();
    impl_->capture_binding_valid=false;impl_->capture_binding_error.clear();
    impl_->capture_ready=false;impl_->capture_texture.unref();impl_->capture_bound_texture=0;
    for(auto* node:impl_->nodes){remove_child(node);memdelete(node);}
    impl_->nodes.clear();impl_->draws.clear();
    for(auto* tag:impl_->tags){remove_child(tag);memdelete(tag);}impl_->tags.clear();
    for(auto* node:impl_->compare_nodes){remove_child(node);memdelete(node);}impl_->compare_nodes.clear();impl_->node_lods.clear();impl_->node_parts.clear();impl_->labels.clear();
    impl_->uploaded.clear();impl_->texture_cursor=impl_->part_cursor=impl_->surface_cursor=0;impl_->building_meshes.clear();impl_->building_materials.clear();impl_->first_bounds=true;
    if(impl_->bounds_node){remove_child(impl_->bounds_node);memdelete(impl_->bounds_node);impl_->bounds_node=nullptr;}
    impl_->wire.unref();impl_->bounds={};impl_->triangles=0;
}
void Ps2VehicleRenderer::clear_car() {
    const auto error=catch_all([&] {
        cancel_load();release_display();impl_->model.reset();impl_->gpu=std::make_shared<GpuCache>();impl_->lod=0;impl_->projected_size=0;impl_->error.clear();
        impl_->requested_car=impl_->requested_skin=-1;
    });
    if(!error.empty())impl_->error=error;
}
godot::Array Ps2VehicleRenderer::get_catalog() {
    godot::Array result;
    const auto error=catch_all([&] {
        if(!requests_enabled())return;
        auto input=assets();port::vehicle::Catalog catalog(input);catalog.load();
        for(const auto& car:catalog.cars()) {
            godot::Dictionary row;row["id"]=int(car.id);row["code"]=string(car.code);
            row["path"]=string(car.resource_path);row["original_path"]=string(car.original_resource_path);
            row["available"]=car.available;row["error"]=string(car.availability_error);
            godot::Array skins;
            for(const auto& skin:car.skins){godot::Dictionary color;color["id"]=int(skin.id);color["name"]=string(skin.name);color["material_hash"]=int64_t(skin.material_hash);skins.push_back(color);}
            row["skins"]=skins;result.push_back(row);
        }
    });
    if(!error.empty())impl_->error=error;
    return result;
}
godot::Dictionary Ps2VehicleRenderer::load_car(const godot::String& path){return load_requested(path,-1,1);}
godot::Dictionary Ps2VehicleRenderer::load_car_id(int car,int skin){return load_requested({},car,skin);}
godot::Dictionary Ps2VehicleRenderer::set_skin(int skin){
    return load_requested({},impl_->model?impl_->model->car_id():impl_->requested_car,skin);
}
godot::Dictionary Ps2VehicleRenderer::load_requested(const godot::String& path,int car,int skin) {
    cancel_load();release_display();impl_->error.clear();impl_->requested_car=car;impl_->requested_skin=skin;
    const auto error=catch_all([&] {
        if(!requests_enabled())throw std::logic_error("CarViewer is disabled in Release builds");
        auto input=assets();if(impl_->gpu->root!=input.root()){impl_->gpu=std::make_shared<GpuCache>();impl_->gpu->root=input.root();}
        if(!impl_->model)impl_->model=std::make_unique<port::vehicle::Model>();
        impl_->model->include_lods(impl_->show_lods);impl_->model->include_wheel_blur(impl_->wheel_blur);
        if(car>=0)impl_->model->load_id(input,car,skin);
        else impl_->model->load(input,path.utf8().get_data());
        impl_->model->set_wheel_speed(impl_->wheel_speed);rebuild_display();
    });
    if(!error.empty()){clear_car();impl_->error=error;impl_->requested_car=car;impl_->requested_skin=skin;}
    return get_state();
}
void Ps2VehicleRenderer::rebuild_display(bool incremental) {
    const auto slice_start=std::chrono::steady_clock::now();
    const auto yield=[&]{return incremental&&std::chrono::steady_clock::now()-slice_start>=std::chrono::milliseconds(2);};
    const auto& model=*impl_->model;
    auto& textures=impl_->uploaded;auto& gpu=*impl_->gpu;
    for(;impl_->texture_cursor<model.textures().size();++impl_->texture_cursor){
        const auto& texture=model.textures()[impl_->texture_cursor];
        auto cached=gpu.images.find(texture.header.tex_hash);
        if(cached!=gpu.images.end()){++gpu.hits;cached->second.age=++gpu.clock;textures.emplace(texture.header.tex_hash,Impl::Texture{cached->second.value,&texture,cached->second.max_alpha});continue;}
        godot::PackedByteArray bytes;bytes.resize(texture.bitmap.rgba.size());
        std::memcpy(bytes.ptrw(),texture.bitmap.rgba.data(),texture.bitmap.rgba.size());
        const auto image=godot::Image::create_from_data(texture.bitmap.width,texture.bitmap.height,false,godot::Image::FORMAT_RGBA8,bytes);
        if(image.is_null())throw std::runtime_error("Could not create vehicle texture image");
        unsigned maximum=0;for(std::size_t i=3;i<texture.bitmap.rgba.size();i+=4)maximum=std::max(maximum,unsigned(texture.bitmap.rgba[i]));
        const auto uploaded=godot::ImageTexture::create_from_image(image);
        textures.emplace(texture.header.tex_hash,Impl::Texture{uploaded,&texture,maximum});
        gpu.bytes+=texture.bitmap.rgba.size();gpu.images.emplace(texture.header.tex_hash,GpuCache::Image{uploaded,texture.bitmap.rgba.size(),++gpu.clock,maximum});gpu.trim();
        if(yield()){++impl_->texture_cursor;return;}
    }
    for(;impl_->part_cursor<model.parts().size();++impl_->part_cursor) {
        const auto part_index=impl_->part_cursor;
        const auto& part=model.parts()[part_index];
        const auto& solid=model.geometry().solids[part.solid];
        if(impl_->building_meshes.empty()){
            impl_->building_key=mesh_key(solid);auto cached=gpu.meshes.find(impl_->building_key);impl_->cached_mesh=cached!=gpu.meshes.end();
            if(impl_->cached_mesh){impl_->building_meshes=cached->second.value;cached->second.age=++gpu.clock;++gpu.hits;}else {godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();impl_->building_meshes.push_back(mesh);}
        }
        for(;impl_->surface_cursor<solid.surfaces.size();++impl_->surface_cursor) {
            const auto surface_index=impl_->surface_cursor;
            const auto& surface=solid.surfaces[surface_index];
            if((surface.render_flags&0x400)||surface.indices.empty())continue;
            // Port-side capacity split only. Do not merge/reorder original strips or materials.
            const auto batch=impl_->building_materials.size()/godot::RenderingServer::MAX_MESH_SURFACES;
            if(batch==impl_->building_meshes.size()){
                if(impl_->cached_mesh)throw std::runtime_error("Cached vehicle mesh batch is missing");
                godot::Ref<godot::ArrayMesh> chunk;chunk.instantiate();impl_->building_meshes.push_back(chunk);
            }
            auto mesh=impl_->building_meshes.at(batch);
            if(!impl_->inspection && (surface.render_flags&0x40))hp2::not_implemented("0x002A8308","Viewer VU flag0x40 material route");
            const auto& texture=textures.at(surface.texture_hash);
            const auto state=port::vehicle::texture_state_00108CC0(texture.data->header);
            const auto test=port::vehicle::test_state_00105AC0(texture.data->header);
            const bool blend=state.blend==port::vehicle::BlendMode::SourceOver;
            std::string modes="fog_disabled, shadows_disabled, ";
            modes+=test.write_depth?"depth_draw_always":"depth_draw_never";
            if(blend||state.deferred)modes+=", blend_mix";
            // Host derivative selection projects the original MXL0 mag-linear/min-nearest state.
            // Raster/filter subpixel precision is not claimed to match GS hardware.
            const auto fragment=car_fragment_source(impl_->inspection,test.alpha_test,blend,state.deferred);
            const auto code=car_base_shader_source(modes,fragment,"filter_linear, repeat_disable");
            auto shader=gpu.shaders.find(code);if(shader==gpu.shaders.end()){godot::Ref<godot::Shader> value;value.instantiate();value->set_code(string(code));shader=gpu.shaders.emplace(code,value).first;}
            godot::Ref<godot::ShaderMaterial> material;material.instantiate();material->set_shader(shader->second);
            // E0055V01: closed BASE-material writer contract for the optional
            // depth-proxy cache. Static parameters are initialized below;
            // update_view also swaps original head/brake texture states. Dynamic
            // texture/suppression fields use the depth consumer's dynamic fallback;
            // the eight camera/lighting parameters retain their staging contract.
            godot::PackedStringArray depth_dynamic;
            for(const char* name:{"source_camera","light_35c","light_35d","light_35e","light_35f","light_360","light_361","light_362","base_texture","flag_0x4","frontend_skip_strip"})depth_dynamic.push_back(name);
            material->set_meta("hp2_depth_dynamic_uniforms",depth_dynamic);
            material->set_meta("hp2_depth_uniform_contract",1);
            material->set_render_priority(state.deferred?2:0);
            material->set_shader_parameter("base_texture",texture.image);
            material->set_shader_parameter("base_lit",!impl_->inspection&&bool(surface.render_flags&0x80));
            material->set_shader_parameter("flag_0x4",bool(surface.render_flags&4)||texture.data->header.Field_0x74!=0);
            const auto& block=model.materials().at(surface.material_hash).blocks;
            for(int i=0;i<4;++i)material->set_shader_parameter(string("material_"+std::to_string(350+i)),godot::Vector4(block[i*4],block[i*4+1],block[i*4+2],block[i*4+3]));
            for(int i=0;i<7;++i){const auto& l=part.lights;const std::string suffix[]={"35c","35d","35e","35f","360","361","362"};material->set_shader_parameter(string("light_"+suffix[i]),godot::Vector4(l[i*4],l[i*4+1],l[i*4+2],l[i*4+3]));}
            if(!impl_->cached_mesh){
            godot::PackedVector3Array positions;godot::PackedVector2Array uv;godot::PackedInt32Array indices;godot::PackedColorArray colors;godot::PackedFloat32Array normals;
            normals.resize(surface.vertices.size()*4);
            for(std::size_t i=0;i<surface.vertices.size();++i){
                const auto& vertex=surface.vertices[i];positions.push_back(host(vertex.position));uv.push_back({vertex.uv[0],vertex.uv[1]});
                const auto c=vertex.raw_color;colors.push_back({float((c&31)*8)/255,float(((c>>5)&31)*8)/255,float(((c>>10)&31)*8)/255,float((c&0x8000)?128:0)/255});
                const float n[4]={vertex.normal[0],vertex.normal[1],vertex.normal[2],0};std::memcpy(normals.ptrw()+i*4,n,sizeof(n));
            }
            for(const auto index:surface.indices)indices.push_back(static_cast<std::int32_t>(index));
            godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);
            arrays[godot::Mesh::ARRAY_VERTEX]=positions;arrays[godot::Mesh::ARRAY_TEX_UV]=uv;arrays[godot::Mesh::ARRAY_INDEX]=indices;arrays[godot::Mesh::ARRAY_COLOR]=colors;arrays[godot::Mesh::ARRAY_CUSTOM0]=normals;
            const uint64_t flags=uint64_t(godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT)<<godot::Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT;
            const auto previous=mesh->get_surface_count();
            mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays,{}, {},flags);
            if(mesh->get_surface_count()!=previous+1)throw std::runtime_error("Godot rejected the original vehicle draw surface upload");
            }
            impl_->building_materials.push_back(material);
            impl_->draws.push_back({material,part_index,surface_index,blend,bool(surface.render_flags&4)||texture.data->header.Field_0x74!=0,texture.max_alpha});
            auto& draw=impl_->draws.back();
            draw.apply_alpha_sorting=state.deferred;
            draw.bound_texture=surface.texture_hash;
            if(impl_->vehicle_depth_staging)draw.depth_parameters=vehicle_depth_staging::create(material);
            draw.batch=batch;draw.batch_surface=(impl_->building_materials.size()-1)%godot::RenderingServer::MAX_MESH_SURFACES;
            if(!impl_->inspection&&(surface.render_flags&0x100)){
                const auto env_code=car_base_shader_source("fog_disabled, shadows_disabled, depth_draw_never, blend_add",car_fragment_source(false,5,true,false),"filter_linear, repeat_disable");
                auto env_shader=gpu.shaders.find(env_code);
                if(env_shader==gpu.shaders.end()){godot::Ref<godot::Shader> value;value.instantiate();value->set_code(string(env_code));env_shader=gpu.shaders.emplace(env_code,value).first;}
                draw.reflection.instantiate();draw.reflection->set_shader(env_shader->second);
                draw.reflection->set_render_priority(state.deferred?3:1);
                draw.reflection->set_shader_parameter("reflection_pass",true);
                draw.reflection->set_shader_parameter("base_texture",textures.at(0x4ecc769c).image);
                draw.reflection->set_shader_parameter("base_lit",bool(surface.render_flags&0x80));
                draw.reflection->set_shader_parameter("flag_0x4",draw.flag_0x4);
                for(int i=0;i<4;++i)draw.reflection->set_shader_parameter(string("material_"+std::to_string(350+i)),godot::Vector4(block[16+i*4],block[17+i*4],block[18+i*4],block[19+i*4]));
                material->set_next_pass(draw.reflection);
            }
            // Exact camera-independent alpha bound for the initialized light slice.
            const bool fixed_alpha=!draw.flag_0x4&&part.lights[3]==0&&part.lights[7]==0&&part.lights[11]==0&&part.lights[15]>=0&&part.lights[15]<=128;
            draw.bounded_alpha=texture.max_alpha<=128&&(!(surface.render_flags&0x80)||fixed_alpha);
            if(yield()){++impl_->surface_cursor;return;}
        }
        if(!impl_->cached_mesh){const auto bytes=impl_->building_key.size();gpu.bytes+=bytes;gpu.meshes.emplace(impl_->building_key,GpuCache::Mesh{impl_->building_meshes,bytes,++gpu.clock});gpu.trim();}
        auto* node=memnew(godot::MeshInstance3D);add_child(node);
        node->set_name(string(part.name));node->set_meta("hp2_part_name",string(part.name));node->set_meta("hp2_wheel_index",part.wheel);node->set_meta("hp2_part_group",int(part.group));node->set_transform(host_matrix(part.transform));
        // One native part still owns one transform/visibility root. Extra batches inherit it.
        for(std::size_t batch=0;batch<impl_->building_meshes.size();++batch){
            auto* chunk=batch==0?node:memnew(godot::MeshInstance3D);
            if(batch){node->add_child(chunk);chunk->set_name(string("Draw batch "+std::to_string(batch)));}
            const auto& mesh=impl_->building_meshes[batch];chunk->set_mesh(mesh);
            const auto first=batch*godot::RenderingServer::MAX_MESH_SURFACES;
            const auto count=std::min(std::size_t(godot::RenderingServer::MAX_MESH_SURFACES),impl_->building_materials.size()-first);
            if(std::size_t(mesh->get_surface_count())!=count)throw std::runtime_error("Vehicle mesh batch surface coverage mismatch");
            for(std::size_t i=0;i<count;++i)chunk->set_surface_override_material(int(i),impl_->building_materials[first+i]);
            chunk->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
            for(auto& draw:impl_->draws)if(draw.part==part_index&&draw.batch==batch)draw.node=chunk;
        }
        impl_->building_meshes.clear();impl_->building_materials.clear();impl_->surface_cursor=0;
        impl_->nodes.push_back(node);node->set_visible(part.selected);
        if(part.selected){const auto bounds=node->get_transform().xform(part_bounds(node));impl_->bounds=impl_->first_bounds?bounds:impl_->bounds.merge(bounds);impl_->first_bounds=false;}

    }
    layout_lods();
    godot::Ref<godot::Shader> shader;shader.instantiate();
    shader->set_code(embedded_shaders::wireframe);
    impl_->wire.instantiate();impl_->wire->set_shader(shader);
    godot::Ref<godot::ImmediateMesh> lines;lines.instantiate();
    godot::Ref<godot::StandardMaterial3D> line_material;line_material.instantiate();
    line_material->set_shading_mode(godot::BaseMaterial3D::SHADING_MODE_UNSHADED);line_material->set_albedo({1,0.8f,0.1f,1});
    lines->surface_begin(godot::Mesh::PRIMITIVE_LINES,line_material);
    for(int corner=0;corner<8;++corner)for(int axis=0;axis<3;++axis)if(!(corner&(1<<axis))) {
        auto a=impl_->bounds.position,b=a;
        for(int k=0;k<3;++k){a[k]+=(corner&(1<<k))?impl_->bounds.size[k]:0;b[k]=a[k];}
        b[axis]+=impl_->bounds.size[axis];lines->surface_add_vertex(a);lines->surface_add_vertex(b);
    }
    lines->surface_end();impl_->bounds_node=memnew(godot::MeshInstance3D);add_child(impl_->bounds_node);
    impl_->bounds_node->set_mesh(lines);impl_->bounds_node->set_visible(impl_->show_bounds);set_wireframe(impl_->wireframe);
    impl_->display_ready=true;
}
void Ps2VehicleRenderer::layout_lods(){
    if(!impl_->model||impl_->nodes.empty())return;
    auto& model=*impl_->model;bool first=true;impl_->labels.clear();model.select(120,0);
    const float spacing=part_bounds(impl_->nodes.front()).size.x+1.f;
    std::array<bool,4> present{};for(const auto& part:model.parts())if(part.group==port::vehicle::Part::Group::Body)present[part.lod]=true;
    for(std::size_t i=0;i<impl_->nodes.size();++i){const auto& part=model.parts()[i];auto* node=impl_->nodes[i];auto transform=host_matrix(part.transform);
        const bool body=part.group==port::vehicle::Part::Group::Body;
        const bool visible=impl_->show_lods&&body?part.enabled:part.selected;
        if(impl_->show_lods&&body&&part.lod>0)transform.origin.x+=part.lod*spacing;
        node->set_transform(transform);node->set_visible(visible);
        if(visible){const auto bounds=transform.xform(part_bounds(node));impl_->bounds=first?bounds:impl_->bounds.merge(bounds);first=false;}
    }
    // Port-side copies use the original per-LOD attachment gates and transforms.
    if(impl_->show_lods&&impl_->compare_nodes.empty())for(int lod=1;lod<4;++lod)if(present[lod]){
        model.select(120,lod);
        for(std::size_t i=0;i<model.parts().size();++i){const auto& part=model.parts()[i];
            if(!part.selected||part.group==port::vehicle::Part::Group::Body)continue;
            auto* node=memnew(godot::MeshInstance3D);add_child(node);node->set_mesh(impl_->nodes[i]->get_mesh());
            std::vector<godot::MeshInstance3D*> batches{node};
            for(int child=0;child<impl_->nodes[i]->get_child_count();++child){
                auto* original=godot::Object::cast_to<godot::MeshInstance3D>(impl_->nodes[i]->get_child(child));
                if(!original)continue;
                auto* chunk=memnew(godot::MeshInstance3D);node->add_child(chunk);chunk->set_mesh(original->get_mesh());
                chunk->set_name(original->get_name());chunk->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);batches.push_back(chunk);
            }
            const auto count=impl_->draws.size();
            for(std::size_t j=0;j<count;++j)if(impl_->draws[j].part==i&&
                (impl_->draws[j].node==impl_->nodes[i]||impl_->nodes[i]->is_ancestor_of(impl_->draws[j].node))){
                auto draw=impl_->draws[j];const auto original_material=draw.material;draw.material=draw.material->duplicate();
                if(impl_->vehicle_depth_staging){
                    const auto source_parameters=draw.depth_parameters;
                    draw.depth_parameters=vehicle_depth_staging::create(draw.material);
                    // duplicate() copied those exact source parameter values. Keep
                    // independent storage; subsequent writes cannot alias another LOD.
                    if(source_parameters&&draw.depth_parameters&&source_parameters->shader==draw.depth_parameters->shader){
                        draw.depth_parameters->values=source_parameters->values;draw.depth_parameters->valid=source_parameters->valid;
                    }
                }
                draw.material->set_meta("hp2_depth_dynamic_uniforms",original_material->get_meta("hp2_depth_dynamic_uniforms"));draw.material->set_meta("hp2_depth_uniform_contract",1);
                if(draw.reflection.is_valid()){draw.reflection=draw.reflection->duplicate();draw.material->set_next_pass(draw.reflection);}
                draw.node=batches.at(draw.batch);draw.node->set_surface_override_material(int(draw.batch_surface),draw.material);impl_->draws.push_back(std::move(draw));
            }
            auto transform=host_matrix(part.transform);transform.origin.x+=lod*spacing;node->set_transform(transform);node->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
            impl_->compare_nodes.push_back(node);impl_->node_lods.push_back(lod);impl_->node_parts.push_back(i);
        }
    }
    for(std::size_t i=0;i<impl_->compare_nodes.size();++i){auto* node=impl_->compare_nodes[i];node->set_visible(model.parts()[impl_->node_parts[i]].enabled);if(node->is_visible())impl_->bounds=impl_->bounds.merge(node->get_transform().xform(part_bounds(node)));}
    for(int lod=0;lod<(impl_->show_lods?4:1);++lod){godot::Dictionary label;label["lod"]=lod;label["available"]=present[lod];int triangles=0;
        if(present[lod]){model.select(120,lod);for(const auto& part:model.parts())if(part.selected)for(const auto& surface:model.geometry().solids[part.solid].surfaces)if(!(surface.render_flags&0x400))triangles+=int(surface.indices.size()/3);}
        label["triangles"]=triangles;impl_->labels.push_back(label);
    }
    model.select(120,0);
    if(impl_->show_lods&&impl_->tags.empty())for(int lod=0;lod<4;++lod){
        const godot::Dictionary label=impl_->labels[lod];auto* tag=memnew(godot::Label3D);add_child(tag);
        tag->set_text(string("LOD "+std::to_string(lod)+(present[lod]?" / "+std::to_string(int(label["triangles"]))+" triangles":" / missing")));
        tag->set_font_size(40);tag->set_pixel_size(.006f);tag->set_billboard_mode(godot::BaseMaterial3D::BILLBOARD_ENABLED);
        tag->set_position({lod*spacing,(impl_->bounds.position.y+impl_->bounds.size.y)+.4f,0});impl_->tags.push_back(tag);
    }
    for(auto* tag:impl_->tags)tag->show();
}
void Ps2VehicleRenderer::cancel_load(){
    if(impl_->ticket){*impl_->ticket->cancelled=true;impl_->ticket.reset();}
    if(impl_->staging){auto current=std::move(impl_);impl_=std::move(current->staging);release_display();impl_=std::move(current);}
    impl_->loading=false;++impl_->generation;
}
godot::Dictionary Ps2VehicleRenderer::request_car(const godot::String& path,int car,int skin){return request_native(path,car,skin,0);}
// E-0098-RB01: explicit per-instance material binding of original reflection flags.
// Discard substitutes a skipped strip submission only; shared depth/stage executor
// and full Debug texture-context equivalence remain separate requirements.
void Ps2VehicleRenderer::apply_frontend_flags(std::uint32_t flags){
    const bool changed=impl_->frontend_flags!=flags;
    impl_->frontend_flags=flags;
    if(changed)impl_->capture_binding_valid=false;
    for(auto& draw:impl_->draws){
        if(!changed && draw.frontend_flags_bound)continue;
        draw.material->set_shader_parameter("frontend_skip_strip",
            !port::selection::frontend_strip_enabled(flags,draw.apply_alpha_sorting));
        const bool envmap=!(flags&1u) && (!impl_->capture_requested || (impl_->capture_ready && !draw.flag_0x4));
        const godot::Ref<godot::Material> next=envmap?godot::Ref<godot::Material>(draw.reflection):godot::Ref<godot::Material>();
        if(draw.material->get_next_pass()!=next)draw.material->set_next_pass(next);
        draw.frontend_flags_bound=true;
    }
}
bool Ps2VehicleRenderer::apply_frontend_pose(const hp2::debug::FrontEndRenderingCar& state,
    const std::array<float,16>& scale,const hp2::debug::SelectCarLights& lights,
    const std::array<float,16>& view,const port::driving::VehicleLightState& texture_lights){
    std::string gap;
    const auto error=catch_all([&]{gap=hp2::guard([&]{
        if(!impl_->model || !impl_->display_ready || impl_->loading)
            throw std::logic_error("Frontend pose requires a completed original model request");
        if(state.LightsOn || state.CopLightsOn)
            hp2::not_implemented("0x00123EA8","selector forced flare-state display is not bound");
        if(bool(state.OverrideModel)!=impl_->model->is_frontend_solid())
            throw std::logic_error("Frontend override requires its actual original named-solid renderer");
        if(state.OverrideModel && impl_->model->geometry().solids.front().hash!=state.OverrideModel->NameHash)
            throw std::logic_error("Frontend named solid does not match original override hash");
        const port::render::VehicleSolid* body=nullptr;
        for(const auto& part:impl_->model->parts())if(part.group==port::vehicle::Part::Group::Body && part.lod==0){
            body=&impl_->model->geometry().solids.at(part.solid);break;}
        if(!body)hp2::not_implemented("0x00123EA8","original frontend Body0 bounds unavailable");
        const auto transform=host_matrix(port::selection::frontend_render_matrix(state,scale,body->bounds_min,body->bounds_max));
        if(!impl_->model->is_frontend_solid())impl_->model->apply_frontend_pose(state.TireMatrices,state.BrakeMatrices);
        if(get_transform()!=transform)set_transform(transform);
        set_visible(state.Visible!=0);impl_->lights=texture_lights;
        const auto same_light=[](const hp2::debug::SelectCarLight& a,const hp2::debug::SelectCarLight& b){return a.Position==b.Position && a.Colour==b.Colour;};
        const auto* previous=impl_->frontend_lights?&*impl_->frontend_lights:nullptr;
        if(!previous || impl_->frontend_view!=view || !same_light(previous->Light0,lights.Light0)
            || !same_light(previous->Light1,lights.Light1) || !same_light(previous->Light2,lights.Light2)
            || !same_light(previous->LightA,lights.LightA)){
            impl_->frontend_lights=lights;impl_->frontend_view=view;++impl_->frontend_light_revision;
        }
        apply_frontend_flags(port::selection::frontend_render_flags(state));
        layout_lods();
    });});
    impl_->error=!error.empty()?error:gap;return impl_->error.empty();
}
godot::Dictionary Ps2VehicleRenderer::request_frontend_solid(std::int64_t hash){
    if(hash!=0xd8173744ll && hash!=0xb077e934ll){
        impl_->error="Unreviewed frontend named solid requested";return get_state();
    }
    return request_native("FRONTEND/FRONTB.BUN",-1,-1,static_cast<std::uint32_t>(hash));
}
godot::Dictionary Ps2VehicleRenderer::request_native(const godot::String& path,int car,int skin,std::uint32_t frontend_solid){
    const auto error=catch_all([&]{
        if(!requests_enabled())throw std::logic_error("CarViewer is disabled in Release builds");
        cancel_load();auto input=assets();if(impl_->gpu->root!=input.root()){impl_->gpu=std::make_shared<GpuCache>();impl_->gpu->root=input.root();}
        impl_->requested_car=car;impl_->requested_skin=skin;impl_->requested_frontend_solid=frontend_solid;
        impl_->requested_path=path.utf8().get_data();impl_->error.clear();impl_->loading=true;impl_->upload_ms=impl_->max_upload_slice_ms=0;impl_->previews.clear();
        if(frontend_solid){impl_->show_lods=false;impl_->wheel_blur=false;}
        impl_->ticket=loader().request(input.root(),path.utf8().get_data(),car,skin,impl_->show_lods,impl_->wheel_blur,frontend_solid);
    });
    if(!error.empty()){release_display();impl_->model.reset();impl_->error=error;impl_->loading=false;impl_->completed=impl_->generation;}return get_state();
}
godot::Dictionary Ps2VehicleRenderer::set_show_lods(bool enabled){
    if(impl_->requested_frontend_solid && (impl_->loading || (impl_->model && impl_->model->is_frontend_solid()))){
        if(enabled)impl_->error="A named frontend solid has no vehicle LOD viewer";
        return get_state();
    }
    impl_->show_lods=enabled;
    if(impl_->loading)return request_car(string(impl_->requested_path),impl_->requested_car,impl_->requested_skin);
    if(impl_->model)return request_car(string(impl_->model->path()),impl_->model->car_id(),impl_->model->skin_id());
    return get_state();
}
godot::Dictionary Ps2VehicleRenderer::set_wheel_blur(bool enabled){
    const auto error=catch_all([&]{if(impl_->model)impl_->model->set_wheel_blur(enabled);impl_->wheel_blur=enabled;});
    if(!error.empty())impl_->error=error;return get_state();
}
godot::Dictionary Ps2VehicleRenderer::set_wheel_speed(double speed){
    const auto error=catch_all([&]{
        if(!std::isfinite(speed)||std::abs(speed)>150)throw std::invalid_argument("Preview wheel speed must be within -150..150 rad/s");
        if(impl_->model)impl_->model->set_wheel_speed(speed);impl_->wheel_speed=speed;
    });
    if(!error.empty())impl_->error=error;return get_state();
}
void Ps2VehicleRenderer::advance_wheels(double delta){
    const auto error=catch_all([&]{if(impl_->driving_pose)throw std::logic_error("Preview integration is disabled for driving snapshots");if(impl_->model)impl_->model->advance_wheels(delta);});
    if(!error.empty())impl_->error=error;
}

godot::Dictionary Ps2VehicleRenderer::pump_load(){
    const auto error=catch_all([&]{
        if(impl_->ticket&&impl_->ticket->result.wait_for(std::chrono::seconds(0))==std::future_status::ready){
            auto result=impl_->ticket->result.get();impl_->ticket.reset();
            impl_->cpu_ms=result.cpu_ms;impl_->read_ms=result.read_ms;impl_->geometry_decode_ms=result.geometry_decode_ms;impl_->texture_decode_ms=result.texture_decode_ms;impl_->binding_ms=result.binding_ms;impl_->file_reads=result.file_reads;impl_->geometry_reads=result.geometry_reads;impl_->geometry_hits=result.geometry_hits;impl_->texture_decodes=result.texture_decodes;impl_->texture_hits=result.texture_hits;
            impl_->previews=std::move(result.previews);impl_->preview_errors=std::move(result.preview_errors);
            if(!result.error.empty()||!result.model){release_display();impl_->model.reset();impl_->error=result.error.empty()?"Vehicle request cancelled":result.error;impl_->loading=false;impl_->completed=impl_->generation;return;}
            auto stage=std::make_unique<Impl>();stage->vehicle_depth_staging=impl_->vehicle_depth_staging;stage->model=std::move(result.model);stage->gpu=impl_->gpu;stage->show_lods=impl_->show_lods;stage->wheel_blur=impl_->wheel_blur;stage->inspection=impl_->inspection;stage->wireframe=impl_->wireframe;stage->show_bounds=impl_->show_bounds;stage->upload_start=std::chrono::steady_clock::now();impl_->staging=std::move(stage);
        }
        if(impl_->staging){
            auto current=std::move(impl_);impl_=std::move(current->staging);const auto start=std::chrono::steady_clock::now();
            const auto failure=catch_all([&]{rebuild_display(true);});
            const double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();current->max_upload_slice_ms=std::max(current->max_upload_slice_ms,elapsed);
            for(auto* tag:impl_->tags)tag->hide();for(auto* node:impl_->nodes)node->hide();for(auto* node:impl_->compare_nodes)node->hide();if(impl_->bounds_node)impl_->bounds_node->hide();
            auto stage=std::move(impl_);impl_=std::move(current);
            if(!failure.empty()){current=std::move(impl_);impl_=std::move(stage);release_display();impl_=std::move(current);release_display();impl_->model.reset();impl_->error=failure;impl_->loading=false;impl_->completed=impl_->generation;return;}
            if(stage->display_ready){
                impl_->upload_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-stage->upload_start).count();
                if(impl_->model&&impl_->model->path()==stage->model->path()&&impl_->nodes.size()==stage->nodes.size()){
                    for(std::size_t i=0;i<stage->nodes.size();++i){auto* old=impl_->nodes[i];auto* replacement=stage->nodes[i];
                        if(!old||old->get_child_count()||replacement->get_child_count()||old->get_mesh()!=replacement->get_mesh())continue;
                        for(int j=0;j<replacement->get_mesh()->get_surface_count();++j)old->set_surface_override_material(j,replacement->get_surface_override_material(j));
                        for(auto& draw:stage->draws)if(draw.node==replacement)draw.node=old;
                        remove_child(replacement);memdelete(replacement);stage->nodes[i]=old;impl_->nodes[i]=nullptr;
                    }
                    impl_->nodes.erase(std::remove(impl_->nodes.begin(),impl_->nodes.end(),nullptr),impl_->nodes.end());
                }
                release_display();
                impl_->model=std::move(stage->model);impl_->nodes=std::move(stage->nodes);impl_->draws=std::move(stage->draws);impl_->bounds_node=stage->bounds_node;stage->bounds_node=nullptr;impl_->bounds=stage->bounds;impl_->wire=stage->wire;
                impl_->uploaded=std::move(stage->uploaded); // Active light variants retain uploaded references.
                impl_->compare_nodes=std::move(stage->compare_nodes);impl_->node_lods=std::move(stage->node_lods);impl_->node_parts=std::move(stage->node_parts);impl_->labels=stage->labels;impl_->tags=std::move(stage->tags);
                impl_->model->set_wheel_speed(impl_->wheel_speed);impl_->model->set_wheel_blur(impl_->wheel_blur);impl_->display_ready=true;impl_->loading=false;impl_->completed=impl_->generation;layout_lods();set_bounds(impl_->show_bounds);set_wireframe(impl_->wireframe);
            }else impl_->staging=std::move(stage);
        }
    });
    if(!error.empty()){cancel_load();release_display();impl_->model.reset();impl_->error=error;impl_->completed=impl_->generation;}return get_state();
}

godot::Dictionary Ps2VehicleRenderer::update_view(const godot::Vector3& position,const godot::Vector3& forward,double projection_scale,int forced_lod) {
    (void)forced_lod;
    if(impl_->model&&!impl_->display_ready)return get_state();
    const auto error=catch_all([&] {
        if(!impl_->model)return;
        impl_->projected_size=port::GetPixelSize(3.0f,float(projection_scale),source(get_global_position()),source(position),source(forward));
        const auto selection=impl_->model->select(impl_->projected_size,impl_->show_lods?0:0);impl_->lod=selection.body;impl_->triangles=0;
        layout_lods();
        // Explicit race capture carries original eEnvMap face2 HackMan matrix.
        // Standalone viewer retains its historical frontend-only orientation.
        const auto environment=impl_->capture_requested?impl_->capture_hack:port::eCreateLookAtMatrix(source(position),source(get_global_position()),{{0,0,1}});
        for(auto& draw:impl_->draws) {
            const auto& part=impl_->model->parts()[draw.part];if(!draw.node||!draw.node->is_visible_in_tree())continue;
            const auto light_texture=impl_->model->driving_texture(part.solid,draw.surface,impl_->lights);
            if(draw.bound_texture!=light_texture){
                const auto& texture=impl_->uploaded.at(light_texture);
                draw.material->set_shader_parameter("base_texture",texture.image);
                const auto& surface=impl_->model->geometry().solids[part.solid].surfaces[draw.surface];
                draw.flag_0x4=bool(surface.render_flags&4)||texture.data->header.Field_0x74!=0;
                draw.material->set_shader_parameter("flag_0x4",draw.flag_0x4);
                draw.maximum_texture_alpha=texture.max_alpha;draw.bound_texture=light_texture;
                draw.apply_alpha_sorting=texture.data->header.Field_0x4d!=0;
                draw.material->set_shader_parameter("frontend_skip_strip",
                    !port::selection::frontend_strip_enabled(impl_->frontend_flags,draw.apply_alpha_sorting));
            }
            const auto world=draw.node->get_global_transform();
            const auto camera=source(world.affine_inverse().xform(position));
            const godot::Vector3 source_camera_value(camera[0],camera[1],camera[2]);
            draw.material->set_shader_parameter("source_camera",source_camera_value);
            if(draw.depth_parameters)vehicle_depth_staging::publish(draw.depth_parameters,0,source_camera_value);
            if(draw.reflection.is_valid()&&(!impl_->capture_requested||impl_->capture_ready)){
                draw.reflection->set_shader_parameter("source_camera",godot::Vector3(camera[0],camera[1],camera[2]));
                const auto part_matrix=source_matrix(world);
                for(int row=0;row<3;++row){
                    std::array<float,3> v{};for(int axis=0;axis<3;++axis)v[axis]=part_matrix[row*4]*environment[axis]+part_matrix[row*4+1]*environment[4+axis]+part_matrix[row*4+2]*environment[8+axis];
                    draw.reflection->set_shader_parameter(string("environment_row"+std::to_string(row)),godot::Vector3(v[0],v[1],v[2]));
                }
            }
            // Port-side world placement adapter for the recovered static light slice.
            if(!draw.lighting_valid||draw.lighting_basis!=world.basis||draw.lights[15]!=part.lights[15]
                || draw.frontend_light_revision!=impl_->frontend_light_revision){
                draw.lights=impl_->frontend_lights?hp2::debug::elSetupLightContext(*impl_->frontend_lights,impl_->frontend_view,source_matrix(world))
                    :hp2::recovered::CarRenderInfo::Sub_00120EF8(source_matrix(world));
                draw.lights[15]=part.lights[15];
                const std::string suffix[]={"35c","35d","35e","35f","360","361","362"};
                for(int i=0;i<7;++i){
                    const godot::Vector4 value(draw.lights[i*4],draw.lights[i*4+1],draw.lights[i*4+2],draw.lights[i*4+3]);
                    draw.material->set_shader_parameter(string("light_"+suffix[i]),value);
                    if(draw.depth_parameters)vehicle_depth_staging::publish(draw.depth_parameters,std::size_t(i)+1,value);
                }
                if(draw.reflection.is_valid())for(int i=0;i<7;++i)draw.reflection->set_shader_parameter(string("light_"+suffix[i]),godot::Vector4(draw.lights[i*4],draw.lights[i*4+1],draw.lights[i*4+2],draw.lights[i*4+3]));
                draw.lighting_basis=world.basis;draw.lighting_valid=true;draw.frontend_light_revision=impl_->frontend_light_revision;
            }
            // Reject the unsupported negative-destination GL blend range explicitly.
            // Convex texture/vertex interpolation is bounded by these maxima.
            if(!impl_->inspection&&draw.source_over&&!draw.bounded_alpha){
                const auto& surface=impl_->model->geometry().solids[part.solid].surfaces[draw.surface];
                const auto& blocks=impl_->model->materials().at(surface.material_hash).blocks;
                port::vehicle::VuMaterialBlock material{};port::vehicle::VuLightBlock lights{};
                std::memcpy(material.data(),blocks.data(),sizeof(material));std::memcpy(lights.data(),draw.lights.data(),sizeof(lights));
                unsigned max_alpha=0;
                for(const auto& vertex:surface.vertices){
                    unsigned alpha=(vertex.raw_color&0x8000)?128:0;
                    if(surface.render_flags&0x80){const auto reflected=port::vehicle::reflection_002A7F90(vertex.position,camera,vertex.normal,port::vehicle::reflection_coefficients_0010AC50());alpha=port::vehicle::base_rgba_002A8308(vertex.normal,reflected,vertex.raw_color,material,lights,draw.flag_0x4)[3];}
                    max_alpha=std::max(max_alpha,alpha);
                }
                if(max_alpha*draw.maximum_texture_alpha>=129*128)
                    hp2::not_implemented("0x00108CC0","GS source-over alpha above128 needs a destination-read renderer; select explicit unlit inspection to inspect geometry");
            }
        }
        for(std::size_t i=0;i<impl_->nodes.size();++i){
            const auto& part=impl_->model->parts()[i];
            if(impl_->nodes[i]->is_visible())for(const auto& surface:impl_->model->geometry().solids[part.solid].surfaces)if(!(surface.render_flags&0x400))impl_->triangles+=int(surface.indices.size()/3);
        }
        for(std::size_t i=0;i<impl_->compare_nodes.size();++i)if(impl_->compare_nodes[i]->is_visible())for(const auto& surface:impl_->model->geometry().solids[impl_->model->parts()[impl_->node_parts[i]].solid].surfaces)if(!(surface.render_flags&0x400))impl_->triangles+=int(surface.indices.size()/3);
        impl_->flare_billboards.clear();impl_->flare_error.clear();
        if(impl_->driving_pose&&impl_->flare_state_available){
            impl_->flare_error=catch_all([&]{
                auto* camera=get_viewport()?get_viewport()->get_camera_3d():nullptr;
                if(!camera)throw std::invalid_argument("Vehicle light flares require the actual view camera");
                if(!impl_->flare_resources){const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");auto image=port::driving::read_debug_image(port::driving::debug_image_path(start.utf8().get_data()));impl_->flare_resources=port::driving::read_vehicle_flare_resources(image);}
                std::vector<port::driving::VehicleFlareMarker> markers;
                for(const auto& raw:impl_->model->position_markers()){const auto marker=port::driving::create_vehicle_flare_marker(*impl_->flare_resources,raw.name_hash,raw.position,std::uint32_t(raw.param),impl_->model->marker_pivot());if(marker)markers.push_back(*marker);}
                if(markers.empty())hp2::not_implemented("0x00120558","Original vehicle has no recognized light flare position markers");
                port::driving::VehicleFlareFrame frame;frame.camera_position=source(camera->get_global_position());
                const auto basis=camera->get_global_basis();const auto right=source(basis.get_column(0)),up=source(basis.get_column(1)),direction=source(-basis.get_column(2));
                for(unsigned axis=0;axis<3;++axis){frame.world_view[axis*4]=right[axis];frame.world_view[axis*4+1]=up[axis];frame.world_view[axis*4+2]=direction[axis];frame.world_view[12]-=right[axis]*frame.camera_position[axis];frame.world_view[13]-=up[axis]*frame.camera_position[axis];frame.world_view[14]-=direction[axis]*frame.camera_position[axis];}frame.world_view[15]=1;frame.camera_matrix=frame.world_view;
                const auto projection=camera->get_camera_projection();const auto clip=projection*godot::Projection(camera->get_global_transform().affine_inverse());constexpr unsigned columns[]{2,0,1,3};for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)frame.world_clip[c*4+r]=float(clip.columns[columns[c]][r]);frame.h=float(projection.columns[0].x)*256.f;
                impl_->flare_billboards=port::driving::vehicle_flare_billboards(*impl_->flare_resources,markers,impl_->flare_state,source_matrix(get_global_transform()),frame);
                if(!impl_->flare_renderer)impl_->flare_renderer=std::make_unique<ParticleBillboardRenderer>(*this,[this](auto hash){return get_visual_texture(hash);});
                impl_->flare_renderer->update(impl_->flare_billboards,impl_->model->textures(),camera->get_global_position(),-basis.get_column(2));
            });
        }
        if((!impl_->flare_error.empty()||!impl_->driving_pose||!impl_->flare_state_available)&&impl_->flare_renderer)impl_->flare_renderer->clear();
    });
    if(!error.empty()){if(impl_->flare_renderer)impl_->flare_renderer->clear();for(auto* node:impl_->nodes)node->set_visible(false);for(auto* node:impl_->compare_nodes)node->set_visible(false);for(auto* tag:impl_->tags)tag->hide();}
    if(!error.empty()||!impl_->loading)impl_->error=error;return get_state();
}
void Ps2VehicleRenderer::set_environment_capture(const godot::Dictionary& capture){
 impl_->capture_requested=true;impl_->capture_enabled=bool(capture.get("enabled",false));impl_->capture_ready=false;impl_->capture_diagnostic=godot::String(capture.get("diagnostic","")).utf8().get_data();
 godot::Ref<godot::Texture2D> next_texture;
 const auto error=catch_all([&]{
 if(impl_->capture_enabled&&bool(capture.get("available",false))){
 const godot::Ref<godot::Texture2D> texture=capture.get("texture",godot::Variant());const godot::PackedFloat32Array matrix=capture.get("hack_matrix_native",godot::PackedFloat32Array());
 if(texture.is_null()||matrix.size()!=16)throw std::invalid_argument("Original environment capture requires texture and16-lane HackMan matrix");
 for(int i=0;i<16;++i)if(!std::isfinite(matrix[i]))throw std::invalid_argument("Nonfinite native environment capture matrix");
 for(int i=0;i<16;++i)impl_->capture_hack[i]=matrix[i];
 next_texture=texture;impl_->capture_ready=true;
 }
 });
 if(!error.empty())impl_->capture_diagnostic=error;
 impl_->capture_texture=next_texture;
 const std::uint64_t texture_id=next_texture.is_valid()?next_texture->get_instance_id():0;
 const bool changed=!impl_->capture_binding_valid||impl_->capture_bound_ready!=impl_->capture_ready||impl_->capture_bound_enabled!=impl_->capture_enabled||impl_->capture_bound_texture!=texture_id||impl_->capture_bound_generation!=impl_->generation||impl_->capture_bound_draw_count!=impl_->draws.size();
 if(!changed){if(!impl_->capture_binding_error.empty())impl_->capture_diagnostic=impl_->capture_binding_error;return;}
 impl_->capture_binding_error.clear();
 if(!(impl_->frontend_flags&1u)&&impl_->capture_ready&&std::any_of(impl_->draws.begin(),impl_->draws.end(),[](const auto& draw){return draw.reflection.is_valid()&&draw.flag_0x4;}))
     impl_->capture_binding_error=catch_all([&]{port::track::require_environment_source_alpha(true,0);});
 for(auto& draw:impl_->draws)if(draw.reflection.is_valid()){
 const bool supported=!(impl_->frontend_flags&1u)&&impl_->capture_ready&&!draw.flag_0x4;
 const godot::Ref<godot::Material> next=supported?godot::Ref<godot::Material>(draw.reflection):godot::Ref<godot::Material>();
 if(draw.material->get_next_pass()!=next)draw.material->set_next_pass(next);
 if(supported&&(!impl_->capture_binding_valid||impl_->capture_bound_texture!=texture_id||impl_->capture_bound_generation!=impl_->generation||impl_->capture_bound_draw_count!=impl_->draws.size()))draw.reflection->set_shader_parameter("base_texture",impl_->capture_texture);
 }
 impl_->capture_binding_valid=true;impl_->capture_bound_ready=impl_->capture_ready;impl_->capture_bound_enabled=impl_->capture_enabled;impl_->capture_bound_texture=texture_id;impl_->capture_bound_generation=impl_->generation;impl_->capture_bound_draw_count=impl_->draws.size();
 if(!impl_->capture_binding_error.empty())impl_->capture_diagnostic=impl_->capture_binding_error;
}
godot::Ref<godot::Texture2D> Ps2VehicleRenderer::get_visual_texture(std::int64_t hash)const {
 const auto found=impl_->uploaded.find(std::uint32_t(hash));return found==impl_->uploaded.end()?godot::Ref<godot::Texture2D>():godot::Ref<godot::Texture2D>(found->second.image);
}
godot::Dictionary Ps2VehicleRenderer::get_visual_state()const {
 godot::Dictionary out;out["available"]=bool(impl_->model&&impl_->display_ready&&!impl_->loading);if(!impl_->model||!impl_->display_ready||impl_->loading)return out;
 out["flare_diagnostic"]=string(impl_->flare_error);out["flare_state_available"]=impl_->flare_state_available;
 if(impl_->flare_renderer)out["flare_stats"]=impl_->flare_renderer->stats();
 godot::Array flare_rows;for(const auto& flare:impl_->flare_billboards){godot::Dictionary row;row["texture_hash"]=int64_t(flare.texture_hash);row["name_hash"]=int64_t(flare.name_hash);row["parameter_index"]=int64_t(flare.parameter_index);row["position_native"]=godot::Vector3(flare.centre[0],flare.centre[1],flare.centre[2]);row["half_size"]=flare.half_size;row["intensity"]=flare.intensity;row["colour"]=godot::Vector4(flare.colour[0],flare.colour[1],flare.colour[2],flare.colour[3]);flare_rows.push_back(row);}out["light_flares"]=flare_rows;
 out["headlight_left"]=impl_->lights.headlight_left;out["headlight_right"]=impl_->lights.headlight_right;out["brakelights"]=impl_->lights.brakelights;
 godot::Array light_bindings;for(const auto& draw:impl_->draws){const auto& part=impl_->model->parts()[draw.part];const auto off=impl_->model->geometry().solids[part.solid].surfaces[draw.surface].texture_hash;if(draw.bound_texture==off)continue;godot::Dictionary item;item["part"]=int64_t(draw.part);item["surface"]=int64_t(draw.surface);item["texture_hash"]=int64_t(draw.bound_texture);light_bindings.push_back(item);}out["active_light_textures"]=light_bindings;
 auto native=source(get_global_position());const auto offset=port::track::environment_eye_offset();auto environment=native;for(unsigned i=0;i<3;++i)environment[i]+=offset[i];
 out["geometry_position_native"]=godot::Vector3(native[0],native[1],native[2]);out["environment_position_native"]=godot::Vector3(environment[0],environment[1],environment[2]);out["geometry_transform"]=get_global_transform();
 godot::PackedFloat32Array matrix;for(float v:source_matrix(get_global_transform()))matrix.push_back(v);out["geometry_matrix_native"]=matrix;
 godot::PackedVector3Array cv;for(const auto& v:impl_->model->collision_vertices())cv.push_back(godot::Vector3(v[0],v[1],v[2]));out["collision_vertices_native"]=cv;out["collision_geometry_space"]=true;
 godot::PackedVector3Array wheels;for(const auto& w:impl_->model->render_config().Field_0x120_0x180)wheels.push_back(godot::Vector3(w[0],w[1],w[2]));out["wheel_centers_native"]=wheels;
 for(const auto& part:impl_->model->parts())if(part.group==port::vehicle::Part::Group::Body&&part.lod==0){const auto& solid=impl_->model->geometry().solids.at(part.solid);out["bounds_min_native"]=godot::Vector3(solid.bounds_min[0],solid.bounds_min[1],solid.bounds_min[2]);out["bounds_max_native"]=godot::Vector3(solid.bounds_max[0],solid.bounds_max[1],solid.bounds_max[2]);break;}
 godot::Array shadow_textures;for(const auto& item:impl_->uploaded){const auto& texture=item.second;if(texture.data->header.tex_hash!=0xbadb4475&&texture.data->header.tex_hash!=hp2::frontend::bStringHash((impl_->model->render_config().prefix+"_SHADOWIG").c_str()))continue;const auto& h=texture.data->header;godot::Dictionary t;t["hash"]=int64_t(item.first);t["texture"]=godot::Ref<godot::Texture2D>(texture.image);t["width"]=h.width;t["height"]=h.height;t["alpha_bits"]=h.alpha_bits;t["alpha_fix"]=h.alpha_fix;t["texture_fx"]=h.texture_fx;t["repeat"]=h.Field_0x4c;t["sorting"]=h.Field_0x4d;t["write_depth"]=h.Field_0x50;t["mxl"]=h.Field_0x6e;t["mmag"]=h.Field_0x6f;t["mmin"]=h.Field_0x70;t["lod_l"]=h.Field_0x71;t["lod_k"]=h.Field_0x72;t["gloss"]=h.Field_0x74;t["semi"]=h.is_any_semitransparency;t["maximum_alpha"]=texture.max_alpha;shadow_textures.push_back(t);}out["shadow_textures"]=shadow_textures;
 out["shadow_ramp_hash"]=int64_t(0xbadb4475);out["shadow_ig_hash"]=int64_t(hp2::frontend::bStringHash((impl_->model->render_config().prefix+"_SHADOWIG").c_str()));out["lod"]=impl_->lod;out["projected_size"]=impl_->projected_size;out["lod_swap1"]=45.f;out["lod_swap2"]=20.f;out["driving_pose"]=impl_->driving_pose;return out;
}
godot::Dictionary Ps2VehicleRenderer::get_state()const {
    godot::Dictionary result;
    const auto error=catch_all([&] {
        result["phase"]=impl_->loading?(impl_->staging?"Uploading":"Reading / decoding"):"Idle";
        result["target_path"]=string(impl_->requested_path);
        result["loading"]=impl_->loading;result["generation"]=int64_t(impl_->generation);result["completed"]=int64_t(impl_->completed);result["show_lods"]=impl_->show_lods;result["wheel_blur"]=impl_->wheel_blur;result["wheel_speed"]=impl_->wheel_speed;result["lods"]=impl_->labels;
        result["cpu_ms"]=impl_->cpu_ms;result["read_ms"]=impl_->read_ms;result["geometry_decode_ms"]=impl_->geometry_decode_ms;result["texture_decode_ms"]=impl_->texture_decode_ms;result["binding_ms"]=impl_->binding_ms;result["file_reads"]=int64_t(impl_->file_reads);result["upload_ms"]=impl_->upload_ms;result["max_upload_slice_ms"]=impl_->max_upload_slice_ms;
        result["geometry_reads"]=int64_t(impl_->geometry_reads);result["geometry_hits"]=int64_t(impl_->geometry_hits);result["texture_decodes"]=int64_t(impl_->texture_decodes);result["texture_hits"]=int64_t(impl_->texture_hits);result["gpu_hits"]=int64_t(impl_->gpu->hits);
        godot::Dictionary previews;for(const auto& entry:impl_->previews)previews[entry.first]=godot::Color(entry.second[0],entry.second[1],entry.second[2],1);result["previews"]=previews;godot::Dictionary preview_errors;for(const auto& entry:impl_->preview_errors)preview_errors[entry.first]=string(entry.second);result["preview_errors"]=preview_errors;
        result["ok"]=impl_->model!=nullptr&&impl_->display_ready&&impl_->error.empty();result["error"]=string(impl_->error);
        result["bounds"]=impl_->bounds;result["lod"]=impl_->lod;result["triangles"]=impl_->triangles;
        result["wheel_phase"]=impl_->model?impl_->model->wheel_phase():0.0;result["projected_size"]=impl_->projected_size;result["inspection"]=impl_->inspection;
        result["draw_surfaces"]=int(impl_->draws.size());
        result["environment_capture_requested"]=impl_->capture_requested;result["environment_capture_ready"]=impl_->capture_ready;result["environment_capture_enabled"]=impl_->capture_enabled;result["environment_capture_diagnostic"]=string(impl_->capture_diagnostic);
        result["reflection_surfaces"]=int(std::count_if(impl_->draws.begin(),impl_->draws.end(),[](const auto& draw){return draw.reflection.is_valid();}));
        result["car_id"]=impl_->requested_car;result["skin_id"]=impl_->requested_skin;
        result["rendering"]=impl_->inspection?"Unlit inspection (debug only)":"Original VU base material / GS static state (partial)";
        godot::Array parts;
        if(impl_->model){
            for(std::size_t i=0;i<impl_->model->parts().size();++i){const auto& part=impl_->model->parts()[i];godot::Dictionary p;p["name"]=string(part.name);p["visible"]=i<impl_->nodes.size()&&impl_->nodes[i]->is_visible();p["enabled"]=part.enabled;
                const auto& surfaces=impl_->model->geometry().solids[part.solid].surfaces;
                p["draw_surfaces"]=int(std::count_if(surfaces.begin(),surfaces.end(),[](const auto& surface){return !(surface.render_flags&0x400)&&!surface.indices.empty();}));
                parts.push_back(p);}
            result["car_id"]=impl_->model->car_id();result["skin_id"]=impl_->model->skin_id();result["path"]=string(impl_->model->path());
            result["textures"]=int(impl_->model->textures().size());
            std::ostringstream text;text<<"parts="<<parts.size()<<" triangles="<<impl_->triangles<<" textures="<<impl_->model->textures().size()<<" LOD="<<impl_->lod<<" projected="<<impl_->projected_size;
            result["summary"]=string(text.str());result["diagnostic"]=string(impl_->model->diagnostic());
        }
        if(impl_->loading){result["car_id"]=impl_->requested_car;result["skin_id"]=impl_->requested_skin;}
        result["parts"]=parts;
    });
    if(!error.empty()){result["ok"]=false;result["error"]=string(error);}return result;
}
void Ps2VehicleRenderer::set_inspection(bool enabled){
    const auto error=catch_all([&]{if(impl_->inspection==enabled)return;impl_->inspection=enabled;if(impl_->loading){request_car(string(impl_->requested_path),impl_->requested_car,impl_->requested_skin);return;}release_display();if(impl_->model)rebuild_display();});
    impl_->error=error;
}
void Ps2VehicleRenderer::set_part_visible(const godot::String& name,bool visible){const auto error=catch_all([&]{if(impl_->model)impl_->model->set_part_visible(name.utf8().get_data(),visible);});if(!error.empty())impl_->error=error;}
void Ps2VehicleRenderer::set_wireframe(bool enabled){const auto error=catch_all([&]{impl_->wireframe=enabled;for(const auto& draw:impl_->draws)if(draw.node)draw.node->set_material_overlay(enabled?impl_->wire:godot::Ref<godot::ShaderMaterial>());});if(!error.empty())impl_->error=error;}
void Ps2VehicleRenderer::set_bounds(bool enabled){const auto error=catch_all([&]{impl_->show_bounds=enabled;if(impl_->bounds_node)impl_->bounds_node->set_visible(enabled);});if(!error.empty())impl_->error=error;}
}

namespace port::godot_bridge {
godot::Dictionary Ps2VehicleRenderer::apply_driving_snapshot(const godot::Dictionary& snapshot){
    const auto error=catch_all([&]{
        if(!impl_->model||impl_->loading)throw std::logic_error("No completed vehicle load for driving publication");
        if(impl_->show_lods)throw std::logic_error("Driving pose requires a single vehicle");
        if(!snapshot.has("body")||!snapshot.has("wheels"))throw std::invalid_argument("Driving snapshot requires body and four wheels");
        if(snapshot["body"].get_type()!=godot::Variant::TRANSFORM3D||snapshot["wheels"].get_type()!=godot::Variant::ARRAY)
            throw std::invalid_argument("Invalid driving body/wheel types");
        const godot::Transform3D body=snapshot["body"];
        if(!std::isfinite(body.basis.determinant())||body.basis.determinant()==0)throw std::invalid_argument("Singular driving body basis");
        port::driving::validate_matrix(source_matrix(body));
        const godot::Array values=snapshot["wheels"];
        if(values.size()!=4)throw std::invalid_argument("Driving snapshot requires exactly four wheels");
        std::array<port::driving::WheelSnapshot,4> wheels;
        for(int i=0;i<4;++i){
            if(values[i].get_type()!=godot::Variant::DICTIONARY)throw std::invalid_argument("Invalid driving wheel type");
            const godot::Dictionary value=values[i];auto& wheel=wheels[i];
            for(const char* key:{"tire","brake","blur","phase","angular_velocity","blur_alpha"})
                if(!value.has(key))throw std::invalid_argument("Incomplete driving wheel publication");
            for(const char* key:{"tire","brake","blur"})if(value[key].get_type()!=godot::Variant::TRANSFORM3D)
                throw std::invalid_argument("Invalid driving wheel transform");
            for(const char* key:{"phase","angular_velocity","blur_alpha"})if(value[key].get_type()!=godot::Variant::INT&&value[key].get_type()!=godot::Variant::FLOAT)
                throw std::invalid_argument("Invalid driving wheel numeric field");
            wheel.tire=source_matrix(value["tire"]);wheel.brake=source_matrix(value["brake"]);wheel.blur=source_matrix(value["blur"]);
            wheel.phase=value["phase"];wheel.angular_velocity=value["angular_velocity"];wheel.blur_alpha=value["blur_alpha"];
        }
        port::driving::VehicleLightState lights;
        if(snapshot.has("lights")){
            if(snapshot["lights"].get_type()!=godot::Variant::DICTIONARY)throw std::invalid_argument("Invalid driving light state");
            const godot::Dictionary value=snapshot["lights"];
            for(const char* key:{"headlight_left","headlight_right","brakelights"})
                if(!value.has(key)||value[key].get_type()!=godot::Variant::BOOL)throw std::invalid_argument("Incomplete driving light publication");
            lights={bool(value["headlight_left"]),bool(value["headlight_right"]),bool(value["brakelights"])};
        }
        port::driving::VehicleFlareState flares;const bool has_flares=snapshot.has("flare_intensities");
        if(has_flares){if(snapshot["flare_intensities"].get_type()!=godot::Variant::PACKED_FLOAT32_ARRAY)throw std::invalid_argument("Invalid driving flare publication");const godot::PackedFloat32Array values=snapshot["flare_intensities"];if(values.size()!=12)throw std::invalid_argument("Driving flare publication requires twelve original lamp intensities");for(unsigned i=0;i<12;++i){if(!std::isfinite(values[i])||values[i]<0||values[i]>1)throw std::invalid_argument("Invalid original lamp intensity");flares.intensity[i]=values[i];}}
        port::driving::WiperSnapshot wipers;
        if(snapshot.has("wipers")){
            if(snapshot["wipers"].get_type()!=godot::Variant::DICTIONARY)throw std::invalid_argument("Invalid driving wiper state");
            const godot::Dictionary value=snapshot["wipers"];
            for(const char* key:{"state","on_time","off_time","world_frame"})
                if(!value.has(key)||value[key].get_type()!=godot::Variant::INT)throw std::invalid_argument("Incomplete driving wiper publication");
            const auto signed_field=[&](const char* key){const int64_t v=value[key];
                if(v<std::numeric_limits<std::int32_t>::min()||v>std::numeric_limits<std::int32_t>::max())throw std::invalid_argument("Driving wiper integer exceeds original width");return std::int32_t(v);};
            wipers.state=signed_field("state");wipers.on_time=signed_field("on_time");wipers.off_time=signed_field("off_time");
            const int64_t frame=value["world_frame"];
            if(frame<0||std::uint64_t(frame)>std::numeric_limits<std::uint32_t>::max())throw std::invalid_argument("Driving wiper frame exceeds original width");
            wipers.world_frame=std::uint32_t(frame);
        }
        port::driving::validate_wheels(wheels);
        impl_->model->apply_wiper_state(wipers);
        impl_->flare_state=flares;impl_->flare_state_available=has_flares;
        impl_->model->apply_driving_pose(wheels);set_transform(body);impl_->driving_pose=true;impl_->lights=lights;
        if(impl_->frontend_lights){impl_->frontend_lights.reset();++impl_->frontend_light_revision;}impl_->error.clear();layout_lods();
    });
    if(!error.empty())impl_->error=error;
    auto result=get_state();result["ok"]=error.empty();result["driving_pose"]=impl_->driving_pose;
    if(!error.empty())result["error"]=string(error);return result;
}
}

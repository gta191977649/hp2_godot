#include "ps2_track_renderer.hpp"
#include "track_environment_renderer.hpp"
#include "track_postprocess.hpp"
#include "track_reflection_renderer.hpp"
#include "track_shadow_renderer.hpp"
#include "track_mesh_batches.hpp"
#include "track_world_batches.hpp"
#include "track_submission_cache.hpp"
#include "track_submission_digest.hpp"
#include "track_optimization.hpp"
#include "track_occlusion.hpp"
#include "track_instance_batches.hpp"
#include "track_instance_pool.hpp"
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include "track_sun_renderer.hpp"
#include "track_particle_renderer.hpp"
#include "ps2_driving_runtime.hpp"
#if HP2_DEBUG_BUILD
#include "track_mesh_batches_tests.hpp"
#include "track_instance_pool_tests.hpp"
#endif
#include "boundary.hpp"
#include "game_core/port/track/runtime.hpp"
#include "game_core/port/track/static_lods.hpp"
#include "game_core/port/track/settings.hpp"
#include "game_core/port/track/projection.hpp"
#include "platform/shaders/track_shader.hpp"
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <sstream>
#include <iomanip>
#include <limits>
#include "game_core/port/track/debug_geometry.hpp"
#include "game_core/port/driving/surface.hpp"
#include "game_core/indep/src/scenery.hpp"
#include "game_core/port/track/props_visibility.hpp"
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
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <set>
#include <godot_cpp/classes/sub_viewport.hpp>
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
godot::Array track_surface_arrays(const port::render::VehicleSurface& surface){
                godot::PackedVector3Array positions;godot::PackedVector2Array uv;godot::PackedColorArray colors;godot::PackedInt32Array indices;godot::PackedFloat32Array normals;normals.resize(surface.vertices.size()*4);std::size_t vi=0;
                for(const auto& v:surface.vertices){const float raw[4]{v.normal[0],v.normal[1],v.normal[2],0};std::memcpy(normals.ptrw()+vi++*4,raw,sizeof(raw));positions.push_back(host(v.position));uv.push_back({v.uv[0],v.uv[1]});const auto c=v.raw_color;
                    if(!v.has_color)throw std::runtime_error("Original track VIF colour stream is absent");
                    colors.push_back(godot::Color(float((c&31)*8)/255.f,float(((c>>5)&31)*8)/255.f,float(((c>>10)&31)*8)/255.f,float((c&0x8000)?128:0)/255.f));}
                for(auto i:surface.indices)indices.push_back(int32_t(i));
                godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);arrays[godot::Mesh::ARRAY_VERTEX]=positions;arrays[godot::Mesh::ARRAY_TEX_UV]=uv;arrays[godot::Mesh::ARRAY_COLOR]=colors;arrays[godot::Mesh::ARRAY_INDEX]=indices;arrays[godot::Mesh::ARRAY_CUSTOM0]=normals;
    return arrays;
}
bool host_switch(const char* prefix,bool fallback){
    const auto args=godot::OS::get_singleton()->get_cmdline_user_args();for(int i=0;i<args.size();++i)if(args[i].begins_with(prefix)){
        const auto value=args[i].substr(godot::String(prefix).length());if(value!="true"&&value!="false")throw std::invalid_argument("Host cache switch requires true or false");return value=="true";
    }return fallback;
}
const char* requested_render_thread(){
    static const std::string requested=[](){const auto args=godot::OS::get_singleton()->get_cmdline_user_args();for(int i=0;i<args.size();++i)if(args[i].begins_with("--render-thread-requested=")){const auto mode=args[i].trim_prefix("--render-thread-requested=");return std::string(mode.utf8().get_data());}return std::string("project");}();return requested.c_str();
}
port::render::SubmitTransform submit_transform(const godot::Transform3D& transform){
    port::render::SubmitTransform out;for(unsigned r=0;r<3;++r){out.origin[r]=transform.origin[r];for(unsigned c=0;c<3;++c)out.basis[r*3+c]=transform.basis[r][c];}return out;
}
bool performance_controls(){
    static const bool enabled=[](){const auto args=godot::OS::get_singleton()->get_cmdline_user_args();for(int i=0;i<args.size();++i)if(args[i]=="--track-performance-analysis")return true;return false;}();
    return bool(HP2_DEBUG_BUILD)||enabled;
}
struct Loaded {std::shared_ptr<const port::track::Scene> scene;std::shared_ptr<port::driving::Memory> math;std::shared_ptr<const port::driving::TrackEnvironment> environment;std::unique_ptr<port::track::DebugGeometry> debug;std::string error,debug_error;std::size_t released_bundle_bytes=0;};
}
struct Ps2TrackRenderer::Impl {
    std::string lod_backend="original";
    std::unique_ptr<port::track::StaticLodScene> static_lods;
    std::map<unsigned,std::vector<std::vector<port::track::LodDistanceRange>>> lod_ranges;
    std::map<unsigned,float> lod_focals;
    std::map<unsigned,godot::Dictionary> native_culling_views;
    std::size_t hlod_warm_cursor=0;
    std::uint64_t hlod_warm_frame_base=0;
    godot::Camera3D* hlod_load_camera=nullptr;
    double environment_submit_ms=0,main_submit_ms=0,reflection_submit_ms=0,post_submit_ms=0,particle_submit_ms=0,sun_submit_ms=0,sun_readback_ms=0;
    godot::Dictionary batch_breaks;
    std::uint64_t scene_generation=0;unsigned prepare_workers=0;bool submit_math_enabled=false;std::map<unsigned,godot::Dictionary> preparation_views;
    int diagnostics_level=0;bool submit_cache=true,depth_param_cache=true,dynamic_plan=false,residency_cache=false,capture_world_batches=false;TrackSubmissionCache submission_plan;std::map<unsigned,TrackSubmissionCache> reflection_plans;
    double load_ms=0;std::chrono::steady_clock::time_point load_begin;
    TrackOptimizationOptions optimization;std::string optimization_startup_error;
    godot::Array active_viewport_rids;
    std::uint64_t parameter_sets=0,parameter_skips=0,batch_cache_skips=0,material_reuses=0,sorting_updates=0;
    godot::Array material_state_samples;std::uint64_t material_state_mismatches=0;
    std::uint64_t source_mesh_creations=0,source_payload_bytes=0;
    std::map<std::tuple<std::uint64_t,std::uint32_t,unsigned,int>,godot::Ref<godot::ShaderMaterial>> shared_materials;
    std::map<std::uint64_t,std::map<godot::StringName,godot::Variant>> uniform_values;
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
    bool render_collision=false,render_material=false,show_lod=false;
    std::string debug_error;
    std::unique_ptr<TrackEnvironmentRenderer> environment_renderer;
    EnvironmentFrame environment_frame;
    std::unique_ptr<TrackPostprocess> postprocess_renderer;
    std::unique_ptr<TrackReflectionRenderer> reflection_renderer;
    std::array<std::unique_ptr<TrackEnvironmentRenderer>,6> reflection_skies;
    std::unique_ptr<TrackShadowRenderer> shadow_renderer;ShadowFrame shadow_frame;godot::Dictionary visual_state;
    std::unique_ptr<TrackSunRenderer> sun_renderer;SunFrame sun_frame;bool readback_connected=false;
    std::unique_ptr<TrackParticleRenderer> particle_renderer;ParticleFrame particle_frame;
    godot::Ref<Ps2DrivingRuntime> driving_effect_source;
    std::uint32_t particle_seed=0x23985727;float visual_timestep=0;
    std::vector<std::pair<std::size_t,std::tuple<unsigned,std::size_t,std::size_t,std::size_t>>> main_alpha_draws;
    bool visual_state_valid=false;std::array<float,3> environment_position{};

    port::track::Runtime runtime;
    port::track::Frame frame;
    mutable std::string packet_digest;
    port::track::FrameView frame_view;
    double native_frame_ms=0,host_submit_ms=0;
    int camera_mode=2;float effective_near=.5f,effective_far=10000.f;bool roadshine_active=false;
    struct DrawState {
        port::render::SubmitPoseCache pose_cache;
        std::array<float,16> matrix{};std::array<float,28> lights{};
        godot::Vector3 local_camera;
        float near_z=0,far_z=0;bool initialized=false,visible=false,roadshine_enabled=true;
        float hlod_begin=-1,hlod_end=-1;
    };
    struct Draw {port::track::EffectState material_effect{};std::vector<std::pair<std::size_t,std::size_t>> batch_sources;std::size_t mesh_bytes=0;float sorting_offset=0;bool sorting_valid=false;godot::MeshInstance3D* node=nullptr;godot::Ref<godot::ShaderMaterial> material;DrawState state;std::uint64_t used_frame=0;
        godot::MeshInstance3D* reflection_node=nullptr;godot::Ref<godot::ShaderMaterial> reflection_material;DrawState reflection_state;
        std::array<float,9> reflection_rows{};godot::Ref<godot::Texture2D> reflection_texture;};
    std::map<std::tuple<unsigned,std::size_t,std::size_t,std::size_t>,Draw> draws;
    std::map<unsigned,std::vector<Draw*>> view_draws;std::uint64_t frame_number=0;
    std::map<std::pair<std::size_t,std::size_t>,godot::Ref<godot::ArrayMesh>> meshes;
    std::map<std::pair<std::size_t,std::size_t>,std::uint64_t> mesh_last_used;
    std::unique_ptr<TrackMeshBatchCache> batch_cache;
    // Host policy only: active views are never evicted or clipped by these limits.
    static constexpr std::size_t inactive_draw_limit=256,inactive_mesh_limit=16u*1024u*1024u;
    std::size_t draw_evictions=0,released_bundle_bytes=0;
    std::unique_ptr<TrackOcclusionPool> occlusion_pool;std::size_t opaque_queue_draws=0;
    std::unique_ptr<TrackInstancePool> instance_pool;std::vector<TrackInstanceBoundary> instance_owners;
    std::vector<godot::MultiMeshInstance3D*> main_instance_nodes;
    std::size_t instance_groups=0,instanced_commands=0,instance_saved_draws=0;
    std::map<unsigned,std::array<std::size_t,3>> submission_counts;
    std::map<unsigned,std::array<double,3>> submission_times;
    const std::array<godot::StringName,7> light_names{{"light_35c","light_35d","light_35e","light_35f","light_360","light_361","light_362"}};
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
    // Host render gaps do not invalidate successfully loaded assets/topology.
    std::string render_error,readback_error;
};
Ps2TrackRenderer::Ps2TrackRenderer():impl_(std::make_unique<Impl>()){const auto failure=catch_all([&]{impl_->optimization=startup_track_optimizations();impl_->submit_math_enabled=host_switch("--track-submit-math=",false);impl_->submit_cache=host_switch("--track-submit-cache=",true);impl_->dynamic_plan=host_switch("--track-dynamic-plan-cache=",false);impl_->residency_cache=host_switch("--track-residency-cache=",false);impl_->capture_world_batches=host_switch("--track-capture-world-batches=",false);impl_->depth_param_cache=host_switch("--track-depth-param-cache=",true);
    const auto args=godot::OS::get_singleton()->get_cmdline_user_args();for(int i=0;i<args.size();++i)if(args[i].begins_with("--render-prepare-workers=")){const auto value=args[i].trim_prefix("--render-prepare-workers=");if(value!="0"&&value!="2"&&value!="4")throw std::invalid_argument("Render preparation workers must be 0, 2 or 4");impl_->prepare_workers=unsigned(value.to_int());}
    const std::string requested=requested_render_thread();const std::string actual=godot::RenderingServer::get_singleton()->is_on_render_thread()?"safe":"separate";
    if(requested!="project"&&requested!=actual)throw std::runtime_error("Godot requested/actual rendering thread mode mismatch: "+requested+" / "+actual);
    for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-lod-backend=")){
        const auto value=args[i].trim_prefix("--track-lod-backend=");if(value!="original")throw std::invalid_argument("Track LOD backend removed: use original native culling");impl_->lod_backend=value.utf8().get_data();}
});if(!failure.empty()){impl_->error=failure;impl_->optimization_startup_error=failure;}}
Ps2TrackRenderer::~Ps2TrackRenderer()=default;
void Ps2TrackRenderer::_notification(int what){
    // Release host-owned child resources while children still exist. Godot's
    // Node destructor recursively frees them before C++ member destructors run.
    if(what==godot::Node::NOTIFICATION_EXIT_TREE){auto* tree=get_tree();const auto observer=callable_mp(this,&Ps2TrackRenderer::depth_source_structure_changed);if(tree){if(tree->is_connected("node_added",observer))tree->disconnect("node_added",observer);if(tree->is_connected("node_removed",observer))tree->disconnect("node_removed",observer);}clear_track();return;}
    if(what==godot::Node3D::NOTIFICATION_VISIBILITY_CHANGED&&!is_visible_in_tree())impl_->postprocess_renderer.reset();
}
void Ps2TrackRenderer::_bind_methods(){
    godot::ClassDB::bind_method(godot::D_METHOD("configure_lod_backend","backend"),&Ps2TrackRenderer::configure_lod_backend);
    godot::ClassDB::bind_method(godot::D_METHOD("get_lod_state"),&Ps2TrackRenderer::get_lod_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_memory_state"),&Ps2TrackRenderer::get_memory_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_lod_ranges","instance","view"),&Ps2TrackRenderer::get_lod_ranges);
    godot::ClassDB::bind_method(godot::D_METHOD("configure_optimizations","options"),&Ps2TrackRenderer::configure_optimizations);
    godot::ClassDB::bind_method(godot::D_METHOD("get_optimization_state"),&Ps2TrackRenderer::get_optimization_state);
    godot::ClassDB::bind_method(godot::D_METHOD("_complete_visual_readback"),&Ps2TrackRenderer::complete_visual_readback);
    godot::ClassDB::bind_method(godot::D_METHOD("get_render_targets"),&Ps2TrackRenderer::get_render_targets);
#if HP2_DEBUG_BUILD
    godot::ClassDB::bind_static_method("Ps2TrackRenderer",godot::D_METHOD("test_mesh_batch_cache"),&test_track_mesh_batch_cache);
    godot::ClassDB::bind_static_method("Ps2TrackRenderer",godot::D_METHOD("test_instance_pool"),&test_track_instance_pool);
    godot::ClassDB::bind_static_method("Ps2TrackRenderer",godot::D_METHOD("test_particle_mesh_reuse"),&TrackParticleRenderer::test_mesh_reuse);
#endif
    godot::ClassDB::bind_method(godot::D_METHOD("get_catalog"),&Ps2TrackRenderer::get_catalog);
    godot::ClassDB::bind_method(godot::D_METHOD("request_track","number"),&Ps2TrackRenderer::request_track);
    godot::ClassDB::bind_method(godot::D_METHOD("load_track","relative_path"),&Ps2TrackRenderer::load_track);
    godot::ClassDB::bind_method(godot::D_METHOD("pump_load"),&Ps2TrackRenderer::pump_load);
    godot::ClassDB::bind_method(godot::D_METHOD("get_state"),&Ps2TrackRenderer::get_state);
    godot::ClassDB::bind_method(godot::D_METHOD("frame_summary"),&Ps2TrackRenderer::frame_summary);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_track"),&Ps2TrackRenderer::clear_track);
    godot::ClassDB::bind_method(godot::D_METHOD("set_effect_enabled","id","enabled"),&Ps2TrackRenderer::set_effect_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_effects"),&Ps2TrackRenderer::get_effects);
    godot::ClassDB::bind_method(godot::D_METHOD("set_render_props","enabled"),&Ps2TrackRenderer::set_render_props);
    godot::ClassDB::bind_method(godot::D_METHOD("restore_render_defaults"),&Ps2TrackRenderer::restore_render_defaults);
    godot::ClassDB::bind_method(godot::D_METHOD("set_visibility_override","enabled","distance"),&Ps2TrackRenderer::set_visibility_override);
    godot::ClassDB::bind_method(godot::D_METHOD("set_diagnostics_level","level"),&Ps2TrackRenderer::set_diagnostics_level);
    godot::ClassDB::bind_method(godot::D_METHOD("get_render_controls"),&Ps2TrackRenderer::get_render_controls);
    godot::ClassDB::bind_method(godot::D_METHOD("get_environment_capture"),&Ps2TrackRenderer::get_environment_capture);
    godot::ClassDB::bind_method(godot::D_METHOD("set_camera_mode","original_type"),&Ps2TrackRenderer::set_camera_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("set_visual_timestep","seconds"),&Ps2TrackRenderer::set_visual_timestep);
    godot::ClassDB::bind_method(godot::D_METHOD("set_visual_state","state"),&Ps2TrackRenderer::set_visual_state);
    godot::ClassDB::bind_method(godot::D_METHOD("set_driving_effect_source","source"),&Ps2TrackRenderer::set_driving_effect_source);
    godot::ClassDB::bind_method(godot::D_METHOD("set_render_collision","enabled"),&Ps2TrackRenderer::set_render_collision);
    godot::ClassDB::bind_method(godot::D_METHOD("set_show_lod","enabled"),&Ps2TrackRenderer::set_show_lod);
    godot::ClassDB::bind_method(godot::D_METHOD("set_render_material","enabled"),&Ps2TrackRenderer::set_render_material);
    godot::ClassDB::bind_method(godot::D_METHOD("update_view","camera"),&Ps2TrackRenderer::update_view);
}
void Ps2TrackRenderer::release_display(){
    impl_->render_error.clear();impl_->readback_error.clear();
    ++impl_->scene_generation;
    if(impl_->hlod_load_camera){impl_->frame_number=impl_->hlod_warm_frame_base;remove_child(impl_->hlod_load_camera);memdelete(impl_->hlod_load_camera);impl_->hlod_load_camera=nullptr;}
    impl_->native_culling_views.clear();impl_->static_lods.reset();impl_->lod_ranges.clear();impl_->lod_focals.clear();impl_->hlod_warm_cursor=0;
    impl_->postprocess_renderer.reset();
    impl_->sun_renderer.reset();impl_->sun_frame={};
    if(impl_->particle_renderer)impl_->particle_seed=impl_->particle_renderer->global_seed();
    impl_->particle_renderer.reset();impl_->particle_frame={};impl_->main_alpha_draws.clear();impl_->visual_timestep=0;
    impl_->reflection_renderer.reset();for(auto& sky:impl_->reflection_skies)sky.reset();
    impl_->shadow_renderer.reset();impl_->shadow_frame={};impl_->visual_state.clear();
    impl_->environment_renderer.reset();impl_->environment_frame={};
    impl_->visual_state_valid=false;
    impl_->runtime.unload();impl_->frame={};impl_->packet_digest.clear();impl_->view_draws.clear();impl_->draws.clear();
    impl_->debug_fill=impl_->debug_edges=nullptr;impl_->debug_material.unref();impl_->debug.reset();impl_->debug_error.clear();
    for(auto it=impl_->nodes.rbegin();it!=impl_->nodes.rend();++it){remove_child(*it);memdelete(*it);}impl_->nodes.clear();
    impl_->occlusion_pool.reset();impl_->opaque_queue_draws=0;
    impl_->instance_pool.reset();impl_->instance_owners.clear();impl_->main_instance_nodes.clear();impl_->instance_groups=impl_->instanced_commands=impl_->instance_saved_draws=0;
    impl_->preparation_views.clear();impl_->material_state_samples.clear();impl_->material_state_mismatches=0;impl_->source_mesh_creations=impl_->source_payload_bytes=0;impl_->reflection_plans.clear();impl_->submission_plan.clear();impl_->batch_cache.reset();impl_->submission_counts.clear();impl_->submission_times.clear();impl_->draw_evictions=impl_->released_bundle_bytes=0;
    impl_->uniform_values.clear();impl_->shared_materials.clear();impl_->active_viewport_rids.clear();impl_->parameter_sets=impl_->parameter_skips=impl_->batch_cache_skips=impl_->material_reuses=impl_->sorting_updates=0;
    impl_->textures.clear();impl_->meshes.clear();impl_->mesh_last_used.clear();impl_->mipmaps.clear();impl_->shaders.clear();impl_->materials.clear();
    impl_->alpha_ranges.clear();impl_->texture_info.clear();impl_->bounds={};impl_->has_bounds=false;
}
godot::Dictionary Ps2TrackRenderer::configure_optimizations(const godot::Dictionary& options){
    const auto failure=catch_all([&]{if(impl_->loading||impl_->scene)throw std::logic_error("Configure host optimizations before loading a track");const auto proposed=optimization_options_from_dictionary(options,impl_->optimization);impl_->optimization=proposed;});
    auto result=get_optimization_state();result["ok"]=failure.empty();result["error"]=text(failure);return result;
}
godot::Dictionary Ps2TrackRenderer::configure_lod_backend(const godot::String& backend){
    const auto failure=catch_all([&]{if(impl_->loading||impl_->scene)throw std::logic_error("Configure LOD backend before loading a track");
        if(backend!="original")throw std::invalid_argument("Godot LOD management has been removed; expected original");
        impl_->lod_backend=backend.utf8().get_data();});
    auto result=get_lod_state();result["ok"]=failure.empty();result["error"]=text(failure);return result;
}
godot::Dictionary Ps2TrackRenderer::get_lod_state()const{
    godot::Dictionary state;state["backend"]=text(impl_->lod_backend);state["source_instances"]=int64_t(impl_->static_lods?impl_->static_lods->instances.size():0);state["resource_levels"]=int64_t(impl_->static_lods?impl_->static_lods->levels:0);
    state["resident_surface_nodes"]=int64_t(impl_->draws.size());state["load_ms"]=impl_->load_ms;
    state["visual_frame_counter"]=int64_t(impl_->frame_number);
    state["missing_model_slots"]=int64_t(impl_->static_lods?impl_->static_lods->gaps.size():0);
    state["original_texture_fixups"]=int64_t(impl_->scene?impl_->scene->texture_fixups.size():0);
    auto* server=godot::RenderingServer::get_singleton();
    state["engine_global_video_memory_bytes"]=server->get_rendering_info(godot::RenderingServer::RENDERING_INFO_VIDEO_MEM_USED);
    state["engine_global_draw_calls"]=server->get_rendering_info(godot::RenderingServer::RENDERING_INFO_TOTAL_DRAW_CALLS_IN_FRAME);
    state["engine_global_triangles"]=server->get_rendering_info(godot::RenderingServer::RENDERING_INFO_TOTAL_PRIMITIVES_IN_FRAME);
    state["selection"]=impl_->show_lod?"Debug all authored levels (native selection bypassed)":"Original zone / tree / instance / projected-size selection";
    state["host_lod_selection"]=false;state["debug_all_lods"]=impl_->show_lod;
    state["boundary_equivalence"]=false;godot::Dictionary views;
    for(const auto& entry:impl_->lod_focals){godot::Dictionary v;v["logical_focal_h"]=entry.second;v["instances"]=int64_t(impl_->lod_ranges.at(entry.first).size());views[entry.first]=v;}
    state["views"]=views;godot::Dictionary culling;for(const auto& entry:impl_->native_culling_views)culling[entry.first]=entry.second;
    state["native_culling_views"]=culling;return state;
}
godot::Dictionary Ps2TrackRenderer::get_lod_ranges(int instance,int view)const{
    godot::Dictionary result;const auto failure=catch_all([&]{
        if(!impl_->static_lods||instance<0||std::size_t(instance)>=impl_->static_lods->instances.size())throw std::invalid_argument("HLOD instance unavailable");
        const auto found=impl_->lod_ranges.find(unsigned(view));if(view<0||found==impl_->lod_ranges.end())throw std::invalid_argument("HLOD view not prepared");
        const auto& object=impl_->static_lods->instances.at(instance);result["anchor"]=host(object.anchor);result["section"]=object.section;godot::Array levels;
        for(const auto& range:found->second.at(instance)){godot::Dictionary level;level["slot"]=range.slot;level["begin"]=range.begin;level["end"]=range.end;
            for(const auto& source:object.levels)if(source.slot==range.slot){level["model_hash"]=int64_t(source.model_hash);level["available"]=source.available;break;}levels.push_back(level);}
        result["levels"]=levels;});result["ok"]=failure.empty();result["error"]=text(failure);return result;
}
godot::Dictionary Ps2TrackRenderer::get_optimization_state()const{
    godot::Dictionary result;result["submit_math_enabled"]=impl_->submit_math_enabled;result["render_prepare_workers"]=impl_->prepare_workers;result["render_thread_requested"]=requested_render_thread();result["render_thread_actual"]=godot::RenderingServer::get_singleton()->is_on_render_thread()?"safe":"separate";godot::Dictionary preparations;for(const auto& p:impl_->preparation_views)preparations[p.first]=p.second;result["render_preparation_views"]=preparations;result["effective"]=optimization_options_dictionary(impl_->optimization);result["loaded"]=bool(impl_->scene);result["resource_reuse_default_validated"]=godot::String(godot::ProjectSettings::get_singleton()->get_setting("hp2/rendering/optimization_profile","baseline"))=="optimized";result["instancing_validation"]="Experimental: Track24 exact colour mismatch; excluded from optimized profile";result["material_sharing_enabled"]=false;result["material_sharing_validation"]="Disabled: Track11 reflection atlas changed under sharing; distinct material identity retained";result["experimental_occlusion"]=true;result["experimental_opaque_queue"]=true;result["world_batches_scope"]="All tracks use the same unlit contiguous main-view batching policy; local lighting, alpha order, capture consumers, nonidentity owners and HLOD retain baseline";result["capture_world_batches"]=impl_->capture_world_batches;result["residency_cache"]=impl_->residency_cache;result["inactive_draw_limit"]=int64_t(impl_->residency_cache?4096:Impl::inactive_draw_limit);result["inactive_mesh_limit_bytes"]=int64_t(impl_->residency_cache?128u*1024u*1024u:Impl::inactive_mesh_limit);result["inactive_retention_frames"]=int64_t(impl_->residency_cache?1800:120);result["dynamic_plan_cache"]=impl_->dynamic_plan;result["submission_plan_cache"]=impl_->submit_cache;result["submission_plan_hits"]=int64_t(impl_->submission_plan.hits);result["submission_plan_builds"]=int64_t(impl_->submission_plan.builds);result["section_owner_builds"]=int64_t(impl_->submission_plan.owner_builds);result["diagnostics_level"]=impl_->diagnostics_level;result["world_batches_consumer_fallback"]=impl_->optimization.world_batches&&impl_->visual_state_valid;
    godot::Dictionary counters;counters["material_state_samples"]=impl_->material_state_samples;counters["material_state_mismatches"]=int64_t(impl_->material_state_mismatches);counters["source_mesh_creations"]=int64_t(impl_->source_mesh_creations);counters["geometry_upload_payload_bytes"]=int64_t(impl_->source_payload_bytes+(impl_->batch_cache?impl_->batch_cache->uploaded_payload_bytes():0));counters["upload_byte_scope"]="Logical track geometry CPU payload on uploads; excludes textures/particles/vehicle/driver conversion; not GPU transfer measurement";counters["batch_mesh_builds"]=int64_t(impl_->batch_cache?impl_->batch_cache->builds():0);counters["batch_mesh_cached_bytes"]=int64_t(impl_->batch_cache?impl_->batch_cache->bytes():0);counters["parameter_sets"]=int64_t(impl_->parameter_sets);counters["parameter_skips"]=int64_t(impl_->parameter_skips);counters["batch_cache_skips"]=int64_t(impl_->batch_cache_skips);counters["shared_materials"]=int64_t(impl_->shared_materials.size());counters["material_reuses"]=int64_t(impl_->material_reuses);counters["sorting_updates"]=int64_t(impl_->sorting_updates);counters["instance_groups"]=int64_t(impl_->instance_groups);counters["instanced_commands"]=int64_t(impl_->instanced_commands);counters["instance_saved_draws"]=int64_t(impl_->instance_saved_draws);if(impl_->instance_pool){counters["instance_pool_entries"]=int64_t(impl_->instance_pool->entries());counters["instance_buffer_uploads"]=int64_t(impl_->instance_pool->transform_uploads());}counters["opaque_queue_draws"]=int64_t(impl_->opaque_queue_draws);if(impl_->occlusion_pool)result["occlusion"]=impl_->occlusion_pool->stats();if(impl_->sun_renderer)result["sun"]=impl_->sun_renderer->stats();if(impl_->particle_renderer)result["particles"]=impl_->particle_renderer->stats();if(impl_->postprocess_renderer)result["postprocess"]=impl_->postprocess_renderer->stats();result["counters"]=counters;return result;
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
    impl_->load_begin=std::chrono::steady_clock::now();impl_->load_ms=0;
    const auto failure=catch_all([&]{if(!impl_->optimization_startup_error.empty())throw std::invalid_argument(impl_->optimization_startup_error);if(!requests_enabled())throw std::logic_error("TrackViewer is disabled in Release builds");if(impl_->catalog.empty())get_catalog();
        auto entry=std::find_if(impl_->catalog.begin(),impl_->catalog.end(),[&](const auto& t){return t.number==number;});
        if(entry==impl_->catalog.end())throw std::invalid_argument("Unknown original track number");
        auto assets=source();const auto root=assets.root();const auto selected=*entry;
        impl_->number=number;impl_->loading=true;impl_->phase="Reading and decoding original track";
        impl_->texture_cursor=impl_->solid_cursor=impl_->surface_cursor=impl_->triangles=impl_->instance_cursor=0;
        const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");const auto project=std::string(start.utf8().get_data());const auto math=impl_->math;
        impl_->pending=std::async(std::launch::async,[root,selected,project,math]{Loaded out;try{
            port::assets::DirectorySource assets(root);auto scene=std::make_shared<port::track::Scene>(port::track::load_scene(assets,selected));
            out.environment=port::driving::load_track_environment(*scene);
            // All render/topology/path owners now contain decoded values. No runtime
            // reader uses the raw bundle; do not retain a second representation.
            out.released_bundle_bytes=scene->bundle.capacity();std::vector<std::uint8_t>().swap(scene->bundle);out.scene=std::move(scene);
            out.math=math?math:std::make_shared<port::driving::Memory>(port::driving::read_debug_image(port::driving::debug_image_path(project)));
        }catch(const std::exception& e){out.error=e.what();}return out;});
    });
    if(!failure.empty()){impl_->error=failure;impl_->loading=false;impl_->phase="Failed";}return get_state();
}
godot::Dictionary Ps2TrackRenderer::pump_load(){
    if(!impl_->loading)return get_state();
    const auto failure=catch_all([&]{
        if(impl_->pending.valid()){
            if(impl_->pending.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
            auto loaded=impl_->pending.get();if(!loaded.error.empty())throw std::runtime_error(loaded.error);impl_->released_bundle_bytes=loaded.released_bundle_bytes;
            impl_->debug=std::move(loaded.debug);impl_->debug_error=std::move(loaded.debug_error);
            impl_->environment=std::move(loaded.environment);impl_->math=std::move(loaded.math);impl_->scene=std::move(loaded.scene);if(!impl_->scene)throw std::runtime_error("Track loader returned no scene");
            impl_->runtime.load(impl_->scene);
            if(impl_->show_lod){
                impl_->static_lods=std::make_unique<port::track::StaticLodScene>(port::track::describe_static_lods(*impl_->scene));
                for(const auto& gap:impl_->static_lods->gaps)impl_->diagnostic+=gap+"\n";
            }
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
        while(impl_->instance_cursor<scene.instances.size()){
            const auto& instance=scene.instances[impl_->instance_cursor++];
            const godot::AABB bounds(host(instance.bbox_min),host(instance.bbox_max)-host(instance.bbox_min));
            impl_->bounds=impl_->has_bounds?impl_->bounds.merge(bounds):bounds;impl_->has_bounds=true;
            if(yield())return;
        }
        if(!impl_->has_bounds)throw std::runtime_error("Original track contains no supported render surfaces");
        if(impl_->static_lods){
            impl_->phase="Warming Godot HLOD resources";
            if(!impl_->hlod_load_camera){impl_->hlod_warm_frame_base=impl_->frame_number;impl_->hlod_load_camera=memnew(godot::Camera3D);add_child(impl_->hlod_load_camera);impl_->hlod_load_camera->set_current(false);}
            port::track::FrameView warm;warm.focal_h=port::track::original_focal_h(*impl_->math,0x4000);
            while(impl_->hlod_warm_cursor<scene.instances.size()){
                auto frame=impl_->runtime.prepare_hlod_frame(warm,*impl_->math,*impl_->static_lods,impl_->hlod_warm_cursor++);
                ++impl_->frame_number;submit_frame(frame,impl_->hlod_load_camera,0);
                if(yield())return;
            }
            // Warm-up stamps are host allocation bookkeeping, not visual time.
            impl_->frame_number=impl_->hlod_warm_frame_base;
            for(auto& entry:impl_->draws){entry.second.used_frame=0;entry.second.node->hide();entry.second.state.visible=false;if(entry.second.reflection_node)entry.second.reflection_node->hide();}
            remove_child(impl_->hlod_load_camera);memdelete(impl_->hlod_load_camera);impl_->hlod_load_camera=nullptr;
        }
        const auto debug_failure=catch_all([&]{build_debug_display();});
        if(!debug_failure.empty())impl_->debug_error=debug_failure;
        impl_->loading=false;impl_->ok=true;impl_->phase="Loaded";impl_->load_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-impl_->load_begin).count();apply_debug_display();
    });
    if(!failure.empty()){release_display();impl_->scene.reset();impl_->environment.reset();impl_->loading=false;impl_->ok=false;impl_->phase="Failed";impl_->error=failure;}
    return get_state();
}
// Tool overlay only: source topology vertices and physics remain unchanged. The
// 0.015 host-up offset prevents coplanar flicker; normal depth occlusion stays on.
void Ps2TrackRenderer::build_debug_display(){
    if(!(impl_->render_collision||impl_->render_material)||impl_->debug_fill||!impl_->environment)return;
    if(!impl_->debug)impl_->debug=std::make_unique<port::track::DebugGeometry>(port::track::build_debug_geometry(impl_->environment->topology,port::driving::read_track_material_catalog(*impl_->math)));
    if(!impl_->debug_error.empty())return;
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
    if(impl_->ok){if(impl_->render_collision||impl_->render_material)build_debug_display();else release_debug_display();}
    const bool ready=impl_->ok&&impl_->debug_error.empty();
    if(impl_->debug_fill)impl_->debug_fill->set_visible(ready&&(impl_->render_collision||impl_->render_material));
    if(impl_->debug_edges)impl_->debug_edges->set_visible(ready&&impl_->render_collision);
    if(impl_->debug_material.is_valid())impl_->debug_material->set_shader_parameter("material_mode",impl_->render_material);
}
void Ps2TrackRenderer::release_debug_display(){
    std::set<godot::Node3D*> removed;
    for(auto* node:{impl_->debug_fill,impl_->debug_edges})if(node){
        if(impl_->postprocess_renderer)impl_->postprocess_renderer->forget_mesh(node);
        removed.insert(node);remove_child(node);memdelete(node);
    }
    impl_->nodes.erase(std::remove_if(impl_->nodes.begin(),impl_->nodes.end(),[&](auto* n){return removed.count(n);}),impl_->nodes.end());
    impl_->debug_fill=impl_->debug_edges=nullptr;impl_->debug_material.unref();impl_->debug.reset();
}

void Ps2TrackRenderer::trim_draw_cache(){
    std::set<const Impl::Draw*> active;
    for(const auto& view:impl_->view_draws)for(auto* draw:view.second)active.insert(draw);
    using Iterator=decltype(impl_->draws)::iterator;std::vector<Iterator> inactive;
    std::size_t bytes=0;for(auto it=impl_->draws.begin();it!=impl_->draws.end();++it)if(!active.count(&it->second)){inactive.push_back(it);bytes+=it->second.mesh_bytes;}
    std::sort(inactive.begin(),inactive.end(),[](auto a,auto b){return a->second.used_frame<b->second.used_frame;});
    std::set<godot::Node3D*> removed;std::size_t remaining=inactive.size();
    for(auto it:inactive){auto& draw=it->second;
        const bool expired=impl_->frame_number>draw.used_frame&&impl_->frame_number-draw.used_frame>(impl_->residency_cache?1800u:120u);
        if(remaining<=(impl_->residency_cache?4096u:Impl::inactive_draw_limit)&&bytes<=(impl_->residency_cache?128u*1024u*1024u:Impl::inactive_mesh_limit)&&!expired)break;
        --remaining;bytes-=draw.mesh_bytes;
        for(auto* node:{draw.node,draw.reflection_node})if(node){
            if(impl_->postprocess_renderer)impl_->postprocess_renderer->forget_mesh(node);
            removed.insert(node);remove_child(node);memdelete(node);
        }
        for(const auto& material:{draw.material,draw.reflection_material})if(material.is_valid()){
            if(impl_->postprocess_renderer)impl_->postprocess_renderer->forget_material(material);
            impl_->uniform_values.erase(material->get_instance_id());
        }
        impl_->draws.erase(it);++impl_->draw_evictions;
    }
    impl_->nodes.erase(std::remove_if(impl_->nodes.begin(),impl_->nodes.end(),[&](auto* n){return removed.count(n);}),impl_->nodes.end());
    if(impl_->batch_cache)impl_->batch_cache->trim_unreferenced();
    // Source meshes are needed only by singleton batches/sky. Merged meshes use
    // decoded CPU attributes directly, avoiding temporary GPU upload/readback.
    std::vector<decltype(impl_->meshes)::iterator> unused;
    for(auto it=impl_->meshes.begin();it!=impl_->meshes.end();++it)if(it->second->get_reference_count()==1)unused.push_back(it);
    std::sort(unused.begin(),unused.end(),[&](auto a,auto b){return impl_->mesh_last_used.at(a->first)<impl_->mesh_last_used.at(b->first);});
    std::size_t retained=unused.size();for(auto it:unused){
        if(retained<=(impl_->residency_cache?2048u:256u)&&impl_->frame_number-impl_->mesh_last_used.at(it->first)<=(impl_->residency_cache?1800u:120u))break;
        --retained;impl_->mesh_last_used.erase(it->first);impl_->meshes.erase(it);
    }
}

godot::Ref<godot::ArrayMesh> Ps2TrackRenderer::acquire_source_mesh(std::size_t solid,std::size_t surface){
    const auto key=std::make_pair(solid,surface);impl_->mesh_last_used[key]=impl_->frame_number;
    const auto found=impl_->meshes.find(key);if(found!=impl_->meshes.end())return found->second;
    const auto& source=impl_->scene->geometry.solids.at(solid).surfaces.at(surface);
    if(source.indices.empty())throw std::runtime_error("Native track frame references an empty source surface");
    godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();
    auto arrays=impl_->batch_cache?impl_->batch_cache->source_arrays(solid,surface):godot::Array();if(arrays.is_empty())arrays=track_surface_arrays(source);
    const auto source_upload_begin=std::chrono::steady_clock::now();
    mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays,{}, {},uint64_t(godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT)<<godot::Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT);
    if(impl_->batch_cache)impl_->batch_cache->note_source_upload(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-source_upload_begin).count());
    if(mesh->get_surface_count()!=1)throw std::runtime_error("Track source mesh upload rejected");
    impl_->source_payload_bytes+=source.vertices.size()*sizeof(float)*13+source.indices.size()*sizeof(std::int32_t);++impl_->source_mesh_creations;impl_->meshes.emplace(key,mesh);return mesh;
}
void Ps2TrackRenderer::set_render_collision(bool enabled){const auto failure=catch_all([&]{if(!requests_enabled())throw std::logic_error("TrackViewer is disabled in Release builds");impl_->render_collision=enabled;apply_debug_display();});if(!failure.empty())impl_->debug_error=failure;}
void Ps2TrackRenderer::set_show_lod(bool enabled){
    const auto failure=catch_all([&]{
        if(!requests_enabled())throw std::logic_error("TrackViewer is disabled in Release builds");
        if(enabled&&impl_->scene&&!impl_->static_lods)impl_->static_lods=std::make_unique<port::track::StaticLodScene>(port::track::describe_static_lods(*impl_->scene));
        impl_->show_lod=enabled;
    });if(!failure.empty())impl_->debug_error=failure;
}
void Ps2TrackRenderer::set_render_material(bool enabled){const auto failure=catch_all([&]{if(!requests_enabled())throw std::logic_error("TrackViewer is disabled in Release builds");impl_->render_material=enabled;apply_debug_display();});if(!failure.empty())impl_->debug_error=failure;}

void Ps2TrackRenderer::submit_frame(port::track::Frame& frame,godot::Camera3D* camera,unsigned view_id){
    const auto view_begin=std::chrono::steady_clock::now();
    // Apply once before batching in main, reflection and HLOD views.
    port::track::apply_props_visibility(frame,port::track::shared_render_settings().render_props);
    if(view_id==0&&impl_->diagnostics_level==2){
        impl_->batch_breaks.clear();
        const auto count=[&](const char* reason){impl_->batch_breaks[reason]=int64_t(impl_->batch_breaks.get(reason,0))+1;};
        for(std::size_t i=1;i<frame.commands.size();++i){const auto& a=frame.commands[i-1];const auto& b=frame.commands[i];
            if(a.instance!=b.instance)count("instance_boundary");
            if(a.effect.environment_pass||b.effect.environment_pass)count("environment_pass");
            if(a.effect.alpha_pass&&a.depth!=b.depth)count("alpha_depth");
            const auto& sa=impl_->scene->geometry.solids.at(a.solid).surfaces.at(a.surface);const auto& sb=impl_->scene->geometry.solids.at(b.solid).surfaces.at(b.surface);
            if(sa.texture_hash!=sb.texture_hash)count("texture");if(sa.material_hash!=sb.material_hash)count("material");
            if(a.matrix!=b.matrix)count("matrix");if(a.light_context!=b.light_context||a.has_light_context!=b.has_light_context)count("lighting");
            if(a.effect.alpha_pass)count("alpha_commands");
            if(!same_track_batch_state(*impl_->scene,a,b))count("state_breaks");
        }
    }
    godot::Dictionary culling;culling["view"]=view_id;if(impl_->diagnostics_level>=1)culling["command_digest"]=text(track_submission_digest(frame));culling["zone"]=frame.zone;
    culling["selected_sections"]=int64_t(frame.sections.size());culling["considered_instances"]=int64_t(frame.considered_instances);
    culling["selected_instances"]=int64_t(frame.visible_instances);culling["surface_commands"]=int64_t(frame.commands.size());
    culling["native_selection"]=!impl_->show_lod;impl_->native_culling_views[view_id]=culling;
    const auto camera_position=camera->get_global_position();
    const auto direction=-camera->get_global_transform().basis.get_column(2);
    const auto& settings=port::track::shared_render_settings();
    const bool hlod=impl_->static_lods&&impl_->show_lod;
    bool ranges_changed=false;
    if(hlod){
        const float focal=frame.host_lod_focal_h;
        const auto old=impl_->lod_focals.find(view_id);
        if(old==impl_->lod_focals.end()||old->second!=focal){
            auto& ranges=impl_->lod_ranges[view_id];ranges.clear();ranges.reserve(impl_->static_lods->instances.size());
            for(const auto& item:impl_->static_lods->instances)ranges.push_back(port::track::lod_distance_ranges(item,focal));
            impl_->lod_focals[view_id]=focal;ranges_changed=true;
        }
    }
    if(view_id==0&&impl_->optimization.occlusion){if(!impl_->occlusion_pool)impl_->occlusion_pool=std::make_unique<TrackOcclusionPool>(*this);impl_->occlusion_pool->begin_frame();}
    if(!impl_->batch_cache)impl_->batch_cache=std::make_unique<TrackMeshBatchCache>([this](std::size_t solid,std::size_t surface){return acquire_source_mesh(solid,surface);},64u*1024u*1024u,
        [this](std::size_t solid,std::size_t surface){return track_surface_arrays(impl_->scene->geometry.solids.at(solid).surfaces.at(surface));});
    impl_->batch_cache->set_cpu_source([owner=impl_->scene](std::size_t solid,std::size_t surface){return std::shared_ptr<const port::render::VehicleSurface>(owner,&owner->geometry.solids.at(solid).surfaces.at(surface));},
        [this](std::size_t solid,std::size_t surface){return impl_->meshes.count({solid,surface})!=0;});
    const auto plan_begin=std::chrono::steady_clock::now();
    const bool use_world_batches=impl_->optimization.world_batches&&get_global_transform()==godot::Transform3D()&&((view_id==0&&!impl_->visual_state_valid)||impl_->capture_world_batches);
    std::vector<TrackCommandBatch> batches;const std::vector<TrackCommandBatch>* cached_plan=nullptr;
    std::map<std::size_t,std::size_t> instance_counts;
    if(impl_->optimization.instancing&&!hlod){
        if(!impl_->instance_pool)impl_->instance_pool=std::make_unique<TrackInstancePool>(*this);
        impl_->instance_pool->begin_view(view_id);if(view_id==0)impl_->main_instance_nodes.clear();
        if(impl_->instance_owners.empty()){impl_->instance_owners.resize(impl_->scene->instances.size());std::vector<unsigned> owners(impl_->instance_owners.size());for(auto& o:impl_->instance_owners)o.known=false;for(const auto& section:impl_->scene->sections)for(std::size_t i=0;i<section.instance_count;++i){const auto index=section.instance_begin+i;if(index>=owners.size())throw std::runtime_error("Section instance ownership exceeds track resources");auto& o=impl_->instance_owners[index];o.section=section.number;o.known=++owners[index]==1;}}
        std::vector<TrackInstanceBoundary> boundaries;boundaries.reserve(frame.commands.size());for(const auto& c:frame.commands){auto b=impl_->instance_owners.at(c.instance);b.view=view_id;b.lod_epoch=c.lod;boundaries.push_back(b);}
        const auto plan=contiguous_track_instance_batches(*impl_->scene,frame,boundaries);auto submissions=track_instance_submissions(*impl_->scene,frame,plan);batches.reserve(submissions.size());for(auto& entry:submissions){if(entry.instance_count>1)instance_counts.emplace(entry.batch.subranges.front().command,entry.instance_count);batches.push_back(std::move(entry.batch));}
        impl_->instance_groups+=plan.groups;impl_->instanced_commands+=plan.instanced_commands;impl_->instance_saved_draws+=plan.saved_draws;
    }else if(hlod){
        // A range belongs to one source instance and one original level. Never
        // combine surfaces whose distance anchors or level selection can differ.
        for(std::size_t i=0;i<frame.commands.size();++i){const auto& c=frame.commands[i];const auto& s=impl_->scene->geometry.solids.at(c.solid).surfaces.at(c.surface);
            const bool append=i&&!batches.empty()&&frame.commands[i-1].instance==c.instance&&frame.commands[i-1].lod==c.lod&&same_track_batch_state(*impl_->scene,frame.commands[i-1],c);
            if(!append)batches.emplace_back();auto& b=batches.back();b.subranges.push_back({i,b.vertices,s.vertices.size(),b.indices,s.indices.size()});b.vertices+=s.vertices.size();b.indices+=s.indices.size();}
    }else if(view_id==0)cached_plan=&impl_->submission_plan.acquire(*impl_->scene,frame,use_world_batches,impl_->submit_cache,impl_->dynamic_plan);
    else if(impl_->dynamic_plan||impl_->capture_world_batches)cached_plan=&impl_->reflection_plans[view_id].acquire(*impl_->scene,frame,use_world_batches,impl_->submit_cache,impl_->dynamic_plan);
    else batches=contiguous_track_batches(*impl_->scene,frame);
    const auto plan_end=std::chrono::steady_clock::now();
    impl_->submission_times[view_id][1]=std::chrono::duration<double,std::milli>(plan_end-plan_begin).count();
    const auto& final_batches=cached_plan?*cached_plan:batches;
    std::size_t largest=0;for(const auto& batch:final_batches)largest=std::max(largest,batch.subranges.size());
    impl_->submission_counts[view_id]={frame.commands.size(),final_batches.size(),largest};
    // Snapshot only required misses. Do not instantiate Godot resources here.
    const auto draw_snapshot_begin=std::chrono::steady_clock::now();
    std::vector<bool> prepare_requested;prepare_requested.reserve(final_batches.size());
    std::vector<decltype(impl_->draws)::iterator> draw_handles;draw_handles.reserve(final_batches.size());
    std::vector<port::render::SubmitMathInput> submit_inputs;if(impl_->submit_math_enabled)submit_inputs.reserve(final_batches.size());
    const auto parent_snapshot=impl_->submit_math_enabled?submit_transform(get_global_transform()):port::render::SubmitTransform{};
    for(const auto& batch:final_batches){const auto& c=frame.commands.at(batch.subranges.front().command);const auto& scene=*impl_->scene;
        const auto key=std::make_tuple(view_id,hlod?scene.instances.size()+c.instance*3+c.lod:c.instance,c.solid,c.surface+(batch.baked_world?scene.geometry.solids.at(c.solid).surfaces.size():0));const auto found=impl_->draws.find(key);
        bool reuse=!batch.baked_world&&impl_->optimization.resource_reuse&&found!=impl_->draws.end()&&found->second.batch_sources.size()==batch.subranges.size();
        if(reuse)for(std::size_t i=0;i<batch.subranges.size();++i){const auto& part=frame.commands.at(batch.subranges[i].command);if(found->second.batch_sources[i]!=std::make_pair(part.solid,part.surface)){reuse=false;break;}}
        prepare_requested.push_back(!reuse);draw_handles.push_back(found);
        if(impl_->submit_math_enabled){port::render::SubmitMathInput input;
            input.local=submit_transform((c.identity||batch.baked_world)?godot::Transform3D():host_matrix(c.matrix));input.parent=parent_snapshot;
            input.camera={{camera_position.x,camera_position.y,camera_position.z}};input.direction={{direction.x,direction.y,direction.z}};
            input.lights=c.light_context;input.command_count=frame.commands.size();input.rank=batch.subranges.front().command;
            if(found!=impl_->draws.end())input.cached=found->second.state.pose_cache;
            input.reflection=view_id==0&&c.effect.environment_pass&&impl_->reflection_renderer&&impl_->reflection_renderer->ready();
            input.needs_local_camera=bool(c.effect.flags&0x1c0)||input.reflection;
            input.model=c.matrix;if(c.identity){input.model={};input.model[0]=input.model[5]=input.model[10]=input.model[15]=1;}
            if(input.reflection)input.reflection_matrix=impl_->reflection_renderer->hack_matrix();
            submit_inputs.push_back(std::move(input));
        }
    }
    const double draw_snapshot_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-draw_snapshot_begin).count();
    impl_->batch_cache->prepare_view(frame,final_batches,prepare_requested,impl_->prepare_workers,impl_->frame_number,view_id,impl_->scene_generation,std::move(submit_inputs));
    struct PreparationScope {TrackMeshBatchCache& cache;std::map<unsigned,godot::Dictionary>& stats;unsigned view;double draw_snapshot_ms;~PreparationScope(){stats[view]=cache.preparation_stats();stats[view]["draw_snapshot_ms"]=draw_snapshot_ms;cache.finish_view();}} preparation_scope{*impl_->batch_cache,impl_->preparation_views,view_id,draw_snapshot_ms};
    const auto set_parameter=[&](const godot::Ref<godot::ShaderMaterial>& material,const godot::StringName& name,const godot::Variant& value){
        if(impl_->optimization.resource_reuse){auto& values=impl_->uniform_values[material->get_instance_id()];const auto old=values.find(name);if(old!=values.end()&&old->second==value){++impl_->parameter_skips;return;}values[name]=value;}
        material->set_shader_parameter(name,value);++impl_->parameter_sets;if(impl_->postprocess_renderer)impl_->postprocess_renderer->notify_material_parameter(material,name,value);};
        std::vector<Impl::Draw*> visible;visible.reserve(frame.commands.size());
        std::size_t rank=0,batch_index=0;
        for(const auto& batch:final_batches){
            const auto index=batch_index++;
            const auto* math=impl_->submit_math_enabled?&impl_->batch_cache->submit_math(index):nullptr;
            if(math&&!math->supported)math=nullptr;
            rank=batch.subranges.front().command;const auto& command=frame.commands.at(rank);const auto grouped=instance_counts.find(rank);const std::size_t instance_count=grouped==instance_counts.end()?1:grouped->second;
            const port::track::LodDistanceRange all_levels{command.lod,0,0};
            const port::track::LodDistanceRange* range=hlod?&all_levels:nullptr;

            const auto& scene=*impl_->scene;const auto& surface=scene.geometry.solids.at(command.solid).surfaces.at(command.surface);
            const auto key=std::make_tuple(view_id,hlod?impl_->scene->instances.size()+command.instance*3+command.lod:command.instance,command.solid,command.surface+(batch.baked_world?scene.geometry.solids.at(command.solid).surfaces.size():0));auto found=draw_handles[index];if(found==impl_->draws.end())found=impl_->draws.find(key);
            bool same_sources=!batch.baked_world&&impl_->optimization.resource_reuse&&found!=impl_->draws.end()&&found->second.batch_sources.size()==batch.subranges.size();
            if(same_sources)for(std::size_t i=0;i<batch.subranges.size();++i){const auto& c=frame.commands[batch.subranges[i].command];if(found->second.batch_sources[i]!=std::make_pair(c.solid,c.surface)){same_sources=false;break;}}
            godot::Ref<godot::ArrayMesh> batch_mesh;if(same_sources){batch_mesh=found->second.node->get_mesh();++impl_->batch_cache_skips;}else batch_mesh=impl_->batch_cache->acquire(frame,batch);
            if(found==impl_->draws.end()){
                const auto& header=impl_->texture_info.at(surface.texture_hash)->header;
                auto fusion=port::track::Fusion40::None;
                if(command.effect.second_pass_320){const auto a=impl_->alpha_ranges.at(surface.texture_hash),b=impl_->alpha_ranges.at(0x0184d1f8);fusion=port::track::fusion_40(command.effect,a[0],a[1],b[0],b[1]);if(fusion==port::track::Fusion40::None)throw std::runtime_error("Track flag40 destination-alpha pass exceeds proven host fusion coverage");}
                auto code=track_shader_source(header,command.effect,fusion);
                if(batch.baked_world)replace_shader_token(code,"void vertex(){","void vertex(){\n    MODELVIEW_MATRIX=VIEW_MATRIX*transpose(mat4(CUSTOM1,CUSTOM2,CUSTOM3,vec4(0.0,0.0,0.0,1.0)));\n");
                if(impl_->optimization.opaque_queue&&eligible_track_opaque_queue(command)){replace_shader_token(code,"ALPHA=1.0;","");++impl_->opaque_queue_draws;}
                auto shader=impl_->shaders.find(code);if(shader==impl_->shaders.end()){godot::Ref<godot::Shader> value;value.instantiate();value->set_code(text(code));shader=impl_->shaders.emplace(code,value).first;}
                Impl::Draw draw;draw.material_effect=command.effect;
                // E0055H01: preserve distinct material identity. Sharing changed
                // actual reflection-atlas pixels despite identical native packets.
                draw.material.instantiate();draw.material->set_shader(shader->second);
                set_parameter(draw.material,"base_texture",impl_->textures.at(surface.texture_hash));
                set_parameter(draw.material,"base_lit",bool(command.effect.flags&0x80));set_parameter(draw.material,"flag_0x4",bool(command.effect.flags&4));
                for(int level=1;level<=header.Field_0x6e;++level)set_parameter(draw.material,text("mip_"+std::to_string(level)),impl_->mipmaps.at(surface.texture_hash).at(level-1));
                if(command.effect.second_pass_320)set_parameter(draw.material,"specular_texture",impl_->textures.at(0x0184d1f8));
                const auto material=scene.materials.find(surface.material_hash);
                if(command.effect.flags&0x1c0){if(material==scene.materials.end())throw std::runtime_error("Native track frame has unresolved lighting material");const auto& block=material->second.blocks;for(int i=0;i<4;++i)set_parameter(draw.material,text("material_"+std::to_string(350+i)),godot::Vector4(block[i*4],block[i*4+1],block[i*4+2],block[i*4+3]));}
                draw.material->set_render_priority(command.effect.alpha_pass?64:-64);
                draw.node=memnew(godot::MeshInstance3D);draw.node->set_layer_mask(1u<<view_id);draw.node->set_meta("track_texture_hash",int64_t(surface.texture_hash));draw.node->set_meta("track_effect_flags",command.effect.flags);draw.node->set_meta("track_matrix_pack",command.effect.matrix_pack);draw.node->set_mesh(batch_mesh);draw.node->set_material_override(draw.material);draw.node->set_sorting_use_aabb_center(false);draw.node->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);add_child(draw.node);impl_->nodes.push_back(draw.node);found=impl_->draws.emplace(key,std::move(draw)).first;
            }
            if(view_id==0&&command.effect.alpha_pass)impl_->main_alpha_draws.emplace_back(rank,key);
            auto& draw=found->second;auto& cached=draw.state;
            if(impl_->diagnostics_level>=1){port::track::DrawCommand old_state{},new_state{};old_state.effect=draw.material_effect;new_state.effect=command.effect;if(!same_submission_input(old_state,new_state)){++impl_->material_state_mismatches;if(impl_->material_state_samples.size()<16){
                godot::Dictionary sample;sample["view"]=int64_t(view_id);sample["frame"]=int64_t(impl_->frame_number);sample["instance"]=int64_t(command.instance);sample["solid"]=int64_t(command.solid);sample["surface"]=int64_t(command.surface);
                const auto add=[&](const char* name,auto a,auto b){if(a!=b)sample[name]=godot::Array::make(text(std::to_string(static_cast<std::uint64_t>(a))),text(std::to_string(static_cast<std::uint64_t>(b))));};
                const auto& a=old_state.effect;const auto& b=new_state.effect;
                add("flags",a.flags,b.flags);add("matrix_pack",a.matrix_pack,b.matrix_pack);add("rendering_order",a.rendering_order,b.rendering_order);add("alpha_pass",a.alpha_pass,b.alpha_pass);add("rewrite_alpha",a.rewrite_alpha,b.rewrite_alpha);add("second_pass_320",a.second_pass_320,b.second_pass_320);add("environment_pass",a.environment_pass,b.environment_pass);
                add("tex_blend",a.texture.blend,b.texture.blend);add("tex_deferred",a.texture.deferred,b.texture.deferred);add("tex_alpha_register",a.texture.alpha_register,b.texture.alpha_register);add("tex_tex1_register",a.texture.tex1_register,b.texture.tex1_register);add("tex_texture_alpha",a.texture.texture_alpha,b.texture.texture_alpha);add("tex_additive",a.texture.additive,b.texture.additive);add("test_register",a.test.test_register,b.test.test_register);add("test_write_depth",a.test.write_depth,b.test.write_depth);add("test_alpha_test",a.test.alpha_test,b.test.alpha_test);add("test_alpha_reference",a.test.alpha_reference,b.test.alpha_reference);add("test_depth_test",a.test.depth_test,b.test.depth_test);
                impl_->material_state_samples.push_back(sample);
            }}}
            draw.mesh_bytes=batch.vertices*sizeof(float)*(batch.baked_world?25:13)+batch.indices*sizeof(std::int32_t);
            if(impl_->optimization.resource_reuse&&!same_sources){draw.batch_sources.clear();draw.batch_sources.reserve(batch.subranges.size());for(const auto& range:batch.subranges){const auto& c=frame.commands[range.command];draw.batch_sources.emplace_back(c.solid,c.surface);}}
            if(view_id==0&&impl_->postprocess_renderer)impl_->postprocess_renderer->mark_tracked_mesh(draw.node);
            if(draw.node->get_mesh()!=batch_mesh){draw.node->set_mesh(batch_mesh);if(impl_->postprocess_renderer)impl_->postprocess_renderer->mark_mesh_changed(draw.node,TrackPostprocess::Geometry|TrackPostprocess::Material|TrackPostprocess::Bounds);}
            if(impl_->postprocess_renderer)impl_->postprocess_renderer->mark_tracked_material(draw.material);
            const bool roadshine_enabled=settings.enabled(port::track::EffectId::Roadshine);
            if(command.effect.second_pass_320){if(view_id==0)impl_->roadshine_active|=roadshine_enabled;if(!cached.initialized||cached.roadshine_enabled!=roadshine_enabled){set_parameter(draw.material,"effect_roadshine_enabled",roadshine_enabled);cached.roadshine_enabled=roadshine_enabled;}}
            const auto transform=(command.identity||batch.baked_world)?godot::Transform3D():host_matrix(command.matrix);
            const bool matrix_changed=!cached.initialized||draw.node->get_transform()!=transform;
            if(matrix_changed){draw.node->set_transform(transform);cached.matrix=command.matrix;if(impl_->postprocess_renderer)impl_->postprocess_renderer->mark_mesh_changed(draw.node,TrackPostprocess::Transform);}
            if(hlod&&(matrix_changed||ranges_changed||cached.hlod_begin!=range->begin||cached.hlod_end!=range->end)){
                const auto anchor=host(impl_->static_lods->instances.at(command.instance).anchor);
                const auto center=transform.affine_inverse().xform(anchor);
                const auto bounds=batch_mesh->get_aabb();const auto low=bounds.position,high=bounds.get_end();
                godot::Vector3 extents;for(int axis=0;axis<3;++axis)extents[axis]=std::max(std::abs(low[axis]-center[axis]),std::abs(high[axis]-center[axis]));
                draw.node->set_custom_aabb({center-extents,extents*2});
                draw.node->set_visibility_range_begin(range->begin);draw.node->set_visibility_range_end(range->end);
                cached.hlod_begin=range->begin;cached.hlod_end=range->end;
                draw.node->set_visibility_range_begin_margin(0);draw.node->set_visibility_range_end_margin(0);draw.node->set_visibility_range_fade_mode(godot::GeometryInstance3D::VISIBILITY_RANGE_FADE_DISABLED);
                draw.node->set_meta("hp2_hlod_instance",int64_t(command.instance));draw.node->set_meta("hp2_hlod_slot",int64_t(command.lod));draw.node->set_meta("hp2_hlod_view",view_id);
                if(impl_->postprocess_renderer)impl_->postprocess_renderer->mark_mesh_changed(draw.node);
            }
            if(command.effect.flags&0x1c0){
                const auto local=math?godot::Vector3(math->local_camera[0],math->local_camera[1],math->local_camera[2]):transform.affine_inverse().xform(camera_position);
                if(!cached.initialized||local!=cached.local_camera){set_parameter(draw.material,"source_camera",godot::Vector3(local.z,local.x,local.y));cached.local_camera=local;}
                if(!cached.initialized||cached.lights!=command.light_context){
                    for(int i=0;i<7;++i)set_parameter(draw.material,impl_->light_names[i],math?godot::Vector4(math->lights[i][0],math->lights[i][1],math->lights[i][2],math->lights[i][3]):godot::Vector4(command.light_context[i*4],command.light_context[i*4+1],command.light_context[i*4+2],command.light_context[i*4+3]));
                    cached.lights=command.light_context;
                }
            }
            if(command.effect.matrix_pack&&(!cached.initialized||cached.near_z!=camera->get_near()||cached.far_z!=camera->get_far())){
                cached.near_z=camera->get_near();cached.far_z=camera->get_far();set_parameter(draw.material,"depth_near",cached.near_z);set_parameter(draw.material,"depth_far",cached.far_z);
            }
            if(view_id==0&&impl_->occlusion_pool&&eligible_track_opaque_geometry(command)&&instance_count==1)impl_->occlusion_pool->submit(rank,batch_mesh,transform);
            const float origin_depth=math?math->origin_depth:(draw.node->get_global_position()-camera->get_global_position()).dot(direction);
            const float draw_offset=math?math->sorting_offset:origin_depth-float(frame.commands.size()-rank)*4096.f;
            if(!(impl_->optimization.resource_reuse&&view_id==0&&command.effect.alpha_pass)&&(!impl_->optimization.resource_reuse||!draw.sorting_valid||draw.sorting_offset!=draw_offset)){draw.node->set_sorting_offset(draw_offset);draw.sorting_offset=draw_offset;draw.sorting_valid=true;++impl_->sorting_updates;if(view_id==0&&impl_->postprocess_renderer)impl_->postprocess_renderer->notify_mesh_sorting(draw.node,draw_offset);}
            const bool reflect=view_id==0&&command.effect.environment_pass&&settings.enabled(port::track::EffectId::Environment)&&impl_->reflection_renderer&&impl_->reflection_renderer->ready();
            if(reflect){
                auto& env=draw.reflection_material;auto& state=draw.reflection_state;const auto texture=impl_->reflection_renderer->texture();
                if(env.is_null()){
                    const auto code=track_environment_shader_source(command.effect);auto shader=impl_->shaders.find(code);if(shader==impl_->shaders.end()){godot::Ref<godot::Shader> value;value.instantiate();value->set_code(text(code));shader=impl_->shaders.emplace(code,value).first;}
                    env.instantiate();env->set_shader(shader->second);env->set_render_priority(draw.material->get_render_priority());
                    const auto& blocks=impl_->scene->materials.at(surface.material_hash).blocks;for(int i=0;i<4;++i)env->set_shader_parameter(text("material_"+std::to_string(350+i)),godot::Vector4(blocks[16+i*4],blocks[17+i*4],blocks[18+i*4],blocks[19+i*4]));
                    draw.reflection_node=memnew(godot::MeshInstance3D);draw.reflection_node->set_layer_mask(1);draw.reflection_node->set_mesh(batch_mesh);draw.reflection_node->set_material_override(env);draw.reflection_node->set_sorting_use_aabb_center(false);draw.reflection_node->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);add_child(draw.reflection_node);impl_->nodes.push_back(draw.reflection_node);
                }
                if(!state.initialized||state.matrix!=command.matrix){draw.reflection_node->set_transform(transform);state.matrix=command.matrix;}
                if(hlod){draw.reflection_node->set_custom_aabb(draw.node->get_custom_aabb());draw.reflection_node->set_visibility_range_begin(range->begin);draw.reflection_node->set_visibility_range_end(range->end);draw.reflection_node->set_visibility_range_begin_margin(0);draw.reflection_node->set_visibility_range_end_margin(0);draw.reflection_node->set_visibility_range_fade_mode(godot::GeometryInstance3D::VISIBILITY_RANGE_FADE_DISABLED);}
                if(draw.reflection_texture!=texture){env->set_shader_parameter("base_texture",texture);draw.reflection_texture=texture;}
                const auto local=math?godot::Vector3(math->local_camera[0],math->local_camera[1],math->local_camera[2]):transform.affine_inverse().xform(camera_position);if(!state.initialized||state.local_camera!=local){env->set_shader_parameter("source_camera",godot::Vector3(local.z,local.x,local.y));state.local_camera=local;}
                if(!state.initialized||state.lights!=command.light_context){for(int i=0;i<7;++i)env->set_shader_parameter(impl_->light_names[i],math?godot::Vector4(math->lights[i][0],math->lights[i][1],math->lights[i][2],math->lights[i][3]):godot::Vector4(command.light_context[i*4],command.light_context[i*4+1],command.light_context[i*4+2],command.light_context[i*4+3]));state.lights=command.light_context;}
                const auto& hack=impl_->reflection_renderer->hack_matrix();auto model=command.matrix;if(command.identity){model={};model[0]=model[5]=model[10]=model[15]=1;}
                std::array<float,9> rows{};if(math)rows=math->reflection_rows;else for(int row=0;row<3;++row)for(int axis=0;axis<3;++axis)rows[row*3+axis]=model[row*4]*hack[axis]+model[row*4+1]*hack[4+axis]+model[row*4+2]*hack[8+axis];
                if(!state.initialized||draw.reflection_rows!=rows){for(int row=0;row<3;++row)env->set_shader_parameter(text("environment_row"+std::to_string(row)),godot::Vector3(rows[row*3],rows[row*3+1],rows[row*3+2]));draw.reflection_rows=rows;}
                if(command.effect.matrix_pack&&(!state.initialized||state.near_z!=camera->get_near()||state.far_z!=camera->get_far())){env->set_shader_parameter("depth_near",camera->get_near());env->set_shader_parameter("depth_far",camera->get_far());state.near_z=camera->get_near();state.far_z=camera->get_far();}
                draw.reflection_node->set_sorting_offset(math?math->reflection_sorting_offset:origin_depth-(float(frame.commands.size()-rank)-.5f)*4096.f);draw.reflection_node->show();state.initialized=true;
            }else if(draw.reflection_node)draw.reflection_node->hide();
            if(instance_count>1){
                std::vector<godot::Transform3D> transforms;transforms.reserve(instance_count);for(std::size_t i=0;i<instance_count;++i){const auto& c=frame.commands[rank+i];transforms.push_back(c.identity?godot::Transform3D():host_matrix(c.matrix));}
                auto* node=impl_->instance_pool->submit(view_id,rank,batch_mesh,draw.material,transforms,1u<<view_id);
                const float origin=(get_global_position()-camera_position).dot(direction);const float offset=origin-float(frame.commands.size()-rank)*4096.f;
                node->set_sorting_use_aabb_center(false);if(node->get_sorting_offset()!=offset)node->set_sorting_offset(offset);
                if(view_id==0){impl_->main_instance_nodes.push_back(node);if(impl_->postprocess_renderer)impl_->postprocess_renderer->mark_instance_batch(node);}
                if(cached.visible){draw.node->hide();cached.visible=false;if(impl_->postprocess_renderer)impl_->postprocess_renderer->mark_mesh_changed(draw.node,TrackPostprocess::Visibility);}
            }else if(!cached.visible){draw.node->set_visible(true);cached.visible=true;if(impl_->postprocess_renderer)impl_->postprocess_renderer->mark_mesh_changed(draw.node,TrackPostprocess::Visibility);}
            if(math&&math->pose.valid)cached.pose_cache=math->pose;cached.initialized=true;draw.used_frame=impl_->frame_number;visible.push_back(&draw);
            ++rank;if(view_id==0)impl_->triangles+=batch.indices/3*instance_count;
        }
        for(auto* draw:impl_->view_draws[view_id])if(draw->used_frame!=impl_->frame_number){draw->node->set_visible(false);draw->state.visible=false;if(impl_->postprocess_renderer)impl_->postprocess_renderer->mark_mesh_changed(draw->node,TrackPostprocess::Visibility);if(draw->reflection_node)draw->reflection_node->hide();}
        impl_->view_draws[view_id]=std::move(visible);if(impl_->instance_pool)impl_->instance_pool->end_view(view_id);if(view_id==0&&impl_->occlusion_pool)impl_->occlusion_pool->end_frame();
        impl_->submission_times[view_id][2]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-view_begin).count();
}

godot::Dictionary Ps2TrackRenderer::update_view(const godot::Vector3& camera_position){
    const auto failure=catch_all([&]{
        if(!impl_->ok||!impl_->math)return;
        auto* viewport=get_viewport();auto* camera=viewport?viewport->get_camera_3d():nullptr;
        if(!camera)throw std::runtime_error("Shared track rendering requires an active Camera3D");
        // Capture-only geometry uses layers2..7; the main view and postprocess
        // colour camera consume layer1 and the caller's remaining world layers.
        if(camera->get_cull_mask()!=0)camera->set_cull_mask(camera->get_cull_mask()&~0x7eu);
        port::track::FrameView view;view.position={float(camera_position.z),float(camera_position.x),float(camera_position.y)};
        const auto direction=-camera->get_global_transform().basis.get_column(2);
        view.direction={float(direction.z),float(direction.x),float(direction.y)};
        auto& settings=port::track::shared_render_settings();
        const auto initial_projection=camera->get_camera_projection();
        const auto horizontal_angle=std::uint16_t(std::lround(std::atan(1.0/double(initial_projection[0][0]))*(65536.0/3.14159265358979323846)));
        const auto depth=port::track::original_view_depth(*impl_->math,1,impl_->camera_mode,horizontal_angle,.5f,10000.f,0,settings.distance_override?settings.distance:0);
        if(camera->get_near()!=depth.near_z)camera->set_near(depth.near_z);if(camera->get_far()!=depth.far_z)camera->set_far(depth.far_z);
        impl_->effective_near=depth.near_z;impl_->effective_far=depth.far_z;
        view.near_z=depth.near_z;view.far_z=depth.far_z;view.frame_counter=std::uint32_t(impl_->frame_number);
        view.visual_timestep=impl_->visual_timestep;impl_->visual_timestep=0;
        view.environment_capture_valid=impl_->visual_state_valid;view.environment_position=impl_->environment_position;
        view.post_controls.DrawFog=settings.enabled(port::track::EffectId::Fog);view.post_controls.DrawDepthOfField=settings.enabled(port::track::EffectId::DepthOfField);
        const auto projection=camera->get_camera_projection();
        const auto clip=projection*godot::Projection(camera->get_global_transform().affine_inverse());
        const int columns[]{2,0,1,3};for(int c=0;c<4;++c)for(int r=0;r<4;++r)view.clip_matrix[c*4+r]=clip[columns[c]][r];
        view.focal_h=port::track::original_focal_h(*impl_->math,horizontal_angle);
        const auto native_begin=std::chrono::steady_clock::now();
        impl_->frame_view=view;
        auto frame=(impl_->static_lods&&impl_->show_lod)?impl_->runtime.prepare_hlod_frame(view,*impl_->math,*impl_->static_lods):impl_->runtime.prepare_frame(view,*impl_->math);
        const auto submit_begin=std::chrono::steady_clock::now();
        impl_->native_frame_ms=std::chrono::duration<double,std::milli>(submit_begin-native_begin).count();
        ++impl_->frame_number;impl_->submission_counts.clear();impl_->submission_times.clear();impl_->instance_groups=impl_->instanced_commands=impl_->instance_saved_draws=0;impl_->active_viewport_rids.clear();impl_->active_viewport_rids.push_back(viewport->get_viewport_rid());
        impl_->packet_digest.clear();impl_->frame_view=view;impl_->triangles=0;impl_->roadshine_active=false;
        impl_->main_alpha_draws.clear();const auto main_begin=std::chrono::steady_clock::now();submit_frame(frame,camera,0);impl_->main_submit_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-main_begin).count();impl_->frame=std::move(frame);
        const auto environment_begin=std::chrono::steady_clock::now();
        if(!impl_->environment_renderer){EnvironmentResourceLookup lookup;
            lookup.mesh=[this](std::size_t solid,std::size_t surface){return acquire_source_mesh(solid,surface);};
            lookup.texture=[this](std::uint32_t hash)->godot::Ref<godot::Texture2D>{return impl_->textures.at(hash);};
            lookup.mip=[this](std::uint32_t hash,unsigned level)->godot::Ref<godot::Texture2D>{return impl_->mipmaps.at(hash).at(level-1);};
            impl_->environment_renderer=std::make_unique<TrackEnvironmentRenderer>(*this,std::move(lookup));
        }
        impl_->environment_frame=impl_->environment_renderer->update(camera,impl_->scene,*impl_->math,std::uint32_t(impl_->frame_number),0,settings.enabled(port::track::EffectId::Sky));
        if(impl_->visual_state_valid&&impl_->environment){if(!impl_->shadow_renderer)impl_->shadow_renderer=std::make_unique<TrackShadowRenderer>(*this);
            impl_->shadow_frame=impl_->shadow_renderer->update(impl_->visual_state,*impl_->scene,*impl_->math,impl_->environment->topology,settings.enabled(port::track::EffectId::Shadows));
        }else if(impl_->shadow_renderer){impl_->shadow_renderer->clear();impl_->shadow_frame={};}
        impl_->environment_submit_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-environment_begin).count();
        const auto reflection_begin=std::chrono::steady_clock::now();
        if(is_visible_in_tree()&&godot::DisplayServer::get_singleton()->get_name()!="headless"&&impl_->visual_state_valid&&settings.enabled(port::track::EffectId::Environment)){
            if(!impl_->reflection_renderer)impl_->reflection_renderer=std::make_unique<TrackReflectionRenderer>(this,[this](const TrackReflectionRenderer::CaptureView& capture,std::string& error){
                error=catch_all([&]{
                    port::track::FrameView sub;const auto* camera=capture.camera;const auto position=camera->get_global_position(),direction=-camera->get_global_transform().basis.get_column(2);
                    sub.position={float(position.z),float(position.x),float(position.y)};sub.direction={float(direction.z),float(direction.x),float(direction.y)};
                    sub.view_id=capture.view_id;sub.exclude_flags=capture.exclude_flags;sub.near_z=capture.native.near_z;sub.far_z=capture.native.far_z;
                    const auto projection=camera->get_camera_projection();const auto clip=projection*godot::Projection(camera->get_global_transform().affine_inverse());
                    const int columns[]{2,0,1,3};for(int c=0;c<4;++c)for(int r=0;r<4;++r)sub.clip_matrix[c*4+r]=clip[columns[c]][r];sub.focal_h=port::track::original_focal_h(*impl_->math,capture.native.horizontal_fov);
                    const auto selection_begin=std::chrono::steady_clock::now();auto frame=(impl_->static_lods&&impl_->show_lod)?impl_->runtime.prepare_hlod_frame(sub,*impl_->math,*impl_->static_lods):impl_->runtime.prepare_frame(sub,*impl_->math);const auto selection_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-selection_begin).count();submit_frame(frame,capture.camera,capture.face_index+1);impl_->submission_times[capture.face_index+1][0]=selection_ms;
                    auto& sky=impl_->reflection_skies[capture.face_index];if(!sky){EnvironmentResourceLookup lookup;
                        lookup.mesh=[this](std::size_t solid,std::size_t surface){return acquire_source_mesh(solid,surface);};
                        lookup.texture=[this](std::uint32_t hash)->godot::Ref<godot::Texture2D>{return impl_->textures.at(hash);};
                        lookup.mip=[this](std::uint32_t hash,unsigned level)->godot::Ref<godot::Texture2D>{return impl_->mipmaps.at(hash).at(level-1);};sky=std::make_unique<TrackEnvironmentRenderer>(*this,std::move(lookup));}
                    sky->update(capture.camera,impl_->scene,*impl_->math,std::uint32_t(impl_->frame_number),0,port::track::shared_render_settings().enabled(port::track::EffectId::Sky),capture.view_id,capture.host_layer);
                });return error.empty();
            });
            impl_->reflection_renderer->update(view.position,impl_->environment_position,std::uint32_t(impl_->frame_number),port::track::original_focal_h(*impl_->math,0x75e4));
        }else if(impl_->reflection_renderer)impl_->reflection_renderer->clear();
        impl_->reflection_submit_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-reflection_begin).count();
        const auto post_begin=std::chrono::steady_clock::now();
        if(is_visible_in_tree()&&godot::DisplayServer::get_singleton()->get_name()!="headless"){
            if(!impl_->postprocess_renderer)impl_->postprocess_renderer=std::make_unique<TrackPostprocess>();
            if(impl_->postprocess_renderer->needs_structure_observer()){auto* tree=get_tree();const auto observer=callable_mp(this,&Ps2TrackRenderer::depth_source_structure_changed);
            if(!tree->is_connected("node_added",observer))tree->connect("node_added",observer);
            if(!tree->is_connected("node_removed",observer))tree->connect("node_removed",observer);
            impl_->postprocess_renderer->set_structure_observer(true,callable_mp(this,&Ps2TrackRenderer::depth_source_order_changed));}
            impl_->postprocess_renderer->set_resource_reuse(impl_->optimization.resource_reuse);impl_->postprocess_renderer->set_occlusion_enabled(impl_->optimization.occlusion);
            for(auto* node:impl_->main_instance_nodes)impl_->postprocess_renderer->mark_instance_batch(node);
            const auto& weather=impl_->frame.weather;
            const bool active=godot::RenderingServer::get_singleton()->get_current_rendering_method()!="gl_compatibility"||!impl_->scene->particles.emitters.empty()||!impl_->frame.postprocess.passes.empty()||(settings.enabled(port::track::EffectId::Sun)&&impl_->scene->environment.sun.available);
            impl_->postprocess_renderer->update(this,camera,{weather.fog_colour,weather.fog_start,weather.fog},depth.near_z,depth.far_z,view.post_controls,active,view.camera_focal_distance,view.camera_depth_of_field,&impl_->frame.postprocess);
            impl_->post_submit_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-post_begin).count();
            const auto sun_begin_time=std::chrono::steady_clock::now();
            if(!impl_->sun_renderer){impl_->sun_renderer=std::make_unique<TrackSunRenderer>(*this,
                [this](std::uint32_t hash)->godot::Ref<godot::Texture2D>{return impl_->textures.at(hash);},
                [this](std::uint32_t hash,unsigned level)->godot::Ref<godot::Texture2D>{return impl_->mipmaps.at(hash).at(level-1);});}
            if(!impl_->readback_connected){godot::RenderingServer::get_singleton()->connect("frame_post_draw",godot::Callable(this,"_complete_visual_readback"));impl_->readback_connected=true;}
            impl_->sun_frame=impl_->sun_renderer->update(camera,*impl_->scene,*impl_->math,impl_->postprocess_renderer->get_depth_texture(),settings.enabled(port::track::EffectId::Sun));
            impl_->sun_submit_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-sun_begin_time).count();
            const auto particle_begin_time=std::chrono::steady_clock::now();
            if(!impl_->particle_renderer){impl_->particle_renderer=std::make_unique<TrackParticleRenderer>(*this,
                [this](std::uint32_t hash)->godot::Ref<godot::Texture2D>{return impl_->textures.at(hash);},
                [this](std::uint32_t hash,unsigned level)->godot::Ref<godot::Texture2D>{return impl_->mipmaps.at(hash).at(level-1);},impl_->particle_seed);impl_->particle_renderer->set_material_callbacks(
                [this](const godot::Ref<godot::ShaderMaterial>& material){if(impl_->depth_param_cache&&impl_->postprocess_renderer)impl_->postprocess_renderer->mark_tracked_material(material);},
                [this](const godot::Ref<godot::ShaderMaterial>& material,const godot::StringName& name,const godot::Variant& value){if(impl_->depth_param_cache&&impl_->postprocess_renderer)impl_->postprocess_renderer->notify_material_parameter(material,name,value);});impl_->particle_renderer->set_mesh_reuse(impl_->optimization.particle_mesh_reuse);impl_->particle_renderer->set_mesh_change_callback([this](godot::MeshInstance3D* node){if(impl_->postprocess_renderer)impl_->postprocess_renderer->mark_mesh_changed(node);});}
            impl_->particle_renderer->set_driving_source(impl_->driving_effect_source.ptr());
            impl_->particle_frame=impl_->particle_renderer->update(impl_->scene,*impl_->math,view,view.visual_timestep,impl_->postprocess_renderer->get_depth_texture(),settings.enabled(port::track::EffectId::Particles),1,camera);
            impl_->particle_submit_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-particle_begin_time).count();
            // Retail eDisplayFrame enqueues scenery, Sun, then WorldEffects.
            // Recreate insertion order before the original reversed radix ordering.
            std::vector<std::size_t> input;for(std::size_t i=0;i<impl_->frame.commands.size();++i)if(impl_->frame.commands[i].effect.alpha_pass)input.push_back(i);
            std::sort(input.begin(),input.end(),[&](auto a,auto b){return impl_->frame.commands[a].original_order<impl_->frame.commands[b].original_order;});
            std::vector<port::track::AlphaKey> keys;std::vector<std::size_t> command_key(impl_->frame.commands.size());
            for(auto i:input){command_key[i]=keys.size();const auto& c=impl_->frame.commands[i];keys.push_back({c.effect.rendering_order,c.depth});}
            const auto sun_begin=keys.size();keys.insert(keys.end(),impl_->sun_frame.alpha_keys.begin(),impl_->sun_frame.alpha_keys.end());
            const auto particle_begin=keys.size();keys.insert(keys.end(),impl_->particle_frame.alpha_keys.begin(),impl_->particle_frame.alpha_keys.end());
            const auto order=port::track::alpha_order_001055B0(keys);std::vector<std::size_t> ranks(keys.size());for(std::size_t i=0;i<order.size();++i)ranks[order[i]]=i;
            for(const auto& entry:impl_->main_alpha_draws){auto& draw=impl_->draws.at(entry.second);const float origin=(draw.node->get_global_position()-camera->get_global_position()).dot(direction);const float offset=origin-float(keys.size()-ranks[command_key[entry.first]])*4096.f;if(!impl_->optimization.resource_reuse||!draw.sorting_valid||draw.sorting_offset!=offset){draw.node->set_sorting_offset(offset);draw.sorting_offset=offset;draw.sorting_valid=true;++impl_->sorting_updates;impl_->postprocess_renderer->notify_mesh_sorting(draw.node,offset);}if(draw.reflection_node)draw.reflection_node->set_sorting_offset(offset+2048.f);}
            const float host_origin=(get_global_position()-camera->get_global_position()).dot(direction);
            impl_->sun_renderer->set_alpha_ranks(std::vector<std::size_t>(ranks.begin()+sun_begin,ranks.begin()+particle_begin),ranks.size(),host_origin);
            impl_->particle_renderer->set_alpha_ranks(std::vector<std::size_t>(ranks.begin()+particle_begin,ranks.end()),ranks.size(),host_origin);
        }
        trim_draw_cache();
        if(impl_->postprocess_renderer)impl_->active_viewport_rids.append_array(impl_->postprocess_renderer->scheduled_viewports());
        if(impl_->sun_renderer)impl_->active_viewport_rids.append_array(impl_->sun_renderer->scheduled_viewports());
        if(impl_->reflection_renderer)impl_->active_viewport_rids.append_array(impl_->reflection_renderer->scheduled_viewports());
        if(impl_->particle_renderer)impl_->particle_renderer->commit_frame();
        impl_->host_submit_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-submit_begin).count();
    });
    if(!failure.empty()&&impl_->particle_renderer)impl_->particle_renderer->abort_frame();
    // A frame dependency gap is not an asset unload. Keep the scene/topology
    // and submitted scenery so the next valid frame can recover in place.
    if(!failure.empty()&&failure!=impl_->render_error)godot::UtilityFunctions::push_warning(text("Track retained after frame render gap: "+failure));
    impl_->render_error=failure;
    return get_state();
}

void Ps2TrackRenderer::depth_source_order_changed(){if(impl_->postprocess_renderer)impl_->postprocess_renderer->notify_source_order();}
void Ps2TrackRenderer::depth_source_structure_changed(godot::Node* node){if(impl_->postprocess_renderer)impl_->postprocess_renderer->notify_scene_node(node);}
void Ps2TrackRenderer::complete_visual_readback(){
    const auto begin=std::chrono::steady_clock::now();
    const auto failure=catch_all([&]{if(impl_->sun_renderer)impl_->sun_renderer->complete_readback();});
    impl_->sun_readback_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    if(!failure.empty()&&failure!=impl_->readback_error)godot::UtilityFunctions::push_warning(text("Track retained after visual readback gap: "+failure));
    impl_->readback_error=failure;
}

void Ps2TrackRenderer::set_effect_enabled(const godot::String& id,bool enabled){
    const auto failure=catch_all([&]{if(!performance_controls())throw std::logic_error("Effect overrides are available in Debug only");port::track::shared_render_settings().set(id.utf8().get_data(),enabled);});if(!failure.empty())impl_->error=failure;
}
void Ps2TrackRenderer::set_diagnostics_level(const godot::String& level){
    const auto failure=catch_all([&]{int value=level=="off"?0:level=="summary"?1:level=="detailed"?2:-1;if(value<0)throw std::invalid_argument("Expected off, summary or detailed diagnostics");impl_->diagnostics_level=value;if(value!=2)impl_->batch_breaks.clear();});if(!failure.empty())impl_->error=failure;
}
void Ps2TrackRenderer::set_render_props(bool enabled){const auto failure=catch_all([&]{if(!performance_controls())throw std::logic_error("Props visibility is available in Debug only");port::track::shared_render_settings().set_props(enabled);});if(!failure.empty())impl_->error=failure;}
void Ps2TrackRenderer::restore_render_defaults(){if(performance_controls())port::track::shared_render_settings().restore();}
void Ps2TrackRenderer::set_visibility_override(bool enabled,double distance){const auto failure=catch_all([&]{if(!performance_controls())throw std::logic_error("Visibility overrides are available in Debug only");port::track::shared_render_settings().set_distance(enabled,float(distance));});if(!failure.empty())impl_->error=failure;}
void Ps2TrackRenderer::set_camera_mode(int original_type){if(original_type!=1&&original_type!=2&&original_type!=5){impl_->error="Unsupported original camera mover type";return;}impl_->camera_mode=original_type;}
void Ps2TrackRenderer::set_visual_state(const godot::Dictionary& state){
    impl_->visual_state_valid=false;impl_->visual_state=state;
    const auto failure=catch_all([&]{if(!bool(state.get("available",false)))return;
        if(!state.has("environment_position_native")||state["environment_position_native"].get_type()!=godot::Variant::VECTOR3)throw std::invalid_argument("Original car environment capture position is absent");
        const godot::Vector3 p=state["environment_position_native"];if(!p.is_finite())throw std::invalid_argument("Nonfinite original car capture position");
        impl_->environment_position={float(p.x),float(p.y),float(p.z)};impl_->visual_state_valid=true;
    });if(!failure.empty())impl_->error=failure;
}
void Ps2TrackRenderer::set_driving_effect_source(const godot::Ref<Ps2DrivingRuntime>& source){
    impl_->driving_effect_source=source;
    if(impl_->particle_renderer)impl_->particle_renderer->set_driving_source(source.ptr());
}
void Ps2TrackRenderer::set_visual_timestep(double seconds){
    const auto failure=catch_all([&]{if(!std::isfinite(seconds)||seconds<0||seconds>std::numeric_limits<float>::max())throw std::invalid_argument("Visual timestep must be finite, nonnegative seconds");impl_->visual_timestep=float(seconds);});
    if(!failure.empty())impl_->error=failure;
}

godot::Dictionary Ps2TrackRenderer::get_environment_capture()const{
    godot::Dictionary result;const auto& settings=port::track::shared_render_settings();result["enabled"]=settings.enabled(port::track::EffectId::Environment);
    const bool ready=impl_->reflection_renderer&&impl_->reflection_renderer->ready();result["available"]=ready;
    result["diagnostic"]=impl_->reflection_renderer?text(impl_->reflection_renderer->diagnostic()):godot::String("Original car environment capture is unavailable");
    if(ready){result["texture"]=impl_->reflection_renderer->texture();godot::PackedFloat32Array matrix;matrix.resize(16);std::memcpy(matrix.ptrw(),impl_->reflection_renderer->hack_matrix().data(),64);result["hack_matrix_native"]=matrix;}
    return result;
}
godot::Dictionary Ps2TrackRenderer::get_render_targets()const{
    godot::Dictionary result;
    if(impl_->postprocess_renderer){result["colour"]=impl_->postprocess_renderer->get_colour_texture();result["native_depth"]=impl_->postprocess_renderer->get_depth_texture();}
    return result;
}

godot::Array Ps2TrackRenderer::get_effects()const{
    godot::Array result;const auto& settings=port::track::shared_render_settings();
    for(const auto& descriptor:port::track::effect_descriptions){godot::Dictionary row;row["id"]=descriptor.key;row["name"]=descriptor.name;row["category"]=descriptor.category;row["enabled"]=settings.enabled(descriptor.id);
        const bool available=descriptor.id==port::track::EffectId::Roadshine||descriptor.id==port::track::EffectId::Sky||descriptor.id==port::track::EffectId::Fog||descriptor.id==port::track::EffectId::DepthOfField||descriptor.id==port::track::EffectId::Environment||descriptor.id==port::track::EffectId::Shadows||descriptor.id==port::track::EffectId::Sun||descriptor.id==port::track::EffectId::Particles;
        const bool active=descriptor.id==port::track::EffectId::Particles?impl_->particle_frame.visible>0:descriptor.id==port::track::EffectId::Sun?impl_->sun_frame.quads>0:descriptor.id==port::track::EffectId::Sky?impl_->environment_frame.sky_draws>0:descriptor.id==port::track::EffectId::Fog?impl_->postprocess_renderer&&impl_->postprocess_renderer->fog_active():descriptor.id==port::track::EffectId::DepthOfField?impl_->postprocess_renderer&&impl_->postprocess_renderer->focus_active():descriptor.id==port::track::EffectId::Environment?settings.enabled(descriptor.id)&&impl_->reflection_renderer&&impl_->reflection_renderer->ready():descriptor.id==port::track::EffectId::Shadows?impl_->shadow_frame.strips>0:impl_->roadshine_active;
        std::string reason=available?(impl_->ok?"Original activation and asset conditions apply":"Load a track to inspect effects"):"Original stage integration is incomplete";
        if(impl_->ok&&descriptor.id==port::track::EffectId::Environment)reason=impl_->reflection_renderer?impl_->reflection_renderer->diagnostic():"Requires the original car capture origin; inactive in free browsing without a car";
        if(impl_->ok&&descriptor.id==port::track::EffectId::Shadows)reason=impl_->shadow_frame.gaps.empty()?"Requires a live car pose, original CV, shadow assets and receiving topology":impl_->shadow_frame.gaps.front();
        if(impl_->ok&&descriptor.id==port::track::EffectId::Sun)reason=impl_->sun_frame.gaps.empty()?"Original cached 32x32 depth visibility and asset conditions apply":impl_->sun_frame.gaps.front();
        if(impl_->ok&&descriptor.id==port::track::EffectId::Particles)reason=impl_->particle_frame.gaps.empty()?"Original static WorldEffects emitters; FrameEffects and unsorted additive branch remain incomplete":impl_->particle_frame.gaps.front();
        row["available"]=available&&impl_->ok;row["active"]=available&&impl_->ok&&active;row["reason"]=text(reason);result.push_back(row);}
    godot::Dictionary lut;lut["id"]="colour_lut";lut["name"]="LUT / colour grading";lut["category"]="Post processing";lut["enabled"]=false;lut["available"]=false;lut["active"]=false;lut["reason"]="Fog/Focus lookup tables are not an independent colour-grading effect; original colour-grading path is not established";result.push_back(lut);return result;
}
godot::Dictionary Ps2TrackRenderer::get_render_controls()const{
    godot::Dictionary result;result["submit_math_enabled"]=impl_->submit_math_enabled;result["render_prepare_workers"]=impl_->prepare_workers;result["render_thread_requested"]=requested_render_thread();result["render_thread_actual"]=godot::RenderingServer::get_singleton()->is_on_render_thread()?"safe":"separate";result["port_ui_optimized_debug"]=bool(HP2_DEBUG_PORT_UI_OPTIMIZED);result["render_selection_optimized_debug"]=bool(HP2_DEBUG_RENDER_SELECTION_OPTIMIZED);result["render_host_optimized_debug"]=bool(HP2_DEBUG_RENDER_HOST_OPTIMIZED);const auto& settings=port::track::shared_render_settings();result["loaded_instances"]=int64_t(impl_->scene?impl_->scene->instances.size():0);result["considered_instances"]=int64_t(impl_->frame.considered_instances);result["visible_instances"]=int64_t(impl_->frame.visible_instances);result["loaded_sections"]=int64_t(impl_->scene?impl_->scene->sections.size():0);result["selected_sections"]=int64_t(impl_->frame.sections.size());result["debug_controls"]=bool(HP2_DEBUG_BUILD);result["performance_controls"]=performance_controls();result["effects"]=get_effects();result["render_ready"]=impl_->ok;result["render_props"]=settings.render_props;result["distance_override"]=settings.distance_override;result["distance"]=settings.distance;result["effective_near"]=impl_->effective_near;result["effective_far"]=impl_->effective_far;result["original_far"]=10000.f;result["original_default_near"]=.5f;result["original_default_far"]=10000.f;result["camera_depth_source"]="Port main-view adapter using original Camera constructor defaults; full camera-key gameflow unresolved";result["camera_mover_type"]=impl_->camera_mode;result["force_far_override"]=settings.distance_override?settings.distance:0.f;result["original_logical_screen_width"]=512;result["native_selection_bypassed"]=impl_->show_lod;result["native_frame_ms"]=impl_->native_frame_ms;result["host_submit_ms"]=impl_->host_submit_ms;return result;
}

godot::Dictionary Ps2TrackRenderer::frame_summary()const{
    if(impl_->ok&&impl_->packet_digest.empty()){
        std::uint64_t digest=14695981039346656037ull;
        const auto hash=[&](const auto& value){const auto* bytes=reinterpret_cast<const unsigned char*>(&value);for(std::size_t i=0;i<sizeof(value);++i){digest^=bytes[i];digest*=1099511628211ull;}};
        const auto& view=impl_->frame_view;const auto& frame=impl_->frame;const auto& scene=*impl_->scene;
        hash(impl_->number);hash(view.position);hash(view.direction);hash(view.clip_matrix);hash(view.focal_h);hash(frame.zone);for(auto section:frame.sections)hash(section);
        hash(view.near_z);hash(view.far_z);hash(frame.weather.rain);hash(frame.weather.fog);hash(frame.weather.snow);hash(frame.weather.wind);hash(frame.weather.fog_start);hash(frame.weather.fog_colour);
        hash(frame.postprocess.fog.enabled);hash(frame.postprocess.fog.linear);hash(frame.postprocess.focus.enabled);hash(frame.postprocess.focus.linear);
        for(const auto& pass:frame.postprocess.passes){hash(pass.stage);hash(pass.render_stage);const auto blend=pass.blend.register_value();hash(blend);hash(pass.write_rgb);hash(pass.write_alpha);hash(pass.source_width);hash(pass.source_height);hash(pass.target_width);hash(pass.target_height);hash(pass.source_offset_fixed16);hash(pass.tex1);hash(pass.test);hash(pass.zbuf);}
        for(const auto& command:frame.commands){
            const auto& surface=scene.geometry.solids.at(command.solid).surfaces.at(command.surface);
            hash(command.instance);hash(command.solid);hash(command.surface);hash(command.lod);hash(command.visibility);hash(command.original_order);hash(command.matrix);hash(command.light_context);hash(command.identity);hash(command.has_light_context);hash(command.depth);
            hash(command.effect.flags);hash(command.effect.alpha_pass);hash(command.effect.rewrite_alpha);hash(command.effect.matrix_pack);hash(command.effect.rendering_order);hash(command.effect.texture.alpha_register);hash(command.effect.texture.tex1_register);hash(command.effect.test.test_register);hash(command.effect.test.write_depth);hash(surface.texture_hash);hash(surface.material_hash);
            const auto native_passes=port::track::pass_sequence(command.effect,surface.texture_hash);for(const auto& pass:native_passes){hash(pass.kind);hash(pass.gs_context);hash(pass.alpha_register);hash(pass.texture_hash);hash(pass.state_available);}
            const auto material_data=scene.materials.find(surface.material_hash);if(material_data!=scene.materials.end())hash(material_data->second.blocks);
        }
        std::ostringstream label;label<<std::hex<<std::setfill('0')<<std::setw(16)<<digest;impl_->packet_digest=label.str();
    }
    return get_state();
}

godot::Dictionary Ps2TrackRenderer::get_memory_state()const{
    // On-demand accounting, not a per-frame walk. Payload/capacity estimates do
    // not include allocator, Godot/driver metadata, alignment or shader binaries.
    godot::Dictionary d;const auto failure=catch_all([&]{std::size_t geometry=0,pixels=0,mips=0,metadata=0,topology=0,source_surfaces=0;
    const auto storage=[](const auto& v){return v.capacity()*sizeof(typename std::decay_t<decltype(v)>::value_type);};
    if(impl_->scene){const auto& s=*impl_->scene;
        metadata=storage(s.instances)+storage(s.sections)+storage(s.zones)+storage(s.light_packs)+storage(s.geometry.solids)+storage(s.textures);
        for(const auto& solid:s.geometry.solids){geometry+=storage(solid.surfaces);for(const auto& surface:solid.surfaces){geometry+=storage(surface.vertices)+storage(surface.indices);++source_surfaces;}}
        for(const auto& t:s.textures)pixels+=t.bitmap.rgba.capacity();
        for(const auto& entry:s.mipmaps)for(const auto& level:entry.second)mips+=level.rgba.capacity();
        for(const auto& section:s.sections){metadata+=storage(section.nodes);for(const auto& node:section.nodes)metadata+=storage(node.children);}
        for(const auto& zone:s.zones)metadata+=storage(zone.sections)+storage(zone.perimeter);
        d["raw_bundle_bytes"]=int64_t(s.bundle.capacity());
    }else d["raw_bundle_bytes"]=0;
    if(impl_->environment){const auto& t=impl_->environment->topology;topology=storage(t.nodes)+storage(t.squares)+storage(t.polygons)+storage(t.indices);}
    d["source_geometry_bytes"]=int64_t(geometry);d["source_texture_bytes"]=int64_t(pixels);d["source_mip_bytes"]=int64_t(mips);d["source_metadata_bytes"]=int64_t(metadata);d["topology_bytes"]=int64_t(topology);
    d["source_bytes"]=int64_t(geometry+pixels+mips+metadata+topology+(impl_->scene?impl_->scene->bundle.capacity():0));
    d["released_bundle_bytes"]=int64_t(impl_->released_bundle_bytes);d["available_source_surfaces"]=int64_t(source_surfaces);
    std::size_t mesh_bytes=0;for(const auto& entry:impl_->meshes){const auto& s=impl_->scene->geometry.solids.at(entry.first.first).surfaces.at(entry.first.second);mesh_bytes+=s.vertices.size()*sizeof(float)*13+s.indices.size()*sizeof(std::int32_t);}
    d["source_meshes"]=int64_t(impl_->meshes.size());d["source_mesh_payload_bytes"]=int64_t(mesh_bytes);
    d["batch_entries"]=int64_t(impl_->batch_cache?impl_->batch_cache->entries():0);d["batch_payload_bytes"]=int64_t(impl_->batch_cache?impl_->batch_cache->bytes():0);
    std::set<const Impl::Draw*> active;for(const auto& view:impl_->view_draws)for(auto* draw:view.second)active.insert(draw);
    std::size_t materials=0,inactive_bytes=0;for(const auto& entry:impl_->draws){materials+=entry.second.material.is_valid()+entry.second.reflection_material.is_valid();if(!active.count(&entry.second))inactive_bytes+=entry.second.mesh_bytes;}
    d["draws"]=int64_t(impl_->draws.size());d["active_draws"]=int64_t(active.size());d["inactive_draws"]=int64_t(impl_->draws.size()-active.size());
    d["inactive_draw_limit"]=int64_t(impl_->residency_cache?4096:Impl::inactive_draw_limit);d["inactive_mesh_limit"]=int64_t(impl_->residency_cache?128u*1024u*1024u:Impl::inactive_mesh_limit);d["inactive_retention_frames"]=int64_t(impl_->residency_cache?1800:120);d["inactive_mesh_payload_bytes"]=int64_t(inactive_bytes);d["draw_evictions"]=int64_t(impl_->draw_evictions);
    d["materials"]=int64_t(materials);d["shaders"]=int64_t(impl_->shaders.size());d["uniform_cache_materials"]=int64_t(impl_->uniform_values.size());
    godot::Dictionary post;if(impl_->postprocess_renderer)post=impl_->postprocess_renderer->stats();d["postprocess"]=post;
    d["depth_proxies"]=post.get("depth_proxies",0);d["depth_materials"]=post.get("depth_materials",0);
    std::size_t debug=0;if(impl_->debug)debug=storage(impl_->debug->triangles)+storage(impl_->debug->edges)+storage(impl_->debug->legend);
    d["debug_geometry_bytes"]=int64_t(debug);d["render_targets"]=0;godot::Array targets;std::uint64_t target_pixels=0;
    std::function<void(const godot::Node*)> visit=[&](const godot::Node* n){
        if(auto* viewport=godot::Object::cast_to<godot::SubViewport>(n)){const auto size=viewport->get_size();godot::Dictionary row;row["width"]=size.x;row["height"]=size.y;targets.push_back(row);target_pixels+=std::uint64_t(size.x)*size.y;}
        for(int i=0;i<n->get_child_count();++i)visit(n->get_child(i));};visit(this);
    d["render_targets"]=targets.size();d["render_target_sizes"]=targets;d["render_target_rgba8_lower_bound_bytes"]=int64_t(target_pixels*4);
    d["accounting"]= "CPU vector capacities and GPU attribute/RGBA8 payload estimates; excludes allocator/driver overhead, shader binaries and shared math image; not a process-memory total";
    });if(!failure.empty())d["error"]=text(failure);return d;
}

godot::Dictionary Ps2TrackRenderer::get_state()const{
    godot::Dictionary result;result["lod"]=get_lod_state();result["geometry_counts_are_candidates"]=impl_->show_lod;result["active_viewport_rids"]=impl_->active_viewport_rids;result["host_stats"]=get_optimization_state();
    godot::Dictionary costs;costs["environment_submit_ms"]=impl_->environment_submit_ms;costs["main_submit_ms"]=impl_->main_submit_ms;costs["reflection_submit_ms"]=impl_->reflection_submit_ms;costs["post_submit_ms"]=impl_->post_submit_ms;costs["particle_submit_ms"]=impl_->particle_submit_ms;costs["sun_submit_ms"]=impl_->sun_submit_ms;costs["previous_sun_readback_ms"]=impl_->sun_readback_ms;costs["batch_breaks_nonexclusive"]=impl_->batch_breaks;costs["batch_breaks_enabled"]=impl_->diagnostics_level==2;result["costs"]=costs;const auto failure=catch_all([&]{result["ok"]=impl_->ok;result["loading"]=impl_->loading;result["track_id"]=impl_->number;
    result["render_ok"]=impl_->render_error.empty()&&impl_->readback_error.empty();result["render_error"]=text(impl_->render_error);result["readback_error"]=text(impl_->readback_error);
    result["show_lod"]=impl_->show_lod;result["render_collision"]=impl_->render_collision;result["render_material"]=impl_->render_material;
    result["debug_ready"]=impl_->ok&&impl_->environment&&impl_->environment->topology.ready()&&impl_->debug_error.empty();
    result["debug_diagnostic"]=text(impl_->debug_error);
    result["debug_fill_visible"]=impl_->debug_fill&&impl_->debug_fill->is_visible();
    result["debug_edges_visible"]=impl_->debug_edges&&impl_->debug_edges->is_visible();
    godot::Array legend;
    if(impl_->debug)for(const auto& row:impl_->debug->legend){godot::Dictionary item;item["id"]=row.id;item["name"]=text(row.name);item["color"]=godot::Color(row.color[0],row.color[1],row.color[2],1);item["polygon_count"]=int64_t(row.polygon_count);item["known"]=row.known;legend.push_back(item);}
    result["material_legend"]=legend;
    result["debug_triangles"]=int64_t(impl_->debug?impl_->debug->triangles.size():0);
    result["debug_edges"]=int64_t(impl_->debug?impl_->debug->edges.size():0);
    result["native_frame_ms"]=impl_->native_frame_ms;result["host_submit_ms"]=impl_->host_submit_ms;
    result["original_focal_h"]=impl_->frame_view.focal_h;
    godot::Dictionary visibility;visibility["default_near"]=.5f;visibility["default_far"]=10000.f;visibility["effective_near"]=impl_->effective_near;visibility["effective_far"]=impl_->effective_far;visibility["camera_mover_type"]=impl_->camera_mode;visibility["override_enabled"]=port::track::shared_render_settings().distance_override;visibility["force_far_override"]=port::track::shared_render_settings().distance_override?port::track::shared_render_settings().distance:0.f;visibility["logical_screen_width"]=512;visibility["camera_key_gameflow_recovered"]=false;result["visibility_audit"]=visibility;
    result["sky_draws"]=int64_t(impl_->environment_frame.sky_draws);result["fog_intensity"]=impl_->frame.weather.fog;result["fog_start"]=impl_->frame.weather.fog_start;result["fog_colour"]=int64_t(impl_->frame.weather.fog_colour);result["postprocess_passes"]=int64_t(impl_->frame.postprocess.passes.size());
    result["particle_emitter_count"]=int64_t(impl_->scene?impl_->scene->particles.emitters.size():0);result["particle_definition_count"]=int64_t(impl_->scene?impl_->scene->particles.definitions.size():0);result["particles_unsupported_quads"]=int64_t(impl_->particle_frame.unsupported_quads);godot::Array particle_gaps;for(const auto& gap:impl_->particle_frame.gaps)particle_gaps.push_back(text(gap));result["particle_recovery_gaps"]=particle_gaps;result["particles_active"]=int64_t(impl_->particle_frame.active);result["particles_visible"]=int64_t(impl_->particle_frame.visible);
    result["sun_quads"]=int64_t(impl_->sun_frame.quads);result["sun_uncovered"]=impl_->sun_frame.uncovered_available?godot::Variant(impl_->sun_frame.uncovered):godot::Variant();result["sun_readback_pending"]=impl_->sun_frame.readback_pending;
    result["environment_capture_ready"]=impl_->reflection_renderer&&impl_->reflection_renderer->ready();result["shadow_strips"]=int64_t(impl_->shadow_frame.strips);result["shadow_triangles"]=int64_t(impl_->shadow_frame.triangles);
    godot::Array submissions;for(const auto& pair:impl_->submission_counts){godot::Dictionary row;row["view_id"]=pair.first==0?1:int(pair.first+3);row["source_frame"]=int64_t(impl_->frame_number);row["scheduled_this_frame"]=true;row["commands"]=int64_t(pair.second[0]);row["batches"]=int64_t(pair.second[1]);row["largest_batch"]=int64_t(pair.second[2]);const auto timing=impl_->submission_times.find(pair.first);if(timing!=impl_->submission_times.end()){row["selection_ms"]=pair.first==0?impl_->native_frame_ms:timing->second[0];row["planning_ms"]=timing->second[1];row["submission_wall_ms"]=timing->second[2];}row["timing_contract"]="Planning is included in submission wall; selection is separate CPU wall, not GPU time";const TrackSubmissionCache* cache=pair.first==0?&impl_->submission_plan:nullptr;const auto reflected=impl_->reflection_plans.find(pair.first);if(reflected!=impl_->reflection_plans.end())cache=&reflected->second;if(cache){row["plan_hits"]=int64_t(cache->hits);row["plan_builds"]=int64_t(cache->builds);row["dynamic_hits"]=int64_t(cache->dynamic_hits);row["surface_builds"]=int64_t(cache->surface_builds);row["surface_cache_entries"]=int64_t(cache->cached_surfaces());row["plan_last_build_reason"]=cache->last_build_reason;}submissions.push_back(row);}result["view_submissions"]=submissions;
    result["batch_cache_bytes"]=int64_t(impl_->batch_cache?impl_->batch_cache->bytes():0);result["batch_cache_entries"]=int64_t(impl_->batch_cache?impl_->batch_cache->entries():0);
    result["loaded_instances"]=int64_t(impl_->scene?impl_->scene->instances.size():0);
    result["loaded_sections"]=int64_t(impl_->scene?impl_->scene->sections.size():0);
    result["considered_instances"]=int64_t(impl_->frame.considered_instances);
    result["effective_near"]=impl_->effective_near;result["effective_far"]=impl_->effective_far;
    result["packet_digest"]=text(impl_->packet_digest);result["commands"]=int64_t(impl_->frame.commands.size());result["zone"]=impl_->frame.zone;result["visible_instances"]=int64_t(impl_->frame.visible_instances);
    godot::Array sections;for(auto section:impl_->frame.sections)sections.push_back(section);result["sections"]=sections;
    std::string diagnostic=impl_->diagnostic;for(const auto& gap:impl_->environment_frame.gaps)diagnostic+=gap+"\n";for(const auto& gap:impl_->particle_frame.gaps)diagnostic+=gap+"\n";for(const auto& gap:impl_->sun_frame.gaps)diagnostic+=gap+"\n";for(const auto& gap:impl_->shadow_frame.gaps)diagnostic+=gap+"\n";if(impl_->reflection_renderer&&!impl_->reflection_renderer->diagnostic().empty())diagnostic+=impl_->reflection_renderer->diagnostic()+"\n";
    if(impl_->show_lod)diagnostic="Show LOD debug: all authored levels | "+std::to_string(impl_->draws.size())+" resident surface groups (not visible draws)\n"+diagnostic;
    if(!impl_->render_error.empty())diagnostic+="Frame render gap (track retained): "+impl_->render_error+"\n";
    if(!impl_->readback_error.empty())diagnostic+="Visual readback gap (track retained): "+impl_->readback_error+"\n";
    result["phase"]=text(impl_->phase);result["error"]=text(impl_->error);result["diagnostic"]=text(diagnostic);
    result["bounds"]=impl_->bounds;result["triangles"]=int64_t(impl_->triangles);
    result["render_nodes"]=int64_t(impl_->nodes.size());result["batched_instances"]=int64_t(impl_->batched_instances);
    result["summary"]=text(std::string(impl_->show_lod?"Debug all LODs: ":"Original track: ")+std::to_string(impl_->triangles)+" triangles / "+std::to_string(impl_->textures.size())+" textures");
    if(impl_->environment){result["start_position"]=host(impl_->environment->start);result["start_forward"]=godot::Vector3(impl_->environment->direction[1],0,impl_->environment->direction[0]);}
    if(impl_->scene)result["track_name"]=text(impl_->scene->entry.name);
    });
    if(!failure.empty()){result["ok"]=false;result["error"]=text(failure);}return result;
}
}

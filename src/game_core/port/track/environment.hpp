#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace port::track {
struct Scene;
}
namespace port::driving {class Memory;}
namespace port::track {
using EnvironmentVector=std::array<float,3>;
using EnvironmentMatrix=std::array<float,16>;
struct SkyModel {
    bool available=false;
    std::uint32_t hash=0;
    std::size_t instance=0;
    EnvironmentMatrix local_world{};
};
struct WeatherRegion {
    std::uint32_t name_hash=0,group_hash=0,colour=0;
    std::string name,group;
    int type=0;
    float fog_start=0,parameter1=0,radius=0,intensity=0,falloff_start=0;
    EnvironmentVector position{};
};
struct WeatherResult {
    float rain=0,fog=0,snow=0,wind=0,fog_start=0;
    std::uint32_t fog_colour=0;
    std::array<std::size_t,4> fog_regions{}; // root plus3 original blend regions, SIZE_MAX absent
    std::array<float,4> fog_scales{};
};
struct SunLayer {
 std::uint32_t texture=0,alpha_type=0;float intensity=0,size=0,offset_x=0,offset_y=0;
 std::array<std::uint8_t,4> colour{};std::int32_t angle=0;float sweep=0;
};
struct SunResources {bool available=false;EnvironmentVector position{},shadow_position{};std::array<SunLayer,6> layers{};};
struct SunReadbackRect {int x=0,y=0,start_x=0,start_y=0,end_x=0,end_y=0;bool requested=false;};
struct SunOcclusion {std::array<std::uint32_t,1024> pixels{};SunReadbackRect rectangle;unsigned uncovered()const;};
SunReadbackRect sun_readback_rect(const SunResources&,unsigned view_id,float screen_x,float screen_y,float camera_z,unsigned width,unsigned height);
struct SunQuad {std::uint32_t texture_hash=0;std::array<std::array<float,2>,4> vertices{};std::array<std::uint8_t,4> colour{};bool additive=false;float native_depth=0;};
struct EnvironmentResources {
    SunResources sun;
    SkyModel sky,environment_sky;
    std::vector<std::size_t> sky_instances; // remove these models from ordinary scenery submissions
    std::vector<WeatherRegion> weather;
    std::array<std::uint32_t,5> sun_textures{{0x47beb4b6,0xd4d26c59,0x1744b82d,0xad3c5239,0xd4d80a65}};
};
struct EnvironmentFace {
    EnvironmentMatrix camera_matrix{}; // original world->camera look-at matrix
    bool active=false;
    std::uint16_t horizontal_fov=0x75e4;
    float near_z=.5f,far_z=10000.f;
};
struct ReflectionSample { float rx=0,ry=0,rz=0,rs=0,rt=0,fs=0,ft=0; };
struct ReflectionTables {
    std::array<std::array<ReflectionSample,36>,6> front{};
    std::array<std::array<ReflectionSample,36>,4> back{};
};
struct EnvironmentWarpVertex {std::array<float,2> destination{},source_uv{};};
struct EnvironmentWarp {
    unsigned source_face=0;
    bool back=false;
    std::vector<EnvironmentWarpVertex> vertices;
    std::vector<std::uint32_t> indices;
};
struct EnvironmentCameras {
    std::array<EnvironmentFace,6> faces{};
    EnvironmentMatrix hack_man_matrix{};
};
// Byte-layout adapters and original-class/free-function bindings are implemented
// separately. No Godot or generic cubemap storage is introduced by these values.
struct ViewDepth {float near_z=.5f,far_z=10000.f;};
ViewDepth original_view_depth(port::driving::Memory&,unsigned view_id,int mover_type,
    std::uint16_t horizontal_fov,float near_z,float far_z,float force_near=0,float force_far=0);
EnvironmentResources decode_environment(const std::vector<std::uint8_t>&);
std::vector<SunQuad> sun_quads(port::driving::Memory&,const EnvironmentResources&,unsigned view_id,
 float screen_x,float screen_y,float camera_z,unsigned width,unsigned height,float zmax,
 unsigned uncovered_depth_samples);
WeatherResult query_weather(const EnvironmentResources&,const EnvironmentVector&);
EnvironmentVector environment_eye_offset();
ReflectionTables reflection_tables();
std::vector<EnvironmentWarp> environment_warps(const ReflectionTables&,
    const std::array<bool,6>& active_faces,unsigned width,unsigned height);
EnvironmentCameras environment_cameras(const EnvironmentVector& viewer,
    const EnvironmentVector& environment_position,std::uint32_t frame_counter,int view_mode=1,bool in_game=true);
}

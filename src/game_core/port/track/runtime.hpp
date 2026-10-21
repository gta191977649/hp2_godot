#pragma once
#include "assets.hpp"
#include "effects.hpp"
#include "postprocess.hpp"
#include "game_core/port/driving/memory.hpp"
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <string>
namespace port::track {
struct StaticLodScene;
// Port-owned immutable resources and frame values; native XYZ, column-major matrices.
struct FrameView {
    std::array<float,3> position{},direction{{1,0,0}};
    std::array<float,16> clip_matrix{}; // native world -> homogeneous clip (-w..w)
    float focal_h=1; // horizontal pixel focal length, original eView::H
    std::uint32_t exclude_flags=0x18; // ordinary single-view, forward direction
    // Explicit original view/visual inputs; no host objects or elapsed wall-clock.
    unsigned view_id=1;int view_mode=1;
    std::uint32_t frame_counter=0;
    float near_z=.5f,far_z=10000.f,visual_timestep=0;
    float camera_focal_distance=0,camera_depth_of_field=0;
    std::array<float,3> environment_position{};
    bool environment_capture_valid=false; // requires the original car capture origin
    postprocess::Controls post_controls;
};
struct DrawCommand {
    std::size_t instance=0,solid=0,surface=0;
    unsigned lod=0,visibility=0;
    std::array<float,16> matrix{};
    std::array<float,28> light_context{};
    bool identity=false;
    bool has_light_context=true;
    std::size_t original_order=0;
    float depth=0; // native camera-forward distance; canonical strip sorting refines this
    EffectState effect;
};
// Port acceleration only: first-occurrence hash ownership matches the linear source.
// The Scene remains immutable; no original ABI layouts are reinterpreted.
struct ResourceIndex {
    std::unordered_map<std::uint32_t,std::size_t> solids,textures;
    std::unordered_map<int,std::size_t> sections;
    explicit ResourceIndex(const Scene&);
    const port::vehicle::Texture& texture(const Scene& scene,std::uint32_t hash) const {
        const auto found=textures.find(hash);
        if(found==textures.end()||found->second>=scene.textures.size()||scene.textures[found->second].header.tex_hash!=hash)throw std::runtime_error("Track model has unresolved texture");
        return scene.textures[found->second];
    }
};
struct Frame {
    float host_lod_focal_h=0; // E0056H01: all-level descriptor projection, zero in original mode.
    const ResourceIndex* resource_index=nullptr; // borrowed only during native preparation
    std::vector<DrawCommand> commands;
    std::vector<int> sections;
    int zone=-1;
    std::size_t considered_instances=0,visible_instances=0;
    std::size_t submitted_surfaces=0; // original raw submit order before strip eligibility
    WeatherResult weather;
    postprocess::Frame postprocess;
    EnvironmentCameras environment_views;
};
class Runtime {
public:
    void load(std::shared_ptr<const Scene>);
    void load(assets::Source&,const Entry&);
    bool poll_load()const{return ready();} // synchronous native completion; host owns async scheduling
    void unload();
    bool ready()const{return bool(scene_);}
    const std::shared_ptr<const Scene>& resources()const{return scene_;}
    Frame prepare_frame(const FrameView&,port::driving::Memory& original_math);
    // E0056H01: alternate host policy. Emits all resource levels, never selects one.
    Frame prepare_hlod_frame(const FrameView&,port::driving::Memory&,const StaticLodScene&,
        std::size_t warm_instance=std::size_t(-1));
    std::string diagnostics()const;
private:
    std::shared_ptr<const Scene> scene_;
    std::unique_ptr<ResourceIndex> index_;
    std::vector<std::size_t> zone_order_;
    std::vector<std::uint32_t> texture_chain_order_;
};
}

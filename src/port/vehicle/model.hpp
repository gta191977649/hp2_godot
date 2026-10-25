#pragma once
#include "port/render/vehicle.hpp"
#include "game_core/port/driving/session.hpp"
#include "port/vehicle/textures.hpp"
#include "port/vehicle/catalog.hpp"
#include "game_core/CarRenderInfo.hpp"
#include "game_core/port/driving/vehicle_lights.hpp"
#include <map>
#include "game_core/port/driving/water_effects.hpp"

namespace port::vehicle {
// Port-side runtime ownership and explicit viewer controls, not an original class.
struct Part {
    std::string name;
    std::size_t solid{};
    std::array<float,16> transform{};
    int lod = -1;
    enum class Group { Body, Tire, Brake, Mirror, Plate, WheelBlur, Wiper } group = Group::Body;
    bool enabled = true;
    bool selected = false;
    std::array<float,28> lights{};
    int wheel=-1;
};
// Single-worker cache; snapshots own their model data and do not read this on the render thread.
struct ModelCache {
    std::string root;
    std::vector<std::uint8_t> global,frontend;
    std::vector<CarChoice> catalog;
    std::shared_ptr<TextureCache> textures=std::make_shared<TextureCache>();
    struct Geometry {render::VehicleGeometry data;std::vector<std::array<float,3>> collision_vertices;std::uint64_t age=0;std::size_t bytes=0;};
    std::map<std::string,Geometry> geometry;
    std::size_t bytes=0;std::uint64_t clock=0,hits=0,reads=0;double geometry_decode_ms=0,binding_ms=0;
};
class Model {
public:
    explicit Model(std::shared_ptr<ModelCache> cache=std::make_shared<ModelCache>()):cache_(std::move(cache)){}
    void include_lods(bool enabled){include_lods_=enabled;}
    bool includes_lods()const{return include_lods_;}
    // Debug force-on of the original blur resource; not a recovered speed threshold.
    void include_wheel_blur(bool enabled){wheel_blur_=enabled;}
    void set_wheel_blur(bool enabled);
    void set_wheel_speed(double radians_per_second);
    void advance_wheels(double delta);
    // Raw original001C0D78 absolute wheel poses; applies recovered00120EF8
    // render basis once. Does not integrate preview phase.
    void apply_driving_pose(const std::array<driving::WheelSnapshot,4>& wheels);
    void apply_wiper_state(const driving::WiperSnapshot& state);
    // Debug00120350/00186940: original on/off textures, not invented emission.
    std::uint32_t driving_texture(std::size_t solid,std::size_t surface,
                                 const driving::VehicleLightState&)const;
    double wheel_phase()const{return phase_;}
    std::shared_ptr<ModelCache> cache()const{return cache_;}
    std::array<float,3> preview_color()const;
    void load(assets::Source& source,const std::string& geometry_path,const std::string& code={},int skin=1);
    void load_id(assets::Source& source,int car_id,int skin_id);
    // Whitelisted original selector PLATFORM/CARCOVER solids; never a fake car ID.
    void load_frontend_solid(assets::Source& source,std::uint32_t hash);
    bool is_frontend_solid()const{return frontend_solid_;}
    // Source-local wheel matrices, consumed without preview rotation integration.
    void apply_frontend_pose(const std::array<std::array<float,16>,4>& tires,
                             const std::array<std::array<float,16>,4>& brakes);
    port::CarLodSelection select(int projected_size,int forced_lod);
    void set_part_visible(const std::string& name,bool visible);
    const render::VehicleGeometry& geometry()const{return geometry_;}
    const std::vector<render::VehiclePositionMarker>& position_markers()const{return position_markers_;}
    const std::array<float,3>& marker_pivot()const{return marker_pivot_;}
    const std::array<bool,2>& wheel_blur_available()const{return wheel_blur_available_;}
    const std::vector<Texture>& textures()const{return textures_;}
    const std::vector<Part>& parts()const{return parts_;}
    const std::string& diagnostic()const{return diagnostic_;}
    const std::map<std::uint32_t,port::CarMaterial>& materials()const{return materials_;}
    const std::vector<std::array<float,3>>& collision_vertices()const{return collision_vertices_;}
    const port::CarRenderConfig& render_config()const{return config_;}
    int car_id()const{return car_id_;}
    int skin_id()const{return skin_id_;}
    const std::string& path()const{return path_;}
private:
    std::shared_ptr<ModelCache> cache_;
    bool include_lods_=true;
    bool frontend_solid_=false;
    bool frontend_pose_=false;
    bool wheel_blur_=false;
    bool driving_pose_=false;
    bool wipers_active_=false;
    CarRenderConfig config_;
    double phase_=0;
    double wheel_speed_=80; // Port-side preview input, not original vehicle physics.
    render::VehicleGeometry geometry_;
    std::vector<render::VehiclePositionMarker> position_markers_;
    std::array<float,3> marker_pivot_{};
    std::array<bool,2> wheel_blur_available_{};
    std::vector<std::array<float,3>> collision_vertices_;
    std::vector<Texture> textures_;
    std::vector<Part> parts_;
    std::map<std::uint32_t,port::CarMaterial> materials_;
    std::map<std::pair<std::size_t,std::size_t>,int> light_roles_;
    std::array<std::uint32_t,4> light_hashes_{}; // head off/on, brake off/on.
    std::array<bool,4> presence_{};
    std::string diagnostic_;
    std::vector<CarChoice> catalog_;
    std::string path_;
    int car_id_=-1,skin_id_=-1;
};
}

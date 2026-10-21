#pragma once
#include "port/assets/catalog.hpp"
#include "port/render/vehicle.hpp"
#include "port/vehicle/textures.hpp"
#include "game_core/CarRenderInfo.hpp"
#include "game_core/src/ecstasy/elighte.hpp"
#include "environment.hpp"
#include "particles.hpp"

// Port-side ownership and presentation adapter, not PS2 ABI storage.
namespace port::track {
struct Entry {
    int number{}, resource_number{};
    std::string name, location;
    std::uint8_t time_of_day{};
    std::vector<std::string> resources;
};
struct Instance {
    std::uint32_t solid_hash{};
    std::array<float,16> transform{};
    std::uint16_t flags{};
    bool facade{};
    std::array<std::uint32_t,3> lod_hashes{};
    std::array<std::int16_t,3> far_clip_sizes{};
    std::array<bool,3> facades{};
    std::array<float,3> position{},bbox_min{},bbox_max{};
    float radius=0;
};
struct SceneryNode {
    std::array<float,3> minimum{},maximum{};
    std::vector<std::int16_t> children;
};
struct Section {
    int number=0;
    std::size_t instance_begin=0,instance_count=0;
    std::vector<SceneryNode> nodes;
};
struct Zone {
    int number=0;
    std::vector<std::int16_t> sections;
    std::vector<std::array<float,2>> perimeter;
};
struct Scene {
    Entry entry;
    EnvironmentResources environment;
    ParticleResources particles;
    render::VehicleGeometry geometry;
    std::vector<vehicle::Texture> textures;
    std::vector<std::string> gaps;
    // E0056T01: original NameHash -> bound TextureInfo hash; raw bundle unchanged.
    std::map<std::uint32_t,std::uint32_t> texture_fixups;
    std::vector<std::uint8_t> bundle;
    std::vector<Instance> instances;
    std::vector<Section> sections;
    std::vector<Zone> zones;
    std::vector<LightPack> light_packs;
    std::map<std::uint32_t,std::vector<hp2::assets::DecodedTexture>> mipmaps; // Original levels1..MXL, no generated pixels.
    std::array<float,28> world_lights{}; // Original world-identity context VU35C..362.
    std::map<std::uint32_t,port::CarMaterial> materials;
    bool world_lights_available=false;
};
std::vector<Entry> load_catalog(assets::Source& source);
std::vector<std::uint8_t> decompress_bundle(const std::vector<std::uint8_t>& bytes);
Scene load_scene(assets::Source& source, const Entry& entry);
// E-0075-R01: original shared AcidFX library and original texture/mip resources.
// Also supports the port-owned flat Test drive scene without track scenery.
void append_particle_library(Scene&,assets::Source&);
}

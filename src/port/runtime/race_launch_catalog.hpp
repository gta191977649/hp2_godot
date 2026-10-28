#pragma once
// Port-owned Retail-asset -> reviewed Debug launch-field adapter, E-0098-RP03.
// TrackInfo row layouts differ; this never overlays a Retail row as Debug memory.
#include "game_core/port/race/launch.hpp"
#include <string>
#include <vector>
namespace port::race {
struct LaunchCharacter {
    int number=0;
    std::string name,body_type;
    std::uint32_t address=0;
};
struct LaunchTrack {
    std::string name;
    int number=0,point_to_point=0;
    std::uint32_t cop_name_hash=0;
};
class LaunchCatalog {
public:
    // Persistent logical resource storage; memory must outlive this catalog and services.
    LaunchCatalog(const std::vector<std::uint8_t>& global,::port::driving::Memory&);
    const std::array<hp2::debug::CarTypeInfo,64>& cars() const{return cars_;}
    const std::vector<LaunchCharacter>& characters() const{return characters_;}
    const LaunchTrack* track(int number) const;
    std::uint32_t character_address(int index) const;
    bool character_is_male(int index) const;
    std::optional<int> character_number_from_hash(std::uint32_t) const;
    std::uint32_t skin_info(int car,int skin,int default_if_missing) const;
    ParticipantServices services(int language) const;
private:
    std::array<hp2::debug::CarTypeInfo,64> cars_;
    std::vector<LaunchCharacter> characters_;
    std::vector<LaunchTrack> tracks_;
    struct Skin {int car,number;std::uint32_t address;};
    std::vector<Skin> skins_;
};
}

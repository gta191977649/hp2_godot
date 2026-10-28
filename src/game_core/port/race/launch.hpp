#pragma once
// Port-owned service bindings for original launch code; no Godot or original ABI.
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include <optional>
namespace port::race {
struct ParticipantServices {
    const std::array<hp2::debug::CarTypeInfo,64>& cars;
    int language=0;
    std::function<std::uint32_t(int)> character_address;
    std::function<bool(int)> character_is_male;
    std::function<std::optional<int>(std::uint32_t)> character_number_from_hash;
    std::function<std::optional<std::uint32_t>(int)> track_cop_hash;
    SkinLookup skin_info;
};
struct EventLaunchServices {
    hp2::debug::FrontendDatabase& fred;
    std::function<std::string(const std::string&)> translated_key;
    std::function<std::string(int)> translated_id;
    std::function<std::string(const hp2::debug::EventTree&,const hp2::debug::EventInfo&,const std::string&)> format_description;
    std::function<std::string(int)> car_full_name;
    std::function<int(int,int)> track_language_id;
};
enum class Flow {none=0,unloading_frontend=2,in_frontend=3,loading_track_common=4};
struct StartServices {
    const ParticipantServices& participants;
    std::function<std::optional<int>(int)> track_point_to_point;
    // Required snapshots/services of original Fred profile/display/high-score data.
    // No default record values or synthesized profile may be substituted.
    std::function<void(::port::driving::Memory&,std::uint32_t)> copy_profile_settings;
    std::function<void(int)> set_record_times;
    std::function<void(Flow,Flow)> change_game_flow;
    Flow current_flow=Flow::in_frontend;
    int maximum_ai_cars=7;
    bool demo_replay_active=false;
};
}

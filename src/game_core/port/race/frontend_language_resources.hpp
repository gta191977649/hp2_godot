#pragma once
// Port raw-resource lifetime, not original file/device/speech initialization.
#include "game_core/port/driving/memory.hpp"
namespace port::race {
class FrontendLanguageResources {
 driving::Memory& memory_;std::uint32_t chunk_=0;
 std::uint32_t track_table_=0;std::vector<std::uint32_t> track_allocations_;
 std::vector<std::uint32_t> track_cop_fields_;
 std::vector<std::uint32_t> track_point_fields_;
 std::vector<std::uint32_t> track_valid_fields_,track_score_fields_;
 std::vector<std::uint8_t> retail_tracks_;
public:
 explicit FrontendLanguageResources(driving::Memory& m):memory_(m){}
 void load(const std::vector<std::uint8_t>&);
 void load_track_names(const std::vector<std::uint8_t>&); //Reviewed Name,40..43,46,48/4C IDs and54CopCharacter hash only; other Debug fields stay unreadable.
 void close();
};
}

#pragma once
// Port context for original shared route resources and explicit unknown stack
// vector padding. No whole AI provider callback or generated target is accepted.
#include "game_core/port/track/route_data.hpp"
#include <optional>
namespace port::race {
class AiPlanningResources {
 ::port::driving::Memory& memory_;
 std::vector<std::uint32_t> raw_chunks_;
 bool skill_chances_loaded_=false;
public:
 explicit AiPlanningResources(::port::driving::Memory& m):memory_(m){}
 // Retail34B00 is3E0bytes; Debug is368bytes. Full original raw chunks stay
 // in shared Memory. Only reviewed +170/stride1C/three-chance fields project
 // into the Debug global. W47 additionally reviews SpeedFactor, byte
 // RoutePointVariance and Misc.RoutePointChangeFrequency; all other fields
 // remain unmapped until their own consumer correspondence is established.
 void load_retail_difficulty_info(const std::vector<std::uint8_t>&);
 float take_sign_chance(const ::port::driving::Memory&,int skill,int chance)const;
 float speed_factor(const ::port::driving::Memory&,int skill)const;
 // E-0110-SK01: skill-2 follows the original signed adjacent-field read at
 // payload148, already reviewed/mapped by W92. Other negative skills guard.
 std::uint8_t route_point_variance(const ::port::driving::Memory&,int skill)const;
 float route_point_change_frequency(const ::port::driving::Memory&)const;
 float boost_calculation_frequency(const ::port::driving::Memory&)const; //W47 Retail3D8 ->Debug360.
 const std::vector<std::uint32_t>& raw_chunk_addresses()const{return raw_chunks_;}
 // W92 individually reviewed distance graphs, position/ratio arrays,
 // SlowFactor and shifted MaxDistanceForBoost; no complete schema import.
 float collision_avoidance_chance(const ::port::driving::Memory&,int skill)const; //W93 field review.
 void require_boost_fields(const ::port::driving::Memory&)const;
 void close();
};
struct AiDriveTargetServices {
 const ::port::track::RouteData* routes=nullptr;
 const AiPlanningResources* planning=nullptr;
 // Original temporary bVector3 padding is uninitialized in001BE4B8. XYZ is
 // recovered. A host execution must state its padding operand explicitly.
 std::optional<std::uint32_t> port_stack_vector_lane;
};
}

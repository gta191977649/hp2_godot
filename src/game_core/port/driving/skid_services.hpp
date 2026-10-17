#pragma once
// TASK-0046: explicit host services and publication; never an alternate skid algorithm.
#include "memory.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace port::driving {
struct ContactProvider;
using SkidVector=std::array<float,3>;
struct SkidTexture { std::string name; std::uint32_t hash=0; bool present=false,semitransparent=false,alpha_sort=false; int rendering_order=0; bool gloss_map_in_alpha=false; };
struct SkidQuad {
    std::array<SkidVector,4> vertices{};
    std::array<std::array<float,2>,4> uv{};
    std::array<std::array<std::uint8_t,4>,4> colours{};
    int terrain=0;
    std::string texture_name;
    std::uint32_t texture_hash=0;
    bool semitransparent=false,alpha_sort=false;
    unsigned draw_context=0,alpha_test=0,alpha_reference=0;
    bool write_depth=false;
};
struct SkidDrawState { unsigned draw_context=0,alpha_test=0,alpha_reference=0; bool write_depth=false; };
struct SkidView {
    SkidVector position{};
    std::array<float,16> clip_matrix{};
    float H=0;
    unsigned view_id=0;
    bool nis_playing=false;
};
// Addresses only: every mutable allocation/list/pool flag lives in the candidate Memory.
// The single synthetic clan is a tool-world boundary, not recovered GetClan/topology.
struct SkidArena {
    std::uint32_t list=0,clan_list=0,clan_bbox=0,pool=0,used=0,nodes=0;
    std::array<SkidTexture,33> textures{};
};
struct SkidServices {
    SkidArena* arena=nullptr;
    ContactProvider* contact=nullptr;
    std::function<void(const SkidQuad&)> emit;
    bool depth_route_enabled=true; // Explicit synthetic normal-target render service.
};
using SkidTextureLookup=std::function<SkidTexture(const std::string&,std::uint32_t)>;
// Host allocator/list binding only; allocation extent/count come from Debug InitSkids.
void allocate_skid_arena(Memory&,SkidArena&);
std::uint32_t allocate_skid_slot(Memory&,const SkidArena&);
void free_skid_slot(Memory&,const SkidArena&,std::uint32_t);
void validate_skid_services(const SkidServices&);
}

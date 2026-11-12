#pragma once
// TASK-0112 port host dependencies for original NIS entrypoints. No Godot,
// ResourceFile overlay, fake loaded/playing state or alternate completion timer.
#include "game_core/port/driving/memory.hpp"
#include <functional>
#include <optional>
#include <string>
namespace port::race {
struct PhysicsConstructionServices;
struct NisClock {int month=0,day=0,year=0,hour=0,minute=0,second=0;}; //Decoded local RTC fields.
struct NisServices {
 PhysicsConstructionServices* physics=nullptr;
 std::optional<std::uint32_t> placement_route_position_w; //Original hidden route-position stackW.
 std::optional<std::uint32_t> placement_bbox_corner_w; //001AECA0 original stackW copied into8 corners.
 // Actual resource ownership: each returned opaque token must retain every
 // consumed BIN/CAM chunk until unload. Callback runs only after real loading.
 std::function<std::uint32_t(const std::string&)> load_resource;
 std::function<void(std::uint32_t)> unload_resource;
 std::function<void()> service_resource_loading;
 std::function<void(std::uint32_t,int)> finished_callback;
 std::function<std::optional<NisClock>()> read_local_clock;
};
namespace detail {struct BoundNisServices {driving::Memory* memory;NisServices* services;};inline thread_local const BoundNisServices* current_nis_services=nullptr;}
class NisServicesScope {
 detail::BoundNisServices value_;const detail::BoundNisServices* previous_;
public:
 NisServicesScope(driving::Memory& memory,NisServices& services):value_{&memory,&services},previous_(detail::current_nis_services){detail::current_nis_services=&value_;}
 ~NisServicesScope(){detail::current_nis_services=previous_;}
 NisServicesScope(const NisServicesScope&)=delete;NisServicesScope& operator=(const NisServicesScope&)=delete;
};
NisServices& require_nis_services(driving::Memory&); //Throws outside matching native owner scope.
}

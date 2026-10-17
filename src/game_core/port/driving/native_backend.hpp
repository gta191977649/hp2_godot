#pragma once
// TASK-0044: port-owned fixture; driving runs the canonical Debug original-class chain.
#include "session.hpp"
#include "original_image.hpp"
#include "track_environment.hpp"
#include "static_collisions.hpp"
#include "game_core/port/driving/projections.hpp"
namespace port::driving {
class NativeBackend final:public Simulation {
public:
    explicit NativeBackend(std::filesystem::path elf):elf_(std::move(elf)){}
    void reset()override;
    Snapshot advance(const Config&,const Input&,float elapsed,const SurfaceSettings& surface={})override;
    float minimum_frame_elapsed()const override;
    void track(std::shared_ptr<const TrackEnvironment>)override;
    void skid_textures(SkidTextureLookup lookup)override{skid_lookup_=std::move(lookup);}
    std::vector<SkidQuad> render_skids(const SkidView&)override;
    const Memory& memory()const{return state_;}
    // Port-side read-only inspection anchor for evidence/regression capture.
    std::uint32_t physics_car_address()const{return mover_;}
    const StaticCollisions& static_collisions()const{return collisions_;}
    void collision_geometry(std::vector<std::uint8_t> bytes){collision_geometry_=std::move(bytes);collision_geometry_available_=true;reset();}
private:
    void initialize(Memory&,const Config&);
    native::InitServices init_services();
    native::DrivingServices driving_services(native::InitServices&);
    Snapshot publish(Memory&,float)const;
    std::filesystem::path elf_;
    Memory image_,state_;
    PlaneContact plane_;
    TrackContact track_contact_;
    StaticCollisions collisions_;
    std::vector<std::uint8_t> collision_geometry_;
    bool collision_geometry_available_=false;
    ContactProvider* contact(){return track_contact_.environment?static_cast<ContactProvider*>(&track_contact_):static_cast<ContactProvider*>(&plane_);}
    std::array<SurfaceMaterial,5> catalog_;
    std::vector<SurfaceMaterial> track_catalog_;
    bool loaded_=false,initialized_=false;
    std::uint32_t mover_=0,car_=0,controls_=0;
    std::uint64_t step_=0;
    SkidArena skid_arena_;
    SkidTextureLookup skid_lookup_;
};
}

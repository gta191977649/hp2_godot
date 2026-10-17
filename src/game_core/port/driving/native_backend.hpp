#pragma once
// Port-owned laboratory vehicle/environment state. Original algorithms live in Sub_* files.
#include "session.hpp"
#include "original_image.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/driving/projections.hpp"
namespace port::driving {
class NativeBackend final:public Simulation {
public:
    explicit NativeBackend(std::filesystem::path elf):elf_(std::move(elf)){}
    void reset()override;
    Snapshot advance(const Config&,const Input&,float elapsed)override;
    float minimum_frame_elapsed()const override;
    const Memory& memory()const{return state_;}
private:
    void initialize(Memory&,const Config&);
    native::InitServices init_services();
    native::DrivingServices driving_services(native::InitServices&);
    Snapshot publish(Memory&,float)const;
    std::filesystem::path elf_;
    Memory image_,state_;
    PlaneContact plane_;
    bool loaded_=false,initialized_=false;
    std::uint32_t mover_=0,car_=0,controls_=0;
    std::uint64_t step_=0;
};
}

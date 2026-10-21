#pragma once
// Port-side state binding of Debug DriveCameraMover POV slices; E-0084-C01/E-0088-C01.
#include "memory.hpp"
#include "game_core/port/track/topology.hpp"
#include "game_core/port/track/voronoi.hpp"
#include <array>
namespace port::driving {
struct DrivingCameraFrame {
    std::array<float,3> position{},target{},up{{0,0,1}};
    int pov_index=1;
    float lookback_param=0;
    int pov_type=2;
    std::uint16_t fov_bangle=0x32dc;
    float near_z=.5f;
    bool hide_car=false,render_dash=false;
};
class DrivingCamera {
public:
    void reset(){*this=DrivingCamera{};}
    int cycle_pov(Memory&,std::uint32_t car);
    int pov_index()const{return pov_index_;}
    // Fullscreen authored POV, default CameraSpring0/LookbackMode1. Null topology is an
    // explicit unbounded laboratory world, not an original missing-world fallback.
    // Memory is read by recovered math helpers; no physical state is written.
    DrivingCameraFrame update(Memory&,std::uint32_t car,float elapsed,bool look_back=false,
        const port::track::Topology* topology=nullptr,
        const port::track::VoronoiPolyhedron* collision_shape=nullptr);
private:
    int pov_index_=1;
    bool initialized_=false;
    float lookback_=0,touching_ground_=1,current_accel_=0,lag_adjust_=0,hold_time_=0,time_fract_=0;
    std::array<float,20> accel_history_{};
    std::uint16_t last_yaw_=0,boom_yaw_=0,boom_pitch_=0;
};
}

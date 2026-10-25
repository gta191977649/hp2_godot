#pragma once
// Port-owned typed projection of authorized Debug selection camera state.
// E-0098-C01. Not a PS2 ABI, allocator, Camera or original declaration header.
#include "game_core/port/driving/memory.hpp"
#include <array>
#include <functional>
namespace hp2::debug {
struct bQuaternion { float x=0,y=0,z=0,w=1; };
void bQuaternionToMatrix(std::array<float,16>&,const bQuaternion&);
void bMatrixToQuaternion(bQuaternion&,const std::array<float,16>&);
void MyQuaternionMultiply(bQuaternion&,const bQuaternion&,const bQuaternion&);
void NormalizeQuaternion(bQuaternion&);
void Slerp(::port::driving::Memory&,bQuaternion&,bQuaternion&,bQuaternion&,float);
}
namespace port::selection {
enum class CameraAudioOp { play,stop,volume,pitch };
struct CameraAudioRequest { CameraAudioOp operation;const char* sample;float value=0; };
struct CameraServices {
    bool sound_enabled=false;
    std::function<void(const CameraAudioRequest&)> audio;
    // Mirrors original Update -> Camera setters -> SetCameraMatrix invocation order.
    std::function<void(const std::array<float,16>&,float,int,float)> publish;
};
struct CameraFrame {
    std::array<float,16> view{};
    std::array<float,3> position{},target{},up{};
    int fov_bangle=0;
    float target_distance=0;
};
// Presentation-only normal Camera00156868 projection. Does not implement the
// remote developer link, camera velocity/history or original stop-updating global.
CameraFrame camera_frame_from_view(const std::array<float,16>&,int,float);
}
namespace hp2::debug {
struct SelectCarCameraMover {
    enum SelCarCamMode { SCC_STATS=0,SCC_ORBIT=1 };
    SelCarCamMode Mode=SCC_STATS;
    float ModeParam=0,ParamSpeed=0;
    std::uint16_t HAngleStats=0,HAngleOrbit=0,VAngleStats=0,VAngleOrbit=0;
    float RadiusStats=5;
    std::array<float,3> OffsetStats{},OffsetOrbit{};
    bQuaternion OrientationStats{},OrientationOrbit{};
    std::uint16_t FOVStats=10000,FOVOrbit=10000;
    float HOrbitSpeed=300,DesiredHOrbitSpeed=300,VOrbitSpeed=300,DesiredVOrbitSpeed=300;
    float Radius=13.199999809265137f,RadiusSpeed=0,DesiredRadiusSpeed=0;
    // Defaults project0015A150 scalar state only; original base/sample constructor
    // binding remains a separate host lifecycle responsibility.
    void SetOrientationStats(::port::driving::Memory&,std::uint16_t h,std::uint16_t v);
    void SetMode(SelCarCamMode,const ::port::selection::CameraServices&);
    void Update(::port::driving::Memory&,float elapsed,const ::port::selection::CameraServices&);
};
}

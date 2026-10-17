#include "game_core/indep/src/timer.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/wheel.hpp"
#include "game_core/indep/src/suspension.hpp"
#include "game_core/indep/src/rigidbody.hpp"
#include "game_core/indep/src/engine.hpp"

// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001A5FB0; explicit host partition; original TU unconfirmed
namespace port::driving::init {
void Sub_001A5FB0(ByteView out){hp2::recovered::Suspension::SetState(out);}
}

// Retail 0x001A5FC0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A5FC0(Memory& m,std::uint32_t object,ContactProvider& contact){hp2::recovered::Suspension(m,object).ComputeAuxiliaries(contact);}
}

// Retail 0x001A5D60; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A5D60(Memory& m,std::uint32_t s,std::uint16_t value) {hp2::recovered::TwoWheelAckermanSteering(m,s).SetSteering(value);}
}

// Retail 0x00153458; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_00153458(Memory& m,std::int32_t x,std::int32_t y) { return hp2::recovered::GetRoadNoiseGridValue(m,x,y); }
}

// Retail 0x00153498; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_00153498(Memory& m,std::uint32_t point,std::int32_t material) { return hp2::recovered::GetRoadNoise(m,point,material); }
}

// Retail 0x0018AC38; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0018AC38(Memory& m,std::uint32_t config,std::uint32_t engineconfig,
                  std::uint32_t out50,std::uint32_t out74,float f26) {return hp2::recovered::CalculateShiftPoints(m,config,engineconfig,out50,out74,f26);}
}

// Retail 0x0018A988; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0018A988(Memory& m,std::uint32_t s) {return hp2::recovered::Engine(m,s).ComputeAuxiliaries();}
}

// Retail 0x0018B008; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_0018B008(Memory& m,std::uint32_t s) {return hp2::recovered::DriveTrain(m,s).GetDifferentialAngularVelocity();}
}

// Retail 0x0018B0D8; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_0018B0D8(Memory& m,std::uint32_t s) {return hp2::recovered::DriveTrain(m,s).GetDriveWheelSlippage();}
}

// Retail 0x0018B178; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0018B178(Memory& m,std::uint32_t s,float value) {return hp2::recovered::DriveTrain(m,s).SetDifferentialAngularVelocity(value);}
}

// Retail 0x0018BC18; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018BC18(Memory& m,std::uint32_t s) {return hp2::recovered::DriveTrain(m,s).Idling();}
}

// Retail 0x0018BC68; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018BC68(Memory& m,std::uint32_t s) {return hp2::recovered::DriveTrain(m,s).ClutchMatched();}
}

// Retail 0x0018B4A0; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0018B4A0(Memory& m,std::uint32_t s) {return hp2::recovered::DriveTrain(m,s).ComputeAuxiliaries();}
}

// Retail 0x0018B698; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018B698(Memory& m,std::uint32_t s,std::uint32_t out) {return hp2::recovered::DriveTrain(m,s).GetDerivative(out);}
}

// Retail 0x0018C368; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018C368(){
    using namespace detail_group_0018f808;return 1;}
}

// Retail 0x0018C1C8; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018C1C8(){
    using namespace detail_group_0018f808;return 1;}
}

// Retail 0x0019CC88; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_0019CC88(Memory& m,std::uint32_t body,std::uint32_t position,std::uint32_t matrix,std::uint32_t velocity,std::uint32_t angular) {hp2::recovered::RigidBody(m,body).SetState(position,matrix,velocity,angular);}
}

// Retail 0x0019D848; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_0019D848(Memory& m,std::uint32_t body,float unused_stack_w) {hp2::recovered::RigidBody(m,body).ComputeAuxiliaries(unused_stack_w);}
}

// Retail 0x0019DA30; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_0019DA30(Memory& m) { return hp2::recovered::Sub_0019B358(m); }
}

// Retail 0x001395B0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001395B0(Memory& m) { return hp2::recovered::Sub_00139510(m); }
}

// Retail 0x001A33A8; explicit host partition; original TU unconfirmed
namespace port::driving::native {
float Sub_001A33A8(Memory& m,std::uint32_t value){return hp2::recovered::GetRealTimeElapsedFromQuantized(m,static_cast<std::int32_t>(value));}
}

// Retail 0x00136DD0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
std::uint32_t Sub_00136DD0(float f12){
    using namespace detail_group_0019da30;f12=f12*4000.0f;f12=f12+0.5f;return static_cast<std::uint32_t>(port::driving::cvt_w_s(f12));}
}

// Retail 0x001A33F0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A33F0(Memory& m,float f12,ScheduleServices& services){hp2::recovered::PrepareRealTimestep(m,f12,services);}
}

// Retail 0x001A3500; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A3500(Memory& m){hp2::recovered::AdvanceRealTime(m);}
}

// Retail 0x001A3598; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A3598(Memory& m){hp2::recovered::ResetWorldTime(m);}
}

// Retail 0x001A35E0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A35E0(Memory& m,float f12){hp2::recovered::PrepareWorldTimestep(m,f12);}
}

// Retail 0x001A35F0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A35F0(Memory& m){hp2::recovered::AdvanceWorldTime(m);}
}

// Retail 0x0011B6E0; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_0011B6E0(Memory&m,float f12) { return hp2::recovered::eSin(m,f12); }
}

// Retail 0x0011B7D8; explicit host partition; original TU unconfirmed
namespace port::driving {
SolverMatrix Sub_0011B7D8(Memory&m,std::uint16_t angle) { SolverMatrix out; hp2::recovered::eRotateZ(m,out,hp2::recovered::bIdentity(),angle); return out; }
}

// Retail 0x0011B950; explicit host partition; original TU unconfirmed
namespace port::driving {
SolverMatrix Sub_0011B950(Memory&m,std::uint16_t angle) { SolverMatrix out; hp2::recovered::eRotateY(m,out,hp2::recovered::bIdentity(),angle); return out; }
}

// Retail 0x001A45A8; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_001A45A8(Memory&m,std::uint32_t wheel,std::uint32_t normal_out,
                  std::uint32_t material_out,std::uint32_t point,std::uint32_t body,ContactProvider&provider){return hp2::recovered::Wheel(m,wheel).CheckTerrain(normal_out,material_out,point,body,provider);}
}

// Retail 0x001A4580; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_001A4580(Memory&m,std::uint32_t wheel,std::uint32_t point,std::uint32_t body,ContactProvider&provider){return hp2::recovered::Wheel(m,wheel).FindDrivingSurface(point,body,provider);}
}

// Retail 0x001A63C0; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_001A63C0(Memory&m,std::uint32_t,std::uint32_t out){return hp2::recovered::Suspension(m,0).GetDerivative(out);}
}

// Retail 0x001A57E0; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_001A57E0(Memory&m) { return hp2::recovered::Sub_001A2FC0(m); }
}

// Retail 0x0025AFA8; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0025AFA8(Memory&m) { return hp2::recovered::Sub_00254350(m); }
}

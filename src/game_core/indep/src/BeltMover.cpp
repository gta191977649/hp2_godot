// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\beltmover.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// Original translation unit: `..\indep\src\beltmover.cpp` (ELF 0x002BB048; E-0029-P01).
// Directory binding through 0x00204098; see reverse/classes/source-path-bindings.json.
// This per-class file is a port-side partition, not the original translation unit or header.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: confirmed original directory (E-0029-P01); per-class basename/header extension port-side.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): BeltMover::Move() [0x002BB078]
#include "game_core/indep/src/BeltMover.hpp"
#include "game_core/port/driving/ee_scalar.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00204098; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00204098.md.
void BeltMover::Sub_00204098() {
    hp2::not_implemented("0x00204098", "BeltMover::Sub_00204098 (signature and full body unresolved)");
}

// ELF 0x002043A0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/002043a0.md.
void BeltMover::Sub_002043A0() {
    hp2::not_implemented("0x002043A0", "BeltMover::Sub_002043A0 (signature and full body unresolved)");
}

// ELF 0x00204468; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00204468.md.
// TASK-0041 E-0041-V01; debug 0x001FF4D8: BeltMover::DoSnapshot(ReplaySnapshot *); retail 0x00204468.
void BeltMover::DoSnapshot() {
    hp2::not_implemented("0x00204468", "BeltMover::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x00204D78; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00204d78.md.
// TASK-0041 E-0041-V01; debug 0x001FFDE0: BeltMover::Move(float); retail 0x00204D78.
void BeltMover::Move() {
    hp2::not_implemented("0x00204D78", "BeltMover::Move (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/race/control_construction.hpp"
#include "game_core/indep/inc/CarMover.hpp"
#include "game_core/indep/src/point.hpp"
#include "game_core/indep/src/wheel.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/src/bMath.hpp"
namespace hp2::debug {
// Debug001FF108,E-0099-W100. Original source-owned Average/Spline/cubic/Wheel
// allocations; inline Point.hpp initialization leaves coefficients/time alone.
void BeltMover::Sub_001FF108(std::uint32_t car){auto& m=m_;const auto a=address_;m.word(a+8,car);for(unsigned off:{0x24u,0x20u,0x1cu,0x10u,0xcu,0x34u})m.word(a+off,0);m.word(a+4,0x2b2c90);
 const auto average=m.allocate(16);Average(m,average).Sub_0018DBF8(30);m.word(a+0x30,average);
 const auto spline=m.allocate(40);QuickSpline(m,spline).Sub_00236D88(2,0,20);m.word(a+0x14,spline);
 auto port_init_cubic=[&](std::uint32_t p){for(unsigned off:{0u,4u,8u,12u})m.word(p+off,0);m.half(p+0x2e,1);m.scalar(p+0x24,1);m.scalar(p+0x28,1);m.half(p+0x2c,0);};
 const auto u=m.allocate(0x30);port_init_cubic(u);m.word(a+0x18,u);
 const auto angle=m.allocate(0x60);port_init_cubic(angle);port_init_cubic(angle+0x30);m.word(a+0x28,angle);
 const auto slide=m.allocate(0x60);port_init_cubic(slide);port_init_cubic(slide+0x30);m.word(a+0x2c,slide);
 const float speed=m.scalar(car+0x3c4),acceleration=speed*(m.scalar(car+0xd8)*m.scalar(0x325d50));const auto heading=m.half(car+0x3a4),side=static_cast<std::uint16_t>(heading+0x4000);
 const float forward_cos=hp2::recovered::bCos(m,heading),forward_sin=hp2::recovered::bSin(m,heading);const std::array<float,2> velocity{{speed*forward_cos,speed*forward_sin}};
 const float side_cos=hp2::recovered::bCos(m,side),side_sin=hp2::recovered::bSin(m,side);const std::array<float,2> derivative{{acceleration*side_cos,acceleration*side_sin}};
 tCubic2D(m,angle).SetVal(velocity);tCubic2D(m,angle).SetdVal(derivative);m.word(slide+0x30+0x24,m.word(0x325d54));m.word(slide+0x24,m.word(0x325d54));
 const auto config=m.word(car+0xc);for(unsigned i=0;i<4;++i){const auto wheel=bOMalloc(m,m.word(0x327028));hp2::recovered::Wheel(m,wheel).port_initialize(config+0x120+0x20*i,config+0x1a0+0x20*(i/2),config+0x300,car+0x40);m.word(a+0x38+4*i,wheel);}ClearCalculatedVariables();
}
// Debug001FF410,E-0099-W100. Actual owner order, signed Wheel destructor
// descriptor and final base CarMover deletion flag.
void BeltMover::Sub_001FF410(int flags){auto& m=m_;const auto a=address_;m.word(a+4,0x2b2c90);if(const auto average=m.word(a+0x30))Average(m,average).Sub_0018DC70(3);if(const auto spline=m.word(a+0x14))QuickSpline(m,spline).Sub_00236E78(3);for(unsigned off:{0x18u,0x28u,0x2cu})if(const auto p=m.word(a+off))m.release(p);for(unsigned i=0;i<4;++i)if(const auto wheel=m.word(a+0x38+4*i)){const auto table=m.word(wheel+8),target=m.word(table+0xc),self=wheel+static_cast<std::int32_t>(m.signed_half(table+8));if(target!=0x1a3410)hp2::not_implemented("0x001FF410","original BeltMover wheel destructor target is not reconstructed");hp2::recovered::Wheel(m,self).Sub_001A3410(3);}CarMover(m,a).Sub_00136B28(flags);}
// Debug001FF4D0,E-0099-W100. Fresh MIPS JR RA/NOP: actual empty body.
void BeltMover::ClearCalculatedVariables(){}
}

#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include <cmath>
namespace hp2::debug {
// Debug001FF9F8,E-0099-W102. Actual target resampling then source control-point
// vector copy with W=0 and dirty flag. No host path generation or ownership swap.
void BeltMover::SetBelt(){auto& m=m_;const auto a=address_,car=m.word(a+8),driver=m.word(car+0x2c);m.half(m.word(a+0x18)+0x2e,0);DriveTarget(m,driver).Resample(car);const int count=m.signed_byte(driver);const auto spline=m.word(a+0x14);QuickSpline(m,spline).SetNumControlPoints(count);if(count<0||count>m.half(spline+0x18))hp2::not_implemented("0x001FF9F8","source target exceeds original owned spline control-point storage");for(int i=0;i<count;++i){const auto output=m.word(spline+0x20)+16*i;m.copy(output,driver+0x10+16*i,16);m.word(output+12,0);m.byte(spline+0x14,1);}FindCoordinates();}
// Debug001FF628,E-0099-W102. Source exact whole-vector subtraction; returned
// dot consumes XYZ only, so original Car position padding is not substituted.
float BeltMover::Dot(const std::array<float,4>& point,std::array<float,4>& to,std::array<float,4>& slope,float t){QuickSpline spline(m_,m_.word(address_+0x14));spline.GetPoint(to,t,0);spline.GetPoint(slope,t,1);for(unsigned i=0;i<4;++i)to[i]=to[i]-point[i];return (to[0]*slope[0]+to[1]*slope[1])+to[2]*slope[2];}
// Debug001FF6D8,E-0099-W102. Original empty-spline branch is genuinely zero;
// exceptional nonempty VU reciprocal sqrt operands remain explicit.
float BeltMover::GetAccelerationDueToGravity(){const auto spline=m_.word(address_+0x14);if(!m_.half(spline+0x1a))return 0;std::array<float,4> slope;QuickSpline(m_,spline).GetPoint(slope,m_.scalar(address_+0x1c),1);const float square=(slope[0]*slope[0]+slope[1]*slope[1])+slope[2]*slope[2];if(!(square>0)||!std::isfinite(square))hp2::not_implemented("0x001FF6D8","original VU spline-gravity reciprocal-square-root operand requires exact semantics");const float inverse=1.f/std::sqrt(square);return -m_.scalar(0x327ba0)*(slope[2]*inverse);}
// Debug001FF760,E-0099-W102. Actual acceleration owner; no feedback callback.
float BeltMover::GetMaximumAcceleration(){const float scale=m_.scalar(address_+0x10);if(scale==0)hp2::not_implemented("0x001FF760","original Belt parameter speed division by zero requires exact EE semantics");return CalculateMaxAcceleration(m_,m_.word(address_+8),m_.scalar(address_+0x20)/scale);}
// Debug001FF788,E-0099-W102. Source spline coordinates, actual driver descriptor
// getters, acceleration bounds and lateral cubic state. No movement integration.
void BeltMover::FindCoordinates(){auto& m=m_;const auto a=address_,car=m.word(a+8),driver=m.word(car+0x2c),spline=m.word(a+0x14);const unsigned count=m.half(spline+0x1a);const float length=m.scalar(driver+0x200);m.word(a+0xc,0);if(length==0||!std::isfinite(length))hp2::not_implemented("0x001FF788","original source spline scale zero/nonfinite division requires exact EE semantics");m.scalar(a+0x10,static_cast<float>(count)/length);QuickSpline curve(m,spline);const auto position=m.vector(car+0x60);const int closest=curve.FindClosestControlPoint(position,0);const float parameter=curve.FindClosestParam(position,static_cast<float>(closest),m.scalar(0x325d58));std::array<float,4> to,slope;(void)Dot(position,to,slope,parameter);m.scalar(a+0x1c,parameter);const float square=(slope[0]*slope[0]+slope[1]*slope[1])+slope[2]*slope[2];if(!(square>0)||!std::isfinite(square))hp2::not_implemented("0x001FF788","original VU spline-forward reciprocal-square-root operand requires exact semantics");const float inverse=1.f/std::sqrt(square);for(float& lane:slope)lane=lane*inverse;const float speed=m.scalar(a+0x10)*m.scalar(car+0x3c4);m.scalar(a+0x20,speed);
 auto desired=[&](bool time){const auto table=m.word(driver+0x22c),offset=time?0x90u:0x88u,target=m.word(table+offset+4),self=driver+static_cast<std::int32_t>(m.signed_half(table+offset));if(target!=(time?0x13aaf8u:0x13ab00u))hp2::not_implemented("0x001FF788","original Belt driver desired-speed/time virtual target is not reconstructed");return time?PsychoDriver(m,self).GetDesiredTimeToSpeed():PsychoDriver(m,self).GetDesiredSpeed();};const float requested=m.scalar(a+0x10)*desired(false)-speed;const float time=desired(true);if(!std::isfinite(requested)||!std::isfinite(time))hp2::not_implemented("0x001FF788","exceptional source desired-speed/time arithmetic requires exact EE semantics");m.scalar(a+0x24,requested/(0.f<time?desired(true):.5f));const float maximum=m.scalar(a+0x10)*GetMaximumAcceleration();if(!std::isfinite(m.scalar(a+0x24))||!std::isfinite(maximum))hp2::not_implemented("0x001FF788","exceptional source acceleration comparison requires exact EE semantics");if(m.scalar(a+0x24)<=maximum){const float minimum=m.scalar(a+0x10)* -20.f;if(m.scalar(a+0x24)<minimum)m.scalar(a+0x24,minimum);}else m.scalar(a+0x24,maximum);
 const std::array<float,3> right{{slope[1],-slope[0],0}};const float value=-((to[0]*right[0]+to[1]*right[1])+to[2]*right[2]);const auto u=m.word(a+0x18);m.scalar(u,value);if(value!=m.scalar(u+8))m.half(u+0x2c,2);const float derivative=((m.scalar(car+0x70)*right[0]+m.scalar(car+0x74)*right[1])+m.scalar(car+0x78)*right[2])*.5f;m.scalar(u+4,derivative);if(derivative!=m.scalar(u+12))m.half(u+0x2c,2);m.scalar(u+0x24,.5f);const auto angle=m.word(a+0x28);m.word(angle+0x30+0x24,m.word(0x325d5c));m.word(angle+0x24,m.word(0x325d5c));}
}

#include "game_core/indep/src/table.hpp"
namespace hp2::debug {
// Debug002005D8,E-0099-W106; exact _GLOBAL_$I$fBeltAccelCoef. Static imported
// point arrays/Graph headers only; no heap resource or invented close function.
void Sub_002005D8(::port::driving::Memory& m){m.scalar(0x3130a0,0);m.scalar(0x3130c0,0);m.scalar(0x3130a4,0);m.scalar(0x3130c4,0);m.scalar(0x3130a8,18);m.word(0x3130ac,m.word(0x325d8c));m.scalar(0x3130c8,14.5f);m.word(0x3130cc,m.word(0x325d90));m.scalar(0x3130b0,22);m.scalar(0x3130d0,16.5f);m.word(0x3130d4,m.word(0x325d94));m.word(0x3130b4,m.word(0x325d90));m.word(0x3130bc,m.word(0x325d98));m.scalar(0x3130dc,1);m.scalar(0x3130d8,30);m.scalar(0x3130b8,30);hp2::recovered::Graph(m,0x328868).Sub_0018D9E8(0x3130a0,4);hp2::recovered::Graph(m,0x328870).Sub_0018D9E8(0x3130c0,4);}
}

namespace hp2::debug {
// Debug00200410,E-0099-W106. Original whole-vector QuickSpline evaluation.
void BeltMover::GetPoint(std::array<float,4>& out,float t){QuickSpline(m_,m_.word(address_+0x14)).GetPoint(out,t*(t*m_.scalar(address_+0x24)+m_.scalar(address_+0x20))+m_.scalar(address_+0x1c),0);}
// Debug00200468,E-0099-W106. Original parameter derivative, all four lanes.
void BeltMover::GetDerivative(std::array<float,4>& out,float t){const float dd=m_.scalar(address_+0x24),dv=m_.scalar(address_+0x20);QuickSpline(m_,m_.word(address_+0x14)).GetPoint(out,t*(t*dd+dv)+m_.scalar(address_+0x1c),1);const float rate=(t+t)*dd+dv;for(float& x:out)x=x*rate;}
// Debug002004E8,E-0099-W106. Both source terms, original grouping and XYZW.
void BeltMover::GetSecondDerivative(std::array<float,4>& out,float t){const float dd=m_.scalar(address_+0x24),dv=m_.scalar(address_+0x20),rate=(t+t)*dd+dv,param=t*(t*dd+dv)+m_.scalar(address_+0x1c);std::array<float,4> second;QuickSpline curve(m_,m_.word(address_+0x14));curve.GetPoint(out,param,1);for(float& x:out)x=x*(dd+dd);curve.GetPoint(second,param,2);for(unsigned i=0;i<4;++i)out[i]=out[i]+second[i]*(rate*rate);}
// Debug001FFC40,E-0099-W106. Original nine ballistic samples and refresh branch.
float BeltMover::AverageAir(float seconds){auto& m=m_;const auto a=address_,info=m.word(m.word(a+8)+0xc);const float scale=m.scalar(a+0x10),mass=m.scalar(info+0x1e0);if(scale==0||mass==0)hp2::not_implemented("0x001FFC40","original AverageAir zero scale/mass EE division is unresolved");const float gravity=-(((m.scalar(a+0x20)/scale)*(m.scalar(info+0x2f4)+m.scalar(info+0x2f8))*.5f)/mass+m.scalar(0x327ba0));std::array<float,4> p,s;GetPoint(p,m.scalar(a+0xc));GetDerivative(s,m.scalar(a+0xc));float vz=s[2],height=p[2],last=0,sum=0;for(int i=1;i<10;++i){const float t=(seconds*static_cast<float>(i))*m.scalar(0x325d70),dt=t-last,future=m.scalar(a+0xc)+t,velocity=dt*gravity+vz;GetPoint(p,future);const float air=(dt*velocity+height)-p[2];if(0<air)sum=sum+air;else{GetDerivative(s,future);vz=s[2];height=p[2];last=t;}}return sum*m.scalar(0x325d74);}
// Debug001FFA98,E-0099-W106. Original old-state Graph selection and transitions.
float BeltMover::SlideCoef(float acceleration){auto& m=m_;const auto a=address_,info=m.word(m.word(a+8)+0xc),old=m.word(a+0x34);const auto acceleration_word=[&](){std::uint32_t word;std::memcpy(&word,&acceleration,4);return std::to_string(word);};if(!std::isfinite(acceleration))hp2::not_implemented("0x001FFA98",("original lateral acceleration nonfinite: acceleration_word="+acceleration_word()+" car="+std::to_string(m.word(a+8))+" scale_word="+std::to_string(m.word(a+0x10))+" speed_word="+std::to_string(m.word(a+0x20))).c_str());float sign;if(acceleration==0){if(!::port::driving::pc_sqrt_carrier_enabled())hp2::not_implemented("0x001FFA98",("original zero lateral acceleration DIV.S requires explicit PC numerical carrier; acceleration_word="+acceleration_word()).c_str());::port::diagnostics::warn("0x001FFAD8","source lateral acceleration divided by its absolute zero uses EE DIV.S signed maximum","using the explicitly enabled finite PC carrier; exact EE maximum encoding/flags remain unverified; original Graph evaluation and sign multiplication are preserved");sign=::port::driving::ee_div_s_zero(acceleration,std::fabs(acceleration),"0x001FFAD8");}else sign=acceleration/std::fabs(acceleration);const float mass=m.scalar(info+0x1e0),lateral=((mass*std::fabs(acceleration))*(m.scalar(info+0x1c8)+m.scalar(info+0x1a8)))*.5f;const auto friction=old?0x10u:0xcu;const float scale=m.scalar(a+0x10);if(scale==0)hp2::not_implemented("0x001FFA98","original SlideCoef speed/scale EE zero division is unresolved");const float force=(((m.scalar(a+0x20)/scale)*(m.scalar(info+0x2f4)+m.scalar(info+0x2f8))*.5f+m.scalar(0x327ba0)*mass)*(m.scalar(info+0x1c0+friction)+m.scalar(info+0x1a0+friction)))*.5f;const auto graph=old?0x328870u:0x328868u,points=m.word(graph);m.scalar(points+8,force*m.scalar(0x325d60));m.scalar(points+16,force*m.scalar(0x325d64));m.scalar(points+24,force*1.5f);const unsigned sliding=force<lateral;if(old!=sliding){const auto cubic=m.word(a+0x2c);if(!sliding){m.word(cubic+0x54,m.word(0x325d68));m.word(cubic+0x24,m.word(0x325d68));auto v=tCubic2D(m,cubic).GetdVal();for(float& x:v)x=x*m.scalar(0x325d6c);tCubic2D(m,cubic).SetdVal(v);}else{m.scalar(cubic+0x54,1);m.scalar(cubic+0x24,1);}m.word(a+0x34,sliding);}return sign*hp2::recovered::Graph(m,graph).GetValue(lateral);}
}

#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/inc/point.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/port/driving/projections.hpp"
#include <cstring>
namespace hp2::debug {
// Debug001FFDE0,E-0099-W106. Source Belt integration, physical contact and
// original Wheel/skid/RPM consumers. Explicit native unwritten avel.W policy.
// E-0099-W123: MIPS00200268/284/290 reads wheel radius config+0x12C, stride0x20.
float BeltMover::Move(float elapsed,::port::race::PhysicsConstructionServices& services){auto& m=m_;const auto a=address_;if(!services.contact)hp2::not_implemented("0x001FFDE0","source Belt Move requires actual topology contact");if(!services.port_stack_vector_lane)hp2::not_implemented("0x001FFDE0","source Belt angular-velocity unwritten W requires explicit native stack-lane policy");if(m.half(m.word(a+0x14)+0x1a)<2)SetBelt();const auto car=m.word(a+8),config=m.word(car+0xc);const auto old_position=m.vector(car+0x60);if(m.scalar(0x325d78)<AverageAir(.5f))m.word(m.word(car+0x1c)+0x238,static_cast<std::uint32_t>(::port::driving::cvt_w_s(m.scalar(0x325d7c))));m.scalar(a+0xc,m.scalar(a+0xc)+elapsed);const float time=m.scalar(a+0xc),scale=m.scalar(a+0x10);if(scale==0||elapsed==0)hp2::not_implemented("0x001FFDE0","original Belt Move zero scale/elapsed EE division requires exact semantics");const float speed=(m.scalar(a+0x20)+m.scalar(a+0x24)*time)/scale,future_offset=m.scalar(0x325d80);std::array<float,4> orientation,future_forward,accel;GetDerivative(orientation,time+.5f);const auto angle=m.word(a+0x28);tCubic2D(m,angle).SetValDesired({{orientation[0],orientation[1]}});cPoint::SplineSeek(m,angle,elapsed);const auto angle_value=tCubic2D(m,angle).GetVal();orientation[0]=angle_value[0];orientation[1]=angle_value[1];const float future_time=time+future_offset;GetDerivative(future_forward,future_time);
 auto right=[](const std::array<float,4>& forward){std::array<float,4> result{{forward[1]*1.f-forward[2]*0.f,forward[2]*0.f-forward[0]*1.f,forward[0]*0.f-forward[1]*0.f,forward[3]}};const float square=(result[0]*result[0]+result[1]*result[1])+result[2]*result[2];if(!(square>0)||!std::isfinite(square))hp2::not_implemented("0x001FFDE0","original Belt VU right-vector reciprocal sqrt domain is unresolved");const float inverse=1.f/std::sqrt(square);for(float& lane:result)lane=lane*inverse;return result;};
 const auto future_right=right(future_forward);GetSecondDerivative(accel,future_time);Average(m,m.word(a+0x30)).Record((future_right[0]*accel[0]+future_right[1]*accel[1])+future_right[2]*accel[2]);const auto u=m.word(a+0x18);(void)tCubic1D::GetddVal(m,u,m.scalar(u+0x20)/m.scalar(u+0x24));const float slide_coefficient=SlideCoef(m.scalar(m.word(a+0x30)+8));auto slide=future_right;const float slide_scale=speed*(slide_coefficient*future_offset);for(float& lane:slide)lane=lane*slide_scale;const auto slide_cubic=m.word(a+0x2c);tCubic2D(m,slide_cubic).SetValDesired({{slide[0],slide[1]}});cPoint::SplineSeek(m,slide_cubic,elapsed);const auto slide_value=tCubic2D(m,slide_cubic).GetVal();slide[0]=slide_value[0];slide[1]=slide_value[1];for(unsigned i=0;i<4;++i)orientation[i]=orientation[i]+slide[i];const auto heading=static_cast<std::uint16_t>(hp2::recovered::bATan(m,orientation[0],orientation[1])),old_heading=m.half(car+0x3a4);m.half(car+0x3a4,heading);std::array<float,4> forward,position;GetDerivative(forward,time);const auto current_right=right(forward);m.scalar(u+8,-slide_coefficient);if(-slide_coefficient!=m.scalar(u))m.half(u+0x2c,2);cPoint::SplineSeek(m,u,elapsed,0,0);GetPoint(position,time);for(unsigned i=0;i<4;++i)position[i]=current_right[i]*m.scalar(u)+position[i];m.vector(car+0x60,position);hp2::recovered::Car vehicle(m,car);vehicle.PutFlatOnTrack(*services.contact);hp2::recovered::CarState(m,car+0x40).CalculateGeometryPosition();float port_angular_w;const auto port_angular_bits=*services.port_stack_vector_lane;std::memcpy(&port_angular_w,&port_angular_bits,sizeof(port_angular_w));const auto difference=static_cast<std::int16_t>(static_cast<std::uint16_t>(heading-old_heading));m.vector(car+0xd0,{{0,0,(static_cast<float>(difference)*m.scalar(0x325d84))/elapsed,port_angular_w}});auto velocity=m.vector(car+0x60);for(unsigned i=0;i<4;++i)velocity[i]=velocity[i]-old_position[i];if(m.scalar(0x325d88)<std::fabs(elapsed)){const float inverse=1.f/elapsed;for(float& lane:velocity)lane=lane*inverse;}else{const auto normalized=hp2::recovered::bNormalize(std::array<float,3>{{velocity[0],velocity[1],velocity[2]}},speed);for(unsigned i=0;i<3;++i)velocity[i]=normalized[i];}m.vector(car+0x70,velocity);const float magnitude=std::sqrt((velocity[0]*velocity[0]+velocity[1]*velocity[1])+velocity[2]*velocity[2]);m.scalar(car+0x3c0,magnitude);m.scalar(car+0x3c4,magnitude);m.word(car+0x440,0);auto dot=[&](std::uint32_t p){return (velocity[0]*m.scalar(p)+velocity[1]*m.scalar(p+4))+velocity[2]*m.scalar(p+8);};const float longitudinal=speed-dot(car+0x90),lateral=-dot(car+0xa0);for(unsigned i=0;i<4;++i){const auto wheel=m.word(a+0x38+4*i),wi=m.word(wheel+0xc);const auto steering=m.signed_word(wi+0x14)<2?m.half(m.word(car+0x2c)+0x210):std::uint16_t(0);const float radius=m.scalar(config+0x12c+0x20*i);if(radius==0)hp2::not_implemented("0x001FFDE0","original Belt Wheel radius zero EE division requires exact semantics");hp2::recovered::Wheel(m,wheel).SetState(car+0x40,config,m.scalar(car+0x300+4*i)+m.scalar(car+0x2f0+4*i)*time,speed/radius,steering,0,*services.contact);m.scalar(car+0x374+8*i,longitudinal);m.scalar(car+0x378+8*i,lateral);m.scalar(car+0x394+4*i,0);if(m.signed_word(wi+0x14)>1){m.scalar(wheel+0x100,longitudinal);m.scalar(wheel+0x104,lateral);m.byte(wheel+0x154,static_cast<std::uint8_t>(m.word(a+0x34)));m.scalar(wheel+0x160,longitudinal);}}
 if(services.enable_skid_trails&&vehicle.ShouldVisualEffectsBeGenerated()&&World(m,m.word(0x3272f0)).GetDistanceToClosestPlayer(std::array<float,3>{{m.scalar(car+0x60),m.scalar(car+0x64),m.scalar(car+0x68)}})<200.f){::port::driving::SkidServices skid;skid.contact=services.contact;skid.shared_world=true;skid.port_stack_vector_lane=services.port_stack_vector_lane;for(unsigned i=0;i<4;++i)hp2::recovered::Wheel(m,m.word(a+0x38+4*i)).MaybeMakeSkids(elapsed,car,skid);}vehicle.ComputeFakeRPM();return elapsed;}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug001FF4D8,E-0099-W110. Original section/order, full cubic bytes,
// XYZ spline codec, Average and all four actual Wheel snapshot children.
void BeltMover::DoSnapshot(std::uint32_t snapshot){auto& m=m_;const auto a=address_;ReplaySnapshot s(m,snapshot);s.BeginSection(0x2b2c68);CarMover(m,a).DoSnapshot(snapshot);for(unsigned off:{0x1cu,0x20u,0x24u,0xcu,0x10u})s.MoveData(a+off,4);s.MoveData(a+0x34,1,4,0);s.MoveData(m.word(a+0x18),0x30);s.MoveData(m.word(a+0x28),0x60);s.MoveData(m.word(a+0x2c),0x60);QuickSpline(m,m.word(a+0x14)).DoSnapshot(snapshot,3);Average(m,m.word(a+0x30)).DoSnapshot(snapshot);for(unsigned i=0;i<4;++i)hp2::recovered::Wheel(m,m.word(a+0x38+4*i)).DoSnapshot(snapshot);if(m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d))ClearCalculatedVariables();s.EndSection();}
}

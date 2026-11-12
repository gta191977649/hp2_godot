// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\race.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): Race::Update() [0x002B0020]
#include "game_core/Race.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/Race.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00148218; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00148218.md.
void Race::SortRacingCarsByLeadership() {
    hp2::not_implemented("0x00148218", "Race::SortRacingCarsByLeadership (signature and full body unresolved)");
}

// ELF 0x001491C0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001491c0.md.
void Race::GetFinisher() {
    hp2::not_implemented("0x001491C0", "Race::GetFinisher (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001493B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001493b8.md.
void RacingCar::Sub_001493B8() {
    hp2::not_implemented("0x001493B8", "RacingCar::Sub_001493B8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00146AE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00146ae8.md.
void TimeAroundTrackTable::GetTimeTableEntry() {
    hp2::not_implemented("0x00146AE8", "TimeAroundTrackTable::GetTimeTableEntry (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/timer.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/inc/CarDriver.hpp"
#include "game_core/port/race/control_construction.hpp"
#include "game_core/port/driving/projections.hpp"
#include <cmath>
namespace port::race {
// Port dispatch for the original GetDesiredSpeed vtable slot. The only two
// Debug concrete implementations are source-backed; unknown targets stay loud.
static float racing_desired_speed(::port::driving::Memory& m,std::uint32_t driver){const auto v=m.word(driver+0x22c);const auto adjust=m.signed_half(v+0x88);const auto target=m.word(v+0x8c);const auto self=driver+static_cast<std::int32_t>(adjust);if(target==0x136bd0)return hp2::debug::CarDriver(m,self).GetDesiredSpeed();if(target==0x13ab00)return hp2::debug::PsychoDriver(m,self).GetDesiredSpeed();hp2::not_implemented("0x00146DC0","RacingCar GetDesiredSpeed virtual target is not recovered");}
}
namespace hp2::debug {
// Debug00149230; E-0099-W34. Original Race.hpp inline ctor emitted in race.cpp.
// Only Timer members are initialized: all other original bytes stay untouched.
void RacingCar::Sub_00149230(){for(unsigned a:{0x24u,0x38u,0x4cu,0x60u,0x74u})for(unsigned i=0;i<5;++i)m_.word(address_+a+4*i,0);for(unsigned i=0;i<11;++i)m_.word(address_+0x88+4*i,0);m_.word(address_+0xb4,0);}
// Debug00146A78; E-0099-W34. Confirmed original empty non-owning method.
void RacingCar::UnInit(){}
// Debug001473C0; E-0099-W34.
std::uint32_t RacingCar::GetPlayer()const{const auto car=m_.word(address_);return car?m_.word(car+0x18):0;}
// Debug00147B60, ComparePercentDistances002034D0; E-0099-W34.
float Race::GetLapNumberExact(float d){float value=TrackPathManager::ComparePercentDistances(d,m_.scalar(address_+0x654+4*m_.signed_byte(address_+0x653)));if(value<0)value=value+100;return value*m_.scalar(0x324f04);}
// Debug00146C48; E-0099-W34. Original shortest-percent ties and time quantization.
int RacingCar::CheckBoundaryCrossing(BoundaryCrossingInfo& out,float distance,float boundary){const float current=TrackPathManager::ComparePercentDistances(distance,boundary),previous=TrackPathManager::ComparePercentDistances(m_.scalar(address_+0x1c),boundary);out.CrossedForwards=0;out.CrossedBackwards=0;out.TimeSinceCrossing=0;if(25<std::abs(current))return 0;if(previous<0&&current>=0)out.CrossedForwards=1;else if(previous>=0&&current<0)out.CrossedBackwards=1;else return 0;const float speed=m_.scalar(m_.word(address_)+0x40+0x380);float time=0;if(1<speed)time=(current*m_.scalar(0x313308)*m_.scalar(0x324ee4))/speed;out.TimeSinceCrossing=::port::driving::cvt_w_s(time*4000+0.5f);return 1;}
// Debug001469A0; E-0099-W34. Full init calls real UpdateLapCounts before NumLapsLed.
void RacingCar::Init(std::uint32_t car){auto& m=m_;const auto a=address_;m.word(car+0x1c,a);m.byte(a+0xa,0);m.byte(a+0xb,0);m.byte(a+0xc,0);m.scalar(a+0x20,0);m.word(a,car);m.half(a+0x12,0);m.scalar(a+0x1c,m.scalar(car+0x7f0)*100);for(unsigned i=0;i<5;++i){m.word(a+0x24+4*i,0);m.word(a+0x38+4*i,0);m.word(a+0x4c+4*i,0);m.word(a+0x60+4*i,0x7fffffff);m.word(a+0x74+4*i,0x7fffffff);}for(unsigned i=0;i<11;++i)m.word(a+0x88+4*i,0);m.word(a+0xb4,0);m.scalar(a+0xb8,0);m.scalar(a+0xbc,0);m.byte(a+9,0);m.byte(a+8,0xff);m.half(a+6,0xffff);m.half(a+4,0xffff);UpdateLapCounts();m.half(a+0x10,0);}
// Debug00147C90; E-0099-W34. Original increment/publication follows successful Init.
void Race::AddRacingCar(std::uint32_t car){const auto n=m_.signed_word(address_+0x18);if(n<0||n>=8)throw std::out_of_range("Race original RacingCarTable exceeds8 entries");RacingCar(m_,address_+0x24+192*n).Init(car);m_.word(address_+0x18,n+1);m_.word(address_+0x624+4*n,address_+0x24+192*n);}
// Debug00146DC0; E-0099-W34. Actual boundary/lap/timer/top-speed state sequence.
void RacingCar::UpdateLapCounts(){
 auto& m=m_;const auto a=address_,car=m.word(a);auto race=m.word(0x327258);if(!race)hp2::not_implemented("0x00146DC0","original pCurrentRace is not constructed");const auto replay=m.word(0x3271a8);if(!replay)hp2::not_implemented("0x00146DC0","original ReplayManager is not constructed");
 m.byte(a+0xd,0);m.byte(a+0xe,0);const float distance=m.scalar(car+0x7f0)*100;
 const int count=m.signed_half(race+0x650);if(count>5)throw std::out_of_range("Race authored boundary count exceeds5 timer entries");
 for(int i=0;i<count;++i){BoundaryCrossingInfo crossing;if(!CheckBoundaryCrossing(crossing,distance,m.scalar(race+0x654+4*i)))continue;race=m.word(0x327258);
  if(crossing.CrossedForwards){if(i==m.signed_byte(race+0x653)){m.byte(a+8,static_cast<std::uint8_t>(m.byte(a+8)+1));if(m.signed_byte(a+9)<m.signed_byte(a+8)){m.byte(a+9,m.byte(a+8));m.byte(a+0xd,1);}}
   const auto region=static_cast<std::int16_t>(m.signed_byte(a+8)*5+i);m.half(a+4,static_cast<std::uint16_t>(region));
   if(m.signed_half(a+6)<region){m.byte(a+0xe,1);m.half(a+6,static_cast<std::uint16_t>(region));Timer time;time.PackedTime=static_cast<std::int32_t>(m.word(0x327cf8)-static_cast<std::uint32_t>(crossing.TimeSinceCrossing));time.RoundTime(m,m.scalar(0x324ee8));const auto rounded=static_cast<std::uint32_t>(time.PackedTime);const auto first=m.signed_word(a+0x24);const bool valid=first!=0&&first!=0x7fffffff;
    if(!m.word(replay)){if(valid){m.word(a+0x4c+4*i,m.word(a+0x38+4*i));m.word(a+0x38+4*i,rounded-static_cast<std::uint32_t>(first));m.word(a+0x74+4*i,m.word(a+0x60+4*i));if(m.signed_word(a+0x38+4*i)<m.signed_word(a+0x60+4*i))m.word(a+0x60+4*i,m.word(a+0x38+4*i));if(i!=m.signed_byte(race+0x653)){m.word(a+0x24+4*i,rounded);continue;}if(m.byte(a+8)<11)m.word(a+0x88+4*m.signed_byte(a+8),m.word(a+0x38+4*i));}}
    else {if(m.signed_word(0x311f98)!=2)continue;if(i!=m.signed_byte(race+0x653))continue;if(valid)m.word(a+0x38+4*i,rounded-static_cast<std::uint32_t>(first));}
    m.word(a+0x24+4*i,rounded);
   }
  }else if(i==m.signed_byte(race+0x653))m.byte(a+8,static_cast<std::uint8_t>(m.byte(a+8)-1));
 }
 if(!m.word(replay)){
  if(m.signed_word(car+0x40+0x37c)>2){const float previous=m.scalar(a+0xb8);float speed;
   if(!GetPlayer()){const float desired=::port::race::racing_desired_speed(m,m.word(car+0x2c));const float boost=m.scalar(car+0x40+0x3a0);const float limited=desired/(boost<1?1:boost);const float forward=std::abs(m.scalar(car+0x40+0x380));const float selected=limited<forward?limited:forward;const float limiter=m.scalar(m.word(car+12)+0x320)*m.scalar(0x324eec);const float chosen=selected<limiter?selected:limiter;const float top=chosen<previous?previous:chosen;speed=(top>=0&&top<=1000)?top:previous;}
   else {const float forward=m.scalar(car+0x40+0x380);speed=forward<previous?previous:forward;}if(speed!=previous)m.scalar(a+0xb8,speed);
  }
  const float seconds=static_cast<float>(m.signed_word(race+0x10))*m.scalar(0x324ef0);if(seconds>0)m.scalar(a+0xbc,m.scalar(car+0x40+0x36c)/seconds);
 }
 const float previous=m.scalar(a+0x14);const float next=static_cast<float>(m.signed_byte(a+8))+Race(m,race).GetLapNumberExact(distance);const float elapsed=m.scalar(0x327d08);m.scalar(a+0x14,next);m.scalar(a+0x20,next);if(elapsed>0)m.scalar(a+0x18,m.scalar(a+0x18)*m.scalar(0x324ef4)+((next-previous)/elapsed)*m.scalar(0x324ef8));m.scalar(a+0x1c,distance);
}
}

#include "game_core/port/track/position_markers.hpp"
#include "game_core/indep/src/fmenus.hpp"
namespace hp2::debug {
// Debug00147800, E-0099-W34. Exact first-marker/default branch and one wrap.
int Race::AddBoundaryPosition(const std::string& name,std::array<float,2>& position,float fallback,const ::port::track::RouteData& routes){position={{0,0}};if(fallback>100)fallback=fallback-100;const auto marker=GetTrackPositionMarker(m_,hp2::frontend::bStringHash(name.c_str()),0);int n=m_.signed_half(address_+0x650);if(!marker&&fallback<0)return -1;if(n<0||n>=5)throw std::out_of_range("Race boundary table exceeds5 entries");float distance=fallback;if(marker){position={{m_.scalar(marker+16),m_.scalar(marker+20)}};::port::driving::Memory::Temporary tp(m_,40);TrackPathCoordinate::Sub_002035A0(m_,tp.address());TrackPathCoordinate::UpdatePosition(m_,tp.address(),routes.paths,position);distance=m_.scalar(tp.address()+0x10)*100;TrackPathCoordinate::Sub_00203630(m_,tp.address(),2);}m_.scalar(address_+0x654+4*n,distance);m_.half(address_+0x650,static_cast<std::uint16_t>(n+1));return n;}
// Debug00147928, E-0099-W34. Reverse split names/order differ deliberately.
void Race::FigureOutBoundaries(const ::port::track::RouteData& routes){auto& m=m_;const auto a=address_;m.half(a+0x650,0);const bool reverse=m.signed_word(0x311f84)!=0;std::array<float,2> position;const int start=AddBoundaryPosition(reverse?"BackwardStartLine":"ForwardStartLine",position,5,routes);m.byte(a+0x652,static_cast<std::uint8_t>(start));m.scalar(a+0x668,position[0]);m.scalar(a+0x66c,position[1]);const float begin=m.scalar(a+0x654+4*start);if(!reverse){AddBoundaryPosition("SplitTimeBoundary1",position,begin+33,routes);AddBoundaryPosition("SplitTimeBoundary2",position,begin+66,routes);AddBoundaryPosition("SplitTimeBoundary3",position,-1,routes);}else{AddBoundaryPosition("SplitTimeBoundary3",position,-1,routes);AddBoundaryPosition("SplitTimeBoundary2",position,begin+33,routes);AddBoundaryPosition("SplitTimeBoundary1",position,begin+66,routes);}if(!m.word(0x311fa0)){m.byte(a+0x653,m.byte(a+0x652));m.word(a+0x670,m.word(a+0x668));m.word(a+0x674,m.word(a+0x66c));return;}const int finish=AddBoundaryPosition(reverse?"BackwardFinishLine":"ForwardFinishLine",position,begin+95,routes);m.byte(a+0x653,static_cast<std::uint8_t>(finish));m.scalar(a+0x674,position[1]);m.scalar(a+0x670,position[0]);}
// Debug001473F0, E-0099-W34. Publishes original owner before boundary queries.
void Race::Sub_001473F0(const ::port::track::RouteData& routes){auto& m=m_;const auto a=address_;m.word(a+0xc,0);m.word(a+0x10,0);m.word(a+0x14,0);for(unsigned i=0;i<8;++i)RacingCar(m,a+0x24+192*i).Sub_00149230();m.word(0x327258,a);m.word(a,0);const auto laps=m.word(0x311fa4);m.word(a+0x644,laps);if(laps&&m.word(0x311fa0))m.word(a+0x644,1);m.word(a+0x648,0);m.word(a+0x64c,0);m.word(a+0xc,0);m.word(a+0x10,0);m.word(a+0x18,0);m.word(a+0x1c,0);m.word(a+0x20,0);for(auto off:{0x67cu,0x678u,0x680u,0x684u,8u,4u,0x668u,0x66cu,0x670u,0x674u})m.word(a+off,0);m.word(a+0x14,0x7fffffff);FigureOutBoundaries(routes);m.word(a+0x6b0,0);}
// Debug001474E8, E-0099-W34/W56. Real time-table lifetime; active ghost
// ownership remains explicit. Original RacingCar.UnInit is genuinely empty.
void Race::Sub_001474E8(int flags){const int count=m_.signed_word(address_+0x18);if(count<0||count>8)throw std::out_of_range("Race racing-car close count outside8");for(int i=0;i<count;++i)RacingCar(m_,address_+0x24+192*i).UnInit();if(m_.word(address_+0x678)||m_.word(address_+0x684))hp2::not_implemented("0x001474E8","active GhostCarData teardown requires actual owner");if(const auto table=m_.word(address_+0x6b0))TimeAroundTrackTable(m_,table).Sub_00149340(3);m_.word(0x327258,0);if(flags&1)m_.release(address_);}
}

namespace hp2::debug {
// Debug00149088; E-0099-W37. Original signed DriverNumber and stagger direction.
bool Race::GracePeriodIsActive(std::uint32_t car)const{float period=m_.scalar(0x327268);const int number=m_.signed_half(car+0xa);if(static_cast<std::uint32_t>(number-101)<49)period=period+static_cast<float>(100-number)*m_.scalar(0x32726c);return static_cast<float>(static_cast<std::int32_t>(m_.word(0x327cf8)-m_.word(address_+12)))*m_.scalar(0x324f34)<=period;}
}

namespace hp2::debug {
// Debug00148E50; E-0099-W47. Exact eRaceStage and BeACop data-type offsets.
bool Race::CanAICarsStartDriving()const{return m_.signed_word(address_)==4||(m_.signed_word(0x3121ec)!=0&&m_.signed_word(address_)>1);}
}

#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include "game_core/indep/src/timer.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/indep/inc/RealDriver.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/port/race/physics_construction.hpp"
namespace hp2::debug {
// Debug001468D8/00146980; E-0099-W56. Original truncation/wrap and optional
// fractional time output; entry pointers remain in the real shared allocation.
std::uint32_t TimeAroundTrackTable::GetTimeTableEntry(float lap,float* seconds){if(lap<0)lap=0;if(!std::isfinite(lap)||lap>=2147483648.f)hp2::not_implemented("0x001468D8","exceptional EE lap conversion is outside scoped finite carrier");const int whole=static_cast<int>(lap);const float fraction=lap-static_cast<float>(whole);const int index=static_cast<int>(fraction*256.f);if(index<0||index>=256)hp2::not_implemented("0x001468D8","original table index outside256 entries");const auto entry=address_+4+8*index;if(seconds){const float numerator=fraction-static_cast<float>(index)*0x1p-8f,denominator=m_.scalar(entry);const float part=denominator==0?::port::driving::ee_div_s_zero(numerator,denominator,"0x00146968"):numerator/denominator;*seconds=(static_cast<float>(whole)*m_.scalar(address_)+m_.scalar(entry+4))+part;}return entry;}
float TimeAroundTrackTable::GetTimeAroundTrack(float lap){float seconds=0;GetTimeTableEntry(lap,&seconds);return seconds;}
void TimeAroundTrackTable::Sub_00149340(int flags){if(flags&1)m_.release(address_);}
// Debug00146590; E-0099-W56. Original class-line selection, route-list order,
// first positive speed per bin and forward-fill of all256 original entries.
void TimeAroundTrackTable::Sub_00146590(int car_class,const ::port::track::RouteData& data){auto& m=m_;const auto a=address_;if(car_class<0||car_class>10)hp2::not_implemented("0x00146590","original class-selected line outside21 reviewed authored lines");for(unsigned i=0;i<0x800;++i)m.byte(a+4+i,0);m.scalar(a,0);const auto race=m.word(0x327258);const auto divide=[&](float numerator,float denominator,const char* site){const float result=denominator==0?::port::driving::ee_div_s_zero(numerator,denominator,site):numerator/denominator;if(!std::isfinite(result)||std::fpclassify(result)==FP_SUBNORMAL)hp2::not_implemented(site,"EE overflow/underflow result requires separately established scalar projection");return result;};const unsigned line=2*static_cast<unsigned>(car_class);for(auto route=m.word(0x30bb30);route!=0x30bb30;route=m.word(route)){if(!(m.word(route+0x28+0x30*line)&3))continue;::port::driving::Memory::Temporary coordinate(m,40);TrackPathCoordinate::Sub_002035A0(m,coordinate.address());for(int i=0;i<m.signed_word(route+0x10)-1;++i){const auto first=route+0x428+0x70*i,second=first+0x70;const float first_offset=static_cast<float>(static_cast<int>(m.signed_half(first+0x30+2*line))*256)*0x1p-16f,second_offset=static_cast<float>(static_cast<int>(m.signed_half(second+0x30+2*line))*256)*0x1p-16f;const auto p=TrackRoutePoint(m,first).GetPosition(0,first_offset),q=TrackRoutePoint(m,second).GetPosition(0,second_offset);if(!race)hp2::not_implemented("0x00146590","actual current Race is required when converting a route segment");TrackPathCoordinate::UpdatePosition(m,coordinate.address(),data.paths,p);float before=Race(m,race).GetLapNumberExact(m.scalar(coordinate.address()+0x10)*100.f);TrackPathCoordinate::UpdatePosition(m,coordinate.address(),data.paths,q);float after=Race(m,race).GetLapNumberExact(m.scalar(coordinate.address()+0x10)*100.f);float distance=after-before;if(.5f<distance){before=before+1.f;distance=after-before;}else if(.5f<before-after){after=after+1.f;distance=after-before;}const float x=p[0]-q[0],y=p[1]-q[1],length=std::sqrt(x*x+y*y);const float speed=static_cast<float>(std::uint32_t(m.byte(first+0x5a+line))<<16)*0x1p-16f;const float time=divide(length,speed,"0x001467D0"),rate=divide(distance,time,"0x001467D8");const auto entry=GetTimeTableEntry(before);if(0.f<rate&&m.scalar(entry)==0)m.scalar(entry,rate);}TrackPathCoordinate::Sub_00203630(m,coordinate.address(),2);}float total=0,previous=m.scalar(0x324edc);for(unsigned i=0;i<256;++i){const auto entry=a+4+8*i;float rate=m.scalar(entry);m.scalar(entry+4,total);if(rate==0){m.scalar(entry,previous);rate=previous;}total=total+divide(0x1p-8f,rate,"0x0014687C");previous=rate;}m.scalar(a,total);}
// Debug00146BF8; E-0099-W56. Source fake crossing is a named original action,
// not a port-generated race result; round actual WorldTimer before publication.
void RacingCar::FakeStartlineCrossing(){auto& m=m_;m.word(address_+0x24,m.word(0x327cf8));Timer timer{m.signed_word(address_+0x24)};timer.RoundTime(m,m.scalar(0x324ee0));m.word(address_+0x24,static_cast<std::uint32_t>(timer.PackedTime));m.half(address_+6,0);if(m.word(0x311fa0)){m.half(address_+4,0);m.byte(address_+8,0);}}
// Debug00148020; E-0099-W56. Original finishing-order/progress adjacent swaps,
// with stable ties and separate previous/current race-position publication.
void Race::SortRacingCarsByLeadership(){auto& m=m_;const auto a=address_;const int count=m.signed_word(a+0x18);if(count<0||count>8)hp2::not_implemented("0x00148020","original racing pointer count exceeds8");bool changed;do{changed=false;for(int i=0;i<count-1;++i){const auto first=m.word(a+0x624+4*i),second=m.word(a+0x624+4*(i+1));const int order=m.signed_byte(first+0xc),next=m.signed_byte(second+0xc);const bool swap=order==0?(next==0?m.scalar(first+0x20)<m.scalar(second+0x20):next<i+2):i+1<order;if(swap){m.word(a+0x624+4*i,second);m.word(a+0x624+4*(i+1),first);changed=true;}}}while(changed);for(int i=0;i<m.signed_word(a+0x18);++i){const auto car=m.word(a+0x624+4*i);const auto previous=m.byte(car+0xa);m.byte(car+0xa,static_cast<std::uint8_t>(i+1));m.byte(car+0xb,previous);}}
// Debug00147BC8. Separate player and AI traversals, followed by two source sorts.
void Race::AddRacingCars(){auto& m=m_;const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x00147BC8","actual current World table is required");for(int i=0;i<m.signed_word(world+0x20);++i)AddRacingCar(m.word(world+0x2c+4*i));for(int i=0;i<m.signed_word(world+0x24)-m.signed_word(world+0x20);++i)AddRacingCar(m.word(world+0x2c+4*(m.signed_word(world+0x20)+i)));SortRacingCarsByLeadership();SortRacingCarsByLeadership();}
// Debug00147CF8. Preserve original stage side effects and original real/AI
// driver state. Traffic transition delegates the concrete original Car bridge.
void Race::SetRaceStage(int stage,::port::race::PhysicsConstructionServices& physics){auto& m=m_;const auto a=address_;m.word(a,static_cast<std::uint32_t>(stage));if(stage==4)FrontendDatabase::TheRaceIsOn(m,0x2f3890,a);for(int i=0;i<m.signed_word(a+0x18);++i){const auto racing=m.word(a+0x624+4*i),car=m.word(racing),driver=m.word(car+0x2c);const auto control=m.word(car+0x20);if(control==1){if(stage==4)hp2::recovered::RealDriver(m,driver).DisableStartLineState();else m.word(driver+0x23c,1);}else if(control==2&&stage==4){m.scalar(driver+0x21c,0);m.scalar(driver+0x214,1);m.scalar(driver+0x218,0);}if(stage==4)RacingCar(m,racing).FakeStartlineCrossing();}if(stage==3){m.word(a+4,0);m.word(a+8,m.word(0x327d04));}else if(stage==4){m.word(a+0xc,m.word(0x327cf8));Timer timer{m.signed_word(a+0xc)};timer.RoundTime(m,m.scalar(0x324f08));m.word(a+0xc,static_cast<std::uint32_t>(timer.PackedTime));m.word(a+8,0);m.word(a+4,m.word(0x327d04));}if(static_cast<std::uint32_t>(stage-3)<2){const auto world=m.word(0x3272f0);const int start=m.signed_word(world+0x28),count=m.signed_word(world+0x1c)-start;for(int i=0;i<count;++i){const auto car=m.word(world+0x2c+4*(start+i));m.byte(car+0x4a8,1);hp2::recovered::Car(m,car).SetMovementMode(2,physics);}}}
// Debug00147768/001483E8; E-0099-W56.
void Race::Init(int stage,const ::port::track::RouteData& routes,::port::race::PhysicsConstructionServices& physics){AddRacingCars();SetRaceStage(stage,physics);auto& m=m_;const int index=m.word(0x3121ec)&&m.signed_word(0x311f78+0x30)==2?1:0;const auto car=m.word(address_+0x24+0xc0*index);const int car_class=m.signed_byte(m.word(car+0xc)+0xe6);const auto table=m.allocate(0x804);TimeAroundTrackTable(m,table).Sub_00146590(car_class,routes);m.word(address_+0x6b0,table);}
void Race::SetupGhostCars(){}
}

namespace hp2::debug {
// E-0099-W62, exact original source; frame subtraction preserves SUBU bits.
int Race::GetCountdownNumberToDisplay()const{
 const auto frames=m_.word(0x327d04),go=m_.word(address_+4),start=m_.word(address_+8);
 if(go)return static_cast<std::int32_t>(frames-go)>60?0:4;
 if(!start)return 0;
 const auto seconds=static_cast<float>(static_cast<std::int32_t>(frames-start))*m_.scalar(0x324f10);
 const float interval=m_.word(0x3121ec)?1.0f:m_.scalar(0x324f0c);
 if(!std::isfinite(seconds)||!std::isfinite(interval))hp2::not_implemented("0x00147EC8","exceptional EE countdown comparison operand");
 if(seconds<interval)return 3;
 if(seconds<interval+interval)return 2;
 if(seconds<interval*3.0f)return 1;
 return 4;
}
float Race::GetCountdownProgress()const{
 const auto frames=m_.word(0x327d04),go=m_.word(address_+4),start=m_.word(address_+8);
 const float interval=m_.word(0x3121ec)?1.0f:m_.scalar(0x324f14);float value=0;
 if(go)value=interval*3.0f+static_cast<float>(static_cast<std::int32_t>(frames-go))*m_.scalar(0x324f18);
 else if(start)value=static_cast<float>(static_cast<std::int32_t>(frames-start))*m_.scalar(0x324f1c);
 if(!std::isfinite(interval)||interval==0)hp2::not_implemented("0x00147F90","exceptional EE countdown interval division");
 return value/(interval*4.0f);
}
int Race::AreAllPlayersFinishedRacing()const{for(int i=0;i<m_.signed_word(0x327780);++i)if(!m_.word(m_.word(0x327778+4*i)+0x4c))return 0;return 1;}
std::uint32_t Race::GetMostRecentlyFinishedPlayer()const{
 std::uint32_t selected=0;std::int32_t latest=0;for(int i=0;i<m_.signed_word(0x327780);++i){const auto player=m_.word(0x327778+4*i);const auto time=m_.signed_word(player+0x4c);if(time&&latest<time){selected=player;latest=time;}}return selected;
}
}

// TASK-0099 E-0099-W70. Debug source race.cpp; authority815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/port/race/race_speech.hpp"
#include "game_core/indep/inc/Player.hpp"
#include "game_core/ReplayManager.hpp"
namespace hp2::debug {
std::uint32_t RacingCar::GetLastFinishLineCrossingTime()const{return address_+0x24+4*static_cast<std::int32_t>(m_.signed_byte(m_.word(0x327258)+0x653));}
std::uint32_t RacingCar::GetLastLapTime()const{return address_+0x38+4*static_cast<std::int32_t>(m_.signed_byte(m_.word(0x327258)+0x653));}
std::uint32_t RacingCar::GetBestLapTime()const{return address_+0x60+4*static_cast<std::int32_t>(m_.signed_byte(m_.word(0x327258)+0x653));}
void RacingCar::ExtrapolateRaceTime(int laps,std::uint32_t start){
 if(!laps)hp2::not_implemented("0x001471E8","zero-lap EE extrapolation division is unresolved");
 const float fraction=m_.scalar(address_+0x14)/static_cast<float>(laps),minimum=m_.scalar(0x324efc);
 if(!std::isfinite(fraction)||!std::isfinite(minimum))hp2::not_implemented("0x001471E8","exceptional EE MAX.S extrapolation operand");
 const float progress=minimum<=fraction?fraction:minimum,scale=m_.scalar(0x324f00);
 if(progress==0||!std::isfinite(progress))hp2::not_implemented("0x001471E8","exceptional EE progress divisor");
 const float elapsed=float(static_cast<std::int32_t>(m_.word(0x327cf8)-m_.word(start)))*scale;
 const auto additional=::port::driving::cvt_w_s((elapsed/progress)*4000.f+.5f);
 Timer total{static_cast<std::int32_t>(m_.word(start)+static_cast<std::uint32_t>(additional))};total.RoundTime(m_,minimum);m_.word(address_+0xb4,static_cast<std::uint32_t>(total.PackedTime));
 if(!m_.signed_byte(address_+9)&&laps>0){const auto race=m_.word(0x327258);const auto crossing=m_.word(address_+0x24+4*static_cast<std::int32_t>(m_.signed_byte(race+0x652)));const float first=float(static_cast<std::int32_t>(m_.word(0x327cf8)-crossing))*scale;Timer best{::port::driving::cvt_w_s(((first/progress)/float(laps))*4000.f+.5f)};best.RoundTime(m_,minimum);m_.word(GetBestLapTime(),static_cast<std::uint32_t>(best.PackedTime));}
}
void Race::UpdateGhostCar(){if(m_.word(address_)==4&&m_.word(address_+0x680))hp2::not_implemented("0x00148718","actual capturing GhostCarData.AddSample");const auto time=m_.word(address_+0x10);if(m_.word(address_+0x678)&&time&&time!=0x7fffffff)hp2::not_implemented("0x00148718","actual GhostCarData.GetSample and WorldObject playback");}
void Race::MaybeSaveGhostCarData(std::uint32_t racing){if(m_.word(racing)==m_.word(address_+0x680))hp2::not_implemented("0x001482F8","actual captured GhostCarData formatting/persistence lifecycle");}
std::uint32_t Race::GetFinisher(int rank)const{for(int i=0;i<m_.signed_word(address_+0x18);++i){const auto row=m_.word(address_+0x624+4*i);if(m_.signed_byte(row+0xc)==rank)return row;}return 0;}
int Race::GetNextFinishRank()const{int rank=1;while(GetFinisher(rank))++rank;return rank;}
void Race::MonitorLeadership(){
 for(int i=0;i<m_.signed_word(0x327780);++i){const auto player=m_.word(0x327778+4*i);if(!player)continue;const auto racing=m_.word(m_.word(player+4)+0x1c);const int previous=m_.signed_byte(racing+0xb),rank=m_.signed_byte(racing+0xa);if(rank==previous)continue;
  if(previous<1||previous>m_.signed_word(address_+0x18))hp2::not_implemented("0x00148128","source previous rank does not identify a live RacingCar");
  const auto other=m_.word(address_+0x624+4*(previous-1)),car=m_.word(racing),other_car=m_.word(other);const float speed=m_.scalar(car+0x3c4),other_speed=m_.scalar(other_car+0x3c4);
  if(!(speed>40))continue;
  const float x=m_.scalar(car+0x60)-m_.scalar(other_car+0x60),y=m_.scalar(car+0x64)-m_.scalar(other_car+0x64),z=m_.scalar(car+0x68)-m_.scalar(other_car+0x68);const float square=(x*x+y*y)+z*z;
  if(!std::isfinite(square)||square<0)hp2::not_implemented("0x00148128","exceptional EE leadership distance sqrt");
  if(std::sqrt(square)<25){const auto replay=m_.word(0x3271a8);const auto name=::port::race::render_resource_text(m_,rank<previous?0x2a7488:0x2a7498);if(rank<=0||previous<=0)hp2::not_implemented("0x00148128","zero-rank EE event divisor");hp2::recovered::ReplayManager(m_,replay).RegisterInterestingEvent(rank<previous?4.f/float(rank)+1.f:2.f/float(previous),name.c_str());if(std::abs(speed-other_speed)<7){const auto close=::port::race::render_resource_text(m_,0x2a74a8);hp2::recovered::ReplayManager(m_,replay).RegisterInterestingEvent(2.5f,close.c_str());}}
 }
}
}

namespace hp2::debug {
void Race::FinishCar(int index,int rank,bool blink,int reason,const ::port::track::RouteData& routes){
 const auto row=m_.word(address_+0x624+4*index),car=m_.word(row),player=RacingCar(m_,row).GetPlayer();
 if(reason==4){Timer time{static_cast<std::int32_t>(m_.word(0x327cf8)-m_.word(address_+0xc))};time.RoundTime(m_,m_.scalar(0x324f20));m_.word(row+0xb4,static_cast<std::uint32_t>(time.PackedTime));}
 else if(reason==5){Timer time{m_.signed_word(row+0xb4)};time.RoundTime(m_,m_.scalar(0x324f24));m_.word(row+0xb4,static_cast<std::uint32_t>(time.PackedTime));m_.word(row+0xb4,m_.word(0x311f9c)+1);}
 if(rank==0){m_.byte(row+0xc,static_cast<std::uint8_t>(GetNextFinishRank()));if(m_.signed_byte(row+9)==m_.signed_word(address_+0x644)){m_.word(row+0xb4,m_.word(RacingCar(m_,row).GetLastFinishLineCrossingTime())-m_.word(address_+0xc));if(player)MaybeSaveGhostCarData(row);}else RacingCar(m_,row).ExtrapolateRaceTime(m_.signed_word(address_+0x644),address_+0xc);}
 else if(rank>=0)m_.byte(row+0xc,static_cast<std::uint8_t>(rank));
 const auto finished=m_.word(address_+0x1c);m_.half(row+0x12,static_cast<std::uint16_t>(reason));m_.word(row+0x20,m_.word(row+0x14));m_.word(address_+0x1c,finished+1);
 RaceEvent::RecordResults(m_,0x312cd0,address_,row,m_.signed_byte(row+0xc));
 if(player){Player(m_,player).SetFinishedRacing();hp2::recovered::Car(m_,car).SetControlMode(2,routes);}
 if(m_.word(car+0x20)==2){const auto controller=m_.word(car+0x28);m_.word(controller+0x210,6);m_.word(controller+0x224,m_.word(0x32826c));}
 if(blink)hp2::recovered::Car(m_,car).BlinkBlinkPoof();
}
// Debug00148D40 (NFSHP2.ELF), E-0110-F01; reverse/task0110/finish.md.
// Complete remaining source participants only after the caller ends the race.
void Race::SimulateRaceFinish(const ::port::track::RouteData& routes){
 for(int i=0;i<m_.signed_word(address_+0x18);++i){
  const auto row=m_.word(address_+0x624+4*i);
  if(!m_.signed_half(row+0x12))FinishCar(i,0,false,1,routes);
 }
 SortRacingCarsByLeadership();
}
// Debug00148DD0 (NFSHP2.ELF), E-0110-F01; reverse/task0110/finish.md.
// The source LB at00148E20 reads RacePosition+A, not FinishRank+C.
void Race::NotifyTheRaceIsOver(){
 for(int i=0;i<m_.signed_word(address_+0x18);++i){
  const auto row=m_.word(address_+0x624+4*i);
  RaceEvent::RecordResults(m_,0x312cd0,address_,row,m_.signed_byte(row+0xa));
 }
}
// Debug00148F30, E-0110-F01. The source ignores this and returns car+1C.
std::uint32_t Race::GetRacingCar(std::uint32_t car)const{return m_.word(car+0x1c);}
void Race::Update(::port::race::PhysicsConstructionServices& physics,const ::port::track::RouteData& routes){
 if(m_.word(address_)==3&&GetCountdownNumberToDisplay()==4)SetRaceStage(4,physics);
 if(m_.word(address_)==4){Timer time{static_cast<std::int32_t>(m_.word(0x327cf8)-m_.word(address_+0xc))};time.RoundTime(m_,m_.scalar(0x324f30));m_.word(address_+0x10,static_cast<std::uint32_t>(time.PackedTime));}
 UpdateGhostCar();
 for(int i=0;i<m_.signed_word(address_+0x18);++i){const auto row=m_.word(address_+0x624+4*i);if(!m_.signed_half(row+0x12)){RacingCar(m_,row).UpdateLapCounts();if(m_.word(m_.word(row)+0x18)){const auto best=RacingCar(m_,row).GetBestLapTime();if(m_.signed_word(best)<m_.signed_word(address_+0x14))m_.word(address_+0x14,m_.word(RacingCar(m_,row).GetBestLapTime()));}}}
 int forced=m_.signed_word(address_+0x648);
 if(forced){
  m_.word(address_+0x1c,0);
  for(int i=0;i<m_.signed_word(address_+0x18);++i){
   const auto row=m_.word(address_+0x624+4*i);if(m_.signed_half(row+0x12))continue;
   const auto player=RacingCar(m_,row).GetPlayer();if(!player||m_.signed_word(player+0x10)!=m_.signed_word(address_+0x64c))continue;
   int reason=1;
   if(const auto manager=m_.word(0x328c1c)){const int index=m_.signed_word(manager+0xc);const auto cop=index==-1?0:m_.word(manager+0x10+4*index);if(cop){const auto car=m_.word(cop+4);if(car&&m_.word(car+0x18)==player){reason=m_.signed_word(address_+0x18)<=forced?5:4;forced=-1;}}}
   FinishCar(i,forced,false,reason,routes);m_.word(address_+0x648,0);break;
  }
  m_.word(address_+0x648,0);m_.word(address_+0x64c,0);
 }
 SortRacingCarsByLeadership();MonitorLeadership();
 if(m_.word(0x311f98)==3){for(int i=0;i<m_.signed_word(address_+0x18);++i){const auto row=m_.word(address_+0x624+4*i);if(m_.byte(row+0xd)&&m_.signed_byte(row+9)+m_.signed_byte(row+0xa)==m_.signed_word(address_+0x18)){for(int j=i+1;j<m_.signed_word(address_+0x18);++j)if(!m_.signed_half(m_.word(address_+0x624+4*j)+0x12))FinishCar(j,j+1,true,3,routes);}}}
 if(m_.word(0x311f98)==2){for(int i=0;i<m_.signed_word(address_+0x18);++i){const auto target=m_.signed_word(0x311f9c);const auto row=m_.word(address_+0x624+4*i);if(m_.byte(row+0xd)&&!m_.signed_half(row+0x12)){const auto time=m_.signed_word(RacingCar(m_,row).GetLastLapTime());if(time&&time!=0x7fffffff&&m_.signed_word(RacingCar(m_,row).GetLastLapTime())<=target)FinishCar(i,0,false,1,routes);}}}
 for(int i=0;i<m_.signed_word(address_+0x18);++i){const auto row=m_.word(address_+0x624+4*i);if(!m_.signed_half(row+0x12)&&m_.byte(row+0xd)&&m_.signed_byte(row+0xa)==1)m_.half(row+0x10,static_cast<std::uint16_t>(m_.half(row+0x10)+1));}
 if(m_.word(address_+0x644)){for(int i=0;i<m_.signed_word(address_+0x18);++i){const auto row=m_.word(address_+0x624+4*i);if(!m_.signed_half(row+0x12)&&m_.signed_byte(row+9)==m_.signed_word(address_+0x644))FinishCar(i,0,false,1,routes);}}
 for(int i=0;i<m_.signed_word(address_+0x18);++i){const auto row=m_.word(address_+0x624+4*i);if(!m_.signed_half(row+0x12)){if(m_.byte(row+0xd))LapFinishedSpeech(m_,m_.word(row),m_.signed_byte(row+9),physics.disable_race_lap_speech);else if(m_.byte(row+0xe))SplitTimeSpeech(m_,m_.word(row));}}
}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug00146A80; E-0099-W74; replay-frame/00146a80.json/type-RacingCar.json.
void RacingCar::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginNamedSection(0x2a7348);
 s.MoveData(address_+4,2);s.MoveData(address_+6,2);for(unsigned off=8;off<=14;++off)s.MoveData(address_+off,1);
 s.MoveData(address_+0x10,2);s.MoveData(address_+0x12,2);for(unsigned off:{0x14u,0x18u,0x1cu,0x20u})s.MoveData(address_+off,4);
 if(m_.word(0x311f98)==2){const auto race=m_.word(0x327258);const auto boundary=m_.signed_byte(race+0x653);
  if(boundary<0||boundary>=5)hp2::not_implemented("0x00146A80","Time-trial finish boundary exceeds original timer arrays");
  s.MoveData(address_+0x24,4);s.MoveData(address_+0x24+4*boundary,4);s.MoveData(address_+0x38+4*boundary,4);
 }
 s.EndNamedSection();
}
// Debug001475D8; E-0099-W74; replay-frame/001475d8.json/type-Race from W56.
void Race::DoSnapshot(std::uint32_t snapshot){
 auto& m=m_;const auto a=address_;ReplaySnapshot s(m,snapshot);s.BeginSection(0x327260);s.MoveData(a,1);
 for(unsigned off:{4u,8u,12u,16u})s.MoveData(a+off,4);
 s.VerifyData(a+0x18,1,4,1);s.MoveData(a+0x1c,1,4,1);s.MoveData(a+0x20,1,4,0);
 if(m.signed_word(a+0x18)>8)hp2::not_implemented("0x001475D8","Race snapshot exceeds eight source participants");
 ::port::driving::Memory::Temporary local(m,1);
 for(int i=0;i<m.signed_word(a+0x18);++i){RacingCar(m,a+0x24+0xc0*i).DoSnapshot(snapshot);
  const auto row=a+0x624+4*i,index=((m.word(row)-a-0x24)*0xaaaaaaabu)>>6;
  m.byte(local.address(),static_cast<std::uint8_t>(index));s.MoveData(local.address(),1);const auto restored=m.signed_byte(local.address());
  if(restored<0||restored>=8)hp2::not_implemented("0x001475D8","Restored leadership pointer index outside source array");
  m.word(row,a+0x24+0xc0*restored);
 }
 s.VerifyData(a+0x644,1,4,1);s.EndSection();
}
}


namespace hp2::debug {
// Debug001485C8,E-0110-P01; original race.cpp. Signed FinishReason and
// signed completed-lap byte select original table/time branches. Arithmetic
// on packed timers wraps as the original ADDU/SUBU; no estimated host clock.
void Race::CalculateSplitTime(Timer& result,std::uint32_t first,std::uint32_t second){
 if(!first||!second)hp2::not_implemented("0x001485C8","Original split-time inputs require two actual RacingCar owners");
 auto& m=m_;const bool first_finished=m.signed_half(first+0x12)==1,second_finished=m.signed_half(second+0x12)==1;
 if(first_finished&&second_finished){result.PackedTime=static_cast<std::int32_t>(m.word(second+0xb4)-m.word(first+0xb4));return;}
 const auto table=m.word(address_+0x6b0);if(!table)hp2::not_implemented("0x001485C8","Original split-time interpolation requires its TimeAroundTrackTable owner");
 TimeAroundTrackTable times(m,table);
 const float second_time=times.GetTimeAroundTrack(second_finished?float(m.signed_byte(second+9)):m.scalar(second+0x20));
 const float first_time=times.GetTimeAroundTrack(first_finished?float(m.signed_byte(first+9)):m.scalar(first+0x20));
 const auto packed=static_cast<std::uint32_t>(::port::driving::cvt_w_s((second_time-first_time)*4000.f+.5f)); // SetTime00136D80.
 if(first_finished)result.PackedTime=static_cast<std::int32_t>(packed-(m.word(0x327cf8)-m.word(first+0xb4)));
 else if(second_finished)result.PackedTime=static_cast<std::int32_t>((packed+m.word(0x327cf8))-m.word(second+0xb4));
 else result.PackedTime=static_cast<std::int32_t>(packed);
}
}

namespace hp2::debug {
// Debug00149028,E-0112-COP03. Port tag distinguishes source Car* from numeric index.
void Race::FinishCar(::port::race::FinishCarPointer pointer,int rank,bool blink,int reason,const ::port::track::RouteData& routes){const auto racing=m_.word(pointer.value+0x1c);for(int i=0;i<m_.signed_word(address_+0x18);++i)if(racing==m_.word(address_+0x624+4*unsigned(i))){if(!rank)rank=m_.signed_byte(racing+0xa);FinishCar(i,rank,blink,reason,routes);return;}}
}

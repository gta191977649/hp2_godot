// TASK-0046 instruction-backed control cases, not PS2 numerical certification.
#include "game_core/indep/inc/RealDriver.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/port/driving/frame_lifecycle.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/horn.hpp"
#include "game_core/port/driving/session.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace port::driving;
namespace {
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void frame_motion_state(){
 Memory m;const auto car=m.allocate(0x900),state=car+0x40;
 m.vector(car+0x60,{{1,2,3,11}});m.vector(car+0x70,{{2,4,6,8}});
 m.vector(car+0xd0,{{4,6,8,10}});m.scalar(state+0x368,10);m.scalar(state+0x36c,20);
 begin_movement_state(m,car,.5f);
 check(m.vector(car+0x470)==std::array<float,4>{{1,2,3,11}},"Begin frame captures full position including W");
 m.vector(car+0x60,{{4,6,3,99}});m.vector(car+0x70,{{3,6,9,12}});
 m.vector(car+0xd0,{{6,9,12,15}});m.scalar(state+0x384,12);
 finish_movement_state(m,car,.5f);
 check(m.vector(car+0x80)==std::array<float,4>{{2,4,6,8}}&&m.vector(car+0xe0)==std::array<float,4>{{4,6,8,10}},"Finish frame uses whole-frame linear/angular differences including W");
 check(m.scalar(state+0x368)==16&&m.scalar(state+0x36c)==25,"Odometer uses speed*time; traveled distance uses XYZ displacement only");
 check(m.vector(car+0x470)==m.vector(car+0x60),"Finish frame republishes full old position");
 const auto acceleration=m.vector(car+0x80),old_velocity=m.vector(car+0x480);
 m.vector(car+0x70,{{100,200,300,400}});begin_movement_state(m,car,0);
 check(m.vector(car+0x480)==old_velocity,"Zero timestep leaves old velocity unchanged");
 finish_movement_state(m,car,0);check(m.vector(car+0x80)==acceleration,"Zero timestep leaves acceleration unchanged");
 for(float speed:{-1.f,0.f,1000.f,1001.f}){
  m.scalar(state+0x368,7);m.scalar(state+0x384,speed);finish_movement_state(m,car,.5f);
  check(m.scalar(state+0x368)==(speed==1000.f?507.f:7.f),"Original odometer includes0/1000 endpoints and rejects negative/above1000");
 }
 m.scalar(state+0x384,std::numeric_limits<float>::quiet_NaN());finish_movement_state(m,car,.5f);
 check(std::isnan(m.scalar(state+0x368)),"Original two-rejection comparison does not hide unordered odometer input");
 check(!hp2::guard([&]{hp2::recovered::Car(m,car).BeginMovement(.5f);}).empty(),"Whole BeginMovement still reports untranslated topology services");
 check(!hp2::guard([&]{hp2::recovered::Car(m,car).FinishMovement(.5f);}).empty(),"Whole FinishMovement still reports untranslated path consumers");
}
void terrain_frame_lifecycle(){
 Memory m;const auto state=m.allocate(0x400);hp2::recovered::CarState car(m,state);
 for(unsigned i=0;i<0x400;++i)m.byte(state+i,0xa5);
 car.BeginTimestep();
 for(unsigned i=0;i<0x400;++i)if(i<0x324||i>=0x334)
  check(m.byte(state+i)==0xa5,"Frame reset must not alter current terrain, contacts or other state");
 for(int i=0;i<4;++i){
  check(m.word(state+0x324+4*i)==1,"Frame starts at original ROAD enum1");
  car.SetWheelTerrain(i,8+i);car.SetWheelTerrain(i,2);
  check(m.word(state+0x324+4*i)==static_cast<unsigned>(8+i),"Repeated contacts retain maximum terrain within a frame");
 }
 car.BeginTimestep();
 for(int i=0;i<4;++i){car.SetWheelTerrain(i,1);
  check(m.word(state+0x314+4*i)==1&&m.word(state+0x324+4*i)==1,"Next road frame must not retain previous off-road terrain");}
}
struct Fixture {
 Memory m;std::uint32_t car,driver,config;PlayerServices services;
 Fixture(){
  m.map(0x324000,0x5000);car=m.allocate(0x900);driver=m.allocate(0x280);config=m.allocate(0x560);
  m.word(car+0xc,config);m.word(config+0x288,6);m.word(car+0x3c8,2);
  // E-0046-C01 exact Ghidra little-endian constant words.
  m.word(0x324eac,0x410f0c79);m.word(0x324eb0,0x3f19999a);m.word(0x324eb4,0x3e4ccccd);
  m.word(0x327180,0x3f000000);m.word(0x327184,0xc1f00000);m.word(0x327188,0xc0000000);
  services.RegisterInterestingEvent=[](Memory&,float,const char*){};
  hp2::recovered::RealDriver(m,driver).Sub_0013AB98(car);
 }
 hp2::recovered::RealDriver view(){return {m,driver};}
 void pedals(float throttle,float brake,float speed){m.scalar(driver+0x25c,throttle);m.scalar(driver+0x264,brake);m.scalar(car+0x3c0,speed);view().DoThrottleBrakeReverse(1.f/60,services);}
};
void control_cases(){
 Fixture f;auto driver=f.view();check(f.m.word(f.driver+0x23c)==1&&f.m.signed_word(f.driver+0x220)==-1,"Original constructor starts automatic at start line");
 driver.DisableStartLineState();check(f.m.word(f.driver+0x23c)==0,"Disable start line calls low gear");
 driver.SetManualTransmission(1);check(f.m.word(f.driver+0x220)==2,"Manual takes actual current gear");
 driver.JoyHandler_ShiftUp(0,0,0);check(f.m.word(f.driver+0x220)==2,"Release event cannot shift");
 driver.JoyHandler_ShiftUp(0,0,1);check(f.m.word(f.driver+0x220)==3,"Manual up one raw gear");
 driver.JoyHandler_ShiftDown(0,0,1);driver.JoyHandler_ShiftDown(0,0,1);driver.JoyHandler_ShiftDown(0,0,1);
 check(f.m.word(f.driver+0x220)==1,"Downshift stops at neutral, not reverse");
 driver.SetDesiredGear(7);driver.JoyHandler_ShiftUp(0,0,1);check(f.m.word(f.driver+0x220)==7,"Manual top gear bound uses config count plus one");
 driver.SetDesiredGear(0);driver.JoyHandler_ShiftUp(0,0,1);check(f.m.word(f.driver+0x220)==0,"Shift-up handler does not exit reverse");
 driver.SetManualTransmission(0);check(f.m.signed_word(f.driver+0x220)==-1,"Automatic mode restores request sentinel");
 f.pedals(0,1,10);check(f.m.scalar(f.driver+0x218)==1&&f.m.scalar(f.driver+0x214)==0,"Forward S brakes above original threshold");
 f.pedals(0,1,0);check(f.m.word(f.driver+0x220)==0&&f.m.scalar(f.driver+0x214)==1&&f.m.scalar(f.driver+0x218)==0,"S at rest selects reverse and saturates doubled pedal");
 f.pedals(1,0,-3);check(f.m.scalar(f.driver+0x214)==0&&f.m.scalar(f.driver+0x218)==1,"W while reversing routes to brake before original transition");
 f.pedals(1,0,0);check(f.m.signed_word(f.driver+0x220)==-1&&f.m.scalar(f.driver+0x214)==1,"W restores automatic low gear after reverse");
 f.m.scalar(f.driver+0x274,1);f.pedals(.7f,0,0);check(f.m.scalar(f.driver+0x21c)==1,"Handbrake reaches original output");
 f.m.scalar(f.driver+0x244,1);f.pedals(1,0,10);check(f.m.scalar(f.driver+0x214)==0&&f.m.scalar(f.driver+0x21c)==1,"Forced stop timer overrides throttle");
 driver.ResetJoyValues();check(f.m.scalar(f.driver+0x274)==0&&f.m.scalar(f.driver+0x254)==0&&f.m.byte(f.driver+0x250)==128,"Reset restores centered steering and zero pedals");
}
void replay_event(){
 Memory m;const auto replay=m.allocate(0x50b4),node=m.allocate(0x200);const auto head=replay+0x6c;
 m.word(head,node);m.word(node,head);const char name[]="INSTANT_REPLAY";for(unsigned i=0;i<sizeof(name);++i)m.byte(node+8+i,name[i]);
 m.scalar(node+0x14c,2);hp2::recovered::ReplayManager r(m,replay);r.RegisterInterestingEvent(1,"EBrake");check(m.scalar(node+0x14c)==2,"Capture-disabled original branch does not mutate snapshots");
 m.word(replay+4,1);r.RegisterInterestingEvent(1,"EBrake");check(m.scalar(node+0x14c)==3,"Capture adds interesting value to instant replay snapshot");
}
void horn_controls(){
 Fixture f;f.m.map(0x312000,0x1000);const auto sound=f.m.allocate(0x50),horn=f.m.allocate(4);
 f.m.word(f.car+0x850,sound);f.m.word(sound+0x34,horn);f.m.word(0x326f40,1);f.m.word(0x327980,1);
 auto driver=f.view();driver.JoyHandler_HonkHorn(0,0,1);
 check(horn_activation(f.m,sound)&&f.m.word(f.car+0x830)==1,"Ordinary horn press activates sound and flashes headlights");
 driver.JoyHandler_HonkHorn(0,0,0);
 check(!horn_activation(f.m,sound)&&f.m.word(f.car+0x830)==0,"Ordinary horn release stops sound and flash");
 f.m.word(0x327980,0);driver.JoyHandler_HonkHorn(0,0,1);
 check(!horn_activation(f.m,sound)&&f.m.word(horn)==0,"Original CarSoundEnabled gate suppresses horn activation");
 driver.JoyHandler_HonkHorn(0,0,0);f.m.word(0x327980,1);f.m.word(f.config+0x538,1);
 driver.JoyHandler_HonkHorn(0,0,1);
 check(!horn_activation(f.m,sound)&&f.m.word(f.car+0x808)==2&&f.m.word(f.car+0x834)==1,"Police-car honk toggles original siren/light state without civilian horn");
 driver.JoyHandler_HonkHorn(0,0,0);driver.JoyHandler_HonkHorn(0,0,1);
 check(f.m.word(f.car+0x808)==0&&f.m.word(f.car+0x834)==0,"Second police press clears toggle state");
 f.m.word(0x3121ec,1);
 check(!hp2::guard([&]{driver.JoyHandler_HonkHorn(0,0,1);}).empty(),"Unbound BeACop manager cannot silently substitute civilian horn");
}
void session_edges(){
 std::vector<Input> calls;Session session([&](const Config&,const Input& input,const Snapshot&){calls.push_back(input);Snapshot out;out.dt=1.f/60;return out;});
 session.configure({});session.run();Input request;request.shift=1;request.transmission=1;request.neutral=true;session.input(request);session.tick();session.tick();
 check(calls.size()==2&&calls[0].shift==1&&calls[0].neutral&&calls[0].transmission==1,"Semantic request reaches next completed step");
 check(calls[1].shift==0&&!calls[1].neutral&&calls[1].transmission==-1,"Successful step consumes command exactly once");
 session.input(request);session.pause();check(session.controls().shift==0&&!session.controls().neutral,"Pause clears pending edges");
 session.input(request);session.input({});check(session.controls().shift==0&&!session.controls().neutral,"Zero semantic frame cancels pending edges on capture");
}
}
int main(){try{horn_controls();frame_motion_state();terrain_frame_lifecycle();control_cases();replay_event();session_edges();std::cout<<"Player and driving frame-state instruction cases passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

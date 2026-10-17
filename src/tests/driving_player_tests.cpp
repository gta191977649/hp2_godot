// TASK-0046 instruction-backed control cases, not PS2 numerical certification.
#include "game_core/indep/inc/RealDriver.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/port/driving/session.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace port::driving;
namespace {
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
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
void session_edges(){
 std::vector<Input> calls;Session session([&](const Config&,const Input& input,const Snapshot&){calls.push_back(input);Snapshot out;out.dt=1.f/60;return out;});
 session.configure({});session.run();Input request;request.shift=1;request.transmission=1;request.neutral=true;session.input(request);session.tick();session.tick();
 check(calls.size()==2&&calls[0].shift==1&&calls[0].neutral&&calls[0].transmission==1,"Semantic request reaches next completed step");
 check(calls[1].shift==0&&!calls[1].neutral&&calls[1].transmission==-1,"Successful step consumes command exactly once");
 session.input(request);session.pause();check(session.controls().shift==0&&!session.controls().neutral,"Pause clears pending edges");
 session.input(request);session.input({});check(session.controls().shift==0&&!session.controls().neutral,"Zero semantic frame cancels pending edges on capture");
}
}
int main(){try{control_cases();replay_event();session_edges();std::cout<<"TASK-0046 player instruction cases passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

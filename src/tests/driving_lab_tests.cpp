#include "game_core/port/driving/session.hpp"
#include "game_core/Sub_00137C88.hpp"
#include "game_core/Sub_001A4E88.hpp"
#include "game_core/port/not_implemented.hpp"
#include "platform/ps2_asset_root.hpp"
#include "port/assets/catalog.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace port::driving;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F body){bool rejected=false;try{body();}catch(const std::exception&){rejected=true;}check(rejected,"Expected rejection");}
// Deliberately synthetic backend for session contracts, not vehicle behavior.
Snapshot tool_backend(const Config&,const Input& input,const Snapshot& old){
    Snapshot next=old;next.dt=0.25f;
    next.velocity=std::array<float,3>{{input.throttle,input.brake,input.steering}};return next;
}
}
int main(){
    Session unavailable;unavailable.configure({"synthetic contract config"});
    check(!unavailable.snapshot().dt&&!unavailable.snapshot().body,"Unknown state must remain unknown");
    unavailable.input({1,0,-1});unavailable.recording(true);unavailable.run();
    check(!unavailable.tick(),"Untranslated constructor must block");
    check(unavailable.paused()&&unavailable.status()=="Blocked","Gap must pause");
    check(unavailable.diagnostic().find("0x00137100")!=std::string::npos,"Gap address is required");
    check(unavailable.snapshot().step==0&&unavailable.tape().empty(),"Failed step must not publish or record");
    check(unavailable.controls().throttle==0,"Gap must release controls");
    Session session(tool_backend);session.configure({"synthetic contract config"});
    check(!session.tick(),"Paused tick must not run");
    rejects([&]{session.input({2,0,0});});rejects([&]{session.input({0,0,std::numeric_limits<float>::quiet_NaN()});});
    session.recording(true);
    session.input({1,0,-1});check(session.single_step(),"Single tool step");
    check(session.paused()&&session.snapshot().step==1,"Single step exactly once");
    session.input({0,1,1});check(session.single_step(),"Second tool step");
    const auto expected=session.snapshot().velocity;
    check(session.tape().size()==2,"Record successful inputs");
    session.recording(false);session.replay();
    check(session.snapshot().step==0&&session.is_replaying(),"Replay resets to initial state");
    session.input({0.5f,0,0});check(session.controls().throttle==0,"Live input cannot replace replay");
    session.run();check(session.tick()&&session.tick(),"Replay both steps");
    check(session.snapshot().velocity==expected,"Replay uses the same inputs");
    check(!session.tick()&&session.paused()&&!session.is_replaying(),"Replay end pauses without extra step");
    session.reset();check(session.snapshot().step==0&&!session.snapshot().dt,"Reset clears state, not fabricated rest");
    Session broken([](const Config&,const Input&,const Snapshot&){Snapshot s;return s;});
    broken.configure({"synthetic"});check(!broken.single_step(),"Unknown timestep cannot commit");
    check(broken.snapshot().step==0,"Bad publication is transactional");
    Session invalid_pose([](const Config&,const Input&,const Snapshot&){Snapshot s;s.dt=0.1f;s.body=Matrix{};return s;});
    invalid_pose.configure({"synthetic"});check(!invalid_pose.single_step(),"Invalid body cannot commit");
    check(!invalid_pose.snapshot().body&&invalid_pose.snapshot().step==0,"Invalid pose retains the last completed state");
    // Independent instruction-derived finite fixtures: E-0038-003.
    // 00137C90..9C: abs(*a1)+1; C8..EC strict comparisons and early return.
    float a1=-2;check(Sub_00137C88(0,1,15,4,2,0,0,a1)==3&&a1==2,"Absolute writeback / early return");
    a1=0;check(Sub_00137C88(1,0,15,0.25f,0.25f,2,0,a1)==1.25f,"Unscaled coefficient path");
    a1=0;check(Sub_00137C88(1,0,15,0.25f,0.25f,2,1,a1)==1.375f,"Branch-likely scaled coefficient path");
    a1=0;check(std::abs(Sub_00137C88(2,0,30,1,0,0,0,a1)-1.7f)<1e-7f,"Upper cap 0x3f333333");
    a1=0;check(Sub_00137C88(1,0,-30,1,0,0,0,a1)==0,"No invented lower clamp");
    // E-0038-005 instruction fixtures: f1=2*4=8; f0=(20-8)/2=6;
    // f2=3*1+5*2=13; f0=((6*4-13)*2)/4=5.5; result=7.5.
    WheelIterationProjection iteration{4,2,3,5,20,2};
    ModifyGroundForce(iteration,{{1,2}});check(iteration.Field_0x114==7.5f,"Wheel iteration instruction fixture");
    iteration={2,4,1,-1,8,0};ModifyGroundForce(iteration,{{2,2}});
    check(iteration.Field_0x114==8,"Wheel iteration opposing projection fixture");
    // Synthetic container validates raw-row preservation and bounds.
    std::vector<std::uint8_t> bundle(16+64*0x560,0);
    auto put=[&](int offset,std::uint32_t v){std::memcpy(bundle.data()+offset,&v,4);};
    put(0,0x34600);put(4,std::uint32_t(bundle.size()-8));
    std::memcpy(bundle.data()+16,"FIXTURE",7);bundle[16+0x310]=0xab;
    auto config=read_config(bundle,"FIXTURE");check(config.index==0&&config.raw[0x310]==0xab,"Raw parameter row preserved");
    rejects([&]{read_config(bundle,"missing");});bundle.pop_back();rejects([&]{read_config(bundle,"FIXTURE");});
    if(const auto root=hp2::platform::resolve_zzdata_root(".")){
        port::assets::DirectorySource original(*root);
        auto actual=read_config(original.read("GLOBAL/GLOBALB.BUN"),"911TURBO");
        check(actual.index==39,"E-0033-B04 original 911TURBO row index");
        std::cout<<"DRIVING_ORIGINAL_CONFIG_OK: row39, original bytes read at runtime\n";
    }else std::cout<<"Original config check unavailable: ZZDATA not configured\n";
    std::cout<<"DRIVING_LAB_OK: tooling and bounded arithmetic; no full driving equivalence\n";
}

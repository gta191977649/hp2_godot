#include "game_core/port/driving/native_backend.hpp"
#include "platform/ps2_asset_root.hpp"
#include "port/assets/catalog.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace port::driving;
namespace {
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void reset_state_leaf(const std::filesystem::path& elf){
    auto m=read_debug_image(elf);const auto state=m.allocate(0x420),config=m.allocate(0x560);
    for(unsigned i=0;i<0x420;++i)m.byte(state+i,0xa5);
    m.word(state,config);
    for(unsigned i=0;i<4;++i){m.vector(config+0x120+0x20*i,{{float(i+1),float(i+2),float(i+3),1}});m.scalar(config+0x138+0x20*i,.25f);}
    for(auto off:{0x3cu,0x4cu,0x9cu,0xacu})m.word(state+off,0x7fc12345);
    std::array<std::uint8_t,0x420> before{};for(unsigned i=0;i<before.size();++i)before[i]=m.byte(state+i);
    hp2::recovered::CarState(m,state).ResetCar();
    const auto changed=[](unsigned i){
        for(auto off:{0x30u,0x40u,0x90u,0xa0u})if(i>=off&&i<off+12)return true;
        return (i>=0x50&&i<0x90)||(i>=0xb0&&i<0x368)||(i>=0x370&&i<0x39c)
            ||(i>=0x3a0&&i<0x3b0)||(i>=0x3c0&&i<0x404);
    };
    for(unsigned i=0;i<before.size();++i)if(!changed(i))check(m.byte(state+i)==before[i],"ResetCar preserves bytes outside original write extent");
    for(auto off:{0x30u,0x40u,0x90u,0xa0u})for(unsigned i=0;i<3;++i)check(m.word(state+off+4*i)==0,"ResetCar zeros motion XYZ");
    check(m.matrix(state+0x50)==hp2::recovered::bIdentity(),"ResetCar restores body identity");
    check(m.word(state+0x37c)==4&&m.word(state+0x388)==1&&m.word(state+0x398)==1,"ResetCar restores ground count, neutral and clutch");
    check(m.scalar(state+0x390)==1000&&m.scalar(state+0x394)==-1&&m.scalar(state+0x3a0)==1&&m.scalar(state+0x3ac)==1,"ResetCar restores original engine/boost/traction scalars");
    for(unsigned i=0;i<4;++i){
        check(m.byte(state+0x310+i)==1&&m.word(state+0x314+4*i)==1&&m.word(state+0x324+4*i)==1,"ResetCar restores four ROAD contacts");
        const auto tire=m.matrix(state+0xb0+0x40*i),brake=m.matrix(state+0x1b0+0x40*i);
        check(tire[12]==float(i+1)&&tire[13]==float(i+2)+.25f&&tire[14]==float(i+3)&&brake[13]==float(i+2),"ResetCar recomputes original tire/brake translations");
    }
    for(unsigned i=0x3c0;i<0x404;++i)check(m.byte(state+i)==0,"ResetCar clears relative matrix and complete validity word");
}
float horizontal_speed(const Snapshot& s){const auto& v=*s.velocity;return std::sqrt(v[0]*v[0]+v[1]*v[1]);}
float horizontal_distance(const Snapshot& a,const Snapshot& b){const float x=(*a.body)[12]-(*b.body)[12],y=(*a.body)[13]-(*b.body)[13];return std::sqrt(x*x+y*y);}
void complete(const Snapshot& s){
    check(s.body&&s.wheels&&s.velocity&&s.angular_velocity&&s.dt,"Production snapshot must publish body, wheels, motion and timestep");
    check(std::isfinite(*s.dt)&&*s.dt>0,"Recovered scheduling must publish a finite positive timestep");
    validate_matrix(*s.body);validate_wheels(*s.wheels);
    for(const auto& vector:{*s.velocity,*s.angular_velocity})for(float value:vector)check(std::isfinite(value),"Production motion must remain finite");
    for(const auto& wheel:*s.wheels)check(wheel.contact.has_value(),"Wheel contact state must be known");
}
bool same_snapshot(const Snapshot& a,const Snapshot& b){
    if(a.step!=b.step||a.dt!=b.dt||a.body!=b.body||a.velocity!=b.velocity||a.angular_velocity!=b.angular_velocity||a.wheels.has_value()!=b.wheels.has_value())return false;
    if(a.wheels)for(unsigned i=0;i<4;++i){const auto& x=(*a.wheels)[i];const auto& y=(*b.wheels)[i];
        if(x.tire!=y.tire||x.brake!=y.brake||x.blur!=y.blur||x.phase!=y.phase||x.angular_velocity!=y.angular_velocity||x.blur_alpha!=y.blur_alpha||x.contact!=y.contact)return false;
    }
    return true;
}
Snapshot frames(NativeBackend& backend,const Config& config,Input input,unsigned count){
    Snapshot result;for(unsigned i=0;i<count;++i){result=backend.advance(config,input,1.0f/60.0f);complete(result);}return result;
}
// These are qualitative acceptance checks for the explicit laboratory scenario.
// Their broad thresholds are port test tolerances, not recovered PS2 constants,
// golden original trajectories or original-versus-native equivalence claims.
void driving_sequence(NativeBackend& backend,const Config& config){
    const auto resting=frames(backend,config,{},120);
    check(horizontal_speed(resting)<0.1f,"Uncontrolled settled car should remain near horizontal rest");
    unsigned contacts=0;for(const auto& wheel:*resting.wheels)if(*wheel.contact)++contacts;
    check(contacts>=2,"Settled laboratory car should retain supporting wheel contacts");
    const auto early=frames(backend,config,{1,0,0},60);
    const auto accelerated=frames(backend,config,{1,0,0},180);
    std::cerr<<"Debug acceptance rest/early/late "<<horizontal_speed(resting)<<' '<<horizontal_speed(early)<<' '<<horizontal_speed(accelerated)<<" travel "<<horizontal_distance(accelerated,resting)<<'\n';
    const auto& mem=backend.memory();const auto mover=backend.physics_car_address(),eng=mem.word(mover+0x40),drive=mem.word(mover+0x44),body=mem.word(mover+0x18),wheel=mem.word(mover+0x30);
    std::cerr<<"Debug engine W/throttle/torque "<<mem.scalar(eng+0xc)<<' '<<mem.scalar(eng+0x44)<<' '<<mem.scalar(eng+0x48)<<" gear/clutch/diff "<<mem.word(drive+0x30)<<' '<<mem.scalar(drive+0xc)<<' '<<mem.scalar(drive+0x40)<<" wheelW/moment/zforce "<<mem.scalar(wheel+0x20)<<' '<<mem.scalar(wheel+0x108)<<' '<<mem.scalar(wheel+0x158)<<" speedlimit "<<mem.scalar(body+0x174)<<'\n';
    const auto table=eng+0x18;std::cerr<<"Debug torqueTable "<<std::hex<<mem.word(table)<<std::dec<<" count/range/mult "<<mem.word(table+4)<<' '<<mem.scalar(table+8)<<' '<<mem.scalar(table+0xc)<<' '<<mem.scalar(table+0x10)<<" points ";for(unsigned i=0;i<9;++i)std::cerr<<mem.scalar(mem.word(table)+4*i)<<' ';std::cerr<<" throttleOverride "<<mem.scalar(eng+0x4c)<<" boost "<<mem.scalar(eng+0x58)<<'\n';
    check(horizontal_speed(accelerated)>horizontal_speed(early)+1,"Sustained throttle must accelerate beyond the first second");
    check(horizontal_distance(accelerated,resting)>1,"Sustained throttle must produce horizontal travel");
    const auto braking=frames(backend,config,{0,1,0},30);
    check(horizontal_speed(braking)<horizontal_speed(accelerated),
          "Original S routing must initially brake a forward-moving car");
    const auto held=frames(backend,config,{0,1,0},270);
    check(held.powertrain&&held.powertrain->gear==0&&held.powertrain->forward_speed<0,
          "Held S must eventually select original reverse and travel backward");
    std::cout<<"native acceptance speeds rest/early/late/braked "<<horizontal_speed(resting)<<' '<<horizontal_speed(early)<<' '<<horizontal_speed(accelerated)<<' '<<horizontal_speed(braking)<<'\n';
}
void steering_and_reset(NativeBackend& backend,const Config& config){
    const auto branch=[&](float steering){backend.reset();frames(backend,config,{},120);const auto prefix=frames(backend,config,{1,0,0},120);const auto end=frames(backend,config,{0.5f,0,steering},60);return std::array<Snapshot,2>{{prefix,end}};};
    // Source001A5D60 stores forward(cos,+sin,0): positive turns left
    // around source+Z. The tool input adapter must preserve this native sign.
    const auto straight=branch(0),left=branch(0.3f),right=branch(-0.3f);
    check(same_snapshot(straight[0],left[0])&&same_snapshot(straight[0],right[0]),"Reset must reproduce the complete same driving prefix");
    const auto& start=*straight[0].body;float fx=start[0],fy=start[1];const float norm=std::sqrt(fx*fx+fy*fy);check(norm>0,"Prefix must have a horizontal forward basis");fx=fx/norm;fy=fy/norm;
    const auto side=[&](const Snapshot& s){const float x=(*s.body)[12]-(*straight[1].body)[12],y=(*s.body)[13]-(*straight[1].body)[13];return fx*y-fy*x;};
    const float l=side(left[1]),r=side(right[1]);
    check(l>0.01f && r< -0.01f,"Positive native steering must travel left and negative steering right relative to straight driving");
    check(std::abs((*left[1].angular_velocity)[2]-(*straight[1].angular_velocity)[2])>0.001f
       && std::abs((*right[1].angular_velocity)[2]-(*straight[1].angular_velocity)[2])>0.001f,
          "Both steering directions must change body turning motion");
    // Repeating a mixed sequence compares every published field, including all
    // four tire/brake/blur matrices, contact bytes, phases and angular velocities.
    std::vector<Snapshot> expected;backend.reset();
    for(unsigned i=0;i<180;++i){const Input input{i<100?0.8f:0.0f,i>=140?0.7f:0.0f,i>=60&&i<140?0.15f:0.0f};auto s=backend.advance(config,input,1.0f/60.0f);complete(s);expected.push_back(s);}
    backend.reset();for(unsigned i=0;i<expected.size();++i){const Input input{i<100?0.8f:0.0f,i>=140?0.7f:0.0f,i>=60&&i<140?0.15f:0.0f};check(same_snapshot(expected[i],backend.advance(config,input,1.0f/60.0f)),"Reset must deterministically reproduce complete mixed-input snapshots");}
    std::cout<<"native opposite-steering lateral offsets "<<l<<' '<<r<<'\n';
}
// A synthetic temporary file exercises the evidence binding without copying,
// mutating or exporting any original ELF bytes. Its path is never reused.
void wrong_image_sha(){
    struct TemporaryFile {
        std::filesystem::path path;
        ~TemporaryFile(){std::error_code error;std::filesystem::remove(path,error);}
    } temporary;
    const auto directory=std::filesystem::temp_directory_path();
    const auto nonce=std::chrono::steady_clock::now().time_since_epoch().count();
    for(unsigned attempt=0;attempt<100;++attempt){
        const auto candidate=directory/("hp2-driving-wrong-sha-"+std::to_string(nonce)+"-"+std::to_string(attempt)+".tmp");
        if(!std::filesystem::exists(candidate)){temporary.path=candidate;break;}
    }
    check(!temporary.path.empty(),"Synthetic SHA fixture must have a unique temporary path");
    {std::ofstream output(temporary.path,std::ios::binary);output<<"Synthetic TASK-0038 wrong-SHA fixture; no original bytes.";check(bool(output),"Synthetic SHA fixture write must succeed");}
    bool rejected=false;
    try{(void)read_debug_image(temporary.path);}
    catch(const std::exception& error){rejected=std::string(error.what()).find("SHA-256")!=std::string::npos;}
    check(rejected,"A synthetic file must fail the original ELF SHA binding before mapping");
    std::error_code error;check(std::filesystem::remove(temporary.path,error)&&!error,"Synthetic SHA fixture must be removed");
    check(!std::filesystem::exists(temporary.path),"Synthetic SHA fixture cleanup must be observable");
}
void rejected_step_preserves_state(const std::filesystem::path& elf,const Config& config){
    NativeBackend tested(elf),control(elf);
    Snapshot prefix;
    for(unsigned i=0;i<90;++i){const Input input{i>=30?0.7f:0.0f,0,0};prefix=tested.advance(config,input,1.0f/60.0f);check(same_snapshot(prefix,control.advance(config,input,1.0f/60.0f)),"Transactional test controls require an identical valid prefix");}
    const auto rejected_then_compare=[&](Input input,float elapsed){
        bool rejected=false;std::string diagnostic;
        // These published scheduling fields are already mapped by a successful
        // native step; a failed candidate must not commit their partial updates.
        const auto before_dt=tested.memory().word(0x00327ce4);
        const auto before_math=tested.memory().vector(0x00317b20);
        try{(void)tested.advance(config,input,elapsed);}
        catch(const std::exception& error){rejected=true;diagnostic=error.what();}
        check(rejected,"Pathological or invalid backend arguments must reject the step");
        check(tested.memory().word(0x00327ce4)==before_dt && tested.memory().vector(0x00317b20)==before_math,
              "Rejected candidates must preserve committed scheduling/math state");
        const Input valid{0.6f,0,0.1f};
        const auto actual=tested.advance(config,valid,1.0f/60.0f);
        const auto expected=control.advance(config,valid,1.0f/60.0f);
        check(same_snapshot(actual,expected),"A valid step after failure must match the untouched control backend in every publication field");
        // Extend beyond one recovery step so delayed random/clock/state mutation
        // cannot hide behind an immediately unchanged body transform.
        for(unsigned i=0;i<30;++i)check(same_snapshot(tested.advance(config,valid,1.0f/60.0f),control.advance(config,valid,1.0f/60.0f)),
                                     "Rejected candidates must not perturb subsequent deterministic motion");
        return diagnostic;
    };
    const auto conversion=rejected_then_compare({0.5f,0,1.0e30f},1.0f/60.0f);
    check(!conversion.empty(),"Finite pathological steering rejection must explain its failure");
    rejected_then_compare({0.5f,0,0},std::numeric_limits<float>::quiet_NaN());
    rejected_then_compare({0.5f,0,0},-0.1f);
    std::cout<<"native transactional failure diagnostic "<<conversion<<'\n';
}
void session_replay(const std::filesystem::path& elf,const Config& config){
    Session session(std::make_shared<NativeBackend>(elf));session.configure(config);session.recording(true);session.run();
    std::vector<Snapshot> expected;unsigned paced_waits=0;
    const float elapsed[]={1.0f/240.0f,1.0f/180.0f,1.0f/90.0f,1.0f/55.0f,1.0f/30.0f};
    for(unsigned i=0;i<220;++i){
        session.input({i<140?0.8f:0.0f,i>=170?0.8f:0.0f,i>=80&&i<160?0.2f:0.0f});
        if(session.tick(elapsed[i%5])){complete(session.snapshot());expected.push_back(session.snapshot());}
        else{check(session.diagnostic().empty(),"Pacing may defer a frame but must not block production physics");++paced_waits;}
        if(i==70){
            session.pause();const auto before=session.snapshot();const auto recorded=session.tape().size();
            check(!session.tick(2)&&same_snapshot(before,session.snapshot())&&session.tape().size()==recorded,"Paused elapsed time must not advance or record physics");
            check(session.controls().throttle==0&&session.controls().brake==0&&session.controls().steering==0,"Pause must clear live controls");
            session.input({0.4f,0,-0.1f});check(session.single_step()&&session.paused(),"Paused production single-step must advance exactly once");
            complete(session.snapshot());check(session.snapshot().step==before.step+1,"Single-step increments the published counter once");expected.push_back(session.snapshot());
            check(!session.tick(1),"Single-step must leave the real backend paused");session.run();
        }
    }
    check(paced_waits>0 && expected.size()>100,"Varied small elapsed times must exercise accumulated pacing and sustained recording");
    session.recording(false);check(session.tape().size()==expected.size(),"Tape must contain exactly successful production frames including single-step");
    session.replay();session.input({1,1,-1});check(session.controls().throttle==0,"Replay ignores new live input");session.run();
    unsigned replayed=0;
    for(unsigned attempt=0;attempt<expected.size()*10 && session.is_replaying();++attempt){
        if(session.tick(elapsed[(attempt+3)%5])){
            check(replayed<expected.size()&&same_snapshot(session.snapshot(),expected[replayed]),"Replay must reproduce every complete snapshot under different render pacing");++replayed;
        }else check(session.diagnostic().empty(),"Replay pacing must not conceal a backend error");
    }
    check(replayed==expected.size()&&session.paused()&&!session.is_replaying(),"Replay must stop at tape end without a new physics frame");
    const auto final=session.snapshot();check(!session.tick(1)&&same_snapshot(final,session.snapshot()),"Completed replay must retain the final publication while paused");
    std::cout<<"native Session recorded/replayed "<<replayed<<" complete snapshots; paced deferrals "<<paced_waits<<'\n';
}
void player_transmission(NativeBackend& backend,const Config& config){
    backend.reset();frames(backend,config,{},120);
    Input manual;manual.transmission=1;
    auto s=backend.advance(config,manual,1.f/60);
    check(s.powertrain&&s.powertrain->manual_transmission==1&&s.powertrain->gear==2,"Manual mode begins in actual first gear");
    Input neutral;neutral.neutral=true;backend.advance(config,neutral,1.f/60);
    s=frames(backend,config,{.7f,0,0},90);
    check(s.powertrain&&s.powertrain->gear==1,"Neutral request survives throttle without invented automatic engagement");
    const auto& mem=backend.memory();const auto mover=backend.physics_car_address(),engine=mem.word(mover+0x40),st=mem.word(mover+0x50);
    check(s.powertrain->rpm==mem.scalar(st+0x390)&&s.powertrain->rpm>0&&mem.scalar(engine+0x44)>0,"HUD RPM is completed original state, engine receives neutral throttle");
    Input up;up.shift=1;backend.advance(config,up,1.f/60);s=frames(backend,config,{.7f,0,0},90);
    check(s.powertrain&&s.powertrain->gear==2&&s.powertrain->forward_speed>0,"One upshift from neutral engages first and travels forward");
    Input handbrake;handbrake.handbrake=1;s=frames(backend,config,handbrake,15);
    check(s.powertrain&&s.powertrain->handbrake,"Handbrake publication follows original driver output");
    s=frames(backend,config,{},1);check(!s.powertrain->handbrake,"Releasing handbrake clears original driver input");
    Input automatic;automatic.transmission=0;backend.advance(config,automatic,1.f/60);s=frames(backend,config,{1,0,0},360);
    check(s.powertrain&&s.powertrain->manual_transmission==0&&s.powertrain->gear>2,"Original automatic chain shifts beyond first under sustained throttle");
}
void terrain_transition(NativeBackend& backend,const Config& config){
    backend.reset();
    for(unsigned i=0;i<120;++i)backend.advance(config,{},1.f/60,{SurfaceMode::Uniform,8});
    const auto state=backend.memory().word(backend.physics_car_address()+0x50);
    for(unsigned i=0;i<4;++i)check(backend.memory().word(state+0x324+4*i)==8,"Snow frame records crossed terrain");
    // Preserve old cumulative terrain through a rejected frame, then clear it on
    // the next successful frame. This checks the production reset and transaction.
    bool rejected=false;
    try{backend.advance(config,{.5f,0,1.e30f},1.f/60);}catch(const std::exception&){rejected=true;}
    check(rejected,"Pathological steering must reject the candidate frame");
    for(unsigned i=0;i<4;++i)check(backend.memory().word(state+0x324+4*i)==8,"Rejected frame preserves crossed terrain");
    backend.advance(config,{},1.f/60);
    for(unsigned i=0;i<4;++i)check(backend.memory().word(state+0x324+4*i)==1,"Successful road frame clears previous snow terrain");
}
void frame_state_and_recovery(NativeBackend& backend,const Config& config){
    backend.reset();auto previous=frames(backend,config,{},120);
    auto state=backend.memory().word(backend.physics_car_address()+0x50);
    float distance=backend.memory().scalar(state+0x36c),odometer=backend.memory().scalar(state+0x368);
    for(unsigned frame=0;frame<60;++frame){
        const auto before=backend.memory().vector(state+0x30);
        const auto old_position=backend.memory().vector(state+0x20);
        const auto result=backend.advance(config,{.6f,0,.1f},1.f/60);
        const auto& m=backend.memory();const auto current=m.vector(state+0x30),position=m.vector(state+0x20);
        const float inverse=1.f/ *result.dt;
        for(unsigned i=0;i<4;++i)check(m.scalar(state+0x40+4*i)==(current[i]-before[i])*inverse,"Backend acceleration is computed once from completed frame state");
        float squared=0;for(unsigned i=0;i<3;++i){const float delta=old_position[i]-position[i];squared+=delta*delta;}
        distance+=std::sqrt(squared);odometer+=m.scalar(state+0x384)* *result.dt;
        check(m.scalar(state+0x36c)==distance&&m.scalar(state+0x368)==odometer,"Backend accumulates completed frame distance and odometer");
        previous=result;
    }
    backend.visual_effects([](Memory& m,std::uint32_t car){m.word(car+0x854,1);});
    const auto saved=backend.memory().vector(state+0x40);const auto count=backend.memory().word(0x327d10);
    const auto failure=hp2::guard([&]{backend.advance(config,{},1.f/60);});
    check(failure.find("0x00132FB0")!=std::string::npos,"Deferred reset reports original missing route dependency");
    check(backend.memory().vector(state+0x40)==saved&&backend.memory().word(0x327d10)==count&&backend.memory().word(state-0x40+0x854)==1,"Rejected route recovery preserves committed state and request");
    backend.reset();complete(backend.advance(config,{},1.f/60));
}
void horn_and_lamps(NativeBackend& backend,const Config& config){
    backend.reset();frames(backend,config,{},60);Input horn;horn.horn=true;
    auto state=backend.advance(config,horn,1.f/60);
    check(state.powertrain&&state.powertrain->horn&&state.lights&&state.lights->headlight_left&&state.lights->headlight_right,"Committed horn input drives original audio/flash state");
    const auto before=backend.memory().word(0x327d10);
    backend.visual_effects([](Memory& m,std::uint32_t car){m.word(car+0x854,1);});
    check(!hp2::guard([&]{backend.advance(config,{},1.f/60);}).empty(),"Untranslated route recovery rejects release candidate");
    check(backend.memory().word(0x327d10)==before,"Rejected horn edge does not advance committed clock");
    backend.visual_effects([](Memory& m,std::uint32_t car){m.word(car+0x854,0);});
    state=backend.advance(config,{},1.f/60);
    check(!state.powertrain->horn&&!state.lights->headlight_left&&!state.lights->headlight_right,"Successful release clears horn/flash transactionally");
}
}
int main(){try{
    check(image_sha256({})=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855","Empty SHA256 vector");
    check(image_sha256({'a','b','c'})=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","ABC SHA256 vector");
    wrong_image_sha();
    const auto root=hp2::platform::resolve_zzdata_root(".");if(!root){std::cout<<"Original resources unavailable\n";return 77;}
    std::filesystem::path elf;try{elf=debug_image_path(".");}catch(const std::exception& e){std::cout<<e.what()<<'\n';return 77;}
    port::assets::DirectorySource source(*root);const auto config=read_config(source.read("GLOBAL/GLOBALB.BUN"),"911TURBO");
    reset_state_leaf(elf);
    NativeBackend backend(elf);driving_sequence(backend,config);steering_and_reset(backend,config);rejected_step_preserves_state(elf,config);session_replay(elf,config);player_transmission(backend,config);terrain_transition(backend,config);frame_state_and_recovery(backend,config);horn_and_lamps(backend,config);
    std::cout<<"DRIVING_BACKEND_SEQUENCE_OK: original data and native algorithms, qualitative port acceptance, not original equivalence\n";
}catch(const std::exception& error){std::cerr<<"DRIVING_BACKEND_FAILED: "<<error.what()<<'\n';return 1;}
}

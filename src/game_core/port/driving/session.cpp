#include "session.hpp"
#include "game_core/indep/inc/PhysicsCar.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace port::driving {
void validate_matrix(const Matrix& m){
    for(const auto f:m)if(!std::isfinite(f))throw std::invalid_argument("Non-finite driving matrix");
    if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)throw std::invalid_argument("Driving matrix must be affine");
    const double det=double(m[0])*(double(m[5])*m[10]-double(m[6])*m[9])
        -double(m[1])*(double(m[4])*m[10]-double(m[6])*m[8])
        +double(m[2])*(double(m[4])*m[9]-double(m[5])*m[8]);
    if(!std::isfinite(det)||det==0)throw std::invalid_argument("Singular driving matrix");
}
void validate_wheels(const std::array<WheelSnapshot,4>& wheels){
    for(const auto& w:wheels){
        for(const auto* matrix:{&w.tire,&w.brake,&w.blur})validate_matrix(*matrix);
        if(!std::isfinite(w.phase)||!std::isfinite(w.angular_velocity)||!std::isfinite(w.blur_alpha)
           ||w.blur_alpha<0||w.blur_alpha>128)throw std::invalid_argument("Unsupported driving wheel state");
    }
}

Config read_config(const std::vector<std::uint8_t>& bytes,const std::string& code){
    const auto chunks=hp2::assets::parse_chunks_top(bytes);
    const auto* table=hp2::assets::find_first(chunks,0x34600);
    if(!table)throw std::runtime_error("Original car config chunk0x34600 missing");
    const auto base=(table->header_offset+0x17u)&~std::size_t(15);
    const auto end=table->payload_offset()+table->size;
    if(base>end||64u*0x560u>end-base||end>bytes.size())
        throw std::runtime_error("Original car config table truncated");
    for(std::uint32_t i=0;i<64;++i){
        const auto begin=bytes.begin()+base+i*0x560u;
        const auto zero=std::find(begin,begin+32,0);
        if(zero==begin+32)throw std::runtime_error("Original car code is not terminated");
        if(std::string(begin,zero)==code){
            Config result;result.code=code;result.index=i;
            std::copy_n(begin,result.raw.size(),result.raw.begin());return result;
        }
    }
    throw std::runtime_error("Original car config not found: "+code);
}
Session::Session(Step backend):backend_(std::move(backend)){}
Session::Session(std::shared_ptr<Simulation> backend):simulation_(std::move(backend)){}
void Session::configure(Config config){config_=std::move(config);tape_.clear();elapsed_tape_.clear();recording_=false;reset();}
void Session::fail(std::string reason){pause();status_="Failed";diagnostic_=std::move(reason);}
void Session::input(Input value){
    if(!std::isfinite(value.throttle)||!std::isfinite(value.brake)||!std::isfinite(value.steering)
       ||value.throttle<0||value.throttle>1||value.brake<0||value.brake>1||std::abs(value.steering)>1)
        throw std::invalid_argument("Driving input must be finite: pedals 0..1, steering -1..1");
    if(!replay_)input_=value;
}
void Session::pause(){paused_=true;input_={};pending_elapsed_=0;if(status_=="Running")status_="Paused";}
void Session::run(){
    if(!config_){fail("Load an original vehicle config first");return;}
    paused_=false;status_="Running";diagnostic_.clear();
}
void Session::reset(){
    if(simulation_)simulation_->reset();
    pause();snapshot_={};replay_=false;cursor_=0;diagnostic_.clear();
    status_=config_?"Paused / physics not initialized":"No vehicle";
}
void Session::recording(bool enabled){
    pause();replay_=false;recording_=enabled;
    if(enabled){tape_.clear();elapsed_tape_.clear();reset();}
}
void Session::replay(){
    if(tape_.empty())throw std::logic_error("No completed simulation steps recorded");
    reset();recording_=false;replay_=true;
}
bool Session::advance(float elapsed){
    if(!config_){fail("Load an original vehicle config first");return false;}
    if(replay_&&cursor_==tape_.size()){pause();replay_=false;status_="Replay complete";return false;}
    const auto value=replay_?tape_[cursor_]:input_;
    const float clock=replay_?elapsed_tape_.at(cursor_):elapsed;
    Snapshot next;
    try{
        const auto gap=hp2::guard([&]{
            if(!backend_&&!simulation_){
                // ELF 00137100; E-0038-001. Existing whole-entry template remains
                // untranslated. No fabricated initial state or host timestep.
                hp2::recovered::PhysicsCar car;car.Sub_00137100();
                throw std::logic_error("PhysicsCar backend is not connected");
            }
            next=simulation_?simulation_->advance(*config_,value,clock):backend_(*config_,value,snapshot_);
            if(!next.dt||!std::isfinite(*next.dt)||*next.dt<=0)
                throw std::logic_error("Backend did not supply a valid evidenced timestep");
            if(next.body)validate_matrix(*next.body);
            if(next.wheels)validate_wheels(*next.wheels);
            for(const auto* vector:{&next.velocity,&next.angular_velocity})if(*vector)
                for(const auto f:**vector)if(!std::isfinite(f))throw std::logic_error("Non-finite driving motion state");
        });
        if(!gap.empty()){pause();status_="Blocked";diagnostic_=gap;return false;}
    }catch(const std::exception& error){fail(error.what());return false;}
    next.step=snapshot_.step+1;snapshot_=std::move(next);
    if(recording_){tape_.push_back(value);elapsed_tape_.push_back(clock);}
    if(replay_)++cursor_;
    return true;
}
bool Session::single_step(){paused_=true;pending_elapsed_=0;const bool ok=advance();if(ok)status_="Paused";return ok;}
bool Session::tick(float elapsed){
    if(paused_)return false;
    if(!std::isfinite(elapsed)||elapsed<0){fail("Invalid elapsed simulation clock input");return false;}
    if(simulation_&&elapsed>0){
        pending_elapsed_+=elapsed;
        if(pending_elapsed_<simulation_->minimum_frame_elapsed())return false;
        elapsed=static_cast<float>(pending_elapsed_);pending_elapsed_=0;
    }
    return advance(elapsed);
}
}

// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\carsoundengine.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/AIEngineSound.hpp"
#include "game_core/port/driving/vehicle_audio.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// E-0082-A02: Debug 815b60c7f473, PlayerEngineSound slices 0020B3D8,
// 0020B498, 0020B590, 0020B858, 0020B908, 0020C0A0 and 0020C3E8.
// Port binding of the ordinary physics-car mono branch. This does not replace
// the unresolved historical Retail ABI methods below.
namespace port::driving {
EngineAudioFrame EngineAudioMixer::update(const VehicleSoundBank& bank,const Memory& m,
        std::uint32_t car,std::uint32_t physics,float dt,float distance,int pov,float music) {
    EngineAudioFrame frame;
    if(pov<0||pov>3||!std::isfinite(dt)||dt<0||!std::isfinite(distance)||distance<0||distance>1)
        hp2::not_implemented("0x0020C0A0","EngineAudioMixer: unsupported view/distance/timestep");
    if(!m.word(0x326f40)||!m.word(0x327980))return frame;
    const auto smooth=[](float current,float target,float up,float down){
        float next=current+up;if(target<=next){next=current-down;if(next<=target)next=target;}return next;
    };
    const auto drive=m.word(physics+0x44),engine=m.word(drive+0x14),info=m.word(engine+0x10);
    const auto config=m.word(car+0xc);
    const float source_throttle=m.scalar(car+0x3cc);
    const int target=cvt_w_s(source_throttle*255.0f);
    if(throttle_<target){throttle_+=(target-throttle_)/2;if(throttle_>253)throttle_=255;}
    else if(target<throttle_){throttle_+=(target-throttle_)/8;if(throttle_<9)throttle_=0;}
    throttle_=std::clamp(throttle_,0,255);
    float raw=m.scalar(0x32818c);if(raw==100.0f)raw=m.scalar(car+0x3d0);
    if(!initialized_){current_rpm_=raw;initialized_=true;}
    float rpm=raw-static_cast<float>(bank.idle_offset);
    const int gear=m.signed_word(drive+0x30),desired=m.signed_word(drive+0x2c);
    if(gear>1){
        const float red=m.scalar(info+4)+static_cast<float>(bank.redline_offset);
        const float point=std::min(m.scalar(drive+0x4c+4*gear),red);
        if(point<=0)throw std::runtime_error("Original engine audio shift RPM unavailable");
        const float ratio=red/point;rpm=rpm*ratio;
        if(m.signed_word(car+0x3c8)==m.signed_word(config+0x288)+1)rpm+=bank.topgear_offset;
        rpm=std::min(rpm,m.scalar(info+8)*ratio);
    }
    current_rpm_=smooth(current_rpm_,rpm,270.0f,m.scalar(0x325df0));
    if(music>m.scalar(0x325dd8)){
        float duck_target=1;
        if(source_throttle<0.5f)duck_time_=0;
        else{duck_time_+=dt;if(duck_time_<=5)duck_target=duck_scale_;else duck_target=0.75f;}
        duck_scale_=smooth(duck_scale_,duck_target,m.scalar(0x325ddc),m.scalar(0x325ddc));
    }
    if(gear<desired)shifting_up_=true;else if(desired<gear)shifting_up_=false;
    float shift_target=1;
    const auto clutch=m.word(drive+0x38);
    if(shifting_up_&&raw>m.scalar(0x325dc8)&&(clutch==0||clutch==2))
        shift_target=std::max(m.scalar(0x325dcc),m.scalar(drive+0xc));
    shift_scale_=smooth(shift_scale_,shift_target,m.scalar(0x325dd0),m.scalar(0x325dd0));
    const int level=std::min(440,cvt_w_s((current_rpm_/m.scalar(info+8))*440.0f));
    const int timbre=std::min(127,int(bank.timbre[pov]));
    if(level<0||timbre<0)hp2::not_implemented("0x0020B908","EngineAudioMixer negative definition index");
    int ratio=std::min(127,(throttle_>>5)+throttle_);if(ratio<4)ratio=0;
    const int base=cvt_w_s(distance*127.0f);
    const auto little_word=[](const std::vector<std::uint8_t>& bytes,std::size_t at){
        return std::uint32_t(bytes.at(at))|(std::uint32_t(bytes.at(at+1))<<8)|
            (std::uint32_t(bytes.at(at+2))<<16)|(std::uint32_t(bytes.at(at+3))<<24);
    };
    for(unsigned group=0;group<2;++group)for(unsigned slot=0;slot<8;++slot){
        const auto& def=bank.definitions[group];const int patch=static_cast<std::int8_t>(def.at(32+slot));
        if(patch<0)continue;
        if(patch>=12)throw std::runtime_error("Invalid original engine patch");
        const auto pointer=296u+4*slot,table=pointer+little_word(def,pointer);
        int value=(int(static_cast<std::int8_t>(def.at(table+level)))*base>>7)*
            int(m.signed_byte(0x313350+(group?ratio:127-ratio)))>>7;
        if(value==0){playing_[patch]=false;continue;}
        value=std::min(127,value*5>>2);
        const bool exhaust=(patch%6)>=3;
        value=value*int(m.signed_byte(0x313350+(exhaust?timbre:127-timbre)))>>7;
        const float scale=((shift_scale_*duck_scale_)*bank.volume[patch])*m.scalar(0x325de8);
        int volume=cvt_w_s(static_cast<float>(value)*scale);
        const bool was_playing=playing_[patch];
        if(was_playing)volumes_[patch]=std::max(0,std::clamp(volume,volumes_[patch]-15,volumes_[patch]+15));
        else{volumes_[patch]=std::max(0,int(static_cast<std::int16_t>(volume)));playing_[patch]=true;}
        const int ramp=static_cast<std::int8_t>(cvt_w_s(static_cast<float>(level*127)*m.scalar(0x325de0)));
        const int cents=bank.bass_add+bank.detune[patch]+((ramp-64)*bank.bend_range>>6);
        auto& voice=frame.voices[patch];voice.playing=true;
        voice.gain=vehicle_sample_gain_scale(m,bank.engine[patch].patch_volume,
            static_cast<float>(volumes_[patch])*m.scalar(was_playing?0x325dc4:0x325dc0));
        voice.pitch_scale=vehicle_sample_pitch_scale(m,bank.engine[patch].pcm.sample_rate,
            static_cast<float>(cents)*m.scalar(0x325de4));
    }
    frame.current_rpm=current_rpm_;frame.redline_sync_rpm=rpm;frame.gear_shift_scale=shift_scale_;
    frame.current_throttle=throttle_;frame.controller_level=level;
    return frame;
}
} // namespace port::driving

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {




// ELF 0x002117C8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/002117c8.md.
void AIEngineSound::Sub_002117C8() {
    hp2::not_implemented("0x002117C8", "AIEngineSound::Sub_002117C8 (signature and full body unresolved)");
}

// ELF 0x002118A0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/002118a0.md.
// TASK-0041 E-0041-V01; debug 0x0020C5B0: AIEngineSound::Update(float, unsigned short, float); retail 0x002118A0.
void AIEngineSound::Update() {
    hp2::not_implemented("0x002118A0", "AIEngineSound::Update (signature and full body unresolved)");
}

// ELF 0x00211A80; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00211a80.md.
// TASK-0041 E-0041-V01; debug 0x0020C790: AIEngineSound::Mute(int); retail 0x00211A80.
void AIEngineSound::Mute() {
    hp2::not_implemented("0x00211A80", "AIEngineSound::Mute (signature and full body unresolved)");
}

// ELF 0x00211D58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00211d58.md.
// TASK-0041 E-0041-V01; debug 0x0020CA68: AIEngineSound::DeactivateSound(void); retail 0x00211D58.
void AIEngineSound::DeactivateSound() {
    hp2::not_implemented("0x00211D58", "AIEngineSound::DeactivateSound (signature and full body unresolved)");
}

// ELF 0x00211D60; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00211d60.md.
// TASK-0041 E-0041-V01; debug 0x0020CA70: AIEngineSound::ActivateSound(void); retail 0x00211D60.
void AIEngineSound::ActivateSound() {
    hp2::not_implemented("0x00211D60", "AIEngineSound::ActivateSound (signature and full body unresolved)");
}

// ELF 0x00212418; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00212418.md.
void AIEngineSound::Sub_00212418() {
    hp2::not_implemented("0x00212418", "AIEngineSound::Sub_00212418 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/PlayerEngineSound.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {




// ELF 0x0020FC70; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0020fc70.md.
void PlayerEngineSound::Sub_0020FC70() {
    hp2::not_implemented("0x0020FC70", "PlayerEngineSound::Sub_0020FC70 (signature and full body unresolved)");
}

// ELF 0x0020FE70; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0020fe70.md.
void PlayerEngineSound::Sub_0020FE70() {
    hp2::not_implemented("0x0020FE70", "PlayerEngineSound::Sub_0020FE70 (signature and full body unresolved)");
}

// ELF 0x00210400; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00210400.md.
// TASK-0041 E-0041-V01; debug 0x0020B110: PlayerEngineSound::DeactivateSound(void); retail 0x00210400.
void PlayerEngineSound::DeactivateSound() {
    hp2::not_implemented("0x00210400", "PlayerEngineSound::DeactivateSound (signature and full body unresolved)");
}

// ELF 0x00210408; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00210408.md.
// TASK-0041 E-0041-V01; debug 0x0020B118: PlayerEngineSound::ActivateSound(void); retail 0x00210408.
void PlayerEngineSound::ActivateSound() {
    hp2::not_implemented("0x00210408", "PlayerEngineSound::ActivateSound (signature and full body unresolved)");
}

// ELF 0x00210410; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00210410.md.
// TASK-0041 E-0041-V01; debug 0x0020B120: PlayerEngineSound::Mute(int); retail 0x00210410.
void PlayerEngineSound::Mute() {
    hp2::not_implemented("0x00210410", "PlayerEngineSound::Mute (signature and full body unresolved)");
}

// ELF 0x00211390; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00211390.md.
// TASK-0041 E-0041-V01; debug 0x0020C0A0: PlayerEngineSound::Update(float, unsigned short, float); retail 0x00211390.
void PlayerEngineSound::Update() {
    hp2::not_implemented("0x00211390", "PlayerEngineSound::Update (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x002105D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002105d0.md.
void PlayerEngineSound::ResolveTables() {
    hp2::not_implemented("0x002105D0", "PlayerEngineSound::ResolveTables (signature and full body unresolved)");
}
} // namespace hp2::recovered

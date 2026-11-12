// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\carsoundengine.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/AIEngineSound.hpp"
#include "game_core/port/race/car_sound_construction.hpp"
#include "game_core/port/race/control_construction.hpp"
#include "game_core/src/bSample.hpp"
#include "game_core/src/bMath.hpp"
#include <cstring>

namespace hp2::debug {
// Debug0020AB30, E-0099-W31. All34 embedded samples use their real constructor.
void PlayerEngineSound::Sub_0020AB30(std::uint32_t car,::port::race::CarSoundResources& resources){
 Sub_00203980();m_.word(address_+0x1c,0x2b3b60);bSample(m_,address_+0x30).Sub_0026B2A0();bSample(m_,address_+0xec).Sub_0026B2A0();
 for(const auto side:{0x1f8u,0xe78u})for(unsigned i=0;i<16;++i){const auto voice=address_+side+200*i;m_.half(voice,0);m_.half(voice+2,0);m_.scalar(voice+4,0);m_.half(voice+8,0);bSample(m_,voice+12).Sub_0026B2A0();}
 for(unsigned i=0;i<12;++i){const auto entry=address_+0x1b14+20*i;m_.word(entry,0xffffffff);m_.half(entry+4,0);m_.half(entry+6,1);m_.word(entry+8,0);m_.word(entry+12,0);m_.scalar(entry+16,1);}
 m_.word(address_+0x20,car);m_.word(address_+0x24,0);float rpm=m_.scalar(0x32818c);if(rpm==100.0f)rpm=m_.scalar(car+0x3d0);
 m_.word(address_+0x1b0,0);m_.word(address_+0x1e8,0);m_.scalar(address_+0x1b4,0);m_.scalar(address_+0x1f0,0);m_.scalar(address_+0x1ec,rpm);m_.scalar(address_+0x1f4,1);m_.scalar(address_+0x1ac,1);
 Init(resources);bSample(m_,address_+0x30).Initialize(::port::race::required_ingame_sample(resources,::port::race::original_sound_text(m_,0x2b3a30)));bSample(m_,address_+0x30).SetCategory(2);
 bSample(m_,address_+0xec).Initialize(::port::race::required_ingame_sample(resources,::port::race::original_sound_text(m_,0x2b3a40)));bSample(m_,address_+0xec).SetCategory(2);Activate(true);
}
// Debug0020AE48. Original authored car patches and two independent EngineDefs.
void PlayerEngineSound::Init(::port::race::CarSoundResources& resources){
 if(!m_.word(0x326f40))return;const auto car=m_.word(address_+0x20),info=m_.word(car+0xc);const auto bank=::port::race::original_sound_text(m_,info+0x330),prefix=::port::race::original_sound_text(m_,info+0x356);
 if(!resources.region(bank))hp2::not_implemented("0x0026FAB8","actual player engine sound region is not loaded");m_.word(address_+0x1e4,1);m_.word(address_+0x1c04,m_.word(0x327f9c));
 for(unsigned i=0;i<4;++i)m_.half(address_+0x1dc+2*i,std::uint16_t(std::int16_t(m_.signed_byte(info+0x360+i))));
 m_.word(address_+0x1d4,std::uint32_t(std::int32_t(m_.signed_half(info+0x364))));m_.word(address_+0x1d8,std::uint32_t(std::int32_t(m_.signed_half(info+0x366))));
 m_.scalar(address_+0x28,float(m_.signed_half(info+0x3b0)));m_.scalar(address_+0x2c,float(m_.signed_half(info+0x3b2)));
 for(unsigned i=0;i<12;++i){const auto entry=address_+0x1b14+20*i;const auto suffix=::port::race::original_sound_text(m_,m_.word(0x2b3a50+4*i));
  m_.word(entry+12,std::uint32_t(std::int32_t(m_.signed_half(info+0x368+2*i))));m_.scalar(entry+16,m_.scalar(info+0x380+4*i));m_.half(entry+4,suffix.size()>1&&suffix[1]=='x'?1:0);
  const auto source=resources.sample(bank,prefix+suffix);if(!source)return;m_.word(entry,1);
  for(const auto side:{0x1f8u,0xe78u}){bSample sample(m_,address_+side+200*i+12);sample.Initialize(source);sample.SetCategory(2);}
 }
 m_.word(address_+0x1b08,resources.engine_definition(0));m_.word(address_+0x1b0c,resources.engine_definition(1));ResolveTables();
}
// Debug0020B2E0. The unusual i>>3 HighestPatch indexing is listing-confirmed;
// do not silently replace it with modulo8 or a legacy host-mixer interpretation.
void PlayerEngineSound::ResolveTables(){
 for(unsigned n=0;n<2;++n){const auto definition=m_.word(address_+0x1b08+4*n);if(m_.byte(definition+5)==0){
  for(unsigned i=0;i<8;++i){const auto first=definition+0x128+4*i,second=definition+0x148+4*i;m_.word(first,first+m_.word(first));m_.word(second,second+m_.word(second));}m_.byte(definition+5,1);}}
 m_.word(address_+0x1bc,0);m_.half(address_+0x1cc,0);m_.byte(address_+0x1cb,0);
 for(unsigned i=0;i<16;++i){const auto definition=m_.word(address_+0x1b08+(i<8?0:4));if(m_.signed_byte(definition+0x20+(i>>3))>=0)m_.byte(address_+0x1cb,std::uint8_t(i+1));}
}
// Debug0020C4D8, E-0099-W31. Actual shared sound RNG and authored AI patch.
void AIEngineSound::Sub_0020C4D8(std::uint32_t car,::port::race::CarSoundResources& resources){
 Sub_00203980();m_.word(address_+0x1c,0x2b3b20);bSample(m_,address_+0x24).Sub_0026B2A0();Average(m_,address_+0xec).Sub_0018DBF8(20);
 m_.word(address_+0x20,car);m_.scalar(address_+0xe8,0);m_.scalar(address_+0xe4,-1);m_.word(address_+0xfc,0xffffffff);m_.word(address_+0x100,10);m_.scalar(address_+0x104,1000);m_.scalar(address_+0x108,1);
 m_.scalar(address_+0xe0,-hp2::recovered::bRandom(m_,0x327eec,m_.scalar(0x325df4)));
 const auto types=m_.word(0x3278e0);if(!types)hp2::not_implemented("0x0020C7A0","actual CarTypeInfoArray is not bound for AI engine sample selection");
 const auto type=m_.signed_word(m_.word(car+0x10));std::int8_t number=0;if(m_.signed_word(types+std::uint32_t(type)*0x560+0x538)!=2)number=std::int8_t(m_.half(m_.word(car+0xc)+0x354));
 const char name[]="AIEngine0";for(unsigned i=0;i<sizeof(name);++i)m_.byte(0x32b988+i,std::uint8_t(name[i]));m_.byte(0x32b990,std::uint8_t(m_.byte(0x32b990)+std::uint8_t(number)));
 bSample(m_,address_+0x24).Initialize(::port::race::required_ingame_sample(resources,::port::race::original_sound_text(m_,0x32b988)));bSample(m_,address_+0x24).SetCategory(0);
}
// Debug0020AD30/0020D128. No native mixer substitutes original sample lifetime.
void PlayerEngineSound::Sub_0020AD30(int flags){m_.word(address_+0x1c,0x2b3b60);for(unsigned i=0;i<2;++i)if(auto definition=m_.word(address_+0x1b08+4*i)){m_.release(definition);m_.word(address_+0x1b08+4*i,0);}
 for(const auto side:{0xe78u,0x1f8u})for(int i=15;i>=0;--i)bSample(m_,address_+side+200*std::uint32_t(i)+12).Sub_0026B3F8(2);
 bSample(m_,address_+0xec).Sub_0026B3F8(2);bSample(m_,address_+0x30).Sub_0026B3F8(2);Sub_00204060(flags);}
void AIEngineSound::Sub_0020D128(int flags){m_.word(address_+0x1c,0x2b3b20);Average(m_,address_+0xec).Sub_0018DC70(2);bSample(m_,address_+0x24).Sub_0026B3F8(2);Sub_00204060(flags);}
}
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

namespace hp2::debug {
// Debug0020CA70, E-0099-W40. Original activation queues a real silent-start voice.
void AIEngineSound::ActivateSound(){if(m_.word(0x326f40)&&m_.word(0x327980))bSample(m_,address_+0x24).Play(0,0,0);}
}

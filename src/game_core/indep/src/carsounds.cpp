// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\carsounds.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/HornSound3d.hpp"

#include "game_core/BaseAICarSound.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/race/car_sound_construction.hpp"

namespace hp2::debug {
// E-0099-W68: integer queue stores in branch delay slots, Debug0018B490.
void BaseCarSound::TriggerSplash(int side,int intensity){m_.word(address_+(side?0x28:0x24),std::uint32_t(intensity));}
}
#include "game_core/src/bSample.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/track.hpp"
#include <string>
#include <stdexcept>

namespace {
// Port-only bounded extraction of original strings, never an invented sample.
std::string port_sound_text(const ::port::driving::Memory& m,std::uint32_t at){std::string s;for(unsigned i=0;i<4096;++i){const auto c=m.byte(at+i);if(!c)return s;s.push_back(char(c));}throw std::out_of_range("Unterminated original car sound string");}
std::uint32_t port_required_ingame(::port::race::CarSoundResources& resources,const std::string& name){const auto sample=resources.in_game_sample(name);if(!sample)hp2::not_implemented("0x001AB900","required authored InGame sample is absent");return sample;}
// Exact constructor sequence emitted inline in BasePlayer/AI constructors.
std::uint32_t port_construct_horn(::port::driving::Memory& m,std::uint32_t source,::port::race::CarSoundResources& resources){
 const auto at=m.allocate(0xe0);hp2::debug::Sound3d(m,at).Sub_00203980();m.word(at+0x1c,0x2ac678);hp2::debug::bSample sample(m,at+0x20);sample.Sub_0026B2A0();
 if(!source){auto name=port_sound_text(m,0x327998);if(name.size()<6)throw std::out_of_range("Original horn template too short");name[5]=char(m.signed_word(0x327994)+'1');m.word(0x327994,std::uint32_t((m.signed_word(0x327994)+1)%3));source=port_required_ingame(resources,name);}
 sample.Initialize(source);sample.SetCategory(0);m.scalar(at+0xdc,1);return at;
}
// Debug0018D390. All three actual source samples and shared selector are kept.
std::uint32_t port_construct_siren(::port::driving::Memory& m,::port::race::CarSoundResources& resources){
 const auto at=m.allocate(0x268);hp2::debug::Sound3d(m,at).Sub_00203980();m.word(at+0x1c,0x2ac790);
 for(unsigned i=0;i<3;++i)hp2::debug::bSample(m,at+0x20+i*0xbc).Sub_0026B2A0();m.scalar(at+0x258,17);
 for(unsigned i=0;i<3;++i){m.byte(0x2ac5c9,std::uint8_t('1'+i));const auto source=resources.sample(port_sound_text(m,0x327988),port_sound_text(m,0x2ac5c0));if(!source)hp2::not_implemented("0x0018D390","required authored cop siren sample is absent");
  hp2::debug::bSample sample(m,at+0x20+i*0xbc);sample.Initialize(source);sample.SetCategory(0);}
 m.word(0x327990,std::uint32_t((m.signed_word(0x327990)+1)%3));m.scalar(at+0x264,1);m.word(at+0x25c,0);m.scalar(at+0x260,1);m.word(at+0x254,m.word(0x327990));return at;
}
void port_destroy_sound_child(::port::driving::Memory& m,std::uint32_t at){
 const auto table=m.word(at+0x1c);
 if(table==0x2ac678){m.word(at+0x1c,table);hp2::debug::bSample(m,at+0x20).Sub_0026B3F8(2);hp2::debug::Sound3d(m,at).Sub_00204060(3);}
 else if(table==0x2ac790){m.word(at+0x1c,table);for(int i=2;i>=0;--i)hp2::debug::bSample(m,at+0x20+std::uint32_t(i)*0xbc).Sub_0026B3F8(2);hp2::debug::Sound3d(m,at).Sub_00204060(3);}
 else if(table==0x2b3b20)hp2::debug::AIEngineSound(m,at).Sub_0020D128(3);
 else if(table==0x2b3b60)hp2::debug::PlayerEngineSound(m,at).Sub_0020AD30(3);
 else if(table==0x2b3ce8)hp2::debug::kAITireSound(m,at).Sub_0020F5A8(3);
 else if(table==0x2b3d38)hp2::debug::PlayerTireSound(m,at).Sub_0020DA00(3);
 else hp2::not_implemented("0x0018B260","original child Sound3d destructor target is not bound");

}
}
namespace hp2::debug {
// Debug0018D100, E-0099-W31. Explicit original process-static envelope fields.
void Sub_0018D100(::port::driving::Memory& m){m.half(0x327978,0x2aaa);m.scalar(0x3077f0,m.scalar(0x3254c0));m.scalar(0x3077f8,m.scalar(0x3254c4));m.scalar(0x307800,m.scalar(0x3254c8));m.scalar(0x3077e8,m.scalar(0x3254c4));m.scalar(0x3077ec,260);m.half(0x329420,0x4000);m.scalar(0x3077fc,130);}
// Debug0018B148, E-0099-W31. Shared samples remain owned by real source regions.
void BaseCarSound::Sub_0018B148(std::uint32_t car,::port::race::CarSoundResources& resources){
 Sub_00203980();m_.word(address_+0x20,car);m_.scalar(address_+0x24,0);m_.scalar(address_+0x28,0);m_.scalar(address_+0x2c,0);m_.scalar(address_+0x30,0);m_.word(address_+0x1c,0x2ac748);
 for(int i=0;i<3;++i){auto name=port_sound_text(m_,0x3279a0);if(name.size()<7)throw std::out_of_range("Original splash template too short");name[6]=char('1'+i);bSample(m_,port_required_ingame(resources,name)).SetCategory(0);}
 const auto blowout=port_required_ingame(resources,"TirePop1");m_.word(address_+0x40,blowout);bSample(m_,blowout).SetCategory(0);m_.word(address_+0x3c,0);m_.word(address_+0x44,0);m_.word(address_+0x48,0);
 m_.word(address_+0x38,m_.signed_word(m_.word(car+0xc)+0x538)==1?port_construct_siren(m_,resources):0);
}
// Debug0018C6F0; AI engine begins inactive, actual AITire activation is empty.
void BaseAICarSound::Sub_0018C6F0(std::uint32_t car,::port::race::CarSoundResources& resources){
 Sub_0018B148(car,resources);m_.word(address_+0x1c,0x2ac6b8);m_.word(address_+0x4c,0x3077f8);
 const auto engine=m_.allocate(0x10c);AIEngineSound(m_,engine).Sub_0020C4D8(car,resources);m_.word(address_+0x3c,engine);
 m_.word(address_+0x34,port_construct_horn(m_,0,resources));const auto tire=m_.allocate(0x324);kAITireSound(m_,tire).Sub_0020EE70(car,resources);m_.word(address_+0x44,tire);
}
// Debug0018B9A8. Real car bank horn and authored engine/tire dependencies.
void BasePlayerCarSound::Sub_0018B9A8(std::uint32_t car,::port::race::CarSoundResources& resources){
 Sub_0018B148(car,resources);m_.word(address_+0x1c,0x2ac700);bSample(m_,address_+0x60).Sub_0026B2A0();const auto topology=m_.allocate(0x2c0);TopologyCoordinate(m_,topology).Sub_00151878();
 m_.scalar(address_+0x54,0);m_.scalar(address_+0x58,0);m_.word(address_+0x50,topology);m_.word(address_+0x5c,1);m_.word(address_+0x4c,0x3077e8);
 bSample(m_,address_+0x60).Initialize(port_required_ingame(resources,port_sound_text(m_,0x3279a8)));bSample(m_,address_+0x60).SetCategory(0);
 const auto info=m_.word(car+0xc);const auto bank=port_sound_text(m_,info+0x330),horn_name=port_sound_text(m_,info+0x356)+port_sound_text(m_,0x3279b0);
 if(!resources.region(bank))hp2::not_implemented("0x0026FAB8","required actual player car sound region is not loaded");
 m_.word(address_+0x34,port_construct_horn(m_,resources.sample(bank,horn_name),resources));const auto engine=m_.allocate(0x1c08);PlayerEngineSound(m_,engine).Sub_0020AB30(car,resources);m_.word(0x3289a4,engine);m_.word(address_+0x3c,engine);
 const auto front=m_.allocate(0xab8);PlayerTireSound(m_,front).Sub_0020D660(car,0,resources);m_.word(address_+0x44,front);
 if(m_.signed_word(0x311f78+0x30)==1){const auto left=m_.allocate(0xab8);PlayerTireSound(m_,left).Sub_0020D660(car,1,resources);m_.word(address_+0x48,left);}
 m_.half(address_+0x11c,0);if(m_.signed_word(0x311f78+0x30)>1){const auto driver=m_.signed_half(car+0xa);if(driver==0)m_.half(address_+0x11c,std::uint16_t(0-m_.half(0x327978)));else if(driver==1)m_.half(address_+0x11c,m_.half(0x327978));}
 Activate(true);
}
// Debug0018B8E0. Original virtual activation only touches engine and two tires.
void BaseCarSound::ActivateSound(){Sound3d(m_,m_.word(address_+0x3c)).Activate(true);for(unsigned i=0;i<2;++i)if(auto child=m_.word(address_+0x44+4*i))Sound3d(m_,child).Activate(true);}
// Debug0018B260/0018C7F0/0018BB80. Actual class-owned cleanup order.
void BaseCarSound::Sub_0018B260(int flags){m_.word(address_+0x1c,0x2ac748);
 for(const auto offset:{0x34u,0x3cu,0x38u,0x44u,0x48u})if(auto child=m_.word(address_+offset))port_destroy_sound_child(m_,child);Sub_00204060(flags);}
void BaseAICarSound::Sub_0018C7F0(int flags){m_.word(address_+0x1c,0x2ac6b8);Sub_0018B260(flags);}
void BasePlayerCarSound::Sub_0018BB80(int flags){m_.word(address_+0x1c,0x2ac700);bSample(m_,address_+0x60).Stop();if(auto topology=m_.word(address_+0x50))TopologyCoordinate(m_,topology).Sub_001518C0(3);bSample(m_,address_+0x60).Sub_0026B3F8(2);Sub_0018B260(flags);}
}
namespace hp2::debug::sound {
// Original factory bodies0018CB30/0018CB78/0018CBC0. Required context is a
// concrete authored data owner, never a replacement factory callback.
std::uint32_t CreatePlayerCarSound(::port::driving::Memory& m,std::uint32_t car,::port::race::CarSoundResources& resources){if(!resources.source_ready())hp2::not_implemented("0x001AB900","complete authored InGame sound sources are not bound");const auto at=m.allocate(0x120);BasePlayerCarSound(m,at).Sub_0018B9A8(car,resources);resources.register_sound(at);return at;}
std::uint32_t CreateAICarSound(::port::driving::Memory& m,std::uint32_t car,::port::race::CarSoundResources& resources){if(!resources.source_ready())hp2::not_implemented("0x001AB900","complete authored InGame sound sources are not bound");const auto at=m.allocate(0x50);BaseAICarSound(m,at).Sub_0018C6F0(car,resources);resources.register_sound(at);return at;}
std::uint32_t CreateAICopCarSound(::port::driving::Memory& m,std::uint32_t car,::port::race::CarSoundResources& resources){if(!resources.source_ready())hp2::not_implemented("0x001AB900","complete authored InGame sound sources are not bound");const auto at=m.allocate(0x50);BaseAICarSound(m,at).Sub_0018C6F0(car,resources);resources.register_sound(at);return at;}
void DeleteCarSound(::port::driving::Memory& m,std::uint32_t at){if(m.word(at+0x1c)==0x2ac700)BasePlayerCarSound(m,at).Sub_0018BB80(3);else if(m.word(at+0x1c)==0x2ac6b8)BaseAICarSound(m,at).Sub_0018C7F0(3);else hp2::not_implemented("0x0018B260","original car sound destructor type is not bound");}
void DeleteCarSound(::port::driving::Memory& m,std::uint32_t at,::port::race::CarSoundResources& resources){DeleteCarSound(m,at);resources.unregister_sound(at);}
}

namespace hp2::recovered {
// ELF 0x0018E7E8; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018e7e8.md.
void BaseAICarSound::Sub_0018E7E8() {
    hp2::not_implemented("0x0018E7E8", "BaseAICarSound::Sub_0018E7E8 (signature and full body unresolved)");
}

// ELF 0x0018E800; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018e800.md.
void BaseAICarSound::Update() {
    hp2::not_implemented("0x0018E800", "BaseAICarSound::Update (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/CarSounds.hpp"

namespace hp2::recovered {
// ELF 0x0018D480; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018d480.md.
void BaseCarSound::GetHornSample() {
    hp2::not_implemented("0x0018D480", "BaseCarSound::GetHornSample (signature and full body unresolved)");
}

// ELF 0x0018D550; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018d550.md.
void BaseCarSound::Mute() {
    hp2::not_implemented("0x0018D550", "BaseCarSound::Mute (signature and full body unresolved)");
}

// ELF 0x0018D830; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018d830.md.
void BaseCarSound::DeactivateSound() {
    hp2::not_implemented("0x0018D830", "BaseCarSound::DeactivateSound (signature and full body unresolved)");
}

// ELF 0x0018D8D8; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018d8d8.md.
void BaseCarSound::ActivateSound() {
    hp2::not_implemented("0x0018D8D8", "BaseCarSound::ActivateSound (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/BasePlayerCarSound.hpp"

namespace hp2::recovered {
// ELF 0x0018DB78; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018db78.md.
void BasePlayerCarSound::Sub_0018DB78() {
    hp2::not_implemented("0x0018DB78", "BasePlayerCarSound::Sub_0018DB78 (signature and full body unresolved)");
}

// ELF 0x0018DBF0; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018dbf0.md.
void BasePlayerCarSound::Update() {
    hp2::not_implemented("0x0018DBF0", "BasePlayerCarSound::Update (signature and full body unresolved)");
}

// ELF 0x0018DD10; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018dd10.md.
void BasePlayerCarSound::Mute() {
    hp2::not_implemented("0x0018DD10", "BasePlayerCarSound::Mute (signature and full body unresolved)");
}

// ELF 0x0018DDA0; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018dda0.md.
void BasePlayerCarSound::GetHornSample() {
    hp2::not_implemented("0x0018DDA0", "BasePlayerCarSound::GetHornSample (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0018F160; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f160.md.
void HornSound3d::ActivateSound() {
    hp2::not_implemented("0x0018F160", "HornSound3d::ActivateSound (signature and full body unresolved)");
}

// ELF 0x0018F188; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f188.md.
void HornSound3d::DeactivateSound() {
    hp2::not_implemented("0x0018F188", "HornSound3d::DeactivateSound (signature and full body unresolved)");
}

// ELF 0x0018F208; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f208.md.
void HornSound3d::Mute() {
    hp2::not_implemented("0x0018F208", "HornSound3d::Mute (signature and full body unresolved)");
}

// ELF 0x0018F218; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f218.md.
void HornSound3d::Update() {
    hp2::not_implemented("0x0018F218", "HornSound3d::Update (signature and full body unresolved)");
}

// ELF 0x0018F2B0; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f2b0.md.
void HornSound3d::Sub_0018F2B0() {
    hp2::not_implemented("0x0018F2B0", "HornSound3d::Sub_0018F2B0 (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/SirenSound3d.hpp"

namespace hp2::recovered {
// ELF 0x0018F4D0; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f4d0.md.
void SirenSound3d::ActivateSound() {
    hp2::not_implemented("0x0018F4D0", "SirenSound3d::ActivateSound (signature and full body unresolved)");
}

// ELF 0x0018F508; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f508.md.
void SirenSound3d::DeactivateSound() {
    hp2::not_implemented("0x0018F508", "SirenSound3d::DeactivateSound (signature and full body unresolved)");
}

// ELF 0x0018F528; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f528.md.
void SirenSound3d::Mute() {
    hp2::not_implemented("0x0018F528", "SirenSound3d::Mute (signature and full body unresolved)");
}

// ELF 0x0018F548; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f548.md.
void SirenSound3d::Update() {
    hp2::not_implemented("0x0018F548", "SirenSound3d::Update (signature and full body unresolved)");
}

// ELF 0x0018F710; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0018f710.md.
void SirenSound3d::Sub_0018F710() {
    hp2::not_implemented("0x0018F710", "SirenSound3d::Sub_0018F710 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0018D258; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0018d258.md.
void BaseCarSound::Sub_0018D258() {
    hp2::not_implemented("0x0018D258", "BaseCarSound::Sub_0018D258 (signature and full body unresolved)");
}

// ELF 0x0018D340; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0018d340.md.
void BaseCarSound::Update() {
    hp2::not_implemented("0x0018D340", "BaseCarSound::Update (signature and full body unresolved)");
}

// ELF 0x0018D750; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0018d750.md.
void BaseCarSound::InSoundSphere() {
    hp2::not_implemented("0x0018D750", "BaseCarSound::InSoundSphere (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug0018D168/0018D190, E-0099-W40. Real queue/voice ownership at zero gain.
void HornSound3d::ActivateSound(){if(m_.word(0x326f40))bSample(m_,address_+0x20).Play(0,0,0);}
void HornSound3d::DeactivateSound(){bSample(m_,address_+0x20).Stop();}
// Debug0018D4D8/0018D510, E-0099-W40. Original current-siren table selection.
void SirenSound3d::ActivateSound(){if(m_.word(0x326f40))bSample(m_,address_+0x20+m_.word(address_+0x254)*0xbc).Play(0,0,0);}
void SirenSound3d::DeactivateSound(){bSample(m_,address_+0x20+m_.word(address_+0x254)*0xbc).Stop();}
}

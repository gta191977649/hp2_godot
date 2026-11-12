// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/sound3dsample.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/inc/Sound3d.hpp"
#include "game_core/src/bSample.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include <cstdio>

namespace {
void port_require_oneshot_list(const ::port::driving::Memory& m){if(!m.word(0x3288f0)||!m.word(0x3288f4))hp2::not_implemented("0x00204C40","original sound3d_oneshot static list is not initialized");}
void port_delete_oneshot(::port::driving::Memory& m,std::uint32_t object){const auto table=m.word(object+0x24),target=m.word(table+12),adjusted=object+8+std::uint32_t(std::int32_t(m.signed_half(table+8)));if(target!=0x204490){char address[11];std::snprintf(address,sizeof(address),"0x%08X",target);hp2::not_implemented(address,"unimplemented original Sound3dSample virtual destructor");}hp2::debug::Sound3dSample(m,adjusted).Sub_00204490(3);}
}
namespace hp2::debug {
// Debug00204C40, E-0099-W40. Exact original process-static intrusive list.
void Sub_00204C40(::port::driving::Memory& m){m.word(0x3288f4,0x3288f0);m.word(0x3288f0,0x3288f0);}
// Debug002043F0, E-0099-W40. Sound3d is the actual base subobject at+8;
// embedded bSample at28 and envelopeE4 are kept in the same original110 bytes.
void Sound3dSample::Sub_002043F0(std::uint32_t position,float base_volume){Sound3d(m_,address_+8).Sub_00203980();m_.word(address_+0x24,0x2b2f58);bSample(m_,address_+0x28).Sub_0026B2A0();m_.scalar(address_+0xe4,base_volume);m_.scalar(address_+0xe8,m_.scalar(0x325dbc));m_.scalar(address_+0xec,135);m_.copy(address_+0x100,position,16);port_require_oneshot_list(m_);const auto first=m_.word(0x3288f0);m_.word(0x3288f0,address_);m_.word(first+4,address_);m_.word(address_+4,0x3288f0);m_.word(address_,first);m_.word(address_+0xf4,1);}
// Debug00204490/00204538, E-0099-W40. Original unlink, embedded cleanup and
// own-slot release. Reset clears deletion callback before invoking this dtor.
void Sound3dSample::Sub_00204490(int flags){m_.word(address_+0x24,0x2b2f58);const auto next=m_.word(address_),previous=m_.word(address_+4);m_.word(previous,next);m_.word(next+4,previous);bSample(m_,address_+0x28).Sub_0026B3F8(2);Sound3d(m_,address_+8).Sub_00204060(0);if(flags&1){const auto pool=m_.word(0x3288f8);if(!pool)hp2::not_implemented("0x00204538","original Sound3dSample pool is not initialized");SlotPool(m_,pool).Free(address_);}}
// Debug00204680, E-0099-W40. Actual embedded callback/data, not the misleading
// overlapping named fields in the imported Sound3dSample datatype.
void Sound3dSample::SetDeleteWhenDone(bool value){m_.word(address_+0x5c,value?address_:0);m_.word(address_+0x58,value?0x204640:0);}
// Debug00204640, E-0099-W40. Message8 completion invokes real virtual deletion.
void RemoveSample(::port::driving::Memory& m,std::uint32_t message,std::uint32_t object,std::uint32_t private_data){(void)object;if(message==8&&private_data)port_delete_oneshot(m,private_data);}
// Debug002045A0/00204548, E-0099-W40. Position update dispatches the same
// concrete original subtype; unused input azimuth is genuinely unused there.
void Sound3dSample::Update(float volume,std::uint16_t azimuth,float pitch){(void)azimuth;if(m_.word(0x326f40)&&m_.word(address_+8)){Sound3d(m_,address_+8).Update(address_+0x100,address_+0xe4);SetVolume(m_.scalar(address_+0x10)*volume);SetPitch(m_.scalar(address_+0x14)*pitch);bSample(m_,address_+0x28).SetAzimuth(m_.half(address_+0x18));}}
void Sound3dSample::Update(std::uint32_t position,float volume,std::uint16_t azimuth,float pitch){if(m_.word(0x326f40)&&m_.word(address_+8)){m_.copy(address_+0x100,position,16);m_.word(address_+0xf4,1);const auto table=m_.word(address_+0x24);if(m_.word(table+0x1c)!=0x2045a0||m_.signed_half(table+0x18)!=-8)hp2::not_implemented("0x00204548","unknown derived Sound3dSample update target");Update(volume,azimuth,pitch);}}
namespace sound3d_oneshot {
// Debug00204090, E-0099-W40. Actual30-slot110-byte original pool component.
void Init(::port::driving::Memory& m){if(!m.word(0x3288f8))m.word(0x3288f8,bNewSlotPool(m,0x110,30,0x2b2f18,0x10));}
// Debug002040C8/00204128, E-0099-W40. Disable callbacks, then virtual-delete all.
void Reset(::port::driving::Memory& m){port_require_oneshot_list(m);while(m.word(0x3288f0)!=0x3288f0){const auto object=m.word(0x3288f0);Sound3dSample(m,object).SetDeleteWhenDone(false);if(object)port_delete_oneshot(m,object);}}
void Close(::port::driving::Memory& m){Reset(m);if(const auto pool=m.word(0x3288f8)){bDeleteSlotPool(m,pool);m.word(0x3288f8,0);}}
// Debug00204160, E-0099-W40. Original paused-word gate; no fake completion.
void Update(::port::driving::Memory& m){port_require_oneshot_list(m);for(auto object=m.word(0x3288f0);object!=0x3288f0;object=m.word(object))if(!m.word(object+0xa8)){const auto table=m.word(object+0x24);if(m.word(table+0x1c)!=0x2045a0||m.signed_half(table+0x18)!=-8)hp2::not_implemented("0x00204160","unknown original one-shot virtual update target");Sound3dSample(m,object).Update(1,0,1);}}
// Debug00204298/00204518, E-0099-W40. Real pool allocation/constructor, source
// clone, Doppler disable, virtual activation, callback and positional update.
std::uint32_t Play(::port::driving::Memory& m,std::uint32_t source,std::uint32_t position,float volume,float pitch){if(!m.word(0x326f40))return 0;if(!source)hp2::not_implemented("0x00204298","actual authored one-shot sample source is not bound");const auto pool=m.word(0x3288f8);if(!pool)hp2::not_implemented("0x00204518","original sound3d_oneshot pool is not initialized");const auto object=bOMalloc(m,pool);Sound3dSample sample(m,object);sample.Sub_002043F0(position,volume);bSample(m,object+0x28).Initialize(source);m.word(object+0xc,0);Sound3d(m,object+8).Activate(true);sample.SetDeleteWhenDone(true);sample.Update(position,volume,0,pitch);return object;}
}
}

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x002095D0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002095d0.md.
void Sound3dSample::Sub_002095D0() {
    hp2::not_implemented("0x002095D0", "Sound3dSample::Sub_002095D0 (signature and full body unresolved)");
}

// ELF 0x002096A0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002096a0.md.
void Sound3dSample::Sub_002096A0() {
    hp2::not_implemented("0x002096A0", "Sound3dSample::Sub_002096A0 (signature and full body unresolved)");
}

// ELF 0x002096F8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002096f8.md.
void Sound3dSample::Sub_002096F8() {
    hp2::not_implemented("0x002096F8", "Sound3dSample::Sub_002096F8 (signature and full body unresolved)");
}

// ELF 0x00209DC0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00209dc0.md.
void Sound3dSample::ActivateSound() {
    hp2::not_implemented("0x00209DC0", "Sound3dSample::ActivateSound (signature and full body unresolved)");
}

// ELF 0x00209E18; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00209e18.md.
void Sound3dSample::DeactivateSound() {
    hp2::not_implemented("0x00209E18", "Sound3dSample::DeactivateSound (signature and full body unresolved)");
}

// ELF 0x00209E98; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00209e98.md.
void Sound3dSample::Mute() {
    hp2::not_implemented("0x00209E98", "Sound3dSample::Mute (signature and full body unresolved)");
}
} // namespace hp2::recovered

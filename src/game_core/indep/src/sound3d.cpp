// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\sound3d.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring class with adopted retail correspondence; evidence: E-0042-S001.
// Placement: TASK-0042 E-0042-S001; exact Debug definition source/header markers where recorded; unmatched companion remains port-side; retail ABI unconfirmed.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/Sound3d.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/race/car_sound_construction.hpp"
#include <cstdio>
#include "game_core/indep/src/sound3dsample.hpp"
#include "game_core/ChopperSound.hpp"

namespace hp2::recovered {
// ELF 0x00208B28; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00208b28.md.
void Sound3d::Activate() {
    hp2::not_implemented("0x00208B28", "Sound3d::Activate (signature and full body unresolved)");
}

// ELF 0x00208C78; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00208c78.md.
void Sound3d::UpdateCamRelativeDistAndVelocity() {
    hp2::not_implemented("0x00208C78", "Sound3d::UpdateCamRelativeDistAndVelocity (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00208FE0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00208fe0.md.
void Sound3d::AzimuthToPan() {
    hp2::not_implemented("0x00208FE0", "Sound3d::AzimuthToPan (signature and full body unresolved)");
}

// ELF 0x002091A0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002091a0.md.
void Sound3d::Sub_002091A0() {
    hp2::not_implemented("0x002091A0", "Sound3d::Sub_002091A0 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug00203980, E-0099-W27, original ../indep/src/sound3d.cpp.
void Sound3d::Sub_00203980(){m_.word(address_+0x1c,0x2b2eb0);m_.word(address_+4,1);m_.scalar(address_+0xc,1);m_.word(address_,0);m_.scalar(address_+8,1);m_.half(address_+0x10,0);m_.scalar(address_+0x14,0);m_.scalar(address_+0x18,0);}
// Debug002039C0, E-0099-W27. Recovered base virtual targets are proven empty;
// derived activation requires its actual class operation, never a generic no-op.
void Sound3d::Activate(bool on){
 if(on&&m_.word(address_)){m_.word(address_,1);return;}
 const auto table=m_.word(address_+0x1c);
 if(table==0x2b2eb0){if(on)ActivateSound();else DeactivateSound();}
 else{
  const auto slot=table+(on?48:40);std::uint32_t target=0;std::int16_t delta=0;
  try{target=m_.word(slot+4);delta=m_.signed_half(slot);}catch(const std::out_of_range&){hp2::not_implemented("0x002039C0","original derived Sound3d virtual table data is not mapped");}
  const auto adjusted=address_+std::uint32_t(std::int32_t(delta));
  // Each listed target is independently recovered as an empty original body;
  // unknown subclasses never inherit a generic silent activation substitute.
  if(on && (target==0x00204000 || target==0x0020b118 || target==0x0020dad8 || target==0x0020f4b0)){}
  else if(!on && target==0x00204008){}
  else if(on && target==0x0018b8e0)BaseCarSound(m_,adjusted).ActivateSound();
  else if(on && target==0x00204c58)Sound3dSample(m_,adjusted).ActivateSound();
  else if(!on && target==0x00204cb0)Sound3dSample(m_,adjusted).DeactivateSound();
  else if(!on && target==0x0020fc20)ChopperSound(m_,adjusted).DeactivateSound();
  else if(on && target==0x0018d168)HornSound3d(m_,adjusted).ActivateSound();
  else if(!on && target==0x0018d190)HornSound3d(m_,adjusted).DeactivateSound();
  else if(on && target==0x0018d4d8)SirenSound3d(m_,adjusted).ActivateSound();
  else if(!on && target==0x0018d510)SirenSound3d(m_,adjusted).DeactivateSound();
  else if(on && target==0x0020ca70)AIEngineSound(m_,adjusted).ActivateSound();
  else{char address[11];std::snprintf(address,sizeof(address),"0x%08X",target);hp2::not_implemented(address,"original Sound3d virtual activation target is not implemented");}
 }
 m_.word(address_,on?1:0);
}
// Debug00204000/00204008, E-0099-W27: both bodies are instruction-confirmed empty.
void Sound3d::ActivateSound(){}void Sound3d::DeactivateSound(){}
// Debug00204060, E-0099-W27.
void Sound3d::Sub_00204060(int flags){m_.word(address_+0x1c,0x2b2eb0);if(flags&1)m_.release(address_);}
}

#include "game_core/indep/src/soundscape.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/src/bMath.hpp"
#include <cmath>
namespace {
float port_sound_camera_distance(::port::driving::Memory& m,std::uint32_t point,std::uint32_t camera){const auto x=m.scalar(point)-m.scalar(camera+0x40),y=m.scalar(point+4)-m.scalar(camera+0x44),z=m.scalar(point+8)-m.scalar(camera+0x48);return std::sqrt((x*x+y*y)+z*z);}
}
namespace hp2::debug {
// Debug00203A30, E-0099-W40. Exact two active-camera scan and scalar fallback.
float Sound3d::GetDistToClosestCamera(::port::driving::Memory& m,std::uint32_t point){const auto array=GetActiveCameras(m);float distance=m.scalar(0x325da4);for(unsigned i=0;i<2;++i)if(const auto camera=m.word(array+i*4)){const auto candidate=port_sound_camera_distance(m,point,camera);if(candidate<distance)distance=candidate;}
 if(distance==m.scalar(0x325da8)){const auto camera=m.word(::port::race::eGetView(m,1)+0x38);if(!camera)hp2::not_implemented("0x00203A30","actual primary eView camera pose is not initialized");distance=port_sound_camera_distance(m,point,camera);}return distance;
}
// Debug00203C90, E-0099-W40. Actual XY heading and original bASin table;
// zero-distance exceptional FP paths retain their existing explicit math guard.
std::uint16_t Sound3d::ComputeAzimuth(::port::driving::Memory& m,std::uint32_t point){const auto players=m.signed_word(0x311fa8);if(players<2){const auto camera=m.word(::port::race::eGetView(m,1)+0x38);if(!camera)hp2::not_implemented("0x00203C90","actual primary eView camera pose is not initialized");float x=m.scalar(point)-m.scalar(camera+0x40),y=m.scalar(point+4)-m.scalar(camera+0x44);const auto length=std::sqrt(x*x+y*y);x=x/length;y=y/length;const auto dir_x=m.scalar(camera+0x50),dir_y=m.scalar(camera+0x54);auto angle=std::uint16_t(0x4000-hp2::recovered::bASin(m,x*dir_x+y*dir_y));if(-y*dir_x+x*dir_y<0)angle=std::uint16_t(0u-angle);return angle;}
 const auto cameras=GetActiveCameras(m);int nearest=-1;float minimum=m.scalar(0x325dac);for(int i=0;i<players;++i){const auto camera=m.word(cameras+std::uint32_t(i)*4);if(!camera)hp2::not_implemented("0x00203C90","required active multiplayer camera pose is not initialized");const auto x=m.scalar(camera+0x40)-m.scalar(point),y=m.scalar(camera+0x44)-m.scalar(point+4);const auto distance=x*x+y*y;if(!(minimum<=distance)){minimum=distance;nearest=i;}}
 const auto azimuth=m.half(0x327978);return nearest==0?std::uint16_t(0u-azimuth):azimuth;
}
// Debug00203E20, E-0099-W40. Original envelope math and operation order.
float Sound3d::ComputeVolume(::port::driving::Memory& m,std::uint32_t point,float sphere){const auto distance=GetDistToClosestCamera(m,point);if(sphere<distance)return 0;const auto ratio=distance*(1.0f/sphere);return 1.0f-std::sqrt((distance+distance)*(1.0f/sphere)-ratio*ratio);}
// Debug00203B10, E-0099-W40. Relative velocity updates only strictly within
// (-80,80); distance always updates. Original source timestep is shared Memory.
void Sound3d::UpdateCamRelativeDistAndVelocity(std::uint32_t position){const float distance=GetDistToClosestCamera(m_,position),velocity=(distance-m_.scalar(address_+0x18))/m_.scalar(0x327d08);if(velocity<80&&-80<velocity)m_.scalar(address_+0x14,velocity);m_.scalar(address_+0x18,distance);}
// Debug00203B88, E-0099-W40. Known actual relative-distance virtual target,
// original Doppler/envelope/azimuth order; unknown targets stay explicit.
void Sound3d::Update(std::uint32_t position,std::uint32_t envelope){const auto table=m_.word(address_+0x1c);if(m_.word(table+0x3c)!=0x203b10||m_.signed_half(table+0x38)!=0)hp2::not_implemented("0x00203B88","unknown original Sound3d camera-relative update target");UpdateCamRelativeDistAndVelocity(position);if(m_.word(address_+4))m_.scalar(address_+0xc,1-m_.scalar(address_+0x14)/m_.scalar(0x3288d8));if(!envelope)m_.scalar(address_+8,1);else{const auto distance=m_.scalar(address_+0x18);float volume;if(m_.scalar(envelope+8)<distance)volume=0;else{const float ratio=distance*m_.scalar(envelope+4);volume=m_.scalar(envelope)*(1-std::sqrt((distance+distance)*m_.scalar(envelope+4)-ratio*ratio));}m_.scalar(address_+8,volume);if(1<volume)m_.scalar(address_+8,1);}m_.half(address_+0x10,ComputeAzimuth(m_,position));}
}

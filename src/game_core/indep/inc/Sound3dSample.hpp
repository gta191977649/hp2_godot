#pragma once
// TASK-0099 E-0099-W40. Original header spelling/inline definitions corroborated
// by live MCP ECOFF markers. Explicit shared-Memory view, not host PS2 ABI.
#include "game_core/port/driving/object_view.hpp"
#include "game_core/src/bSample.hpp"
namespace hp2::debug {
class Sound3dSample:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_002043F0(std::uint32_t position,float base_volume);
 void Sub_00204490(int flags);
 // Debug00204C58, E-0099-W40. Original inline target; zero gain starts a voice.
 void ActivateSound(){if(m_.word(0x326f40)){bSample sample(m_,address_+0x28);if(!sample.IsPlaying())sample.Play(0,0,0);}}
 // Debug00204CB0, E-0099-W40. Actual embedded sample stop.
 void DeactivateSound(){bSample sample(m_,address_+0x28);if(sample.IsPlaying())sample.Stop();}
 void SetDeleteWhenDone(bool); //00204680.
 void Update(std::uint32_t position,float volume,std::uint16_t azimuth,float pitch); //00204548.
 void Update(float volume,std::uint16_t azimuth,float pitch); //002045A0.
 // Debug00204D30, E-0099-W40. Original inline setter and base value.
 void SetVolume(float volume){m_.scalar(address_+0x10,volume);bSample(m_,address_+0x28).SetVolume(volume);}
 // Debug00204D48, E-0099-W40. Original factor -> octave parameter conversion.
 void SetPitch(float pitch){m_.scalar(address_+0x14,pitch);bSample(m_,address_+0x28).SetPitch(pitch-1);}
 // Debug00204D18, E-0099-W40. Original inline azimuth and embedded sample setter.
 void SetAzimuth(std::uint16_t azimuth){m_.half(address_+0x18,azimuth);bSample(m_,address_+0x28).SetAzimuth(azimuth);}
};
}

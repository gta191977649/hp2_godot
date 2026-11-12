// Port-side declarations for exact Debug SDK software channel operations.
// TASK-0099 E-0099-W40; companion header spelling is port-side.
#pragma once
#include "game_core/port/driving/memory.hpp"
namespace hp2::debug {
int SOUND_FindFreeSPUChannels(::port::driving::Memory&,unsigned start,unsigned end,unsigned channels,bool bClobber,std::uint32_t locked); //002648B0
int SOUND_GetKeyStatus(const ::port::driving::Memory&,int); //00264B78
std::uint16_t bSnd_LockChannels(::port::driving::Memory&,std::uint16_t); //0025C610
void bSnd_UnlockChannels(::port::driving::Memory&,std::uint16_t,std::uint16_t); //0025C710
}
namespace hp2::debug {
void SOUND_StartCommand(::port::driving::Memory&,std::int16_t); //002625B8
void SOUND_AddData(::port::driving::Memory&,std::int16_t); //002628F0
void SOUND_EndCommand(::port::driving::Memory&); //00262660; blocking RPC flush remains explicit
void SOUND_SetChannelMasterVolumeGroup(::port::driving::Memory&,int,unsigned); //00267C48
void SOUND_SetSPUChannelVolume(::port::driving::Memory&,int,std::int16_t,std::int16_t); //00267D60
void SOUND_MixVol(::port::driving::Memory&,int,int); //00267C90
int SOUND_ScaleVol(int,int); //00267E30
unsigned SOUND_HzToSPU(unsigned); //00265508
int SOUND_PlaySFX(::port::driving::Memory&,int ID,int Chan,int VL,int VR,int Spd,int attack,int release,unsigned loop,int offset); //002630C8
}
namespace hp2::debug {
std::uint16_t bSnd_GetPatchID(const ::port::driving::Memory&,std::uint32_t region,std::uint32_t patch); //0025FC58
std::uint16_t bSnd_GetPatchPitch(const ::port::driving::Memory&,std::uint32_t,std::uint32_t); //0025DCC8
std::uint16_t bSnd_GetPatchVolume(const ::port::driving::Memory&,std::uint32_t,std::uint32_t); //0025DDF8
std::uint16_t bSnd_GetPatchChannels(const ::port::driving::Memory&,std::uint32_t,std::uint32_t); //0025E020
void ClearVoice(::port::driving::Memory&,std::uint16_t); //00260A48
bool bSnd_GetIsChannelBusy(const ::port::driving::Memory&,std::uint16_t); //0025C480
void bSnd_SetStereo(::port::driving::Memory&); //002585F0; inactive startup branch
int SOUND_GetStreamChannel(const ::port::driving::Memory&,unsigned); //00265C08
int SOUND_StopSound(::port::driving::Memory&,int); //00263790
void SOUND_SetChannelEffectOn(::port::driving::Memory&,int); //00265150
void SOUND_SetChannelEffectOff(::port::driving::Memory&,int); //002651B0
void bSnd_VoiceStop(::port::driving::Memory&,std::uint16_t); //0025C1A8
}
namespace hp2::debug {
std::uint16_t GetAzimuthFromPan(const ::port::driving::Memory&,std::int8_t); //00261C88
std::int8_t GetPanFromAzimuth(std::uint16_t); //00261B30
void GetVolumesFromPan(const ::port::driving::Memory&,std::uint16_t&,std::uint16_t&,std::uint16_t,std::int8_t); //0025FE50
void GetVolumesFromAzimuth(const ::port::driving::Memory&,int&,int&,int,int,int); //00261D50
std::uint16_t bSnd_VoicePlay(::port::driving::Memory&,std::uint32_t bank,std::uint32_t patch,unsigned hertz,std::uint16_t volume,std::int8_t pan,std::uint16_t channel); //00259AF8
std::uint16_t bSnd_VoicePlay(::port::driving::Memory&,std::uint32_t bank,std::uint32_t patch,unsigned hertz,std::uint16_t volume,std::int8_t pan,unsigned offset,unsigned size,std::uint16_t channel); //00259BA0
std::uint16_t bSnd_VoicePlay(::port::driving::Memory&,std::uint32_t bank,std::uint32_t patch,unsigned hertz,std::uint16_t volume,std::uint16_t azimuth,unsigned offset,unsigned size,std::uint16_t channel); //0025A078
}

namespace hp2::debug {void SOUND_GetStatus(::port::driving::Memory&,int AllowLoad,int request_check); /*00263820 E-0099-W40*/}
namespace hp2::debug {
int SOUND_SetChannelVolume(::port::driving::Memory&,int,int,int); //00263408
int SOUND_SetSmoothChannelVolume(::port::driving::Memory&,int,int,int,int); //002634E8
int SOUND_SetChannelPitch(::port::driving::Memory&,int,int); //002635D8
void SetChannelAzimuth(::port::driving::Memory&,std::uint16_t,std::uint16_t); //0025CA50
void bSnd_SetVoiceAzimuth(::port::driving::Memory&,std::uint16_t,std::uint16_t); //0025CCB0
void bSnd_SetVoiceVolume(::port::driving::Memory&,std::uint16_t,std::uint16_t); //0025CDA8
void bSnd_SetVoiceVolumePan(::port::driving::Memory&,std::uint16_t,std::uint16_t,std::int8_t); //0025D080
void bSnd_SetVoicePitch(::port::driving::Memory&,std::uint16_t,unsigned); //0025C7A8
}

namespace hp2::debug {std::uint16_t bSnd_GetVoiceVolume(const ::port::driving::Memory&,std::uint16_t); /*0025D500 E-0099-W40*/}

namespace hp2::debug {
void bSnd_SetMono(::port::driving::Memory&); //00258500,E-0112-P03.
void bSnd_SetPrologic(::port::driving::Memory&); //002586D8,E-0112-P03.
}

// TASK-0099 E-0099-W40. Exact original src/bDriverPS2.cpp confirmed by
// read-only MCP ECOFF FDR/SYMR/marker/path captures. Debug815b60c7... revision unknown.
#include "bDriverPS2.hpp"
namespace hp2::debug {
// Debug0025C610. Original reverse scan reserves the high end first.
std::uint16_t bSnd_LockChannels(::port::driving::Memory& m,std::uint16_t channels){
 const auto first=SOUND_FindFreeSPUChannels(m,47,0,channels,false,0x319550);
 if(first<0)return 0xfffc;
 for(unsigned i=0;i<channels;++i)m.byte(0x319550+std::uint32_t(first)+i,1);
 return std::uint16_t(first);
}
// Debug0025C710. Original writes the requested span; Memory validates its extent.
void bSnd_UnlockChannels(::port::driving::Memory& m,std::uint16_t channel,std::uint16_t channels){for(unsigned i=0;i<channels;++i)m.byte(0x319550+unsigned(channel)+i,0);}
}

#include "game_core/port/not_implemented.hpp"
#include <cstdio>
namespace {
bool port_sdk_name_equal(const ::port::driving::Memory& m,std::uint32_t a,std::uint32_t b,unsigned limit){for(unsigned i=0;i<limit;++i){const auto x=m.byte(a+i),y=m.byte(b+i);if(x!=y)return false;if(!x)return true;}return true;}
}
namespace hp2::debug {
// Debug0025FC58, E-0099-W40. Region first-match, reverse signed16 patch scan.
std::uint16_t bSnd_GetPatchID(const ::port::driving::Memory& m,std::uint32_t region,std::uint32_t patch){if(m.byte(region)=='#'){const auto id=std::uint16_t(std::int16_t(m.signed_byte(patch)));return id<m.half(0x3290e8)?id:0xfffa;}
 const auto table=m.word(0x3290d4),patches=m.word(0x3290d8);if(!table||!patches)hp2::not_implemented("0x0025FC58","actual consumed SDK region/patch lookup tables are not loaded");
 for(unsigned i=0;i<m.half(0x3290e6);++i){const auto info=table+i*20;if(port_sdk_name_equal(m,info,region,16)){for(int id=m.signed_half(info+18);id>=int(m.half(info+16));--id)if(port_sdk_name_equal(m,patches+std::uint32_t(id)*52,patch,32))return std::uint16_t(id);}}
 std::fputs("Original bSnd patch lookup failed\n",stderr);return 0xfffa;
}
// Debug0025DCC8/0025DDF8/0025E020, E-0099-W40.
std::uint16_t bSnd_GetPatchPitch(const ::port::driving::Memory& m,std::uint32_t region,std::uint32_t patch){const auto id=bSnd_GetPatchID(m,region,patch);return id==0xfffa?0:m.half(m.word(0x3290d8)+unsigned(id)*52+34);}
std::uint16_t bSnd_GetPatchVolume(const ::port::driving::Memory& m,std::uint32_t region,std::uint32_t patch){const auto id=bSnd_GetPatchID(m,region,patch);return id==0xfffa?0:m.half(m.word(0x3290d8)+unsigned(id)*52+36);}
std::uint16_t bSnd_GetPatchChannels(const ::port::driving::Memory& m,std::uint32_t region,std::uint32_t patch){const auto id=bSnd_GetPatchID(m,region,patch);return id==0xfffa?0:std::uint16_t((m.byte(m.word(0x3290d8)+unsigned(id)*52+33)>>4)+1);}
// Debug00260A48, E-0099-W40. Azimuth/ProVol/padding deliberately untouched.
void ClearVoice(::port::driving::Memory& m,std::uint16_t id){const auto at=0x318640+unsigned(id)*80;m.byte(at,0);m.byte(at+1,0);m.half(at+4,0);m.word(at+8,0);m.half(at+12,0);m.half(at+24,0);m.word(at+28,0);for(unsigned i=0;i<48;++i)m.byte(at+32+i,0);m.byte(at+2,0xff);m.byte(at+3,0);}
// Debug0025C480, E-0099-W40. Includes reserved stream channels, not key-only.
bool bSnd_GetIsChannelBusy(const ::port::driving::Memory& m,std::uint16_t channel){const auto status=SOUND_GetKeyStatus(m,channel);if(status<0){std::fputs("Original bSnd channel outside valid range\n",stderr);return true;}return status!=0||SOUND_GetStreamChannel(m,channel)>=0;}
// Debug002585F0, E-0099-W40. Actual inactive startup branch, no invented active
// parameter update. The original channel-mode write occurs before the scan.
void bSnd_SetStereo(::port::driving::Memory& m){m.byte(0x3290cc,2);for(unsigned i=0;i<48;++i)if(bSnd_GetIsChannelBusy(m,std::uint16_t(i))){const auto at=0x318640+i*80;bSnd_SetVoiceVolumePan(m,std::uint16_t(i),m.half(at+12),GetPanFromAzimuth(m.half(at+26)));}}
// Debug0025C1A8, E-0099-W40. Actual resident SFX stopping; streams remain explicit.
void bSnd_VoiceStop(::port::driving::Memory& m,std::uint16_t id){const auto at=0x318640+unsigned(id)*80;const auto type=m.half(at+4);const int children=m.byte(at+1)>>4;if(type==1||type==2||type==4){for(int i=0;i<=children;++i){SOUND_StopSound(m,unsigned(id)+i);ClearVoice(m,std::uint16_t(unsigned(id)+i));}}else if(type==3)hp2::not_implemented("0x0025C1A8","actual stream stop dependency is not implemented");}
}

#include <cmath>
#include "game_core/port/driving/projections.hpp"
namespace {
void port_sdk_copy(::port::driving::Memory& m,std::uint32_t to,std::uint32_t from,unsigned limit){for(unsigned i=0;i<limit;++i){const auto c=m.byte(from+i);m.byte(to+i,c);if(!c)break;}}
bool port_sdk_prefix(const ::port::driving::Memory& m,std::uint32_t at,const char* text,unsigned limit){for(unsigned i=0;i<limit;++i){const auto c=std::uint8_t(text[i]);if(c!=m.byte(at+i))return false;if(!c)return true;}return true;}
}
namespace hp2::debug {
// Debug00261C88, E-0099-W40. Source scalar table factors retained verbatim.
std::uint16_t GetAzimuthFromPan(const ::port::driving::Memory& m,std::int8_t pan){return std::uint16_t(::port::driving::cvt_w_s(pan<0?65536.0f-float(pan)*m.scalar(0x3262c0):float(pan)*m.scalar(0x3262bc)));}
// Debug00261B30, E-0099-W40. All quadrant products use original truncation.
std::int8_t GetPanFromAzimuth(std::uint16_t azimuth){const int a=azimuth;const int product=a<=0x4000?a*127:a<0xc000?(0x8000-a)*127:(0x10000-a)*-127;return std::int8_t(product/0x4000);}
// Debug0025FE50, E-0099-W40. Original mono rule and actual signed-pan table.
void GetVolumesFromPan(const ::port::driving::Memory& m,std::uint16_t& left,std::uint16_t& right,std::uint16_t volume,std::int8_t pan){const int p=m.signed_byte(0x3290cc)<2?0:pan;left=std::uint16_t((unsigned(volume)*m.half(0x3198f0+std::uint32_t(p+127)*2))>>14);right=std::uint16_t((unsigned(volume)*m.half(0x3198f0+std::uint32_t(127-p)*2))>>14);}
// Debug00261D50, E-0099-W40. Preserve original ProLogic quadrant/phase update.
void GetVolumesFromAzimuth(const ::port::driving::Memory& m,int& left,int& right,int previous_left,int previous_right,int azimuth){if(m.byte(0x3290cc)!=3)return;int previous;
 if(previous_left<0&&previous_right<0)previous=previous_left<previous_right?1:2;else if(previous_left<0||previous_right<0)previous=std::abs(previous_right)<std::abs(previous_left)?4:3;else previous=previous_left<previous_right?1:2;
 int flip=0;if(azimuth<0x4000||azimuth>0xbfff){const int quadrant=left<right?2:1;if(previous>2){flip=2;if(quadrant==2&&previous==3)flip=1;}}else{const int quadrant=left<right?3:4;if(previous<3){flip=2;if(quadrant==3&&previous==2)flip=1;}}
 if(previous_left<0)left=-left;
 if(previous_right<0)right=-right;
 if(flip==1)left=-left;else if(flip==2)right=-right;
}
// Debug00259AF8/00259BA0, E-0099-W40. Exact pan wrappers, including offset/size.
std::uint16_t bSnd_VoicePlay(::port::driving::Memory& m,std::uint32_t bank,std::uint32_t patch,unsigned hz,std::uint16_t volume,std::int8_t pan,std::uint16_t channel){return bSnd_VoicePlay(m,bank,patch,hz,volume,GetAzimuthFromPan(m,pan),0,0,channel);}
std::uint16_t bSnd_VoicePlay(::port::driving::Memory& m,std::uint32_t bank,std::uint32_t patch,unsigned hz,std::uint16_t volume,std::int8_t pan,unsigned offset,unsigned size,std::uint16_t channel){return bSnd_VoicePlay(m,bank,patch,hz,volume,GetAzimuthFromPan(m,pan),offset,size,channel);}
// Debug0025A078, E-0099-W40. Complete resident-patch enqueue path. The original
// streaming/chain branches require their distinct platform-owned resources.
std::uint16_t bSnd_VoicePlay(::port::driving::Memory& m,std::uint32_t bank,std::uint32_t patch,unsigned hz,std::uint16_t volume,std::uint16_t azimuth,unsigned offset,unsigned size,std::uint16_t channel){
 if(port_sdk_prefix(m,bank,"_STREAM",7)||port_sdk_prefix(m,bank,"_CHAIN",16))hp2::not_implemented("0x0025A078","original stream/chain voice playback resources are not implemented");
 const auto id=bSnd_GetPatchID(m,bank,patch);if(id==0xfffa)return 0xfffe;const auto info=m.word(0x3290d8)+unsigned(id)*52;const auto channels=m.byte(info+33)>>4;auto first=channel;auto pan=GetPanFromAzimuth(azimuth);
 if(channel==48){const auto free=SOUND_FindFreeSPUChannels(m,0,47,unsigned(channels)+1,false,0x319550);first=std::uint16_t(free);if(std::int16_t(first)<0)return 0xfffe;}
 if(channels==1&&unsigned(first)+1>47)return 0xfffc;
 if(hz==0xffffffff)hz=m.half(info+34);
 auto level=volume;
 for(unsigned i=0;i<=unsigned(channels);++i){if(level==0xffff)level=m.half(info+36);if(!channels){if(pan==-128)pan=m.signed_byte(info+32);}else pan=i==0?-127:127;
  std::uint16_t left=0,right=0;GetVolumesFromPan(m,left,right,level,pan);int projected_left=left,projected_right=right;if(azimuth){GetVolumesFromAzimuth(m,projected_left,projected_right,left,right,azimuth);left=std::uint16_t(projected_left)&0x7fff;right=std::uint16_t(projected_right)&0x7fff;}
  auto adsr=m.word(info+44);if(!adsr)adsr=0x1a00000a;const auto voice=0x318640+(unsigned(first)+i)*80;m.word(voice+8,hz);m.half(voice+12,level);m.word(voice+16,std::uint32_t(projected_left));m.word(voice+20,std::uint32_t(projected_right));m.half(voice+24,m.half(info+38));m.byte(voice,std::uint8_t(pan));m.word(voice+28,adsr);const auto loop=m.byte(info+33)&1;m.half(voice+4,azimuth?4:std::uint16_t(loop+1));m.byte(voice+1,std::uint8_t(loop+unsigned(channels)*16));port_sdk_copy(m,voice+32,bank,16);port_sdk_copy(m,voice+48,patch,32);m.byte(voice+2,0xff);m.half(voice+26,azimuth);
  SOUND_PlaySFX(m,int(id)-int(i),int(first)+int(i),left,right,int(SOUND_HzToSPU(hz)),int((adsr>>24)&0x7f),int(adsr&0x1f),0,int(offset));if(m.half(info+38)==0)SOUND_SetChannelEffectOff(m,int(first)+int(i));else SOUND_SetChannelEffectOn(m,int(first)+int(i));
 }
 (void)size;return first;
}
}

namespace {
void port_sdk_volume(::port::driving::Memory& m,unsigned channel,unsigned left,unsigned right){if(m.word(0x329124))hp2::debug::SOUND_SetSmoothChannelVolume(m,int(channel),int(left),int(right),6);else hp2::debug::SOUND_SetChannelVolume(m,int(channel),int(left),int(right));}
}
namespace hp2::debug {
// Debug0025CA50, E-0099-W40. Preserve phase mask and original cached unsigned
// projected-volume writes before the optional smooth-volume command.
void SetChannelAzimuth(::port::driving::Memory& m,std::uint16_t channel,std::uint16_t azimuth){const auto at=0x318640+unsigned(channel)*80;const auto pan=GetPanFromAzimuth(azimuth);std::uint16_t left=0,right=0;GetVolumesFromPan(m,left,right,m.half(at+12),pan);int projected_left=left,projected_right=right;if(m.byte(0x3290cc)!=1)GetVolumesFromAzimuth(m,projected_left,projected_right,m.signed_word(at+16),m.signed_word(at+20),azimuth);left=std::uint16_t(projected_left)&0x7fff;right=std::uint16_t(projected_right)&0x7fff;m.byte(at,std::uint8_t(pan));m.half(at+26,azimuth);m.word(at+16,left);m.word(at+20,right);port_sdk_volume(m,channel,left,right);}
// Debug0025CCB0, E-0099-W40. Original stream voices ignore this operation.
void bSnd_SetVoiceAzimuth(::port::driving::Memory& m,std::uint16_t voice,std::uint16_t azimuth){const auto at=0x318640+unsigned(voice)*80;if(m.signed_byte(at+2)==-1)for(unsigned i=0;i<=unsigned(m.byte(at+1)>>4);++i)SetChannelAzimuth(m,std::uint16_t(unsigned(voice)+i),azimuth);}
// Debug0025D080, E-0099-W40.
void bSnd_SetVoiceVolumePan(::port::driving::Memory& m,std::uint16_t voice,std::uint16_t volume,std::int8_t pan){const auto at=0x318640+unsigned(voice)*80;for(unsigned i=0;i<=unsigned(m.byte(at+1)>>4);++i){std::uint16_t left=0,right=0;GetVolumesFromPan(m,left,right,volume,pan);port_sdk_volume(m,unsigned(voice)+i,left,right);const auto current=at+i*80;m.half(current+12,volume);m.byte(current,std::uint8_t(pan));}}
// Debug0025CDA8, E-0099-W40. Equality short-circuit, ProLogic and stream modes.
void bSnd_SetVoiceVolume(::port::driving::Memory& m,std::uint16_t voice,std::uint16_t volume){const auto at=0x318640+unsigned(voice)*80;if(m.half(at+12)==volume)return;if(m.byte(0x3290cc)==3&&m.signed_byte(at+2)==-1){for(unsigned i=0;i<=unsigned(m.byte(at+1)>>4);++i)m.half(at+i*80+12,volume);bSnd_SetVoiceAzimuth(m,voice,m.half(at+26));}else for(unsigned i=0;i<=unsigned(m.byte(at+1)>>4);++i){std::uint16_t left=0,right=0;const auto current=at+i*80;GetVolumesFromPan(m,left,right,volume,m.signed_byte(current));port_sdk_volume(m,unsigned(voice)+i,left,right);m.half(current+12,volume);}}
// Debug0025C7A8, E-0099-W40. Pitch packets always emit; paused cache is retained.
void bSnd_SetVoicePitch(::port::driving::Memory& m,std::uint16_t voice,unsigned hertz){const auto pitch=SOUND_HzToSPU(hertz)&0xffffu,at=0x318640+unsigned(voice)*80;for(unsigned i=0;i<=unsigned(m.byte(at+1)>>4);++i){SOUND_SetChannelPitch(m,int(unsigned(voice)+i),int(pitch));if(!m.byte(at+3))m.word(at+i*80+8,hertz);}}
}

namespace hp2::debug {
// Debug0025D500, E-0099-W40.
std::uint16_t bSnd_GetVoiceVolume(const ::port::driving::Memory& m,std::uint16_t voice){return m.half(0x318640+unsigned(voice)*80+12);}
}

namespace hp2::debug {
// Debug00258500/002586D8; E-0112-P03. Software voice mode transition.
void bSnd_SetMono(::port::driving::Memory& m){m.byte(0x3290cc,1);for(unsigned i=0;i<48;++i)if(bSnd_GetIsChannelBusy(m,std::uint16_t(i))){const auto at=0x318640+i*80;if(m.byte(at)){m.byte(at,0);bSnd_SetVoiceVolumePan(m,std::uint16_t(i),m.half(at+12),0);}}}
void bSnd_SetPrologic(::port::driving::Memory& m){m.byte(0x3290cc,3);for(unsigned i=0;i<48;++i)if(bSnd_GetIsChannelBusy(m,std::uint16_t(i)))SetChannelAzimuth(m,std::uint16_t(i),m.half(0x318640+i*80+26));}
}

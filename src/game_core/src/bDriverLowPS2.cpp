// TASK-0099 E-0099-W40. Exact original src/bDriverLowPS2.cpp confirmed by
// read-only MCP ECOFF FDR/SYMR/marker/path captures. Debug815b60c7... revision unknown.
#include "bDriverPS2.hpp"
namespace hp2::debug {
// Debug002648B0. Preserve ascending/descending scan, exact errors, key1 clobber,
// zero-count behavior and contiguous-run reset. No device status is fabricated.
int SOUND_FindFreeSPUChannels(::port::driving::Memory& m,unsigned start,unsigned end,unsigned channels,bool clobber,std::uint32_t locked){
 if(start>=48)return -2;
 if(end>=48)return -3;
 const int step=end<start?-1:1;unsigned count=0;
 for(int i=int(start);i!=int(end)+step;i+=step){const auto status=m.byte(0x319b00+std::uint32_t(i));
  if(!m.byte(locked+std::uint32_t(i))&&(!status||(clobber&&status==1))){if(++count==channels)return step<1?i:(i+1)-int(count);}
  else count=0;
 }
 return -1;
}
// Debug00264B78, E-0099-W40. Current software key-state observation only.
int SOUND_GetKeyStatus(const ::port::driving::Memory& m,int channel){return channel<0||channel>47?-1:int(m.byte(0x319b00+std::uint32_t(channel)));}
}

#include "game_core/port/not_implemented.hpp"
#include <cstdio>
namespace hp2::debug {
// Debug002625B8, E-0099-W40. Existing/dead commands preserve original state.
void SOUND_StartCommand(::port::driving::Memory& m,std::int16_t command){if(m.word(0x32924c))return;if(m.signed_word(0x329264)==-1){m.word(0x329260,0);m.word(0x329264,std::uint32_t(std::int32_t(command)));}else std::fputs("Original SOUND_StartCommand: previous command is incomplete\n",stderr);}
// Debug002628F0, E-0099-W40. Actual signed16 payload,32-bit counter.
void SOUND_AddData(::port::driving::Memory& m,std::int16_t data){if(!m.word(0x32924c)){const auto n=m.word(0x329260);m.half(0x31c140+n*2,std::uint16_t(data));m.word(0x329260,n+1);}}
// Debug00262660, E-0099-W40. Raw IOP batching; no pretend device consumption.
void SOUND_EndCommand(::port::driving::Memory& m){if(m.signed_word(0x329264)==-1){std::fputs("Original SOUND_EndCommand: no command was started\n",stderr);return;}
 const auto at=m.word(0x3291b8),count=m.word(0x329260);m.half(0x31a140+(at+1)*2,std::uint16_t(m.word(0x329264)));m.half(0x31a140+(at+2)*2,std::uint16_t(count));
 for(int i=0;i<std::int32_t(count);++i)m.half(0x31a140+(at+std::uint32_t(i)+3)*2,m.half(0x31c140+std::uint32_t(i)*2));
 m.word(0x3291b8,at+2+count);m.word(0x329268,m.word(0x329268)+1);m.word(0x329260,0);m.word(0x329264,0xffffffff);
 if(m.word(0x3291b8)+0x10u>0xfff){m.byte(0x329220,1);hp2::not_implemented("0x00264CF0","required blocking original IOP sound command flush has no bound device");}
}
// Debug00267C48, E-0099-W40.
void SOUND_SetChannelMasterVolumeGroup(::port::driving::Memory& m,int channel,unsigned group){m.word(0x319c20+std::uint32_t(channel)*4,group);}
// Debug00267E30/listing, E-0099-W40. Preserve phase encoding and signed low32
// multiplication followed by truncation toward zero, including negative values.
int SOUND_ScaleVol(int mix,int channel){const bool phase=channel>0x3fff;const auto adjusted=phase?0x7fffu-std::uint32_t(channel):std::uint32_t(channel);auto product=std::int32_t(adjusted*std::uint32_t(mix));if(product<0)product=std::int32_t(std::uint32_t(product)+0xfff);const auto scaled=product>>12;return phase?std::int32_t(0x7fffu-std::uint32_t(scaled)):scaled;}
// Debug00267C90, E-0099-W40.
void SOUND_MixVol(::port::driving::Memory& m,int group,int channel){const auto index=std::uint32_t((group>>16)-1);m.word(0x32929c,std::uint32_t(SOUND_ScaleVol(m.signed_half(0x329250+index*2),m.signed_half(0x319b60+std::uint32_t(channel)*2))));m.word(0x3292a0,std::uint32_t(SOUND_ScaleVol(m.signed_half(0x329258+index*2),m.signed_half(0x319bc0+std::uint32_t(channel)*2))));}
// Debug00267D60, E-0099-W40.
void SOUND_SetSPUChannelVolume(::port::driving::Memory& m,int channel,std::int16_t left,std::int16_t right){const auto group=m.word(0x319c20+std::uint32_t(channel)*4);if(!group){m.word(0x32929c,std::uint32_t(std::int32_t(left)));m.word(0x3292a0,std::uint32_t(std::int32_t(right)));}else{m.half(0x319b60+std::uint32_t(channel)*2,std::uint16_t(left));m.half(0x319bc0+std::uint32_t(channel)*2,std::uint16_t(right));SOUND_MixVol(m,std::int32_t(group),channel);}}
// Debug00265508, E-0099-W40. Original32-bit shift precedes signed division.
unsigned SOUND_HzToSPU(unsigned hz){return std::uint32_t(std::int32_t(hz<<12)/48000);}
// Debug002630C8, E-0099-W40. Original low-level command0 plus eight16-bit words.
int SOUND_PlaySFX(::port::driving::Memory& m,int id,int channel,int left,int right,int speed,int attack,int release,unsigned loop,int offset){const auto index=std::uint32_t(channel)&0xffffu;if(index>=48)return -1;m.byte(0x319b00+index,3);SOUND_SetChannelMasterVolumeGroup(m,int(index),std::uint32_t(channel)&0xffff0000u);SOUND_SetSPUChannelVolume(m,int(index),std::int16_t(left),std::int16_t(right));SOUND_StartCommand(m,0);
 const auto adsr=std::uint16_t(std::uint16_t(attack)+std::uint16_t(release)*0x100u);
 for(const auto word:{std::uint16_t(id),std::uint16_t(channel),std::uint16_t(m.word(0x32929c)),std::uint16_t(m.word(0x3292a0)),std::uint16_t(speed),adsr,std::uint16_t(loop),std::uint16_t(offset)})SOUND_AddData(m,std::int16_t(word));
 SOUND_EndCommand(m);return 0;}
}

namespace hp2::debug {
// Debug00265C08, E-0099-W40.
int SOUND_GetStreamChannel(const ::port::driving::Memory& m,unsigned channel){if(channel>=48)return -2;const auto ref=m.byte(0x319b30+channel);return ref==0xff?-1:ref;}
// Debug00263790, E-0099-W40. Original stop is queued; no fake immediate key off.
int SOUND_StopSound(::port::driving::Memory& m,int channel){if(channel<0||channel>47)return -1;SOUND_StartCommand(m,4);SOUND_AddData(m,std::int16_t(channel));SOUND_EndCommand(m);return 0;}
// Debug00265150/002651B0, E-0099-W40. Preserve actual effect command payloads.
void SOUND_SetChannelEffectOn(::port::driving::Memory& m,int channel){SOUND_StartCommand(m,0x18);SOUND_AddData(m,std::int16_t(channel));SOUND_EndCommand(m);}
void SOUND_SetChannelEffectOff(::port::driving::Memory& m,int channel){SOUND_StartCommand(m,0x19);SOUND_AddData(m,std::int16_t(channel));SOUND_EndCommand(m);}
}

namespace hp2::debug {
// Debug00263820, E-0099-W40. Exact status packet and original load-request writes.
void SOUND_GetStatus(::port::driving::Memory& m,int allow_load,int request_check){if(allow_load!=-1){m.word(0x3291c4,1);m.word(0x3291dc,0xffffffff);}SOUND_StartCommand(m,9);SOUND_AddData(m,std::int16_t(allow_load));SOUND_AddData(m,std::int16_t(request_check));SOUND_EndCommand(m);m.byte(0x329220,1);}
}

namespace hp2::debug {
// Debug00263408/002634E8, E-0099-W40. Exact raw master-scaled volume packets.
int SOUND_SetChannelVolume(::port::driving::Memory& m,int channel,int left,int right){const auto id=std::uint32_t(channel)&0xffffu;if(id>=48)return -1;SOUND_SetSPUChannelVolume(m,int(id),std::int16_t(left),std::int16_t(right));SOUND_StartCommand(m,2);SOUND_AddData(m,std::int16_t(channel));SOUND_AddData(m,std::int16_t(m.word(0x32929c)));SOUND_AddData(m,std::int16_t(m.word(0x3292a0)));SOUND_EndCommand(m);return 0;}
int SOUND_SetSmoothChannelVolume(::port::driving::Memory& m,int channel,int left,int right,int duration){const auto id=std::uint32_t(channel)&0xffffu;if(id>=48)return -1;SOUND_SetSPUChannelVolume(m,int(id),std::int16_t(left),std::int16_t(right));SOUND_StartCommand(m,0xcb);SOUND_AddData(m,std::int16_t(channel));SOUND_AddData(m,std::int16_t(m.word(0x32929c)));SOUND_AddData(m,std::int16_t(m.word(0x3292a0)));SOUND_AddData(m,std::int16_t(duration));SOUND_EndCommand(m);return 0;}
// Debug002635D8, E-0099-W40.
int SOUND_SetChannelPitch(::port::driving::Memory& m,int channel,int pitch){if(channel<0||channel>47)return -1;SOUND_StartCommand(m,3);SOUND_AddData(m,std::int16_t(channel));SOUND_AddData(m,std::int16_t(pitch));SOUND_EndCommand(m);return 0;}
}

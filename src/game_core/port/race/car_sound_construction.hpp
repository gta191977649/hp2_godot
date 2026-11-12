#pragma once
// Port-side logical object views of recovered Debug classes, E-0099-W31.
#include "game_core/indep/inc/Sound3d.hpp"
#include "car_sound_resources.hpp"
#include "game_core/port/not_implemented.hpp"
namespace port::race {
inline std::string original_sound_text(const driving::Memory& m,std::uint32_t at){std::string s;for(unsigned i=0;i<4096;++i){const auto c=m.byte(at+i);if(!c)return s;s.push_back(char(c));}throw std::out_of_range("Unterminated original car sound string");}
inline std::uint32_t required_ingame_sample(CarSoundResources& resources,const std::string& name){const auto at=resources.in_game_sample(name);if(!at)hp2::not_implemented("0x001AB900","required authored InGame sample is absent");return at;}
}
namespace hp2::debug {
class HornSound3d:public Sound3d {public:using Sound3d::Sound3d;void ActivateSound();void DeactivateSound();};
class SirenSound3d:public Sound3d {public:using Sound3d::Sound3d;void ActivateSound();void DeactivateSound();};
class BaseCarSound:public Sound3d {
public:using Sound3d::Sound3d;
 void TriggerSplash(int side,int intensity); //Debug0018B490, E-0099-W68, source queue only.
 void Sub_0018B148(std::uint32_t,::port::race::CarSoundResources&);
 void Sub_0018B260(int);
 void ActivateSound();
};
class BaseAICarSound:public BaseCarSound {
public:using BaseCarSound::BaseCarSound;
 void Sub_0018C6F0(std::uint32_t,::port::race::CarSoundResources&);
 void Sub_0018C7F0(int);
};
class BasePlayerCarSound:public BaseCarSound {
public:using BaseCarSound::BaseCarSound;
 void Sub_0018B9A8(std::uint32_t,::port::race::CarSoundResources&);
 void Sub_0018BB80(int);
};
class PlayerEngineSound:public Sound3d {
public:using Sound3d::Sound3d;
 void Sub_0020AB30(std::uint32_t,::port::race::CarSoundResources&);
 void Init(::port::race::CarSoundResources&);void ResolveTables();
 void Sub_0020AD30(int);
};
class AIEngineSound:public Sound3d {
public:using Sound3d::Sound3d;
 void Sub_0020C4D8(std::uint32_t,::port::race::CarSoundResources&);
 void Sub_0020D128(int);
 void ActivateSound(); //0020CA70 E-0099-W40
};
class BaseTireSound:public Sound3d {
public:using Sound3d::Sound3d;
 void Sub_0020D200(::port::race::CarSoundResources&);void Sub_0020F680(int);
};
class PlayerTireSound:public BaseTireSound {
public:using BaseTireSound::BaseTireSound;
 void Sub_0020D660(std::uint32_t,int,::port::race::CarSoundResources&);
 void Sub_0020DA00(int);
};
class kAITireSound:public BaseTireSound {
public:using BaseTireSound::BaseTireSound;
 // Original constructor emitted inline within CreateAITire0020EE70.
 void Sub_0020EE70(std::uint32_t,::port::race::CarSoundResources&);
 void Sub_0020F5A8(int);
};
// Original process-static car sound envelope initialization0018D100.
void Sub_0018D100(::port::driving::Memory&);
}

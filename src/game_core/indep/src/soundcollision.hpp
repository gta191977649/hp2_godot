#pragma once
// Exact original soundcollision.cpp companion declarations; header is port-side.
// TASK-0099 E-0099-W40; explicit logical views, not native PS2 object ABI.
#include "game_core/src/bSample.hpp"
namespace hp2::debug {
void Sub_001E2348(::port::driving::Memory&); // Actual process-static free list.
std::uint32_t GetCollisionSoundType(::port::driving::Memory&,std::uint32_t);
float GetDistanceToCamera(::port::driving::Memory&,std::uint32_t);
class CollisionSample:public bSample {public:using bSample::bSample;void Initialize(std::uint32_t);void Sub_001E2360(int);};
class CollisionSounds {public:
 static void Register(::port::driving::Memory&);static void UnRegister(::port::driving::Memory&);
 static void LoadSampleTable(::port::driving::Memory&);static void Update(::port::driving::Memory&);
 static float CalcCollisionVolume(::port::driving::Memory&,std::uint32_t);
 static float CalcCollisionScrapeVolume(::port::driving::Memory&,std::uint32_t);
 static void StartScraping(::port::driving::Memory&,std::uint32_t);
 static void UpdateSample(::port::driving::Memory&,std::uint32_t);
};
}
namespace hp2::debug::collision_sound {
void Register(::port::driving::Memory&);void UnRegister(::port::driving::Memory&);
void Update(::port::driving::Memory&);void Reset(::port::driving::Memory&);
std::uint32_t CreateSample(::port::driving::Memory&,std::uint32_t collision); //001E1838
void PlayScrape(::port::driving::Memory&,std::uint32_t collision); //001E1D80
void PlayGlass(::port::driving::Memory&,std::uint32_t position); //001E1C20.
void ReleaseSample(::port::driving::Memory&,std::uint32_t collision); //001E1970
}

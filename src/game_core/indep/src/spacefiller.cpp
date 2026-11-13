#include "indep/inc/Pursuit.hpp"
#include "indep/src/speech/mgr.hpp"
#include "indep/inc/Car.hpp"
#include "Chopper.hpp"
#include "bware/inc/bMath.hpp"

#include <array>
#include <cmath>

namespace {
using Check=WorldServicesResult (*)(PursuitManager*,PursuitCollisionGroup*,int32_t*);
using Speak=WorldServicesResult (*)(PursuitManager*,PursuitCollisionGroup*);
struct Filler {uint32_t bank;uint32_t weight;Check appropriate;Speak speak;};

WorldServicesResult raised(PursuitManager* manager,WorldSpeechResult result)
{
    if(result==WORLD_SPEECH_OK || result==WORLD_SPEECH_SUPPRESSED || result==WORLD_SPEECH_DROPPED)
        return manager->result=WORLD_SERVICES_OK;
    return manager->result=result==WORLD_SPEECH_INVALID_BINDING ? WORLD_SERVICES_INVALID_BINDING : WORLD_SERVICES_OWNER_FAILURE;
}

// PAL22BB28..22BBB0 uses the full XYZ vector distance. ChopperClose below
// is a different source helper and deliberately measures only XY.
WorldServicesResult copNear(PursuitCollisionGroup* group,float threshold,int32_t* near)
{
    *near=0;
    for(int32_t i=0;i<group->activeCount;++i) {
        const PursuitCollisionEntry* entry=group->active[i];
        if(!entry)continue;
        if(!entry->movement || !entry->movement->bindings.state)return WORLD_SERVICES_INVALID_BINDING;
        const float* a=entry->movement->bindings.state->position;
        const float* b=group->target->bindings.state->position;
        const float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];
        const float x2=x*x,y2=y*y,z2=z*z;
        if(std::sqrt((x2+y2)+z2)<threshold){*near=1;break;}
    }
    return WORLD_SERVICES_OK;
}

// PAL22BAB8..22BB28: only the actual attacking chopper phase qualifies.
WorldServicesResult chopperClose(PursuitManager*,PursuitCollisionGroup* group,int32_t* near)
{
    *near=0;
    if(group->extendedTimeout!=3 || !group->chopper)return WORLD_SERVICES_OK;
    const float* b=group->target->bindings.state->position;
    const float x=group->chopper->position[0]-b[0],y=group->chopper->position[1]-b[1];
    const float x2=x*x,y2=y*y;
    *near=std::sqrt(x2+y2)<200.0f;return WORLD_SERVICES_OK;
}

// Base CopFillerSpeech::IsAppropriate231C10; GetWeight231C08 returns2.
WorldServicesResult copAppropriate(PursuitManager*,PursuitCollisionGroup* group,int32_t* yes)
{return copNear(group,65.0f,yes);}
WorldServicesResult copLocation(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* yes)
{
    const auto result=copAppropriate(manager,group,yes);
    if(result==WORLD_SERVICES_OK && *yes)*yes=group->owner->landmark!=0;
    return result;
}
WorldServicesResult copTactic(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* yes)
{
    const auto result=copAppropriate(manager,group,yes);
    if(result==WORLD_SERVICES_OK && *yes)*yes=!manager->parameters->specialPursuit;
    return result;
}
WorldServicesResult copSpeed(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* yes)
{
    auto result=copAppropriate(manager,group,yes);
    if(result!=WORLD_SERVICES_OK || !*yes)return result;
    float speed=0.0f;result=pursuitGroupDisplaySpeed(group,&speed);
    if(result==WORLD_SERVICES_OK)*yes=100.0f<speed;
    return result;
}
WorldServicesResult copReckless(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* yes)
{
    const auto result=copAppropriate(manager,group,yes);
    if(result==WORLD_SERVICES_OK && *yes)*yes=35.76218032836914f<=group->target->bindings.state->speed;
    return result;
}
WorldServicesResult tacticHit(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* yes)
{
    *yes=0;
    if(!(group->requestFlags&4u) || manager->parameters->specialPursuit)return WORLD_SERVICES_OK;
    int32_t secondary=0;
    auto result=pursuitGroupReadSecondarySpeaker(manager,group,&secondary);
    if(result!=WORLD_SERVICES_OK || secondary==5)return result;
    int32_t first=0;
    result=pursuitGroupReadSpeaker(manager,group,&first);if(result!=WORLD_SERVICES_OK)return result;
    // Source calls the secondary query again; it remains a real deque peek.
    result=pursuitGroupReadSecondarySpeaker(manager,group,&secondary);
    if(result!=WORLD_SERVICES_OK || first==secondary)return result;
    return copNear(group,40.0f,yes);
}
WorldServicesResult rotorTraffic(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* yes)
{
    const auto result=chopperClose(manager,group,yes);
    if(result==WORLD_SERVICES_OK && *yes)*yes=0.8999999761581421f<manager->parameters->trafficDensity;
    return result;
}
WorldServicesResult rotorLocation(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* yes)
{
    const auto result=chopperClose(manager,group,yes);
    if(result==WORLD_SERVICES_OK && *yes)*yes=group->owner->landmark!=0;
    return result;
}

WorldServicesResult speakHit(PursuitManager* manager,PursuitCollisionGroup* group)
{
    int32_t first=0,second=0;
    auto result=pursuitGroupReadSpeaker(manager,group,&first);if(result!=WORLD_SERVICES_OK)return result;
    result=pursuitGroupReadSecondarySpeaker(manager,group,&second);if(result!=WORLD_SERVICES_OK)return result;
    return raised(manager,worldSpeechRaiseTacticHit(manager->collision->speech,first,second));
}
// Both actual Direction231910 and Reckless231940 use ChatterGeneral with
// variant zero in the final executable; no speculative direction phrase.
WorldServicesResult speakGeneral(PursuitManager* manager,PursuitCollisionGroup* group)
{
    int32_t speaker=0;const auto result=pursuitGroupReadSpeaker(manager,group,&speaker);
    if(result!=WORLD_SERVICES_OK)return result;
    return raised(manager,worldSpeechRaiseChatterGeneral(manager->collision->speech,speaker));
}
WorldServicesResult speakSpeed(PursuitManager* manager,PursuitCollisionGroup* group)
{
    int32_t speaker=0;float speed=0.0f;
    auto result=pursuitGroupReadSpeaker(manager,group,&speaker);if(result!=WORLD_SERVICES_OK)return result;
    result=pursuitGroupDisplaySpeed(group,&speed);if(result!=WORLD_SERVICES_OK)return result;
    return raised(manager,worldSpeechRaiseCurrentSpeed(manager->collision->speech,speaker,speed));
}
WorldServicesResult speakTactic(PursuitManager* manager,PursuitCollisionGroup* group)
{
    const uint32_t choice=vectorDefaultRandomBounded(2);
    int32_t speaker=0;const auto result=pursuitGroupReadSpeaker(manager,group,&speaker);
    if(result!=WORLD_SERVICES_OK)return result;
    return raised(manager,choice==0 ? worldSpeechRaiseTacticDivert(manager->collision->speech,speaker) :
        worldSpeechRaiseTacticCutoff(manager->collision->speech,speaker));
}
WorldServicesResult speakLocation(PursuitManager* manager,PursuitCollisionGroup* group)
{
    int32_t speaker=0;const auto result=pursuitGroupReadSpeaker(manager,group,&speaker);
    if(result!=WORLD_SERVICES_OK)return result;
    return raised(manager,worldSpeechRaisePursuitUpdateSubjectLocation(manager->collision->speech,speaker,group->owner->landmark,0));
}
WorldServicesResult speakRotorMisc(PursuitManager* manager,PursuitCollisionGroup* group)
{return raised(manager,worldSpeechRaiseRotorChatter(manager->collision->speech,30.0f<group->target->bindings.state->forwardSpeed));}
WorldServicesResult speakRotorTraffic(PursuitManager* manager,PursuitCollisionGroup*)
{return raised(manager,worldSpeechRaiseRotorTraffic(manager->collision->speech));}
WorldServicesResult speakRotorLocation(PursuitManager* manager,PursuitCollisionGroup* group)
{return raised(manager,worldSpeechRaiseRotorLocation(manager->collision->speech,group->owner->landmark));}

// Actual final31C658 array and2317B0 static constructor/vtable slots.
// Native records bind concrete implementations, never foreign callbacks.
constexpr std::array<Filler,9> fillers{{
    {1,4,rotorLocation,speakRotorLocation},{2,4,rotorTraffic,speakRotorTraffic},
    {4,4,chopperClose,speakRotorMisc},{8,2,copLocation,speakLocation},
    {32,2,copTactic,speakTactic},{64,2,copSpeed,speakSpeed},
    {128,2,copReckless,speakGeneral},{256,2,copAppropriate,speakGeneral},
    {16,5,tacticHit,speakHit}
}};
}

// PAL2315D0..23175C. Actual class order, both weight passes and the single
// default-RNG draw remain intact. A source-suppressed event still uses bank.
WorldServicesResult pursuitTryFillerSpeech(PursuitManager* manager,PursuitCollisionGroup* group,
    uint32_t* usedBanks,int32_t* spoken)
{
    if(!manager || !group || !group->owner || group->owner->manager!=manager ||
        !manager->collision || !manager->collision->speech || !manager->parameters ||
        !group->target || !group->target->bindings.state || !group->target->bindings.state->position ||
        !usedBanks || !spoken || group->activeCount<0 || group->activeCount>2)
        return WORLD_SERVICES_INVALID_BINDING;
    std::array<const Filler*,9> eligible{};size_t count=0;uint32_t total=0;
    for(const auto& filler:fillers) {
        if(filler.bank&*usedBanks)continue;
        int32_t allowed=0;const auto result=filler.appropriate(manager,group,&allowed);
        if(result!=WORLD_SERVICES_OK)return manager->result=result;
        if(allowed){eligible[count++]=&filler;total+=filler.weight;}
    }
    *spoken=0;
    if(!total)return manager->result=WORLD_SERVICES_OK;
    const uint32_t choice=vectorDefaultRandomBounded(total);uint32_t sum=0;
    for(size_t i=0;i<count;++i) {
        const Filler& filler=*eligible[i];sum+=filler.weight;
        if(choice<sum) {
            const auto result=filler.speak(manager,group);if(result!=WORLD_SERVICES_OK)return manager->result=result;
            *usedBanks|=filler.bank;break;
        }
    }
    *spoken=1;return manager->result=WORLD_SERVICES_OK;
}

// PAL231760..2317B0. Exhausted/inappropriate masks retry once after clear.
WorldServicesResult pursuitSayFillerSpeech(PursuitManager* manager,PursuitCollisionGroup* group,uint32_t* usedBanks)
{
    int32_t spoken=0;auto result=pursuitTryFillerSpeech(manager,group,usedBanks,&spoken);
    if(result!=WORLD_SERVICES_OK || spoken)return result;
    *usedBanks=0;
    return pursuitTryFillerSpeech(manager,group,usedBanks,&spoken);
}

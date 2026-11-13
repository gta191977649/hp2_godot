#include "indep/inc/Fred.hpp"
#include "Language.hpp"
#include "bware/inc/bWare.hpp"
#include <cstdio>
#include <cstring>
#include "indep/inc/EventTree.hpp"
#include "indep/inc/Flags.hpp"

namespace hp2 {

namespace {
// Final PAL31CFC0, ten56-byte task rows: only the fields actually read by
// this complete dependency group are projected (gameType+24,car+28,id+34).
struct TaskCarReward {
    std::int32_t id;
    float undamaged;
    std::int32_t players,mode,opponents,laps,difficulty,checkpoint,cops;
    std::int32_t gameType,model,language,description,index;
};
// The complete ten actual final-PAL31CFC0 rows. The prior projection is
// replaced in place so availability and completion share one table.
constexpr TaskCarReward taskCarRewards[]={
    {1,0.0f,1,3,-1,4,1,-1,-1,-1,16,49101,960001,0},
    {2,0.0f,1,3,-1,8,1,-1,-1,-1,5,49102,960002,1},
    {3,-1.0f,1,3,3,2,1,2,1,7,31,49103,960003,2},
    {4,-1.0f,1,3,7,2,1,2,0,-1,44,49104,960004,3},
    {5,-1.0f,1,3,3,4,1,4,1,7,34,49105,960005,4},
    {6,-1.0f,1,3,7,4,1,4,0,-1,36,49106,960006,5},
    {7,-1.0f,1,3,-1,8,1,8,0,-1,24,49107,960007,6},
    {8,-1.0f,1,4,-1,-1,1,-1,1,7,32,49108,960008,7},
    {9,-1.0f,1,4,-1,-1,1,-1,0,-1,14,49109,960009,8},
    {10,-1.0f,1,5,-1,-1,1,-1,1,7,33,49110,960010,9}};
// Final PAL31D1F0, eighteen12-byte points rows. Source thresholds are
// retained beside their model/reward slot; earned state belongs to FRED.
struct PointsCarReward { std::int32_t points,model,index; };
constexpr PointsCarReward pointsCarRewards[]={
    {50000,15,0},{100000,20,1},{150000,28,2},{200000,25,3},{250000,42,4},
    {300000,23,5},{400000,30,6},{500000,12,7},{750000,40,8},{1000000,26,9},
    {1500000,3,10},{2000000,46,11},{2500000,1,12},{3000000,48,13},
    {3500000,18,14},{4000000,8,15},{4500000,6,16},{5000000,10,17}};

// Native dependency projections for the proved IsCarUnlocked owner below.
// The leaf machine-code identities are retained in the report; their beta
// debug data do not grant individual original source/header placement.
const TaskCarReward* FindTaskCarReward(std::int32_t model,std::int32_t gameType)
{
    for (const auto& reward:taskCarRewards)
        if ((reward.gameType==-1 || reward.gameType==gameType) && reward.model==model)return &reward;
    return nullptr;
}

const PointsCarReward* FindPointsCarReward(std::int32_t model)
{
    for (const auto& reward:pointsCarRewards)if (reward.model==model)return &reward;
    return nullptr;
}

}

bool IsCarUnlocked(const FrontendPlayerManager& frontend,const EventTreeResources* events,
    std::int32_t model,std::int32_t gameType)
{
    // PAL24B208: task rewards take precedence. Be-a-cop mode7 cannot use
    // event/points rewards; ordinary modes then consult the actual trees.
    // The explicit testing switch bypasses these reward gates only. It
    // does not turn unsupported/traffic catalogue rows into selectable cars.
    if (const auto* task=FindTaskCarReward(model,gameType))
        return applicationUnlockAllForTesting || frontend.taskRewardsEarned[static_cast<std::size_t>(task->index)]!=0;
    if (gameType==7)return false;
    // PAL1FFE80 clears the event registry when FRONTB retires. An empty
    // registry is valid here (PAL1FF8B8); task and points rewards remain live.
    if (events && IsCarAnEventReward(*events,model))return applicationUnlockAllForTesting || EventTreeIsCarUnlocked(*events,model);
    if (const auto* points=FindPointsCarReward(model))
        return applicationUnlockAllForTesting || frontend.pointsRewardsEarned[static_cast<std::size_t>(points->index)]!=0;
    return false;
}

bool IsTrackUnlocked(const EventTreeResources& resources,std::int32_t track,std::int32_t direction)
{
    if(applicationUnlockAllForTesting)return true;
    if (IsTrackAnEventReward(resources,track,direction))
        return EventTreeIsTrackUnlocked(resources,track,direction);
    return true;
}

void GetTrackUnlockInfo(char* output,std::size_t capacity,std::int32_t track,
    std::int32_t direction,const EventTreeResources& events,const LanguageManager& language)
{
    Require(output && capacity,"Track unlock description has no retained output buffer");
    std::string text;
    if (IsTrackAnEventReward(events,track,direction)) {
        const auto* event=FindEventThatTrackIsARewardFor(events,track,direction);
        Require(event && event->tree,"Track reward has no actual awarding event/tree");
        const auto type=event->tree->TreeType();
        const auto languageId=type==0 ? 0xd6d9 : type==1 ? 0xd6da : 0;
        const auto* format=language.GetString(languageId);
        Require(format,"Track unlock description is missing from the actual language resource");
        text=FormatLocalizedText(format,{static_cast<std::int32_t>(event->number)},capacity);
    } else text="This track should be unlocked";
    // PAL sprintf ignores the incoming capacity. The real caller owns256
    // bytes; keep that valid-asset contract with a checked native C string.
    Require(text.size()<capacity,"Track unlock description exceeds the caller's retained buffer");
    std::memcpy(output,text.c_str(),text.size()+1);
}

} // namespace hp2

#include "Race.hpp"
#include "Player.hpp"
#include "CarDamage.hpp"
namespace hp2 {
namespace {
bool CompletedTask(const TaskCarReward& task,const FrontendPlayerManager& frontend,
    const RaceFrame& race,const RaceCarEntry& entry,const RaceParameters& parameters)
{
    const auto* player=raceCarEntryPlayer(&entry,&race);
    Require(player && entry.car && entry.car->Movement() && entry.car->Movement()->bindings.damage,
        "Task completion has no actual Player/CarDamage owners");
    (void)bodyDamageVisibleTotal(&entry.car->Movement()->bindings.damage->body);
    if(task.undamaged!=-1.0f && !player->undamagedRace)return false;
    if(task.players!=-1 && task.players!=parameters.mode)return false;
    if(task.mode!=-1) {
        if(task.mode!=frontend.gameType)return false;
        if(frontend.gameType==4 && frontend.playerCount==1) {
            if(parameters.mode+parameters.opponentCount>=3 || entry.finishPosition!=1)return false;
        }
        if(frontend.gameType==5 && entry.finishPosition!=1)return false;
    }
    if(task.opponents!=-1 && task.opponents!=parameters.opponentCount)return false;
    if(task.laps!=-1 && task.laps!=parameters.totalLaps)return false;
    if(task.difficulty!=-1 && task.difficulty!=parameters.opponentDifficulty)return false;
    if(task.checkpoint!=-1 && task.checkpoint!=entry.furthestCheckpoint)return false;
    if(task.cops!=-1 && task.cops!=(parameters.policeDriverCount>0))return false;
    return true;
}
}

void CheckForTaskAndPointRewards(FrontendPlayerManager& frontend,const RaceFrame& race,
    const RaceParameters& parameters)
{
    Require(race.services && race.services->players,"Reward completion has no actual Player registry");
    auto& transient=frontend.transientStats;
    transient.pointsReward=-1;transient.taskReward=-1;
    const auto& players=*race.services->players;
    if(players.count>=2 || !players.playersByNumber[0])return;
    const auto* player=players.playersByNumber[0];
    Require(player->car && player->car->bindings.owner && player->car->bindings.owner->raceEntry &&
        frontend.players[0],"Reward completion has no actual first racing Player/profile");
    const auto& entry=*player->car->bindings.owner->raceEntry;
    if(frontend.gameType!=2) {
        for(const auto& reward:taskCarRewards) {
            const auto index=static_cast<std::size_t>(reward.index);
            if(CompletedTask(reward,frontend,race,entry,parameters) && !frontend.taskRewardsEarned[index]) {
                frontend.taskRewardsEarned[index]=1;transient.taskReward=reward.id;
            }
        }
    }
    const auto total=frontend.players[0]->totalPoints;
    for(const auto& reward:pointsCarRewards) {
        const auto index=static_cast<std::size_t>(reward.index);
        if(total>=reward.points && !frontend.pointsRewardsEarned[index]) {
            frontend.pointsRewardsEarned[index]=1;transient.pointsReward=reward.index;
        }
    }
}
}

namespace hp2 {
std::int32_t GetCarForPointsReward(std::int32_t index)
{
    if(index<0 || static_cast<std::size_t>(index)>=std::size(pointsCarRewards))return -1;
    return pointsCarRewards[static_cast<std::size_t>(index)].model;
}
std::int32_t GetCarForTaskReward(std::int32_t taskId)
{
    for(const auto& task:taskCarRewards)if(task.id==taskId)return task.model;
    return -1;
}
std::int32_t GetLanguageIDForTaskReward(std::int32_t taskId)
{
    // PAL24AF40 selects task+30, distinct from the car-unlock blurb at+2C.
    for(const auto& task:taskCarRewards)if(task.id==taskId)return task.description;
    return -1;
}
std::int32_t GetGameTypeForTaskReward(std::int32_t taskId)
{
    for(const auto& task:taskCarRewards)if(task.id==taskId)return task.gameType;
    return -1;
}
std::int32_t GetNumPointsRewardsEarned(const FrontendPlayerManager& frontend)
{
    std::int32_t result=0;
    for(const auto& reward:pointsCarRewards)if(frontend.pointsRewardsEarned[static_cast<std::size_t>(reward.index)])++result;
    return result;
}
std::int32_t GetNumCarsUnlocked(const FrontendPlayerManager& frontend,
    const CarInfoResource& cars,const EventTreeResources* events,std::int32_t usage)
{
    std::int32_t count=0;
    for(const auto& car:cars.Cars())
        if(car.bodyCategory==usage && (IsCarUnlocked(frontend,events,car.modelCode,-1) || car.challengeAvailability==0))++count;
    return count;
}
std::int32_t GetNumCarsAvailable(const CarInfoResource& cars,std::int32_t usage)
{
    std::int32_t count=0;for(const auto& car:cars.Cars())if(car.bodyCategory==usage)++count;return count;
}
std::int32_t GetNumCopCarsUnlocked(const FrontendPlayerManager& frontend,
    const CarInfoResource& cars,const EventTreeResources* events)
{
    std::int32_t count=0;
    for(const auto& car:cars.Cars())if(car.bodyCategory==1 && (car.pursuitAvailability==0 ||
        (car.pursuitAvailability==1 && IsCarUnlocked(frontend,events,car.modelCode,7))))++count;
    return count;
}
std::int32_t GetNumCopCarsAvailable(const CarInfoResource& cars)
{
    std::int32_t count=0;for(const auto& car:cars.Cars())if(car.bodyCategory==1)++count;return count;
}
std::string FormatRewardPoints(std::int32_t points,const LanguageManager& language)
{
    // PAL24B468; the separator is language-specific, not a locale from the
    // host machine. Signed divisions/remainders preserve the source values.
    const char* separator=language.currentLanguage==1?" ":language.currentLanguage==2?".":",";
    const auto millions=points/1000000,thousands=(points/1000)%1000,remainder=points%1000;
    char text[64];
    if(millions)std::snprintf(text,sizeof(text),"%d%s%03d%s%03d",millions,separator,thousands,separator,remainder);
    else if(thousands)std::snprintf(text,sizeof(text),"%d%s%03d",thousands,separator,remainder);
    else std::snprintf(text,sizeof(text),"%d",remainder);
    return text;
}
}

namespace hp2 {
// Full final PAL24B548. The actual task/event/points tables above are shared
// with availability and completion; no second reward catalogue is created.
void FormatUnlockBlurbForCar(char* output,std::size_t capacity,std::int32_t model,
    std::int32_t gameType,std::int32_t secondPlayer,const EventTreeResources& events,
    const LanguageManager& language)
{
    Require(output && capacity,"Car unlock description has no retained output buffer");
    std::string text;
    if (const auto* task=FindTaskCarReward(model,gameType))
        text=language.GetString(task->language);
    else if (gameType==7 && !secondPlayer)
        text="Be A cop Player 1 -- should not see this message";
    else if (IsCarAnEventReward(events,model)) {
        const auto* event=FindEventThatCarIsARewardFor(events,model);
        Require(event && event->tree,"Car reward has no actual awarding event/tree");
        const auto type=event->tree->TreeType();
        const auto* format=type==0 ? language.GetString(0xc030) : type==1 ?
            language.GetString(0xc031) : "Unknown event tree type";
        text=FormatLocalizedText(format,{static_cast<std::int32_t>(event->number)},capacity);
    } else if (const auto* points=FindPointsCarReward(model)) {
        const auto formatted=FormatRewardPoints(points->points,language);
        text=FormatLocalizedText(language.GetString(0xbfb8),{formatted.c_str()},capacity);
    } else text="This car should be unlocked";
    Require(text.size()<capacity,"Car unlock description exceeds its actual retained buffer");
    std::memcpy(output,text.c_str(),text.size()+1);
}
} // namespace hp2

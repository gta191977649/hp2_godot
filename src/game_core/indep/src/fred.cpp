#include "bware/inc/bChunk.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/Fred.hpp"
#include "Language.hpp"
#include "indep/inc/Fred.hpp"
#include "bware/inc/bWare.hpp"
#include "Fred.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/EventTree.hpp"
#include "indep/inc/DemoModeReplay.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Fred.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace hp2 {

namespace {
// The PC extension adds actual traffic records to the same profile-owned
// selection table. Their physics, model, skin and five-column pace class
// remain those of the selected traffic vehicle; no racer is substituted.
void MakeNativeTrafficCarsAvailable(FrontendPlayerInfo& player)
{
    if(!applicationNativeGameplay.trafficVehicles)return;
    Require(player.cars,"Traffic selection has no canonical car catalogue");
    for(std::int32_t model=0;model<64;++model) {
        if(!player.cars->IsAvailable(static_cast<std::size_t>(model)))continue;
        const auto& car=player.cars->Car(static_cast<std::size_t>(model));
        if(car.bodyCategory!=2)continue;
        Require(car.routeVehicleClass>=0 && car.routeVehicleClass<5,
            "Traffic pace class exceeds the authored five-column table");
        player.SetCarAvailability(0,model,-1,false);
    }
}

bool HasNativeCockpitResources(const CarDefinition& car)
{
    if(!car.supportsDashboardTextures || car.dashboardGeometryPath.empty())return false;
    const auto& catalogue=car.cameraCatalogue;
    for(std::int32_t index=0;index<catalogue.viewCount[0];++index)
        if(catalogue.views[0][index].kind==1)return true;
    return false;
}
}

// Complete PAL187518..187560. The original scans both global Player slots,
// independently of the active count. A present unfinished Player marks the
// profile's abort cell before SimulateRaceFinish writes the remaining rows.
void FrontendPlayerManager::CheckForAbortion(const CarFramePlayerRegistry& registry)
{
    raceAborted=0;
    for (std::size_t slot=0;slot<2;++slot) {
        const auto* player=registry.players[slot];
        if (player && !player->drivingLocked) raceAborted=1;
    }
}

// Complete final PAL183D80..183EC0, individually matched to fred.cpp.
void FrontendPlayerManager::MakeEventCarsAvailable(const EventTree& tree,const EventInfo& event)
{
    Require(event.tree==&tree && players[0] && players[1],
        "Campaign availability has no actual event tree and two profile owners");
    players[0]->FlushAvailableCars();players[1]->FlushAvailableCars();
    for (std::size_t index=0;index<event.carIndices.size();++index) {
        const auto* car=event.GetCar(static_cast<std::int32_t>(index));
        Require(car,"Campaign availability has an invalid authored car record");
        const auto packed=static_cast<std::uint32_t>(*car);
        const auto model=static_cast<std::int32_t>((packed>>16)&255u);
        auto skin=static_cast<std::int32_t>((packed>>24)&15u);
        if (!skin)skin=-1;
        const bool dashboard=((packed>>30)&1u)!=0;
        for (auto* player:players)player->SetCarAvailability(0,model,skin,dashboard);
    }
    // The PC traffic option belongs to unrestricted race setup. Campaign
    // events retain their authored car/skin whitelist, including one-car
    // events. A previously saved traffic selection must not leak into this list.
    for(auto* profile:players) {
        const auto* selected=profile->GetSelectedRideInfo(gameType);
        const auto model=selected->appearance.modelCode;
        if(model<0 || model>=64 || !profile->cars ||
           profile->cars->Car(static_cast<std::size_t>(model)).bodyCategory!=2 ||
           profile->GetCarStatus(model)!=3)continue;
        const auto* first=event.GetCar(0);
        Require(first,"Campaign traffic recovery has no authored allowed car");
        const auto packed=static_cast<std::uint32_t>(*first);
        const auto allowed=static_cast<std::int32_t>((packed>>16)&255u);
        const auto& available=profile->availability[static_cast<std::size_t>(allowed)];
        Require(available.skinCount>0,"Campaign allowed car has no authored skin");
        profile->AssignCarOfType(gameType,allowed,available.skinNumbers[0],((packed>>30)&1u)!=0);
    }
}

void FrontendPlayerManager::MakeChallengeCarsAvailable(const CarInfoResource& cars,
    const EventTreeResources& events,const std::int32_t& demoDisc)
{
    Require(players[0] && players[1] && players[0]->cars==&cars && players[1]->cars==&cars,
        "Challenge availability has no two actual profiles/canonical car catalogue");
    if (gameType==7) {
        for (std::int32_t model=0;model<64;++model) {
            if (!cars.IsAvailable(static_cast<std::size_t>(model)))continue;
            const auto& car=cars.Car(static_cast<std::size_t>(model));
            const auto status=car.bodyCategory==1 ?
                (IsCarUnlocked(*this,&events,model,gameType) ? 0 : car.pursuitAvailability) : 3;
            players[0]->SetCarAvailability(status,model,-1,false);
        }
        for (std::int32_t model=0;model<64;++model) {
            if (!cars.IsAvailable(static_cast<std::size_t>(model)))continue;
            const auto& car=cars.Car(static_cast<std::size_t>(model));
            std::int32_t status=3;
            if (car.bodyCategory!=1) {
                if (IsCarUnlocked(*this,&events,model,3))status=0;
                else status=demoDisc && car.challengeAvailability!=3 ? 0 : car.challengeAvailability;
            }
            players[1]->SetCarAvailability(status,model,-1,false);
        }
        // In Be the Cop the first slot must remain a police driver. The
        // second slot is the human racer and can use the PC traffic option.
        MakeNativeTrafficCarsAvailable(*players[1]);
        return;
    }
    for (auto* player:players) {
        for (std::int32_t model=0;model<64;++model) {
            if (!cars.IsAvailable(static_cast<std::size_t>(model)))continue;
            const auto& car=cars.Car(static_cast<std::size_t>(model));
            std::int32_t status;
            if (IsCarUnlocked(*this,&events,model,gameType))status=0;
            else if (demoDisc && car.challengeAvailability!=3)status=car.bodyCategory==1 ? 3 : 0;
            else status=car.challengeAvailability;
            player->SetCarAvailability(status,model,-1,false);
        }
        MakeNativeTrafficCarsAvailable(*player);
    }
}

void FrontendPlayerManager::StartDemoMode(std::int32_t track,const FrontendRaceSetupBindings& bindings)
{
    Require(bindings.parameters && bindings.applicationOptions,
        "Demo race has no actual race parameters/launch configuration");
    auto& race=*bindings.parameters;
    race.selectedScenario=track;
    race.chopperEnabled=1;
    gameType=1;
    race.authoredTrackOptionSecond=3;
    race.reverse=bindings.applicationOptions->demoTrackDirection;
    race.trafficDensity=1.0f;
    race.opposingTrafficProbability=0.5f;
    race.policeDriverCount=4;
    race.authoredTrackOptionFirst=0;
    race.eventGame=0;
    race.raceKind=1;
    race.fastSpeedTrapSpawns=1;
    race.mode=0;
    race.totalLaps=0;
    race.opponentCount=4;
    race.pulloversEnabled=1;
    race.roadBlocksEnabled=1;
    race.authoredPursuitOption268=1;
    race.authoredPursuitOption26C=1;
    race.initialRoadBlockRequests=0;
    race.initialChopperRequests=0;
    pendingState=0;
    StartRace(bindings);
}

void FrontendPlayerManager::StartDemoDiscGame(const FrontendRaceSetupBindings& bindings)
{
    // Complete PAL184DE0. Every store below belongs to the real race bank;
    // the selected car is published to the actual first profile before the
    // existing full StartRace dependency consumes these same owners.
    Require(bindings.parameters && bindings.cars && bindings.language && bindings.behavior &&
        bindings.runtimeSettings && bindings.parameters->runtimeSettings==bindings.runtimeSettings && players[0],
        "Demo-disc race lacks its actual race/profile/car/settings owners");
    auto& race=*bindings.parameters;
    race.selectedScenario=12;
    race.authoredTrackOptionFirst=0;race.authoredTrackOptionSecond=3;race.reverse=0;
    race.trafficDensity=1.0f;race.opposingTrafficProbability=0.5f;
    race.raceKind=1;race.totalLaps=2;race.mode=1;race.opponentCount=3;
    race.playerStartingPositions[0]=4;race.playerStartingPositions[1]=5;
    race.policeDriverCount=4;
    race.pulloversEnabled=1;race.roadBlocksEnabled=1;
    race.authoredPursuitOption268=1;race.authoredPursuitOption26C=1;
    race.chopperEnabled=1;race.specialPursuit=0;race.eventGame=0;
    race.authoredRaceOption254=1;race.authoredRaceOption40=0;
    race.authoredRaceOption44=1;race.authoredRaceOption48=1;
    race.opponentDifficulty=1;race.pursuitDifficulty=1;
    bindings.runtimeSettings->players[0].authoredOption=1;
    bindings.runtimeSettings->players[1].authoredOption=1;
    const auto model=vectorDefaultRandomBounded(2) ? 45 : 21;
    const auto skin=bindings.cars->RandomSkin(static_cast<std::size_t>(model),bindings.language->currentLanguage!=0);
    players[0]->SetCarAvailability(0,model,skin,true);
    players[0]->SetDefaultCarType(model,static_cast<std::uint32_t>(skin),true);
    const auto vehicleClass=bindings.cars->Car(static_cast<std::size_t>(model)).routeVehicleClass;
    Require(vehicleClass>=0 && vehicleClass<5,"Demo-disc car class exceeds the actual behavior table");
    race.playerPaceContribution[0]=bindings.behavior->playerPaceContributionByDifficulty[1][vehicleClass];
    StartRace(bindings);
}

const char* FrontendPlayerManager::RecordPlayerName(std::int32_t index) const
{
    // Read-only native access to the actual saved name bank. The source
    // permits negative address arithmetic; valid saved records use0..127.
    Require(index>=0 && static_cast<std::size_t>(index)<recordPlayerNames.size(),
        "Saved record refers to a name outside the live profile bank");
    const auto& name=recordPlayerNames[static_cast<std::size_t>(index)];
    Require(std::memchr(name.data(),0,name.size()),"Saved record name is not terminated");
    return name.data();
}

std::int32_t frontendMaximumOpponents=7;

void FrontendPlayerManager::BindEventProgress(const EventTreeResources& trees)
{
    if (boundEventResources==&trees)return;
    // The final34A05 publication binds the two authored trees to the two
    // banks inside this same FRED profile. Resource traversal order is kept,
    // so the first event32 publication supplies the shared state reference.
    for (std::size_t index=0;index<trees.TreeCount();++index) {
        auto& tree=trees.TreeAt(index);
        const auto kind=tree.TreeType();
        Require(kind==0 || kind==1,"Event tree has no source profile bank");
        tree.CalcStatePointers(eventProgress,static_cast<std::size_t>(kind),4);
    }
    boundEventResources=&trees;
}

void RandomizeList(std::vector<FrontendRandomlyPickableTrack>& tracks)
{
    const auto count=tracks.size();
    Require(count<=UINT32_MAX,"Challenge track list exceeds the source random range");
    // The source swaps each position with a draw over the FULL count; it
    // deliberately does not reduce the random range after each position.
    for (std::size_t index=0;index<count;++index) {
        const auto other=vectorDefaultRandomBounded(static_cast<std::uint32_t>(count));
        std::swap(tracks[index],tracks[other]);
    }
}

namespace {
std::uint32_t EncodeChallengeTrack(std::uint32_t previous,const TrackInfo& info,
    std::int32_t direction,std::int32_t time,std::int32_t laps,const RaceParameters& race)
{
    auto word=(previous&0xffffff00u)|static_cast<std::uint8_t>(info.number);
    word=(word&0xfffff3ffu)|((static_cast<std::uint32_t>(race.authoredTrackOptionFirst)&3u)<<10);
    word=(word&0xffffcfffu)|((static_cast<std::uint32_t>(time)&3u)<<12);
    word=(word&0xfffffdffu)|((static_cast<std::uint32_t>(direction)&1u)<<9);
    const auto traffic=static_cast<std::uint32_t>(convertFloatToInt32(race.trafficDensity*100.0f));
    word=(word&0xfe03ffffu)|((traffic&127u)<<18);
    const auto opposing=static_cast<std::uint32_t>(convertFloatToInt32(race.opposingTrafficProbability*100.0f));
    word=(word&0x01ffffffu)|(opposing<<25);
    return (word&0xfffc3fffu)|((static_cast<std::uint32_t>(laps)&15u)<<14);
}
}

void FrontendPlayerManager::PickRandomTracksForChallengeKnockout(const FrontendRaceSetupBindings& bindings)
{
    Require(bindings.parameters && bindings.tracks && bindings.events && bindings.currentEvent,
        "Challenge tracks have no canonical race/track/event owners");
    const auto& race=*bindings.parameters;
    auto& current=*bindings.currentEvent;
    std::vector<FrontendRandomlyPickableTrack> choices;
    const auto& records=bindings.tracks->Records();
    for (const auto& info:records) {
        if (!info.name[0])continue;
        if (IsTrackUnlocked(*bindings.events,info.baseNumber,0))choices.push_back({&info,0});
        if (IsTrackUnlocked(*bindings.events,info.baseNumber,1))choices.push_back({&info,1});
    }
    RandomizeList(choices);
    std::size_t selected=0;
    for (;selected<choices.size();++selected)
        if (choices[selected].info->number==race.selectedScenario && choices[selected].direction==race.reverse)break;
    if (selected<choices.size()) {
        const auto entry=choices[selected];
        choices.erase(choices.begin()+static_cast<std::ptrdiff_t>(selected));
        choices.insert(choices.begin(),entry);
    } else {
        const auto* info=bindings.tracks->Find(race.selectedScenario);
        Require(info,"Selected challenge track has no actual GLOBALB record");
        choices.insert(choices.begin(),{info,race.reverse});
    }
    Require(current.tracks.size()<=choices.size(),"Challenge requests more tracks than the real unlocked list");
    for (std::size_t index=0;index<current.tracks.size();++index) {
        const auto& pick=choices[index];
        auto& track=current.tracks[index];
        track.configuration=EncodeChallengeTrack(track.configuration,*pick.info,pick.direction,
            pick.info->timeOfDay,pick.info->routeKind ? 1 : 2,race);
    }
}

void FrontendPlayerManager::SetupFakeRaceEventForChallengeKnockout(const FrontendRaceSetupBindings& bindings)
{
    Require(bindings.parameters && bindings.currentEvent && bindings.tracks,
        "Challenge knockout has no actual setup owners");
    auto& current=*bindings.currentEvent;
    const auto& race=*bindings.parameters;
    current.state=&current.priorState;
    // Actual EventKoState::Reset clears only the first two result words.
    current.priorState.lastFinishPositions=0;current.priorState.ranks=0;
    current.name="Challenge Race Knockout";
    const auto packedCount=static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(static_cast<std::uint16_t>(race.opponentCount))+
        static_cast<std::uint32_t>(playerCount)-1u);
    std::int16_t count;std::memcpy(&count,&packedCount,sizeof(count));
    Require(count>=0 && count<=8,"Challenge track/opponent count exceeds its source capacity");
    current.objective={1,0,3};current.kind=3;
    current.playerCount=static_cast<std::int16_t>(playerCount);
    current.opponents.resize(static_cast<std::size_t>(count));
    current.tracks.resize(static_cast<std::size_t>(count));
    current.number=0;current.category=0;current.cars.clear();current.rewards.clear();
    if (knockoutTrackPolicy==0)PickRandomTracksForChallengeKnockout(bindings);
    else if (knockoutTrackPolicy==1 && count>0) {
        const auto* info=bindings.tracks->Find(race.selectedScenario);
        Require(info,"Challenge knockout has no actual selected track");
        for (auto& track:current.tracks)
            track.configuration=EncodeChallengeTrack(track.configuration,*info,race.reverse,
                race.authoredTrackOptionSecond,race.totalLaps,race);
    }
    current.ResetResults();
}

void FrontendPlayerManager::MaybeSetSelectedCarForMultiTrackEvent(const FrontendRaceSetupBindings& bindings)
{
    if (!IsEventGame())return;
    Require(players[0] && bindings.events && bindings.demoDisc,
        "Multitrack selection has no canonical profile/event-tree owners");
    const auto& ride=players[0]->GetSelectedRideInfo(gameType)->appearance;
    const auto model=ride.modelCode;
    const auto skin=static_cast<std::int32_t>(ride.skinNumber);
    const auto dashboard=ride.dashboardTextures!=0;
    auto* tree=GetEventTree(*bindings.events,*bindings.demoDisc);
    Require(tree,"Multitrack selection has no authored event tree");
    auto* event=tree->GetEventNumber(eventSelection.GetSelectedEvent(tree->TreeType()));
    Require(event,"Multitrack selection has no stored event in its authored tree");
    event->SetSelectedCar(model,skin,dashboard);
}

std::array<std::int8_t,9> highScoreLapCounts{{1,2,3,4,5,6,7,8,10}};

HighScoreTable::HighScoreTable(TrackInfoResources& tracks,const FrontendPlayerManager& frontend)
    : tracks_(tracks)
{
    lapView_={&tracks_.Catalog(),lap_.data(),lap_.size(),&frontend};
    InitStatsTables();
}

void HighScoreTable::InitStatsTables()
{
    auto& tracks=tracks_.MutableRecords();
    std::int32_t next=0;
    for (auto& track:tracks) {
        if (!track.name[0] || !track.courseAvailable || track.number!=track.baseNumber)continue;
        track.courseIndex=static_cast<std::int16_t>(next<16 ? next : 0);
        ++next;
    }
    for (auto& track:tracks) {
        if (!track.name[0] || !track.courseAvailable || track.number==track.baseNumber)continue;
        const auto* base=tracks_.Find(track.baseNumber);
        Require(base,"High-score track variant has no canonical base track");
        track.courseIndex=base->courseIndex;
    }
    total_.fill({0,0,0});lap_.fill({0,0,0});
    for (auto& record:total_)record.ticks=std::numeric_limits<std::int32_t>::max();
    for (auto& record:lap_)record.ticks=std::numeric_limits<std::int32_t>::max();
}

std::int32_t HighScoreTable::GetUniqueLapIndex(std::int32_t laps) const
{
    const auto choices=highScoreLapCounts;
    for (std::int32_t index=0;index<9;++index)
        if (choices[static_cast<std::size_t>(index)]==laps)return index;
    return 0;
}

std::int32_t HighScoreTable::GetTotalTimesIndex(std::int32_t scenario,std::int32_t vehicleClass,
    std::int32_t reverse,std::int32_t laps) const
{
    const auto course=raceProfileCourseIndex(&lapView_,scenario);
    const auto lap=GetUniqueLapIndex(laps);
    return course*450+lap*5+(reverse!=0 ? 45 : 0)+vehicleClass*90;
}

RaceProfileBestTime* FrontendPlayerManager::GetBestTotalTimesTable(std::int32_t scenario,
    std::int32_t vehicleClass,std::int32_t reverse,std::int32_t laps) const
{
    Require(highScores,"Frontend total-time lookup has no live high-score owner");
    const auto index=highScores->GetTotalTimesIndex(scenario,vehicleClass,reverse,laps);
    auto& bank=highScores->TotalRecords();
    Require(index>=0 && static_cast<std::size_t>(index)<bank.size(),"Total-time index is outside the actual profile bank");
    return &bank[static_cast<std::size_t>(index)];
}

void FrontendPlayerManager::SetRecordTimesToBeat(std::int32_t seat,
    const FrontendRaceSetupBindings& bindings)
{
    Require(bindings.parameters && bindings.cars && bindings.runtimeSettings && highScores &&
        seat>=0 && seat<2 && players[static_cast<std::size_t>(seat)],
        "Record-time setup has no canonical profile/runtime bindings");
    const auto& race=*bindings.parameters;
    const auto* selected=players[static_cast<std::size_t>(seat)]->GetSelectedRideInfo(gameType);
    const auto vehicleClass=bindings.cars->Car(static_cast<std::size_t>(selected->appearance.modelCode)).routeVehicleClass;
    auto& output=bindings.runtimeSettings->players[static_cast<std::size_t>(seat)];
    if (race.totalLaps)output.bestTotalTimeTicks=GetBestTotalTimesTable(
        race.selectedScenario,vehicleClass,race.reverse,race.totalLaps)->ticks;
    const auto* lap=raceProfileFindBestTime(&highScores->LapView(),race.selectedScenario,vehicleClass,race.reverse);
    Require(lap,"Lap-time index is outside the actual profile bank");
    output.bestLapTimeTicks=lap->ticks;
}

EventTree* FrontendPlayerManager::GetEventTree(const EventTreeResources& trees,
    const std::int32_t& demoDisc) const
{
    if (gameType==8)return trees.Find(demoDisc ? "Championship E3" : "Championship");
    if (gameType==9)return trees.Find(demoDisc ? "Hot Pursuit E3" : "Hot Pursuit");
    return nullptr;
}

void FrontendPlayerManager::CalculateStartingPositions(const FrontendRaceSetupBindings& bindings)
{
    Require(bindings.parameters && bindings.cars,"Starting grid has no canonical setup owners");
    auto& race=*bindings.parameters;
    Require(race.driverCount>=0 && race.driverCount<=RACE_DRIVER_LIMIT &&
        (!race.driverCount || race.drivers),"Starting grid has no driver storage");
    if (UseTheRaceEvent()) {
        Require(bindings.currentEvent,"Event starting grid has no selected race event");
        auto& event=*bindings.currentEvent;
        // Exact PAL2022C0 operations, kept within this licensed caller. The
        // beta has no demonstrated standalone counterpart for this helper.
        if ((event.kind==3 || event.kind==4) && event.GetRaceIndex()!=0) {
            if (event.kind==3) {
                for (std::size_t seat=0;seat<2;++seat) {
                    const auto value=static_cast<std::uint8_t>(event.results[seat].resultCode);
                    std::memcpy(&race.playerStartingPositions[seat],&value,1);
                }
                for (std::int32_t index=0;index<8;++index) {
                    auto* driver=raceParametersGetDriverInfoByDriverNumber(&race,event.DriverNumberFromRacerIndex(index));
                    if (driver && !driver->eliminated) {
                        const auto value=static_cast<std::uint8_t>(event.results[static_cast<std::size_t>(index)].resultCode);
                        std::memcpy(&driver->startingPosition,&value,1);
                    }
                }
            } else {
                auto* const* state=std::get_if<EventTournamentState*>(&event.state);
                Require(state && *state,"Tournament grid has no live profile state");
                const auto first=static_cast<std::uint8_t>((*state)->RacerRank(0));
                std::memcpy(&race.playerStartingPositions[0],&first,1);
                for (std::int32_t index=0;index<8;++index) {
                    auto* driver=raceParametersGetDriverInfoByDriverNumber(&race,event.DriverNumberFromRacerIndex(index));
                    if (driver && !driver->eliminated) {
                        const auto value=static_cast<std::uint8_t>((*state)->RacerRank(index));
                        std::memcpy(&driver->startingPosition,&value,1);
                    }
                }
            }
        }
    }
    std::array<RaceDriverInfo*,RACE_DRIVER_LIMIT> unplaced{};
    std::array<float,RACE_DRIVER_LIMIT> scores{};
    std::size_t count=0;
    for (std::int32_t index=0;index<race.driverCount;++index) {
        auto& driver=race.drivers[index];
        if (!driver.eliminated && !driver.startingPosition && driver.driverClass>0 && driver.driverClass<3)
            unplaced[count++]=&driver;
    }
    for (std::size_t index=0;index<count;++index) {
        const auto& car=bindings.cars->Car(static_cast<std::size_t>(unplaced[index]->appearance.modelCode));
        float peak=0.0f;
        for (const float sample:car.engineInfo.torqueCurve)peak=peak>sample ? peak : sample;
        float score=peak/car.rigidBodyInfo.mass;
        score=score*car.driveTrainInfo.gearRatios[2];
        score=score*car.driveTrainInfo.rearFinalDrive;
        scores[index]=score/car.wheels[3].radius;
    }
    for (std::size_t pass=0;pass<count;++pass) {
        for (std::size_t index=0;index+1<count;++index) {
            if (scores[index]<scores[index+1]) {
                std::swap(unplaced[index],unplaced[index+1]);
                std::swap(scores[index],scores[index+1]);
            }
        }
    }
    for (std::size_t index=0;index<count;++index) {
        std::int32_t position=1;
        for (;position<9;++position) {
            std::int32_t other=0;
            for (;other<race.driverCount;++other)
                if (race.drivers[other].startingPosition==position)break;
            if (other==race.driverCount)break;
        }
        unplaced[index]->startingPosition=static_cast<std::int8_t>(position);
    }
}

void FrontendPlayerManager::SetupCarsForRace(const FrontendRaceSetupBindings& bindings)
{
    Require(bindings.parameters && bindings.cars && bindings.language && bindings.tracks &&
        bindings.characters && bindings.behavior && bindings.applicationOptions,
        "Race setup has no canonical frontend/resource bindings");
    auto& race=*bindings.parameters;
    const auto& cars=*bindings.cars;
    const auto& language=*bindings.language;
    const auto& characters=*bindings.characters;
    raceParametersClearEliminatedDrivers(&race);
    vectorRandomSeed(race.eventRandomSeed,&vectorDefaultRandomState);
    std::array<std::int32_t,8> characterChoices{{0,1,2,3,4,5,6,7}};
    const auto humanAndOpponentCount=static_cast<std::uint32_t>(race.mode)+static_cast<std::uint32_t>(race.opponentCount);
    const auto total=static_cast<std::int32_t>(humanAndOpponentCount+static_cast<std::uint32_t>(race.policeDriverCount));
    race.driverCount=0;
    if (total>=9)race.policeDriverCount=humanAndOpponentCount<8u ? 8-static_cast<std::int32_t>(humanAndOpponentCount) : 0;
    Require(race.drivers,"Race setup has no retained driver storage");
    const auto selectedRide=[this](std::int32_t seat)->CarAppearanceSelection& {
        Require(seat>=0 && seat<2 && players[static_cast<std::size_t>(seat)],"Race setup has no live player profile");
        auto& profile=*players[static_cast<std::size_t>(seat)];
        Require(gameType>=0 && static_cast<std::size_t>(gameType)<profile.selectedRides.size(),"Race setup game type is outside the profile");
        return profile.selectedRides[static_cast<std::size_t>(gameType)].appearance;
    };
    const auto eliminate=[this,&bindings,&race](std::int32_t number) {
        if (!UseTheRaceEvent())return false;
        Require(bindings.currentEvent,"Event race setup has no selected current event");
        const auto& event=*bindings.currentEvent;
        if (!event.DriverEliminated(number,race))return false;
        const auto* state=std::get_if<EventKoState*>(&event.state);
        Require(state && *state,"Eliminated event driver has no live knockout state");
        raceParametersEliminateDriver(&race,number,(*state)->GetRank(event.RacerIndexFromDriverNumber(number)));
        return true;
    };
    if (!race.fastSpeedTrapSpawns) {
        const auto requestedPlayers=race.mode;
        Require(requestedPlayers<=2,"Race setup requests more than the two actual player profiles");
        std::int32_t active=0;
        for (std::int32_t seat=0;seat<requestedPlayers;++seat) {
            Require(players[static_cast<std::size_t>(seat)],"Race setup has no player profile");
            std::int8_t preference;
            std::memcpy(&preference,&players[static_cast<std::size_t>(seat)]->settings.authoredTimeDisplayOption,1);
            auto character=PickCharacterNumberThatIsntRacing(characterChoices.data(),8,
                preference,characters);
            if (race.specialPursuit && !seat)character=PickCopCharacterNumber(race,*bindings.tracks,characters);
            auto& ride=selectedRide(seat);ride.reserved=1;
            auto liveRide=ride;
            // PC cockpit admission is a live race choice, not a profile or
            // camera-data rewrite. Load the actual declared dashboard before
            // the existing camera cycle can enter its authored kind-1 view.
            // Released split-screen catalogues have no interior views.
            if(applicationNativeGameplay.cockpitView && race.mode==1 &&
               HasNativeCockpitResources(cars.Car(static_cast<std::size_t>(ride.modelCode))))
                liveRide.dashboardTextures=1;
            RaceDriverInfo driver{};
            InitializeDriverInfo(driver,seat,1,liveRide,character,-2,race.playerStartingPositions[seat],characters);
            AddDriverInfo(race,driver,cars);
            if (!eliminate(seat))++active;
        }
        race.mode=active;
    }
    std::int32_t minimumClass,maximumClass;
    if (race.fastSpeedTrapSpawns)minimumClass=maximumClass=static_cast<std::int32_t>(vectorDefaultRandomBounded(5));
    else if (race.mode==1)minimumClass=maximumClass=cars.Car(static_cast<std::size_t>(selectedRide(0).modelCode)).routeVehicleClass;
    else {
        minimumClass=cars.Car(static_cast<std::size_t>(selectedRide(0).modelCode)).routeVehicleClass;
        maximumClass=cars.Car(static_cast<std::size_t>(selectedRide(1).modelCode)).routeVehicleClass;
        if (maximumClass<minimumClass)std::swap(minimumClass,maximumClass);
    }
    bool triedMinimum=false,triedMaximum=false;
    const auto requestedOpponents=race.opponentCount;
    std::int32_t activeOpponents=0;
    for (std::int32_t opponent=0;opponent<requestedOpponents;++opponent) {
        const auto character=PickCharacterNumberThatIsntRacing(characterChoices.data(),8,-1,characters);
        const auto number=opponent+100;
        CarAppearanceSelection ride{};ride.modelCode=-1;
        std::int32_t behavior=-1;
        if (race.opponentVehicleClass==2) {
            Require(bindings.events && bindings.demoDisc,"Authored opponents have no live event-tree registry/policy");
            auto* tree=GetEventTree(*bindings.events,*bindings.demoDisc);
            Require(tree,"Authored opponents have no selected event tree");
            auto* event=tree->GetEventNumber(selectedEventNumber);
            Require(event,"Authored opponents have no selected event record");
            const auto* packed=event->GetOpponent(opponent);
            Require(packed,"Authored opponent index is outside the event");
            const auto bits=static_cast<std::uint32_t>(*packed);
            ride.modelCode=static_cast<std::int32_t>(bits&255u);
            const auto skin=static_cast<std::uint8_t>(bits>>8);
            std::memcpy(&ride.skinNumber,&skin,1);
            if (ride.skinNumber<0)ride.skinNumber=static_cast<std::int8_t>(PickRandomSkinOfUniqueColour(race,cars,language,ride.modelCode));
            behavior=static_cast<std::int32_t>((bits>>16)&255u);
        } else {
            const auto draw=vectorDefaultRandomFloat(1.0f);
            Require(race.opponentDifficulty>=0 && race.opponentDifficulty<3,"Opponent difficulty is outside the authored three CDFs");
            const auto& chances=bindings.behavior->opponentProfileCumulativeChances[race.policeDriverCount<1 ? 1 : 0][race.opponentDifficulty];
            for (std::int32_t profile=0;profile<10;++profile) {
                if (draw<=chances[profile]) { behavior=profile;break; }
            }
            if (behavior==-1)behavior=0;
            const auto forced=bindings.applicationOptions->forcedOpponentModel;
            if (forced!=-1 || race.opponentVehicleClass==1) {
                ride.modelCode=forced!=-1 ? forced : selectedRide(0).modelCode;
                ride.skinNumber=static_cast<std::int8_t>(PickRandomSkinOfUniqueColour(race,cars,language,ride.modelCode));
            } else {
                std::int32_t innerAttempt=0;
                for (std::int32_t attempt=0;attempt<500;++attempt) {
                    do {
                        std::int32_t vehicleClass;
                        if (!triedMinimum) { vehicleClass=minimumClass;triedMinimum=true; }
                        else if (!triedMaximum) { vehicleClass=maximumClass;triedMaximum=true; }
                        else vehicleClass=minimumClass+static_cast<std::int32_t>(vectorDefaultRandomBounded(
                            static_cast<std::uint32_t>(maximumClass-minimumClass+1)));
                        cars.WeightedPickRide(ride,0,vehicleClass,race,language);
                        if (ride.modelCode==-1)cars.WeightedPickRide(ride,0,-1,race,language);
                        ++innerAttempt;
                    } while (innerAttempt<1001 && AreThereTooManyUniqueCarGeometries(race,ride,cars));
                    bool accepted=true;
                    if (attempt<100 && AreThereAnyCarsOfType(race,ride.modelCode))accepted=false;
                    if (attempt<200 && AreThereAnyCarsOfColour(race,cars,ride.modelCode,ride.skinNumber))accepted=false;
                    if (accepted)break;
                    innerAttempt=0;
                }
            }
        }
        RaceDriverInfo driver{};
        InitializeDriverInfo(driver,number,2,ride,character,behavior,0,characters);
        AddDriverInfo(race,driver,cars);
        if (!eliminate(number))++activeOpponents;
    }
    race.opponentCount=activeOpponents;
    const auto trafficCount=std::fabs(race.trafficDensity)<=0.001f ? 0 : 6;
    for (std::int32_t index=0;index<trafficCount;++index) {
        CarAppearanceSelection ride{};ride.modelCode=-1;
        for (std::int32_t attempt=0;attempt<100;++attempt) {
            cars.WeightedPickRide(ride,2,-1,race,language);ride.skinNumber=-1;
            if (!AreThereAnyCarsOfType(race,ride.modelCode))break;
        }
        RaceDriverInfo driver{};
        InitializeDriverInfo(driver,index+50,3,ride,-1,-1,0,characters);AddDriverInfo(race,driver,cars);
    }
    std::int32_t roadClass=5;
    for (std::int32_t index=0;index<race.driverCount;++index)
        roadClass=std::min(roadClass,static_cast<std::int32_t>(cars.Car(
            static_cast<std::size_t>(race.drivers[index].appearance.modelCode)).routeVehicleClass));
    const auto policeCount=race.policeDriverCount;
    for (std::int32_t index=0;index<policeCount;++index) {
        const auto character=PickCopCharacterNumber(race,*bindings.tracks,characters);
        std::int32_t selected=-1,selectedClass=0;
        for (std::size_t model=0;model<CarInfoResource::CarCount;++model) {
            const auto& car=cars.Car(model);
            if (car.bodyCategory!=1 || car.geometryPath.empty())continue;
            if (selected==-1 || std::abs(roadClass-car.routeVehicleClass)<std::abs(roadClass-selectedClass)) {
                selected=static_cast<std::int32_t>(model);selectedClass=car.routeVehicleClass;
            }
        }
        CarAppearanceSelection ride{};InitializeRideInfo(ride,selected,1,0,0);
        RaceDriverInfo driver{};
        InitializeDriverInfo(driver,index+10,4,ride,character,-1,0,characters);AddDriverInfo(race,driver,cars);
    }
    CalculateStartingPositions(bindings);
}

void FrontendPlayerManager::StartRace(const FrontendRaceSetupBindings& bindings)
{
    Require(bindings.parameters && bindings.currentEvent && bindings.runtimeSettings &&
        bindings.cars && bindings.tracks && bindings.language && bindings.characters &&
        bindings.applicationOptions && bindings.behavior && bindings.events && bindings.demoDisc &&
        bindings.demoReplay && bindings.flow && bindings.parameters->runtimeSettings==bindings.runtimeSettings,
        "StartRace has no single canonical configuration/resource/application owners");
    Require(boundEventResources==bindings.events && players[0] && players[1],
        "StartRace requires both actual profiles and their published event progress");
    auto& race=*bindings.parameters;
    auto& current=*bindings.currentEvent;
    auto& runtime=*bindings.runtimeSettings;
    if(IsEventGame()) {
        // A continuing tournament can enter StartRace without reopening the
        // selector. Reapply the same authored admission before any live
        // driver or progress mutation, so old traffic selections are safe.
        auto* tree=GetEventTree(*bindings.events,*bindings.demoDisc);
        const auto* event=tree ? tree->GetEventNumber(selectedEventNumber) : nullptr;
        Require(event && current.tree==tree && current.number==event->number,
            "Campaign race has no same-owner authored car restrictions");
        MakeEventCarsAvailable(*tree,*event);
    }
    if (gameType==4) {
        current.playerCount=static_cast<std::int16_t>(playerCount);
        SetupFakeRaceEventForChallengeKnockout(bindings);
    }
    if (UseTheRaceEvent()) {
        if (current.GetStatus()!=2)current.ResetState();
    } else current.playerCount=static_cast<std::int16_t>(playerCount);
    if (UseTheRaceEvent()) {
        Require(!current.tracks.empty(),"Selected race event has no authored track choices");
        const auto index=current.GetRaceIndex();
        Require(index>=0,"Race event returned a negative track index");
        current.tracks[static_cast<std::size_t>(index)%current.tracks.size()].SetRace(race,*bindings.tracks);
        if (current.kind==3 && current.GetRaceIndex()!=0) {
            auto* const* progress=std::get_if<EventKoState*>(&current.state);
            Require(progress && *progress,"Continuing knockout has no live progress owner");
            for (std::size_t seat=0;seat<2;++seat)
                race.playerStartingPositions[seat]=static_cast<std::int8_t>(
                    ((*progress)->lastFinishPositions>>(seat*4))&15u);
        }
        if (current.kind==4 && current.GetStatus()==2) {
            auto* const* progress=std::get_if<EventTournamentState*>(&current.state);
            Require(progress && *progress,"Continuing tournament has no live progress owner");
            race.playerStartingPositions[0]=static_cast<std::int8_t>((*progress)->RacerRank(0));
        }
    }
    current.willUnlockBonus=0;
    if (UseTheRaceEvent()) {
        auto* tree=GetEventTree(*bindings.events,*bindings.demoDisc);
        if (tree) {
            const auto* selected=tree->GetEventNumber(selectedEventNumber);
            Require(selected,"StartRace has no selected authored event");
            current.willUnlockBonus=selected->WillSuperBonusEventBeUnlockedIfThisEventIsWon();
        }
    }
    if (IsChallengeGame() || gameType==2) {
        Require(race.mode>=0 && race.mode<=2 && race.opponentDifficulty>=0 && race.opponentDifficulty<3,
            "Player pace setup exceeds its two players/three authored difficulties");
        for (std::int32_t seat=0;seat<race.mode;++seat) {
            const auto* ride=players[static_cast<std::size_t>(seat)]->GetSelectedRideInfo(gameType);
            const auto vehicleClass=bindings.cars->Car(static_cast<std::size_t>(ride->appearance.modelCode)).routeVehicleClass;
            Require(vehicleClass>=0 && vehicleClass<5,"Player pace class exceeds its five authored columns");
            race.playerPaceContribution[seat]=bindings.behavior->playerPaceContributionByDifficulty[
                race.opponentDifficulty][vehicleClass];
        }
    } else if (!IsEventGame() && gameType==1) {
        race.playerPaceContribution[0]=1.0f;race.playerPaceContribution[1]=1.0f;
    }
    if (frontendMaximumOpponents<race.opponentCount)race.opponentCount=frontendMaximumOpponents;
    const auto total=static_cast<std::uint32_t>(race.opponentCount)+static_cast<std::uint32_t>(race.mode);
    std::int32_t signedTotal;std::memcpy(&signedTotal,&total,sizeof(signedTotal));
    if (signedTotal>=9)race.opponentCount=8-race.mode;
    if (!race.pointToPoint) {
        const auto* track=bindings.tracks->Find(race.selectedScenario);
        Require(track,"StartRace has no actual selected track record");
        if (track->routeKind)race.pointToPoint=1;
    }
    SetupCarsForRace(bindings);
    runtime.display=displaySettings;
    runtime.players[0]=players[0]->settings;
    runtime.players[1]=players[1]->settings;
    if (IsEventGame()) {
        const auto* first=raceParametersGetDriverInfoByDriverNumber(&race,0);
        if (first && first->eliminated)runtime.players[0].initialVehicleMode=3;
    }
    for (std::int32_t seat=0;seat<race.mode;++seat)SetRecordTimesToBeat(seat,bindings);
    // ChooseHudTextures1EE388 is exactly JR RA/NOP: retired, no empty wrapper.
    MaybeSetSelectedCarForMultiTrackEvent(bindings);
    if (bindings.demoReplay->active) {
        bindings.demoReplay->RestoreRaceParametersSnapshot(race,*bindings.applicationOptions,
            bindings.language->currentLanguage,*bindings.characters);
        // Recorded snapshots and BitStream are still borrowed by the next
        // race's ReplayManager. Freeing the resource now removed the very
        // first replay frame before the receiving owner was constructed.
        // Release it after ReplayManager has returned its borrowed nodes.
    }
    race.eventGame=gameType==8 || gameType==9;
    // Apply only to the committed live Quick Game / campaign configuration.
    // Authored event records, Challenge's own setting and recorded Demo
    // snapshots remain authoritative when this explicit PC switch is off.
    if(applicationNativeGameplay.disableCatchUp && !bindings.demoReplay->active &&
       (gameType==2 || race.eventGame))race.relativePaceEnabled=0;
    race.unlockedRewardCarCount=0;
    // Complete PAL-only1FF978 operations within this licensed caller: count
    // actual car rewards which are unlocked across the same retained trees.
    for (std::int32_t model=0;model<64;++model)
        if (IsCarAnEventReward(*bindings.events,model) && EventTreeIsCarUnlocked(*bindings.events,model))
            ++race.unlockedRewardCarCount;
    ChangeGameFlow(*bindings.flow,bindings.flow->currentState==3 ? 2 : 4,0);
    // PAL185528..185558: this is FRED+10728, the race-aborted result cell.
    // FRED+180 (pendingState) remains owned by menu/demo navigation.
    raceAborted=0;
}

void InitializeRideInfo(CarAppearanceSelection& ride,std::int32_t model,
    std::int32_t skin,std::int32_t dashboard,std::int32_t reserved)
{
    ride.reserved=static_cast<std::uint8_t>(reserved);
    ride.modelCode=model;
    const auto skinByte=static_cast<std::uint8_t>(skin);
    const auto dashboardByte=static_cast<std::uint8_t>(dashboard);
    std::memcpy(&ride.skinNumber,&skinByte,1);
    std::memcpy(&ride.dashboardTextures,&dashboardByte,1);
    ride.variant=0;
}

std::int32_t PickCharacterNumberThatIsntRacing(std::int32_t* numbers,
    std::int32_t count,std::int32_t preferredAppearanceGroup,
    const CarDriverAppearanceCatalogue& characters)
{
    std::int32_t selected=0;
    for (std::int32_t index=0;index<count;++index) {
        if (numbers[index]==-1)continue;
        if (preferredAppearanceGroup!=-1) {
            const auto* character=carDriverCharacterAt(&characters,numbers[index]);
            Require(character,"Character choice does not refer to the live character catalogue");
            const bool male=stringCompareInsensitive(character->appearanceName,"MALE")==0;
            if (!((male && preferredAppearanceGroup==0) || (!male && preferredAppearanceGroup==1)))continue;
        }
        selected=numbers[index];numbers[index]=-1;break;
    }
    return selected;
}

std::int32_t PickCopCharacterNumber(const RaceParameters& parameters,
    const TrackInfoResources& tracks,const CarDriverAppearanceCatalogue& characters)
{
    const auto* track=tracks.Find(parameters.selectedScenario);
    Require(track,"Police character selection has no published track record");
    auto key=track->policeCharacterKey;
    if (!key)key=hashString("Cop");
    const auto* character=carDriverCharacterFindKey(&characters,key);
    return character ? character->id : -1;
}

void FrontendPlayerInfo::SetCarAvailability(std::int32_t status,std::int32_t model,
    std::int32_t skin,bool allowDashboard)
{
    Require(model>=0 && static_cast<std::size_t>(model)<availability.size(),"Frontend availability model is out of range");
    auto& record=availability[static_cast<std::size_t>(model)];
    record.status=status;record.allowDashboardTextures=allowDashboard;record.skinCount=0;
    if (skin==-1) {
        Require(cars,"Frontend availability has no canonical car catalogue");
        const auto count=cars->AvailableSkinCount(static_cast<std::size_t>(model));
        const auto& definition=cars->Car(static_cast<std::size_t>(model));
        if (definition.bodyCategory==1)
            Require(language,"Frontend skin enumeration has no live language manager");
        for (std::int32_t index=0;index<count;++index) {
            const auto number=cars->AvailableSkinNumber(static_cast<std::size_t>(model),index,
                language && language->currentLanguage!=0);
            if (record.skinCount<static_cast<std::int32_t>(record.skinNumbers.size()))
                record.skinNumbers[static_cast<std::size_t>(record.skinCount)]=static_cast<std::uint8_t>(number);
            ++record.skinCount;
        }
    } else { record.skinNumbers[0]=static_cast<std::uint8_t>(skin);record.skinCount=1; }
}

void FrontendPlayerInfo::FlushAvailableCars()
{
    for (auto& record:availability) record.status=3;
}

std::int32_t FrontendPlayerInfo::GetCarStatus(std::int32_t model) const
{
    Require(model>=0 && static_cast<std::size_t>(model)<availability.size(),"Frontend availability model is out of range");
    return availability[static_cast<std::size_t>(model)].status;
}

void FrontendPlayerInfo::BuildRideInfo(FrontendRideInfo& destination,std::int32_t model,
    std::uint32_t skin,bool allowDashboard) const
{
    Require(cars && model>=0 && static_cast<std::size_t>(model)<availability.size(),
        "Frontend ride has no canonical vehicle resource");
    const auto& available=availability[static_cast<std::size_t>(model)];
    Require(available.skinCount>=0 && static_cast<std::size_t>(available.skinCount)<=available.skinNumbers.size(),
        "Frontend skin choices exceed the profile's authored slots");
    std::int32_t index=0;
    while (index<available.skinCount && skin!=available.skinNumbers[static_cast<std::size_t>(index)]) ++index;
    if (index==available.skinCount) skin=available.skinNumbers[0];
    bool dashboard=false;
    if (allowDashboard && available.allowDashboardTextures)
        dashboard=cars->Car(static_cast<std::size_t>(model)).supportsDashboardTextures;
    destination.appearance.modelCode=model;
    const auto selected=static_cast<std::uint8_t>(skin);
    std::memcpy(&destination.appearance.skinNumber,&selected,sizeof(selected));
    destination.appearance.dashboardTextures=dashboard ? 1 : 0;
    destination.appearance.reserved=0;destination.appearance.variant=0;
}

void FrontendPlayerInfo::AssignCarOfType(std::int32_t gameType,std::int32_t model,
    std::uint32_t skin,bool allowDashboard)
{
    Require(gameType>=0 && static_cast<std::size_t>(gameType)<selectedRides.size(),"Frontend game type is out of range");
    BuildRideInfo(selectedRides[static_cast<std::size_t>(gameType)],model,skin,allowDashboard);
}

void FrontendPlayerInfo::SetDefaultCarType(std::int32_t model,std::uint32_t skin,bool allowDashboard)
{
    for (std::int32_t gameType=0;gameType<11;++gameType) AssignCarOfType(gameType,model,skin,allowDashboard);
}

const FrontendRideInfo* FrontendPlayerInfo::GetSelectedRideInfo(std::int32_t gameType) const
{
    Require(gameType>=0 && static_cast<std::size_t>(gameType)<selectedRides.size(),"Frontend game type is out of range");
    return &selectedRides[static_cast<std::size_t>(gameType)];
}

std::int32_t FrontendPlayerInfo::GetSelectedCarClass(std::int32_t gameType) const
{
    const auto* ride=GetSelectedRideInfo(gameType);
    Require(cars,"Frontend ride has no canonical car catalogue");
    return cars->Car(static_cast<std::size_t>(ride->appearance.modelCode)).routeVehicleClass;
}

bool FrontendPlayerManager::UseTheRaceEvent() const
{
    return gameType==4 || (gameType>=8 && gameType<10);
}

bool FrontendPlayerManager::IsChallengeGame() const
{
    return gameType>=3 && gameType<8;
}

bool FrontendPlayerManager::IsEventGame() const
{
    return gameType>=8 && gameType<10;
}

bool AreThereAnyCarsOfType(const RaceParameters& race,std::int32_t model)
{
    Require(race.driverCount>=0 && race.driverCount<=RACE_DRIVER_LIMIT &&
        (!race.driverCount || race.drivers),"Race car selection has no live driver array");
    for (std::int32_t index=0;index<race.driverCount;++index) {
        const auto& driver=race.drivers[index];
        if (!driver.eliminated && driver.appearance.modelCode==model) return true;
    }
    return false;
}

bool AreThereAnyCarsOfColour(const RaceParameters& race,const CarInfoResource& cars,
    std::int32_t model,std::int32_t skin)
{
    const auto* requested=cars.FindSkin(model,skin,true);
    Require(requested,"Race colour selection has no loaded skin catalogue");
    const auto mask=requested->selectionMask;
    Require(race.driverCount>=0 && race.driverCount<=RACE_DRIVER_LIMIT &&
        (!race.driverCount || race.drivers),"Race colour selection has no live driver array");
    for (std::int32_t index=0;index<race.driverCount;++index) {
        const auto& driver=race.drivers[index];
        if (driver.eliminated) continue;
        const auto* selected=cars.FindSkin(driver.appearance.modelCode,driver.appearance.skinNumber,true);
        Require(selected,"Selected race driver has no loaded skin catalogue");
        if (selected->selectionMask==mask) return true;
    }
    return false;
}

std::int32_t PickRandomSkinOfUniqueColour(const RaceParameters& race,const CarInfoResource& cars,
    const LanguageManager& language,std::int32_t model)
{
    Require(model>=0 && static_cast<std::size_t>(model)<CarInfoResource::CarCount,
        "Race colour selection model is out of range");
    std::int32_t selected=1;
    for (std::int32_t attempt=0;attempt<100;++attempt) {
        selected=cars.RandomSkin(static_cast<std::size_t>(model),language.currentLanguage!=0);
        if (!AreThereAnyCarsOfColour(race,cars,model,selected)) break;
    }
    return selected;
}

} // namespace hp2

extern "C" std::int32_t raceProfileUsesResults(const RaceProfileTimes* profile)
{
    if (!profile || !profile->frontend) return -1;
    return profile->frontend->UseTheRaceEvent();
}

int32_t raceProfileCourseIndex(const RaceProfileTimes* profile, int32_t scenario)
{
    const TrackScenarioDefinition* definition = trackScenarioFind(profile->catalog, scenario);
    return definition && definition->courseAvailable ? definition->courseIndex : 0;
}

int32_t raceProfileTimeIndex(const RaceProfileTimes* profile, int32_t scenario,
                             int32_t vehicleClass, int32_t reverse)
{
    return raceProfileCourseIndex(profile, scenario) * 50 + vehicleClass * 10 + (reverse != 0) * 5;
}

RaceProfileBestTime* raceProfileFindBestTime(const RaceProfileTimes* profile, int32_t scenario,
                                            int32_t vehicleClass, int32_t reverse)
{
    const int32_t index = raceProfileTimeIndex(profile, scenario, vehicleClass, reverse);
    if (index < 0 || (size_t)index >= profile->bestTimeCount) return NULL;
    return &profile->bestTimes[index];
}



#include "Race.hpp"
#include "Pursuit.hpp"
#include "CarSounds.hpp"
#include "NativeAudioProfileValidation.hpp"
#include "bsound/include/bSound.h"

namespace hp2 {
namespace {
std::int32_t AddProfilePoints(std::int32_t left,std::int32_t right)
{
    const auto bits=static_cast<std::uint32_t>(left)+static_cast<std::uint32_t>(right);
    std::int32_t value;std::memcpy(&value,&bits,sizeof(value));return value;
}
}

std::int32_t FrontendPlayerManager::InsertPlayerName(const char* name)
{
    Require(name,"Profile record insertion has no player name");
    for(std::size_t index=0;index<recordPlayerNames.size();++index) {
        auto& slot=recordPlayerNames[index];
        if(!slot[0]) {
            slot.fill(0);
            for(std::size_t i=0;i+1<slot.size() && name[i];++i)slot[i]=name[i];
            return static_cast<std::int32_t>(index);
        }
        if(std::strcmp(name,slot.data())==0)return static_cast<std::int32_t>(index);
    }
    return -1;
}

std::int32_t HighScoreTable::MaybeRecordHighScore(FrontendPlayerManager& frontend,
    const RaceFrame& race,RaceCarEntry& entry,const RaceParameters& parameters)
{
    auto* player=raceCarEntryPlayer(&entry,&race);
    Require(player && entry.car && entry.car->Movement() &&
        entry.car->Movement()->bindings.info,"High-score writer has no actual Player/CarInfo");
    player->recordLapTime=nullptr;player->recordTotalTime=nullptr;
    const auto vehicleClass=entry.car->Movement()->bindings.info->routeVehicleClass;
    const auto totalIndex=GetTotalTimesIndex(parameters.selectedScenario,vehicleClass,
        parameters.reverse,parameters.totalLaps);
    const auto lapIndex=raceProfileTimeIndex(&lapView_,parameters.selectedScenario,vehicleClass,parameters.reverse);
    Require(totalIndex>=0 && static_cast<std::size_t>(totalIndex)+5<=total_.size() &&
        lapIndex>=0 && static_cast<std::size_t>(lapIndex)+5<=lap_.size(),
        "High-score insertion exceeds the actual five-row profile tables");
    const auto insert=[&](RaceProfileBestTime* table,std::int32_t ticks,RaceProfileBestTime*& inserted) {
        for(std::int32_t rank=0;rank<5;++rank) {
            if(ticks>=table[rank].ticks)continue;
            std::move_backward(table+rank,table+4,table+5);
            const auto name=frontend.InsertPlayerName(player->settings.playerName);
            table[rank]={ticks,static_cast<std::int16_t>(std::max(name,0)),
                static_cast<std::int16_t>(entry.car->Movement()->bindings.info->modelCode)};
            inserted=&table[rank];return rank;
        }
        return -1;
    };
    std::int32_t totalRank=-1;
    if(entry.lap==race.targetLapCount)
        totalRank=insert(total_.data()+totalIndex,entry.finishTicks,player->recordTotalTime);
    (void)insert(lap_.data()+lapIndex,raceCarEntryBestLapTicks(&entry,&race),player->recordLapTime);
    // The released return retains only total-rank's low byte; the computed
    // lap-rank high lane in beta is not returned by final PAL186D60..94.
    return static_cast<std::uint8_t>(totalRank);
}

void FrontendPlayerManager::GetOptionsSetFromRace(const CarFramePlayerRegistry& registry,
    RaceParameters& parameters,const CarSoundBindings& sounds)
{
    Require(parameters.runtimeSettings && sounds.audio && sounds.audio->library,
        "Finishing options has no actual runtime settings/mixer owner");
    for(std::int32_t index=0;index<registry.count;++index) {
        const auto* player=registry.players[index];
        Require(player && player->movement.seat>=0 && player->movement.seat<2,
            "Finishing options has an invalid actual Player number");
        const auto seat=static_cast<std::size_t>(player->movement.seat);
        Require(players[seat],"Finishing options has no profile for the Player");
        players[seat]->settings=player->settings;
        parameters.runtimeSettings->players[seat]=player->settings;
    }
    if(gameType>=3 && gameType<=7)
        savedChallengeOptions[static_cast<std::size_t>(gameType-3)]=parameters.authoredRaceOption40;
    displaySettings=parameters.runtimeSettings->display;
    // GetOptionsFromGame1EEF60(forceMusicList=0) does not rebuild the song
    // catalogue. It captures seven actual mixer values and the audio mode.
    constexpr std::array<std::uint32_t,7> categories{{5,4,0,1,2,3,6}};
    for(std::size_t index=0;index<categories.size();++index)
        musicSettings.savedVolumes[index]=sounds.audio->library->GetCategoryVolume(categories[index]);
    musicSettings.savedAudioMode=worldAudioGetMode();
}

std::int32_t FrontendPlayerManager::FigureOutBonusPoints(const RaceFrame& race,
    const RaceCarEntry& entry,const RaceParameters& parameters) const
{
    if(parameters.specialPursuit)return 0;
    auto* player=raceCarEntryPlayer(&entry,&race);
    if(!player || player->movement.seat!=0)return 0;
    bool eligible=false;
    if(race.entryCount==2)eligible=entry.finishPosition==1;
    else if(race.entryCount==3)eligible=entry.finishPosition>=1 && entry.finishPosition<=2;
    else if(race.entryCount!=1)eligible=true;
    return eligible?carFramePlayerCalcBonusPoints(player,entry.finishPosition):0;
}

void FrontendPlayerManager::AwardPointsToPlayer(const RaceFrame& race,
    const RaceCarEntry& entry,const RaceParameters& parameters)
{
    auto* player=raceCarEntryPlayer(&entry,&race);
    if(!player)return;
    const auto seat=player->movement.seat;
    Require(seat>=0 && seat<2 && players[static_cast<std::size_t>(seat)],
        "Race points have no matching actual profile");
    std::int32_t earned;
    Require(carFramePlayerReadTotalScore(player,&earned)!=0,"Race points have no live Player score owner");
    auto& total=players[static_cast<std::size_t>(seat)]->totalPoints;
    total=AddProfilePoints(total,earned);
    total=AddProfilePoints(total,FigureOutBonusPoints(race,entry,parameters));
    if(parameters.specialPursuit) {
        Require(race.services && race.services->pursuit,
            "Pursuit finish bonus has no actual PursuitManager timer owner");
        const auto remaining=race.services->pursuit->remainingSeconds;
        const auto ticks=static_cast<std::uint32_t>(convertFloatToInt32(remaining))*100u;
        std::int32_t bonus;std::memcpy(&bonus,&ticks,sizeof(bonus));total=AddProfilePoints(total,bonus);
    }
}

void TransientStatsElement::Construct(const RaceCarEntry& entry,const RaceFrame& race,
    const FrontendPlayerManager& frontend,const RaceParameters& parameters)
{
    Require(entry.car && entry.car->Movement(),"Transient result has no live participant Car");
    const auto* player=raceCarEntryPlayer(&entry,&race);
    position=entry.position;driverNumber=entry.car->driverNumber;completionReason=entry.completionReason;
    totalTicks=entry.finishTicks;bestLapTicks=raceCarEntryBestLapTicks(&entry,&race);
    maximumSpeed=entry.maximumSpeed;averageSpeed=entry.averageSpeed;pursuitSeconds=0;
    if(!player) {
        const auto* info=entry.car->Movement()->bindings.info;
        Require(info,"AI result has no actual physics CarInfo");
        maximumSpeed-=maximumSpeed*(vectorDefaultRandomFloat(1.0f)*0.05000000074505806f);
        maximumSpeed=minimumFinite(maximumSpeed,info->forwardSpeedLimit);
    }
    tickets=0;
    if(race.services->pursuit) {
        Require(pursuitManagerGetPulloverCount(race.services->pursuit,entry.car,&tickets)==WORLD_SERVICES_OK,
            "Result tickets have no canonical pursuit participant");
        if(parameters.specialPursuit && player && player->movement.seat==0)
            pursuitSeconds=race.services->pursuit->remainingSeconds;
    }
    if(player) {
        Require(carFramePlayerReadTotalScore(player,&racePoints)!=0,"Transient score has no live Player total");
        const auto seat=player->movement.seat;
        Require(seat>=0 && seat<2 && frontend.players[static_cast<std::size_t>(seat)],
            "Transient result has no matching Player profile");
        profilePoints=frontend.players[static_cast<std::size_t>(seat)]->totalPoints;
        bonusPoints=frontend.FigureOutBonusPoints(race,entry,parameters);
        transmission=player->settings.gearInputMode;
    } else {racePoints=0;profilePoints=0;bonusPoints=0;transmission=-1;}
    lapTicks.fill(0);laps=entry.lap;
    for(std::int32_t index=1;index<=laps && index<11;++index)
        lapTicks[static_cast<std::size_t>(index)]=entry.lapTicks[index];
}

void TransientStats::Construct(const RaceFrame& race,const FrontendPlayerManager& frontend,
    const RaceParameters& parameters)
{
    Require(race.entryCount<=rows.size() && (!race.entryCount || race.entries),
        "Transient statistics exceed the original eight racers");
    count=0;
    for(std::size_t index=0;index<race.entryCount;++index) {
        Require(race.entries[index],"Transient statistics have a missing ordered participant");
        rows[index].Construct(*race.entries[index],race,frontend,parameters);++count;
    }
}
const TransientStatsElement* TransientStats::GetDriver(std::int32_t driver) const
{
    for(std::int32_t index=0;index<count;++index)
        if(rows[static_cast<std::size_t>(index)].driverNumber==driver)return &rows[static_cast<std::size_t>(index)];
    return nullptr;
}

void FrontendPlayerManager::TheRaceIsOver(RaceFrame& race,const FrontendRaceSetupBindings& setup,
    const CarSoundBindings& sound)
{
    Require(race.services && race.services->players && setup.parameters && highScores &&
        race.services->settings==setup.parameters && race.services->currentEvent==setup.currentEvent,
        "Race finish does not reference the live FRED/RaceEvent/parameters owners");
    auto& parameters=*setup.parameters;
    GetOptionsSetFromRace(*race.services->players,parameters,sound);
    std::array<RaceCarEntry*,2> human{};
    for(std::size_t index=0;index<race.entryCount;++index) {
        auto* entry=race.entries[index];Require(entry,"Finish has a missing ordered participant");
        auto* player=raceCarEntryPlayer(entry,&race);
        if(!player)continue;
        if(gameType!=7 && player->movement.seat>=0 && player->movement.seat<2)
            human[static_cast<std::size_t>(player->movement.seat)]=entry;
        if(playerCount==1)AwardPointsToPlayer(race,*entry,parameters);
    }
    if(human[0] && human[1]) {
        if(raceCarEntryBestLapTicks(human[0],&race)<raceCarEntryBestLapTicks(human[1],&race)) {
            highScores->MaybeRecordHighScore(*this,race,*human[0],parameters);
            highScores->MaybeRecordHighScore(*this,race,*human[1],parameters);
        } else {
            highScores->MaybeRecordHighScore(*this,race,*human[1],parameters);
            highScores->MaybeRecordHighScore(*this,race,*human[0],parameters);
        }
    } else if(human[0])highScores->MaybeRecordHighScore(*this,race,*human[0],parameters);
    else if(human[1])highScores->MaybeRecordHighScore(*this,race,*human[1],parameters);
    transientStats.Construct(race,*this,parameters);
    if(UseTheRaceEvent()) {
        Require(setup.currentEvent,"Event finish has no actual RaceEvent");
        setup.currentEvent->RaceIsOver(*this,parameters);
    }
    CheckForTaskAndPointRewards(*this,race,parameters);
}
}

namespace hp2 {
void FrontendPlayerManager::TheRaceWasRestarted(const FrontendRaceSetupBindings& setup)
{
    if(UseTheRaceEvent()) {
        Require(setup.currentEvent && setup.parameters,"Restarted race has no current event/setup");
        setup.currentEvent->RestartRace(*this,*setup.parameters);
    }
}
void FrontendPlayerManager::TheRaceIsLeaving(const FrontendRaceSetupBindings& setup)
{
    Require(setup.flow,"Leaving race has no application flow");
    if(UseTheRaceEvent() && setup.flow->currentState!=5) {
        Require(setup.currentEvent,"Leaving race has no canonical current event");
        setup.currentEvent->MaybeDiscardResult(*this);
    }
}
void FrontendPlayerManager::TheRaceWasQuit(RaceFrame& race,const FrontendRaceSetupBindings& setup,
    const CarSoundBindings& sound)
{
    Require(race.services && race.services->players && setup.parameters,
        "Quit race has no canonical player/settings owners");
    CheckForAbortion(*race.services->players);
    GetOptionsSetFromRace(*race.services->players,*setup.parameters,sound);
    if(UseTheRaceEvent()) {
        Require(setup.currentEvent,"Quit race has no canonical current event");
        if(carFramePlayerAllDrivingLocked(race.services->players)) {
            Require(raceFrameSimulateFinish(&race)==WORLD_SERVICES_OK,"Quit race finish simulation failed");
            Require(raceFrameNotifyRaceOver(&race)==WORLD_SERVICES_OK,"Quit race result publication failed");
        }
        setup.currentEvent->RaceIsOver(*this,*setup.parameters);
        setup.currentEvent->MaybeDiscardResult(*this);
    }
}
}

// Original checksum relocated without changing its modulo32-bit algorithm.


static uint32_t loadWord(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void storeWord(uint8_t* p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

uint32_t saveGameCalculateChecksum(const uint8_t* data)
{
    uint32_t sum = 0;
    for (size_t offset = 0; offset < GAME_SAVE_CHECKSUM_OFFSET; offset += 4)
        sum += loadWord(data + offset);
    return sum;
}

int saveGameIsValid(const uint8_t* data, size_t size)
{
    return data != NULL && size == GAME_SAVE_SIZE &&
           loadWord(data + GAME_SAVE_CHECKSUM_OFFSET) == saveGameCalculateChecksum(data);
}

int saveGameFinalize(uint8_t* data, size_t size)
{
    if (data == NULL || size != GAME_SAVE_SIZE) return 0;
    storeWord(data + GAME_SAVE_CHECKSUM_OFFSET, saveGameCalculateChecksum(data));
    return 1;
}

#include <type_traits>
#include <stdexcept>
#include <charconv>
namespace hp2 {

// PC text persistence: one bounded JSON document, not a dump of an ABI or
// of the old binary transfer image. The logical field walk below is shared
// with the read-only legacy decoder, so gameplay still has one FRED owner.
namespace {
void ProfileCheck(bool condition,const char* description)
{
    // The validated commit path must not allocate just to test an invariant.
    if(!condition)throw std::runtime_error(description);
}
struct ProfileJsonValue {
    enum class Kind {Object,Array,String,Number,Boolean,Null};
    Kind kind{Kind::Null};
    std::string text;
    std::vector<ProfileJsonValue> children;
    std::vector<std::string> keys;
    bool used{};
};
class ProfileJsonParser final {
public:
    static constexpr std::size_t MaximumBytes=4u*1024u*1024u;
    ProfileJsonParser(const void* data,std::size_t bytes):text_(static_cast<const char*>(data)),bytes_(bytes) {
        ProfileCheck(data && bytes && bytes<=MaximumBytes,"Profile JSON must contain 1..4194304 UTF-8 bytes");
        if(bytes>=3 && static_cast<unsigned char>(text_[0])==0xef &&
           static_cast<unsigned char>(text_[1])==0xbb && static_cast<unsigned char>(text_[2])==0xbf)at_=3;
    }
    ProfileJsonValue Read() {
        auto value=Value(0);Space();ProfileCheck(at_==bytes_,"Profile JSON has trailing content");return value;
    }
    static void Utf8(std::string& output,std::uint32_t value) {
        ProfileCheck(value<=0x10ffffu && !(value>=0xd800u && value<=0xdfffu),"Profile JSON has an invalid Unicode scalar");
        if(value<0x80u)output.push_back(static_cast<char>(value));
        else if(value<0x800u) {
            output.push_back(static_cast<char>(0xc0u|(value>>6)));output.push_back(static_cast<char>(0x80u|(value&63u)));
        } else if(value<0x10000u) {
            output.push_back(static_cast<char>(0xe0u|(value>>12)));output.push_back(static_cast<char>(0x80u|((value>>6)&63u)));
            output.push_back(static_cast<char>(0x80u|(value&63u)));
        } else {
            output.push_back(static_cast<char>(0xf0u|(value>>18)));output.push_back(static_cast<char>(0x80u|((value>>12)&63u)));
            output.push_back(static_cast<char>(0x80u|((value>>6)&63u)));output.push_back(static_cast<char>(0x80u|(value&63u)));
        }
    }
    static std::uint32_t Codepoint(const std::string& text,std::size_t& at) {
        ProfileCheck(at<text.size(),"Profile JSON has truncated UTF-8");
        const auto first=static_cast<unsigned char>(text[at++]);if(first<0x80)return first;
        unsigned count=0;std::uint32_t value=0,minimum=0;
        if(first>=0xc2 && first<=0xdf){count=1;value=first&31u;minimum=0x80;}
        else if(first>=0xe0 && first<=0xef){count=2;value=first&15u;minimum=0x800;}
        else if(first>=0xf0 && first<=0xf4){count=3;value=first&7u;minimum=0x10000;}
        else ProfileCheck(false,"Profile JSON has invalid UTF-8");
        for(unsigned i=0;i<count;++i) {
            ProfileCheck(at<text.size(),"Profile JSON has truncated UTF-8");
            const auto byte=static_cast<unsigned char>(text[at++]);
            ProfileCheck((byte&0xc0u)==0x80u,"Profile JSON has invalid UTF-8 continuation");value=(value<<6)|(byte&63u);
        }
        ProfileCheck(value>=minimum && value<=0x10ffffu && !(value>=0xd800u && value<=0xdfffu),
            "Profile JSON has an invalid Unicode scalar");return value;
    }
private:
    const char* text_{};std::size_t bytes_{},at_{},nodes_{};
    void Space(){while(at_<bytes_ && (text_[at_]==' ' || text_[at_]=='\t' || text_[at_]=='\r' || text_[at_]=='\n'))++at_;}
    bool Take(char value){Space();if(at_<bytes_ && text_[at_]==value){++at_;return true;}return false;}
    std::uint32_t Hex4() {
        ProfileCheck(bytes_-at_>=4,"Profile JSON has a truncated Unicode escape");std::uint32_t result=0;
        for(unsigned i=0;i<4;++i) {
            const char c=text_[at_++];unsigned digit=0;
            if(c>='0' && c<='9')digit=static_cast<unsigned>(c-'0');
            else if(c>='a' && c<='f')digit=static_cast<unsigned>(c-'a'+10);
            else if(c>='A' && c<='F')digit=static_cast<unsigned>(c-'A'+10);
            else ProfileCheck(false,"Profile JSON has an invalid Unicode escape");
            result=(result<<4)|digit;
        }return result;
    }
    std::string String() {
        ProfileCheck(Take('"'),"Profile JSON expected a string");std::string result;
        bool ended=false;
        while(at_<bytes_) {
            const auto c=static_cast<unsigned char>(text_[at_++]);
            if(c=='"'){ended=true;break;}
            ProfileCheck(c>=0x20,"Profile JSON contains an unescaped control character");
            if(c=='\\') {
                ProfileCheck(at_<bytes_,"Profile JSON has a truncated escape");const auto escape=text_[at_++];
                switch(escape) {
                case '"':result.push_back('"');break;case '\\':result.push_back('\\');break;case '/':result.push_back('/');break;
                case 'b':result.push_back('\b');break;case 'f':result.push_back('\f');break;case 'n':result.push_back('\n');break;
                case 'r':result.push_back('\r');break;case 't':result.push_back('\t');break;
                case 'u': {
                    auto value=Hex4();
                    if(value>=0xd800u && value<=0xdbffu) {
                        ProfileCheck(bytes_-at_>=2 && text_[at_]=='\\' && text_[at_+1]=='u',"Profile JSON has an unpaired Unicode surrogate");
                        at_+=2;const auto low=Hex4();ProfileCheck(low>=0xdc00u && low<=0xdfffu,"Profile JSON has an invalid Unicode surrogate pair");
                        value=0x10000u+((value-0xd800u)<<10)+(low-0xdc00u);
                    }
                    Utf8(result,value);break;
                }
                default:ProfileCheck(false,"Profile JSON has an unknown escape");
                }
            } else result.push_back(static_cast<char>(c));
            ProfileCheck(result.size()<=4096,"Profile JSON string exceeds the allowed size");
        }
        ProfileCheck(ended,"Profile JSON has an unterminated string");
        for(std::size_t cursor=0;cursor<result.size();)Codepoint(result,cursor);
        return result;
    }
    ProfileJsonValue Value(unsigned depth) {
        ProfileCheck(depth<=20 && ++nodes_<=100000,"Profile JSON exceeds its nesting or value limit");Space();
        ProfileCheck(at_<bytes_,"Profile JSON ends before a required value");ProfileJsonValue value;
        const char c=text_[at_];
        if(c=='{' || c=='[') {
            ++at_;const bool object=c=='{';value.kind=object?ProfileJsonValue::Kind::Object:ProfileJsonValue::Kind::Array;
            const char end=object?'}':']';if(Take(end))return value;
            do {
                ProfileCheck(value.children.size()<(object?64u:8192u),"Profile JSON collection exceeds its allowed size");
                if(object) {
                    auto key=String();ProfileCheck(key.size()<=80,"Profile JSON key exceeds its allowed size");
                    ProfileCheck(std::find(value.keys.begin(),value.keys.end(),key)==value.keys.end(),"Profile JSON duplicates an object key");
                    ProfileCheck(Take(':'),"Profile JSON expected a colon");value.keys.push_back(std::move(key));
                }
                value.children.push_back(Value(depth+1));if(Take(end))return value;
                ProfileCheck(Take(','),"Profile JSON expected a comma or collection end");
            }while(true);
        }
        if(c=='"'){value.kind=ProfileJsonValue::Kind::String;value.text=String();return value;}
        const auto literal=[&](const char* expected,ProfileJsonValue::Kind kind) {
            const auto count=std::strlen(expected);ProfileCheck(bytes_-at_>=count && !std::memcmp(text_+at_,expected,count),"Profile JSON has an invalid literal");
            value.kind=kind;value.text.assign(expected,count);at_+=count;
        };
        if(c=='t'){literal("true",ProfileJsonValue::Kind::Boolean);return value;}
        if(c=='f'){literal("false",ProfileJsonValue::Kind::Boolean);return value;}
        if(c=='n'){literal("null",ProfileJsonValue::Kind::Null);return value;}
        const auto begin=at_;if(c=='-')++at_;
        ProfileCheck(at_<bytes_ && text_[at_]>='0' && text_[at_]<='9',"Profile JSON expected a decimal number");
        if(text_[at_]=='0')++at_;else while(at_<bytes_ && text_[at_]>='0' && text_[at_]<='9')++at_;
        if(at_<bytes_ && text_[at_]=='.') {
            ++at_;const auto fraction=at_;while(at_<bytes_ && text_[at_]>='0' && text_[at_]<='9')++at_;
            ProfileCheck(at_!=fraction,"Profile JSON number has an empty fraction");
        }
        if(at_<bytes_ && (text_[at_]=='e' || text_[at_]=='E')) {
            ++at_;if(at_<bytes_ && (text_[at_]=='+' || text_[at_]=='-'))++at_;
            const auto exponent=at_;while(at_<bytes_ && text_[at_]>='0' && text_[at_]<='9')++at_;
            ProfileCheck(at_!=exponent,"Profile JSON number has an empty exponent");
        }
        ProfileCheck(at_-begin<=64,"Profile JSON number exceeds its allowed size");
        value.kind=ProfileJsonValue::Kind::Number;value.text.assign(text_+begin,at_-begin);return value;
    }
};
class FrontendProfileJson final {
public:
    explicit FrontendProfileJson(bool read):reading_(read) {}
    FrontendProfileJson(const void* data,std::size_t bytes):reading_(true),root_(ProfileJsonParser(data,bytes).Read()) {}
    bool Reading() const noexcept{return reading_;}
    void BeginObject(const char* name){Begin(name,ProfileJsonValue::Kind::Object,0);}
    void BeginArray(const char* name,std::size_t count){Begin(name,ProfileJsonValue::Kind::Array,count);}
    void End() {
        ProfileCheck(!stack_.empty(),"Unbalanced native profile JSON collection");auto& frame=stack_.back();
        if(reading_)for(const auto& child:frame.node->children)
            ProfileCheck(child.used,"Profile JSON contains an unknown or unused field");
        stack_.pop_back();
    }
    template<class T>T Scalar(const char* name,T value) {
        auto& node=Field(name,ProfileJsonValue::Kind::Number);
        if(reading_) {
            T result{};std::from_chars_result parsed{};
            if constexpr(std::is_floating_point_v<T>)parsed=std::from_chars(node.text.data(),node.text.data()+node.text.size(),result,std::chars_format::general);
            else parsed=std::from_chars(node.text.data(),node.text.data()+node.text.size(),result,10);
            ProfileCheck(parsed.ec==std::errc{} && parsed.ptr==node.text.data()+node.text.size(),"Profile JSON number does not fit the named field");
            if constexpr(std::is_floating_point_v<T>)ProfileCheck(std::isfinite(result),"Profile JSON number is not finite");
            return result;
        }
        std::array<char,64> buffer{};std::to_chars_result result{};
        if constexpr(std::is_floating_point_v<T>) {
            ProfileCheck(std::isfinite(value),"Live profile contains a non-finite number");
            result=std::to_chars(buffer.data(),buffer.data()+buffer.size(),value,std::chars_format::general,std::numeric_limits<T>::max_digits10);
        } else result=std::to_chars(buffer.data(),buffer.data()+buffer.size(),value,10);
        ProfileCheck(result.ec==std::errc{},"Profile JSON number formatting failed");node.text.assign(buffer.data(),result.ptr);return value;
    }
    std::string String(const char* name,const std::string& value) {
        auto& node=Field(name,ProfileJsonValue::Kind::String);if(!reading_)node.text=value;return node.text;
    }
    void Header() {
        BeginObject(nullptr);const auto format=String("format","NFSHP2 profile");
        const auto version=Scalar("version",std::uint32_t{1});const auto encoding=String("name_encoding","ISO-8859-1");
        ProfileCheck(format=="NFSHP2 profile" && version==1 && encoding=="ISO-8859-1","Unsupported native text profile format, version or name encoding");
    }
    std::string Finish() {
        ProfileCheck(stack_.empty(),"Profile JSON document was not finished");std::string text;
        text.reserve(600000);Print(root_,text,0);text.push_back('\n');
        ProfileCheck(text.size()<=ProfileJsonParser::MaximumBytes,"Native text profile exceeds its size limit");return text;
    }
private:
    struct Frame {ProfileJsonValue* node;std::size_t next{};};
    bool reading_{};ProfileJsonValue root_{};std::vector<Frame> stack_;
    ProfileJsonValue& Field(const char* name,ProfileJsonValue::Kind kind) {
        ProfileJsonValue* result=nullptr;
        if(stack_.empty()) {ProfileCheck(!root_.used,"Profile JSON has more than one root");result=&root_;}
        else {
            auto& frame=stack_.back();const bool object=frame.node->kind==ProfileJsonValue::Kind::Object;
            ProfileCheck(object==(name!=nullptr),"Profile JSON field has the wrong object/array owner");
            if(reading_) {
                if(object) {
                    const auto found=std::find(frame.node->keys.begin(),frame.node->keys.end(),name);
                    ProfileCheck(found!=frame.node->keys.end(),"Profile JSON omits a required field");
                    result=&frame.node->children[static_cast<std::size_t>(found-frame.node->keys.begin())];
                } else {ProfileCheck(frame.next<frame.node->children.size(),"Profile JSON array is too short");result=&frame.node->children[frame.next++];}
            } else {
                if(object) {
                    ProfileCheck(std::find(frame.node->keys.begin(),frame.node->keys.end(),name)==frame.node->keys.end(),"Native text profile writes a duplicate field");
                    frame.node->keys.emplace_back(name);
                }
                frame.node->children.emplace_back();result=&frame.node->children.back();
            }
        }
        ProfileCheck(!result->used,"Profile JSON field was read more than once");result->used=true;
        if(reading_)ProfileCheck(result->kind==kind,"Profile JSON field has the wrong value type");else result->kind=kind;
        return *result;
    }
    void Begin(const char* name,ProfileJsonValue::Kind kind,std::size_t count) {
        auto& value=Field(name,kind);
        if(reading_ && kind==ProfileJsonValue::Kind::Array)ProfileCheck(value.children.size()==count,"Profile JSON array has the wrong element count");
        stack_.push_back({&value,0});
    }
    static void Quote(const std::string& value,std::string& output) {
        static constexpr char hex[]="0123456789abcdef";output.push_back('"');
        for(const auto byte:value) {
            const auto c=static_cast<unsigned char>(byte);
            if(c=='"' || c=='\\'){output.push_back('\\');output.push_back(byte);}
            else if(c<0x20) {output.append("\\u00");output.push_back(hex[c>>4]);output.push_back(hex[c&15]);}
            else output.push_back(byte);
        }output.push_back('"');
    }
    static void Print(const ProfileJsonValue& value,std::string& output,unsigned depth) {
        using Kind=ProfileJsonValue::Kind;
        if(value.kind==Kind::String){Quote(value.text,output);return;}
        if(value.kind!=Kind::Object && value.kind!=Kind::Array){output+=value.text;return;}
        const bool object=value.kind==Kind::Object;output.push_back(object?'{':'[');
        // Record rows remain one readable line. Larger objects and arrays
        // have stable indentation; no lossy compact/binary representation.
        const bool flat=std::all_of(value.children.begin(),value.children.end(),[](const auto& child){
            return child.kind!=Kind::Array && child.kind!=Kind::Object;});
        for(std::size_t i=0;i<value.children.size();++i) {
            if(i)output.push_back(',');
            if(flat){if(i || object)output.push_back(' ');}else {output.push_back('\n');output.append((depth+1)*2,' ');}
            if(object){Quote(value.keys[i],output);output.append(": ");}
            Print(value.children[i],output,depth+1);
        }
        if(!value.children.empty()) {if(flat){if(object)output.push_back(' ');}else {output.push_back('\n');output.append(depth*2,' ');}}
        output.push_back(object?'}':']');
    }
};
} // namespace

// Binary transfers remain available only for legacy migration and bounded
// regression snapshots. New files use the named UTF-8 JSON representation.
struct FrontendProfileStream {
    enum class Mode {Save,Validate,Commit,ExportText,ImportText,ExportLegacyText};
    Mode mode;
    std::uint8_t* output{};
    const std::uint8_t* input{};
    std::size_t position{20},limit{GAME_SAVE_CHECKSUM_OFFSET};
    FrontendProfileJson* json{};
    FrontendProfileStream* validated{};
    bool checkEventBindings{true};
    std::string* playerName{};
    std::array<std::array<std::unique_ptr<EventStateSnapshot>,256>,2> preparedEvents{};
    bool Reading() const noexcept {return mode!=Mode::Save && mode!=Mode::ExportText;}
    bool Applying() const noexcept {return mode==Mode::Commit;}
    bool WritingBinary() const noexcept {return mode==Mode::Save || mode==Mode::ImportText;}
    void Object(const char* name){if(json)json->BeginObject(name);}
    void Array(const char* name,std::size_t count){if(json)json->BeginArray(name,count);}
    void End(){if(json)json->End();}
    template<class T> T Scalar(const char* name,T& value,bool transportOnly=false) {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
        static_assert(sizeof(T)==1 || sizeof(T)==2 || sizeof(T)==4);
        ProfileCheck(position<=limit && sizeof(T)<=limit-position,"Native profile stream exceeds its declared payload");
        T copy=value;std::uint32_t bits=0;
        if(Reading() && mode!=Mode::ImportText) {
            for(std::size_t i=0;i<sizeof(T);++i)bits|=std::uint32_t(input[position+i])<<(i*8);
            if constexpr(sizeof(T)==1) {const auto narrow=static_cast<std::uint8_t>(bits);std::memcpy(&copy,&narrow,1);}
            else if constexpr(sizeof(T)==2) {const auto narrow=static_cast<std::uint16_t>(bits);std::memcpy(&copy,&narrow,2);}
            else std::memcpy(&copy,&bits,4);
        }
        if(json && !transportOnly)copy=json->Scalar(name,copy);
        if constexpr(std::is_floating_point_v<T>)ProfileCheck(std::isfinite(copy),"Profile has a non-finite scalar");
        if(Applying())value=copy;
        if(WritingBinary()) {
            if constexpr(sizeof(T)==1) {std::uint8_t narrow;std::memcpy(&narrow,&copy,1);bits=narrow;}
            else if constexpr(sizeof(T)==2) {std::uint16_t narrow;std::memcpy(&narrow,&copy,2);bits=narrow;}
            else std::memcpy(&bits,&copy,4);
            for(std::size_t i=0;i<sizeof(T);++i)output[position+i]=static_cast<std::uint8_t>(bits>>(i*8));
        }
        position+=sizeof(T);return copy;
    }
    void Bytes(const char* name,char* values,std::size_t count,bool terminated=false) {
        ProfileCheck(position<=limit && count<=limit-position,"Native profile byte string exceeds its payload");
        // Released names are byte-font strings. ISO-8859-1 gives each source
        // byte one exact Unicode scalar, including escaped embedded NULs.
        // Trailing zero padding is implicit; nonzero tails are not discarded.
        std::array<char,12> decoded{};ProfileCheck(count<=decoded.size(),"Native profile name exceeds its source bank");
        const char* source=(Reading() && mode!=Mode::ImportText)?reinterpret_cast<const char*>(input+position):values;
        if(json) {
            if(json->Reading()) {
                const auto text=json->String(name,{});std::size_t offset=0,used=0;
                while(offset<text.size()) {
                    const auto code=ProfileJsonParser::Codepoint(text,offset);
                    ProfileCheck(code<=255u && used<count,"Saved name is outside the released byte-font alphabet or length");
                    decoded[used++]=static_cast<char>(code);
                }
                source=decoded.data();
            } else {
                auto used=count;while(used && source[used-1]==0)--used;
                std::string text;for(std::size_t i=0;i<used;++i)ProfileJsonParser::Utf8(text,static_cast<unsigned char>(source[i]));
                json->String(name,text);
            }
        }
        if(terminated)ProfileCheck(std::memchr(source,0,count)!=nullptr,"Native saved name has no terminator");
        if(playerName && name && std::strcmp(name,"player_name")==0) {
            *playerName=source;
            playerName=nullptr;
        }
        if(Applying())std::memcpy(values,source,count);
        if(WritingBinary())std::memcpy(output+position,source,count);
        position+=count;
    }
    template<class T,std::size_t N> void Scalars(const char* name,std::array<T,N>& values) {
        Array(name,N);for(auto& value:values)Scalar(nullptr,value);End();
    }
    void Settings(PlayerSettings& p) {
        Object("settings");
        Scalar("alternate_camera",p.alternateCamera);Scalar("split_time_mode",p.splitTimeMode);Scalar("camera_spring",p.cameraSpring);Scalar("lookback",p.lookback);
        Scalar("hud_flags",p.hudFlags);Scalar("hud_hotkey",p.hudHotkey);Scalar("scoring",p.scoring);Scalar("input_profile",p.inputProfile);
        Scalar("authored_option",p.authoredOption);Scalar("initial_vehicle_mode",p.initialVehicleMode);Scalar("gear_input_mode",p.gearInputMode);Scalar("camera_view",p.cameraView);
        Scalar("authored_time_display_option",p.authoredTimeDisplayOption);Bytes("player_name",p.playerName,12,true);
        Scalar("best_total_time_ticks",p.bestTotalTimeTicks);Scalar("best_lap_time_ticks",p.bestLapTimeTicks);End();
    }
    void Ride(CarAppearanceSelection& p) {
        Object(nullptr);const auto model=Scalar("model",p.modelCode);ProfileCheck(model>=-1 && model<64,"Saved car model is outside the original profile table");
        Scalar("skin",p.skinNumber);Scalar("dashboard_textures",p.dashboardTextures);Scalar("variant",p.variant);Scalar("reserved",p.reserved);End();
    }
    void Player(FrontendPlayerInfo& p) {
        Object(nullptr);Array("availability",p.availability.size());
        for(auto& entry:p.availability) {
            Object(nullptr);
            // Released GLOBALB contains status4 for several car variants.
            const auto status=Scalar("status",entry.status);ProfileCheck(status>=0 && status<=4,"Saved car availability status is invalid");
            Scalars("skins",entry.skinNumbers);const auto count=Scalar("skin_count",entry.skinCount);
            ProfileCheck(count>=0 && count<=10,"Saved car skin count exceeds the original bank");
            Scalar("dashboard_textures",entry.allowDashboardTextures);End();
        }
        End();Array("selected_rides",p.selectedRides.size());for(auto& ride:p.selectedRides)Ride(ride.appearance);End();
        Settings(p.settings);Scalar("total_points",p.totalPoints);End();
    }
    void EventValue(EventStateSnapshot& state) {
        std::visit([&](auto& value) {
            using T=std::decay_t<decltype(value)>;
            if constexpr(std::is_same_v<T,EventRankState>)Scalar("rank",value.rank);
            else if constexpr(std::is_same_v<T,EventTimeState>)Scalar("ticks",value.ticks);
            else if constexpr(std::is_same_v<T,EventKoState>) {
                Scalar("ranks",value.ranks);Scalar("last_finish_positions",value.lastFinishPositions);Scalar("selected_model",value.selectedModel);
                Scalar("selected_skin",value.selectedSkin);Scalar("selected_variant",value.selectedVariant);Scalar("random_seed",value.randomSeed);
            } else if constexpr(std::is_same_v<T,EventTournamentState>) {
                Scalars("points",value.points);Scalar("selected_model",value.selectedModel);Scalar("selected_skin",value.selectedSkin);
                Scalar("selected_variant",value.selectedVariant);Scalar("random_seed",value.randomSeed);
            } else ProfileCheck(false,"Native profile has an untyped event state");
        },state);
    }
    static EventStateSnapshot EmptyEvent(std::uint8_t tag) {
        switch(tag) {
        case 1:return EventRankState{};case 2:return EventTimeState{};
        case 3:return EventKoState{};case 4:return EventTournamentState{};
        default:throw std::runtime_error("Saved event type is outside the original typed profile states");
        }
    }
    void Events(EventTreeProgress& progress) {
        Array("event_progress",progress.banks_.size());
        for(std::size_t bankIndex=0;bankIndex<progress.banks_.size();++bankIndex) {
            auto& bank=progress.banks_[bankIndex];Object(nullptr);
            std::uint16_t count=0;if(!Reading())for(const auto& entry:bank)if(entry)++count;
            count=Scalar("count",count);ProfileCheck(count<=bank.size(),"Saved event bank has too many cells");Array("cells",count);
            std::array<bool,256> seen{};
            if(!Reading()) {
                for(std::size_t i=0;i<bank.size();++i)if(bank[i]) {
                    Object(nullptr);auto offset=static_cast<std::uint8_t>(i);auto tag=static_cast<std::uint8_t>(bank[i]->index());
                    Scalar("offset",offset);Scalar("kind",tag);EventValue(*bank[i]);End();
                }
            } else for(std::uint16_t row=0;row<count;++row) {
                Object(nullptr);std::uint8_t offset=0,tag=0;offset=Scalar("offset",offset);tag=Scalar("kind",tag);
                ProfileCheck(!seen[offset],"Saved event bank duplicates one live state owner");seen[offset]=true;
                auto value=EmptyEvent(tag);EventValue(value);End();auto& cell=bank[offset];
                if(checkEventBindings)ProfileCheck(!cell || cell->index()==tag,"Saved event state disagrees with the bound final resource type");
                if(mode==Mode::Validate && !cell)preparedEvents[bankIndex][offset]=std::make_unique<EventStateSnapshot>(value);
                if(Applying()) {
                    if(cell)*cell=std::move(value);
                    else {
                        ProfileCheck(validated && validated->preparedEvents[bankIndex][offset],"Profile commit lacks its prepared event cell");
                        cell=std::move(validated->preparedEvents[bankIndex][offset]);*cell=std::move(value);
                    }
                }
            }
            if(Reading() && checkEventBindings)for(std::size_t i=0;i<bank.size();++i)
                ProfileCheck(!bank[i] || seen[i],"Saved profile omits an event already bound by the final resource");
            End();End();
        }End();
    }
    void Records(HighScoreTable& table) {
        Object("records");
        auto visit=[&](const char* name,auto& entries) {
            Array(name,entries.size());for(auto& record:entries) {
                Object(nullptr);Scalar("ticks",record.ticks);const auto player=Scalar("player_name",record.playerNameIndex);Scalar("model",record.modelCode);
                ProfileCheck(player>=-1 && player<128,"Saved high score refers outside the retained player-name bank");End();
            }End();
        };
        visit("total",table.TotalRecords());visit("lap",table.LapRecords());End();
    }
    void Run(FrontendPlayerManager& f) {
        ProfileCheck(f.players[0] && f.players[1] && f.highScores,"Profile I/O requires the actual two players and high-score owner");
        const auto language=Scalar("language",f.currentLanguage);ProfileCheck(language>=-1 && language<=5,"Saved language is outside the released language set");
        const auto autosave=Scalar("autosave",f.autoSave);ProfileCheck(autosave==0 || autosave==1,"Saved autosave option is invalid");
        const auto type=Scalar("game_type",f.gameType);ProfileCheck(type>=0 && type<11,"Saved game type is out of range");
        const auto players=Scalar("player_count",f.playerCount);ProfileCheck(players>=0 && players<=2,"Saved player count is out of range");
        Scalar("selected_event",f.selectedEventNumber);Object("event_selection");
        Scalar("tree",f.eventSelection.treeType);Scalar("hot_pursuit_event",f.eventSelection.selectedHotPursuitEvent);
        Scalar("championship_event",f.eventSelection.selectedChampionshipEvent);Scalar("last_view_state",f.eventSelection.lastViewState);End();
        Object("track_selection");Scalar("base_track",f.trackSelection.baseTrackNumber);Scalar("laps",f.trackSelection.lapCount);
        Scalar("direction",f.trackSelection.direction);Scalar("time_of_day",f.trackSelection.timeOfDay);End();
        Scalars("display_calibration",f.displayCalibration);auto& d=f.displaySettings;Object("display_settings");
        Scalar("leaderboard_mode",d.leaderboardMode);Scalar("authored_option_0",d.authoredOption0);Scalar("language_dependent_option",d.languageDependentOption);
        Scalar("single_view_track_map",d.singleViewTrackMapMode);Scalar("split_view_track_map",d.splitViewTrackMapMode);Scalar("pursuit_track_map",d.pursuitTrackMapMode);
        Scalar("authored_option_6",d.authoredOption6);Scalar("authored_option_7",d.authoredOption7);End();
        Scalar("knockout_track_policy",f.knockoutTrackPolicy);Scalar("pending_state",f.pendingState);Scalar("record_display_suppressed",f.recordDisplaySuppressed);
        Scalar("cops_enabled",f.copsEnabled);Scalars("task_rewards_earned",f.taskRewardsEarned);Scalars("points_rewards_earned",f.pointsRewardsEarned);
        Scalar("race_aborted",f.raceAborted);Scalars("challenge_damage",f.savedChallengeOptions);
        Array("record_player_names",f.recordPlayerNames.size());for(auto& name:f.recordPlayerNames)Bytes(nullptr,name.data(),name.size(),true);End();
        auto& music=f.musicSettings;FrontendMusicSettings incomingAudio;
        Object("audio");Array("playlist",music.rows.size());
        for(std::size_t index=0;index<music.rows.size();++index) {
            auto& row=music.rows[index];auto& incoming=incomingAudio.rows[index];
            Object(nullptr);incoming.group=Scalar("group",row.group);incoming.track=Scalar("track",row.track);
            incoming.status=Scalar("status",row.status);incoming.genreBit=Scalar("genre_bit",row.genreBit);End();
        }End();
        incomingAudio.count=Scalar("playlist_count",music.count);
        incomingAudio.enabledGenres=Scalar("enabled_genres",music.enabledGenres);
        Array("volumes",music.savedVolumes.size());
        for(std::size_t index=0;index<music.savedVolumes.size();++index)
            incomingAudio.savedVolumes[index]=Scalar(nullptr,music.savedVolumes[index]);
        End();incomingAudio.savedAudioMode=Scalar("mode",music.savedAudioMode);End();
        // Scalar returns the incoming value in the non-mutating validation
        // pass. Checking live music here would incorrectly admit bad files.
        ProfileCheck(nativeAudioProfileValid(incomingAudio,soundJukeboxPlayer.IsCartridgeLoaded()?&soundJukeboxPlayer:nullptr),
            "Saved audio options have invalid volumes, mode or playlist references");
        auto& stats=f.transientStats;Object("race_results");const auto statsCount=Scalar("count",stats.count);
        ProfileCheck(statsCount>=0 && statsCount<=8,"Saved results exceed eight actual racer rows");
        Scalar("points_reward",stats.pointsReward);Scalar("task_reward",stats.taskReward);Array("racers",stats.rows.size());
        for(auto& row:stats.rows) {
            Object(nullptr);Scalar("position",row.position);Scalar("driver",row.driverNumber);Scalar("completion_reason",row.completionReason);
            Scalar("total_ticks",row.totalTicks);Scalar("best_lap_ticks",row.bestLapTicks);Scalar("race_points",row.racePoints);Scalar("profile_points",row.profilePoints);
            Scalar("bonus_points",row.bonusPoints);Scalar("tickets",row.tickets);Scalar("transmission",row.transmission);
            Scalar("maximum_speed",row.maximumSpeed);Scalar("average_speed",row.averageSpeed);Scalar("pursuit_seconds",row.pursuitSeconds);
            Scalars("lap_ticks",row.lapTicks);Scalar("laps",row.laps);End();
        }End();End();
        Array("players",f.players.size());for(auto* player:f.players)Player(*player);End();
        Records(*f.highScores);Events(f.eventProgress);Object("native_display");
        const auto width=Scalar("width",f.nativeDisplay.width),height=Scalar("height",f.nativeDisplay.height);
        ProfileCheck(width>=320 && width<=16384 && height>=200 && height<=16384,"Saved PC resolution is outside supported bounds");
        std::uint8_t fullscreen=f.nativeDisplay.fullscreen,vsync=f.nativeDisplay.vsync;
        fullscreen=Scalar("fullscreen",fullscreen);vsync=Scalar("vsync",vsync);
        ProfileCheck(fullscreen<=1 && vsync<=1,"Saved presentation flags are invalid");
        if(Applying()){f.nativeDisplay.fullscreen=fullscreen!=0;f.nativeDisplay.vsync=vsync!=0;}End();
        constexpr std::uint32_t RaceSetupTag=0x33505552u,JoystickTag=0x33594f4au,MusicNavigationTag=0x3453554Du;
        if(!Reading() || mode==Mode::ImportText) {
            auto tag=RaceSetupTag;Scalar(nullptr,tag,true);RaceSetup(f);
            tag=JoystickTag;Scalar(nullptr,tag,true);Joysticks(f);
            tag=MusicNavigationTag;Scalar(nullptr,tag,true);MusicNavigation(f);
        } else {
            bool raceSeen=false,joystickSeen=false,musicSeen=false;
            while(position<limit) {
                std::uint32_t tag=0;tag=Scalar(nullptr,tag,true);
                if(tag==RaceSetupTag) {ProfileCheck(!raceSeen,"Duplicate native RaceSetup profile extension");raceSeen=true;RaceSetup(f);}
                else if(tag==JoystickTag) {ProfileCheck(!joystickSeen,"Duplicate native joystick profile extension");joystickSeen=true;Joysticks(f);}
                else if(tag==MusicNavigationTag) {ProfileCheck(!musicSeen,"Duplicate native music navigation profile extension");musicSeen=true;MusicNavigation(f);}
                else ProfileCheck(false,"Unknown native profile extension");
            }
            if(Applying()) {
                if(!raceSeen){f.challengeSetup={};f.challengeModeSetup={};}
                if(!joystickSeen)f.joystickConfigurations.fill(0);
                if(!musicSeen){f.musicSettings.menuState={{0,5,0}};f.musicSettings.menuMode=0;}
            }
            if(mode==Mode::ExportLegacyText && (!raceSeen || !joystickSeen || !musicSeen)) {
                // Old native versions without a typed tail receive exactly
                // the same defaults as their existing binary load path.
                FrontendPlayerManager defaults;defaults.musicSettings.menuState={{0,5,0}};
                FrontendProfileStream extension{Mode::ExportText};extension.json=json;
                if(!raceSeen)extension.RaceSetup(defaults);
                if(!joystickSeen)extension.Joysticks(defaults);
                if(!musicSeen)extension.MusicNavigation(defaults);
            }
        }
    }
    void RaceSetup(FrontendPlayerManager& f) {
        const auto bounded=[&](const char* name,std::int32_t& value,std::int32_t low,std::int32_t high) {
            const auto copy=Scalar(name,value);ProfileCheck(copy>=low && copy<=high,"Saved RaceSetup choice is outside its source domain");
        };
        Object("race_setup");auto& choice=f.challengeSetup;
        bounded("players",choice.playerCount,1,2);bounded("game_type",choice.gameType,3,6);
        bounded("race_option_254",choice.raceOption254,0,1);bounded("opponent_skill",choice.opponentSkill,0,2);bounded("cop_skill",choice.copSkill,0,2);
        auto& modes=f.challengeModeSetup;Object("single");
        bounded("grid_without_cops",modes.single[0],0,2);bounded("grid_with_cops",modes.single[1],0,2);bounded("opponent_type",modes.single[2],0,1);
        bounded("catch_up",modes.single[3],0,1);bounded("traffic_without_cops",modes.single[4],0,3);bounded("traffic_with_cops",modes.single[5],0,3);End();
        Object("knockout");bounded("opponents",modes.knockout[0],0,7);bounded("opponent_type",modes.knockout[1],0,1);bounded("catch_up",modes.knockout[2],0,1);
        bounded("traffic_without_cops",modes.knockout[3],0,3);bounded("traffic_with_cops",modes.knockout[4],0,3);End();
        Object("lap_knockout");bounded("opponents",modes.lapKnockout[0],0,7);bounded("opponent_type",modes.lapKnockout[1],0,1);
        bounded("catch_up",modes.lapKnockout[2],0,1);bounded("traffic_without_cops",modes.lapKnockout[3],0,3);bounded("traffic_with_cops",modes.lapKnockout[4],0,3);End();
        Object("free_run");bounded("traffic_without_cops",modes.freeRun[0],0,3);bounded("traffic_with_cops",modes.freeRun[1],0,3);End();
        Object("be_the_cop");bounded("grid",modes.beTheCop[0],0,2);bounded("opponent_type",modes.beTheCop[1],0,1);
        bounded("catch_up",modes.beTheCop[2],0,1);bounded("traffic",modes.beTheCop[3],0,3);End();End();
    }
    void MusicNavigation(FrontendPlayerManager& f) {
        auto& navigation=f.musicSettings.menuState;Object("music_navigation");
        const auto playlist=Scalar("playlist",navigation[0]),song=Scalar("song",navigation[1]);
        Scalar("scroll_position",navigation[2]);const auto genre=Scalar("genre",f.musicSettings.menuMode);End();
        ProfileCheck(playlist>=0 && playlist<20 && song>=0 && song<32 && genre<6,
            "Saved music navigation index exceeds its original backing array");
    }
    void Joysticks(FrontendPlayerManager& f) {
        constexpr std::array<std::int32_t,16> allowed{{0,10,11,12,13,20,21,22,23,30,31,32,33,40,50,51}};
        Array("joystick_configurations",f.joystickConfigurations.size());
        for(auto& configuration:f.joystickConfigurations) {
            const auto value=Scalar(nullptr,configuration);
            ProfileCheck(std::find(allowed.begin(),allowed.end(),value)!=allowed.end(),"Saved joystick configuration is outside the original supported IDs");
        }End();
    }
};

namespace {
std::size_t ValidateProfileEnvelope(const void* data,std::size_t bytes)
{
    const auto* image=static_cast<const std::uint8_t*>(data);
    ProfileCheck(saveGameIsValid(image,bytes)!=0,"Native profile size/checksum does not match its stored image");
    ProfileCheck(loadWord(image)==GAME_SAVE_SIZE && !std::memcmp(image+4,"NFRED57",8) && loadWord(image+12)==1,
        "Unsupported native profile version; raw PS2 memory images are not this native format");
    const auto end=loadWord(image+16);ProfileCheck(end>=20 && end<=GAME_SAVE_CHECKSUM_OFFSET,"Saved payload length is invalid");
    for(std::size_t i=end;i<GAME_SAVE_CHECKSUM_OFFSET;++i)ProfileCheck(image[i]==0,"Native profile reserved bytes are not zero");
    return end;
}
void ProfileEnvelope(std::array<std::uint8_t,GAME_SAVE_SIZE>& image,std::size_t end)
{
    storeWord(image.data(),GAME_SAVE_SIZE);std::memcpy(image.data()+4,"NFRED57",8);storeWord(image.data()+12,1);
    storeWord(image.data()+16,static_cast<std::uint32_t>(end));
    ProfileCheck(saveGameFinalize(image.data(),image.size())!=0,"Native profile checksum finalization failed");
}
std::array<std::uint8_t,GAME_SAVE_SIZE> ParseTextProfile(const void* data,std::size_t bytes,FrontendPlayerManager& owner,bool checkEventBindings=true)
{
    FrontendProfileJson json(data,bytes);json.Header();std::array<std::uint8_t,GAME_SAVE_SIZE> image{};
    FrontendProfileStream stream{FrontendProfileStream::Mode::ImportText,image.data(),nullptr};stream.json=&json;stream.checkEventBindings=checkEventBindings;
    stream.Run(owner);json.End();ProfileEnvelope(image,stream.position);return image;
}
}

std::array<std::uint8_t,GAME_SAVE_SIZE> FrontendPlayerManager::SerializeNativeProfile() const
{
    std::array<std::uint8_t,GAME_SAVE_SIZE> image{};
    FrontendProfileStream stream{FrontendProfileStream::Mode::Save,image.data(),nullptr};stream.Run(const_cast<FrontendPlayerManager&>(*this));
    ProfileEnvelope(image,stream.position);return image;
}

bool FrontendPlayerManager::DeserializeNativeProfile(const void* data,std::size_t bytes,std::string& error)
{
    error.clear();
    try {
        const auto end=ValidateProfileEnvelope(data,bytes);const auto* image=static_cast<const std::uint8_t*>(data);
        FrontendProfileStream validate{FrontendProfileStream::Mode::Validate,nullptr,image,20,end};validate.Run(*this);
        ProfileCheck(validate.position==end,"Saved native profile has trailing fields from a different schema");
        // Every absent typed event cell is allocated during validation. The
        // final traversal only assigns scalars and moves prepared owners;
        // failures cannot leave half a loaded profile or invalidate borrowers.
        FrontendProfileStream commit{FrontendProfileStream::Mode::Commit,nullptr,image,20,end};commit.validated=&validate;commit.Run(*this);
        return true;
    } catch(const std::exception& e) {error=e.what();return false;}
}

std::string FrontendPlayerManager::SerializeNativeProfileText() const
{
    FrontendProfileJson json(false);json.Header();FrontendProfileStream stream{FrontendProfileStream::Mode::ExportText};stream.json=&json;
    stream.Run(const_cast<FrontendPlayerManager&>(*this));json.End();return json.Finish();
}

bool FrontendPlayerManager::ValidateNativeProfileText(const void* data,std::size_t bytes,std::string& error,
    bool checkEventBindings,std::string* playerName) const
{
    error.clear();
    try {
        auto& owner=const_cast<FrontendPlayerManager&>(*this);const auto image=ParseTextProfile(data,bytes,owner,checkEventBindings);
        const auto end=ValidateProfileEnvelope(image.data(),image.size());
        std::string name;
        FrontendProfileStream validate{FrontendProfileStream::Mode::Validate,nullptr,image.data(),20,end};
        validate.checkEventBindings=checkEventBindings;validate.playerName=playerName?&name:nullptr;validate.Run(owner);
        ProfileCheck(validate.position==end,"Saved text profile has an incomplete logical payload");
        if(playerName)*playerName=std::move(name);
        return true;
    } catch(const std::exception& e){error=e.what();return false;}
}

bool FrontendPlayerManager::DeserializeNativeProfileText(const void* data,std::size_t bytes,std::string& error)
{
    error.clear();
    try {const auto image=ParseTextProfile(data,bytes,*this);return DeserializeNativeProfile(image.data(),image.size(),error);}
    catch(const std::exception& e){error=e.what();return false;}
}

bool FrontendPlayerManager::ConvertLegacyProfileToText(const void* data,std::size_t bytes,std::string& text,std::string& error) const
{
    error.clear();
    try {
        const auto end=ValidateProfileEnvelope(data,bytes);const auto* image=static_cast<const std::uint8_t*>(data);
        auto& owner=const_cast<FrontendPlayerManager&>(*this);
        FrontendProfileStream validate{FrontendProfileStream::Mode::Validate,nullptr,image,20,end};validate.Run(owner);
        ProfileCheck(validate.position==end,"Legacy profile has trailing fields from a different schema");
        FrontendProfileJson json(false);json.Header();
        FrontendProfileStream stream{FrontendProfileStream::Mode::ExportLegacyText,nullptr,image,20,end};stream.json=&json;stream.Run(owner);json.End();
        text=json.Finish();return true;
    } catch(const std::exception& e){error=e.what();return false;}
}
} // namespace hp2

namespace hp2 {
void FrontendPlayerManager::InitWithDefaults(const FrontendRaceSetupBindings& bindings)
{
    // Final183830 startup stores use the existing native profile/resource
    // owners. FRONTB event resources may still be unloaded at InitFred:
    // with newly cleared progress no task/points/event reward is unlocked.
    Require(bindings.applicationOptions && bindings.cars && bindings.language && bindings.tracks &&
        players[0] && players[1] && highScores,"Frontend defaults lack the actual startup profile/resources");
    Require(bindings.language->currentLanguage>=0,"Frontend defaults require loaded final-language names");
    const auto& launch=*bindings.applicationOptions;
    for(std::size_t seat=0;seat<2;++seat) {
        auto& player=*players[seat];player.cars=bindings.cars;player.language=bindings.language;
        for(auto& choice:player.availability) {choice.status=3;choice.skinCount=0;choice.allowDashboardTextures=0;}
        for(auto& ride:player.selectedRides)ride.appearance={-1,0,0,0,0};
        for(std::int32_t model=0;model<64;++model) {
            if(!bindings.cars->IsAvailable(static_cast<std::size_t>(model)))continue;
            const auto& car=bindings.cars->Car(static_cast<std::size_t>(model));
            const bool unlocked=IsCarUnlocked(*this,bindings.events,model,gameType);
            const auto status=unlocked ? 0 : bindings.demoDisc && *bindings.demoDisc && car.challengeAvailability!=3 ?
                (car.bodyCategory==1 ? 3 : 0) : car.challengeAvailability;
            player.SetCarAvailability(status,model,-1,false);
        }
        const auto model=launch.startupPlayerModel[seat],skin=launch.startupPlayerSkin[seat];
        Require(model>=0 && model<64 && bindings.cars->IsAvailable(static_cast<std::size_t>(model)),
            "Startup model does not exist in the loaded final car catalogue");
        player.SetCarAvailability(0,model,skin,true);
        player.SetDefaultCarType(model,static_cast<std::uint32_t>(skin),launch.skipFrontend!=0);
        playerSettingsResetToDefaults(&player.settings,&launch);player.totalPoints=0;
        const auto* name=bindings.language->GetString(seat==0?0x27165:0x27166);
        Require(name && std::strlen(name)<sizeof(player.settings.playerName),"Final default player name exceeds its profile field");
        std::strcpy(player.settings.playerName,name);
    }
    autoSave=0;playerCount=1;gameType=3;
    trackSelection.InitWithDefaults(*bindings.tracks);eventSelection={};
    globalDisplaySettingsResetToDefaults(&displaySettings,&launch,&bindings.language->currentLanguage);
    // Display calibration is initialized by its original constructor and
    // applied on profile load; InitFred does not reset that live display slot.
    recordDisplaySuppressed=0;copsEnabled=1;
    selectedEventNumber=0;pendingState=0;raceAborted=0;savedChallengeOptions.fill(1);
    challengeSetup={};challengeModeSetup={};knockoutTrackPolicy=0;joystickConfigurations.fill(0);
    for(auto& bank:eventProgress.banks_)for(auto& cell:bank)if(cell) {
        const auto tag=static_cast<std::uint8_t>(cell->index());*cell=FrontendProfileStream::EmptyEvent(tag);
    }
    highScores->InitStatsTables();for(auto& name:recordPlayerNames)name.fill(0);
    // GetOptionsFromGame/GenreMgr needs the real mixer and final jukebox
    // cartridge, and is called through CaptureAudioOptions after those live
    // owners initialize. No guessed mixer volumes or song catalogue here.
}
} // namespace hp2

namespace hp2 {
void FrontendPlayerManager::SetupRaceParametersAfterRaceSetup(const FrontendRaceSetupBindings& bindings)
{
    // Final PAL1862B0..186708, reached by both Challenge callbacks and all
    // five concrete setup destructors. Source FRED lanes are documented in
    // the accompanying full-body receipt; no gameplay configuration is owned
    // by the menu or reconstructed from the text shown on screen.
    Require(bindings.parameters && bindings.language,
        "Race setup needs the actual RaceParameters/language owner");
    const auto numberOfPlayers=playerCount;
    auto cops=copsEnabled;
    auto difficulty=challengeSetup.opponentSkill;
    std::int32_t opponents=0,traffic=0,opponentType=0,catchup=1,damage=1;
    std::int32_t special=0,resetTimer=1,pulloverLimit=2,roadBlocks=0,choppers=0,seconds=0;
    const auto& options=challengeModeSetup;
    if(gameType==7) {
        opponents=numberOfPlayers==1?4:0;pulloverLimit=numberOfPlayers==1?0:2;
        difficulty=challengeSetup.copSkill;
        opponentType=options.beTheCop[1];catchup=options.beTheCop[2];traffic=options.beTheCop[3];
        damage=savedChallengeOptions[4];cops=1;special=1;roadBlocks=choppers=3;
        if(numberOfPlayers==1) {
            if(difficulty==1)seconds=125;
            else if(difficulty<2)seconds=150;
            else if(difficulty==2)seconds=110;
        } else {resetTimer=0;seconds=300;}
    } else if(gameType==3) {
        const auto& single=options.single;auto grid=single[0];
        if(numberOfPlayers==1 && cops)grid=single[1];
        else if(numberOfPlayers==2 && cops)grid=0;
        if(grid==1)opponents=1;
        else if(grid==2) {
            if(cops)opponents=numberOfPlayers==1?3:0;
            else opponents=numberOfPlayers==1?7:numberOfPlayers==2?2:0;
        }
        opponentType=single[2];catchup=single[3];traffic=single[cops?5:4];
        damage=savedChallengeOptions[0];
    } else if(gameType==4 || gameType==5) {
        opponents=cops?(numberOfPlayers==1?3:numberOfPlayers==2?2:0):numberOfPlayers==1?
            (gameType==4?options.knockout[0]:options.lapKnockout[0]):2;
        if(gameType==4) {
            opponentType=options.knockout[1];catchup=options.knockout[2];
            traffic=options.knockout[cops?4:3];damage=savedChallengeOptions[1];
        } else {
            opponentType=options.lapKnockout[1];catchup=options.lapKnockout[2];
            traffic=options.lapKnockout[cops?4:3];damage=savedChallengeOptions[2];
        }
    } else if(gameType==6) {
        traffic=options.freeRun[cops?1:0];damage=savedChallengeOptions[3];
        catchup=0;opponentType=0;
    }
    // PAL PMINW at1865A0 clamps only to eight minus the actual player count.
    opponents=std::min(opponents,8-numberOfPlayers);
    constexpr std::array<float,4> densities{{0.3300000131130218505859375f,
        0.666999995708465576171875f,1.0f,0.0f}};
    Require(traffic>=0 && traffic<4,"Race setup traffic choice is outside the authored density table");
    auto& race=*bindings.parameters;
    race.policeDriverCount=cops?(special?2:6):0;
    race.authoredPursuitOption26C=1;race.chopperEnabled=cops;
    race.opponentCount=opponents;race.trafficDensity=densities[static_cast<std::size_t>(traffic)];
    race.playerStartingPositions[0]=static_cast<std::int8_t>(opponents+1);
    race.playerStartingPositions[1]=static_cast<std::int8_t>(opponents+2);
    race.relativePaceEnabled=catchup;race.pursuitDifficulty=difficulty;
    race.opponentVehicleClass=opponentType;race.authoredRaceOption40=damage>0;
    race.initialRoadBlockRequests=roadBlocks;race.initialChopperRequests=choppers;
    race.pulloverLimit=pulloverLimit;race.resetPursuitTimerAfterPullover=resetTimer;
    race.authoredRaceOption48=displaySettings.authoredOption7>0;
    race.authoredRaceOption44=displaySettings.authoredOption6>0;
    race.roadBlocksEnabled=cops;race.authoredPursuitOption268=cops;
    race.opposingTrafficProbability=0.5f;race.opponentDifficulty=difficulty;
    race.specialPursuit=special;race.authoredRaceOption4C=0;
    if(special) {
        race.eventSelected=1;
        const auto* text=bindings.language->GetString(0xecd11);
        Require(text,"Be The Cop setup lacks the authored event description");
        std::strncpy(race.eventDescription,text,512);race.eventDescription[512]=0;
        race.timeLimitTicks=convertFloatToInt32(static_cast<float>(seconds)*4000.0f+0.5f);
    } else {race.timeLimitTicks=0;race.eventSelected=0;race.eventDescription[0]=0;}
}
} // namespace hp2

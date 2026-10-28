// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\fred.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/tRandomlyPickableTrackInfo.hpp"

#include "game_core/indep/src/fred.hpp"
#include "game_core/indep/inc/Fred.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::debug {
// Debug00185CD8, E-0098-S01. Original count increments even past ten storage slots.
void CarStatus::AddAllSkins(const CarTypeInfo& info,int language){
    const int count=info.GetNumAvailableSkins();nSkins=0;
    for(int i=0;i<count;++i){
        const auto skin=info.GetAvailableSkin(i,language);
        if(nSkins<10)aSkins[nSkins]=static_cast<std::uint8_t>(skin);
        ++nSkins;
    }
}
// Debug00185AF8, E-0098-S01. Deliberately leaves skins and dashboard state intact.
void FrontendPlayerInfo::FlushAvailableCars(){for(auto& car:aCarStatus)car.nStatus=eCAR_LOCKED_INVISIBLE;}
// Debug00185B28, E-0098-S01; host bounds guard replaces out-of-domain native access.
CarStatus& FrontendPlayerInfo::GetCarStatus(int type){
    if(type<0 || type>=64)hp2::not_implemented("0x00185B28","car identity outside original64 statuses");
    return aCarStatus[type];
}
// Debug00185AB0, E-0098-S01.
void FrontendPlayerInfo::SetCarAvailability(eCarStatus status,int type,int skin_number,bool dash_available,
    const std::array<CarTypeInfo,64>& cars,int language){
    if(type<0 || type>=64)hp2::not_implemented("0x00185AB0","car identity outside original64 statuses");
    auto& car=aCarStatus[type];car.nStatus=status;car.bDashAvailable=dash_available;car.nSkins=0;
    if(skin_number==-1)car.AddAllSkins(cars[type],language);
    else{car.aSkins[0]=static_cast<std::uint8_t>(skin_number);car.nSkins=1;}
}
// Debug00185B88, E-0098-S01. Zero count still selects stored slot0 in the original.
void FrontendPlayerInfo::BuildRideInfo(RideInfo& ri,int type,std::uint32_t skin_number,
    int use_dash_if_available,const std::array<CarTypeInfo,64>& cars) const{
    if(type<0 || type>=64)hp2::not_implemented("0x00185B88","invalid car identity");
    const auto& car=aCarStatus[type];
    if(car.nSkins<0 || car.nSkins>10)hp2::not_implemented("0x00185B88","skin list exceeds decoded storage");
    int i=0;while(i<car.nSkins && skin_number!=car.aSkins[i])++i;
    if(i==car.nSkins)skin_number=car.aSkins[0];
    ri.Type=type;ri.SkinNumber=static_cast<std::uint8_t>(skin_number);
    ri.HasDash=use_dash_if_available && car.bDashAvailable && cars[type].DashGeometryFilename[0];
    ri.CanBeVertexDamaged=0;ri.InstanceIndex=0;
}
// Debug00185C38, E-0098-S01. No Retail profile layout is bound here.
void FrontendPlayerInfo::AssignCarOfType(int game_type,int type,std::uint32_t skin_number,
    int use_dash_if_available,const std::array<CarTypeInfo,64>& cars){
    if(game_type<0 || game_type>=11)hp2::not_implemented("0x00185C38","invalid original game type");
    BuildRideInfo(RideInfoByGameMode[game_type],type,skin_number,use_dash_if_available,cars);
}
// Debug00185CC8, E-0098-S01.
RideInfo& FrontendPlayerInfo::GetSelectedRideInfo(int game_type){
    if(game_type<0 || game_type>=11)hp2::not_implemented("0x00185CC8","invalid original game type");
    return RideInfoByGameMode[game_type];
}
// Debug001822F0, E-0098-S01. Skin0 means all; dashboard is packed bit30.
void FrontendDatabase::MakeEventCarsAvailable(const EventTree& tree,const EventInfo& event,
    const std::array<CarTypeInfo,64>& cars,int language){
    for(auto& player:FrontendPlayers)player.FlushAvailableCars();
    for(int i=0;i<event.nCars;++i){
        const auto* packed=event.GetCar(tree,i);
        if(!packed)continue;
        const int type=(*packed>>16)&255,skin=(*packed>>24)&15;
        if(type>=64)hp2::not_implemented("0x001822F0","event car identity outside original table");
        for(auto& player:FrontendPlayers)player.SetCarAvailability(eCAR_UNLOCKED_FULLY_VISIBLE,
            type,skin?skin:-1,((*packed>>30)&1)!=0,cars,language);
    }
}
}

namespace hp2::recovered {
// ELF 0x001841A8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001841a8.md.
void FrontendDatabase::UseTheRaceEvent() {
    hp2::not_implemented("0x001841A8", "FrontendDatabase::UseTheRaceEvent (signature and full body unresolved)");
}

// ELF 0x001855F0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001855f0.md.
void FrontendDatabase::IsChallengeGame() {
    hp2::not_implemented("0x001855F0", "FrontendDatabase::IsChallengeGame (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00187928; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00187928.md.
void FrontendPlayerInfo::FlushAvailableCars() {
    hp2::not_implemented("0x00187928", "FrontendPlayerInfo::FlushAvailableCars (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00187080; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00187080.md.
void TransientStats::GetDriver() {
    hp2::not_implemented("0x00187080", "TransientStats::GetDriver (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/race/launch.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <cmath>
#include <algorithm>
namespace hp2::debug {
// Debug00182610, E-0098-RP02. First unused gender-compatible character; no random draw.
int PickCharacterNumberThatIsntRacing(std::array<int,8>& available,int gender,
    const std::function<bool(int)>& is_male){
    for(auto& number:available){
        if(number==-1)continue;
        bool match=gender==-1;
        if(!match){
            if(!is_male)hp2::not_implemented("0x00182610","original character body-type binding missing");
            const bool male=is_male(number);match=male?gender==0:gender==1;
        }
        if(match){const int result=number;number=-1;return result;}
    }
    return 0;
}
// Debug00183A08, E-0098-RP02. Original argument ignored; global race track is read.
int PickCopCharacterNumber(::port::driving::Memory& memory,std::uint32_t race,int unused,
    const ::port::race::ParticipantServices& services){
    (void)unused;
    if(!services.track_cop_hash || !services.character_number_from_hash)
        hp2::not_implemented("0x00183A08","original track/character binding missing");
    const auto hash=services.track_cop_hash(memory.signed_word(race));
    if(!hash)hp2::not_implemented("0x00183A08","original GetTrackInfo missing record");
    const auto character=services.character_number_from_hash(*hash?*hash:hp2::frontend::bStringHash("Cop"));
    return character?*character:-1;
}
// Debug00184390, E-0098-RP02. Bounded no-unassigned-racers branch used by first HP event.
// Power/weight ordering for unassigned racers remains explicitly untranslated.
void FrontendDatabase::CalculateStartingPositions(::port::driving::Memory& memory,std::uint32_t address){
    RaceParameters race(memory,address);const int count=memory.signed_word(address+0x308);
    if(count<0 || count>14)hp2::not_implemented("0x00184390","original driver count outside host table");
    for(int i=0;i<count;++i){const auto driver=race.GetDriverInfo(i);const int type=memory.signed_word(driver+8);
        if(memory.signed_byte(driver)==0 && memory.signed_byte(driver+4)==0 && type>0 && type<3)
            hp2::not_implemented("0x00184390","original unassigned racer power/weight ordering not translated");
    }
}
// Debug00183A58, E-0098-RP02. First single-player HP delivery path; other event/AI
// branches fail before mutation. Explicit services bind original resource globals.
void FrontendDatabase::SetupCarsForRace(::port::driving::Memory& memory,std::uint32_t address,
    const EventInfo& event,const ::port::race::ParticipantServices& services){
    if(GameType!=9 || event.nNumber!=1 || event.nType!=5 || event.nOpponents!=0
        || memory.word(address+0x30)!=1 || memory.word(address+0x34)!=0
        || memory.word(address+0x18)!=0 || memory.word(address+0x274)!=0 || memory.word(address+0x20)!=1)
        hp2::not_implemented("0x00183A58","non-first-delivery SetupCarsForRace branch not translated");
    if(!services.character_address || !services.skin_info)
        hp2::not_implemented("0x00183A58","original character/skin providers missing");
    RaceParameters race(memory,address);race.ClearEliminatedDrivers();
    hp2::recovered::bSetRandomSeed(memory,memory.word(address+0x30c),0x329060);
    std::array<int,8> characters{0,1,2,3,4,5,6,7};memory.word(address+0x308,0);
    ::port::driving::Memory::Scratch scratch(memory);const auto driver=memory.allocate(24);
    const int character=PickCharacterNumberThatIsntRacing(characters,FrontendPlayers[0].mPlayerSettings.PreferredGender,services.character_is_male);
    auto& selected=FrontendPlayers[0].GetSelectedRideInfo(GameType);
    selected.CanBeVertexDamaged=1;
    DriverInfo(memory,driver,0,1,selected,character,-2,memory.signed_byte(address+0x3c),services.character_address);
    race.AddDriverInfo(driver,services.skin_info);
    // RacerEliminated001FC808 is false for the guarded SINGLE_RACE branch.
    memory.word(address+0x30,1);memory.word(address+0x34,0);
    const int traffic=std::abs(memory.scalar(address+0x10))<=memory.scalar(0x3253fc)?0:6;
    const auto already_present=[&](int type){
        for(int i=0;i<memory.signed_word(address+0x308);++i){const auto entry=race.GetDriverInfo(i);
            if(memory.signed_byte(entry)==0 && memory.signed_word(entry+12)==type)return true;}
        return false;
    };
    for(int i=0;i<traffic;++i){
        RideInfo ride;ride.Init(-1,0,0,0);
        for(int attempt=0;attempt<100;++attempt){
            GenerateRandomRideInfo(memory,ride,CAR_USAGE_TYPE_TRAFFIC,-1,services.cars,race.GetLocationIndex(),services.language);
            ride.SkinNumber=255; // Original overwrites AFTER the skin RNG draw.
            if(!already_present(ride.Type))break;
        }
        DriverInfo(memory,driver,i+50,3,ride,-1,-1,0,services.character_address);race.AddDriverInfo(driver,services.skin_info);
    }
    int minimum_class=5; //0018420C uses signed SLT; decompiler's ulong is misleading.
    for(int i=0;i<memory.signed_word(address+0x308);++i){const auto type=memory.signed_word(race.GetDriverInfo(i)+12);
        if(type<0 || type>=64)hp2::not_implemented("0x00183A58","generated car outside original catalog domain");
        const int car_class=services.cars[type].CarClass;
        if(car_class<minimum_class)minimum_class=car_class;
    }
    int cops=memory.signed_word(address+0x25c);
    if(8<memory.signed_word(address+0x30)+memory.signed_word(address+0x34)+cops){
        cops=memory.signed_word(address+0x30)+memory.signed_word(address+0x34);memory.word(address+0x25c,cops);
    }
    for(int i=0;i<cops;++i){
        const int cop_character=PickCopCharacterNumber(memory,address,memory.signed_word(address),services);
        int type=-1,closest_class=0;
        for(int n=0;n<64;++n){const auto& car=services.cars[n];
            if(car.UsageType!=CAR_USAGE_TYPE_COP || !car.GeometryFilename[0])continue;
            if(type==-1 || std::abs(int(minimum_class)-car.CarClass)<std::abs(int(minimum_class)-closest_class)){
                type=n;closest_class=car.CarClass;
            }
        }
        RideInfo ride;ride.Init(type,1,0,0);
        DriverInfo(memory,driver,i+10,4,ride,cop_character,-1,0,services.character_address);race.AddDriverInfo(driver,services.skin_info);
    }
    CalculateStartingPositions(memory,address);
}
}

namespace hp2::debug {
// Debug00183378, E-0098-RP05. First HP delivery branch; requests original GameFlow
// unload/load transition, never directly starts a single-car fixture scene.
void FrontendDatabase::StartRace(::port::driving::Memory& memory,std::uint32_t address,
    RaceEvent& active,const ::port::race::StartServices& services){
    if(GameType!=9 || active.nType!=5 || active.nNumber!=1 || nCurrentEvent!=1
        || !active.pTree || active.pTree->nType!=1 || !active.pState.data || services.demo_replay_active)
        hp2::not_implemented("0x00183378","non-first-delivery or demo-replay StartRace branch not translated");
    if(!services.track_point_to_point || !services.copy_profile_settings || !services.set_record_times || !services.change_game_flow)
        hp2::not_implemented("0x00183378","required original track/profile/records/GameFlow binding missing");
    const auto event=std::find_if(active.pTree->pEvents.begin(),active.pTree->pEvents.end(),
        [&](const auto& info){return info.nNumber==nCurrentEvent;});
    if(event==active.pTree->pEvents.end())hp2::not_implemented("0x00183378","current original event not found in owning tree");
    // GetStatus(time-state) never returns INPROGRESS; keep status lookup before reset.
    const auto status=EventTimeState::GetStatus(active.pState,active.Objective);
    if(status!=eEVENT_INPROGRESS)active.ResetState();
    if(active.nTracks<1 || active.nTracks>8)hp2::not_implemented("0x00183378","original track-index division/table domain invalid");
    const int track_index=active.GetRaceIndex()%active.nTracks;
    EventTrack::SetRace(active.aTracks[track_index],memory.view(address,0x310),services.track_point_to_point);
    active.bWillSuperBonusEventBeUnlockedIfThisEventIsWon=0;
    active.bWillSuperBonusEventBeUnlockedIfThisEventIsWon=event->WillSuperBonusEventBeUnlockedIfThisEventIsWon();
    if(services.maximum_ai_cars<memory.signed_word(address+0x34))memory.word(address+0x34,services.maximum_ai_cars);
    if(8<memory.signed_word(address+0x34)+memory.signed_word(address+0x30))memory.word(address+0x34,8-memory.word(address+0x30));
    if(memory.word(address+0x28)==0){
        const auto track=services.track_point_to_point(memory.signed_word(address));
        if(!track)hp2::not_implemented("0x00183378","original track record required for point-to-point correction");
        if(*track)memory.word(address+0x28,1);
    }
    SetupCarsForRace(memory,address,*event,services.participants);
    services.copy_profile_settings(memory,address);
    for(int player=0;player<memory.signed_word(address+0x30);++player)services.set_record_times(player);
    RaceParameters(memory,address).ChooseHudTextures();
    // MaybeSetSelectedCarForMultiTrackEvent00182818 -> EventInfo::SetSelectedCar001FC5E8
    // makes no writes for the guarded type5; KO/tournament branches are not substituted.
    memory.word(address+0x258,1);
    using ::port::race::Flow;
    services.change_game_flow(services.current_flow==Flow::in_frontend?Flow::unloading_frontend:Flow::loading_track_common,Flow::none);
    bRaceWasAborted=0;
}
}

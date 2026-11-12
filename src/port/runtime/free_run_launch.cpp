#include "free_run_launch.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/indep/src/frontendscreens/racesetup.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <stdexcept>

namespace port::race {
void prepare_free_run_launch(driving::Memory& m, WorldLaunchRequest& request, const LaunchCatalog& catalog){
    constexpr std::uint32_t fred=0x2f3890, rp=0x311f78;
    const auto* track=catalog.track(request.track);
    if(!track)throw std::invalid_argument("Selected original FreeRun track is unavailable");
    int selected=request.car_id;
    if(selected<0){
        for(unsigned i=0;i<catalog.cars().size();++i){
            const auto& name=catalog.cars()[i].CarTypeName;
            const std::string code(name.data(),std::find(name.begin(),name.end(),'\0')-name.begin());
            if(code==request.car_code){selected=static_cast<int>(i);break;}
        }
    }
    if(selected<0||selected>=64)throw std::invalid_argument("Selected original FreeRun car is unavailable");
    const auto info=m.word(0x3278e0)+0x560u*static_cast<unsigned>(selected);
    if(!m.word(0x3278e0)||!m.byte(info+0x40))throw std::invalid_argument("Selected original FreeRun car has no geometry");
    const int skin=request.skin_id<0?m.signed_word(info+0x554):request.skin_id;
    if(!catalog.skin_info(selected,skin,0))throw std::invalid_argument("Selected original FreeRun skin is unavailable; no fallback skin selected");
    request.car_id=selected;request.skin_id=skin;
    const auto& name=catalog.cars()[selected].CarTypeName;
    request.car_code.assign(name.data(),std::find(name.begin(),name.end(),'\0')-name.begin());

    // Debug input staging is port-owned. These are the needed fresh-input
    // slices confirmed in original Fred.InitWithDefaults00181D78 and the
    // ChallengeScreen/SelectTrackScreen handlers, not a whole Fred default UI.
    hp2::debug::FrontendDatabase::Sub_00181BE8(m,fred);
    hp2::debug::RaceParameters(m,rp).InitWithDefaults({});
    hp2::debug::HighScoreTable::InitStatsTables(m,fred+0x6b4);
    hp2::debug::DataChallengeScreen::InitWithDefaults(m,fred+8);
    hp2::debug::DataFreeRunSetup::InitWithDefaults(m,fred+0x50);
    hp2::debug::tGlobalDisplaySettings::ResetToDefaults(m,fred+0x148,{});
    m.word(fred+0x18,0);m.word(fred+0x1c,request.minimal_driving?0:1);
    // TASK-0111 port preset, not a new original default. E-0111-P01:
    // original no-cops FreeRun traffic default is index3, density0. The source
    // producer below constructs the actual roster; no participant is deleted.
    m.word(fred+0xc,hp2::debug::FRED_CHALLENGE_GAME_FREE_RUN); // Explicit debug mode selection.
    m.word(fred+0x1072c,hp2::debug::FRED_CHALLENGE_GAME_FREE_RUN);
    m.word(fred+0x10734,1); // Explicit single-player debug input.
    m.word(rp+0x30,1);m.word(rp+0x254,m.word(fred+0x10));
    for(unsigned player=0;player<2;++player)
        hp2::debug::cPlayerSettings::ResetToDefaults(m,fred+0x10738+player*0x6d0+0x644,{});

    // A selected RideInfo is a debug input, not a CarStatus/profile unlock.
    // Use recovered RideInfo.Init00185FA0 and transport only its proven fields.
    hp2::debug::RideInfo ride;ride.Init(selected,skin,m.byte(info+0x80)!=0,0);
    const auto destination=hp2::debug::FrontendPlayerInfo::GetSelectedRideInfo(m,fred+0x10738,hp2::debug::FRED_CHALLENGE_GAME_FREE_RUN);
    m.word(destination,static_cast<std::uint32_t>(ride.Type));m.byte(destination+4,ride.InstanceIndex);
    m.byte(destination+5,ride.SkinNumber);m.byte(destination+6,ride.HasDash);m.byte(destination+7,ride.CanBeVertexDamaged);
    hp2::debug::FrontendDatabase::SetupRaceParametersAfterRaceSetup(m,fred);

    // The original SelectTrackScreen.StartRace0019E008 FreeRun stores.
    // TrackNumber/direction are explicit debug choices; Point2Point comes from
    // the individually reviewed TrackInfo field, never guessed from its name.
    m.word(rp,static_cast<std::uint32_t>(request.track));m.word(rp+0xc,request.reverse?1:0);
    m.word(rp+0x28,static_cast<std::uint32_t>(track->point_to_point));m.word(rp+0x20,1);m.word(rp+0x2c,0);m.word(rp+0x258,0);
    hp2::debug::FrontendDatabase::StartRace(m,fred);
    if(!hp2::debug::RaceParameters(m,rp).IsFreeRun())
        throw std::logic_error("Original FreeRun parameter producers did not publish a FreeRun request");
}
}

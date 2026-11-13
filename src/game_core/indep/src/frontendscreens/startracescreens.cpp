#include "FMenus.hpp"
#include "Fred.hpp"
#include "EventTree.hpp"
#include "Language.hpp"
#include "CarInfo.hpp"
#include "Race.hpp"
#include "Player.hpp"
#include "RaceCoordinator.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace hp2 {
std::array<float,4> startRaceEliminatedColor{{128.0f,0.0f,0.0f,128.0f}};
namespace {
StartRaceScreens* boundStart{};
std::unique_ptr<PreRaceStats> currentStats;
std::unique_ptr<PreRaceBlurb> currentBlurb;
std::unique_ptr<PreRace2PBeACop> currentTwoPlayerBlurb;
// Final GP-43E0: the last authored event126 glyph survives disconnection.
std::uint32_t objectiveButtonChar=0x89;
GameInputRouter& Input(RaceScreenServices& services)
{
    Require(services.frontend && services.frontend->screens && services.frontend->screens->input &&
        services.frontend->screens->input->router,"Race objective has no actual input owner");
    return *services.frontend->screens->input->router;
}
const RaceParameters& Parameters(const StartRaceScreens& owner)
{
    Require(owner.services.frontend && owner.services.frontend->raceSetup &&
        owner.services.frontend->raceSetup->parameters,"Start screens have no actual race setup");
    return *owner.services.frontend->raceSetup->parameters;
}
FlailerUiState& Ui(StartRaceScreens& owner)
{
    Require(owner.services.frontend && owner.services.frontend->ui &&
        owner.services.frontend->ui->resources,"Start screens have no actual Flailer resource owner");
    return *owner.services.frontend->ui;
}
bool ShowsKnockout(const StartRaceScreens& owner)
{
    // Final1E2920, no individually named beta PDR: internal direct closure.
    const auto& services=*owner.services.frontend;
    const auto type=services.frontend->gameType;
    if (type==4)return true;
    if (type!=8 && type!=9)return false;
    Require(services.raceSetup->currentEvent,"Knockout screen has no selected RaceEvent");
    return services.raceSetup->currentEvent->kind==3;
}
bool ShowsTournament(const StartRaceScreens& owner)
{
    // Final1E2980, same restriction on the original-name claim.
    const auto& services=*owner.services.frontend;
    const auto type=services.frontend->gameType;
    if (type!=8 && type!=9)return false;
    Require(services.raceSetup->currentEvent,"Tournament screen has no selected RaceEvent");
    return services.raceSetup->currentEvent->kind==4;
}
void SetStatColor(FlailerResources& resources,std::uint32_t screen,
    std::uint32_t key,const std::array<float,4>& color)
{
    auto* object=resources.FindObject(screen,key);
    if (!object)return;
    const auto rgb=static_cast<std::uint32_t>(convertFloatToInt32(color[0])) |
        (static_cast<std::uint32_t>(convertFloatToInt32(color[1]))<<8) |
        (static_cast<std::uint32_t>(convertFloatToInt32(color[2]))<<16);
    if (object->kind==1)static_cast<FlailerTextChunk*>(object)->SetColor(rgb);
    else if (object->kind==2)static_cast<FlailerBitmapChunk*>(object)->SetColor(rgb);
    object->SetAlpha(convertFloatToInt32(color[3]));
}
void ContinueRace(StartRaceScreens& owner)
{
    if (owner.services.coordinator)
        Require(raceActionsEnqueue(&owner.services.coordinator->actions,4,0)!=0,
            "RaceCoordinator message queue rejected pre-race continuation");
}
}
FlailerScreenConfig fPreRaceBlurbScreen{"Pre Race Blurb",PreRaceBlurb::Callback,
    {{static_cast<std::int32_t>(0x887B4D37u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};
FlailerScreenConfig fPreRace2PBeACopScreen{"Pre Race 2P Be A Cop",PreRace2PBeACop::Callback,
    {{static_cast<std::int32_t>(0xB5ECD7A0u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};
FlailerScreenConfig fPreRaceStatsScreen{"Pre Race Stats",PreRaceStats::Callback,
    {{static_cast<std::int32_t>(0x89B2FC0Fu),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};
FlailerScreenConfig fPreRaceLetterboxScreen{"Pre Race Letterbox",PreRaceLetterboxCallback,
    {{static_cast<std::int32_t>(0xA3E1BC36u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};
void BindStartRaceScreens(StartRaceScreens& owner)
{
    Require(!boundStart || boundStart==&owner,"Another StartRaceScreens owner is already bound");
    boundStart=&owner;
}
void UnbindStartRaceScreens()
{
    Require(!currentStats && !currentBlurb && !currentTwoPlayerBlurb,
        "StartRaceScreens must deactivate its live screen before unbinding");
    boundStart=nullptr;
}
std::exception_ptr StartRaceScreens::Shutdown() noexcept
{
    if(boundStart==this) {
        if(currentBlurb)closeFailure_=currentBlurb->Close();
        currentStats.reset();currentBlurb.reset();currentTwoPlayerBlurb.reset();
        boundStart=nullptr;
    }
    screenCount=0;current=0;
    return closeFailure_;
}
StartRaceScreens::~StartRaceScreens() noexcept {(void)Shutdown();}
std::int32_t StartRaceScreens::NeedToShowRaceObjective() const
{
    const auto& parameters=Parameters(*this);
    return parameters.specialPursuit ? parameters.mode==1 : parameters.eventSelected;
}
std::int32_t StartRaceScreens::NeedToShowRaceObjective2P() const
{
    const auto& parameters=Parameters(*this);
    return parameters.specialPursuit && parameters.mode==2;
}
std::int32_t StartRaceScreens::NeedToShowRaceStats() const
{
    return !Parameters(*this).specialPursuit && services.frontend->frontend->gameType!=6;
}
void StartRaceScreens::AddScreen(const char* name)
{
    if (screenCount<4)screens[static_cast<std::size_t>(screenCount++)]=name;
}
void StartRaceScreens::Enable()
{
    screenCount=0;current=0;screens[0]=nullptr;screens[1]=nullptr;
    if (NeedToShowRaceObjective())AddScreen("Pre Race Blurb");
    if (NeedToShowRaceObjective2P())AddScreen("Pre Race 2P Be A Cop");
    if (NeedToShowRaceStats())AddScreen("Pre Race Stats");
    if (!screenCount)AddScreen("Pre Race Letterbox");
    services.frontend->screens->ChangeToScreen(screens[static_cast<std::size_t>(current)],true);
}
void StartRaceScreens::Disable()
{
    services.frontend->screens->ChangeToScreen(nullptr,false);
}
bool ReadyToSkipStartRaceScreens(const StartRaceScreens& owner)
{
    return owner.current==owner.screenCount-1;
}
std::int32_t PreRaceStats::BuildStatsDefault()
{
    Require(manager_.services.race && manager_.services.players,
        "PreRaceStats has no actual race/player registry");
    const auto& race=*manager_.services.race;
    const auto& players=*manager_.services.players;
    const auto count=std::min(race.entryCount,stats.size());
    for (std::size_t index=0;index<count;++index) {
        Require(race.entries && race.entries[index] && race.entries[index]->car &&
            race.entries[index]->car->driverInfo,"PreRaceStats participant has no actual DriverInfo");
        const auto& driver=*race.entries[index]->car->driverInfo;
        auto& row=stats[index];
        if (driver.driverClass==1) {
            Require(driver.driverNumber>=0 && driver.driverNumber<2,
                "PreRaceStats human driver number is outside the real player registry");
            const auto* player=players.playersByNumber[static_cast<std::size_t>(driver.driverNumber)];
            Require(player && player->movement.seat>=0 && player->movement.seat<2,
                "PreRaceStats human driver has no live Player");
            row.name=player->settings.playerName;
            row.color=endGamePlayerColors[static_cast<std::size_t>(player->movement.seat)];
        } else {row.name=RaceDriverName(driver);row.color=endGameOpponentColor;}
        row.position=driver.startingPosition;
        row.model=driver.appearance.modelCode;row.status=0;row.points=0;
    }
    return static_cast<std::int32_t>(count);
}
void PreRaceStats::BuildStatsTournament()
{
    BuildStatsDefault();
    auto& event=*manager_.services.frontend->raceSetup->currentEvent;
    const auto* standings=event.GetTournamentState();
    Require(standings,"Tournament grid has no actual tournament state");
    const auto& race=*manager_.services.race;
    Require(race.entryCount<=stats.size(),"Tournament starting grid exceeds its original eight rows");
    for (std::size_t index=0;index<race.entryCount;++index) {
        const auto driver=race.entries[index]->car->driverInfo->driverNumber;
        const auto racer=event.RacerIndexFromDriverNumber(driver);
        Require(racer>=0 && racer<8,"Tournament driver does not map to an authored racer");
        stats[index].points=standings->points[static_cast<std::size_t>(racer)];
    }
}
void PreRaceStats::BuildStatsRaceKnockout()
{
    auto& setup=*manager_.services.frontend->raceSetup;
    Require(setup.currentEvent,"Knockout grid has no actual RaceEvent");
    (void)setup.currentEvent->GetKoState();
    auto count=BuildStatsDefault();
    for (std::int32_t index=0;index<count;++index)stats[static_cast<std::size_t>(index)].status=0x3AD6A;
    const auto& parameters=*setup.parameters;
    for (std::int32_t index=0;index<parameters.driverCount;++index) {
        const auto* driver=raceParametersGetDriverInfo(setup.parameters,index);
        Require(driver,"Knockout grid driver is absent from actual race setup");
        if (driver->eliminated<=0)continue;
        Require(count<8,"Knockout grid exceeds its original eight rows");
        auto& row=stats[static_cast<std::size_t>(count++)];
        row.model=driver->appearance.modelCode;row.name=RaceDriverName(*driver);
        if (driver->driverClass==1) {
            Require(parameters.runtimeSettings && driver->driverNumber>=0 && driver->driverNumber<2,
                "Eliminated human has no actual runtime PlayerSettings bank");
            row.name=parameters.runtimeSettings->players[static_cast<std::size_t>(driver->driverNumber)].playerName;
        }
        row.color=startRaceEliminatedColor;row.status=0x3AD69;row.points=0;row.position=driver->eliminated;
    }
}
PreRaceStat* PreRaceStats::GetStatsForRacePosition(std::int32_t position)
{
    for (auto& row:stats)if (row.position==position)return &row;
    return nullptr;
}
PreRaceStats::PreRaceStats(StartRaceScreens& owner,std::uint32_t screen)
    : manager_(owner),screen_(screen)
{
    auto& ui=Ui(owner);auto& resources=*ui.resources;
    Require(ui.language && owner.services.frontend->raceSetup->cars,
        "PreRaceStats has no actual Language/CarInfo catalogue");
    SetRaceTrackCaption(owner.services,screen,0xF601F2D4u,0xD0CED2E2u,1);
    const bool knockout=ShowsKnockout(owner),tournament=ShowsTournament(owner);
    resources.SetObjectVisibility(screen,0xDC289A64u,knockout||tournament);
    if (knockout) {resources.SetLanguageID(screen,0xDC289A64u,0x3A982);BuildStatsRaceKnockout();}
    else if (tournament) {resources.SetLanguageID(screen,0xDC289A64u,0x3AD6B);BuildStatsTournament();}
    else BuildStatsDefault();
    for (std::int32_t position=1;position<=8;++position) {
        const auto pos=SprintfAndHash("POS_%d",position),name=SprintfAndHash("NAME_%d",position);
        const auto type=SprintfAndHash("TYPE_%d",position),status=SprintfAndHash("STATUS_%d",position);
        const auto* row=GetStatsForRacePosition(position);
        for (const auto key:{pos,name,type,status})resources.SetObjectVisibility(screen,key,row!=nullptr);
        if (!row)continue;
        resources.Printf(screen,pos,"%2d",row->position);
        resources.Printf(screen,name,"%s",row->name);
        const auto& car=owner.services.frontend->raceSetup->cars->Car(static_cast<std::size_t>(row->model));
        resources.Printf(screen,type,"%s",GetTranslatedString(*ui.language,car.GetFullName(*ui.language)));
        if (knockout)resources.SetLanguageID(screen,status,static_cast<std::uint32_t>(row->status));
        else if (tournament)resources.Printf(screen,status,"%d",row->points);
        else resources.SetObjectVisibility(screen,status,0);
        for (const auto key:{pos,name,type,status})SetStatColor(resources,screen,key,row->color);
    }
    resources.SetObjectVisibility(screen,0xAF6F1CDCu,owner.NeedToShowRaceObjective()||owner.NeedToShowRaceObjective2P());
    resources.SetObjectVisibility(screen,0x0019DE3Eu,1);
}
PreRaceStats::~PreRaceStats()=default;
void PreRaceStats::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    if (message==ScreenMessage::Previous || message==ScreenMessage::Next) {data.sound=0;return;}
    if (message==ScreenMessage::LeaveScreen) {
        data.value=0;
        const auto* previous=manager_.screens[static_cast<std::size_t>(manager_.current)];
        if (manager_.current>0)--manager_.current;
        if (previous==manager_.screens[static_cast<std::size_t>(manager_.current)])data.sound=0;
        else manager_.services.frontend->screens->ChangeToScreen(
            manager_.screens[static_cast<std::size_t>(manager_.current)],false);
    } else if (message==ScreenMessage::Select) {
        if (data.value==7)data.sound=0;
        else {
            Require(manager_.services.coordinator,"PreRaceStats select has no actual RaceCoordinator");
            ContinueRace(manager_);
        }
    }
}
void PreRaceStats::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundStart && data,"PreRaceStats callback has no real owner/payload");
    // Menu item retirement precedes ScreenDeactivate even if constructing
    // the concrete screen failed during ScreenActivate.
    if(!currentStats && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if (message==ScreenMessage::ScreenActivate) {
        Require(!currentStats,"PreRaceStats is already active");
        currentStats=std::make_unique<PreRaceStats>(*boundStart,data->key);
    } else if (message==ScreenMessage::ScreenDeactivate)currentStats.reset();
    else {Require(currentStats!=nullptr,"PreRaceStats callback has no live screen");currentStats->HandleEvent(message,*data);}
}
void PreRaceLetterboxCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundStart && data,"PreRaceLetterbox callback has no real owner/payload");
    if (message==ScreenMessage::Select)ContinueRace(*boundStart);
    else if (message==ScreenMessage::LeaveScreen) {data->value=0;data->sound=0;}
    else if (message==ScreenMessage::Previous || message==ScreenMessage::Next)data->sound=0;
}
void EnableStartRaceScreens(StartRaceScreens& owner) {owner.Enable();}
void DisableStartRaceScreens(StartRaceScreens& owner) {owner.Disable();}
const char* FigureOutRaceObjective(RaceScreenServices& services,std::int32_t player)
{
    Require(services.frontend && services.frontend->raceSetup && services.frontend->raceSetup->parameters &&
        services.frontend->ui && services.frontend->ui->language,"Race objective has no actual owners");
    const auto& parameters=*services.frontend->raceSetup->parameters;
    auto& language=*services.frontend->ui->language;
    // Source+258 is eventGame, distinct from eventSelected at+50.
    if (!parameters.specialPursuit || parameters.eventGame)return parameters.eventDescription;
    if (parameters.mode==1) {
        auto& input=Input(services);
        if (!joystickIsPluggedIn(&input,0))return language.GetString(0xDBBAB);
        if (joystickIsTypeWheel(&input,0))
            return language.GetString(joystickGetType(&input,0)==0x16?0xECD24:0xECD21);
        Require(services.players && player>=0 && player<2 && services.players->playersByNumber[player],
            "Race objective has no actual live Player settings");
        const auto mode=services.players->playersByNumber[player]->settings.gearInputMode;
        const bool digital=joystickGetType(&input,0)==0x41;
        if (mode==0)return language.GetString(digital?0xECD23:0xECD20);
        if (mode==1 || mode==2)return language.GetString(digital?0xECD22:0xECD11);
        return nullptr;
    }
    if (parameters.mode==2) {
        if (player==0)return language.GetString(0xECD1A);
        if (player==1)return language.GetString(0xECD1B);
    }
    return nullptr;
}
namespace {
void AdvanceObjective(StartRaceScreens& manager,ScreenCallbackData& data)
{
    if (data.value==7)return;
    data.value=0;data.sound=0;
    Require(manager.current>=0 && manager.current<manager.screenCount,
        "Objective navigation is outside the actual start screen list");
    const auto* previous=manager.screens[static_cast<std::size_t>(manager.current)];
    if (manager.current<manager.screenCount-1)++manager.current;
    const auto* next=manager.screens[static_cast<std::size_t>(manager.current)];
    if (previous!=next) {
        manager.services.frontend->screens->ChangeToScreen(next,false);data.sound=5;
    } else {
        Require(manager.services.coordinator,"Objective continuation has no actual RaceCoordinator");
        ContinueRace(manager);
    }
}
}
PreRaceBlurb::PreRaceBlurb(StartRaceScreens& manager,std::uint32_t screen)
    :manager_(manager),screen_(screen)
{
    auto& ui=Ui(manager);const auto& parameters=Parameters(manager);
    if (parameters.eventGame) {
        Require(ui.language && manager.services.frontend->raceSetup->currentEvent,
            "Event objective title has no actual RaceEvent/Language");
        const auto title=FormatLocalizedText(ui.language->GetString(0xF619),
            {static_cast<std::int32_t>(manager.services.frontend->raceSetup->currentEvent->number)});
        ui.resources->SetLocalizedText(screen,0x6A77AF98u,title.c_str());
    } else ui.resources->SetLanguageID(screen,0x6A77AF98u,parameters.specialPursuit?0xECD10:0xF618);
    ui.resources->SetObjectVisibility(screen,0x0019DE3Eu,1);
}
std::exception_ptr PreRaceBlurb::Close() noexcept
{
    if(closed_)return closeFailure_;
    closed_=true;
    try {Ui(manager_).resources->SetObjectVisibility(screen_,0x0019DE3Eu,0);}
    catch(...) {closeFailure_=std::current_exception();}
    return closeFailure_;
}
PreRaceBlurb::~PreRaceBlurb() noexcept {(void)Close();}
void PreRaceBlurb::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    if (message==ScreenMessage::Previous || message==ScreenMessage::Next) {data.sound=0;return;}
    if (message==ScreenMessage::RenderModels) {
        auto& ui=Ui(manager_);auto& input=Input(manager_.services);
        if (joystickIsPluggedIn(&input,0))objectiveButtonChar=joystickGetButtonChar(&input,0,126);
        const auto* format=FigureOutRaceObjective(manager_.services,0);
        Require(format && ui.render,"Race objective has no source text or live renderer");
        const auto text=FormatLocalizedButtonText(format,objectiveButtonChar,512);
        ui.render->RenderTextIntoRectangle(*ui.resources,screen_,0xAEFE36D8u,0xCCCF9A75u,text.c_str(),1.0f);
    } else if (message==ScreenMessage::Select)AdvanceObjective(manager_,data);
}
void PreRaceBlurb::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundStart && data,"PreRaceBlurb callback has no actual owner/payload");
    if(!currentBlurb && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if (message==ScreenMessage::ScreenActivate) {
        Require(!currentBlurb,"PreRaceBlurb is already active");
        currentBlurb=std::make_unique<PreRaceBlurb>(*boundStart,data->key);
    } else if (message==ScreenMessage::ScreenDeactivate) {
        const auto failure=currentBlurb?currentBlurb->Close():std::exception_ptr{};
        currentBlurb.reset();
        if(failure)std::rethrow_exception(failure);
    }
    else if (message==ScreenMessage::LeaveScreen) {data->value=0;data->sound=0;}
    else {Require(currentBlurb!=nullptr,"PreRaceBlurb has no live screen");currentBlurb->HandleEvent(message,*data);}
}
PreRace2PBeACop::PreRace2PBeACop(StartRaceScreens& manager,std::uint32_t screen)
    :manager_(manager),screen_(screen)
{
    Ui(manager).resources->SetObjectVisibility(screen,0x0019DE3Eu,1);
}
PreRace2PBeACop::~PreRace2PBeACop()=default;
void PreRace2PBeACop::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    if (message==ScreenMessage::Previous || message==ScreenMessage::Next) {data.sound=0;return;}
    if (message==ScreenMessage::RenderModels) {
        auto& ui=Ui(manager_);Require(ui.render,"Two-player objective has no actual renderer");
        const auto* first=FigureOutRaceObjective(manager_.services,0);
        const auto* second=FigureOutRaceObjective(manager_.services,1);
        Require(first && second,"Two-player objective has no actual source texts");
        ui.render->RenderTextIntoRectangle(*ui.resources,screen_,0x8EC51608u,0x232320F5u,first,1.0f);
        ui.render->RenderTextIntoRectangle(*ui.resources,screen_,0x8EC51609u,0x232320F6u,second,1.0f);
    } else if (message==ScreenMessage::Select)AdvanceObjective(manager_,data);
}
void PreRace2PBeACop::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundStart && data,"PreRace2PBeACop callback has no actual owner/payload");
    if(!currentTwoPlayerBlurb && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if (message==ScreenMessage::ScreenActivate) {
        Require(!currentTwoPlayerBlurb,"PreRace2PBeACop is already active");
        currentTwoPlayerBlurb=std::make_unique<PreRace2PBeACop>(*boundStart,data->key);
    } else if (message==ScreenMessage::ScreenDeactivate)currentTwoPlayerBlurb.reset();
    else if (message==ScreenMessage::LeaveScreen) {data->value=0;data->sound=0;}
    else {Require(currentTwoPlayerBlurb!=nullptr,"PreRace2PBeACop has no live screen");currentTwoPlayerBlurb->HandleEvent(message,*data);}
}

}

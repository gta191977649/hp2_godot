#include "FMenus.hpp"
#include "FEAnim.hpp"
#include "Fred.hpp"
#include "Language.hpp"
#include "EasterEggs.hpp"
#include "CarSounds.hpp"
#include "GameFlow.hpp"
#include "Flags.hpp"
#include "bware/inc/bMath.hpp"
#include <climits>
#include <vector>
#include "FMenus.hpp"
#include "bware/inc/bWare.hpp"
#include <cstring>
#include <memory>

namespace hp2 {

// Final3319F8. The source checks and clears this independent cell after
// returning from another frontend screen; the input queue remains EasterEggs.
std::int32_t mainMenuClearEasterEggButtons=0;

namespace {
constexpr std::uint32_t GlobalScreen=0xae0f8aa1u;
constexpr std::uint32_t EasterEggGroup=0x74e404b4u;
constexpr std::uint32_t MainMenuItems[]={
    0x6e34e350u,0x708a0bf1u,0x72df3492u,0x75345d33u,0x778985d4u};

float MainMenuLeft(FlailerUiState& ui,std::uint32_t screen)
{
    auto* object=ui.resources->FindObject(screen,MainMenuItems[0]);
    Require(object,"Main menu has no authored first item anchor");
    float x,y;ui.resources->GetTopLeft(*object,x,y,ui.language);
    return x;
}

// Port-side rows borrow the third authored row's font and geometry.
class PortMainMenuRow final {
public:
    PortMainMenuRow(MainMenuScreenServices& services,std::uint32_t screen,int slot)
        :ui_(*services.ui)
    {
        resources_=ui_.resources->FindScreen(screen);
        auto* node=services.screens->active;
        screen_=node?node->FindFlailerScreen(static_cast<std::int32_t>(screen)):nullptr;
        Require(resources_ && screen_ && screen_->menu,"Port row has no active main-menu owner");
        menu_=screen_->menu;
        const auto* sourceText=static_cast<const FlailerTextChunk*>(ui_.resources->FindObject(screen,0x52e039f5u,1));
        const auto* sourceMask=static_cast<const FlailerBitmapChunk*>(ui_.resources->FindObject(screen,MainMenuItems[2],2));
        const auto key=sourceText ? sourceText->key+static_cast<std::uint32_t>(slot-3):0;
        Require(sourceText && sourceMask && !ui_.resources->FindObject(screen,key),
            "Port row requires the original three-row main menu");
        text_=std::make_unique<FlailerTextChunk>(*sourceText);
        mask_=std::make_unique<FlailerBitmapChunk>(*sourceMask);
        const bool fourth=slot==4;
        const auto group=hashString(fourth?"MENUITEM_4_GROUP":"MENUITEM_5_GROUP");
        const float offset=55.0f*static_cast<float>(slot-3);
        PrepareObject(*text_,key,group,fourth?"MENUITEM_4":"MENUITEM_5",offset);
        PrepareObject(*mask_,MainMenuItems[slot-1],group,fourth?"MI_4_MASK":"MI_5_MASK",offset);
        if(text_->clipObjectKey==sourceMask->key)text_->clipObjectKey=mask_->key;
        text_->textCapacity=64;text_->ownedText.assign(65,0);text_->text=text_->ownedText.data();
        text_->languageId=0;
        // Allocate the node before publishing either object into live rings.
        item_=std::make_unique<FlailerMenuItem>(menu_,text_->key,slot);
        item_->object=text_.get();
        Append(*mask_);Append(*text_);++resources_->bitmapCount;++resources_->textCount;
        item_->ResetSavedParameters();
        item_->previous=menu_->root.previous;item_->next=&menu_->root;
        item_->previous->next=item_.get();menu_->root.previous=item_.get();
    }
    ~PortMainMenuRow()
    {
        // Normal FlailerScreen deactivation destroys its authored menu first
        // and clears screen.menu before this callback owner is retired. On a
        // constructor unwind the same menu is still alive: unlink our node.
        if(item_ && screen_->menu==menu_) {
            if(menu_->current==item_.get())menu_->current=menu_->root.next==item_.get()?nullptr:menu_->root.next;
            item_->previous->next=item_->next;item_->next->previous=item_->previous;
        }
        item_.reset();
        Remove(*text_);Remove(*mask_);--resources_->textCount;--resources_->bitmapCount;
    }
    void SetLabel(const char* label) {text_->SetString(label);}
    const char* ExitDescription() const
    {
        if(IsPublishedRussian())return "B\xD2XO\xC4 \xD1" "3 \xD1\xC8P\xD2";
        constexpr const char* text[]{
            "Exit the game and return to Windows.","Quitter le jeu et revenir a Windows.",
            "Spiel beenden und zu Windows zurueckkehren.","Esci dal gioco e torna a Windows.",
            "Salir del juego y volver a Windows.","Avsluta spelet och aterga till Windows."};
        return text[Language()];
    }
    const char* ExitLabel() const
    {
        if(IsPublishedRussian())return "B\xD2XO\xC4";
        constexpr const char* labels[]{"Exit","Quitter","Beenden","Esci","Salir","Avsluta"};
        return labels[Language()];
    }
private:
    std::size_t Language() const
    {
        const auto language=ui_.language?ui_.language->currentLanguage:0;
        return language>=0 && language<6?static_cast<std::size_t>(language):0;
    }
    bool IsPublishedRussian() const
    {
        // The accepted custom FE_ENGLISH uses its own byte-font mapping, not
        // UTF-8 or CP1251. Recognise its exact existing Options label; use the
        // same verified glyph bytes without replacing that language image.
        const auto* label=ui_.language?ui_.language->GetString(10005):nullptr;
        return Language()==0 && label && std::strcmp(label,"O\xC9\xD9\xD1\xD1")==0;
    }
    static void PrepareObject(FlailerObjectChunk& object,std::uint32_t key,std::uint32_t group,
        const char* name,float offset)
    {
        object.next=nullptr;object.previous=nullptr;object.strip=nullptr;
        object.key=key;object.parentKey=group;object.name.fill(0);
        std::memcpy(object.name.data(),name,std::strlen(name));
        object.topLeftY+=offset;object.visible=1;object.updateKind=10;
    }
    void Append(FlailerObjectChunk& object)
    {
        object.previous=resources_->objects.previous;object.next=&resources_->objects;
        object.previous->next=&object;resources_->objects.previous=&object;
    }
    static void Remove(FlailerObjectChunk& object)
    {
        object.DeleteFlailerStrip();object.previous->next=object.next;object.next->previous=object.previous;
        object.next=nullptr;object.previous=nullptr;
    }
    FlailerUiState& ui_;
    FlailerScreenResource* resources_{};
    FlailerScreen* screen_{};
    FlailerMenu* menu_{};
    std::unique_ptr<FlailerTextChunk> text_;
    std::unique_ptr<FlailerBitmapChunk> mask_;
    std::unique_ptr<FlailerMenuItem> item_;
};

class NewMainMenu final {
public:
    NewMainMenu(MainMenuScreenServices& services,std::uint32_t screen)
        : services_(services),screenKey_(screen),
          extra_(services.extension?std::make_unique<PortMainMenuRow>(services,screen,4):nullptr),
          exit_(services,screen,extra_?5:4),
          blurb_(*services.ui,screen,0x75980cf0u,0x75980cf0u,0x70a404ecu),
          highlight_(*services.ui,screen,0xab671c9au,services.ui->animationSettings.highlightColor),
          slide_(*services.ui,screen,MainMenuLeft(*services.ui,screen))
    {
        // PAL216460: preserve the original resource/profile/audio/movie
        // order. No menu success is synthesized when a dependency is absent.
        const auto& setup=*services_.raceSetup;
        services_.frontend->MakeChallengeCarsAvailable(*setup.cars,*setup.events,*setup.demoDisc);
        raceParametersInitializeDefaults(setup.parameters,setup.applicationOptions,
            &setup.language->currentLanguage);
        idleStartedTicks_=services_.ui->clock->realTimer;
        Require(soundJukeboxStart(*services_.sounds,*services_.frontend,*setup.parameters,
            *setup.flow,true)==WORLD_AUDIO_OK,"Main menu jukebox failed on its real audio owner");
        selection_.showOtherItems=1;
        if(extra_)extra_->SetLabel(services.extension->label);
        exit_.SetLabel(exit_.ExitLabel());
        selection_.AddItems(*services_.ui->resources,screenKey_,MainMenuItems,extra_?5:4);
        services_.screens->StartMovie("MOVIES\\mainmenu_movie.AVI");
        Require(easterEggs.Activate()==GAME_INPUT_OK,"Main menu could not register actual EasterEgg input");
        services_.ui->resources->SetGroupVisibility(GlobalScreen,EasterEggGroup,0);
    }

    ~NewMainMenu()
    {
        // PAL216598. Member destruction then restores the alpha/slide
        // resources in reverse construction order; there is no extra movie
        // stop here in the original owner.
        easterEggs.UnActivate();
        services_.ui->resources->SetGroupVisibility(GlobalScreen,EasterEggGroup,0);
    }

    void HandleEvent(ScreenMessage message,ScreenCallbackData& data)
    {
        // Final24-entry jump table at2BC480: only these five messages have
        // work. LeaveScreen17 is the original back-navigation veto.
        switch (message) {
        case ScreenMessage::LeaveScreen:data.sound=0;data.value=0;break;
        case ScreenMessage::Tick:OnTick();break;
        case ScreenMessage::Highlight:OnHighlight(data);break;
        case ScreenMessage::Input:idleStartedTicks_=services_.ui->clock->realTimer;break;
        case ScreenMessage::Select:
            if (data.key==0x52e039f3u) {
                services_.frontend->copsEnabled=1;
                services_.screens->ChangeToScreen("Cops On Mode Select",true);
            } else if (data.key==0x52e039f4u) {
                services_.frontend->copsEnabled=0;
                services_.screens->ChangeToScreen("Cops Off Mode Select",true);
            } else if (data.key==0x52e039f5u) {
                services_.screens->ChangeToScreen("Options",true);
            } else if (data.key==0x52e039f6u && extra_) {
                services_.screens->ChangeToScreen(services_.extension->screen->name,true);
            } else if (data.key==(extra_?0x52e039f7u:0x52e039f6u)) {
                // The next native application tick exits through normal
                // ApplicationRuntime::Shutdown and its actual owner cleanup.
                applicationDebugExitRequested=true;
            }
            break;
        default:break;
        }
    }

    void OnTick()
    {
        auto& ui=*services_.ui;
        auto& screens=*services_.screens;
        const auto now=ui.clock->realTimer;
        if (screens.help->IsActive()) {
            easterEggs.active=0;
            idleStartedTicks_=now;
        } else {
            easterEggs.active=1;
            if (easterEggs.pendingMessage) {
                idleStartedTicks_=now;
                ui.sound->Play(30);
                ui.resources->SetGroupVisibility(GlobalScreen,EasterEggGroup,1);
                const auto language=easterEggs.lastMessage;
                easterEggs.pendingMessage=0;
                ui.resources->SetLanguageID(GlobalScreen,0xf58c1a4bu,language);
                easterEggMessageActive_=true;
                easterEggMessageStarted_=now;
            }
            // Final-only2167BC..2167D4; beta has no corresponding gate.
            if (mainMenuClearEasterEggButtons) {
                easterEggs.ClearButtons();
                mainMenuClearEasterEggButtons=0;
            }
        }
        if (screens.GetPopupActiveState(0))idleStartedTicks_=now;
        services_.demoMode->ConsiderStartDemoMode(idleStartedTicks_);
        blurb_.Animate();
        highlight_.Animate();
        slide_.Animate();
        if (easterEggMessageActive_) {
            const auto raw=static_cast<std::uint32_t>(now)-static_cast<std::uint32_t>(easterEggMessageStarted_);
            std::int32_t elapsed;std::memcpy(&elapsed,&raw,sizeof(elapsed));
            if (3.0f<static_cast<float>(elapsed)*0.0002500000118743628f) {
                easterEggMessageActive_=false;
                ui.resources->SetGroupVisibility(GlobalScreen,EasterEggGroup,0);
            }
        }
    }

    void HighlightMenuItem(std::uint32_t group,std::int32_t languageId,const char* nativeText=nullptr)
    {
        auto& ui=*services_.ui;
        std::int32_t left,top,right,bottom;
        Require(ui.resources->GetBoundingBox(screenKey_,group,left,top,right,bottom,ui.language),
            "Main menu selection has no authored bounding rectangle");
        if (auto* lower=ui.resources->FindObject(screenKey_,0x32c91daeu))
            lower->SetTopLeftY(static_cast<float>(bottom)-3.0f);
        if (auto* upper=ui.resources->FindObject(screenKey_,0xc671680cu))
            upper->SetTopLeftY(static_cast<float>(top)-6.0f);
        if(nativeText)blurb_.NewText(nativeText);else blurb_.NewText(languageId);
        selection_.SetCurrentItem(group);
    }

    void OnHighlight(ScreenCallbackData& data)
    {
        highlight_.SetCurrentMenuItem(data.key);
        switch (data.key) {
        case 0x52e039f3u:HighlightMenuItem(0x6e34e350u,10016);break;
        case 0x52e039f4u:HighlightMenuItem(0x708a0bf1u,10015);break;
        case 0x52e039f5u:HighlightMenuItem(0x72df3492u,10009);break;
        case 0x52e039f6u:HighlightMenuItem(MainMenuItems[3],0,
            extra_?services_.extension->description:exit_.ExitDescription());break;
        case 0x52e039f7u:
            if(extra_)HighlightMenuItem(MainMenuItems[4],0,exit_.ExitDescription());
            break;
        default:break;
        }
    }

private:
    MainMenuScreenServices& services_;
    std::uint32_t screenKey_;
    std::unique_ptr<PortMainMenuRow> extra_;
    PortMainMenuRow exit_;
    FlailerAnimatedBlurb blurb_;
    FlailerHighlightAnimation highlight_;
    FlailerSlideInAnimation slide_;
    std::int32_t idleStartedTicks_{};
    FlailerSelectionSet selection_;
    bool easterEggMessageActive_{};
    std::int32_t easterEggMessageStarted_{};
};

MainMenuScreenServices* mainMenuServices{};
std::unique_ptr<NewMainMenu> mainMenuOwner;
std::unique_ptr<CopsOnModeSelect> copsOnOwner;
std::unique_ptr<CopsOffModeSelect> copsOffOwner;
}

void BindMainMenuScreen(MainMenuScreenServices& services)
{
    Require(!mainMenuOwner && !copsOnOwner && !copsOffOwner,
        "Cannot rebind services while an original main-menu screen owns authored objects");
    Require(services.ui && services.ui->resources && services.ui->clock && services.ui->sound &&
        services.ui->language && services.screens && services.screens->ui==services.ui &&
        services.screens->help && services.frontend && services.raceSetup && services.sounds &&
        services.demoMode,"Main menu bindings lack actual UI/profile/audio/demo owners");
    const auto& setup=*services.raceSetup;
    Require(setup.parameters && setup.applicationOptions && setup.language && setup.flow &&
        setup.cars && setup.events && setup.demoDisc && services.ui->flow==setup.flow &&
        services.ui->language==setup.language && services.demoMode->screens==services.screens &&
        services.demoMode->frontend==services.frontend && services.demoMode->raceSetup==&setup,
        "Main menu bindings do not refer to the same live race/frontend owners");
    BindOptionsScreen(services);
    BindRaceSetupScreens(services);
    BindJoystickScreens(services);
    BindNameEntryScreen(services);
    mainMenuServices=&services;
}

void UnbindMainMenuScreen()
{
    UnbindNameEntryScreen();
    UnbindJoystickScreens();
    UnbindRaceSetupScreens();
    UnbindOptionsScreen();
    copsOnOwner.reset();
    copsOffOwner.reset();
    mainMenuOwner.reset();
    mainMenuServices=nullptr;
}

void MainMenuScreenCallback(ScreenMessage message,ScreenCallbackData* data)
{
    // PAL216A10. The native unique_ptr replaces only source allocation and
    // deleting-destructor flags. Ordinary messages reach the same owner.
    Require(mainMenuServices && data,"Main menu callback has no actual binding/data");
    if (message==ScreenMessage::ScreenActivate) {
        Require(!mainMenuOwner,"Main menu received duplicate activation");
        mainMenuOwner=std::make_unique<NewMainMenu>(*mainMenuServices,data->key);
    } else if (message==ScreenMessage::ScreenDeactivate) {
        mainMenuOwner.reset();
    } else {
        Require(mainMenuOwner!=nullptr,"Main menu message arrived outside its actual lifetime");
        mainMenuOwner->HandleEvent(message,*data);
    }
}

namespace {
// PAL31A438, passed by the exact source static constructor218038.
constexpr std::int32_t MainMenuHelpItems[]={800001,800002,0,0};
}

// Native registration of the original fNewMainMenuScreen. The PAL
// constructor218038 binds precisely this name, callback, resource key and
// help array; no screen identity is inferred from shared template objects.
FlailerScreenConfig fNewMainMenuScreen={
    "New Main Menu",&MainMenuScreenCallback,
    {{static_cast<std::int32_t>(0xae0f8aa1u),0,0,0}},{{-1,-1,-1,-1}},1,MainMenuHelpItems};


// PAL216AA8. The temporary source list contains two possible directions per
// unlocked authored record. Native storage keeps that exact order without
// copying the source list's invalid temporary link pointers into the result.
MainMenuTrackDescription PickTrackNumberForExpressGame(const MainMenuScreenServices& services)
{
    if (applicationBootState.trackSelectedInConfigFile>=0)
        return {applicationBootState.trackSelectedInConfigFile,0};
    Require(services.raceSetup && services.raceSetup->tracks && services.raceSetup->events,
        "Express track selection requires the actual track/event catalogues");
    const auto& setup=*services.raceSetup;
    const auto& tracks=*setup.tracks;
    std::vector<MainMenuTrackDescription> choices;
    Require(tracks.Records().size()<=static_cast<std::size_t>(INT32_MAX),
        "Express track catalogue exceeds the original signed record range");
    for (std::size_t index=0;index<tracks.Records().size();++index) {
        const auto* track=tracks.Find(static_cast<std::int32_t>(index));
        if (!track)continue;
        if (IsTrackUnlocked(*setup.events,track->baseNumber,0))
            choices.push_back({track->number,0});
        if (IsTrackUnlocked(*setup.events,track->baseNumber,1))
            choices.push_back({track->number,1});
    }
    Require(!choices.empty() && choices.size()<=UINT32_MAX,
        "Express race has no actual unlocked track/direction");
    return choices[vectorDefaultRandomBounded(static_cast<std::uint32_t>(choices.size()))];
}

// PAL216C18. This configures the existing FRED and RaceParameters owners and
// finishes at the real StartRace/flow request, not an appearance-only helper.
void StartQuickRace(MainMenuScreenServices& services,std::int32_t manualTransmission,int requestedTrack)
{
    Require(services.raceSetup && services.frontend && services.screens,
        "Quick race has no actual frontend/race/screen bindings");
    const auto& setup=*services.raceSetup;
    Require(setup.parameters && setup.applicationOptions && setup.language && setup.tracks &&
        setup.cars && setup.events && setup.demoDisc && services.frontend->players[0],
        "Quick race lacks actual catalogue/configuration/profile owners");
    auto& race=*setup.parameters;
    auto& frontend=*services.frontend;
    auto& profile=*frontend.players[0];
    raceParametersInitializeDefaults(&race,setup.applicationOptions,&setup.language->currentLanguage);
    services.screens->StopMovie();
    const auto cops=frontend.copsEnabled;
    frontend.gameType=2;
    frontend.playerCount=1;
    const auto description=requestedTrack>=0 ? MainMenuTrackDescription{requestedTrack,0}:PickTrackNumberForExpressGame(services);
    const auto* track=setup.tracks->Find(description.trackNumber);
    Require(track,"Quick race selected a missing authored track");
    race.selectedScenario=description.trackNumber;
    race.reverse=description.direction;
    race.authoredTrackOptionSecond=track->timeOfDay;
    race.pointToPoint=track->routeKind;
    race.opponentCount=cops?3:7;
    race.policeDriverCount=cops?4:0;
    race.pulloverLimit=cops?2:0;
    race.playerStartingPositions[0]=static_cast<std::uint8_t>(race.opponentCount+1);
    race.playerPaceContribution[1]=1.0f;
    race.trafficDensity=0.33000001311302185f;
    race.opposingTrafficProbability=0.5f;
    race.playerPaceContribution[0]=1.0f;
    race.pursuitDifficulty=1;
    race.raceKind=1;
    race.totalLaps=2;
    race.eventGame=0;
    race.opponentDifficulty=1;
    race.relativePaceEnabled=1;
    race.mode=1;
    race.roadBlocksEnabled=cops;
    race.authoredPursuitOption268=cops;
    race.chopperEnabled=cops;
    race.authoredPursuitOption26C=cops;
    race.initialRoadBlockRequests=0;
    race.initialChopperRequests=0;
    race.pulloversEnabled=cops;
    race.eventSelected=0;
    race.eventDescription[0]='\0';
    race.authoredRaceOption4C=0;
    frontend.MakeChallengeCarsAvailable(*setup.cars,*setup.events,*setup.demoDisc);
    if (requestedTrack<0 && applicationBootState.trackSelectedInConfigFile>=0) {
        const auto model=setup.applicationOptions->startupPlayerModel[0];
        profile.SetCarAvailability(0,model,-1,true);
        profile.AssignCarOfType(frontend.gameType,model,
            static_cast<std::uint32_t>(setup.applicationOptions->startupPlayerSkin[0]),false);
    } else {
        CarAppearanceSelection ride{};
        ride.modelCode=-1;
        do {
            Require(setup.cars->WeightedPickRide(ride,0,-1,race,*setup.language),
                "Quick race could not select a real weighted car/skin");
        } while (profile.GetCarStatus(ride.modelCode)!=0);
        profile.AssignCarOfType(frontend.gameType,ride.modelCode,
            static_cast<std::uint32_t>(ride.skinNumber),false);
    }
    // Original profile+647 is cPlayerSettings.TransmissionType (+3 in the
    // source settings record), already represented by gearInputMode.
    profile.settings.gearInputMode=static_cast<std::int8_t>(manualTransmission!=0);
    frontend.StartRace(setup);
}

// One owned native arrangement per original concrete Cops screen. All game
// data and services are borrowed. Animations and authored object selection
// retain their original construction/destruction order.
struct CopsMenuState {
    MainMenuScreenServices& services;
    std::uint32_t screen;
    FlailerSelectionSet selection;
    FlailerAnimatedBlurb blurb;
    FlailerHighlightAnimation highlight;
    FlailerSlideInAnimation slide;
    std::unique_ptr<FlailerExitScreenAnimation> exit;
    std::int32_t l1Status{};
    CopsMenuState(MainMenuScreenServices& bindings,std::uint32_t key)
        : services(bindings),screen(key),
          blurb(*bindings.ui,key,0x75980cf0u,0x75980cf0u,0x70a404ecu),
          highlight(*bindings.ui,key,0xab671c9au,bindings.ui->animationSettings.highlightColor),
          slide(*bindings.ui,key,MainMenuLeft(*bindings.ui,key)) {}
};

namespace {
GameInputRouter& CopsInput(MainMenuScreenServices& services)
{
    Require(services.screens && services.screens->input && services.screens->input->router,
        "Cops mode screen requires the actual frontend input router");
    return *services.screens->input->router;
}
void BeginCopsExit(CopsMenuState& state,const char* destination)
{
    Require(!state.exit,"Cops mode already owns an exit animation");
    auto& ui=*state.services.ui;
    if(ui.externalPresentation) {
        state.services.screens->ChangeToScreen(destination,true);
        return;
    }
    state.exit=std::make_unique<FlailerExitScreenAnimation>(ui,*state.services.screens,
        state.screen,destination,true);
    auto* anchor=ui.resources->FindObject(state.screen,0x32c91daeu);
    Require(anchor,"Cops mode exit has no authored lower highlight anchor");
    float x,y;
    ui.resources->GetTopLeft(*anchor,x,y,ui.language);
    state.exit->AddGroup(0x97bf4b25u,0x32c91daeu,x,650.0f,state.exit->MaximumDuration());
}
void SetCopsHighlight(CopsMenuState& state,std::uint32_t group,std::int32_t languageId)
{
    auto& ui=*state.services.ui;
    std::int32_t left,top,right,bottom;
    Require(ui.resources->GetBoundingBox(state.screen,group,left,top,right,bottom,ui.language),
        "Cops mode selection has no authored bounding rectangle");
    if (auto* lower=ui.resources->FindObject(state.screen,0x32c91daeu))
        lower->SetTopLeftY(static_cast<float>(bottom)-3.0f);
    if (auto* upper=ui.resources->FindObject(state.screen,0xc671680cu))
        upper->SetTopLeftY(static_cast<float>(top)-6.0f);
    state.blurb.NewText(languageId);
    state.selection.SetCurrentItem(group);
}
}

CopsOnModeSelect::CopsOnModeSelect(MainMenuScreenServices& services,std::uint32_t screen)
    : state_(std::make_unique<CopsMenuState>(services,screen))
{
    // PAL216E80. The actual authored filename is not inferred from the
    // display label; disabled/enabled behavior belongs to ScreenList.
    services.screens->StartMovie("MOVIES\\hotpursuit_movie.AVI");
    state_->selection.showOtherItems=1;
    state_->selection.AddItems(*services.ui->resources,screen,MainMenuItems,5);
    state_->l1Status=0;
    GameInputTarget target{};
    target.kind=GAME_INPUT_COPS_ON_MODE_SELECT;target.owner.copsOnModeSelect=this;
    auto& input=CopsInput(services);
    Require(gameInputRegister(&input,0,23,target)==GAME_INPUT_OK,
        "Cops On could not register its actual event23 input owner");
    const auto result=soundJukeboxStart(*services.sounds,*services.frontend,
        *services.raceSetup->parameters,*services.raceSetup->flow,true);
    if (result!=WORLD_AUDIO_OK)gameInputUnregister(&input,0,23);
    Require(result==WORLD_AUDIO_OK,"Cops On jukebox failed on the actual audio owner");
}
CopsOnModeSelect::~CopsOnModeSelect()
{
    // PAL216F88: unregister before destroying exit/slide/highlight/blurb.
    gameInputUnregister(&CopsInput(state_->services),0,23);
}
void CopsOnModeSelect::StartChangeToScreen(const char* destination)
{
    // PAL217020, including the independently authored GP9538 =650.0f.
    BeginCopsExit(*state_,destination);
}
void CopsOnModeSelect::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    // PAL2170B0 actually ticks twice when no exit is active. Preserve both
    // calls: selection/animation state is observable between them.
    if (message==ScreenMessage::Tick)OnTick();
    if (state_->exit) {state_->exit->HandleMessages(message,data);return;}
    if (message==ScreenMessage::Highlight) {OnHighlight(message,data);return;}
    if (message==ScreenMessage::Tick) {OnTick();return;}
    if (message!=ScreenMessage::Select)return;
    auto& services=state_->services;
    auto& frontend=*services.frontend;
    switch (data.key) {
    case 0x52e039f3u:
        if (data.joystickPort==0)StartQuickRace(services,state_->l1Status);
        else data.sound=0;
        break;
    case 0x52e039f4u:
        frontend.recordDisplaySuppressed=1;
        StartChangeToScreen("ChallengeBeACop");
        break;
    case 0x52e039f5u:
        services.raceSetup->parameters->relativePaceEnabled=1;
        frontend.recordDisplaySuppressed=0;
        StartChangeToScreen("Challenge");
        break;
    case 0x52e039f6u:
        if (data.joystickPort==0) {
            frontend.gameType=9;frontend.playerCount=1;
            frontend.eventSelection.treeType=1;
            frontend.eventSelection.lastViewState=0;
            StartChangeToScreen("View Event Tree");
        } else data.sound=0;
        break;
    case 0x52e039f7u:StartChangeToScreen("Options");break;
    default:break;
    }
}
void CopsOnModeSelect::OnTick()
{
    // Actual source callee16E368 is only JR RA/NOP; no empty runtime method.
    state_->blurb.Animate();
    state_->highlight.Animate();state_->slide.Animate();
}
void CopsOnModeSelect::SetupHighlight(std::uint32_t group,std::int32_t languageId)
{
    SetCopsHighlight(*state_,group,languageId);
}
void CopsOnModeSelect::OnHighlight(ScreenMessage,ScreenCallbackData& data)
{
    state_->highlight.SetCurrentMenuItem(data.key);
    switch (data.key) {
    case 0x52e039f3u:SetupHighlight(MainMenuItems[0],10017);break;
    case 0x52e039f4u:SetupHighlight(MainMenuItems[1],10014);break;
    case 0x52e039f5u:SetupHighlight(MainMenuItems[2],10012);break;
    case 0x52e039f6u:SetupHighlight(MainMenuItems[3],10013);break;
    case 0x52e039f7u:SetupHighlight(MainMenuItems[4],10009);break;
    default:break;
    }
}
void CopsOnModeSelect::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(mainMenuServices && data,"Cops On callback has no actual binding/data");
    if (message==ScreenMessage::ScreenActivate) {
        Require(!copsOnOwner,"Cops On received duplicate activation");
        copsOnOwner=std::make_unique<CopsOnModeSelect>(*mainMenuServices,data->key);
    } else if (message==ScreenMessage::ScreenDeactivate)copsOnOwner.reset();
    else {
        Require(copsOnOwner!=nullptr,"Cops On message arrived outside its actual lifetime");
        copsOnOwner->HandleEvent(message,*data);
    }
}
void CopsOnModeSelect::JoyHandlerBridge(std::int32_t port,std::int32_t event,
    std::uint8_t value,CopsOnModeSelect* owner)
{
    owner->JoyHandler(port,event,static_cast<std::int32_t>(value));
}
void CopsOnModeSelect::JoyHandler(std::int32_t,std::int32_t event,std::int32_t value)
{
    if (event==23)state_->l1Status=value;
}

CopsOffModeSelect::CopsOffModeSelect(MainMenuScreenServices& services,std::uint32_t screen)
    : state_(std::make_unique<CopsMenuState>(services,screen))
{
    // The space after '_' is present in final PAL2BC5C0. Keep it exactly.
    services.screens->StartMovie("MOVIES\\worldracing_ movie.AVI");
    state_->selection.showOtherItems=1;
    state_->selection.AddItems(*services.ui->resources,screen,MainMenuItems,5);
    state_->l1Status=0;
    GameInputTarget target{};
    target.kind=GAME_INPUT_COPS_OFF_MODE_SELECT;target.owner.copsOffModeSelect=this;
    auto& input=CopsInput(services);
    Require(gameInputRegister(&input,0,23,target)==GAME_INPUT_OK,
        "Cops Off could not register its actual event23 input owner");
    const auto result=soundJukeboxStart(*services.sounds,*services.frontend,
        *services.raceSetup->parameters,*services.raceSetup->flow,true);
    if (result!=WORLD_AUDIO_OK)gameInputUnregister(&input,0,23);
    Require(result==WORLD_AUDIO_OK,"Cops Off jukebox failed on the actual audio owner");
}
CopsOffModeSelect::~CopsOffModeSelect()
{
    gameInputUnregister(&CopsInput(state_->services),0,23);
}
void CopsOffModeSelect::StartChangeToScreen(const char* destination)
{
    // PAL2176D8, same value read from the distinct source GP953C cell.
    BeginCopsExit(*state_,destination);
}
void CopsOffModeSelect::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    // PAL217768 has only the initial tick; unlike Cops On there is no
    // second Tick branch after checking the active exit animation.
    if (message==ScreenMessage::Tick)OnTick();
    if (state_->exit) {state_->exit->HandleMessages(message,data);return;}
    if (message==ScreenMessage::Highlight) {OnHighlight(message,data);return;}
    if (message!=ScreenMessage::Select)return;
    auto& services=state_->services;
    auto& frontend=*services.frontend;
    switch (data.key) {
    case 0x52e039f3u:
        if (data.joystickPort==0)StartQuickRace(services,state_->l1Status);
        else data.sound=0;
        break;
    case 0x52e039f4u:
        services.raceSetup->parameters->relativePaceEnabled=1;
        frontend.recordDisplaySuppressed=0;
        StartChangeToScreen("Challenge");
        break;
    case 0x52e039f5u:
        if (data.joystickPort==0) {
            frontend.gameType=8;frontend.playerCount=1;
            frontend.eventSelection.treeType=0;
            frontend.eventSelection.lastViewState=0;
            StartChangeToScreen("View Event Tree");
        } else data.sound=0;
        break;
    case 0x52e039f6u:StartChangeToScreen("Options");break;
    default:break;
    }
}
void CopsOffModeSelect::OnTick()
{
    // Actual source callee16E368 is only JR RA/NOP; no empty runtime method.
    state_->blurb.Animate();
    state_->highlight.Animate();state_->slide.Animate();
}
void CopsOffModeSelect::SetupHighlight(std::uint32_t group,std::int32_t languageId)
{
    SetCopsHighlight(*state_,group,languageId);
}
void CopsOffModeSelect::OnHighlight(ScreenMessage,ScreenCallbackData& data)
{
    state_->highlight.SetCurrentMenuItem(data.key);
    switch (data.key) {
    case 0x52e039f3u:SetupHighlight(MainMenuItems[0],10006);break;
    case 0x52e039f4u:SetupHighlight(MainMenuItems[1],10007);break;
    case 0x52e039f5u:SetupHighlight(MainMenuItems[2],10008);break;
    case 0x52e039f6u:SetupHighlight(MainMenuItems[3],10009);break;
    default:break;
    }
}
void CopsOffModeSelect::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(mainMenuServices && data,"Cops Off callback has no actual binding/data");
    if (message==ScreenMessage::ScreenActivate) {
        Require(!copsOffOwner,"Cops Off received duplicate activation");
        copsOffOwner=std::make_unique<CopsOffModeSelect>(*mainMenuServices,data->key);
    } else if (message==ScreenMessage::ScreenDeactivate)copsOffOwner.reset();
    else {
        Require(copsOffOwner!=nullptr,"Cops Off message arrived outside its actual lifetime");
        copsOffOwner->HandleEvent(message,*data);
    }
}
void CopsOffModeSelect::JoyHandlerBridge(std::int32_t port,std::int32_t event,
    std::uint8_t value,CopsOffModeSelect* owner)
{
    owner->JoyHandler(port,event,static_cast<std::int32_t>(value));
}
void CopsOffModeSelect::JoyHandler(std::int32_t,std::int32_t event,std::int32_t value)
{
    if (event==23)state_->l1Status=value;
}

namespace {
constexpr std::int32_t CopsOnHelpItems[]={802225,802226,802227,802231,0,0};
constexpr std::int32_t CopsOffHelpItems[]={802228,802229,802230,802232,0,0};
}
// Exact static registrations PAL21806C/218098. Shared template object hashes
// are not used as screen identities; these distinct screen keys are authored.
FlailerScreenConfig fCopsOnModeSelectScreen={
    "Cops On Mode Select",&CopsOnModeSelect::Callback,
    {{static_cast<std::int32_t>(0x5b35f533u),0,0,0}},{{-1,-1,-1,-1}},1,CopsOnHelpItems};
FlailerScreenConfig fCopsOffModeSelecttScreen={
    "Cops Off Mode Select",&CopsOffModeSelect::Callback,
    {{static_cast<std::int32_t>(0x033e7771u),0,0,0}},{{-1,-1,-1,-1}},1,CopsOffHelpItems};

} // namespace hp2

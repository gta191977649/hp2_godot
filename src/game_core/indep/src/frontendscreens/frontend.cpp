#include "FMenus.hpp"
#include "Fred.hpp"
#include "DemoDisc.hpp"
#include "GameFlow.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include "Joylog.hpp"
#include "indep/src/MoviePlayer/MV3Codec.h"
#include "DemoModeReplay.hpp"
#include "Flags.hpp"
#include "FEAnim.hpp"
#include "Language.hpp"
#include "CarSounds.hpp"
#include "bware/inc/bMath.hpp"
#include <cstring>
#include <cstdio>
#include <limits>

namespace hp2 {

// PAL330664 / beta FrontendLaunchType. This is independent of the live
// ApplicationFrontendFlow state and retains the source initial value.
std::int32_t frontendLaunchType=1;

namespace {
constexpr std::uint32_t TitleKey=0xb5761d00u;
constexpr std::uint32_t TitleMovie=0x3acf2d73u;
constexpr std::uint32_t TitleAlternate=0xbfaebf79u;
constexpr std::uint32_t TitlePrompt=0xb070dbc9u;
constexpr std::uint32_t LegalImage=0xdd479599u;
constexpr std::uint32_t LegalTexture=0xa7ebc389u;
constexpr std::uint32_t AntiPiracyTexture=0x382582f3u;
// These two source callbacks own independent completion cells. They do not
// hold a MumboJumbo pointer which could outlive an asynchronous texture read.
bool legalScreenLoaded{};
bool antiPiracyScreenLoaded{};
constexpr float LegalDuration=7.0f;       // PAL330650.
constexpr float AntiPiracyDuration=7.0f;  // PAL330654.
FrontendStartupServices* startupServices{};
std::unique_ptr<TitleScreen> titleOwner;
std::unique_ptr<MumboJumbo> mumboOwner;

// Authored final PAL2FB750/2FB7D0. German/Italian/Spanish/Swedish use the
// English NFS introduction, while the EA logo has a separate language file.
constexpr TitleMovieDescription TitleMovies[6][2]={
    {{"MOVIES\\INTRO\\EALOGO_EN.AVI",0},{"MOVIES\\INTRO\\NFSINTRO_EN.AVI",1}},
    {{"MOVIES\\INTRO\\EALOGO_FR.AVI",0},{"MOVIES\\INTRO\\NFSINTRO_FR.AVI",1}},
    {{"MOVIES\\INTRO\\EALOGO_GE.AVI",0},{"MOVIES\\INTRO\\NFSINTRO_EN.AVI",1}},
    {{"MOVIES\\INTRO\\EALOGO_IT.AVI",0},{"MOVIES\\INTRO\\NFSINTRO_EN.AVI",1}},
    {{"MOVIES\\INTRO\\EALOGO_SP.AVI",0},{"MOVIES\\INTRO\\NFSINTRO_EN.AVI",1}},
    {{"MOVIES\\INTRO\\EALOGO_SW.AVI",0},{"MOVIES\\INTRO\\NFSINTRO_EN.AVI",1}}};
constexpr TitleMovieDescription DemoTitleMovies[6]={
    {"MOVIES\\INTRO\\EALOGO_EN.AVI",0},{"MOVIES\\INTRO\\EALOGO_FR.AVI",0},
    {"MOVIES\\INTRO\\EALOGO_GE.AVI",0},{"MOVIES\\INTRO\\EALOGO_IT.AVI",0},
    {"MOVIES\\INTRO\\EALOGO_SP.AVI",0},{"MOVIES\\INTRO\\EALOGO_SW.AVI",0}};

std::int32_t ElapsedTitleTicks(std::int32_t now,std::int32_t then)
{
    const auto raw=static_cast<std::uint32_t>(now)-static_cast<std::uint32_t>(then);
    std::int32_t elapsed;std::memcpy(&elapsed,&raw,sizeof(elapsed));return elapsed;
}

void StartTitleMusic(MainMenuScreenServices& services)
{
    const auto& setup=*services.raceSetup;
    Require(soundJukeboxStart(*services.sounds,*services.frontend,*setup.parameters,
        *setup.flow,true)==WORLD_AUDIO_OK,"Title jukebox failed on the actual audio owner");
}
}

const TitleMovieDescription& GetTitleMovieDescription(std::int32_t index,
    const LanguageManager& language,const ::DemoDiscManager& demo)
{
    Require(language.currentLanguage>=0 && language.currentLanguage<6 && index>=0 &&
        index<GetNumTitleMovies(demo),"Title description has no authored language/movie entry");
    const auto selected=static_cast<std::size_t>(language.currentLanguage);
    return demo.ActivationWord() ? DemoTitleMovies[selected] : TitleMovies[selected][static_cast<std::size_t>(index)];
}

std::int32_t GetNumTitleMovies(const ::DemoDiscManager& demo)
{
    return demo.ActivationWord() ? 1 : 2;
}

TitleScreen::TitleScreen(FrontendStartupServices& services,std::uint32_t screen)
    : screenKey(screen),services_(services),highlight_(std::make_unique<FlailerHighlightAnimation>(
        *services.menus->ui,TitleKey,0xab671c9au,services.menus->ui->animationSettings.highlightColor))
{
    auto& menu=*services_.menus;auto& ui=*menu.ui;
    Require(ui.movie && services_.movieLog,"Title screen has no actual movie/replay owners");
    ui.resources->SetObjectVisibility(TitleKey,TitleMovie,0);
    ui.resources->SetObjectVisibility(TitleKey,0x807d4c5cu,0);
    ui.resources->SetObjectVisibility(TitleKey,TitleAlternate,0);
    if (menu.raceSetup->applicationOptions->skipMovies) {
        phase=1;StartTitleMusic(menu);
    } else {
        phase=2;
        bitmap=static_cast<FlailerBitmapChunk*>(ui.resources->FindObject(TitleKey,TitleMovie,2));
        Require(bitmap,"Title screen is missing its actual authored movie bitmap");
        ui.movie->Close();OpenCurrentMovie();
        ui.resources->SetObjectVisibility(TitleKey,0x807d4c5cu,0);
    }
    startedTicks=ui.clock->realTimer;
    highlight_->SetCurrentMenuItem(TitlePrompt);
}

TitleScreen::~TitleScreen()
{
    for(const auto key:{TitleMovie,TitleAlternate})
        if(auto* object=services_.menus->ui->resources->FindObject(TitleKey,key,2))
            static_cast<FlailerBitmapChunk*>(object)->SetNativeFullscreenMovie(false);
    if (const auto failure=services_.menus->ui->movie->Shutdown()) {
        try {std::rethrow_exception(failure);}
        catch (const std::exception& error) {RecordSessionEvent("movie.title.cleanup_failed",error.what());}
        catch (...) {RecordSessionEvent("movie.title.cleanup_failed","Unknown movie cleanup failure");}
    }
}

void TitleScreen::OpenCurrentMovie()
{
    // Arrangement of the repeated source calls, not another decoder owner.
    auto& menu=*services_.menus;auto& ui=*menu.ui;
    const auto& movie=GetTitleMovieDescription(movieIndex,*ui.language,*menu.demoMode->demoDisc);
    Require(ui.movie->InitFile(movie.filename)!=0, std::string("Title movie could not open ")+movie.filename);
    if(auto* object=ui.resources->FindObject(TitleKey,movie.alternateBitmap?TitleAlternate:TitleMovie,2))
        static_cast<FlailerBitmapChunk*>(object)->SetNativeFullscreenMovie(ui.externalPresentation || movie.alternateBitmap==0);
    ui.resources->SetObjectVisibility(TitleKey,movie.alternateBitmap ? TitleAlternate : TitleMovie,1);
}

bool TitleScreen::CanAcceptInput() const
{
    // Final-only16F4F8 dependency: no beta name or separately counted body.
    if (!inputVisibleTicks || inputVisibleTicks==std::numeric_limits<std::int32_t>::max())return false;
    return static_cast<float>(ElapsedTitleTicks(services_.menus->ui->clock->realTimer,inputVisibleTicks))*
        0.0002500000118743628f>4.0f;
}

void TitleScreen::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    auto& menu=*services_.menus;
    switch (message) {
    case ScreenMessage::LeaveScreen:data.value=0;data.sound=0;return;
    case ScreenMessage::Left:case ScreenMessage::Right:
    case ScreenMessage::Previous:case ScreenMessage::Next:
    case ScreenMessage::ItemEvent:case ScreenMessage::ScreenBack:
        data.sound=0;return;
    case ScreenMessage::Select:
        if (phase==1) {
            if (CanAcceptInput() && !menu.demoMode->demoDisc->ActivationWord()) {
                startedTicks=menu.ui->clock->realTimer;
                // ChangeToScreen may destroy this exact owner. No member is
                // accessed after the original tail transition.
                menu.screens->ChangeToScreen("New Main Menu",true);return;
            }
        } else if (phase==2 && movieIndex!=0) {
            menu.ui->movie->Close();phase=1;startedTicks=menu.ui->clock->realTimer;
            highlight_->ForceStart();return;
        }
        data.sound=0;return;
    default:return;
    }
}

void TitleScreen::HandleTick()
{
    auto& menu=*services_.menus;auto& ui=*menu.ui;
    auto& demo=*menu.demoMode->demoDisc;
    if (phase==2) {
        Require(bitmap,"Active title movie has no authored target bitmap");
        if (DriveMovie(ui,*bitmap,*services_.movieLog)==1) {
            ++movieIndex;
            if (demo.ActivationWord())ui.render->SetScreenVisibility(*ui.resources,TitleKey,1);
            ui.movie->Close();
            ui.resources->SetObjectVisibility(TitleKey,TitleMovie,0);
            ui.resources->SetObjectVisibility(TitleKey,TitleAlternate,0);
            if (movieIndex==GetNumTitleMovies(demo)) {
                phase=1;startedTicks=ui.clock->realTimer;
                if (!demo.ActivationWord())StartTitleMusic(menu);
            } else OpenCurrentMovie();
        }
    } else if (phase==1) {
        if (demo.ActivationWord() && !demo.IsAttractMode())
            menu.frontend->StartDemoDiscGame(*menu.raceSetup);
        else menu.demoMode->ConsiderStartDemoMode(startedTicks);
        highlight_->Animate();
    }
    const auto noMovie=phase!=2;
    ui.resources->SetGroupVisibility(TitleKey,hashString("TITLE_IMAGE"),noMovie);
    if (noMovie && (!inputVisibleTicks || inputVisibleTicks==std::numeric_limits<std::int32_t>::max()))
        inputVisibleTicks=ui.clock->realTimer;
    ui.resources->SetObjectVisibility(TitleKey,TitlePrompt,CanAcceptInput());
}

void BindFrontendStartup(FrontendStartupServices& services)
{
    Require(!titleOwner && !mumboOwner,"Cannot rebind active title/legal screen owners");
    Require(services.menus && services.movieLog,"Frontend startup has no actual services/replay owner");
    const auto& menu=*services.menus;
    Require(menu.ui && menu.ui->resources && menu.ui->clock && menu.ui->movie &&
        menu.ui->language && menu.ui->render && menu.screens && menu.screens->ui==menu.ui &&
        menu.frontend && menu.raceSetup && menu.sounds && menu.demoMode && menu.demoMode->demoDisc &&
        menu.demoMode->screens==menu.screens && menu.demoMode->frontend==menu.frontend &&
        menu.demoMode->raceSetup==menu.raceSetup && menu.raceSetup->parameters &&
        menu.raceSetup->applicationOptions && menu.raceSetup->flow,
        "Frontend startup bindings do not share the actual live owners");
    startupServices=&services;
}

void UnbindFrontendStartup()
{
    titleOwner.reset();mumboOwner.reset();startupServices=nullptr;
}

void TitleScreenCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(startupServices && data,"Title callback has no actual binding/data");
    if (message==ScreenMessage::ScreenActivate) {
        Require(!titleOwner,"Title screen received duplicate activation");
        titleOwner=std::make_unique<TitleScreen>(*startupServices,data->key);
    } else if (message==ScreenMessage::ScreenDeactivate)titleOwner.reset();
    else {
        Require(titleOwner!=nullptr,"Title message arrived outside its lifetime");
        if (message==ScreenMessage::Tick)titleOwner->HandleTick();
        else titleOwner->HandleEvent(message,*data);
    }
}

void CallbackFinishedLegalScreen() {legalScreenLoaded=true;}
void CallbackFinishedAntiPiracyScreen() {antiPiracyScreenLoaded=true;}

MumboJumbo::MumboJumbo(FrontendStartupServices& services,std::uint32_t screen)
    : screenKey(screen),services_(services)
{
    bitmap=static_cast<FlailerBitmapChunk*>(services.menus->ui->resources->FindObject(screen,TitleMovie,2));
}

bool MumboJumbo::ShouldSeeLegalScreen() const
{
    // The complete PAL16F558 body returns zero. This policy is not an
    // invented success branch; its exact caller proves the clone identity.
    return false;
}

bool MumboJumbo::ShouldSeeAntiPiracy() const
{
    // Same final release policy at16F560, independently anchored by caller.
    return false;
}

void MumboJumbo::HandleEvent(ScreenMessage message,ScreenCallbackData&)
{
    if (message!=ScreenMessage::Tick)return;
    auto& menu=*services_.menus;auto& ui=*menu.ui;
    auto load=[&](std::uint32_t key,FrontendLegalTextureCompletion completion) {
        Require(services_.legalTextures,"Legal screen has no actual texture-file owner");
        services_.legalTextures->LoadTexturesFromFile(&key,1,completion);
    };
    auto unload=[&](std::uint32_t key) {
        Require(services_.legalTextures,"Legal screen has no actual texture-file owner");
        services_.legalTextures->UnloadTexturesFromFile(&key,1);
    };
    auto display=[&](std::uint32_t key) {
        // Exact FlailerChangeTexture144A28 target: the actual bitmap's
        // existing ChangeTexture resolves the same live texture catalogue.
        auto* object=static_cast<FlailerBitmapChunk*>(ui.resources->FindObject(screenKey,LegalImage,2));
        if (object)object->ChangeTexture(key);
        ui.resources->SetObjectVisibility(screenKey,LegalImage,1);startedTicks=ui.clock->realTimer;
    };
    switch (phase) {
    case 0:
        if (ShouldSeeLegalScreen()) {
            phase=1;legalScreenLoaded=false;load(LegalTexture,FrontendLegalTextureCompletion::Legal);
            ui.resources->SetObjectVisibility(screenKey,LegalImage,0);
        } else if (ShouldSeeAntiPiracy()) {
            phase=3;antiPiracyScreenLoaded=false;ui.resources->SetObjectVisibility(screenKey,LegalImage,0);
            load(AntiPiracyTexture,FrontendLegalTextureCompletion::AntiPiracy);
        } else menu.screens->MoveToPreviousScreen(1,false,-1);
        return;
    case 1:if (legalScreenLoaded) {phase=2;display(LegalTexture);}return;
    case 2:
        if (ElapsedTitleTicks(ui.clock->realTimer,startedTicks)>convertFloatToInt32(LegalDuration*4000.0f)) {
            unload(LegalTexture);
            if (ShouldSeeAntiPiracy()) {
                phase=3;antiPiracyScreenLoaded=false;load(AntiPiracyTexture,FrontendLegalTextureCompletion::AntiPiracy);
            } else menu.screens->MoveToPreviousScreen(1,false,-1);
        }
        return;
    case 3:if (antiPiracyScreenLoaded) {phase=4;display(AntiPiracyTexture);}return;
    case 4:
        if (ElapsedTitleTicks(ui.clock->realTimer,startedTicks)>convertFloatToInt32(AntiPiracyDuration*4000.0f)) {
            unload(AntiPiracyTexture);menu.screens->MoveToPreviousScreen(1,false,-1);
        }
        return;
    default:return;
    }
}

void MumboJumboCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(startupServices && data,"Legal callback has no actual binding/data");
    if (message==ScreenMessage::ScreenActivate) {
        Require(!mumboOwner,"Legal screen received duplicate activation");
        mumboOwner=std::make_unique<MumboJumbo>(*startupServices,data->key);
    } else if (message==ScreenMessage::ScreenDeactivate)mumboOwner.reset();
    else {
        Require(mumboOwner!=nullptr,"Legal message arrived outside its lifetime");
        mumboOwner->HandleEvent(message,*data);
    }
}

// Exact final static registration170260, including names, screen hashes,
// callback targets and no help list. Native storage supplies only lifetime.
FlailerScreenConfig fTitleScreen={"Title Screen",&TitleScreenCallback,
    {{static_cast<std::int32_t>(TitleKey),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};
FlailerScreenConfig fMumboJumboScreen={"Legal MumboJumbo",&MumboJumboCallback,
    {{static_cast<std::int32_t>(0xdd4197c9u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};

bool ShouldSeeAutoloadScreen(const ::DemoDiscManager& demo)
{
    return !demo.ActivationWord() && frontendLaunchType==1;
}

bool ShouldSeeLanguageSelectScreen(const FlailerUiState& ui,const ::DemoDiscManager& demo)
{
    Require(ui.language && ui.clock,"Language startup policy has no actual language/video owners");
    if (frontendLaunchType==1 && ui.language->currentLanguage==-1)return true;
    if (demo.ActivationWord())return false;
    return gameTimeGetVideoMode(ui.clock)==0 && frontendLaunchType==1;
}

bool ShouldSeeLegalMumboJumbo()
{
    // Actual final16F938 release policy, not a missing-screen workaround.
    return false;
}

bool ShouldSeeTitleSequence()
{
    return frontendLaunchType==1 || frontendLaunchType==2;
}

void LaunchFrontendMenus(FrontendStartupServices& services)
{
    auto& menu=*services.menus;auto& screens=*menu.screens;
    const auto logLaunch=[&](const char* destination) {
        char detail[224]{};
        std::snprintf(detail,sizeof(detail),"launch_type=%d game_type=%d stack=%zu destination=\"%s\"",
            frontendLaunchType,menu.frontend->gameType,static_cast<std::size_t>(screens.stack.count),destination?destination:"");
        RecordSessionEvent("frontend.launch",detail);
    };
    if (frontendLaunchType==2 && menu.frontend->pendingState==2) {
        screens.stack.count=0;
        logLaunch("New Main Menu");
        screens.ChangeToScreen("New Main Menu",true);return;
    }
    // ScreenStack::Print16B470 is JR RA/NOP in PAL, so no fake method is
    // emitted. The launch stack, including an already pending menu, is real.
    if (ShouldSeeTitleSequence())screens.stack.Push("Title Screen");
    if (ShouldSeeLegalMumboJumbo())screens.stack.Push("Legal MumboJumbo");
    if (ShouldSeeAutoloadScreen(*menu.demoMode->demoDisc))screens.stack.Push("BootAutoload");
    if (ShouldSeeLanguageSelectScreen(*menu.ui,*menu.demoMode->demoDisc))
        screens.stack.Push("Language Options Pre FE");
    const auto* current=screens.GetCurrentScreenNameFromStack();
    logLaunch(current);
    screens.PopScreens(1);
    if (std::strcmp(current,"View Event Tree")==0)menu.frontend->eventSelection.lastViewState=0;
    screens.ChangeToScreen(current,false);
}

void UnlaunchFrontendMenus(FrontendStartupServices& services)
{
    Require(services.menus && services.menus->screens && services.menus->frontend &&
        services.menus->raceSetup && services.menus->raceSetup->parameters && services.menus->sounds,
        "Frontend unload has no actual screen/FRED/race/audio owners");
    auto& menu=*services.menus;auto& screens=*menu.screens;
    // PAL16FA98..16FAAC reads FRED+1072C (30C77C), the retained game type.
    // The distinct pendingState at FRED+180 belongs to menu/demo navigation.
    // Reading it here left setup/detail screens on the return stack instead
    // of selecting the source per-mode menu destination.
    const auto gameType=menu.frontend->gameType;
    char detail[224]{};
    std::snprintf(detail,sizeof(detail),"game_type=%d stack=%zu active=\"%s\" return=\"%s\"",
        gameType,static_cast<std::size_t>(screens.stack.count),screens.active?screens.active->configuration->name:"",
        screens.GetCurrentScreenNameFromStack());
    RecordSessionEvent("frontend.retire",detail);
    // Source ScreenStack.Print and Help.Reset are complete JR RA/NOP bodies.
    // The real screen callbacks, film close and alpha destruction remain.
    if (gameType>=2 && gameType<=7)screens.PopUntil("New Main Menu");
    else if (gameType==8 || gameType==9)screens.PopUntil("View Event Tree");
    screens.PopScreens(0);
    screens.ChangeToScreen(nullptr,false);
    screens.SetMainScreen(nullptr);
    Require(soundJukeboxStop(*menu.sounds)==WORLD_AUDIO_OK,
        "Frontend unload could not stop the actual jukebox voice");
    const auto& parameters=*menu.raceSetup->parameters;
    // PAL RaceParameters+18/+1C: the existing AI-demo and recorded-demo
    // source owners; do not use the frontend game-type or replay flag copy.
    frontendLaunchType=parameters.fastSpeedTrapSpawns || parameters.demoReplay ? 2 : 3;
    screens.StopMovie();
    screens.newSongMonitor.Reset();
}

void FrontendDemoMode::BuildDemoTrackList()
{
    // PAL16EC10 caches the original one-based authored order, up to32.
    if (trackListBuilt)return;
    Require(raceSetup && raceSetup->tracks,"Demo track list has no actual track catalogue");
    if (applicationBootState.trackSelectedInConfigFile!=-1) {
        tracks[0]=applicationBootState.trackSelectedInConfigFile;
        trackCount=1;
    } else {
        trackCount=0;
        const auto count=raceSetup->tracks->Catalog().scenarioCount;
        for (std::size_t number=1;number<=count;++number) {
            auto* info=raceSetup->tracks->Find(static_cast<std::int32_t>(number));
            if (info && info->routeKind==0 && static_cast<std::size_t>(trackCount)<tracks.size())
                tracks[static_cast<std::size_t>(trackCount++)]=static_cast<std::int32_t>(number);
        }
    }
    trackListBuilt=true;
}

void FrontendDemoMode::StartDemoMode()
{
    Require(screens && frontend && raceSetup && demoDisc,"Demo start has no actual frontend owners");
    screens->StopMovie();
    BuildDemoTrackList();
    Require(trackCount>0,"Demo track catalogue contains no actual eligible track");
    const auto selected=trackSequence%trackCount;
    Require(selected>=0,"Demo track sequence wrapped into a negative source index");
    const auto next=static_cast<std::uint32_t>(trackSequence)+7u;
    std::memcpy(&trackSequence,&next,sizeof(next));
    const auto track=demoDisc->ActivationWord() ? 12 : tracks[static_cast<std::size_t>(selected)];
    frontend->StartDemoMode(track,*raceSetup);
}

void FrontendDemoMode::ConsiderStartDemoMode(std::int32_t idleStartedTicks)
{
    Require(screens && screens->ui && screens->ui->clock && raceSetup &&
        raceSetup->applicationOptions && raceSetup->demoReplay && demoDisc && files,
        "Demo consideration has no actual frontend/clock/replay/resource owners");
    const auto raw=static_cast<std::uint32_t>(screens->ui->clock->realTimer)-
        static_cast<std::uint32_t>(idleStartedTicks);
    std::int32_t elapsed;std::memcpy(&elapsed,&raw,sizeof(elapsed));
    bool start=raceSetup->applicationOptions->beginDemoSeconds<
        static_cast<float>(elapsed)*0.0002500000118743628f;
    if (demoDisc->ActivationWord() && demoDisc->IsAttractMode())start=true;
    if (!start)return;
    auto& replay=*raceSetup->demoReplay;
    if (replay.IsLoadPending())return;
    if (!demoDisc->ActivationWord() && replay.forcedRecording<0) {
        // Native PC attract mode uses the original live AI driver/session.
        // Explicit recordings and demo-disc selection keep their source path;
        // never replace the controls or snapshots of an active recording.
        if (replay.active || replay.IsReplayDataBorrowed())return;
        if (replay.HasLoadedResource())replay.UnloadResourceFile();
        StartDemoMode();
        return;
    }
    if (replay.HasLoadedResource()) {
        replay.Launch();
        StartDemoMode();
    } else if (!replay.ConsiderLaunching(*files,*screens->ui->clock,*demoDisc)) {
        StartDemoMode();
    }
}

std::int32_t DriveMovie(FlailerUiState& ui,FlailerBitmapChunk& bitmap,ReplayIntegerLog& log)
{
    // PAL16EE18: decoding updates the actual texture before the result is
    // replaced by recorded channel6 data. Never mistake replay for upload.
    std::int32_t finished=0;
    auto* texture=textureInfoForDescriptor(bitmap.texture);
    if (ui.movie && ui.movie->RequestedMovie()) {
        finished=ui.movie->Update(ui.clock->realTimeElapsed,nullptr,0)^1;
    } else if (texture && texture->pixels.Data()) {
        Require(ui.movie && ui.clock,"DriveMovie has no actual player or clock owner");
        finished=ui.movie->Update(ui.clock->realTimeElapsed,
            texture->pixels.MutableData(),texture->pixels.size)^1;
    }
    if (log.replaying)finished=static_cast<std::int32_t>(replayIntegerLogRead(&log,4,6));
    else replayIntegerLogWrite(&log,static_cast<std::uint32_t>(finished),4,6);
    return finished;
}

FlailerLoadingScreenAnimation::FlailerLoadingScreenAnimation(FlailerUiState& ui,
    const ApplicationFrontendFlow& flow,const FrontendPlayerManager& frontend,
    const ::DemoDiscManager& demo,std::uint32_t screen)
    : ui_(ui),flow_(flow),frontend_(frontend),screenKey_(screen)
{
    Require(ui_.resources && ui_.render && ui_.clock,
        "Loading screen has no actual UI/resource/render/clock owners");
    if (demo.ActivationWord())screenKey_=hashString("DEMO_CONTROLS");
    highlight_=std::make_unique<FlailerHighlightAnimation>(ui_,screenKey_,
        hashString("GLOBAL_ANIMATING_MENUITEM"),ui_.animationSettings.highlightColor);
    highlight_->SetCurrentMenuItem(hashString("GLOBAL_LOADING_STRING"));
    highlight_->restartDuration=2.0f;
    gearNotice_=static_cast<FlailerTextChunk*>(ui_.resources->FindObject(screenKey_,0x120e827cu,1));
}

FlailerLoadingScreenAnimation::~FlailerLoadingScreenAnimation()=default;

void FlailerLoadingScreenAnimation::Tick()
{
    // PAL16FCF4 reads InGameFlag330540, not desired flow330548.
    if (flow_.currentState==3 || flow_.inGameFlag)return;
    if (ui_.render->IsScreenVisible(screenKey_))highlight_->Animate();
    // Source writes through this object before its later nullable test.
    // A missing authored notice is an invalid native binding, not success.
    Require(gearNotice_,"Active loading screen has no actual gear notice text");
    gearNotice_->visible=0;gearNotice_->DeleteFlailerStrip();
    if (frontend_.gameType!=2 || (flow_.currentState!=4 && flow_.currentState!=5))return;
    Require(frontend_.players[0],"Loading screen has no actual first frontend profile");
    if (frontend_.players[0]->settings.gearInputMode==1)gearNotice_->visible=1;
}

} // namespace hp2

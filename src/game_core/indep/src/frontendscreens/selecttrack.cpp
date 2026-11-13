#include "Fred.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include "FlailerDB.hpp"
#include "FMenus.hpp"
#include "bware/inc/bMath.hpp"
#include "EasterEggs.hpp"
#include "indep/inc/Timer.hpp"
#include "Language.hpp"
#include "FEAnim.hpp"
#include "Texture.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/src/speech/mgr.hpp"

#include <cstdio>
#include <cmath>
#include <limits>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace hp2 {

TrackPresentationSettings trackPresentationSettings;

struct SelectTrackScreen::Storage {
    SelectTrackScreenServices* services{};
    tCubic1D horizontal{};
    tCubic1D vertical{};
    SelectableTrackManager manager;
    std::unique_ptr<SelectTrackSlideshow> slideshow;
    std::unique_ptr<TrackTimeOfDayMenu> timeMenu;
    std::unique_ptr<TrackRecordDisplayer> record;
    std::unique_ptr<TrackLockedDisplayer> locked;
    std::unique_ptr<FlailerHighlightAnimation> highlight;
    std::unique_ptr<FrontendStillCamera> camera;
    std::array<FlailerBitmapChunk*,6> bitmaps{};
    float phase{};
    std::int32_t previewAudioState{};
    std::exception_ptr shutdownFailure;
    bool shutdownStarted{};
};

namespace {
SelectTrackScreenServices* trackSelectServices{};
std::unique_ptr<SelectTrackScreen> trackSelectScreen;

void FadeTrackPreview()
{
    auto* speech=worldSpeechGetInstance();
    Require(speech,"Track preview could not allocate the actual speech manager");
    const auto result=worldSpeechFade(speech,1.0f);
    Require(result==WORLD_SPEECH_OK || result==WORLD_SPEECH_SUPPRESSED,
        "Track preview fade has an invalid live audio binding");
}


}

std::string FormatTrackDistanceForDisplay(float miles,bool metric,const char* route)
{
    // PAL SelectTrackScreen::ResyncScreen keeps the authored float and
    // 1.6093000173568726f conversion. Its custom vsprintf at 0x251440 gives
    // %f three fractional digits, unlike the host CRT's default precision.
    // Preserve that display precision without changing the track data.
    Require(route && std::isfinite(miles) && miles>=0.0f,
        "Track length label needs a finite authored distance and route");
    const float converted=metric ? miles*1.6093000173568726f : miles;
    Require(std::isfinite(converted) && converted<1.0e7f,
        "Track length exceeds its PC display precision range");
    // Round the actual converted binary32 value. A value below a decimal
    // midpoint must stay below it; an epsilon would round such inputs up.
    const double thousandths=std::floor(static_cast<double>(converted)*1000.0+0.5);
    const double display=thousandths/1000.0;
    char caption[128];
    const auto written=std::snprintf(caption,sizeof(caption),"%.3f %s %s",
        display,metric?"km":"mi",route);
    Require(written>=0 && static_cast<std::size_t>(written)<sizeof(caption),
        "Track caption exceeds Flailer text capacity");
    return caption;
}

std::uint32_t FigureOutTrackNameImageHash(std::int32_t track,std::int32_t direction,
    const LanguageManager& language)
{
    // PAL19FB00 hashes the width-two decimal name, then continues that same
    // checksum with each selected writable suffix;2534E8 is not uppercase.
    char name[32];
    const auto count=std::snprintf(name,sizeof(name),"LOGOTRACK%2d",track);
    Require(count>=0 && static_cast<std::size_t>(count)<sizeof(name),"Track logo exceeds its native scratch buffer");
    auto key=hashString(name);
    if (direction==1)key=hashStringContinue(trackPresentationSettings.reverseLogoSuffix.c_str(),key);
    if (language.currentLanguage==1)key=hashStringContinue(trackPresentationSettings.frenchLogoSuffix.c_str(),key);
    return key;
}

SelectTrackScreen::SelectTrackScreen()=default;

SelectTrackScreen::SelectTrackScreen(SelectTrackScreenServices& services,
    std::uint32_t screen,ScreenCallbackData& callback)
    : frontend(services.frontend),ui(services.ui),screens(services.screens),screenKey(screen),
      storage_(std::make_unique<Storage>())
{
    Require(ui && ui->resources && ui->language && ui->clock && frontend && services.resources &&
        services.tracks && services.events && services.cameras && services.screens && services.raceSetup &&
        services.raceSetup->tracks==services.tracks && services.raceSetup->events==services.events &&
        services.raceSetup->language==ui->language && services.cameras->clock==ui->clock,
        "Track screen requires its actual frontend/resource/camera owners");
    auto& owner=*storage_;owner.services=&services;
    for (auto* curve:{&owner.horizontal,&owner.vertical})tCubic1DInitialize(curve);
    tracks=&owner.manager;
    owner.slideshow=std::make_unique<SelectTrackSlideshow>(*ui,*services.resources,screenKey);
    owner.timeMenu=std::make_unique<TrackTimeOfDayMenu>(*ui,screens,screenKey);
    owner.record=std::make_unique<TrackRecordDisplayer>(*ui,*frontend,screenKey);
    owner.locked=std::make_unique<TrackLockedDisplayer>(*ui,*services.events,screenKey);
    owner.highlight=std::make_unique<FlailerHighlightAnimation>(*ui,screenKey,0xab671c9au,
        ui->animationSettings.highlightColor);
    highlight=owner.highlight.get();
    data=frontend->trackSelection;
    constexpr std::array<std::uint32_t,6> keys{{0xd1cda594u,0x9345e4a7u,
        0xedf2e4eeu,0x20d73d01u,0x3b89e410u,0x3489f2a3u}};
    for (std::size_t index=0;index<keys.size();++index)
        owner.bitmaps[index]=static_cast<FlailerBitmapChunk*>(ui->resources->FindObject(screenKey,keys[index],2));
    StopLapSelector();
    for (auto* curve:{&owner.horizontal,&owner.vertical}) {
        curve->continuous=0;curve->duration=0.5f;
    }
    auto* spacingObject=ui->resources->FindObject(screenKey,0xfe9485f7u,2);
    Require(spacingObject,"Track thumbnail spacing bitmap is absent from the authored screen");
    const auto spacing=convertFloatToInt32(ui->resources->ScaledBitmapWidth(
        *static_cast<FlailerBitmapChunk*>(spacingObject)));
    tracks->BuildList(*frontend,*services.tracks,*services.events,*ui->resources,screenKey,
        static_cast<float>(spacing));
    tracks->SetCurrent(*services.tracks,data.baseTrackNumber,data.direction,data.timeOfDay);
    ResyncData();
    owner.horizontal.desiredValue=tracks->GetDesiredHorizThumbnailOffset();
    if (owner.horizontal.value!=owner.horizontal.desiredValue)owner.horizontal.mode=2;
    owner.horizontal.value=owner.horizontal.desiredValue;owner.horizontal.mode=0;
    owner.horizontal.derivative=owner.horizontal.desiredDerivative;
    owner.vertical.desiredValue=130.0f;
    if (owner.vertical.value!=owner.vertical.desiredValue)owner.vertical.mode=2;
    owner.vertical.mode=0;owner.vertical.value=owner.vertical.desiredValue;
    owner.vertical.derivative=owner.vertical.desiredDerivative;
    callback.value=ResyncScreen();
    tracks->DrawThumbnails(*ui->resources,screenKey,tracks->GetDesiredHorizThumbnailOffset(),130.0f);
    owner.phase=0.0f;
    owner.camera=std::make_unique<FrontendStillCamera>(*services.cameras,1);
}

SelectTrackScreen::~SelectTrackScreen()
{
    if (const auto failure=Shutdown()) {
        try {std::rethrow_exception(failure);}
        catch (const std::exception& error) {RecordSessionEvent("track_screen.cleanup_failed",error.what());}
        catch (...) {RecordSessionEvent("track_screen.cleanup_failed","Unknown track-screen cleanup failure");}
    }
}

std::exception_ptr SelectTrackScreen::Shutdown() noexcept
{
    // Default-constructed fixtures have borrowed leaf bindings only.
    if (!storage_)return {};
    auto& owner=*storage_;
    if (owner.shutdownStarted)return owner.shutdownFailure;
    owner.shutdownStarted=true;
    frontend->trackSelection=data;
    if (owner.services->frame)owner.services->frame->Clear();
    owner.camera.reset();owner.highlight.reset();owner.locked.reset();owner.record.reset();
    if (owner.slideshow)owner.shutdownFailure=owner.slideshow->Shutdown();
    owner.slideshow.reset();owner.timeMenu.reset();owner.manager.ownedTracks.clear();
    tracks=nullptr;highlight=nullptr;
    return owner.shutdownFailure;
}

FrontendStillCamera* SelectTrackScreen::Camera() noexcept
{
    return storage_ ? storage_->camera.get() : nullptr;
}
const tCubic1D* SelectTrackScreen::HorizontalMotion() const noexcept
{
    return storage_ ? &storage_->horizontal : nullptr;
}
const tCubic1D* SelectTrackScreen::VerticalMotion() const noexcept
{
    return storage_ ? &storage_->vertical : nullptr;
}
float SelectTrackScreen::PulsePhase() const noexcept
{
    return storage_ ? storage_->phase : 0.0f;
}

std::int32_t SelectTrackScreen::ResyncScreen()
{
    Require(storage_ && tracks && tracks->current && ui && ui->resources && ui->language && frontend,
        "Track screen synchronization has no actual selection/UI owner");
    auto& owner=*storage_;
    const auto& current=*tracks->current;
    Require(current.baseInfo && current.currentTime && current.currentTime->info,
        "Track screen has no selected authored variant");
    const auto& base=*current.baseInfo;const auto& time=*current.currentTime->info;
    const auto* elevationFormat=ui->language->GetString(0xd6eb);
    const auto* route=ui->language->GetString(base.routeKind ? 0xd995 : 0xd994);
    Require(elevationFormat && route,"Track screen language resource lacks a required caption");
    auto& resources=*ui->resources;
    const bool metric=frontend->displaySettings.languageDependentOption!=0;
    const auto length=FormatTrackDistanceForDisplay(base.lengthMiles,metric,route);
    resources.Printf(screenKey,0x79e8a895u,"%s",length.c_str());
    const float first=metric?time.elevationFeet[0]*0.30000001192092896f:time.elevationFeet[0];
    const float second=metric?time.elevationFeet[1]*0.30000001192092896f:time.elevationFeet[1];
    const auto elevation=FormatLocalizedText(elevationFormat,
        {static_cast<double>(first),static_cast<double>(second),metric?"m":"ft"});
    resources.SetLocalizedText(screenKey,0x20a05a3au,elevation.c_str());
    Require(time.difficulty>=0 && static_cast<std::size_t>(time.difficulty)<trackPresentationSettings.difficultyLanguage.size(),
        "Track difficulty is outside the authored language table");
    resources.SetLanguageID(screenKey,0x0145f386u,
        trackPresentationSettings.difficultyLanguage[static_cast<std::size_t>(time.difficulty)]);
    resources.SetLanguageID(screenKey,0xf601f2d4u,time.nameLanguageIds[current.direction ? 1u : 0u]);
    owner.horizontal.desiredValue=tracks->GetDesiredHorizThumbnailOffset();
    if (owner.horizontal.desiredValue!=owner.horizontal.value)owner.horizontal.mode=2;
    const auto multipleTimes=current.ownedTimes.size()>=2;
    for (auto key:trackPresentationSettings.timeMenuObjects)
        resources.SetObjectVisibility(screenKey,key,multipleTimes ? 1 : 0);
    if (multipleTimes) {
        if (auto* object=resources.FindObject(screenKey,0x20d73d01u))
            object->SetTopLeftY(static_cast<float>(convertFloatToInt32(current.timeMenuHeight+130.0f)));
    }
    owner.slideshow->SetCurrentTrack(tracks->current);
    const auto selected=owner.timeMenu->SetSelectableTrack(tracks->current);
    owner.record->SetCurrentTrack(tracks->current);
    owner.locked->SetCurrentTrack(tracks->current);
    const auto logo=FigureOutTrackNameImageHash(base.number,current.direction,*ui->language);
    if (auto* object=resources.FindObject(screenKey,0xf60114e4u,2))
        static_cast<FlailerBitmapChunk*>(object)->ChangeTexture(logo);
    resources.SetObjectVisibility(screenKey,0x84f544f3u,tracks->AtLeftEdge() ? 0 : 1);
    resources.SetObjectVisibility(screenKey,0xabb370a6u,tracks->AtRightEdge() ? 0 : 1);
    return selected;
}

void SelectTrackScreen::DoTick()
{
    Require(storage_ && storage_->camera && ui && ui->clock && ui->resources && tracks,
        "Track screen tick has no actual camera/clock/UI owners");
    auto& owner=*storage_;
    tracks->DrawThumbnails(*ui->resources,screenKey,owner.horizontal.value,owner.vertical.value);
    tCubic1DUpdate(&owner.horizontal,ui->clock->realTimeElapsed,0.0f,0.0f);
    tCubic1DUpdate(&owner.vertical,ui->clock->realTimeElapsed,0.0f,0.0f);
    constexpr float fullTurn=6.283185005187988f;
    owner.phase=owner.phase+trackPresentationSettings.pulseSpeed*ui->clock->realTimeElapsed;
    if (fullTurn<owner.phase)owner.phase=owner.phase-fullTurn;
    const auto alpha=convertFloatToInt32((rotationSineRadians(owner.phase)+1.0f)*64.0f);
    for (std::size_t index=4;index<6;++index)
        if (owner.bitmaps[index])owner.bitmaps[index]->SetAlpha(alpha,-1);
    owner.camera->eye=trackPresentationSettings.cameraEye;
    owner.camera->target=trackPresentationSettings.cameraTarget;
    trackPresentationSettings.modelAngle=trackPresentationSettings.modelAngle+trackPresentationSettings.modelAngleStep;
    if (fullTurn<trackPresentationSettings.modelAngle)
        trackPresentationSettings.modelAngle=trackPresentationSettings.modelAngle-fullTurn;
    owner.slideshow->Update();owner.record->Update();owner.locked->Update();highlight->Animate();
}

void FrontendFrame::Clear() noexcept
{
    if (textRenderer)textRenderer->CompleteSubmissions();
    textRenderer=nullptr;
    models.clear();
    modelDrawCpuClosureRequired=false;
}

void SelectTrackScreen::DoRender()
{
    Require(storage_ && storage_->services->frame && tracks && tracks->current,
        "Track screen rendering has no actual frontend frame/selection owner");
    auto& frame=*storage_->services->frame;
    const auto& model=tracks->current->model;
    if (model.solid) {
        FrontendModelRequest request;request.model=&model;request.viewNumber=1;
        rotationMatrixIdentity(request.transform.data());
        request.transform[0]*=trackPresentationSettings.modelScale;
        request.transform[5]*=trackPresentationSettings.modelScale;
        request.transform[10]*=trackPresentationSettings.modelScale;
        const auto angle=static_cast<std::uint16_t>(convertFloatToInt32(
            trackPresentationSettings.modelAngle*10430.3779296875f));
        motionMatrixRotateYaw(request.transform.data(),request.transform.data(),angle);
        frame.models.push_back(request);
        // This is the real model/matrix input to eModelDraw. Its CPU cull,
        // LOD and light selection remain an explicit unported dependency.
        frame.modelDrawCpuClosureRequired=true;
    }
    if (tracks->current->availability) {
        Require(ui->render,"Locked track text has no actual Flailer render manager");
        Require(!frame.textRenderer || frame.textRenderer==ui->render,
            "Frontend frame cannot mix Flailer render owners");
        frame.textRenderer=ui->render;
        ui->render->RenderTextIntoRectangle(*ui->resources,screenKey,0x2a67e2d5u,
            0x2a65daf9u,storage_->locked->message.data(),1.0f);
    }
}

void SelectTrackScreen::HandleLapSelection(ScreenMessage message,ScreenCallbackData& callback)
{
    Require(storage_ && storage_->services->raceSetup,"Track lap input has no actual race setup binding");
    switch (message) {
    case ScreenMessage::Tick:DoTick();return;
    case ScreenMessage::LeaveScreen:
        StopLapSelector();callback.value=0;callback.sound=6;ResyncScreen();
        Require(screens && screens->active,"Track lap exit has no active screen node");
        highlight->SetCurrentMenuItem(screens->active->GetActiveMenuItem());return;
    case ScreenMessage::Left:
        callback.sound=1;data.lapCount=data.lapCount/2;
        if (data.lapCount<2)data.lapCount=8;
        ui->resources->Printf(screenKey,0x7e544804u,"%d",data.lapCount);
        highlight->SetCurrentMenuItem(0x7e544804u);return;
    case ScreenMessage::Right: {
        callback.sound=2;
        const auto doubled=static_cast<std::uint32_t>(data.lapCount)<<1;
        std::memcpy(&data.lapCount,&doubled,sizeof(doubled));
        if (data.lapCount>=9)data.lapCount=2;
        ui->resources->Printf(screenKey,0x7e544804u,"%d",data.lapCount);
        highlight->SetCurrentMenuItem(0x7e544804u);return;
    }
    case ScreenMessage::Previous:case ScreenMessage::Next:
        callback.sound=0;callback.value=1;return;
    case ScreenMessage::Select:case ScreenMessage::ScreenBack:
        if (!PassJoystickTest(callback.joystickPort))callback.sound=0;
        else StartRace(*storage_->services->raceSetup);
        return;
    default:return;
    }
}

void SelectTrackScreen::ScreenCallback(ScreenMessage message,ScreenCallbackData& callback)
{
    Require(storage_ && storage_->services->raceSetup && tracks && tracks->current,
        "Track screen input has no actual selection/race setup owner");
    if (!selectionState) {HandleLapSelection(message,callback);return;}
    switch (message) {
    case ScreenMessage::Tick:DoTick();return;
    case ScreenMessage::RenderModels:DoRender();return;
    case ScreenMessage::ScreenEvent13:
        // PAL16FDB0 is exactly JR RA; MOVE V0,ZERO. This source build never
        // executes the guarded availability write; no guessed input flag.
        return;
    case ScreenMessage::Highlight:highlight->SetCurrentMenuItem(callback.key);return;
    case ScreenMessage::Previous:case ScreenMessage::Next:
        if (tracks->ScrollTimeOfDay(message==ScreenMessage::Previous ? -1 : 1)) {
            ResyncData();ResyncScreen();callback.sound=message==ScreenMessage::Previous ? 11 : 10;
            highlight->SetCurrentMenuItem(callback.key);
        } else {callback.sound=0;callback.value=1;}
        return;
    case ScreenMessage::Left:case ScreenMessage::Right:
        if (tracks->ScrollSelectedTrack(message==ScreenMessage::Left ? -1 : 1)) {
            ResyncData();ResyncScreen();callback.sound=message==ScreenMessage::Left ? 11 : 10;
            FadeTrackPreview();storage_->previewAudioState=0;
        }
        return;
    case ScreenMessage::Select:
        if (!PassJoystickTest(callback.joystickPort) || tracks->current->availability)callback.sound=0;
        else if (CanSelectLaps(*tracks->current->baseInfo))StartLapSelector();
        else StartRace(*storage_->services->raceSetup);
        return;
    case ScreenMessage::ScreenBack:callback.sound=0;return;
    case ScreenMessage::ItemEvent:
        if (tracks->current->availability)return;
        storage_->previewAudioState=!storage_->previewAudioState;
        if (storage_->previewAudioState) {
            auto* speech=worldSpeechGetInstance();
            Require(speech && speech->messages,"Track preview has no canonical audio message owner");
            const auto description=static_cast<std::uint32_t>(worldSpeechConvertTrackNumberToDescription(
                tracks->current->baseInfo->number));
            Require(worldAudioMessagesEnqueue(speech->messages,0x9bu,&description,1)==WORLD_AUDIO_MESSAGES_OK,
                "Track preview audio message queue has no capacity");
        } else FadeTrackPreview();
        return;
    default:return;
    }
}

void BindTrackSelectScreen(SelectTrackScreenServices& services)
{
    Require(!trackSelectScreen,"Cannot replace live track-screen services");
    trackSelectServices=&services;
}
void UnbindTrackSelectScreen()
{
    // Failed deactivation cannot leave static model/texture borrowers alive
    // beyond the frontend providers. Preserve the error after actual release.
    const auto failure=trackSelectScreen ? trackSelectScreen->Shutdown() : std::exception_ptr{};
    trackSelectScreen.reset();trackSelectServices=nullptr;
    if (failure)std::rethrow_exception(failure);
}
SelectTrackScreen* CurrentTrackSelectScreen() noexcept {return trackSelectScreen.get();}
FlailerScreenConfig& TrackSelectScreenConfiguration()
{
    static FlailerScreenConfig configuration{"Select Track",TrackSelectScreenCallback,
        {{static_cast<std::int32_t>(0x15417b53u),0,0,0}},{{-1,-1,-1,-1}},1,
        trackPresentationSettings.helpItems.data()};
    return configuration;
}
void TrackSelectScreenCallback(ScreenMessage message,ScreenCallbackData* callback)
{
    if (message==ScreenMessage::ScreenActivate) {
        Require(callback && trackSelectServices && !trackSelectScreen,
            "Track screen activation has no bound frontend owner or is already active");
        trackSelectScreen=std::make_unique<SelectTrackScreen>(*trackSelectServices,callback->key,*callback);
    } else if (message==ScreenMessage::ScreenDeactivate) {
        auto failure=trackSelectScreen ? trackSelectScreen->Shutdown() : std::exception_ptr{};
        trackSelectScreen.reset();
        try {FadeTrackPreview();}catch (...) {if (!failure)failure=std::current_exception();}
        if (failure)std::rethrow_exception(failure);
    } else if (trackSelectScreen) {
        Require(callback,"Track screen callback has no message data");
        trackSelectScreen->ScreenCallback(message,*callback);
    }
}

void SelectTrackSlideshow::TextureLoadedCallback()
{
    loadCompleted=1;
}

namespace {
constexpr std::array<std::uint32_t,5> TimeOfDayIcons{{
    0x132521f5u,0xa05ac96cu,0xdaa55f27u,0x72da223fu,0xdaa55f27u}};
constexpr std::array<std::uint32_t,5> TimeOfDayLanguage{{
    0xd6e0u,0xd6e1u,0xd6e2u,0xd6e3u,0xd70au}};

enum class TrackRaceSetup { LapRace,Pursuit,Knockout,SingleRace,MultiplayerLaps };
// Semantic targets of the final writable five-entry table2B5690. This is
// the one live native dispatch table, rather than retained PS2 code pointers.
std::array<TrackRaceSetup,5> trackRaceSetups{{TrackRaceSetup::LapRace,
    TrackRaceSetup::Pursuit,TrackRaceSetup::Knockout,TrackRaceSetup::SingleRace,
    TrackRaceSetup::MultiplayerLaps}};

std::uint32_t FormattedKey(const char* format,std::int32_t value)
{
    char buffer[32];
    const auto length=std::snprintf(buffer,sizeof(buffer),format,value);
    Require(length>=0 && static_cast<std::size_t>(length)<sizeof(buffer),"Track UI key exceeds its native scratch buffer");
    return hashString(buffer);
}
}

TimeOfDayTrack::TimeOfDayTrack(float offset,const TrackInfo* track)
    : info(track),thumbnailOffset(offset)
{
}

std::uint32_t SelectableTrack::PickTrackTextureForSlideshow(::hp2::TexturePack& pack) const
{
    Require(baseInfo && currentTime && currentTime->info,"Slideshow track has no actual route/time record");
    const auto variant=vectorDefaultRandomBounded(3);
    const auto time=currentTime->info->timeOfDay;
    // Final authored Fall/Alpine/Autumn variants have time4 and are
    // selectable. PAL indexes unwritten output scratch instead of a fifth
    // suffix pointer. This checked Windows boundary has no exactness claim
    // for that undefined domain; the original RNG call above still occurs.
    if (time<0 || time>=4)return 0;
    const auto& settings=trackPresentationSettings;
    char name[32];
    const auto size=std::snprintf(name,sizeof(name),"TRACK%d%s%s%s",baseInfo->number,
        settings.directionPrefixes[direction==1 ? 1u : 0u].c_str(),
        settings.pictureVariants[variant].c_str(),settings.pictureTimeSuffixes[static_cast<std::size_t>(time)].c_str());
    Require(size>=0 && static_cast<std::size_t>(size)<sizeof(name),"Track picture name exceeds source scratch capacity");
    const auto key=hashString(name);
    return pack.GetTexture(key) ? key : 0;
}

SelectTrackSlideshow::SelectTrackSlideshow(FlailerUiState& state,const GameResources& resources,std::uint32_t screen)
    : ui(&state),screenKey(screen),
      currentAlpha(std::make_unique<FlailerAlphaAnimation>(state,screen)),
      nextAlpha(std::make_unique<FlailerAlphaAnimation>(state,screen))
{
    Require(ui->resources && ui->clock,"Slideshow has no actual screen/clock owners");
    texturePack=std::make_unique<::hp2::TexturePack>(::hp2::texturePackRegistry,
        resources,ui->resources->Models(),"TrackSelect");
    Require(texturePack->LoadTextureTableFromFile("FRONTEND\\TSELECT.BIN"),"The actual track-select texture table could not be read");
    currentAlpha->AddObject(0x883beb15u);nextAlpha->AddObject(0x886bfd7du);
}

SelectTrackSlideshow::~SelectTrackSlideshow()
{
    if (const auto failure=Shutdown()) {
        try {std::rethrow_exception(failure);}
        catch (const std::exception& error) {RecordSessionEvent("track_slideshow.cleanup_failed",error.what());}
        catch (...) {RecordSessionEvent("track_slideshow.cleanup_failed","Unknown slideshow cleanup failure");}
    }
}

std::exception_ptr SelectTrackSlideshow::Shutdown() noexcept
{
    if (shutdownStarted_)return shutdownFailure_;
    shutdownStarted_=true;
    if (texturePack)texturePack->BeginShutdown();
    // A Select Track screen can be revisited after its slideshow table was
    // unloaded. Its Flailer bitmaps cache pointers into that table; retaining
    // those descriptors past their TexturePack owner produces an invalid
    // draw at the next Gather (even when the old bitmap is transparent).
    // PAL140300 retires the strips AND the cached descriptor binding for
    // every bitmap on the screen. Do that before releasing the table, while
    // its owner remains live, rather than masking a missing texture in Vulkan.
    try {
        if(ui && ui->resources) {
            if(auto* screen=ui->resources->FindScreen(screenKey))
                screen->DeleteAllFlailerStrips();
        }
    } catch (...) {shutdownFailure_=std::current_exception();}
    if (texturePack) {
        const auto failure=texturePack->Shutdown();
        if (!shutdownFailure_)shutdownFailure_=failure;
    }
    texturePack.reset();nextAlpha.reset();currentAlpha.reset();currentTrack=nullptr;
    return shutdownFailure_;
}

void SelectTrackSlideshow::MaybeTriggerLoad()
{
    Require(currentTrack,"Slideshow has no current track");
    for (std::int32_t attempt=0;attempt<10;++attempt) {
        const auto key=currentTrack->PickTrackTextureForSlideshow(*texturePack);
        if (!key || key==pendingTextureKey)continue;
        loadCompleted=0;
        texturePack->LoadTexturesFromFile(&key,1,this);
        pendingTextureKey=key;return;
    }
    if (currentTrack->currentTime && currentTrack->currentTime->info &&
        (currentTrack->currentTime->info->timeOfDay<0 || currentTrack->currentTime->info->timeOfDay>=4)) {
        // Do not publish an old queued picture as a new optional image in
        // the undefined PAL domain. The actual TRACK%d base bitmap remains.
        pendingTextureKey=0;loadCompleted=0;transitionState=0;
    }
}

void SelectTrackSlideshow::SetCurrentTrack(SelectableTrack* track)
{
    if (track==currentTrack)return;
    Require(track && track->baseInfo,"Slideshow selection has no actual base track");
    currentTrack=track;
    if (auto* image=ui->resources->FindObject(screenKey,0x883beb15u,2))
        static_cast<FlailerBitmapChunk*>(image)->ChangeTexture(FormattedKey("TRACK%d",track->baseInfo->number));
    currentAlpha->SetAlphaScale(1.0f);nextAlpha->SetAlphaScale(0.0f);
    startedTicks=ui->clock->realTimer;transitionState=0;
    texturePack->UnloadAllTexturesFromFile();MaybeTriggerLoad();
}

void SelectTrackSlideshow::Update()
{
    const auto now=ui->clock->realTimer;
    auto difference=static_cast<std::uint32_t>(now)-static_cast<std::uint32_t>(startedTicks);
    std::int32_t ticks;std::memcpy(&ticks,&difference,sizeof(ticks));
    const float elapsed=static_cast<float>(ticks)*0.0002500000118743628f;
    if (!transitionState && 4.0f<elapsed && loadCompleted) {
        transitionTicks=now;transitionState=1;
    }
    if (transitionState!=1)return;
    difference=static_cast<std::uint32_t>(ui->clock->realTimer)-static_cast<std::uint32_t>(transitionTicks);
    std::memcpy(&ticks,&difference,sizeof(ticks));
    const float fade=(static_cast<float>(ticks)*0.0002500000118743628f)*0.5f;
    if (fade<1.0f) {
        if (auto* image=ui->resources->FindObject(screenKey,0x886bfd7du,2))
            static_cast<FlailerBitmapChunk*>(image)->ChangeTexture(pendingTextureKey);
        currentAlpha->SetAlphaScale(1.0f-fade);nextAlpha->SetAlphaScale(fade);
        ui->resources->SetObjectVisibility(screenKey,0x886bfd7du,1);
    } else {
        if (auto* image=ui->resources->FindObject(screenKey,0x883beb15u,2))
            static_cast<FlailerBitmapChunk*>(image)->ChangeTexture(pendingTextureKey);
        currentAlpha->SetAlphaScale(1.0f);
        ui->resources->SetObjectVisibility(screenKey,0x886bfd7du,0);
        startedTicks=ui->clock->realTimer;transitionState=0;MaybeTriggerLoad();
    }
}

SelectableTrack::SelectableTrack(const TrackInfoResources& tracks,EditorMarkerResources& models,
    const TrackInfo& base,std::int32_t reverse,std::int32_t status,
    std::uint32_t thumbnail,float offset)
    : direction(reverse),horizontalThumbnail(offset),baseInfo(&base),
      availability(status),thumbnailKey(thumbnail)
{
    timeRoot.next=timeRoot.previous=&timeRoot;
    float nextOffset=0.0f;
    for (const auto& track:tracks.Records()) {
        if (!track.name[0] || track.baseNumber!=base.number) continue;
        auto time=std::make_unique<TimeOfDayTrack>(nextOffset,&track);
        nextOffset+=50.0f;
        time->previous=timeRoot.previous;time->next=&timeRoot;
        time->previous->next=time.get();timeRoot.previous=time.get();
        ownedTimes.push_back(std::move(time));
    }
    timeMenuHeight=nextOffset;
    SetTimeOfDay(0);
    editorMarkerModelInitialize(&model,&models,FormattedKey("TRACK%d_MINI",base.number));
}

SelectableTrack::~SelectableTrack()
{
    if (model.resources) editorMarkerModelDispose(&model);
    for (auto& time:ownedTimes) {
        time->previous->next=time->next;
        time->next->previous=time->previous;
    }
}

TimeOfDayTrack* SelectableTrack::GetTimeOfDay(std::int32_t value)
{
    for (auto* time=timeRoot.next;time!=&timeRoot;time=time->next)
        if (time->info->timeOfDay==value) return time;
    return nullptr;
}

bool SelectableTrack::SetTimeOfDay(std::int32_t value)
{
    currentTime=GetTimeOfDay(value);
    return currentTime!=nullptr;
}

bool SelectableTrack::ScrollTimeOfDay(std::int32_t step)
{
    auto* original=currentTime;
    if (step==1) {
        currentTime=currentTime->next;
        if (currentTime==&timeRoot) currentTime=timeRoot.next;
    } else if (step==-1) {
        currentTime=currentTime->previous;
        if (currentTime==&timeRoot) currentTime=timeRoot.previous;
    }
    return original!=currentTime;
}

void SelectableTrack::DrawTimeOfDay(FlailerResources& resources,std::uint32_t screen,float offset)
{
    for (auto* time=timeRoot.next;time!=&timeRoot;time=time->next) {
        const auto index=time->info->timeOfDay;
        Require(index>=0 && static_cast<std::size_t>(index)<TimeOfDayIcons.size(),"Track time-of-day icon index exceeds the authored table");
        auto* object=resources.FindObject(screen,TimeOfDayIcons[static_cast<std::size_t>(index)],2);
        Require(object,"Track time-of-day icon is absent from its authored screen");
        object->SetTopLeftY(offset+time->thumbnailOffset);
        object->SetTopLeftX(530.0f);
        object->visible=1;
    }
    Require(currentTime,"Track time-of-day selection has no current item");
    resources.SetTopLeftPosition(screen,0x724e34b0u,525,
        convertFloatToInt32((offset+currentTime->thumbnailOffset)-5.0f));
}

std::int32_t SelectableTrackManager::GetCurrentBaseTrackNumber() const
{
    return current->baseInfo->number;
}

std::int32_t SelectableTrackManager::GetCurrentTimeOfDay() const
{
    return current->currentTime->info->timeOfDay;
}

std::int32_t SelectableTrackManager::GetCurrentDirection() const
{
    return current->direction;
}

SelectableTrack* SelectableTrackManager::GetNextTrack() const
{
    return current==last ? current : current->next;
}

SelectableTrack* SelectableTrackManager::GetPrevTrack() const
{
    return current==first ? current : current->previous;
}

float SelectableTrackManager::GetDesiredHorizThumbnailOffset() const
{
    return 244.0f-current->horizontalThumbnail;
}

bool SelectableTrackManager::AtLeftEdge() const
{
    return current==first;
}

bool SelectableTrackManager::AtRightEdge() const
{
    return current==last;
}

TrackRecordDisplayer::TrackRecordDisplayer(FlailerUiState& state,
    FrontendPlayerManager& manager,std::uint32_t screen)
    : ui(&state),frontend(&manager),screenKey(screen)
{
}

void TrackRecordDisplayer::SetCurrentTrack(SelectableTrack* track)
{
    if (track==previousTrack)return;
    Require(track && track->baseInfo && ui && ui->resources && ui->language && ui->clock &&
        frontend && frontend->highScores && frontend->players[0],
        "Track records have no canonical selection/profile/UI owners");
    auto vehicleClass=frontend->players[0]->GetSelectedCarClass(frontend->gameType);
    if (frontend->playerCount==2) {
        Require(frontend->players[1],"Split-screen records have no second live player profile");
        const auto second=frontend->players[1]->GetSelectedCarClass(frontend->gameType);
        if (second<vehicleClass)vehicleClass=second;
    }
    const auto number=track->baseInfo->number;
    const auto direction=track->direction;
    auto* lap=raceProfileFindBestTime(&frontend->highScores->LapView(),number,vehicleClass,direction);
    Require(lap,"Track records have no canonical best-lap record");
    auto ticks=lap->ticks;
    const auto* name=frontend->RecordPlayerName(lap->playerNameIndex);
    auto& resources=*ui->resources;
    if (ticks && ticks!=INT32_MAX && !frontend->recordDisplaySuppressed) {
        char text[GAME_TIMER_TEXT_CAPACITY];
        gameTimerPrintToString(&ticks,text,0,&ui->language->currentLanguage);
        resources.SetGroupVisibility(screenKey,0xabcc6035u,1);
        resources.Printf(screenKey,0xf3363be5u,"%s %s",text,name);
    } else resources.SetGroupVisibility(screenKey,0xabcc6035u,0);
    records.clear();
    if (!track->baseInfo->routeKind) {
        for (std::int32_t laps=1;laps<9;laps*=2) {
            auto* time=frontend->GetBestTotalTimesTable(number,vehicleClass,direction,laps);
            Require(time,"Track records have no canonical total-race record");
            if (time->ticks && time->ticks!=INT32_MAX)records.push_back({laps,time});
        }
    }
    currentRecord=0;
    ShowCurrentRecord();
    advanceArmed=true;
    startedTicks=ui->clock->realTimer;
    // PAL compares source+4 on entry but does not write it in this method.
    // Do not manufacture a cache assignment absent from the real body.
}

void TrackRecordDisplayer::ShowCurrentRecord()
{
    Require(ui && ui->resources && ui->language && frontend,
        "Track record display has no live frontend/UI owners");
    auto& resources=*ui->resources;
    if (records.empty() || frontend->recordDisplaySuppressed) {
        resources.SetGroupVisibility(screenKey,0xf4bd8abcu,0);
        return;
    }
    Require(currentRecord<records.size(),"Track record rotation is outside its live list");
    const auto& record=records[currentRecord];
    Require(record.time,"Track record node has no canonical score record");
    if (!record.time->ticks || record.time->ticks==INT32_MAX)return;
    char text[GAME_TIMER_TEXT_CAPACITY];
    gameTimerPrintToString(&record.time->ticks,text,0,&ui->language->currentLanguage);
    resources.Printf(screenKey,0xae5a12ccu,"%s %s",text,
        frontend->RecordPlayerName(record.time->playerNameIndex));
    if (record.laps>=2) {
        const auto* format=ui->language->GetString(0xd6f1);
        Require(format,"Track record lap caption is missing from the language resource");
        const auto lapCaption=FormatLocalizedText(format,{record.laps});
        resources.SetLocalizedText(screenKey,0x5cc6e73eu,lapCaption.c_str());
    } else {
        const auto* format=ui->language->GetString(0xd6e9);
        Require(format,"Track record single-lap caption is missing from the language resource");
        resources.Printf(screenKey,0x5cc6e73eu,"%s",format);
    }
    resources.SetGroupVisibility(screenKey,0xf4bd8abcu,1);
}

void TrackRecordDisplayer::Update()
{
    Require(ui && ui->clock && ui->resources,"Track record update has no live clock/UI owners");
    const auto now=ui->clock->realTimer;
    const auto difference=static_cast<std::uint32_t>(now)-static_cast<std::uint32_t>(startedTicks);
    std::int32_t ticks;std::memcpy(&ticks,&difference,sizeof(ticks));
    const float elapsed=static_cast<float>(ticks)*0.0002500000118743628f;
    if (!(5.0f<elapsed))return;
    if (records.size()>=2) {
        if (elapsed<5.5f) {
            const float remaining=5.5f-elapsed;
            const float doubled=remaining+remaining;
            ui->resources->SetGroupAlpha(screenKey,0xf4bd8abcu,convertFloatToInt32(doubled*128.0f));
            return;
        }
        if (elapsed<6.0f) {
            const float remaining=6.0f-elapsed;
            const float doubled=remaining+remaining;
            const float opacity=1.0f-doubled;
            ui->resources->SetGroupAlpha(screenKey,0xf4bd8abcu,convertFloatToInt32(opacity*128.0f));
            if (advanceArmed) {
                if (++currentRecord==records.size())currentRecord=0;
                ShowCurrentRecord();
                advanceArmed=false;
            }
            return;
        }
    }
    startedTicks=now;
    advanceArmed=true;
}

TrackLockedDisplayer::TrackLockedDisplayer(FlailerUiState& state,
    const EventTreeResources& eventResources,std::uint32_t screen)
    : ui(&state),events(&eventResources),screenKey(screen)
{
    Require(ui->resources,"Locked-track display has no actual screen resources");
    auto* anchor=ui->resources->FindObject(screenKey,0xa8af730bu);
    Require(anchor,"Locked-track display has no authored anchor");
    float y;ui->resources->GetTopLeft(*anchor,anchorX,y,ui->language);
    if (auto* object=ui->resources->FindObject(screenKey,0xbea3c79cu)) {
        if (object->kind==1)initialAlpha=static_cast<FlailerTextChunk*>(object)->GetAlpha();
        else if (object->kind==2)initialAlpha=static_cast<FlailerBitmapChunk*>(object)->GetAlpha(0);
    }
    animation=std::make_unique<FlailerAnimationGroup>(*ui,screenKey,0x6a096871u,0xa8af730bu,
        640.0f,anchorX,trackPresentationSettings.lockedSlideDuration);
}

TrackLockedDisplayer::~TrackLockedDisplayer()=default;

void TrackLockedDisplayer::SetCurrentTrack(SelectableTrack* track)
{
    if (track==currentTrack)return;
    Require(track && track->baseInfo && ui && ui->resources && ui->language && ui->clock && animation,
        "Locked-track selection has no actual track/UI owners");
    currentTrack=track;
    auto& resources=*ui->resources;
    if (static_cast<std::uint32_t>(track->availability)<2u) {
        resources.SetGroupVisibility(screenKey,0x6a096871u,0);
        resources.SetObjectVisibility(screenKey,0xbea3c79cu,0);
    } else if (track->availability==2) {
        resources.SetGroupVisibility(screenKey,0x6a096871u,1);
        resources.SetObjectVisibility(screenKey,0xbea3c79cu,1);
        resources.SetObjectVisibility(screenKey,0xedf2e4eeu,0);
        resources.SetObjectVisibility(screenKey,0x20d73d01u,0);
    }
    resources.SetObjectVisibility(screenKey,0x2a67e2d5u,0);
    GetTrackUnlockInfo(message.data(),message.size(),track->baseInfo->number,track->direction,
        *events,*ui->language);
    animation->SetupToAnimate(650.0f,anchorX);
    startedTicks=ui->clock->realTimer;
}

void TrackLockedDisplayer::Update()
{
    Require(ui && ui->clock && ui->resources && animation,"Locked-track update has no live UI owners");
    const auto delta=static_cast<std::uint32_t>(ui->clock->realTimer)-static_cast<std::uint32_t>(startedTicks);
    std::int32_t ticks;std::memcpy(&ticks,&delta,sizeof(ticks));
    const float elapsed=static_cast<float>(ticks)*0.0002500000118743628f;
    const float shifted=elapsed-trackPresentationSettings.lockedDelay;
    float progress=shifted/trackPresentationSettings.lockedSlideDuration;
    if (trackPresentationSettings.lockedDelay<elapsed)animation->Animate();
    else progress=0.0f;
    const auto alpha=progress<1.0f ? convertFloatToInt32(static_cast<float>(initialAlpha)*progress) : initialAlpha;
    if (auto* object=ui->resources->FindObject(screenKey,0xbea3c79cu))object->SetAlpha(alpha);
}

SelectableTrack* SelectableTrackManager::FindTrack(const TrackInfoResources& resources,
    std::int32_t number,std::int32_t reverse) const
{
    const auto* descriptor=resources.Find(number);
    Require(descriptor,"Track selection refers to an absent authored track");
    for (auto* track=first;track;track=track->next)
        if (track->baseInfo->number==descriptor->baseNumber && track->direction==reverse) return track;
    return nullptr;
}

void SelectableTrackManager::SetCurrent(const TrackInfoResources& resources,
    std::int32_t number,std::int32_t reverse,std::int32_t timeOfDay)
{
    current=FindTrack(resources,number,reverse);
    if (!current) current=first;
    Require(current,"Track selection has no authored entries");
    if (!current->SetTimeOfDay(timeOfDay)) current->SetTimeOfDay(0);
}

bool SelectableTrackManager::ScrollSelectedTrack(std::int32_t step)
{
    SelectableTrack* selected=nullptr;
    if (step==1) selected=GetNextTrack();
    else if (step==-1) selected=GetPrevTrack();
    if (current==selected) return false;
    current=selected;
    return true;
}

bool SelectableTrackManager::ScrollTimeOfDay(std::int32_t step)
{
    Require(current,"Track selection has no current track");
    return current->ScrollTimeOfDay(step);
}

void SelectableTrackManager::DrawThumbnails(FlailerResources& resources,std::uint32_t screen,
    float horizontal,float vertical)
{
    for (auto* track=first;track;track=track->next) {
        resources.SetTopLeftPosition(screen,track->thumbnailKey,convertFloatToInt32(horizontal),401);
        horizontal+=thumbnailSpacing;
    }
    Require(current,"Track thumbnails have no current track");
    current->DrawTimeOfDay(resources,screen,vertical);
}

SelectableTrack& SelectableTrackManager::AddTrack(const TrackInfoResources& tracks,
    EditorMarkerResources& models,const TrackInfo& base,std::int32_t reverse,
    std::int32_t status,std::uint32_t thumbnail,float offset)
{
    auto track=std::make_unique<SelectableTrack>(tracks,models,base,reverse,status,thumbnail,offset);
    auto* node=track.get();
    ownedTracks.push_back(std::move(track));
    node->previous=last;
    if (last)last->next=node;
    else first=node;
    last=node;
    return *node;
}

std::int32_t SelectableTrackManager::GetTrackStatus(const FrontendPlayerManager& frontend,
    const TrackInfoResources& tracks,const EventTreeResources& events,
    std::int32_t track,std::int32_t direction) const
{
    const auto locked=IsTrackUnlocked(events,track,direction) ? 0 : 2;
    return IsTrackUnavailable(frontend,tracks,track,direction) ? 1 : locked;
}

void SelectableTrackManager::BuildList(const FrontendPlayerManager& frontend,
    const TrackInfoResources& tracks,const EventTreeResources& events,
    FlailerResources& resources,std::uint32_t screen,float spacing)
{
    thumbnailSpacing=spacing;
    float offset=0.0f;
    // The source makes two authored-order passes, all forward entries first.
    for (std::int32_t direction=0;direction<2;++direction) {
        for (const auto& base:tracks.Records()) {
            if (!base.name[0] || base.baseNumber!=base.number)continue;
            auto status=GetTrackStatus(frontend,tracks,events,base.baseNumber,direction);
            const auto thumbnail=FormattedKey(direction ? "TN_TRACK%dR" : "TN_TRACK%d",base.baseNumber);
            if (status==1) {
                resources.SetObjectVisibility(screen,thumbnail,0);
                continue;
            }
            if (easterEggs.IsTrackUnlocked(static_cast<std::uint32_t>(base.baseNumber),direction!=0))status=0;
            resources.SetObjectVisibility(screen,thumbnail,1);
            AddTrack(tracks,resources.Models(),base,direction,status,thumbnail,offset);
            offset=offset+thumbnailSpacing;
        }
    }
    // PrintSelectionStatus walks the same list and invokes only the exact
    // JR RA/NOP SelectableTrack::PrintStatus19E100; no CPU effect is omitted.
}

TrackTimeOfDayMenu::TrackTimeOfDayMenu(FlailerUiState& state,FlailerScreenList* list,std::uint32_t screen)
    : ui(&state),screens(list),screenKey(screen)
{
}

std::int32_t TrackTimeOfDayMenu::SetSelectableTrack(SelectableTrack* track)
{
    std::int32_t selected=-1;
    if (track==current) return selected;
    Require(ui && ui->resources && track && track->currentTime,
        "Track time-of-day menu has no live resource or selection owner");
    current=track;
    auto& resources=*ui->resources;
    std::int32_t item=1;
    for (std::int32_t time=0;time<5;++time) {
        const bool available=track->GetTimeOfDay(time)!=nullptr;
        const auto menuKey=FormattedKey("MENUITEM_%d",item);
        const auto backing=FormattedKey("TOD_BACKING_%d",item);
        if (available) {
            resources.SetObjectVisibility(screenKey,TimeOfDayIcons[static_cast<std::size_t>(time)],1);
            resources.SetObjectVisibility(screenKey,backing,1);
            resources.SetObjectVisibility(screenKey,menuKey,1);
            resources.SetLanguageID(screenKey,menuKey,TimeOfDayLanguage[static_cast<std::size_t>(time)]);
            if (current->currentTime->info->timeOfDay==time) {
                selected=item;
                // The original global wrapper permits a missing screen-list
                // owner. When present it calls the same active node owner.
                if (screens) {
                    Require(screens->active,"Track time-of-day selection has no active screen node");
                    screens->active->SetActiveMenuItem(menuKey);
                }
            }
            ++item;
        } else {
            resources.SetObjectVisibility(screenKey,TimeOfDayIcons[static_cast<std::size_t>(time)],0);
            resources.SetObjectVisibility(screenKey,menuKey,0);
            resources.SetObjectVisibility(screenKey,backing,0);
        }
    }
    return selected;
}

void DataSelectTrackScreen::InitWithDefaults(const TrackInfoResources& tracks)
{
    const auto* track=tracks.Find(11);
    Require(track,"Track-selection defaults require the actual track11 record");
    lapCount=2;baseTrackNumber=track->baseNumber;direction=0;timeOfDay=track->timeOfDay;
}

std::int32_t DataSelectTrackScreen::CalculateTrackNumberToLoad(const TrackInfoResources& tracks) const
{
    // PAL includes record zero in its search; GetTrackInfo(0) is null. The
    // native catalogue stores one-based records and preserves authored holes.
    for (const auto& track:tracks.Records())
        if (!!track.name[0] && track.baseNumber==baseTrackNumber && track.timeOfDay==timeOfDay)
            return track.number;
    return baseTrackNumber;
}

void SelectTrackScreen::ResyncData()
{
    Require(tracks && tracks->current,"Track-selection screen has no current track owner");
    data.baseTrackNumber=tracks->GetCurrentBaseTrackNumber();
    data.direction=tracks->GetCurrentDirection();data.timeOfDay=tracks->GetCurrentTimeOfDay();
}

bool SelectTrackScreen::PassJoystickTest(std::int32_t port) const
{
    if (!port) return true;
    Require(frontend,"Track-selection input has no frontend player manager");
    return port==1 && frontend->playerCount==2;
}

bool SelectTrackScreen::CanSelectLaps(const TrackInfo& track) const
{
    if (track.routeKind) return false;
    Require(frontend,"Track-selection lap policy has no frontend player manager");
    return frontend->gameType==3 || (frontend->gameType==7 && frontend->playerCount==2);
}

void SelectTrackScreen::StartLapSelector()
{
    Require(ui && ui->resources && highlight,"Track lap selection has no live screen/highlight owner");
    selectionState=0;
    ui->resources->Printf(screenKey,0x7e544804u,"%d",data.lapCount);
    ui->resources->SetGroupVisibility(screenKey,0x7e9e3688u,1);
    ui->resources->SetGroupVisibility(screenKey,0xb5caff20u,0);
    highlight->SetCurrentMenuItem(0x7e544804u);
}

void SelectTrackScreen::StopLapSelector()
{
    Require(ui && ui->resources,"Track lap selection has no live resource owner");
    selectionState=1;
    ui->resources->SetGroupVisibility(screenKey,0x7e9e3688u,0);
    ui->resources->SetGroupVisibility(screenKey,0xb5caff20u,1);
}

bool IsTrackUnavailable(const FrontendPlayerManager& frontend,
    const TrackInfoResources& tracks,std::int32_t track,std::int32_t direction)
{
    (void)direction;
    if (frontend.gameType!=5 && frontend.gameType!=7)return false;
    const auto* descriptor=tracks.Find(track);
    Require(descriptor,"Track availability query has no actual selected track");
    return descriptor->routeKind!=0;
}

void SelectTrackScreen::StartRace(const FrontendRaceSetupBindings& bindings)
{
    Require(frontend && tracks && tracks->current && tracks->current->baseInfo &&
        bindings.parameters && bindings.tracks,"Track race start has no live selection/setup owners");
    // Source copies all four selection words to the same FRED+154 record.
    frontend->trackSelection=data;
    auto& race=*bindings.parameters;
    race.reverse=tracks->current->direction;
    race.pointToPoint=tracks->current->baseInfo->routeKind;
    const auto index=static_cast<std::uint32_t>(frontend->gameType)-3u;
    if (index<trackRaceSetups.size()) {
        switch (trackRaceSetups[index]) {
        case TrackRaceSetup::LapRace:
            race.totalLaps=data.lapCount;race.raceKind=1;
            break;
        case TrackRaceSetup::Pursuit:
            race.raceKind=4;race.totalLaps=race.pointToPoint ? 1 : 2;
            break;
        case TrackRaceSetup::Knockout: {
            race.raceKind=3;
            // The source performs this real read-only query, then discards
            // its result. Keep the lookup and valid-resource precondition.
            (void)IsTrackUnavailable(*frontend,*bindings.tracks,
                frontend->trackSelection.CalculateTrackNumberToLoad(*bindings.tracks),
                tracks->current->direction);
            const auto bits=static_cast<std::uint32_t>(race.opponentCount)+
                static_cast<std::uint32_t>(race.mode)-1u;
            std::int32_t laps;std::memcpy(&laps,&bits,sizeof(laps));
            race.totalLaps=laps>0 ? laps : 0;
            break;
        }
        case TrackRaceSetup::SingleRace:
            race.raceKind=1;race.totalLaps=0;
            break;
        case TrackRaceSetup::MultiplayerLaps:
            race.raceKind=1;race.totalLaps=race.mode==1 ? 0 : data.lapCount;
            break;
        }
    }
    race.selectedScenario=frontend->trackSelection.CalculateTrackNumberToLoad(*bindings.tracks);
    race.eventGame=0;
    frontend->StartRace(bindings);
}

TrackInfoResources::TrackInfoResources(std::vector<std::uint8_t> authored)
    : owned_(std::make_unique<TrackScenarioData>(authored)),data_(owned_.get())
{
    Require(!data_->Records().empty(),"GLOBALB has no track catalogue");
}

TrackInfoResources::TrackInfoResources(TrackScenarioData& owner) noexcept
    : data_(&owner)
{
}

const TrackInfo* TrackInfoResources::Find(std::int32_t recordNumber) const noexcept
{
    return data_->Find(recordNumber);
}

TrackSelectionStorage::TrackSelectionStorage(const TrackInfoResources& resources,
    const std::vector<std::int32_t>& visibleBaseTracks,float spacing)
{
    manager_.thumbnailSpacing=spacing;
    for (std::int32_t direction=0;direction<2;++direction) {
        for (std::int32_t number:visibleBaseTracks) {
            const auto* base=resources.Find(number);
            Require(base && base->number==base->baseNumber,"Visible track is not an authored base track");
            auto track=std::make_unique<SelectableTrack>();
            track->baseInfo=base;track->direction=direction;
            track->horizontalThumbnail=static_cast<float>(manager_.ownedTracks.size())*spacing;
            track->timeRoot.next=track->timeRoot.previous=&track->timeRoot;
            float timeOffset=0.0f;
            for (const auto& record:resources.Records()) {
                if (!record.name[0] || record.baseNumber!=base->number) continue;
                auto time=std::make_unique<TimeOfDayTrack>(timeOffset,&record);
                timeOffset+=50.0f;
                time->previous=track->timeRoot.previous;time->next=&track->timeRoot;
                time->previous->next=time.get();track->timeRoot.previous=time.get();
                if (!track->currentTime) track->currentTime=time.get();
                track->ownedTimes.push_back(std::move(time));
            }
            track->timeMenuHeight=timeOffset;
            track->thumbnailKey=FormattedKey(direction ? "TN_TRACK%dR" : "TN_TRACK%d",base->number);
            Require(track->currentTime,"Visible track has no authored time of day");
            track->previous=manager_.last;
            if (manager_.last) manager_.last->next=track.get();
            else manager_.first=track.get();
            manager_.last=track.get();manager_.ownedTracks.push_back(std::move(track));
        }
    }
    manager_.current=manager_.first;
}

void TrackSelectionStorage::BindModels(EditorMarkerResources& models)
{
    for (auto& track:manager_.ownedTracks) {
        if (track->model.resources) editorMarkerModelDispose(&track->model);
        char name[32];
        const auto length=std::snprintf(name,sizeof(name),"TRACK%d_MINI",static_cast<int>(track->baseInfo->number));
        Require(length>=0 && static_cast<std::size_t>(length)<sizeof(name),"Track-preview model name exceeds its native buffer");
        editorMarkerModelInitialize(&track->model,&models,hashString(name));
    }
}

TrackSelectionStorage::~TrackSelectionStorage()
{
}

} // namespace hp2

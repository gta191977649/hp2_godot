#include "ReplayScreen.hpp"
#include "CarSounds.hpp"
#include "DemoDisc.hpp"
#include "DemoModeReplay.hpp"
#include "FlailerDB.hpp"
#include "Fred.hpp"
#include "Flags.hpp"
#include "Language.hpp"
#include "Player.hpp"
#include "Race.hpp"
#include "Replay.hpp"
#include "TrackedController.hpp"
#include "bware/inc/bFile.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/NIS.hpp"
#include "FMenus.hpp"
#include "indep/inc/Ecstasy.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <filesystem>
#include <limits>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <sys/vfs.h>
#include <linux/magic.h>
#endif

namespace hp2 {
namespace {
constexpr std::uint32_t replayScreenKey=0x60befaedu;
constexpr std::uint32_t hiddenScreenKey=0xddce04a3u;
constexpr float secondsPerTick=0.0002500000118743628f;
// Original ReplayHelp[8] at PAL31A758, supplied to the same HelpMenu owner.
constexpr std::array<std::int32_t,8> replayHelp{{802250,802251,802252,802253,802254,802255,802256,802257}};
constexpr std::array<std::int32_t,4> replayCameraCycle{{0,1,2,3}};

std::int32_t Difference(std::int32_t a,std::int32_t b) noexcept
{
    const auto bits=static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b);
    std::int32_t result;std::memcpy(&result,&bits,sizeof(result));return result;
}
void Visibility(FlailerUiState& ui,FlailerObjectChunk* object,bool visible)
{
    if(object)ui.resources->SetObjectVisibility(replayScreenKey,object->key,visible?1:0);
}
FlailerBitmapChunk* Bitmap(FlailerUiState& ui,std::uint32_t key)
{
    return static_cast<FlailerBitmapChunk*>(ui.resources->FindObject(replayScreenKey,key,2));
}
// Native optical-media boundary for the original bIsThereACDInTheDrive
// query. It observes the volume backing the actual resource owner. An ISO
// or BIN stored as an ordinary file is not a physical optical volume.
bool ResourceVolumeIsOptical(const GameResources& resources)
{
    const auto path=std::filesystem::absolute(resources.SourcePath());
#ifdef _WIN32
    std::wstring root(32768,L'\0');
    if(!GetVolumePathNameW(path.c_str(),root.data(),static_cast<DWORD>(root.size())))
        throw std::system_error(static_cast<int>(GetLastError()),std::system_category(),"Replay resource volume query failed");
    const auto type=GetDriveTypeW(root.c_str());
    Require(type!=DRIVE_UNKNOWN && type!=DRIVE_NO_ROOT_DIR,"Replay resource volume type is unavailable");
    return type==DRIVE_CDROM;
#else
    struct statfs volume{};
    if(statfs(path.c_str(),&volume)!=0)
        throw std::system_error(errno,std::generic_category(),"Replay resource volume query failed");
    return volume.f_type==ISOFS_SUPER_MAGIC || volume.f_type==UDF_SUPER_MAGIC;
#endif
}
}

FlailerUiState& ReplayScreen::Ui() const
{
    Require(services_.frontend && services_.frontend->ui && services_.frontend->ui->resources &&
        services_.frontend->ui->render,"ReplayScreen has no actual Flailer owner");
    return *services_.frontend->ui;
}
const GameTime& ReplayScreen::Clock() const
{
    Require(Ui().clock,"ReplayScreen has no actual GameTime owner");return *Ui().clock;
}
CarFramePlayerState& ReplayScreen::Player() const
{
    Require(services_.coordinator && services_.coordinator->viewSelection && services_.players &&
        services_.players->count>=1 && services_.players->count<=2,
        "ReplayScreen has no actual session/player selection");
    const auto seat=services_.coordinator->viewSelection->seat;
    Require(seat>=0 && seat<2 && services_.players->playersByNumber[seat],
        "ReplayScreen selected player is not live");
    auto& player=*services_.players->playersByNumber[seat];
    Require(player.car && player.car->bindings.owner && player.car->bindings.info,
        "ReplayScreen selected player has no actual car");return player;
}
GameInputRouter& ReplayScreen::Input() const
{
    Require(services_.frontend && services_.frontend->screens && services_.frontend->screens->input &&
        services_.frontend->screens->input->router,"ReplayScreen has no actual input router");
    return *services_.frontend->screens->input->router;
}
ReplayManagerOwner& ReplayScreen::Replay() const
{
    Require(services_.replay && services_.replay->owner &&
        &services_.replay->owner->State()==services_.replay,"ReplayScreen has no canonical ReplayManager");
    return *services_.replay->owner;
}
DemoModeReplay& ReplayScreen::Demo() const
{
    Require(services_.frontend && services_.frontend->raceSetup && services_.frontend->raceSetup->demoReplay,
        "ReplayScreen has no actual DemoModeReplay owner");return *services_.frontend->raceSetup->demoReplay;
}
void ReplayScreen::Post(std::int32_t message,std::intptr_t argument)
{
    Require(services_.coordinator && raceActionsEnqueue(&services_.coordinator->actions,message,argument),
        "ReplayScreen could not enqueue its actual coordinator request");
}
void ReplayScreen::PlayCue(const char* name)
{
    Require(Ui().sound,"ReplayScreen has no actual frontend sound owner");
    PlayCommonRegionSample(*Ui().sound,name,1.0f);
    Require(Ui().sound->result==WORLD_AUDIO_OK,"ReplayScreen common-region sound request failed");
}

ReplayScreen::ReplayScreen(RaceScreenServices& services,std::int32_t seat):services_(services)
{
    auto& ui=Ui();auto& replay=Replay().State();
    Require(services_.coordinator && services_.coordinator->viewSelection,
        "ReplayScreen has no selected-player owner");
    services_.coordinator->viewSelection->seat=seat;
    (void)Player();(void)Clock();(void)Input();(void)Demo();
    Require(services_.race && services_.frontend->sounds,
        "ReplayScreen has no live race/audio binding");
    try {
        ui.render->SetScreenVisibility(*ui.resources,replayScreenKey,0);screenVisible_=true;
        startSeconds_=static_cast<float>(replay.beginCapturingTime)*secondsPerTick;
        endSeconds_=static_cast<float>(replay.endCapturingTime)*secondsPerTick;
        if(endSeconds_<=startSeconds_)endSeconds_=startSeconds_+1.0f;
        if(auto* timeline=Bitmap(ui,0xbd2bc882u)) {
            timelineLeft_=convertFloatToInt32(timeline->topLeftX);
            timelineWidth_=convertFloatToInt32(ui.resources->ScaledBitmapWidth(*timeline));
        }
        cursor_=Bitmap(ui,0xb8e8186fu);CalculateTimelineMarkers(true);
        indicators_[0]=ui.resources->FindObject(replayScreenKey,0xacf8f0b5u);
        indicators_[1]=ui.resources->FindObject(replayScreenKey,0xd32490beu);
        indicators_[2]=ui.resources->FindObject(replayScreenKey,0x4b53438fu);
        AddJoyHandlers(0);AddJoyHandlers(1);ChangeCameraMover();
        Visibility(ui,Bitmap(ui,0xc9b7f0c0u),services_.players->count==1);
        MuteReplay(false);
        ++services_.frontend->screens->pointerEpoch;
    } catch(...) {
        const auto original=std::current_exception();
        try {Close();} catch(...) {}
        std::rethrow_exception(original);
    }
}
ReplayScreen::~ReplayScreen() noexcept
{
    if(!closed_) {
        try {Close();}
        catch(...) {
            if(services_.coordinator && services_.coordinator->result==RC_RESULT_OK)
                services_.coordinator->result=RC_RESULT_RACE_FAILURE;
        }
    }
    RemoveJoyHandlers(0);RemoveJoyHandlers(1);
    try {RetireCameras();}catch(...) {
        if(services_.coordinator && services_.coordinator->result==RC_RESULT_OK)
            services_.coordinator->result=RC_RESULT_CAMERA_FAILURE;
    }
}
void ReplayScreen::Close()
{
    if(closed_)return;
    ++services_.frontend->screens->pointerEpoch;
    // Detach observers before reporting audio errors. No STOP/READY/PLAYING
    // completion is synthesized, and the original failed result survives.
    std::exception_ptr first;
    const auto attempt=[&](auto&& body) {try {body();}catch(...) {if(!first)first=std::current_exception();}};
    attempt([&] {
        if(screenVisible_)Ui().render->SetScreenVisibility(*Ui().resources,hidden_?hiddenScreenKey:replayScreenKey,1);
        screenVisible_=false;CalculateTimelineMarkers(false);
    });
    RemoveJoyHandlers(0);RemoveJoyHandlers(1);
    attempt([&] {Pause(0);});
    if(services_.replay)services_.replay->slowMotion=0;
    attempt([&] {RetireCameras();});
    attempt([&] {Demo().EndRecordingSnapshot(false,Clock());Demo().EndDemoMode();Demo().SetRecordingTest(false);});
    closed_=true;
    if(first) {
        if(services_.coordinator && services_.coordinator->result==RC_RESULT_OK)
            services_.coordinator->result=RC_RESULT_RACE_FAILURE;
        std::rethrow_exception(first);
    }
}
void ReplayScreen::AddJoyHandlers(std::int32_t port)
{
    Require(port>=0 && port<2,"ReplayScreen input port is invalid");
    Require(!registered_[port],"ReplayScreen input handlers already exist");
    GameInputTarget target{};target.kind=GAME_INPUT_REPLAY_SCREEN;target.owner.replayScreen=this;
    for(std::int32_t event=99;event<=110;++event) {
        const auto result=gameInputRegister(&Input(),port,event,target);
        if(result!=GAME_INPUT_OK) {RemoveJoyHandlers(port);Require(false,"ReplayScreen input registration failed");}
        registered_[port]|=1u<<static_cast<unsigned>(event-99);
    }
}
void ReplayScreen::RemoveJoyHandlers(std::int32_t port) noexcept
{
    if(port<0 || port>=2 || !registered_[port])return;
    auto* router=services_.frontend && services_.frontend->screens && services_.frontend->screens->input?
        services_.frontend->screens->input->router:nullptr;
    if(router)for(std::int32_t event=99;event<=110;++event)
        if(registered_[port]&(1u<<static_cast<unsigned>(event-99)))gameInputUnregister(router,port,event);
    registered_[port]=0;
}

std::int32_t ReplayScreen::GetTimelinePositionX(float seconds) const
{
    const auto offset=convertFloatToInt32(static_cast<float>(timelineWidth_)*
        ((seconds-startSeconds_)/(endSeconds_-startSeconds_)));
    const auto bits=static_cast<std::uint32_t>(timelineLeft_)+static_cast<std::uint32_t>(offset);
    std::int32_t result;std::memcpy(&result,&bits,sizeof(result));return result;
}
void ReplayScreen::SetFlailerBitmapPosition(FlailerBitmapChunk* bitmap,float seconds) const
{
    if(bitmap)bitmap->SetTopLeftX(static_cast<float>(GetTimelinePositionX(seconds)));
}
void ReplayScreen::CalculateTimelineMarkers(bool visible)
{
    auto& ui=Ui();
    for(std::size_t i=0;i<markerCount_;++i)Visibility(ui,markers_[i].bitmap,false);
    for(std::size_t i=0;i<segmentCount_;++i)Visibility(ui,segments_[i].bitmap,false);
    if(!visible)return;
    markerCount_=0;segmentCount_=0;std::int32_t lap=1;bool instant=false;
    auto& replay=Replay();
    for(std::size_t i=0;i<replay.ActiveSnapshotCount();++i) {
        auto* snapshot=replay.ActiveSnapshot(i);Require(snapshot,"ReplayScreen snapshot list changed during enumeration");
        TimelineMarker* marker=nullptr;std::uint32_t key=0;
        if(!std::strcmp(snapshot->name,"LAP_SNAPSHOT")) {
            Require(markerCount_<markers_.size(),"ReplayScreen exceeds the original marker extent");
            marker=&markers_[markerCount_++];key=SprintfAndHash("LAP_MARKER%d",lap++);
        } else if(!std::strcmp(snapshot->name,"HIGHLIGHT_SNAPSHOT")) {
            // The released PAL branch1EC204 skips this name deliberately.
        } else if(!std::strcmp(snapshot->name,"DEMO_MODE_SNAPSHOT")) {
            Require(segmentCount_<segments_.size(),"ReplayScreen exceeds the original segment extent");
            marker=&segments_[segmentCount_++];key=SprintfAndHash("HIGHLIGHT_MARKER%d",static_cast<std::int32_t>(segmentCount_));
        } else if(!instant && !std::strcmp(snapshot->name,"INSTANT_REPLAY")) {
            Require(markerCount_<markers_.size(),"ReplayScreen exceeds the original marker extent");
            marker=&markers_[markerCount_++];key=0xadaa0f01u;instant=true;
        }
        if(marker) {
            *marker={};marker->bitmap=Bitmap(ui,key);
            if(marker->bitmap) {
                marker->start=static_cast<float>(snapshot->worldTimer)*secondsPerTick;
                marker->end=static_cast<float>(snapshot->endTime)*secondsPerTick;
                marker->snapshot=snapshot;Visibility(ui,marker->bitmap,true);
            }
        }
    }
    for(std::size_t i=0;i<markerCount_;++i)SetFlailerBitmapPosition(markers_[i].bitmap,markers_[i].start);
    for(std::size_t i=0;i<segmentCount_;++i) {
        auto& marker=segments_[i];if(!marker.bitmap)continue;
        SetFlailerBitmapPosition(marker.bitmap,marker.start);
        marker.bitmap->SetWidth(*ui.resources,static_cast<float>(Difference(GetTimelinePositionX(marker.end),GetTimelinePositionX(marker.start))));
        marker.bitmap->SetHeight(*ui.resources,8.0f);
        const bool highlight=!std::strcmp(marker.snapshot->name,"HIGHLIGHT_SNAPSHOT");
        marker.bitmap->SetTopLeftY(highlight?46.0f:40.0f);
        marker.bitmap->SetColor(highlight?0x50a040c0u:0x80f08020u);
    }
}

bool ReplayScreen::ShouldWeBeWatchingNis() const
{
    Require(Player().replay,"ReplayScreen player has no actual NisManager playback");
    return carFramePlayerReplayActive(&Player())!=0;
}
void ReplayScreen::StopWatchingNis()
{
    if(!nisCamera_)return;
    Require(Player().replay,"ReplayScreen cannot retire a NIS camera without its actual playback owner");
    const auto result=carViewMoverReplayRemove(Player().replay,nisCamera_);
    Require(result==CAR_VIEW_MOVER_OK,"ReplayScreen NIS camera retirement failed");nisCamera_=nullptr;
}
void ReplayScreen::RetireCameras()
{
    if(chase_) {carChaseCameraDispose(chase_);chase_=nullptr;}
    if(demoCamera_) {carViewMoverDispose(demoCamera_);demoCamera_=nullptr;}
    StopWatchingNis();
}
bool ReplayScreen::HasCameraMover() const noexcept {return chase_ || demoCamera_ || nisCamera_;}
void ReplayScreen::ChangeCameraMover()
{
    auto& player=Player();
    Require(player.cameras && player.cameras->context,"ReplayScreen has no live world camera bindings");
    const auto& cameras=*player.cameras->context;
    Require(cameras.services && cameras.services->radialBlur && cameras.viewMovers,
        "ReplayScreen has no actual camera/radial-blur owners");
    bool blur=player.movement.steeringFeedbackBusy!=0;
    if(auto* driver=player.car->bindings.owner->Driver();driver && driver->kind==CAR_DRIVER_REAL) {
        Require(driver->owner.real,"ReplayScreen real driver has no controller owner");
        blur=blur || realDriverNitroActive(&driver->owner.real->input)!=0;
    }
    auto& screen=cameras.services->screens[1];
    if(blur)screenRadialBlurStartScreen(cameras.services->radialBlur,&screen);
    else screenRadialBlurStopScreen(&screen);
    if(chase_) {carChaseCameraDispose(chase_);chase_=nullptr;}
    if(demoCamera_) {carViewMoverDispose(demoCamera_);demoCamera_=nullptr;}
    StopWatchingNis();
    if(ShouldWeBeWatchingNis()) {
        CarViewMoverResult result;nisCamera_=carLiveReplayCreateView(player.replay,1,&result);
        Require(nisCamera_ && result==CAR_VIEW_MOVER_OK,"ReplayScreen could not create the actual NIS camera");return;
    }
    Require(cameraSelection_>=0 && cameraSelection_<4,"ReplayScreen camera cycle is invalid");
    const auto type=replayCameraCycle[static_cast<std::size_t>(cameraSelection_)];
    if(type==0) {
        CarViewMoverResult result;demoCamera_=carViewMoverCreateForCar(cameras.viewMovers,1,player.car,&result);
        Require(demoCamera_ && result==CAR_VIEW_MOVER_OK,"ReplayScreen could not create the actual demo camera");
    } else if(type>=1 && type<=3) {
        CarChaseCameraContext context{};
        context.services=cameras.services;context.terrain=cameras.terrain;context.player=&player;
        context.movement=player.car;context.vehicles=cameras.chaseVehicles;context.vehicleCount=cameras.vehicleCount;
        context.defaultCatalogue=cameras.defaultCatalogue;context.simulation=cameras.simulation;
        context.shake=cameras.shake;context.settings=cameras.chaseSettings;context.viewNumber=1;
        context.viewIndex=type==3?0:1;context.dynamicResponse=0;context.initiallyFacingBack=type==2;
        context.facingRate=300.8999938964844f;context.extendedVisibility=1;
        CarCinematicResult result;chase_=carChaseCameraCreate(&context,&result);
        Require(chase_ && result==CAR_CINEMATIC_OK,"ReplayScreen could not create the actual drive camera");
    }
}

void ReplayScreen::MuteReplay(bool paused)
{
    Require(services_.frontend->sounds && services_.coordinator && services_.coordinator->bindings &&
        services_.coordinator->bindings->cameraRegistry,"ReplayScreen has no actual replay sound environment");
    Require(soundMuteReplay(*services_.frontend->sounds,
        services_.coordinator->bindings->cameraRegistry->environment.simulation,paused)==WORLD_AUDIO_OK,
        "ReplayScreen replay sound request failed");
}
void ReplayScreen::Pause(std::int32_t paused)
{
    Replay().State().paused=paused;MuteReplay(paused!=0);
}
ReplayPointerState ReplayScreen::PointerState() const
{
    const auto& replay=Replay().State();
    return {hidden_?hiddenScreenKey:replayScreenKey,services_.players->count==2,
        !hidden_ && replay.stream && replay.endCapturingTime>replay.beginCapturingTime &&
            replay.endCapturingLoopCount>replay.beginCapturingLoopCount && !Demo().IsTestingSnapshot(),
        Seeking(),seek_?seek_->position:0.0f};
}

void ReplayScreen::Seek(float position,bool preview)
{
    if(!std::isfinite(position) || !PointerState().canSeek ||
        services_.frontend->screens->help->IsActive())return;
    auto& owner=Replay();
    auto& replay=owner.State();
    position=std::clamp(position,0.0f,1.0f);
    const auto timeRange=static_cast<double>(replay.endCapturingTime)-replay.beginCapturingTime;
    auto ticks=static_cast<std::int32_t>(std::lround(replay.beginCapturingTime+timeRange*position));
    ReplaySnapshot* nearest=nullptr;
    ReplaySnapshot* first=nullptr;
    for(std::size_t i=0;i<owner.ActiveSnapshotCount();++i) {
        auto* snapshot=owner.ActiveSnapshot(i);
        if(std::strcmp(snapshot->name,"LAP_SNAPSHOT") && std::strcmp(snapshot->name,"INSTANT_REPLAY") &&
            std::strcmp(snapshot->name,"HIGHLIGHT_SNAPSHOT"))continue;
        if(snapshot->worldLoopCounter<replay.beginCapturingLoopCount ||
            snapshot->worldLoopCounter>=replay.endCapturingLoopCount)continue;
        if(!first || snapshot->worldTimer<first->worldTimer)first=snapshot;
        if(snapshot->worldTimer<=ticks && (!nearest || snapshot->worldTimer>nearest->worldTimer))
            nearest=snapshot;
    }
    if(!first)return;
    if(!nearest) {
        nearest=first;
        ticks=first->worldTimer;
        position=std::clamp(static_cast<float>((static_cast<double>(ticks)-replay.beginCapturingTime)/timeRange),
            0.0f,1.0f);
    }
    const auto now=Clock().worldTimer;
    const bool continueScrub=scrubPlayback_ && !replay.exhausted && now<=ticks && nearest->worldTimer<=now;
    if(scrubPlayback_ && scrubPlayback_->lastTarget==ticks) {
        if(seek_)seek_->preview=preview;
        else if(!preview)CancelSeek();
        return;
    }
    if(!scrubPlayback_)scrubPlayback_=PlaybackMode{replay.paused,replay.slowMotion,ticks};
    else scrubPlayback_->lastTarget=ticks;
    if(!continueScrub) {
        StopWatchingNis();
        Post(14,reinterpret_cast<std::intptr_t>(nearest));
    }
    seek_=SeekState{ticks,position,preview};
    replay.paused=0;
    replay.slowMotion=0;
    MuteReplay(true);
}

void ReplayScreen::CancelSeek()
{
    if(!scrubPlayback_)return;
    auto& replay=Replay().State();
    const auto& clock=Clock();
    const bool ended=Difference(replay.endCapturingLoopCount,clock.worldLoopCounter)<=0;
    const auto mode=*scrubPlayback_;
    seek_.reset();
    scrubPlayback_.reset();
    if(ended)stoppedAtTicks_=clock.realTimer;
    replay.slowMotion=mode.slowMotion;
    Pause(ended || mode.paused || replay.exhausted);
}

void ReplayScreen::Tick()
{
    auto& replay=Replay().State();const auto& clock=Clock();
    const bool ended=Difference(replay.endCapturingLoopCount,clock.worldLoopCounter)<=0;
    if(seek_) {
        if(!ended && clock.worldTimer<seek_->ticks && !replay.exhausted)return;
        if(seek_->preview) {
            seek_.reset();
            Pause(1);
        } else {
            CancelSeek();
        }
    }
    if(scrubPlayback_)return;
    if(ended && !replay.paused) {stoppedAtTicks_=clock.realTimer;Pause(1);}
    if(raceCoordinatorHasEndGameScreens(*services_.coordinator) && ended && replay.paused &&
        4.0f<static_cast<float>(Difference(clock.realTimer,stoppedAtTicks_))*secondsPerTick)Post(11,0);
    if(Demo().IsTestingSnapshot() && !Demo().ServiceDemoMode(*services_.coordinator,clock))Pause(1);
}
void ReplayScreen::Update()
{
    CalculateTimelineMarkers(true);auto& ui=Ui();auto& player=Player();auto& race=*services_.race;
    SetFlailerBitmapPosition(cursor_,static_cast<float>(Clock().worldTimer)*secondsPerTick);
    auto* entry=player.car->bindings.owner->raceEntry;
    Require(entry && entry->car==player.car->bindings.owner,"ReplayScreen player has no actual racing-car record");
    ui.resources->Printf(replayScreenKey,0x18b2dbc8u,"%d",services_.coordinator->viewSelection->seat+1);
    auto* lap=ui.resources->FindObject(replayScreenKey,0x6486623bu);
    auto* lapCaption=ui.resources->FindObject(replayScreenKey,0x51072bf1u);
    auto* position=ui.resources->FindObject(replayScreenKey,0x3405c44eu);
    auto* positionCaption=ui.resources->FindObject(replayScreenKey,0x98071aa4u);
    if(player.car->bindings.info->bodyCategory==1) {
        Visibility(ui,lap,false);Visibility(ui,lapCaption,false);Visibility(ui,position,false);Visibility(ui,positionCaption,false);
    } else {
        Require(services_.frontend->raceSetup->parameters,"ReplayScreen has no actual race settings");
        const bool showLaps=race.targetLapCount!=1 && !services_.frontend->raceSetup->parameters->pointToPoint;
        Visibility(ui,lap,showLaps);Visibility(ui,lapCaption,showLaps);
        if(showLaps)ui.resources->Printf(replayScreenKey,0x6486623bu,"%d/%d",
            std::min(static_cast<std::int32_t>(entry->lap)+1,race.targetLapCount),race.targetLapCount);
        Require(race.entryCount<=static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()),
            "ReplayScreen race participant count exceeds native signed range");
        auto count=static_cast<std::int32_t>(race.entryCount);
        if(services_.players->count>=2) {
            const auto seat=services_.coordinator->viewSelection->seat;
            const auto otherSeat=(seat+1)%services_.players->count;
            if(otherSeat!=seat) {
                auto* other=services_.players->playersByNumber[otherSeat];
                Require(other && other->car && other->car->bindings.info,"ReplayScreen second player is not live");
                if(other->car->bindings.info->bodyCategory==1)--count;
            }
        }
        Visibility(ui,position,count>=2);Visibility(ui,positionCaption,count>=2);
        if(count>=2)ui.resources->Printf(replayScreenKey,0x3405c44eu,"%d/%d",entry->position,count);
    }
    char time[GAME_TIMER_TEXT_CAPACITY]{};
    Require(ui.language,"ReplayScreen has no actual language owner");
    auto ticks=race.elapsedTicks;gameTimerPrintToString(&ticks,time,0,&ui.language->currentLanguage);
    auto* timer=ui.resources->FindObject(replayScreenKey,0x15e0f0a8u);
    if(entry->completionReason==2 || entry->completionReason==3) {
        Visibility(ui,timer,false);
        ui.resources->SetLanguageID(replayScreenKey,0xd53d9dbeu,static_cast<std::uint32_t>(raceFinishReasonLanguageId(entry->completionReason)));
    } else {
        Visibility(ui,timer,true);ui.resources->Printf(replayScreenKey,0x15e0f0a8u,"%s",time);
        ui.resources->SetLanguageID(replayScreenKey,0xd53d9dbeu,0x61a8au);
    }
    const bool blink=Clock().realTimeFrames%24>12;
    Visibility(ui,indicators_[0],Demo().IsTestingSnapshot() && blink);
    Visibility(ui,indicators_[1],Demo().IsRecordingSnapshot() && blink);
    Visibility(ui,indicators_[2],Demo().IsRecordingTest() && blink);
    if(!HasCameraMover())ChangeCameraMover();
    if(ShouldWeBeWatchingNis()!=(nisCamera_!=nullptr))ChangeCameraMover();
}
void ReplayScreen::JoyHandlerBridge(std::int32_t port,std::int32_t event,std::uint8_t value,ReplayScreen* owner)
{
    Require(owner,"ReplayScreen callback has no owner");owner->JoyHandler(port,event,value);
}
void ReplayScreen::JoyHandler(std::int32_t port,std::int32_t event,std::uint8_t value)
{
    (void)port;auto& screens=*services_.frontend->screens;
    Require(screens.help,"ReplayScreen has no actual HelpMenu owner");
    if(event!=106 && screens.help->IsActive())return;
    if(scrubPlayback_ && event!=99 && event!=104 && event!=105 && event!=106 && event!=107)return;
    if(lastJoyLoop_==Clock().realLoopCounter)return;
    lastJoyLoop_=Clock().realLoopCounter;
    if(event<99 || event>110)return;
    auto& replay=Replay().State();
    switch(event) {
    case 99:PlayCue("MenuUpDown");Post(11,0);break;
    case 100:
        PlayCue("MenuUpDown");
        if(!replay.paused)Pause(1);
        else if(Difference(replay.endCapturingLoopCount,Clock().worldLoopCounter)>0)Pause(0);
        break;
    case 101:case 102: {
        PlayCue("MenuLeftRight");
        if(Demo().IsTestingSnapshot()) {Demo().AdvanceSnapshot(event==102,*services_.coordinator);break;}
        TimelineMarker* selected=nullptr;
        const float now=static_cast<float>(Clock().worldTimer)*secondsPerTick;
        for(std::size_t i=0;i<markerCount_;++i) {
            auto& marker=markers_[i];
            if(event==101) {if(1.0f<=now-marker.start)selected=&marker;}
            else if(now<marker.start) {selected=&marker;break;}
        }
        if(selected) {
            Require(selected->snapshot,"ReplayScreen timeline destination has no retained snapshot");
            Pause(0);StopWatchingNis();Post(14,reinterpret_cast<std::intptr_t>(selected->snapshot));
        }
        break;
    }
    case 103:
        PlayCue("MenuUpDown");
        if(replay.paused) {Pause(0);replay.slowMotion=1;}
        else replay.slowMotion=replay.slowMotion==0;
        break;
    case 104:
        cameraSelection_=(cameraSelection_+1)&3;ChangeCameraMover();Ui().sound->Play(3);break;
    case 105:
        if(services_.players->count==2) {
            StopWatchingNis();auto& seat=services_.coordinator->viewSelection->seat;seat=(seat+1)%2;
            ChangeCameraMover();Ui().sound->Play(5);
        }
        break;
    case 106: {
        CancelSeek();
        const bool wasActive=screens.help->IsActive();
        const bool paused=screens.MakeHelpGo(replayHelp.data(),value);
        if(paused!=wasActive)++screens.pointerEpoch;
        if(Difference(replay.endCapturingLoopCount,Clock().worldLoopCounter)>0)Pause(paused?1:0);
        break;
    }
    case 107:
        CancelSeek();
        hidden_=hidden_==0;
        ++screens.pointerEpoch;
        Ui().render->SetScreenVisibility(*Ui().resources,hidden_?replayScreenKey:hiddenScreenKey,1);
        Ui().render->SetScreenVisibility(*Ui().resources,hidden_?hiddenScreenKey:replayScreenKey,0);
        Ui().sound->Play(5);break;
    case 108: {
        const auto* demo=services_.frontend->demoMode;
        Require(demo && demo->files && demo->demoDisc,"ReplayScreen recording has no actual resource/disc-policy owner");
        if(ResourceVolumeIsOptical(*demo->files))break;
        if(value) {demo->demoDisc->SetReplayRecordingTimeouts();Demo().BeginRecordingSnapshot();}
        else Demo().EndRecordingSnapshot(false,Clock());
        break;
    }
    case 109:
        if(Demo().CanTestDemoMode()) {
            if(Demo().IsTestingSnapshot())Demo().EndDemoMode();
            else Demo().BeginDemoModeTest(*services_.coordinator);
            Pause(0);
        }
        break;
    case 110:
        if(!value)Demo().SetRecordingTest(false);
        else if(Demo().IsTestingSnapshot())Demo().SetRecordingTest(true);
        break;
    default:break;
    }
    Require(!Ui().sound || Ui().sound->result==WORLD_AUDIO_OK,"ReplayScreen frontend sound request failed");
}
} // namespace hp2

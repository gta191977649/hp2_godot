#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/FMenus.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/TrackPath.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/DemoDisc.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/ReplayScreen.hpp"
#include "indep/inc/PauseMenu.hpp"
#include "indep/inc/CarSounds.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/Sound3dSample.hpp"
#include "indep/src/speech/mgr.hpp"
#include "indep/inc/RealDriver.hpp"
#include "indep/inc/CarLoader.hpp"
#include "indep/inc/EasterEggs.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/EventTree.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/DemoModeReplay.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/Race.hpp"
#include "indep/src/speech/mgr.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/World.hpp"
#include "bware/inc/bChunk.hpp"
#include <cstring>
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <cmath>
#include <stdlib.h>

// PostMessage1496D8 keeps the accepted bounded Windows FIFO adaptation.
// GameSessionCameras is the existing native ownership adapter for the real
// Resume/SuspendDemoCameras1:1 receiver and concrete CarViewMover lifecycle.

void raceActionsInitialize(RaceActionQueue* queue, RaceAction* storage, size_t capacity)
{
    queue->entries = storage;
    queue->capacity = capacity;
    queue->readIndex = queue->count = 0;
    queue->overflow = 0;
}

int32_t raceActionsEnqueue(RaceActionQueue* queue, int32_t kind, intptr_t argument)
{
    if (!queue || !queue->entries || !queue->capacity || queue->readIndex >= queue->capacity || queue->count >= queue->capacity) {
        if (queue) queue->overflow = 1;
        return 0;
    }
    size_t index = queue->readIndex + queue->count;
    if (index >= queue->capacity) index -= queue->capacity;
    queue->entries[index].kind = kind;
    queue->entries[index].argument = argument;
    ++queue->count;
    return 1;
}

int32_t raceActionsTake(RaceActionQueue* queue, RaceAction* action)
{
    if (!queue || !action || !queue->count) return 0;
    // A restored or retired frontend queue must not turn a stale cursor
    // into an arbitrary native pointer read. No changes to healthy FIFO
    // order or the authored action arguments.
    if (!queue->entries || !queue->capacity || queue->readIndex >= queue->capacity ||
        queue->count > queue->capacity) {
        queue->overflow = 1;
        return 0;
    }
    *action = queue->entries[queue->readIndex];
    if (++queue->readIndex == queue->capacity) queue->readIndex = 0;
    --queue->count;
    return 1;
}

struct GameSessionCameras {
    GameSessionCameraContext context;
    CarViewMover* current;
};

GameSessionCameras* gameSessionCamerasCreate(const GameSessionCameraContext* context,
    CarViewMoverResult* result)
{
    if (result) *result = CAR_VIEW_MOVER_INVALID_BINDING;
    if (!context || !context->coordinators || !context->players) return NULL;
    GameSessionCameras* cameras = static_cast<GameSessionCameras*>(calloc(1, sizeof(*cameras)));
    if (!cameras) {
        if (result) *result = CAR_VIEW_MOVER_ALLOCATION_FAILURE;
        return NULL;
    }
    cameras->context = *context;
    if (result) *result = CAR_VIEW_MOVER_OK;
    return cameras;
}

CarViewMoverResult gameSessionCameraActivate(GameSessionCameras* cameras)
{
    if (!cameras) return CAR_VIEW_MOVER_INVALID_BINDING;
    if (cameras->current) return CAR_VIEW_MOVER_OK;
    const CarFramePlayerRegistry* players = cameras->context.players;
    if (players->count < 1 || !players->players[0] || !players->players[0]->car)
        return CAR_VIEW_MOVER_INVALID_BINDING;
    CarViewMoverResult result;
    cameras->current = carViewMoverCreateForCar(cameras->context.coordinators, 1,
        players->players[0]->car, &result);
    return result;
}

void gameSessionCameraDeactivate(GameSessionCameras* cameras)
{
    if (!cameras || !cameras->current) return;
    carViewMoverDispose(cameras->current);
    cameras->current = NULL;
}

void gameSessionCamerasDispose(GameSessionCameras* cameras)
{
    if (!cameras) return;
    gameSessionCameraDeactivate(cameras);
    free(cameras);
}

CarViewMover* gameSessionCurrentCamera(const GameSessionCameras* cameras)
{
    return cameras ? cameras->current : NULL;
}

// Original14A7F8 callback ignores port/value and only posts on JoyEvent22.
void raceCoordinatorJoystickEvent(GameSession* session,int32_t event)
{
    if(event==22) raceActionsEnqueue(&session->actions,4,0);
}

// Complete PAL14B108. Strict comparisons at the authored quarter boundaries
// matter: an element is hidden at exactly its start, and appears afterwards.
void raceCoordinatorAnimateCountdownElement(hp2::FlailerUiState& ui,
    WorldAudioRequestQueue& audio,float start,float end,float progress,
    uint32_t screen,uint32_t element)
{
    hp2::Require(ui.resources,"Countdown has no live Flailer resource owner");
    auto& resources=*ui.resources;
    if (start<progress) {
        float phase=1.0f;
        if (progress<end) phase=(progress-start)/(end-start);
        phase*=6.0f;
        if (1.0f<phase) phase=1.0f;
        auto* object=resources.FindObject(screen,element);
        if (object && !object->visible) {
            const auto* cue=worldAudioFindInGame(&audio,"CountDownLow");
            hp2::Require(audio.result==WORLD_AUDIO_OK,"Countdown sample lookup failed");
            if (cue) {
                WorldAudioVoiceRef voice{};
                hp2::Require(worldAudioAcquireBankSample(&audio,audio.inGameBank,cue,&voice)==WORLD_AUDIO_OK,
                    "Countdown has no retained bank voice");
                hp2::Require(worldAudioPlayVoice(&audio,voice,1.0f,0.0f,0.0f)==WORLD_AUDIO_OK,
                    "Countdown play was not submitted");
            }
        }
        resources.SetObjectVisibility(screen,element,1);
        if (auto* found=resources.FindObject(screen,element))
            found->SetAlpha(convertFloatToInt32(phase*128.0f));
        const float scaled=phase*-3.0f;
        resources.ScaleObjectAboutCenter(screen,element,scaled+4.0f,ui.language);
    } else resources.SetObjectVisibility(screen,element,0);
}

// Complete PAL14B260. The high note borrows the same named bank sample and
// respects its real playing state; no extra local "already played" flag.
void raceCoordinatorDisplayCountdown(hp2::FlailerUiState& ui,
    WorldAudioRequestQueue& audio,const RaceFrame& race,int32_t specialPursuit,int32_t number)
{
    hp2::Require(ui.resources && ui.render && race.clock,"Countdown has no live screen/race owners");
    const float progress=raceFrameCountdownProgress(&race,specialPursuit);
    if (number==4 && progress<0.800000011920929f)
        hp2::Require(worldAudioPlayInGameIfIdle(&audio,"CountDownHigh",1.0f,0.0f,0.0f)==WORLD_AUDIO_OK,
            "Countdown high note was not submitted");
    constexpr uint32_t screen=0x936ace42;
    if (1.0f<progress || number==0 || number==4) {
        ui.render->SetScreenVisibility(*ui.resources,screen,1);
        return;
    }
    ui.render->SetScreenVisibility(*ui.resources,screen,0);
    raceCoordinatorAnimateCountdownElement(ui,audio,0.0f,0.25f,progress,screen,0x93667af3);
    raceCoordinatorAnimateCountdownElement(ui,audio,0.25f,0.5f,progress,screen,0x93667af2);
    raceCoordinatorAnimateCountdownElement(ui,audio,0.5f,0.75f,progress,screen,0x93667af1);
}

// PAL14A020. All requested authored ordinals must exist; otherwise the
// original route-based placement replaces the whole grid. W is untouched.
int32_t raceCoordinatorCalculateRacerPositions(const TrackScenarioCatalog& catalog,
    const TrackRoute& route,TerrainTopology& terrain,int32_t reverse,
    int32_t count,float (*positions)[4])
{
    if (count<0 || count>8 || (count && !positions) ||
        (catalog.landmarkCount && !catalog.landmarks)) return 0;
    const uint32_t key=hashString(reverse ? "BackwardStartingPosition" : "ForwardStartingPosition");
    const int32_t markers=trackLandmarkCount(&catalog,key);
    bool complete=markers>=count;
    if (complete) {
        for (int32_t position=0;position<count;++position) {
            bool found=false;
            for (int32_t index=0;index<markers;++index) {
                const auto* marker=trackLandmarkFind(&catalog,key,index);
                if (!marker) return 0;
                if (marker->authoredValue==static_cast<uint32_t>(position)) {
                    positions[position][0]=marker->position[0];
                    positions[position][1]=marker->position[1];
                    found=true;break;
                }
            }
            if (!found) {complete=false;break;}
        }
    }
    if (!complete) {
        TrackRouteLocation location;
        trackRouteLocationInitialize(&location);
        for (float distance=10.0f;distance<=100.0f;distance+=10.0f)
            if (!trackCoordinatePositionAtDistance(&route,&location,distance,0.0f)) return 0;
        for (int32_t index=0;index<count;++index) {
            const float distance=static_cast<float>(index/2)*17.0f+100.0f;
            const float across=(index&1) ? -3.0f : 3.0f;
            if (!trackCoordinatePositionAtDistance(&route,&location,distance,across)) return 0;
            positions[index][0]=location.point[0];positions[index][1]=location.point[1];
        }
        // The source coordinate destructor only retires a console debug
        // display object. No diagnostic display owner exists in this port.
    }
    TerrainSurface surface;
    terrainSurfaceInitialize(&surface,&terrain);
    float previousElevation=0.0f;
    for (int32_t index=0;index<count;++index) {
        // PAL retains f0 from the preceding elevation query for the next
        // point's starting Z. Resetting every point to zero changes selection.
        positions[index][2]=previousElevation;
        previousElevation=terrainSurfaceElevation(&surface,positions[index],nullptr,nullptr,nullptr);
        positions[index][2]=previousElevation;
    }
    terrainSurfaceDispose(&surface);
    return 1;
}

namespace {
// Complete released static AddRacingCar149F10. Its own stStaticProc(14)
// PDR and immediately preceding N_SOL prove this source path; see the
// complete individual_construction_paths54.json instruction correspondence.
void addRacingCar(World& world,RaceDriverInfo& selection,const float position[4],
    CarDriverKind control)
{
    const auto& binding=hp2::WorldCarBindings(world);
    hp2::Require(binding.movement && binding.movement->route,
        "AddRacingCar has no actual track route");
    TrackRouteLocation location;
    trackRouteLocationAtPoint(binding.movement->route,&location,position);
    hp2::Require(location.node,"AddRacingCar could not locate the authored start position");
    const auto heading=vectorAtan2Angle(location.direction[0],location.direction[1]);
    // PAL adds the actual coordinate.distance F12 argument; the beta body
    // did not pass this value. The constructor consumes the final argument.
    std::unique_ptr<Car> car(hp2::WorldCreateCar(world,selection,position,heading,location.distance));
    hp2::Require(car && car->Driver() && car->Movement(),"AddRacingCar construction failed");
    auto& driver=*car->Driver();
    hp2::Require(carDriverChange(&driver,car.get(),control)==CAR_DRIVER_OK,
        "AddRacingCar controller construction failed");
    int32_t movementMode=1;
    if(control==CAR_DRIVER_AI) {
        hp2::Require(driver.environment && driver.environment->ai &&
            driver.environment->ai->tuning,"AddRacingCar has no live AI tuning");
        movementMode=driver.environment->ai->tuning->defaultMovementMode;
    }
    hp2::Require(carMovementSetMode(car->Movement(),movementMode)==CAR_MOVEMENT_OK,
        "AddRacingCar mover construction failed");
    worldAddCar(&world,car.get());
    try {
        if(control==CAR_DRIVER_AI)worldRegisterAiCar(&world,car.get());
    } catch(...) {
        worldRemoveCar(car.get());
        throw;
    }
    (void)car.release();
    // The source coordinate destructor only releases its optional PS2
    // diagnostic display object; this native coordinate owns no object.
}
}

// Complete PAL14A2C8..14A5F0, including both release-only explicit refresh
// arguments and the canonical EasterEggs entry5 Chopper branch. Driver order and
// the first matching starting-position rule come from the final executable.
void raceCoordinatorInitCars(World& world,const RaceParameters& parameters,
    const TrackScenarioCatalog& catalog)
{
    const auto& binding=hp2::WorldCarBindings(world);
    hp2::Require(world.carList.next==&world.carList && world.carList.previous==&world.carList,
        "InitCars requires the newly constructed empty World");
    hp2::Require(binding.topology && binding.movement && binding.movement->route &&
        binding.movement->recoveryPoints && parameters.driverCount>=0 &&
        (!parameters.driverCount || parameters.drivers),
        "InitCars is missing actual track/driver owners");
    const auto& road=*binding.movement->recoveryPoints;
    hp2::Require(!road.groupCount || road.groups,"InitCars road registry is invalid");
    float positions[8][4]{};
    hp2::Require(raceCoordinatorCalculateRacerPositions(catalog,*binding.movement->route,
        *binding.topology,parameters.reverse,8,positions)!=0,
        "InitCars could not calculate the complete original start grid");
    for(int32_t position=1;position<=8;++position) {
        for(int32_t index=0;index<parameters.driverCount;++index) {
            auto& selection=parameters.drivers[index];
            if(selection.eliminated>0 || selection.startingPosition!=position)continue;
            if(selection.driverClass==1 || selection.driverClass==2)
                addRacingCar(world,selection,positions[position-1],
                    selection.driverClass==1 ? CAR_DRIVER_REAL : CAR_DRIVER_AI);
            // A matching non-racer class also consumes this position. The
            // original does not search a later duplicate starting position.
            break;
        }
    }
    hp2::Require(worldBuildCarTable(&world)==WORLD_CARS_OK,"InitCars racer table overflow");
    const auto* scenario=trackScenarioFind(&catalog,parameters.selectedScenario);
    hp2::Require(scenario,"InitCars has no selected authored scenario");
    // Native scratch starts absent when PAL has no authored route index.
    // This does not invent a valid pose: the first actual AI must FindWhereIAm.
    CarRecoverySelection firstRoad{};
    if(scenario->racerRouteIndex>=0) {
        for(size_t index=0;index<road.groupCount;++index) {
            const auto& group=road.groups[index];
            if(group.id==scenario->racerRouteIndex) {
                hp2::Require(group.poses && group.poseCount>0,"InitCars authored racer route has no points");
                firstRoad={&group,&group.poses[0]};break;
            }
        }
        hp2::Require(firstRoad.group,"InitCars authored racer route is absent");
    }
    for(int32_t index=world.firstGroupEnd;index<world.secondGroupEnd;++index) {
        auto* car=world.carTable[index];
        hp2::Require(car && car->Driver() && car->Driver()->kind==CAR_DRIVER_AI &&
            car->Driver()->owner.ai,"InitCars AI table does not reference actual AI controllers");
        auto& driver=car->Driver()->owner.ai->base;
        if(index==world.firstGroupEnd && !driverRoadSelectionValid(&firstRoad)) {
            // Direct FindWhereIAm, not OnCarPlaced (which also resets the
            // controller and would introduce behavior absent from this call).
            driverAIRefreshRoad(&driver);
            hp2::Require(driver.result==CAR_DRIVER_OK,"InitCars first AI route refresh failed");
            firstRoad.group=driverAICurrentGroup(&driver,1);
            firstRoad.pose=driverAICurrentPose(&driver,1);
        } else driverAISetRoad(&driver,firstRoad.group,firstRoad.pose);
        hp2::Require(driver.result==CAR_DRIVER_OK,"InitCars AI route selection failed");
    }
    int32_t copCount=0;
    for(int32_t index=0;index<parameters.driverCount;++index) {
        auto& selection=parameters.drivers[index];
        if(selection.driverClass==3)(void)hp2::WorldSpawnTraffic(world,selection);
        else if(selection.driverClass==4 || selection.driverClass==5) {
            ++copCount;
            hp2::Require(hp2::WorldCreateCop(world,&selection,false),"InitCars cop construction failed");
        }
    }
    hp2::Require(worldBuildCarTable(&world)==WORLD_CARS_OK,"InitCars complete car table overflow");
    // PAL31D7F4 is gEasterEggs+0C, not a track-resource pointer. Its
    // +148 word is entry5.unlocked in the one authored78-record table.
    const auto& eggs=hp2::easterEggs;
    if(!eggs.records || eggs.recordCount<=5)
        throw std::runtime_error("InitCars requires the actual EasterEggs table");
    if(copCount || eggs.records[5].unlocked)
        hp2::Require(hp2::WorldCreateChopper(world),"InitCars Chopper construction failed");
    if(copCount && parameters.specialPursuit)
        hp2::Require(hp2::WorldCreateCop(world,nullptr,true),"InitCars player cop construction failed");
}

// Complete PAL14A5F0..14A664. Cars are unlinked before their actual owned
// controller/mover/model/damage/audio graph is destroyed; Chopper is last.
void raceCoordinatorCleanupCars(World& world)
{
    hp2::Require(world.carList.next && world.carList.previous,"CleanupCars has no live World list");
    for(auto* car=world.carList.next;car!=&world.carList;car=car->next)
        hp2::Require(!car->Frame() || !car->Frame()->environment.player,
            "CleanupCars requires CleanupPlayers before deleting attached cars");
    hp2::WorldPrepareCarCleanup(world);
    while(world.carList.next!=&world.carList) {
        auto* car=world.carList.next;
        worldUnregisterAiCar(&world,car);
        worldRemoveCar(car);
        // Race is destroyed after Cars in PAL. Retire the reciprocal native
        // borrower before freeing its Car so Race disposal cannot dereference
        // freed memory. This is one link, not another race state owner.
        if(car->raceEntry && car->raceEntry->car==car)car->raceEntry->car=nullptr;
        car->raceEntry=nullptr;
        delete car;
    }
    hp2::WorldDestroyChopper(world);
    // Retire native table/frame borrowers after deletion; no game update is
    // run during shutdown and no source lifecycle stage is fabricated.
    hp2::Require(worldBuildCarTable(&world)==WORLD_CARS_OK,"CleanupCars empty table retirement failed");
}

struct RaceCoordinatorOwnedState {
    explicit RaceCoordinatorOwnedState(const RaceCoordinatorInitBindings& b):bindings(b) {}
    RaceCoordinatorInitBindings bindings;
    // These two structures only borrow the same services. They outlive
    // World, which must never retain a pointer into InitForRace's stack.
    hp2::WorldConstructionBindings worldBindings{};
    TrafficSchedulerBindings trafficBindings{};
    std::unique_ptr<ReplayManagerOwner> replay;
    std::unique_ptr<RaceFrameOwner> race;
    std::unique_ptr<World> world;
    std::unique_ptr<hp2::EndGameScreens> endScreens;
    std::unique_ptr<hp2::ReplayScreen> replayScreen;
    // These are stable views of the cars created by this owner. They cannot
    // be prepared by a caller before InitCars has constructed those cars.
    std::array<CarChaseCameraVehicle,WORLD_MAX_CARS> cameraVehicles{};
    std::array<const CarMovement*,WORLD_MAX_CARS> cameraMovements{};
    // Only human traffic without authored views uses these native fitted
    // catalogues. The same RC owner outlives Player and replay borrowers.
    std::array<CarCameraCatalogue,2> trafficPlayerCameras{};
    CarFramePlayerCameraContext playerCameraContext{};
    bool cameraResourcesBound{};
    // Native exception cleanup tracks only the two actual borrowed input
    // registrations; this is not another demo/playback state owner.
    std::array<bool,2> demoInputBound{};
};

namespace {
void fitTrafficPlayerCameras(CarCameraCatalogue& destination,const Car& car,
    const hp2::CarInfoResource& cars)
{
    const CarCameraCatalogue* source=nullptr;
    for(const auto& candidate:cars.Cars()) {
        const auto& views=candidate.cameraCatalogue;
        if(candidate.bodyCategory==0 && views.viewCount[0]>=3 && views.viewCount[1]>=3 &&
           views.views[0][0].kind==0 && views.views[1][0].kind==0 &&
           views.views[0][1].kind==2 && views.views[0][2].kind==2 &&
           views.views[1][1].kind==2 && views.views[1][2].kind==2) {source=&views;break;}
    }
    hp2::Require(source,"Traffic player camera has no actual bumper/chase source views");
    destination=*source;
    float minimum[4]{},maximum[4]{};
    carRenderBounds(&car,minimum,maximum);
    for(std::size_t axis=0;axis<3;++axis)
        hp2::Require(std::isfinite(minimum[axis]) && std::isfinite(maximum[axis]) &&
            minimum[axis]<maximum[axis],"Traffic player camera has invalid live model bounds");
    const float length=maximum[0]-minimum[0];
    const float width=maximum[1]-minimum[1];
    const float front=std::max(maximum[0],0.0f);
    const float rear=std::max(-minimum[0],0.0f);
    const float roof=std::max(maximum[2],0.0f);
    // Keep the source view kinds/lens policy, then place the near plane in
    // front of the real bumper and both chase eyes behind/above the body.
    // This covers vans/trucks without changing their geometry or physics.
    const float nearDistance=rear+std::max({2.8f,width*1.25f,length*0.65f});
    const float farDistance=nearDistance+std::max(1.1f,length*0.25f);
    const float chaseHeight=roof+std::max(0.65f,roof*0.2f);
    for(std::size_t layout=0;layout<2;++layout) {
        destination.viewCount[layout]=3; // Traffic has no authored cockpit.
        auto& views=destination.views[layout];
        views[0].offset[0]=-(front+0.75f);
        views[0].offset[1]=std::max(0.9f,roof*0.6f);
        views[0].offset[2]=0.0f;
        views[1].offset[0]=std::max(views[1].offset[0],farDistance);
        views[2].offset[0]=std::max(views[2].offset[0],nearDistance);
        views[1].offset[1]=std::max(views[1].offset[1],chaseHeight+0.2f);
        views[2].offset[1]=std::max(views[2].offset[1],chaseHeight);
    }
}
}

// Native borrows of the original RC+28/+2C allocations. No parallel flag
// or independently owned replay screen is introduced.
bool raceCoordinatorHasEndGameScreens(const GameSession& session) noexcept
{
    return session.ownedState && session.ownedState->endScreens!=nullptr;
}

void raceCoordinatorUpdateReplayScreen(GameSession& session)
{
    if (session.ownedState && session.ownedState->replayScreen)
        session.ownedState->replayScreen->Update();
}

namespace {
// PAL14A94C..14A998. AI Demo owns the same two input registrations and
// demo camera, but it does not enter/exit the recorded DemoModeReplay owner.
void coordinatorEndAIDemo(GameSession& session)
{
    auto& owner=*session.ownedState;
    auto& ui=*session.bindings->startScreens->services.frontend->ui;
    ui.render->SetScreenVisibility(*ui.resources,0xcc28b8a9u,1);
    auto& input=*owner.bindings.players->input;
    for(int32_t port=0;port<2;++port)if(owner.demoInputBound[static_cast<std::size_t>(port)]) {
        (void)gameInputUnregister(&input,port,22);
        owner.demoInputBound[static_cast<std::size_t>(port)]=false;
    }
    gameSessionCameraDeactivate(session.cameras);
}

// Exit arm of PAL14AA38, also used to retire a partially entered native
// callback after a checked resource failure. All borrowers still live here.
bool coordinatorEndReplayDemo(GameSession& session)
{
    auto& owner=*session.ownedState;
    auto& binding=*session.bindings;
    auto& ui=*binding.startScreens->services.frontend->ui;
    ui.render->SetScreenVisibility(*ui.resources,0xcc28b8a9u,1);
    auto& input=*owner.bindings.players->input;
    for(int32_t port=0;port<2;++port)if(owner.demoInputBound[static_cast<std::size_t>(port)]) {
        (void)gameInputUnregister(&input,port,22);
        owner.demoInputBound[static_cast<std::size_t>(port)]=false;
    }
    gameSessionCameraDeactivate(session.cameras);
    owner.bindings.demoReplay->EndDemoMode();
    // Final PAL14AB24 adds this real type-monitor reset to the beta body.
    return joystickResetTypeHandlers(&input)==GAME_INPUT_OK;
}
}

// Complete PAL1498F0..149988, independently matched to beta149768.
// The source visits the same World, Race and creation-order Players; no
// serialized replacement GameSession or shadow race participant exists.
void raceCoordinatorDoSnapshot(GameSession& session,ReplaySnapshot& snapshot)
{
    hp2::Require(session.ownedState && session.bindings && session.ownedState->world &&
        session.ownedState->race && session.ownedState->bindings.players &&
        session.ownedState->bindings.players->registry,
        "RaceCoordinator snapshot has no actual owned World/Race/Player graph");
    auto& own=*session.ownedState;
    auto& players=*own.bindings.players->registry;
    replaySnapshotBeginSection(&snapshot,&session,"RaceCoordinator");
    worldDoSnapshot(own.world.get(),&snapshot);
    raceFrameOwnerDoSnapshot(own.race.get(),&snapshot);
    hp2::Require(players.count>=0 && players.count<=2,"RaceCoordinator snapshot has an invalid Player count");
    for(int32_t index=0;index<players.count;++index) {
        hp2::Require(players.players[index],"RaceCoordinator snapshot has a missing actual Player");
        carFramePlayerDoSnapshot(players.players[index],&snapshot);
    }
    replaySnapshotEndSection(&snapshot);
}

// Final PAL149988..149B88. The complete instruction correspondence records
// its added eliminated-player tests separately from beta. A native ownership
// adapter retains partially constructed actual owners if an operation fails,
// so the caller can inspect the error and retire them with DisposeOwnedRace.
void raceCoordinatorInitForRace(GameSession& session,const RaceCoordinatorInitBindings& b)
{
    hp2::Require(!session.ownedState && !session.bindings &&
        (!session.processingMessages || session.state==RC_STATE_NONE) &&
        b.parameters && b.players && b.world && b.raceServices && b.dispatch &&
        b.playerCameras && b.catalog && b.route && b.skipStartSequence &&
        b.demoReplay && b.collisionAudio && b.oneShotAudio && b.sounds && b.particles,
        "InitForRace has no complete canonical construction/resource bindings");
    hp2::Require(b.players->registry && b.players->registry->services && b.players->clock &&
        b.players->input && b.players->parameters==b.parameters && b.world->parameters==b.parameters &&
        b.world->clock==b.players->clock && b.world->simulation && b.world->simulation->replay &&
        b.world->traffic && b.world->speech && b.world->cars && b.world->cars->render &&
        b.players->cars==b.world->cars->render->cars && b.dispatch->startScreens &&
        b.dispatch->resources && b.dispatch->cameraRegistry,
        "InitForRace received different Player/Car/World/track resource owners");
    auto& players=*b.players->registry;
    hp2::Require(players.count==0 && !players.players[0] && !players.players[1] &&
        !players.world && !players.services->race && !players.services->replay &&
        !b.world->simulation->world,"InitForRace cannot replace a live race");
    session.ownedState=new RaceCoordinatorOwnedState(b);
    auto& own=*session.ownedState;
    players.services->session=&session;players.services->raceActions=&session.actions;
    players.clock=b.players->clock;
    const auto* first=raceParametersGetDriverInfoByDriverNumber(b.parameters,0);
    if((first && first->eliminated<=0) || b.parameters->fastSpeedTrapSpawns)
        (void)hp2::CreatePlayer(0,0,*b.players);
    const auto* second=raceParametersGetDriverInfoByDriverNumber(b.parameters,1);
    if(second && second->eliminated<=0)(void)hp2::CreatePlayer(1,1,*b.players);
    const auto* scenario=trackScenarioFind(b.catalog,b.parameters->selectedScenario);
    hp2::Require(scenario,"InitForRace has no selected final track scenario");
    own.replay=std::make_unique<ReplayManagerOwner>(*b.demoReplay);
    auto& replay=own.replay->State();
    players.services->replay=&replay;
    auto& simulation=*b.world->simulation;
    simulation.replay->manager=&replay;
    simulation.replay->clock=b.players->clock;
    simulation.replay->raceActions=&session.actions;
    auto& raceServices=*b.raceServices;
    raceServices.settings=b.parameters;raceServices.session=&session;
    raceServices.players=&players;raceServices.replay=&replay;
    own.race=std::make_unique<RaceFrameOwner>();
    hp2::Require(raceFrameOwnerInitialize(own.race.get(),&raceServices,b.players->clock,
        b.route,b.catalog)==WORLD_SERVICES_OK,"InitForRace Race construction failed");
    auto& race=own.race->race;
    if(raceServices.commentary) {
        raceServices.commentary->race=&race;raceServices.commentary->session=&session;
    }
    players.services->race=&race;simulation.race=&race;simulation.players=&players;
    b.sounds->race=&race;b.sounds->session=&session;
    b.world->speech->session=&session;
    own.worldBindings=*b.world;own.trafficBindings=*b.world->traffic;
    own.trafficBindings.race=&race;own.trafficBindings.players=&players;
    own.trafficBindings.parameters=b.parameters;own.worldBindings.traffic=&own.trafficBindings;
    own.world=std::make_unique<World>(own.worldBindings);
    auto& world=*own.world;
    raceServices.world=&world;players.world=&world;
    if(players.services->cameras)players.services->cameras->world=&world;
    raceServices.pursuit=hp2::WorldPursuitManager(world);
    players.services->pursuit=raceServices.pursuit;
    b.players->input->world=&world;
    auto& dispatch=*b.dispatch;
    dispatch.world=&world;dispatch.race=&race;dispatch.replayOwner=own.replay.get();
    dispatch.clock=b.players->clock;dispatch.specialPursuit=&b.parameters->specialPursuit;
    auto& screens=dispatch.startScreens->services;
    screens.race=&race;screens.replay=&replay;screens.players=&players;
    screens.coordinator=&session;screens.currentTrack=scenario;
    hp2::Require(raceCoordinatorBindDispatch(session,dispatch)!=0,"InitForRace cannot bind actual state dispatcher");
    hp2::Require(b.world->cars->render->characters,"InitForRace replay requires the loaded actual character catalogue");
    own.replay->BindRace(session,*b.parameters,*b.players->clock,*b.world->cars->render->characters,*b.players->input);
    raceCoordinatorInitCars(world,*b.parameters,*b.catalog);
    own.playerCameraContext=*b.playerCameras;
    hp2::Require(world.totalCars>=0 && world.totalCars<=WORLD_MAX_CARS,
        "InitForRace camera view exceeds the actual World car table");
    for(int32_t index=0;index<world.totalCars;++index) {
        auto* car=world.carTable[index];
        hp2::Require(car && car->driverInfo,"InitForRace camera view has no actual Car owner");
        const auto& type=b.players->cars->Car(static_cast<std::size_t>(car->driverInfo->appearance.modelCode));
        own.cameraVehicles[static_cast<std::size_t>(index)]={car->Movement(),&type.cameraCatalogue,&type.cameraTargetClearance};
        if(car->driverInfo->driverClass==1 && type.bodyCategory==2 &&
           (!type.cameraCatalogue.viewCount[0] || !type.cameraCatalogue.viewCount[1])) {
            const auto seat=car->driverInfo->driverNumber;
            hp2::Require(seat>=0 && seat<2,"Traffic player camera has no real player seat");
            auto& fitted=own.trafficPlayerCameras[static_cast<std::size_t>(seat)];
            fitTrafficPlayerCameras(fitted,*car,*b.players->cars);
            own.cameraVehicles[static_cast<std::size_t>(index)].catalogue=&fitted;
        }
        own.cameraMovements[static_cast<std::size_t>(index)]=car->Movement();
    }
    own.playerCameraContext.chaseVehicles=own.cameraVehicles.data();
    own.playerCameraContext.movements=own.cameraMovements.data();
    own.playerCameraContext.vehicleCount=static_cast<std::size_t>(world.totalCars);
    dispatch.cameraRegistry->environment.vehicles=own.cameraVehicles.data();
    dispatch.cameraRegistry->environment.vehicleCount=static_cast<std::size_t>(world.totalCars);
    // The phase embeds the canonical registry, so its original per-Car
    // replay presentation borrows must be published with this actual table.
    // A separate carViewMoverRegistryCreate allocator would duplicate owners.
    auto& cameraRegistry=*dispatch.cameraRegistry;
    hp2::Require(cameraRegistry.presentationCount==0,
        "InitForRace cannot replace live camera presentation owners");
    for(std::size_t index=0;index<static_cast<std::size_t>(world.totalCars);++index) {
        auto* movement=own.cameraVehicles[index].movement;
        hp2::Require(movement && movement->bindings.owner && movement->bindings.state &&
            movement->bindings.ownerMotion,"Camera presentation has no live movement owner");
        auto& presentation=cameraRegistry.presentations[index];
        presentation={}; // Recorded poses are filled only by actual replay producers.
        presentation.car=movement->bindings.owner;
        presentation.state.motion=movement->bindings.ownerMotion;
    }
    cameraRegistry.presentationCount=static_cast<std::size_t>(world.totalCars);
    hp2::Require(carViewCameraResourcesBindMovers(cameraRegistry.environment.resources,&cameraRegistry)!=0,
        "Camera resources are bound to another live mover registry");
    own.cameraResourcesBound=true;
    for(int32_t i=0;i<players.count;++i)
        hp2::Require(hp2::BindPlayerForRace(*players.players[i],own.playerCameraContext),
            "InitForRace Player camera construction failed");
    hp2::Require(raceCoordinatorInitPlayers(session,b.parameters->fastSpeedTrapSpawns)!=0,
        "InitForRace could not attach the actual Players to their Cars");
    // Beta RaceCoordinator::InitForRace 149938 calls DisableRenderEggs
    // after InitPlayers 149920, before Race::Init(stage) 14999C.
    // Final PAL 24C000 clears active renderer mutations first.
    if(b.parameters->fastSpeedTrapSpawns)
        DisableRenderEggs();
    int32_t nextState=RC_STATE_RACE_IN_PROGRESS,raceStage=4;
    if(b.parameters->fastSpeedTrapSpawns)nextState=RC_STATE_AI_DEMO;
    else if(b.parameters->demoReplay)nextState=RC_STATE_REPLAY_DEMO;
    else if(!*b.skipStartSequence) {
        if(b.parameters->specialPursuit || b.dispatch->resources->FindResourceFile("GLOBAL\\PRERACEANIM.LZC")) {
            nextState=RC_STATE_CAMERA_FLYBY;raceStage=1;
        } else raceStage=3;
    }
    hp2::Require(raceFrameOwnerInitStage(own.race.get(),raceStage,
        hp2::WorldCarBindings(world).movement->recoveryPoints)==WORLD_SERVICES_OK,
        "InitForRace could not initialize actual RacingCar/time-around-track owners");
    // Final SynchronizeCars1485E0 is stack adjustment, JR RA, restore only.
    // No empty runtime stub is introduced for that excluded source body.
    SetAIDriverModesFromPlayers(&world,&race,*b.parameters,players);
    hp2::Require(collisionAudioReset(b.collisionAudio)==WORLD_AUDIO_OK,
        "InitForRace could not release actual detached collision voices");
    hp2::Require(worldSpeechFlush(b.world->speech)==WORLD_SPEECH_OK,
        "InitForRace could not flush the real speech manager");
    hp2::Require(raceCoordinatorChangeState(&session,nextState)!=0,
        "InitForRace selected an unavailable original state dependency");
}

// PAL149B88 CleanupAfterRace. The same function also retires the partial
// native ownership graph after a checked construction failure. AFX1276A8 is
// KillAllActiveEffects; authored auto emitters remain in their actual owner.
void raceCoordinatorDisposeOwnedRace(GameSession& session)
{
    auto* owned=session.ownedState;
    if(!owned)return;
    hp2::Require(!session.processingMessages || session.state==RC_STATE_NONE,
        "Cannot retire owned race outside the source StateNone restart transition");
    auto& b=owned->bindings;
    auto& players=*b.players->registry;
    if(session.state==RC_STATE_AI_DEMO)coordinatorEndAIDemo(session);
    if(session.state==RC_STATE_REPLAY_DEMO)
        hp2::Require(coordinatorEndReplayDemo(session),
            "Replay-demo retirement could not reset actual joystick handlers");
    // Normal state exit checks Close() above. Partial construction or
    // exception retirement uses its noexcept destructor, which closes
    // every borrower and records failure without replacing the first error.
    owned->replayScreen.reset();
    if(session.preRaceCamera)carViewMoverDispose(session.preRaceCamera);
    session.preRaceCamera=nullptr;
    if(session.flybyCamera)carViewMoverDispose(session.flybyCamera);
    session.flybyCamera=nullptr;
    gameSessionCameraDeactivate(session.cameras);
    // A failed native construction can leave an unattached Player. The PAL
    // Detach method requires a Car, so only that native exception case skips
    // the call; a fully initialized race visits every original Player.
    for(int32_t i=0;i<players.count;++i) {
        auto* player=players.players[i];
        // A direct native close can arrive with a live driving HUD. Its
        // SetHudFeatures source body borrows Race and cameras while hiding
        // the live split rows and removing overlays. Retire those actual
        // dependencies before detach/owner destruction; partial startup
        // has no active HUD and must not require an unpublished Race.
        if(carFramePlayerHudFeatures(player)!=0)
            hp2::Require(carFramePlayerSetHudFeatures(player,0)!=0,
                "Owned race retirement could not release the actual HUD dependencies");
        if(player->car)
            hp2::Require(carFramePlayerDetachFromCar(player)!=0,
                "Owned race retirement could not detach the actual Player");
    }
    // Detach/restore above still needs the real Car motion records. Retire
    // these borrowed views now, before CleanupCars destroys their owners.
    auto& cameraRegistry=*b.dispatch->cameraRegistry;
    cameraRegistry.presentationCount=0;
    for(auto& presentation:cameraRegistry.presentations)presentation={};
    cameraRegistry.environment.vehicles=nullptr;
    cameraRegistry.environment.vehicleCount=0;
    if(owned->cameraResourcesBound) {
        hp2::Require(carViewCameraResourcesBindMovers(cameraRegistry.environment.resources,nullptr)!=0,
            "Camera resource registry could not retire its mover binding");
        owned->cameraResourcesBound=false;
    }
    if(owned->world)raceCoordinatorCleanupCars(*owned->world);
    worldParticlesKillAllActiveEffects(b.particles);
    if(owned->race) {
        raceFrameOwnerDispose(owned->race.get());
        owned->race.reset();
    }
    b.raceServices->world=nullptr;b.raceServices->pursuit=nullptr;
    players.services->race=nullptr;players.services->pursuit=nullptr;
    b.sounds->race=nullptr;
    b.world->simulation->race=nullptr;
    owned->world.reset();
    players.world=nullptr;b.players->input->world=nullptr;
    if(players.services->cameras)players.services->cameras->world=nullptr;
    hp2::Require(soundJukeboxStop(*b.sounds)==WORLD_AUDIO_OK,
        "Owned race retirement could not stop the actual jukebox");
    hp2::Require(b.oneShotAudio->Reset()==WORLD_AUDIO_OK,
        "Owned race retirement could not delete actual one-shot voices");
    owned->replay.reset();
    players.services->replay=nullptr;b.raceServices->replay=nullptr;
    b.world->simulation->replay->manager=nullptr;
    if(owned->endScreens)
        if(auto failure=owned->endScreens->Close())std::rethrow_exception(failure);
    owned->endScreens.reset();
    while(players.count)
        hp2::Require(hp2::DestroyPlayer(players.players[players.count-1]),
            "Owned race retirement could not destroy the actual Player");
    auto& screens=b.dispatch->startScreens->services;
    screens.race=nullptr;screens.replay=nullptr;screens.currentTrack=nullptr;
    b.dispatch->race=nullptr;b.dispatch->world=nullptr;b.dispatch->replayOwner=nullptr;
    if(b.raceServices->commentary) {
        b.raceServices->commentary->race=nullptr;b.raceServices->commentary->session=nullptr;
    }
    session.bindings=nullptr;session.ownedState=nullptr;
    delete owned;
}

namespace {
struct CoordinatorCloseErrors {
    std::exception_ptr first;
    void Remember(std::exception_ptr error) noexcept {if(!first)first=error;}
    template<class Operation> void Run(Operation&& operation) noexcept {
        try {operation();}catch(...) {Remember(std::current_exception());}
    }
};

// Terminal cleanup does not enter a new state or stop at a presentation or
// audio failure. The ordinary checked restart API remains separate above.
void CloseOwnedRace(GameSession& session,CoordinatorCloseErrors& errors) noexcept
{
    auto* owned=session.ownedState;
    if(!owned)return;
    auto& b=owned->bindings;
    auto& players=*b.players->registry;
    auto& input=*b.players->input;
    for(size_t index=0;index<input.count;) {
        const auto& target=input.registrations[index].target;
        if(target.kind!=GAME_INPUT_RACE_COORDINATOR || target.owner.raceCoordinator!=&session) {++index;continue;}
        for(size_t next=index+1;next<input.count;++next)input.registrations[next-1]=input.registrations[next];
        --input.count;
    }
    for(auto& event:input.queuedEvents)
        if(event.target.kind==GAME_INPUT_RACE_COORDINATOR && event.target.owner.raceCoordinator==&session)
            event.active=0;
    owned->demoInputBound={};
    errors.Run([&]{b.demoReplay->EndDemoMode();});
    if(owned->endScreens)errors.Remember(owned->endScreens->Close());
    if(owned->replayScreen)errors.Run([&]{owned->replayScreen->Close();});
    owned->replayScreen.reset();
    errors.Run([&]{if(session.preRaceCamera)carViewMoverDispose(session.preRaceCamera);session.preRaceCamera=nullptr;});
    errors.Run([&]{if(session.flybyCamera)carViewMoverDispose(session.flybyCamera);session.flybyCamera=nullptr;});
    errors.Run([&]{gameSessionCameraDeactivate(session.cameras);});
    for(int32_t index=0;index<players.count;++index)
        errors.Remember(hp2::DetachPlayerForShutdown(players.players[index]));
    auto& cameraRegistry=*b.dispatch->cameraRegistry;
    cameraRegistry.presentationCount=0;
    for(auto& presentation:cameraRegistry.presentations)presentation={};
    cameraRegistry.environment.vehicles=nullptr;cameraRegistry.environment.vehicleCount=0;
    if(owned->cameraResourcesBound) {
        errors.Run([&]{hp2::Require(carViewCameraResourcesBindMovers(cameraRegistry.environment.resources,nullptr)!=0,
            "Terminal camera resources could not retire their mover registry");});
        owned->cameraResourcesBound=false;
    }
    if(owned->world)errors.Run([&]{raceCoordinatorCleanupCars(*owned->world);});
    errors.Run([&]{worldParticlesKillAllActiveEffects(b.particles);});
    if(owned->race) {
        errors.Run([&]{raceFrameOwnerDispose(owned->race.get());});
        owned->race.reset();
    }
    b.raceServices->world=nullptr;b.raceServices->pursuit=nullptr;
    players.services->race=nullptr;players.services->pursuit=nullptr;
    b.sounds->race=nullptr;b.sounds->session=nullptr;
    b.world->speech->session=nullptr;
    b.world->simulation->race=nullptr;
    owned->world.reset();
    players.world=nullptr;input.world=nullptr;
    if(players.services->cameras)players.services->cameras->world=nullptr;
    errors.Run([&]{hp2::Require(soundJukeboxStop(*b.sounds)==WORLD_AUDIO_OK,
        "Terminal race jukebox retirement failed");});
    errors.Run([&]{hp2::Require(b.oneShotAudio->Reset()==WORLD_AUDIO_OK,
        "Terminal race one-shot voice retirement failed");});
    owned->replay.reset();
    players.services->replay=nullptr;b.raceServices->replay=nullptr;
    b.world->simulation->replay->manager=nullptr;
    owned->endScreens.reset();
    while(players.count) {
        auto* player=players.players[players.count-1];
        if(!player) {--players.count;continue;}
        errors.Remember(hp2::DestroyPlayerForShutdown(player));
    }
    auto& screens=b.dispatch->startScreens->services;
    screens.race=nullptr;screens.replay=nullptr;screens.currentTrack=nullptr;screens.coordinator=nullptr;
    b.dispatch->race=nullptr;b.dispatch->world=nullptr;b.dispatch->replayOwner=nullptr;
    players.services->session=nullptr;players.services->raceActions=nullptr;players.sessionState=nullptr;
    b.raceServices->session=nullptr;
    b.world->simulation->replay->raceActions=nullptr;
    if(b.raceServices->commentary) {
        b.raceServices->commentary->race=nullptr;b.raceServices->commentary->session=nullptr;
    }
    session.bindings=nullptr;session.ownedState=nullptr;
    delete owned;
}

int32_t coordinatorFailure(GameSession* session,RaceCoordinatorResult result)
{
    if(session)session->result=result;
    return 0;
}

// Full source callback14ABE0..14ACF4. Native failures are reported through
// the existing session boundary result; source callback booleans are not
// used as a simulation of successful startup.
int32_t coordinatorFlybyMessage(GameSession& session,int32_t kind,int32_t argument)
{
    auto& binding=*session.bindings;
    switch(kind) {
    case 1: {
        if(!session.flybyCamera) {
            CarViewMoverResult status;
            session.flybyCamera=carViewMoverCreateForFlyby(binding.cameraRegistry,1,&status);
            if(!session.flybyCamera)return coordinatorFailure(&session,RC_RESULT_CAMERA_FAILURE);
        }
        hp2::EnableStartRaceScreens(*binding.startScreens);
        return 1;
    }
    case 2:
        hp2::DisableStartRaceScreens(*binding.startScreens);
        return 1;
    case 3:
        if(!carViewMoverFlybyFinished(session.flybyCamera))return 1;
        [[fallthrough]];
    case 4:
        if(raceFrameSetState(binding.race,2)!=WORLD_SERVICES_OK)
            return coordinatorFailure(&session,RC_RESULT_RACE_FAILURE);
        return raceCoordinatorChangeState(&session,RC_STATE_PRE_RACE);
    case 5:
        session.pausingJoystickPort=argument;
        session.stateBeforePause=session.state;
        return raceCoordinatorChangeState(&session,RC_STATE_PAUSE);
    default:return 0;
    }
}

// Full14ACF8..14AE80. Its exit retires the actual optional pre-race animation
// ResourceFile and clears animations on the original World intrusive list.
int32_t coordinatorPreRaceMessage(GameSession& session,int32_t kind,int32_t argument)
{
    auto& binding=*session.bindings;
    switch(kind) {
    case 1: {
        if(!session.preRaceCamera) {
            CarViewMoverResult status;
            session.preRaceCamera=carViewMoverCreateForPreRace(binding.cameraRegistry,1,
                *binding.specialPursuit,&status);
            if(!session.preRaceCamera)return coordinatorFailure(&session,RC_RESULT_CAMERA_FAILURE);
        }
        if(session.flybyCamera) {
            carViewMoverDispose(session.flybyCamera);
            session.flybyCamera=nullptr;
        }
        return 1;
    }
    case 2: {
        if(!binding.world->carList.next || !binding.world->carList.previous)
            return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
        auto* const sentinel=&binding.world->carList;
        for(auto* car=sentinel->next;car!=sentinel;car=car->next) {
            if(!car)return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
            if(car->visual)carDriverVisualClearAnimations(car->visual);
        }
        if(auto* file=binding.resources->FindResourceFile("GLOBAL\\PRERACEANIM.LZC"))
            binding.resources->UnloadResourceFile(file);
        return 1;
    }
    case 3:
        if(binding.race->state!=2 || !carViewMoverPreRaceFinished(session.preRaceCamera))return 1;
        [[fallthrough]];
    case 4:
        if(raceFrameSetState(binding.race,3)!=WORLD_SERVICES_OK)
            return coordinatorFailure(&session,RC_RESULT_RACE_FAILURE);
        return raceCoordinatorChangeState(&session,RC_STATE_RACE_IN_PROGRESS);
    case 5:
        session.pausingJoystickPort=argument;
        session.stateBeforePause=session.state;
        return raceCoordinatorChangeState(&session,RC_STATE_PAUSE);
    default:return 0;
    }
}

bool coordinatorDelayExpired(const CarFramePlayerState& player,const GameTime& clock)
{
    // PAL ADDU wraps before the signed SLT. Both fields are the existing
    // Player-owned world-frame timestamp/length cells (+50/+54).
    // PAL14AF90/14AFE0 read worldTimeFrames at330C84 for both deadlines.
    const uint32_t bits=static_cast<uint32_t>(player.drivingLocked)+
        static_cast<uint32_t>(player.finishCounter);
    int32_t end;std::memcpy(&end,&bits,sizeof(end));
    return end<clock.worldTimeFrames;
}

bool coordinatorDrivingOwnersAvailable(const GameSession& session)
{
    const auto* b=session.bindings;
    if(!b || !b->replayOwner || !b->race->services || !b->race->services->players ||
        !b->startScreens->services.frontend)return false;
    const auto& services=b->startScreens->services;
    const auto* menus=services.frontend;
    const auto* players=b->race->services->players;
    if(!menus->ui || !menus->ui->resources || !menus->ui->render ||
        !menus->sounds || !menus->sounds->audio || !menus->sounds->globals ||
        services.players!=players || players->world!=b->world ||
        players->clock!=b->clock || players->count<1 || players->count>2 ||
        b->race->clock!=b->clock || b->race->services->world!=b->world ||
        b->race->services->replay!=&b->replayOwner->State() ||
        services.replay!=&b->replayOwner->State() ||
        b->cameraRegistry->environment.simulation->players!=players)return false;
    for(int32_t i=0;i<players->count;++i)
        if(!players->players[i] || players->players[i]->registry!=players)return false;
    return true;
}

bool coordinatorRacingNeedsFinish(const GameSession& session,int32_t kind)
{
    if(kind!=3 && kind!=4)return false;
    const auto& b=*session.bindings;
    if(!carFramePlayerAllDrivingLocked(b.race->services->players))return false;
    if(kind==4)return true;
    const auto* recent=raceFrameMostRecentlyFinishedPlayer(b.race);
    // A native invalid-domain timer cannot be silently treated as an
    // unfinished race. The source dereferences the returned live Player.
    return !recent || coordinatorDelayExpired(*recent,*b.clock);
}

// PAL14AE80..14B108: actual moving/countdown/spectator branches. Terminal
// TheRaceHasFinished14BCE0 and AutoReplay/return lifecycle remain explicit
// gates in coordinatorCanDispatch; this callback is not a whole-body count.
int32_t coordinatorRacingMessage(GameSession& session,int32_t kind,int32_t argument)
{
    auto& b=*session.bindings;
    auto& menus=*b.startScreens->services.frontend;
    auto& ui=*menus.ui;
    auto& audio=*menus.sounds->audio;
    auto& players=*b.race->services->players;
    switch(kind) {
    case 1:
        if(session.preRaceCamera) {
            carViewMoverDispose(session.preRaceCamera);
            session.preRaceCamera=nullptr;
        }
        return 1;
    case 2:
        raceCoordinatorDisplayCountdown(ui,audio,*b.race,*b.specialPursuit,0);
        return 1;
    case 3: {
        const auto countdown=raceFrameCountdownStep(b.race,*b.specialPursuit);
        raceCoordinatorDisplayCountdown(ui,audio,*b.race,*b.specialPursuit,countdown);
        auto& replay=b.replayOwner->State();
        if((replay.beginCapturingTime==0 || replay.beginCapturingTime==INT32_MAX) && b.race->state==4) {
            if(!b.replayAvailableBytes)return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
            b.replayOwner->MarkTheBeginningOfTime(*b.clock,*b.replayAvailableBytes);
            // PAL's following bCountFreeMemory(16) only gathers a diagnostic
            // allocation metric; it does not write a gameplay state cell.
            if(soundJukeboxDelayedStart(*menus.sounds,6.0f)!=WORLD_AUDIO_OK)
                return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
        }
        if(coordinatorRacingNeedsFinish(session,kind) && !raceCoordinatorTheRaceHasFinished(session))return 0;
        if(players.count<2)return 1;
        for(int32_t i=0;i<players.count;++i) {
            auto* player=players.players[i];
            if(!player->drivingLocked || !coordinatorDelayExpired(*player,*b.clock) ||
                carFramePlayerAllDrivingLocked(b.race->services->players))continue;
            for(int32_t other=0;other<players.count;++other) {
                if(other==i || players.players[other]->drivingLocked)continue;
                // The released call176868 is BeginDemoCameras(Car*), not
                // a car reassignment. Keep the finished Player's own Car.
                if(carFramePlayerSetScriptedCamera(player,players.players[other]->car)!=CAR_CINEMATIC_OK)
                    return coordinatorFailure(&session,RC_RESULT_CAMERA_FAILURE);
                break;
            }
        }
        return 1;
    }
    case 4:
        return carFramePlayerAllDrivingLocked(b.race->services->players) ?
            raceCoordinatorTheRaceHasFinished(session) : 1;
    case 5:
        session.pausingJoystickPort=argument;
        session.stateBeforePause=session.state;
        return raceCoordinatorChangeState(&session,RC_STATE_PAUSE);
    case 13:return 1;
    case 9:
        if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
        return raceCoordinatorSendMessage(&session,9,argument);
    default:return 0;
    }
}

// PAL14B3D8..14B568. BeginPause14D668/EndPause14D670 are proved JR RA/NOP
// calls and are folded, with no runtime stubs and no invented pause flag.
// Restart, quit and instant replay dispatch through their actual state owners.
int32_t coordinatorPauseMessage(GameSession& session,int32_t kind,int32_t argument)
{
    auto& b=*session.bindings;
    auto& services=b.startScreens->services;
    auto& menus=*services.frontend;
    switch(kind) {
    case 1:
        if(!carFramePlayerHideHUD(services.players))
            return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
        b.pauseMenu->EnterPause(session.pausingJoystickPort);
        menus.screens->ChangeToScreen("Pause Menu",true);
        if(soundMute(*menus.sounds,b.cameraRegistry->environment.simulation,true)!=WORLD_AUDIO_OK)
            return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
        // Platform adaptation: mute matches PAL but does not pause the
        // native XAudio2 speech cursor. Preserve the current sentence.
        if (soundDriverState.speechEnabled && menus.sounds->globals &&
            menus.sounds->globals->policy.audioSystemEnabled &&
            worldSpeechPausePlayback(worldSpeechGetInstance(),1)!=WORLD_SPEECH_OK)
            return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
        return 1;
    case 2:
        menus.screens->ChangeToScreen(nullptr,false);
        b.pauseMenu->ExitPause();
        if(!carFramePlayerShowHUD(services.players))
            return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
        return 1;
    case 3:return 1;
    case 6:
        if(argument==1) {
            if(soundMute(*menus.sounds,b.cameraRegistry->environment.simulation,false)!=WORLD_AUDIO_OK)
                return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
            if (soundDriverState.speechEnabled && menus.sounds->globals &&
                menus.sounds->globals->policy.audioSystemEnabled &&
                worldSpeechPausePlayback(worldSpeechGetInstance(),0)!=WORLD_SPEECH_OK)
                return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
            return raceCoordinatorChangeState(&session,session.stateBeforePause);
        }
        if(argument==2) {
            if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
            return raceCoordinatorSendMessage(&session,8,0);
        }
        if(argument==3) {
            menus.frontend->TheRaceWasQuit(*b.race,*menus.raceSetup,*menus.sounds);
            if(b.demoDisc && b.demoDisc->ActivationWord())b.demoDisc->SetEndReason(5);
            if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
            return raceCoordinatorSendMessage(&session,10,0);
        }
        if(argument==4)return raceCoordinatorChangeState(&session,RC_STATE_INSTANT_REPLAY);
        return 1;
    default:return 0;
    }
}

int32_t coordinatorNoneMessage(GameSession& session,int32_t kind,int32_t argument)
{
    if(kind>=1 && kind<=3)return 1;
    if(kind<8 || kind>10)return 0;
    auto& bindings=*session.bindings;
    auto& menus=*bindings.startScreens->services.frontend;
    auto& frontend=*menus.frontend;
    const auto& setup=*menus.raceSetup;
    hp2::Require(setup.parameters && setup.flow && setup.runtimeSettings,
        "StateNone has no canonical frontend/settings/flow owners");
    if(kind==8) {
        hp2::Require(session.ownedState,"StateNone restart has no owned InitForRace bindings");
        const auto construction=session.ownedState->bindings;
        raceCoordinatorDisposeOwnedRace(session);
        raceCoordinatorInitForRace(session,construction);
        hp2::Require(soundMute(*menus.sounds,bindings.cameraRegistry->environment.simulation,false)==WORLD_AUDIO_OK,
            "StateNone restart could not unmute the actual sound graph");
        // Restart retires the old race, including its pause-menu owner, but
        // reuses the localized speech stream. Flush preserves a menu pause
        // for normal queue operations; release it at this owner transition.
        if (soundDriverState.speechEnabled && menus.sounds->globals &&
            menus.sounds->globals->policy.audioSystemEnabled)
            hp2::Require(worldSpeechPausePlayback(worldSpeechGetInstance(),0)==WORLD_SPEECH_OK,
                "StateNone restart could not resume native speech playback");
    } else if(kind==9 && argument && frontend.UseTheRaceEvent()) {
        hp2::Require(setup.currentEvent,"StateNone continuation has no canonical current event");
        (void)setup.currentEvent->SetupNextRace(frontend,setup);
    }
    if(kind==8 || kind==9) {
        for(int32_t seat=0;seat<setup.parameters->mode;++seat)frontend.SetRecordTimesToBeat(seat,setup);
        setup.runtimeSettings->display=frontend.displaySettings;
        if(kind==8)soundDriverState.delayedJukeboxSeconds=8999999488.0f; // final GP-7A98 -> GP-4A54
        else hp2::ChangeGameFlow(*setup.flow,7,5);
        return 1;
    }
    bool exitDemo=false;
    if(bindings.demoDisc && bindings.demoDisc->ActivationWord()) {
        auto& demo=*bindings.demoDisc;
        if(demo.IsAttractMode()) {
            if(frontend.pendingState==2)demo.SetEndReason(0);
            else {
                hp2::Require(session.ownedState && session.ownedState->bindings.demoReplay,
                    "StateNone demo exit has no actual DemoModeReplay owner");
                if(session.ownedState->bindings.demoReplay->selectedRecording<5) {
                    hp2::ChangeGameFlow(*setup.flow,7,9);return 1;
                }
                demo.SetEndReason(1);
            }
        }
        exitDemo=true;
    }
    hp2::ChangeGameFlow(*setup.flow,7,exitDemo?9:0);return 1;
}

int32_t coordinatorPostRaceMessage(GameSession& session,int32_t kind,int32_t argument)
{
    auto& binding=*session.bindings;
    auto& owned=*session.ownedState;
    auto& services=binding.startScreens->services;
    auto& menus=*services.frontend;
    auto& players=*services.players;
    auto& replay=binding.replayOwner->State();
    const auto restartCamera=[&]() {
        gameSessionCameraDeactivate(session.cameras);
        const auto seat=session.viewSelection->seat;
        hp2::Require(seat>=0 && seat<2 && players.playersByNumber[seat] && players.playersByNumber[seat]->car,
            "PostRace demo camera has no selected actual Player/Car");
        CarViewMoverResult result;
        session.cameras->current=carViewMoverCreateForCar(binding.cameraRegistry,1,
            players.playersByNumber[seat]->car,&result);
        hp2::Require(session.cameras->current && result==CAR_VIEW_MOVER_OK,"PostRace demo camera allocation failed");
    };
    switch(kind) {
    case 1: {
        for(int32_t i=0;i<players.count;++i)
            hp2::Require(carFramePlayerChangeCamera(players.players[i],0,0)==CAR_CINEMATIC_OK,
                "PostRace could not retire an actual Player camera");
        hp2::Require(soundMute(*menus.sounds,binding.cameraRegistry->environment.simulation,false)==WORLD_AUDIO_OK,
            "PostRace could not unmute actual sound");
        hp2::Require(worldSpeechFlush(owned.bindings.world->speech)==WORLD_SPEECH_OK,
            "PostRace could not flush actual speech");
        if(replay.desynchronized)
            hp2::Require(soundMuteReplay(*menus.sounds,binding.cameraRegistry->environment.simulation,true)==WORLD_AUDIO_OK,
                "PostRace replay mute failed");
        binding.replayOwner->BeginRaceHighlights(*binding.clock);
        owned.endScreens->ResumeScreens();
        session.viewSelection->seat=0;
        if(players.count>=2) {
            int32_t best=8;
            for(int32_t i=0;i<players.count;++i) {
                const auto seat=players.players[i]->movement.seat;
                const auto* stats=menus.frontend->transientStats.GetDriver(seat);
                if(stats && stats->position<best){best=stats->position;session.viewSelection->seat=seat;}
            }
        }
        restartCamera();return 1;
    }
    case 2:
        owned.endScreens->SuspendScreens();gameSessionCameraDeactivate(session.cameras);
        session.viewSelection->seat=0;
        hp2::Require(menus.screens->input && menus.screens->input->router,
            "PostRace exit has no actual joystick router");
        hp2::Require(joystickResetTypeHandlers(menus.screens->input->router)==GAME_INPUT_OK,
            "PostRace exit could not reset actual type handlers");return 1;
    case 3:
        if(binding.clock->worldLoopCounter>=replay.endCurrentHighlightTime) {
            binding.replayOwner->BeginNextRaceHighlight(*binding.clock);restartCamera();
        }
        return 1;
    case 8:
        menus.frontend->TheRaceWasRestarted(*menus.raceSetup);
        if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
        return raceCoordinatorSendMessage(&session,8,0);
    case 9:case 10:
        if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
        return raceCoordinatorSendMessage(&session,kind,kind==9?argument:0);
    case 11:
        hp2::Require(soundMuteReplay(*menus.sounds,binding.cameraRegistry->environment.simulation,true)==WORLD_AUDIO_OK,
            "PostRace next highlight could not mute actual replay sound");
        binding.replayOwner->BeginNextRaceHighlight(*binding.clock);restartCamera();return 1;
    case 12:return raceCoordinatorChangeState(&session,RC_STATE_INSTANT_REPLAY);
    case 13:return 1;
    default:return 0;
    }
}

// PAL14A8C0..14AA38 and the thirteen-entry table at2AF920. AI attract
// mode is distinct from recorded playback and uses the live Race timer.
int32_t coordinatorAIDemoMessage(GameSession& session,int32_t kind)
{
    auto& binding=*session.bindings;
    auto& owner=*session.ownedState;
    auto& menus=*binding.startScreens->services.frontend;
    auto& input=*owner.bindings.players->input;
    switch(kind) {
    case 1: {
        auto& ui=*menus.ui;
        ui.render->SetScreenVisibility(*ui.resources,0xcc28b8a9u,0);
        GameInputTarget target{};target.kind=GAME_INPUT_RACE_COORDINATOR;
        target.owner.raceCoordinator=&session;
        for(int32_t port=0;port<2;++port) {
            if(gameInputRegister(&input,port,22,target)!=GAME_INPUT_OK)
                return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
            owner.demoInputBound[static_cast<std::size_t>(port)]=true;
        }
        if(gameSessionCameraActivate(session.cameras)!=CAR_VIEW_MOVER_OK)
            return coordinatorFailure(&session,RC_RESULT_CAMERA_FAILURE);
        return 1;
    }
    case 2:coordinatorEndAIDemo(session);return 1;
    case 3:
        // PAL32FEB8 is the configured duration (authored default120s).
        // The strict comparison leaves the exact endpoint in the Demo.
        if(!(applicationLaunchOptions.demoDurationSeconds<
            static_cast<float>(binding.race->elapsedTicks)*0.0002500000118743628f))return 1;
        menus.frontend->pendingState=1;
        if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
        return raceCoordinatorSendMessage(&session,10,0);
    case 4:
        menus.frontend->pendingState=2;
        [[fallthrough]];
    case 10:
        if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
        return raceCoordinatorSendMessage(&session,10,0);
    case 13:return 1;
    default:return 0;
    }
}

// Full PAL14AA38..14ABDC. Message14 borrows its node from the one retained
// ReplayManager list; queue transport preserves native pointer width.
int32_t coordinatorReplayDemoMessage(GameSession& session,int32_t kind,intptr_t argument)
{
    auto& binding=*session.bindings;
    auto& owner=*session.ownedState;
    auto& demo=*owner.bindings.demoReplay;
    auto& menus=*binding.startScreens->services.frontend;
    auto& input=*owner.bindings.players->input;
    switch(kind) {
    case 1: {
        demo.BeginDemoMode(session);
        auto& ui=*menus.ui;
        ui.render->SetScreenVisibility(*ui.resources,0xcc28b8a9u,0);
        GameInputTarget target{};target.kind=GAME_INPUT_RACE_COORDINATOR;
        target.owner.raceCoordinator=&session;
        for(int32_t port=0;port<2;++port) {
            if(gameInputRegister(&input,port,22,target)!=GAME_INPUT_OK)
                return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
            owner.demoInputBound[static_cast<std::size_t>(port)]=true;
        }
        if(gameSessionCameraActivate(session.cameras)!=CAR_VIEW_MOVER_OK)
            return coordinatorFailure(&session,RC_RESULT_CAMERA_FAILURE);
        return 1;
    }
    case 2:
        if(!coordinatorEndReplayDemo(session))
            return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
        return 1;
    case 3:
        if(demo.ServiceDemoMode(session,*binding.clock))return 1;
        menus.frontend->pendingState=1;
        if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
        return raceCoordinatorSendMessage(&session,10,0);
    case 4:case 11:
        menus.frontend->pendingState=2;
        [[fallthrough]];
    case 10:
        if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
        return raceCoordinatorSendMessage(&session,10,0);
    case 13:return 1;
    case 14: {
        auto* snapshot=reinterpret_cast<ReplaySnapshot*>(argument);
        bool retained=false;
        for(std::size_t index=0;index<binding.replayOwner->ActiveSnapshotCount();++index)
            if(binding.replayOwner->ActiveSnapshot(index)==snapshot) {retained=true;break;}
        if(!retained)return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
        binding.replayOwner->RestoreSnapshot(*snapshot);
        // Mark this exact node as applied only AFTER the real snapshot's
        // World/Car/BitStream participant restoration has succeeded.
        demo.SnapshotRestoreCompleted(*snapshot);
        gameSessionCameraDeactivate(session.cameras);
        if(gameSessionCameraActivate(session.cameras)!=CAR_VIEW_MOVER_OK)
            return coordinatorFailure(&session,RC_RESULT_CAMERA_FAILURE);
        return 1;
    }
    default:return 0;
    }
}

// Complete PAL14B568..14B848. The real ReplayScreen owns replay controls;
// snapshots and the return-to-present point remain in ReplayManager.
int32_t coordinatorInstantReplayMessage(GameSession& session,int32_t kind,intptr_t argument)
{
    auto& owned=*session.ownedState;
    auto& binding=*session.bindings;
    auto& services=binding.startScreens->services;
    auto& menus=*services.frontend;
    auto& players=*services.players;
    auto& replay=*binding.replayOwner;
    switch(kind) {
    case 1: {
        if(owned.endScreens) {
            if(!replay.RestoreSnapshot("LAP_SNAPSHOT"))
                if(!raceCoordinatorChangeState(&session,RC_STATE_POST_RACE))return 0;
        } else {
            if(!replay.TakeSnapshot("THE_PRESENT") || !replay.RestoreSnapshot("INSTANT_REPLAY"))
                return raceCoordinatorChangeState(&session,RC_STATE_PAUSE);
        }
        for(int32_t index=0;index<players.count;++index)
            hp2::Require(carFramePlayerDeactivateCameras(players.players[index])==CAR_CINEMATIC_OK,
                "Instant replay could not deactivate the actual Player cameras");
        int32_t seat=0;
        const auto port=binding.pauseMenu->GetPortThatHitReplay();
        if(port==1)seat=1;
        else if(port==-1) {
            const hp2::TransientStatsElement* best=nullptr;
            for(int32_t index=0;index<players.count;++index) {
                const auto* stats=menus.frontend->transientStats.GetDriver(players.players[index]->movement.seat);
                if(stats && (!best || stats->position<best->position))best=stats;
            }
            if(best)seat=best->driverNumber;
        }
        hp2::Require(!owned.replayScreen,"Instant replay cannot replace a live ReplayScreen");
        owned.replayScreen=std::make_unique<hp2::ReplayScreen>(services,seat);
        return 1;
    }
    case 2:
        if(owned.replayScreen)owned.replayScreen->Close();
        owned.replayScreen.reset();
        hp2::Require(soundMute(*menus.sounds,binding.cameraRegistry->environment.simulation,true)==WORLD_AUDIO_OK,
            "Instant replay exit could not mute the actual sound owner");
        for(int32_t index=0;index<players.count;++index)
            hp2::Require(carFramePlayerActivateCameras(players.players[index])==CAR_CINEMATIC_OK,
                "Instant replay exit could not activate the actual Player cameras");
        hp2::Require(joystickResetTypeHandlers(owned.bindings.players->input)==GAME_INPUT_OK,
            "Instant replay exit could not reset the actual joystick type handlers");
        return 1;
    case 3:
        if(owned.replayScreen)owned.replayScreen->Tick();
        return 1;
    case 11:
        if(owned.endScreens)return raceCoordinatorChangeState(&session,RC_STATE_POST_RACE);
        (void)replay.RestoreSnapshot("THE_PRESENT");
        (void)replay.DeleteSnapshot("THE_PRESENT");
        return raceCoordinatorChangeState(&session,RC_STATE_PAUSE);
    case 13:return 1;
    case 14: {
        auto* snapshot=reinterpret_cast<ReplaySnapshot*>(argument);
        bool retained=false;
        for(std::size_t index=0;index<replay.ActiveSnapshotCount();++index)
            if(replay.ActiveSnapshot(index)==snapshot){retained=true;break;}
        if(!retained || !owned.replayScreen)return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
        replay.RestoreSnapshot(*snapshot);
        // Demo clip/test selection uses the same deferred action14 while
        // ReplayScreen owns this state. Complete only its actual pending
        // node, after restoration, just as in recorded Demo state9.
        auto& demo=*owned.bindings.demoReplay;
        if(demo.IsSnapshotRestorePending())demo.SnapshotRestoreCompleted(*snapshot);
        owned.replayScreen->ChangeCameraMover();
        return 1;
    }
    default:return 0;
    }
}


bool coordinatorCanDispatch(const GameSession& session,const RaceAction& message)
{
    const int32_t state=message.kind==1?static_cast<int32_t>(message.argument):session.state;
    if(state==RC_STATE_INSTANT_REPLAY)
        return session.ownedState && session.bindings && session.bindings->pauseMenu &&
            session.cameras && session.viewSelection && coordinatorDrivingOwnersAvailable(session);
    if(state==RC_STATE_AI_DEMO) {
        if(!coordinatorDrivingOwnersAvailable(session) || !session.ownedState || !session.cameras)return false;
        const auto& construction=session.ownedState->bindings;
        const auto* menus=session.bindings->startScreens->services.frontend;
        return construction.players && construction.players->input && menus->frontend &&
            menus->raceSetup && menus->raceSetup->parameters==construction.parameters;
    }
    if(state==RC_STATE_REPLAY_DEMO) {
        if(!coordinatorDrivingOwnersAvailable(session) || !session.ownedState || !session.cameras)return false;
        const auto& construction=session.ownedState->bindings;
        const auto* menus=session.bindings->startScreens->services.frontend;
        return construction.demoReplay && construction.players && construction.players->input &&
            menus->frontend && menus->raceSetup && menus->raceSetup->demoReplay==construction.demoReplay;
    }
    if(state==RC_STATE_CAMERA_FLYBY || state==RC_STATE_PRE_RACE)return session.bindings!=nullptr;
    if(state==RC_STATE_RACE_IN_PROGRESS) {
        if(!coordinatorDrivingOwnersAvailable(session))return false;
        if(message.kind==7)return false;
        return !coordinatorRacingNeedsFinish(session,message.kind) || session.ownedState;
    }
    if(state==RC_STATE_PAUSE) {
        if(!coordinatorDrivingOwnersAvailable(session))return false;
        const auto& b=*session.bindings;
        if(!b.pauseMenu || &b.pauseMenu->Services()!=&b.startScreens->services ||
            !b.startScreens->services.frontend->screens)return false;
        return true;
    }
    // PAL StateNone's exact compiler table returns1 for1/2/3 and0 for4..7
    // and out-of-range messages, with no owner writes. Restart8, return9 and
    // demo-exit10 have real lifecycle dependencies that remain open.
    if(state==RC_STATE_NONE)return message.kind<8 || message.kind>10 || session.ownedState;
    if(state==RC_STATE_POST_RACE)
        return session.ownedState && session.ownedState->endScreens &&
            session.cameras && session.viewSelection && coordinatorDrivingOwnersAvailable(session);
    // The remaining actual states have not acquired their complete owners.
    // No replacement callback is dispatched, and the pending message remains
    // in the same FIFO until its real dependency is available.
    return false;
}

int32_t coordinatorDispatch(GameSession& session,const RaceAction& message)
{
    if(session.state==RC_STATE_INSTANT_REPLAY)
        return coordinatorInstantReplayMessage(session,message.kind,message.argument);
    if(session.state==RC_STATE_AI_DEMO)
        return coordinatorAIDemoMessage(session,message.kind);
    if(session.state==RC_STATE_REPLAY_DEMO)
        return coordinatorReplayDemoMessage(session,message.kind,message.argument);
    const auto argument=static_cast<int32_t>(message.argument);
    if(session.state==RC_STATE_CAMERA_FLYBY)
        return coordinatorFlybyMessage(session,message.kind,argument);
    if(session.state==RC_STATE_PRE_RACE)
        return coordinatorPreRaceMessage(session,message.kind,argument);
    if(session.state==RC_STATE_RACE_IN_PROGRESS)
        return coordinatorRacingMessage(session,message.kind,argument);
    if(session.state==RC_STATE_PAUSE)
        return coordinatorPauseMessage(session,message.kind,argument);
    if(session.state==RC_STATE_NONE)
        return coordinatorNoneMessage(session,message.kind,argument);
    if(session.state==RC_STATE_POST_RACE)
        return coordinatorPostRaceMessage(session,message.kind,argument);
    throw std::logic_error("RaceCoordinator attempted an unclosed source state callback");
}
}

// Complete PAL149CB8..149D74. The released normal-player branch checks a
// missing racer before attachment; the beta body lacked precisely this check.
int32_t raceCoordinatorInitPlayers(GameSession& session,int32_t aiDemo)
{
    auto* b=session.bindings;
    if(!b || !b->race->services || !b->race->services->players)
        return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
    auto& players=*b->race->services->players;
    if(players.count<0 || players.count>2 || players.world!=b->world)
        return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
    for(int32_t i=0;i<players.count;++i) {
        auto* player=players.players[i];
        if(!player || player->registry!=&players)
            return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
        Car* car;
        if(aiDemo) {
            car=worldGetCarByIndex(b->world,b->world->firstGroupEnd+player->movement.seat);
            if(!car)return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
        } else {
            car=worldGetCarByDriverNumber(b->world,player->movement.seat);
            if(!car)continue;
        }
        if(!carFramePlayerAttachToCar(player,car))
            return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
    }
    return 1;
}

// Complete PAL149D78..149DD4; uses creation order, not the by-seat view.
int32_t raceCoordinatorCleanupPlayers(GameSession& session)
{
    auto* b=session.bindings;
    if(!b || !b->race->services || !b->race->services->players)
        return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
    auto& players=*b->race->services->players;
    if(players.count<0 || players.count>2)
        return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
    for(int32_t i=0;i<players.count;++i) {
        if(!players.players[i] || players.players[i]->registry!=&players)
            return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
        if(!carFramePlayerDetachFromCar(players.players[i]))
            return coordinatorFailure(&session,RC_RESULT_RESOURCE_FAILURE);
    }
    return 1;
}

int32_t raceCoordinatorBindDispatch(GameSession& session,RaceCoordinatorBindings& binding)
{
    if((session.processingMessages && session.state!=RC_STATE_NONE) || session.bindings || session.flybyCamera || session.preRaceCamera ||
        !binding.race || !binding.world || !binding.cameraRegistry || !binding.startScreens ||
        !binding.resources || !binding.specialPursuit || !binding.clock ||
        !binding.cameraRegistry->environment.simulation ||
        binding.startScreens->services.coordinator!=&session ||
        binding.startScreens->services.race!=binding.race ||
        binding.cameraRegistry->environment.simulation->world!=binding.world ||
        binding.cameraRegistry->environment.simulation->race!=binding.race)
        return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
    session.bindings=&binding;
    session.result=RC_RESULT_OK;
    return 1;
}

void raceCoordinatorReleaseScreenCameras(GameSession& session)
{
    if(session.processingMessages)
        throw std::logic_error("Cannot release RaceCoordinator cameras during dispatch");
    if(session.preRaceCamera)carViewMoverDispose(session.preRaceCamera);
    session.preRaceCamera=nullptr;
    if(session.flybyCamera)carViewMoverDispose(session.flybyCamera);
    session.flybyCamera=nullptr;
    session.bindings=nullptr;
}

int32_t raceCoordinatorExecuteQueuedMessages(GameSession* session)
{
    if(!session || !session->actions.entries || !session->actions.capacity ||
        session->actions.readIndex>=session->actions.capacity)
        return coordinatorFailure(session,RC_RESULT_INVALID_BINDING);
    if(session->processingMessages)return 1;
    session->result=RC_RESULT_OK;
    while(session->actions.count) {
        const auto& pending=session->actions.entries[session->actions.readIndex];
        if(pending.kind!=14 && (pending.argument<INT32_MIN || pending.argument>INT32_MAX))
            return coordinatorFailure(session,RC_RESULT_INVALID_BINDING);
        if(!coordinatorCanDispatch(*session,pending))
            return coordinatorFailure(session,RC_RESULT_UNIMPLEMENTED_STATE_DEPENDENCY);
        RaceAction message{};
        if(!raceActionsTake(&session->actions,&message))
            return coordinatorFailure(session,RC_RESULT_INVALID_BINDING);
        session->processingMessages=1;
        if(message.kind==1)session->state=static_cast<int32_t>(message.argument);
        try {
            // Original Execute ignores the source callback's boolean.
            (void)coordinatorDispatch(*session,message);
        } catch(...) {
            session->processingMessages=0;
            session->result=RC_RESULT_RESOURCE_FAILURE;
            throw;
        }
        if(message.kind==2)session->state=RC_STATE_NONE;
        session->processingMessages=0;
        if(session->result!=RC_RESULT_OK)return 0;
    }
    return 1;
}

int32_t raceCoordinatorSendMessage(GameSession* session,int32_t kind,intptr_t argument)
{
    if(!session)return 0;
    if(!raceActionsEnqueue(&session->actions,kind,argument))
        return coordinatorFailure(session,RC_RESULT_QUEUE_CAPACITY);
    return session->processingMessages?1:raceCoordinatorExecuteQueuedMessages(session);
}

int32_t raceCoordinatorChangeState(GameSession* session,int32_t state)
{
    if(!session)return 0;
    // The source allocating queue has no fixed capacity. A nested native
    // transition reserves the three actual messages before modifying it.
    if(session->processingMessages && session->actions.capacity-session->actions.count<3)
        return coordinatorFailure(session,RC_RESULT_QUEUE_CAPACITY);
    if(!raceCoordinatorSendMessage(session,2,session->state))return 0;
    if(!raceCoordinatorSendMessage(session,1,state))return 0;
    return raceCoordinatorSendMessage(session,3,0);
}

int32_t raceCoordinatorCheckForDemoDiscTimeout(GameSession* session)
{
    if(!session || !session->bindings || !session->bindings->demoDisc || !session->bindings->clock) {
        coordinatorFailure(session,RC_RESULT_INVALID_BINDING);
        return 0;
    }
    auto& demo=*session->bindings->demoDisc;
    const auto& clock=*session->bindings->clock;
    if(!demo.ActivationWord() || demo.IsAttractMode())return 0;
    const auto elapsed=[&](int32_t since) {
        const uint32_t bits=static_cast<uint32_t>(clock.realTimer)-static_cast<uint32_t>(since);
        int32_t difference;std::memcpy(&difference,&bits,sizeof(difference));
        return static_cast<float>(difference)*0.0002500000118743628f;
    };
    if(static_cast<float>(demo.Parameters().totalTimeoutSeconds)<elapsed(demo.InitialRealTime())) {
        demo.SetEndReason(3);
        (void)raceCoordinatorChangeState(session,RC_STATE_NONE);
        (void)raceCoordinatorSendMessage(session,10,0);
    }
    if(static_cast<float>(demo.Parameters().inactiveTimeoutSeconds)<elapsed(demo.LastRealTime())) {
        demo.SetEndReason(2);
        (void)raceCoordinatorChangeState(session,RC_STATE_NONE);
        (void)raceCoordinatorSendMessage(session,10,0);
    }
    // Actual return is always zero, independently of whether a timeout fired.
    return 0;
}

struct RaceCoordinatorOwner::Storage {
    GameSession session{};
    std::array<RaceAction,16> messages{};
    std::unique_ptr<hp2::PauseMenu> pause;
    RaceCoordinatorBindings* dispatch{};
    bool pauseBound{},closed{};
    std::exception_ptr closeFailure;
};
RaceCoordinatorOwner::RaceCoordinatorOwner(const RaceCoordinatorInitBindings& bindings,
    GameSessionViewSelection& selection,const GameSessionCameraContext& cameraContext)
    :storage_(std::make_unique<Storage>())
{
    hp2::Require(bindings.dispatch && bindings.dispatch->startScreens && bindings.players &&
        cameraContext.players==bindings.players->registry &&
        cameraContext.coordinators==bindings.dispatch->cameraRegistry,
        "RaceCoordinator constructor received different live camera/player owners");
    auto& owner=*storage_;auto& session=owner.session;
    session.viewSelection=&selection;session.pausingJoystickPort=-1;
    raceActionsInitialize(&session.actions,owner.messages.data(),owner.messages.size());
    owner.dispatch=bindings.dispatch;
    try {
        CarViewMoverResult result;
        session.cameras=gameSessionCamerasCreate(&cameraContext,&result);
        hp2::Require(session.cameras && result==CAR_VIEW_MOVER_OK,"RaceCoordinator camera owner allocation failed");
        hp2::Require(!owner.dispatch->pauseMenu,"RaceCoordinator cannot replace another PauseMenu");
        owner.pause=std::make_unique<hp2::PauseMenu>(owner.dispatch->startScreens->services);
        owner.dispatch->pauseMenu=owner.pause.get();
        hp2::BindPauseMenu(*owner.pause);
        owner.pauseBound=true;
        raceCoordinatorInitForRace(session,bindings);
    } catch(...) {
        // A failed constructor has no destructor. Retire the partial graph
        // here without replacing its initiating exception.
        (void)Close();
        throw;
    }
}
void RaceCoordinatorOwner::Retire()
{
    if(!storage_)return;
    auto& owner=*storage_;auto& session=owner.session;
    if(owner.closed) {
        if(owner.closeFailure)std::rethrow_exception(owner.closeFailure);
        return;
    }
    if(session.state!=RC_STATE_NONE)
        hp2::Require(raceCoordinatorChangeState(&session,RC_STATE_NONE)!=0,
            "RaceCoordinator could not leave its actual state");
    raceCoordinatorDisposeOwnedRace(session);
    gameSessionCamerasDispose(session.cameras);session.cameras=nullptr;
    if(owner.pauseBound) {hp2::UnbindPauseMenu();owner.pauseBound=false;}
    if(owner.dispatch && owner.dispatch->pauseMenu==owner.pause.get())owner.dispatch->pauseMenu=nullptr;
    owner.pause.reset();session.actions={};owner.closed=true;
}
std::exception_ptr RaceCoordinatorOwner::Close() noexcept
{
    if(!storage_)return {};
    auto& owner=*storage_;auto& session=owner.session;
    if(owner.closed)return owner.closeFailure;
    owner.closed=true;
    CoordinatorCloseErrors errors;
    // Terminal admission is already revoked. Do not publish fresh UI/audio
    // work through ordinary state-exit messages; retire owners directly.
    session.processingMessages=0;session.state=RC_STATE_NONE;
    session.actions.count=0;session.actions.readIndex=0;
    if(owner.pause)errors.Remember(owner.pause->Shutdown());
    owner.pauseBound=false;
    CloseOwnedRace(session,errors);
    errors.Run([&]{gameSessionCamerasDispose(session.cameras);session.cameras=nullptr;});
    if(owner.dispatch && owner.dispatch->pauseMenu==owner.pause.get())owner.dispatch->pauseMenu=nullptr;
    owner.pause.reset();session.actions={};
    owner.closeFailure=errors.first;
    return errors.first;
}
RaceCoordinatorOwner::~RaceCoordinatorOwner() noexcept {(void)Close();}
GameSession& RaceCoordinatorOwner::Session() noexcept {return storage_->session;}
const GameSession& RaceCoordinatorOwner::Session() const noexcept {return storage_->session;}

int32_t raceCoordinatorTheRaceHasFinished(GameSession& session)
{
    auto* b=session.bindings;
    if(!b || !session.ownedState || !b->replayOwner || !b->race || !b->race->services ||
        !b->race->services->players || !b->startScreens || !b->startScreens->services.frontend)
        return coordinatorFailure(&session,RC_RESULT_INVALID_BINDING);
    auto& menus=*b->startScreens->services.frontend;
    hp2::Require(menus.frontend && menus.raceSetup && menus.sounds && menus.raceSetup->parameters &&
        menus.raceSetup->currentEvent,"Race finish has no canonical frontend/event/audio owners");
    auto& frontend=*menus.frontend;
    auto& parameters=*menus.raceSetup->parameters;
    replayManagerRequestStop(&b->replayOwner->State());
    frontend.CheckForAbortion(*b->race->services->players);
    if(raceFrameSimulateFinish(b->race)!=WORLD_SERVICES_OK ||
        raceFrameNotifyRaceOver(b->race)!=WORLD_SERVICES_OK)
        return coordinatorFailure(&session,RC_RESULT_RACE_FAILURE);
    frontend.TheRaceIsOver(*b->race,*menus.raceSetup,*menus.sounds);
    const bool more=frontend.UseTheRaceEvent() && menus.raceSetup->currentEvent->CanContinue(frontend,parameters);
    // PAL sets2 first, then MOVN clears it when IsMultiTrack is true.
    const int32_t continuation=more?1:((parameters.raceKind==4 || parameters.raceKind==5)?0:2);
    if(b->demoDisc && b->demoDisc->ActivationWord()) {
        b->demoDisc->SetEndReason(4);
        if(!raceCoordinatorChangeState(&session,RC_STATE_NONE))return 0;
        return raceCoordinatorSendMessage(&session,10,0);
    }
    hp2::Require(!session.ownedState->endScreens,"Race finish cannot replace a live results owner");
    session.ownedState->endScreens=std::make_unique<hp2::EndGameScreens>(
        b->startScreens->services,continuation);
    return raceCoordinatorChangeState(&session,RC_STATE_POST_RACE);
}

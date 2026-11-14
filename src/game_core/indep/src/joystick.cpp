/* Existing typed host input adapter, arranged in the individually
 * established joystick.cpp unit. This move adds no ELF body. */
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/EasterEggs.hpp"
#include "indep/inc/Flags.hpp"
#include <algorithm>
#include <cstring>
#include <limits.h>
#include "indep/inc/Replay.hpp"
#include "indep/inc/DemoDisc.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"

uint32_t joystickReadMappedControl(const JoystickControls* controls,uint8_t field,uint32_t invert,uint32_t mask)
{
    hp2::Require(controls,"Input mapping has no native controls");
    uint32_t value=0;
    if(field>=16 && field<32) {
        // Source table inversion flags describe released active-low actions.
        // The live sample itself always stores positive logical held bits.
        value=static_cast<uint16_t>(~controls->held)>>(field-16);
    } else if(field>=32 && field<64) {
        value=controls->sticks[(field-32)/8]>>(field%8);
    } else if(field>=64 && field<160) {
        value=controls->pressures[(field-64)/8]>>(field%8);
    } else {
        hp2::Require(field==0,"Input mapping is outside logical control fields");
    }
    return (value^invert)&mask;
}

namespace {
void ConnectScanner(GameInputRouter& router,GameInputRegistration& node);
uint32_t nativeInputResetEpoch[2]{};
const char* nativeDebugCaption{};
constexpr char NativeDebugCameraCaption[]="DEBUG CAMERA - Back: next view; hold Start then Back: drive";
bool NativeDebugTarget(const GameInputRegistration& node)
{
    return node.target.kind==GAME_INPUT_PLAYER_CAMERA || node.target.kind==GAME_INPUT_FIXED_CAMERA ||
        node.target.kind==GAME_INPUT_ORBIT_CAMERA || node.target.kind==GAME_INPUT_TRACK_ROUTE_EDITOR;
}
int32_t ScannerPort(const GameInputRegistration& node)
{
    // Only the four typed debug consumers borrow P1. Device identity, P2
    // gameplay, type monitoring and event registration keys stay unchanged.
    return applicationDebugToolsEnabled && NativeDebugTarget(node)?0:node.device;
}
}

void joystickNotifyNativeInputReset(int32_t port)
{
    if(port<0 || port>=2)return;
    ++nativeInputResetEpoch[port];
    if(port==0)nativeDebugCaption=nullptr;
}
const char* joystickNativeDebugCaption() {return nativeDebugCaption;}

// Complete PAL1544F8..154540: CurrentJoystickTypes is the same two-byte
// owner read by GetJoystickType154308. Device packets and handlers remain
// live native objects; the original method does not serialize them.
void joystickDoSnapshot(GameInputRouter* router,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!router) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;}
    replaySnapshotBeginSection(snapshot,nullptr,"Joystick");
    replaySnapshotMoveData(snapshot,router->monitoredTypes,2,-1,0);
    replaySnapshotEndSection(snapshot);
}

void gameInputRouterInitialize(GameInputRouter* router,GameInputRegistration* storage,size_t capacity)
{
    router->registrations=storage;
    router->capacity=capacity;
    router->count=0;
    router->nextOrder=0;
    router->result=GAME_INPUT_OK;
    router->dispatching=0;
    router->dispatchDevice=0;
    router->dispatchEvent=0;
    router->dispatchCancelled=0;
    router->world=nullptr;router->nativeDebug={};nativeDebugCaption=nullptr;
    router->queuedEventCount=0;router->generatingEvents=0;
    for (int32_t port=0;port<2;++port) {
        auto& scanner=router->scanners[port];
        scanner.currentType=-1;scanner.pendingType=-1;scanner.stableSamples=0;
        for (auto& axis:scanner.calibration) {
            axis.keys[0]={0.0f,0.0f};axis.keys[1]={128.0f,128.0f};
            axis.keys[2]={128.0f,128.0f};axis.keys[3]={255.0f,255.0f};axis.hysteresis=0;
        }
        router->monitoredTypes[port]=255;
        router->userConfigurations[port]=0;
        router->pendingMotorDirty[port]=0;
        for (auto& byte:router->pendingMotorBytes[port])byte=0;
        router->forceWheels[port]={-1,0,0};
        for (auto& channel:router->rumblers[port])channel={nullptr,0.0f,1.0f,0,0};
    }
}

GameInputResult gameInputRegister(GameInputRouter* router,int32_t device,int32_t event,GameInputTarget target)
{
    if(!router||!router->registrations||device<0||device>=2||target.kind<GAME_INPUT_REAL_DRIVER||
       GAME_INPUT_REPLAY_SCREEN<target.kind||
       (target.kind==GAME_INPUT_REAL_DRIVER?!target.owner.real:
        target.kind==GAME_INPUT_FIXED_CAMERA?!target.owner.fixedCamera:
        target.kind==GAME_INPUT_ORBIT_CAMERA?!target.owner.orbitCamera:
        target.kind==GAME_INPUT_TRACK_ROUTE_EDITOR?!target.owner.editorInput:
        target.kind==GAME_INPUT_FRONTEND?!target.owner.frontend:
        target.kind==GAME_INPUT_EASTER_EGGS?!target.owner.easterEggs:
        target.kind==GAME_INPUT_COPS_ON_MODE_SELECT?!target.owner.copsOnModeSelect:
        target.kind==GAME_INPUT_COPS_OFF_MODE_SELECT?!target.owner.copsOffModeSelect:
        target.kind==GAME_INPUT_CAR_SELECTOR?!target.owner.carSelector:
        target.kind==GAME_INPUT_RACE_COORDINATOR?!target.owner.raceCoordinator:
        target.kind==GAME_INPUT_TYPE_MONITOR?!target.owner.typeMonitor:
        target.kind==GAME_INPUT_JOYSTICK_CALIBRATION?!target.owner.joystickCalibration:
        target.kind==GAME_INPUT_REPLAY_SCREEN?!target.owner.replayScreen:!target.owner.player))
        return router?router->result=GAME_INPUT_INVALID_BINDING:GAME_INPUT_INVALID_BINDING;
    if(router->count==router->capacity||router->nextOrder==UINT64_MAX)
        return router->result=GAME_INPUT_CAPACITY;
    GameInputRegistration* registration=&router->registrations[router->count++];
    registration->device=device;
    registration->event=event;
    registration->target=target;
    registration->order=++router->nextOrder;
    registration->scannerPhase=0;ConnectScanner(*router,*registration);
    return router->result=GAME_INPUT_OK;
}

int32_t gameInputUnregister(GameInputRouter* router,int32_t device,int32_t event)
{
    if(!router)return 0;
    for (uint32_t i=0;i<router->queuedEventCount;++i) {
        auto& queued=router->queuedEvents[i];
        if (queued.port==device && queued.registeredEvent==static_cast<uint8_t>(event))queued.active=0;
    }
    /* The original unregister also cancels every queued callback for this
     * device/event pair, even if another duplicate registration remains. */
    if(router->dispatching&&router->dispatchDevice==device&&router->dispatchEvent==event)
        router->dispatchCancelled=1;
    for(size_t index=0;index<router->count;++index){
        const GameInputRegistration* registration=&router->registrations[index];
        if(registration->device!=device||registration->event!=event)continue;
        for(size_t following=index+1;following<router->count;++following)
            router->registrations[following-1]=router->registrations[following];
        --router->count;
        return 1;
    }
    return 0;
}

#include "indep/inc/CarDriver.hpp"
#include "RealDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "FMenus.hpp"
#include "ReplayScreen.hpp"
#include "Fred.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include <stdint.h>

namespace {
GameInputResult DispatchTarget(GameInputRouter* router,GameInputTarget target,int32_t device,int32_t event,uint8_t value)
{
        switch(target.kind){
        case GAME_INPUT_REAL_DRIVER:
            realDriverDispatchInput(target.owner.real,device,event,value);
            if(target.owner.real->result!=CAR_DRIVER_OK){

                return router->result=GAME_INPUT_OWNER_FAILURE;
            }
            break;
        case GAME_INPUT_PLAYER:carFramePlayerDispatchInput(target.owner.player,device,event,value);break;
        case GAME_INPUT_PLAYER_ROUTE:carFramePlayerRouteInput(target.owner.player,device,event,value);break;
        case GAME_INPUT_PLAYER_MENU:carFramePlayerMenuInput(target.owner.player,device,event,value);break;
        case GAME_INPUT_PLAYER_RACE:carFramePlayerRaceInput(target.owner.player,device,event,value);break;
        case GAME_INPUT_PLAYER_CAMERA:carFramePlayerCycleCameraInput(target.owner.player,device,event,value);break;
        case GAME_INPUT_FIXED_CAMERA:carPlayerFixedCameraDispatchInput(target.owner.fixedCamera,device,event,value);break;
        case GAME_INPUT_ORBIT_CAMERA:carPlayerOrbitCameraDispatchInput(target.owner.orbitCamera,device,event,value);break;
        case GAME_INPUT_TRACK_ROUTE_EDITOR:trackRouteEditorInputDispatch(target.owner.editorInput,value);break;
        case GAME_INPUT_FRONTEND:
            if(flailerFrontendDispatchInput(target.owner.frontend,device,event,value)!=GAME_INPUT_OK){
                return router->result=GAME_INPUT_OWNER_FAILURE;
            }
            break;
        case GAME_INPUT_EASTER_EGGS:
            target.owner.easterEggs->JoyHandler(device,event,value);
            break;
        case GAME_INPUT_COPS_ON_MODE_SELECT:
            hp2::CopsOnModeSelect::JoyHandlerBridge(device,event,value,target.owner.copsOnModeSelect);
            break;
        case GAME_INPUT_COPS_OFF_MODE_SELECT:
            hp2::CopsOffModeSelect::JoyHandlerBridge(device,event,value,target.owner.copsOffModeSelect);
            break;
        case GAME_INPUT_CAR_SELECTOR:
            hp2::CarSelector::JoyHandlerBridge(device,event,value,target.owner.carSelector);
            break;
        case GAME_INPUT_RACE_COORDINATOR:
            raceCoordinatorJoystickEvent(target.owner.raceCoordinator,event);
            break;
        case GAME_INPUT_JOYSTICK_CALIBRATION:
            hp2::JoystickCalibrationInput(target.owner.joystickCalibration,device,event,value);
            break;
        case GAME_INPUT_REPLAY_SCREEN:
            hp2::ReplayScreen::JoyHandlerBridge(device,event,value,target.owner.replayScreen);
            break;
        case GAME_INPUT_TYPE_MONITOR:
            if (joystickTypeChanged(target.owner.typeMonitor,device,value)!=GAME_INPUT_OK) {
                return router->result=GAME_INPUT_OWNER_FAILURE;
            }
            break;
        default:return router->result=GAME_INPUT_INVALID_BINDING;
        }
    return router->result=GAME_INPUT_OK;
}
}

GameInputResult gameInputDispatch(GameInputRouter* router,int32_t device,int32_t event,uint8_t value)
{
    if(!router)return GAME_INPUT_INVALID_BINDING;
    if(router->dispatching)return router->result=GAME_INPUT_INVALID_BINDING;
    router->dispatching=1;
    router->dispatchDevice=device;
    router->dispatchEvent=event;
    router->dispatchCancelled=0;
    const uint64_t lastOrder=router->nextOrder;
    uint64_t previousOrder=0;
    for(;;){
        size_t selected=router->count;
        uint64_t nextOrder=UINT64_MAX;
        for(size_t index=0;index<router->count;++index){
            const GameInputRegistration* registration=&router->registrations[index];
            if(registration->device==device&&registration->event==event&&
               previousOrder<registration->order&&registration->order<=lastOrder&&registration->order<nextOrder){
                selected=index;nextOrder=registration->order;
            }
        }
        if(selected==router->count||router->dispatchCancelled)break;
        const GameInputTarget target=router->registrations[selected].target;
        previousOrder=nextOrder;
        const auto result=DispatchTarget(router,target,device,event,value);
        if (result!=GAME_INPUT_OK) {router->dispatching=0;return result;}
    }
    router->dispatching=0;
    return router->result=GAME_INPUT_OK;
}

#include "World.hpp"
#include "TrackedController.hpp"
#include "indep/inc/CarDriver.hpp"
#include "Car.hpp"
#include "indep/inc/CarDriver.hpp"
#include <cstring>

namespace {
bool InputPort(const GameInputRouter* router,int32_t port)
{
    return router && port>=0 && port<2;
}
}
uint8_t joystickGetType(const GameInputRouter* router,int32_t port)
{
    if (!InputPort(router,port))return 255;
    return router->monitoredTypes[port];
}
int32_t joystickIsPluggedIn(const GameInputRouter* router,int32_t port)
{
    return joystickGetType(router,port)!=255;
}
GameInputResult joystickInitializeTypeMonitoring(GameInputRouter* router)
{
    if (!router || !router->registrations)return GAME_INPUT_INVALID_BINDING;
    if (router->capacity-router->count<2 || router->nextOrder>UINT64_MAX-2)
        return router->result=GAME_INPUT_CAPACITY;
    GameInputTarget target{};target.kind=GAME_INPUT_TYPE_MONITOR;target.owner.typeMonitor=router;
    for (int32_t port=0;port<2;++port) {
        router->monitoredTypes[port]=255;
        const auto result=gameInputRegister(router,port,1,target);
        if (result!=GAME_INPUT_OK)return result;
    }
    return GAME_INPUT_OK;
}
void joystickCloseTypeMonitoring(GameInputRouter* router)
{
    if (!router)return;
    for (int32_t port=0;port<2;++port) {
        router->monitoredTypes[port]=255;
        gameInputUnregister(router,port,1);
    }
}
GameInputResult joystickResetTypeHandlers(GameInputRouter* router)
{
    if (!router || !router->registrations)return GAME_INPUT_INVALID_BINDING;
    size_t removable=0;
    for (int32_t port=0;port<2;++port)
        for (size_t index=0;index<router->count;++index)
            if (router->registrations[index].device==port && router->registrations[index].event==1) {
                ++removable;break;
            }
    if (router->capacity-router->count+removable<2 || router->nextOrder>UINT64_MAX-2)
        return router->result=GAME_INPUT_CAPACITY;
    GameInputTarget target{};target.kind=GAME_INPUT_TYPE_MONITOR;target.owner.typeMonitor=router;
    // Complete final1543F0, called by Pause.MaybeResume. It keeps current types.
    for (int32_t port=0;port<2;++port) {
        gameInputUnregister(router,port,1);
        const auto result=gameInputRegister(router,port,1,target);
        if (result!=GAME_INPUT_OK)return result;
    }
    return GAME_INPUT_OK;
}
GameInputResult joystickTypeChanged(GameInputRouter* router,int32_t port,uint8_t type)
{
    if (!InputPort(router,port))return GAME_INPUT_INVALID_BINDING;
    // Full final-only StopPortRumble155E40 closure: evaluate OLD type before
    // publishing the new one, then request zero on both actual motor channels.
    if (joystickSupportsPressureMode(router->monitoredTypes[port]))
        joystickSetRumble(router,port,0,0);
    // Full final-only14E8C8 direct closure. No guessed World::ResetJoystick
    // definition is introduced: use the existing typed CarDriver owner.
    if (router->world) {
        const auto& world=*router->world;
        if (world.totalCars<0 || world.totalCars>WORLD_MAX_CARS)return router->result=GAME_INPUT_INVALID_BINDING;
        for (int32_t index=0;index<world.totalCars;++index) {
            auto* car=world.carTable[index];
            if (!car || !car->controller)continue;
            auto* driver=static_cast<CarDriver*>(car->controller);
            if (driver->kind!=CAR_DRIVER_REAL)continue;
            auto* real=driver->owner.real;
            auto* movement=static_cast<CarMovement*>(car->movement);
            if (!real || real->input.movement!=car->movement || real->base.movement!=car->movement ||
                !movement || movement->bindings.owner!=car)
                return router->result=GAME_INPUT_INVALID_BINDING;
            if (real->input.port==port)realDriverClearInput(&real->input);
        }
    }
    router->monitoredTypes[port]=type;
    return router->result=GAME_INPUT_OK;
}
void joystickSetUserConfiguration(GameInputRouter* router,int32_t port,int32_t configuration)
{
    if (InputPort(router,port))router->userConfigurations[port]=configuration;
}
int32_t joystickGetUserConfiguration(const GameInputRouter* router,int32_t port)
{
    return InputPort(router,port)?router->userConfigurations[port]:0;
}

// Final PAL only:230 ScannerConfig lookup rows at2DC500 and7 type tables at2DE558.
namespace {
enum class ScannerKind { None,TypeChanged,AnyButton,DigitalDown,DigitalUpOrDown,DoublePress,Repeat,DigitalSteer,DigitalAnalog,Analog };
struct ButtonMapping { int8_t configurations[8]; int32_t event; uint32_t button,shift; const char* name; ScannerKind scanner; uint8_t fields[8]; };
constexpr ButtonMapping buttonMappings[]{
    {{-1,0,0,0,0,0,0,0},1,0,0,nullptr,ScannerKind::TypeChanged,{0,0,0,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},2,0,0,"START + SELECT",ScannerKind::DigitalDown,{1,1,19,16,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},3,0,0,nullptr,ScannerKind::Repeat,{1,0,20,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},4,0,0,nullptr,ScannerKind::Repeat,{1,0,22,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},5,0,0,nullptr,ScannerKind::Repeat,{1,0,23,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},6,0,0,nullptr,ScannerKind::Repeat,{1,0,21,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},7,0,0,nullptr,ScannerKind::DigitalDown,{1,0,19,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},8,0,0,nullptr,ScannerKind::DigitalDown,{1,0,16,0,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},9,0,0,nullptr,ScannerKind::DigitalDown,{0,0,47,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},9,0,0,nullptr,ScannerKind::DigitalDown,{1,0,30,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},10,0,0,nullptr,ScannerKind::DigitalDown,{1,0,31,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},11,0,0,nullptr,ScannerKind::DigitalDown,{1,0,28,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},12,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,29,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},17,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,31,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},13,0,0,nullptr,ScannerKind::DigitalDown,{1,0,26,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},14,0,0,nullptr,ScannerKind::DigitalDown,{1,0,24,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},15,0,0,nullptr,ScannerKind::DigitalDown,{1,0,27,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},16,0,0,nullptr,ScannerKind::DigitalDown,{1,0,25,0,0,0,1,0}},
    {{20,21,22,30,31,32,33,34},18,0,0,nullptr,ScannerKind::DigitalAnalog,{0,0,48,0,1,0,1,0}},
    {{20,21,22,30,31,32,33,34},19,0,0,nullptr,ScannerKind::DigitalAnalog,{0,0,48,0,2,0,1,0}},
    {{20,21,22,30,31,32,33,34},20,0,0,nullptr,ScannerKind::DigitalAnalog,{0,0,56,0,1,0,1,0}},
    {{20,21,22,30,31,32,33,34},21,0,0,nullptr,ScannerKind::DigitalAnalog,{0,0,56,0,2,0,1,0}},
    {{-1,0,0,0,0,0,0,0},24,0,0,nullptr,ScannerKind::AnyButton,{1,0,16,0,16,0,0,0}},
    {{-1,0,0,0,0,0,0,0},22,0,0,nullptr,ScannerKind::AnyButton,{1,0,16,0,16,0,1,0}},
    {{-1,0,0,0,0,0,0,0},23,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,26,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},25,0,0,nullptr,ScannerKind::DigitalDown,{1,0,30,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},26,0,0,nullptr,ScannerKind::DigitalDown,{1,0,29,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},27,0,0,nullptr,ScannerKind::DigitalDown,{1,0,28,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},28,0,0,nullptr,ScannerKind::DigitalDown,{1,0,31,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},29,0,0,nullptr,ScannerKind::Repeat,{1,0,20,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},30,0,0,nullptr,ScannerKind::Repeat,{1,0,22,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},31,0,0,nullptr,ScannerKind::Repeat,{1,0,23,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},32,0,0,nullptr,ScannerKind::Repeat,{1,0,21,0,0,0,1,0}},
    {{20,21,22,30,31,32,0,0},41,0,0,nullptr,ScannerKind::Analog,{0,0,48,0,0,0,0,0}},
    {{20,21,22,30,31,32,0,0},42,0,0,nullptr,ScannerKind::Analog,{0,0,56,0,0,0,0,0}},
    {{20,21,22,30,31,32,0,0},43,0,0,nullptr,ScannerKind::Analog,{0,0,40,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},33,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,31,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},34,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,26,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},35,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,23,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},36,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,21,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},37,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,20,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},38,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,22,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},39,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,25,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},40,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,27,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},45,0,0,nullptr,ScannerKind::DigitalDown,{1,0,16,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},46,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,21,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},47,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,23,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},48,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,20,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},49,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,22,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},50,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,24,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},51,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,26,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},52,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,27,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},53,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,25,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},54,143,0,"R3",ScannerKind::DigitalUpOrDown,{1,0,18,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},55,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,23,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},56,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,21,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},57,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,20,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},58,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,22,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},59,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,24,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},60,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,26,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},61,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,30,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},62,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,28,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},63,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,31,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},64,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,29,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},65,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,27,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},66,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,25,0,0,0,0,0}},
    {{20,30,0,0,0,0,0,0},67,0,0,nullptr,ScannerKind::DigitalDown,{1,0,18,0,0,0,0,0}},
    {{20,30,0,0,0,0,0,0},68,0,0,nullptr,ScannerKind::DigitalDown,{1,0,17,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},69,0,0,nullptr,ScannerKind::DigitalDown,{1,0,19,0,0,0,0,0}},
    {{20,21,22,30,31,32,0,0},70,0,0,nullptr,ScannerKind::Analog,{0,0,48,0,0,1,0,0}},
    {{20,21,22,30,31,32,0,0},71,0,0,nullptr,ScannerKind::Analog,{0,0,56,0,0,1,0,0}},
    {{20,21,22,30,31,32,0,0},72,0,0,nullptr,ScannerKind::Analog,{0,0,32,0,0,1,0,0}},
    {{20,21,22,30,31,32,0,0},73,0,0,nullptr,ScannerKind::Analog,{0,0,40,0,0,1,0,0}},
    {{20,30,0,0,0,0,0,0},74,0,0,nullptr,ScannerKind::Analog,{0,0,48,0,0,1,0,0}},
    {{20,30,0,0,0,0,0,0},75,0,0,nullptr,ScannerKind::Analog,{0,0,56,0,0,1,0,0}},
    {{20,30,0,0,0,0,0,0},76,0,0,nullptr,ScannerKind::Analog,{0,0,40,0,0,1,0,0}},
    {{20,30,0,0,0,0,0,0},77,0,0,nullptr,ScannerKind::Analog,{0,0,32,0,0,1,0,0}},
    {{-1,0,0,0,0,0,0,0},78,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,18,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},79,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,26,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},80,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,24,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},81,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,20,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},82,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,22,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},83,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,23,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},84,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,21,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},85,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,28,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},86,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,25,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},87,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,27,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},88,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,19,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},89,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,29,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},90,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,31,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},91,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,30,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},92,0,0,nullptr,ScannerKind::DoublePress,{1,1,25,24,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},99,0,0,nullptr,ScannerKind::DigitalDown,{1,0,19,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},100,0,0,nullptr,ScannerKind::DigitalDown,{1,0,30,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},101,0,0,nullptr,ScannerKind::DigitalDown,{1,0,23,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},102,0,0,nullptr,ScannerKind::DigitalDown,{1,0,21,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},103,0,0,nullptr,ScannerKind::DigitalDown,{1,0,31,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},104,0,0,nullptr,ScannerKind::DigitalDown,{1,0,28,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},105,0,0,nullptr,ScannerKind::DigitalDown,{1,0,26,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},106,0,0,nullptr,ScannerKind::DigitalDown,{1,0,29,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},107,0,0,nullptr,ScannerKind::DigitalDown,{1,0,16,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},108,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,25,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},109,0,0,nullptr,ScannerKind::DigitalDown,{1,0,24,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},110,0,0,nullptr,ScannerKind::DigitalUpOrDown,{1,0,27,0,0,0,1,0}},
    {{10,11,12,13,0,0,0,0},111,154,0,"DPAD",ScannerKind::DigitalSteer,{1,1,23,21,0,0,0,0}},
    {{10,11,0,0,0,0,0,0},113,134,0,"CROSS",ScannerKind::DigitalUpOrDown,{1,0,30,0,255,0,0,0}},
    {{0,0,12,13,0,0,0,0},113,136,0,"SQUARE",ScannerKind::DigitalUpOrDown,{1,0,31,0,255,0,0,0}},
    {{10,11,0,0,0,0,0,0},116,136,0,"SQUARE",ScannerKind::DigitalUpOrDown,{1,0,31,0,255,0,0,0}},
    {{0,0,12,13,0,0,0,0},116,134,0,"CROSS",ScannerKind::DigitalUpOrDown,{1,0,30,0,255,0,0,0}},
    {{10,0,12,0,0,0,0,0},121,139,0,"R1",ScannerKind::DigitalUpOrDown,{1,0,27,28,255,0,0,0}},
    {{0,11,0,13,0,0,0,0},121,137,0,"CIRCLE",ScannerKind::DigitalUpOrDown,{1,0,29,0,255,0,0,0}},
    {{10,11,12,13,0,0,0,0},123,141,0,"R2",ScannerKind::DigitalUpOrDown,{1,0,25,28,0,0,0,0}},
    {{10,11,12,13,0,0,0,0},124,140,0,"L2",ScannerKind::DigitalUpOrDown,{1,0,24,28,0,0,0,0}},
    {{10,11,12,13,0,0,0,0},94,138,0,"L1",ScannerKind::DigitalUpOrDown,{1,0,26,28,0,0,0,0}},
    {{10,11,12,13,0,0,0,0},93,135,0,"TRIANGLE",ScannerKind::DigitalDown,{0,0,28,0,0,0,0,0}},
    {{10,11,12,13,0,0,0,0},95,140,135,"TRIANGLE + L2",ScannerKind::DigitalDown,{1,1,24,28,0,0,0,0}},
    {{10,11,12,13,0,0,0,0},96,141,135,"TRIANGLE + R2",ScannerKind::DigitalDown,{1,1,25,28,0,0,0,0}},
    {{10,11,12,13,0,0,0,0},127,139,135,"TRIANGLE + R1",ScannerKind::DigitalDown,{1,1,27,28,0,0,0,0}},
    {{10,11,12,13,0,0,0,0},129,140,135,"TRIANGLE + L2",ScannerKind::DigitalDown,{1,1,24,28,0,0,0,0}},
    {{10,11,12,13,0,0,0,0},128,141,135,"TRIANGLE + R2",ScannerKind::DigitalDown,{1,1,25,28,0,0,0,0}},
    {{10,0,12,0,0,0,0,0},126,137,0,"CIRCLE",ScannerKind::DigitalUpOrDown,{1,0,29,0,0,0,0,0}},
    {{0,11,0,13,0,0,0,0},126,139,0,"R1",ScannerKind::DigitalUpOrDown,{1,0,27,28,0,0,0,0}},
    {{20,21,0,0,0,0,0,0},113,134,0,"CROSS",ScannerKind::DigitalUpOrDown,{1,0,30,0,255,0,0,0}},
    {{0,0,22,23,0,0,0,0},113,136,0,"SQUARE",ScannerKind::DigitalUpOrDown,{1,0,31,0,255,0,0,0}},
    {{20,21,22,23,0,0,0,0},115,150,0,"ANALOGR",ScannerKind::Analog,{1,0,40,0,1,3,0,0}},
    {{20,21,0,0,0,0,0,0},116,136,0,"SQUARE",ScannerKind::DigitalUpOrDown,{1,0,31,0,255,0,0,0}},
    {{0,0,22,23,0,0,0,0},116,134,0,"CROSS",ScannerKind::DigitalUpOrDown,{1,0,30,0,255,0,0,0}},
    {{0,0,0,0,0,0,0,0},116,140,0,"L2",ScannerKind::DigitalUpOrDown,{1,0,24,28,255,0,0,0}},
    {{20,21,22,23,0,0,0,0},118,151,0,"ANALOGR",ScannerKind::Analog,{1,0,40,0,2,2,0,0}},
    {{20,0,22,0,0,0,0,0},121,139,0,"R1",ScannerKind::DigitalUpOrDown,{1,0,27,28,255,0,0,0}},
    {{0,21,0,23,0,0,0,0},121,137,0,"CIRCLE",ScannerKind::DigitalUpOrDown,{1,0,29,0,255,0,0,0}},
    {{0,0,0,0,0,0,0,0},121,134,0,"CROSS",ScannerKind::DigitalUpOrDown,{1,0,30,0,255,0,0,0}},
    {{20,21,22,23,0,0,0,0},112,148,0,"ANALOGL",ScannerKind::Analog,{1,0,48,0,0,1,0,0}},
    {{20,21,22,23,0,0,0,0},123,141,0,"R2",ScannerKind::DigitalUpOrDown,{1,0,25,28,0,0,0,0}},
    {{20,21,22,23,0,0,0,0},124,140,0,"L2",ScannerKind::DigitalUpOrDown,{1,0,24,28,0,0,0,0}},
    {{20,21,22,23,0,0,0,0},94,138,0,"L1",ScannerKind::DigitalUpOrDown,{1,0,26,28,0,0,0,0}},
    {{20,21,22,23,0,0,0,0},93,135,0,"TRIANGLE",ScannerKind::DigitalDown,{0,0,28,0,0,0,0,0}},
    {{20,21,22,23,0,0,0,0},95,140,135,"TRIANGLE + L2",ScannerKind::DigitalDown,{1,1,24,28,0,0,0,0}},
    {{20,21,22,23,0,0,0,0},96,141,135,"TRIANGLE + R2",ScannerKind::DigitalDown,{1,1,25,28,0,0,0,0}},
    {{20,21,22,23,0,0,0,0},127,143,0,"R3",ScannerKind::DigitalDown,{1,0,18,0,0,0,0,0}},
    {{20,21,22,23,0,0,0,0},129,140,135,"TRIANGLE + L2",ScannerKind::DigitalDown,{1,1,24,28,0,0,0,0}},
    {{20,21,22,23,0,0,0,0},128,141,135,"TRIANGLE + R2",ScannerKind::DigitalDown,{1,1,25,28,0,0,0,0}},
    {{20,0,22,0,0,0,0,0},126,137,0,"CIRCLE",ScannerKind::DigitalUpOrDown,{1,0,29,0,0,0,0,0}},
    {{0,21,0,23,0,0,0,0},126,139,0,"R1",ScannerKind::DigitalUpOrDown,{1,0,27,28,0,0,0,0}},
    {{30,31,0,0,0,0,0,0},114,134,0,"CROSS",ScannerKind::Analog,{1,0,112,0,0,3,0,0}},
    {{0,0,32,33,0,0,0,0},114,136,0,"SQUARE",ScannerKind::Analog,{1,0,120,0,0,3,0,0}},
    {{30,31,32,33,0,0,0,0},115,150,0,"ANALOGR",ScannerKind::Analog,{1,0,40,0,1,3,0,0}},
    {{30,31,0,0,0,0,0,0},117,136,0,"SQUARE",ScannerKind::Analog,{1,0,120,0,0,2,0,0}},
    {{0,0,32,33,0,0,0,0},117,134,0,"CROSS",ScannerKind::Analog,{1,0,112,0,0,2,0,0}},
    {{30,31,32,33,0,0,0,0},118,151,0,"ANALOGR",ScannerKind::Analog,{1,0,40,0,2,2,0,0}},
    {{30,0,32,0,0,0,0,0},122,139,0,"R1",ScannerKind::Analog,{1,0,136,28,0,0,0,0}},
    {{0,31,0,33,0,0,0,0},122,137,0,"CIRCLE",ScannerKind::Analog,{1,0,104,0,0,0,0,0}},
    {{30,31,32,33,0,0,0,0},112,148,0,"ANALOGL",ScannerKind::Analog,{1,0,48,0,0,1,0,0}},
    {{30,31,32,33,0,0,0,0},123,141,0,"R2",ScannerKind::DigitalUpOrDown,{1,0,25,28,0,0,0,0}},
    {{30,31,32,33,0,0,0,0},124,140,0,"L2",ScannerKind::DigitalUpOrDown,{1,0,24,28,0,0,0,0}},
    {{30,31,32,33,0,0,0,0},94,138,0,"L1",ScannerKind::DigitalUpOrDown,{1,0,26,28,0,0,0,0}},
    {{30,31,32,33,0,0,0,0},93,135,0,"TRIANGLE",ScannerKind::DigitalDown,{0,0,28,0,0,0,0,0}},
    {{30,31,32,33,0,0,0,0},95,140,135,"TRIANGLE + L2",ScannerKind::DigitalDown,{1,1,24,28,0,0,0,0}},
    {{30,31,32,33,0,0,0,0},96,141,135,"TRIANGLE + R2",ScannerKind::DigitalDown,{1,1,25,28,0,0,0,0}},
    {{30,31,32,33,0,0,0,0},127,143,0,"R3",ScannerKind::DigitalDown,{1,0,18,0,0,0,0,0}},
    {{30,31,32,33,0,0,0,0},129,140,135,"TRIANGLE + L2",ScannerKind::DigitalDown,{1,1,24,28,0,0,0,0}},
    {{30,31,32,33,0,0,0,0},128,141,135,"TRIANGLE + R2",ScannerKind::DigitalDown,{1,1,25,28,0,0,0,0}},
    {{30,0,32,0,0,0,0,0},126,137,0,"CIRCLE",ScannerKind::DigitalUpOrDown,{1,0,29,0,0,0,0,0}},
    {{0,31,0,33,0,0,0,0},126,139,0,"R1",ScannerKind::DigitalUpOrDown,{1,0,27,28,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},114,1,0,"I",ScannerKind::Analog,{1,0,40,0,0,3,0,0}},
    {{40,0,0,0,0,0,0,0},117,2,0,"II",ScannerKind::Analog,{1,0,48,0,2,2,0,0}},
    {{40,0,0,0,0,0,0,0},119,4,0,"B",ScannerKind::DigitalUpOrDown,{1,0,28,0,255,0,0,0}},
    {{40,0,0,0,0,0,0,0},111,0,0,nullptr,ScannerKind::None,{1,0,0,0,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},112,0,0,"WHEEL",ScannerKind::Analog,{1,0,32,0,0,1,0,0}},
    {{40,0,0,0,0,0,0,0},123,5,0,"L",ScannerKind::DigitalUpOrDown,{1,0,24,29,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},124,6,0,"R",ScannerKind::DigitalUpOrDown,{1,0,27,29,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},94,138,0,"L1",ScannerKind::DigitalUpOrDown,{1,0,26,29,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},93,137,0,"CIRCLE",ScannerKind::DigitalDown,{0,0,29,0,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},95,140,137,"CIRCLE + L2",ScannerKind::DigitalDown,{1,1,24,29,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},96,138,137,"CIRCLE + L1",ScannerKind::DigitalDown,{1,1,26,29,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},127,143,0,"R3",ScannerKind::DigitalDown,{1,0,18,0,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},129,140,137,"CIRCLE + L2",ScannerKind::DigitalDown,{1,1,24,29,0,0,0,0}},
    {{40,0,0,0,0,0,0,0},128,138,137,"CIRCLE + L1",ScannerKind::DigitalDown,{1,1,26,29,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},114,156,0,"GAS",ScannerKind::Analog,{1,0,88,0,0,3,0,0}},
    {{0,51,0,0,0,0,0,0},117,157,0,"BRAKE",ScannerKind::Analog,{1,0,96,0,0,2,0,0}},
    {{0,51,0,0,0,0,0,0},121,137,0,"CIRCLE",ScannerKind::DigitalUpOrDown,{1,0,29,22,255,0,0,0}},
    {{0,51,0,0,0,0,0,0},111,155,0,nullptr,ScannerKind::None,{1,0,0,0,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},112,0,0,"WHEEL",ScannerKind::Analog,{1,0,40,0,0,1,0,0}},
    {{0,51,0,0,0,0,0,0},123,139,0,"R1",ScannerKind::DigitalUpOrDown,{1,0,27,25,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},124,138,0,"L1",ScannerKind::DigitalUpOrDown,{1,0,26,25,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},93,135,0,"TRIANGLE",ScannerKind::DigitalDown,{0,0,28,0,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},95,136,131,"SQUARE + DPAD",ScannerKind::DigitalDown,{1,1,31,22,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},96,137,131,"CIRCLE + DPAD",ScannerKind::DigitalDown,{1,1,29,22,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},127,134,0,"CROSS",ScannerKind::DigitalDown,{1,0,30,0,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},129,136,131,"SQUARE + DPAD",ScannerKind::DigitalDown,{1,1,31,22,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},128,137,131,"CIRCLE + DPAD",ScannerKind::DigitalDown,{1,1,29,22,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},126,141,0,"R2",ScannerKind::DigitalUpOrDown,{1,0,25,0,0,0,0,0}},
    {{0,51,0,0,0,0,0,0},94,140,0,"L2",ScannerKind::DigitalUpOrDown,{1,0,24,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},114,156,0,"GAS",ScannerKind::Analog,{1,0,88,0,0,3,0,0}},
    {{50,0,0,0,0,0,0,0},117,157,0,"BRAKE",ScannerKind::Analog,{1,0,96,0,0,2,0,0}},
    {{50,0,0,0,0,0,0,0},121,141,0,"R2",ScannerKind::DigitalUpOrDown,{1,0,25,22,255,0,0,0}},
    {{50,0,0,0,0,0,0,0},111,155,0,nullptr,ScannerKind::None,{1,0,0,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},112,0,0,"WHEEL",ScannerKind::Analog,{1,0,40,0,0,1,0,0}},
    {{50,0,0,0,0,0,0,0},123,139,0,"R1",ScannerKind::DigitalUpOrDown,{1,0,27,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},124,138,0,"L1",ScannerKind::DigitalUpOrDown,{1,0,26,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},93,135,0,"TRIANGLE",ScannerKind::DigitalDown,{0,0,28,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},126,137,0,"CIRCLE",ScannerKind::DigitalUpOrDown,{1,0,29,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},94,140,0,"L2",ScannerKind::DigitalUpOrDown,{1,0,24,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},127,142,0,"L3",ScannerKind::DigitalDown,{1,0,17,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},129,0,0,"DPADL",ScannerKind::DigitalDown,{1,1,23,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},128,0,0,"DPADR",ScannerKind::DigitalDown,{1,1,21,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},95,132,0,"DPADL",ScannerKind::DigitalDown,{1,1,23,0,0,0,0,0}},
    {{50,0,0,0,0,0,0,0},96,133,0,"DPADR",ScannerKind::DigitalDown,{1,1,21,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},111,153,0,"DPAD",ScannerKind::DigitalSteer,{1,1,23,21,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},130,130,0,"DPAD",ScannerKind::DigitalDown,{1,0,20,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},131,128,0,"START",ScannerKind::DigitalDown,{1,0,19,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},132,137,0,"CIRCLE",ScannerKind::DigitalDown,{1,0,30,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},125,129,0,"SELECT",ScannerKind::DigitalUpOrDown,{1,0,16,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},126,143,0,"R3",ScannerKind::DigitalUpOrDown,{1,0,18,0,0,0,0,0}},
    {{-1,0,0,0,0,0,0,0},133,0,0,"L1",ScannerKind::DigitalDown,{1,0,26,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},134,0,0,"L2",ScannerKind::DigitalDown,{1,0,24,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},135,0,0,"R1",ScannerKind::DigitalDown,{1,0,27,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},136,0,0,"R2",ScannerKind::DigitalDown,{1,0,25,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},137,0,0,"DPADL",ScannerKind::DigitalDown,{1,0,23,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},138,0,0,"DPADR",ScannerKind::DigitalDown,{1,0,21,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},139,0,0,"SQUARE",ScannerKind::DigitalDown,{1,0,31,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},140,0,0,"TRIANGLE",ScannerKind::DigitalDown,{1,0,28,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},141,0,0,"L3",ScannerKind::DigitalDown,{1,0,17,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},142,0,0,"R3",ScannerKind::DigitalDown,{1,0,18,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},143,0,0,"DPADU",ScannerKind::DigitalDown,{1,0,20,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},144,0,0,"DPADD",ScannerKind::DigitalDown,{1,0,22,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},145,0,0,"START",ScannerKind::DigitalDown,{1,0,19,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},146,0,0,"SELECT",ScannerKind::DigitalDown,{1,0,16,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},147,0,0,"CROSS",ScannerKind::DigitalDown,{1,0,30,0,0,0,1,0}},
    {{-1,0,0,0,0,0,0,0},148,0,0,"CIRCLE",ScannerKind::DigitalDown,{1,0,29,0,0,0,1,0}},
};
struct DeviceConfigurations { int32_t type; int8_t values[10]; uint8_t response[6]; };
constexpr DeviceConfigurations deviceConfigurations[]{
    {65,{10,11,12,13,0,0,0,0,0,0},{0,0,0,0,0,0}},
    {83,{20,21,22,23,0,0,0,0,0,0},{4,20,12,20,12,20}},
    {115,{20,21,22,23,0,0,0,0,0,0},{4,20,12,20,12,20}},
    {121,{30,31,32,33,0,0,0,0,0,0},{4,20,12,20,12,20}},
    {35,{40,0,0,0,0,0,0,0,0,0},{4,8,12,8,12,8}},
    {21,{50,0,0,0,0,0,0,0,0,0},{2,0,12,8,12,8}},
    {22,{51,0,0,0,0,0,0,0,0,0},{2,0,12,8,12,8}},
};
}

int32_t joystickDetermineAvailableConfigurations(int32_t* output,int32_t capacity,int32_t type)
{
    if (capacity>0 && !output)return 0;
    for (const auto& row:deviceConfigurations) {
        if (row.type!=type)continue;
        int32_t count=0;
        for (const auto value:row.values)
            if (value!=0 && count<capacity)output[count++]=value;
        return count;
    }
    return 0;
}
int32_t joystickChooseConfiguration(int32_t type,int32_t requested)
{
    int32_t available[10]{};
    const auto count=joystickDetermineAvailableConfigurations(available,10,type);
    if (!count)return 0;
    for (int32_t index=0;index<count;++index)if (available[index]==requested)return requested;
    return available[0];
}
namespace {
const ButtonMapping* FindScannerConfig(int32_t event,int32_t type,int32_t requested)
{
    const auto chosen=joystickChooseConfiguration(type,requested);
    for (const auto& row:buttonMappings) {
        if (row.event!=event)continue;
        for (const auto config:row.configurations) {
            if (chosen==0) {if (event==1)return &row;}
            else if (config!=0 && (config==-1 || config==chosen))return &row;
        }
    }
    return nullptr;
}
}
uint32_t joystickGetButtonChar(const GameInputRouter* router,int32_t port,int32_t event)
{
    const auto* row=FindScannerConfig(event,joystickGetType(router,port),joystickGetUserConfiguration(router,port));
    return row?row->button:0;
}
const char* joystickGetButtonName(const GameInputRouter* router,int32_t port,int32_t event)
{
    const auto* row=FindScannerConfig(event,joystickGetType(router,port),joystickGetUserConfiguration(router,port));
    return row && row->name?row->name:"";
}
void joystickGetAndShiftButtonChar(const GameInputRouter* router,int32_t port,int32_t event,
    uint32_t* button,uint32_t* shift)
{
    if (!button || !shift)return;
    *button=0;*shift=0;
    if (const auto* row=FindScannerConfig(event,joystickGetType(router,port),joystickGetUserConfiguration(router,port))) {
        *button=row->button;*shift=row->shift;
    }
}

namespace {
// PAL153C50: changing the connected configuration resets Value and Initial,
// but deliberately retains the independent repeat/double-press phase.
void ConnectScanner(GameInputRouter& router,GameInputRegistration& node)
{
    const auto port=ScannerPort(node);
    const auto* configuration=FindScannerConfig(node.event,
        router.scanners[port].currentType,router.userConfigurations[port]);
    node.scannerConfiguration=configuration?static_cast<int32_t>(configuration-buttonMappings):-1;
    node.scannerValue=0;node.scannerInitial=1;
}

// PAL155510..1555C4. Axis0 intentionally retains the default deadzone20;
// axes1..3 take the released per-device pairs. These are not SDL deadzones.
void ChooseResponse(GameInputCalibration& axis,int32_t type,int32_t number)
{
    uint8_t hysteresis=12,deadzone=20;
    for (const auto& row:deviceConfigurations) {
        if (row.type!=type)continue;
        if (number==1) {hysteresis=row.response[0];deadzone=row.response[1];}
        else if (number==2) {hysteresis=row.response[2];deadzone=row.response[3];}
        else if (number==3) {hysteresis=row.response[4];deadzone=row.response[5];}
        else hysteresis=row.response[0];
    }
    axis.keys[1].coordinate=128.0f-static_cast<float>(deadzone);
    axis.keys[2].coordinate=128.0f+static_cast<float>(deadzone);
    axis.hysteresis=hysteresis;
}

uint32_t PadBits(const JoystickControls* data,uint8_t field,uint8_t invert,uint32_t mask)
{
    return joystickReadMappedControl(data,field,invert,mask);
}

// The shared expression is identical in the four complete released digital
// scanners. A shifted chord requires a stable modifier and a new primary edge.
uint32_t DigitalValue(const ButtonMapping& config,const JoystickControls* current,const JoystickControls* previous)
{
    const auto* f=config.fields;
    uint32_t first=f[2]?PadBits(current,f[2],f[0],1):1;
    uint32_t second=f[3]?PadBits(current,f[3],f[1],1):1;
    if (f[3] && f[1]) {
        const auto oldFirst=PadBits(previous,f[2],f[0],1);
        const auto oldSecond=PadBits(previous,f[3],f[1],1);
        if (second!=oldSecond)second=0;
        first&=first^oldFirst;
    }
    return first&second;
}

uint16_t PackedEvent(const GameInputRegistration& node,uint32_t value)
{
    return static_cast<uint16_t>(static_cast<uint16_t>(node.event)+(value<<8));
}

// PAL1545E8 through155000, all nine scanner functions selected by the final
// 230-row table. No raw code pointer or beta event configuration is executed.
uint16_t ScanEvent(GameInputRegistration& node,GameInputPortScanner& port,
    const ButtonMapping& config,const JoystickControls* current,const JoystickControls* previous,int32_t videoBlank)
{
    const auto* f=config.fields;
    switch(config.scanner) {
    case ScannerKind::None:return 0; // Original table has a null scanner.
    case ScannerKind::TypeChanged: {
        const auto value=static_cast<uint8_t>(port.currentType);
        if (node.scannerValue==value && !node.scannerInitial)return 0;
        node.scannerInitial=0;node.scannerValue=value;return PackedEvent(node,value);
    }
    case ScannerKind::AnyButton: {
        const uint32_t mask=(uint32_t{1}<<(f[4]&31))-1;
        const uint32_t value=joystickReadMappedControl(current,f[2],f[0]?UINT32_MAX:0,mask)!=0;
        if (node.scannerInitial) {node.scannerInitial=0;node.scannerValue=value;}
        if (node.scannerValue==value)return 0;
        node.scannerValue=value;return value?PackedEvent(node,value):0;
    }
    case ScannerKind::DigitalDown:
    case ScannerKind::DigitalUpOrDown: {
        uint32_t value=DigitalValue(config,current,previous);
        if (value && f[4])value=f[4];
        if (config.scanner==ScannerKind::DigitalDown && node.scannerInitial) {
            node.scannerInitial=0;node.scannerValue=value;
        }
        if (node.scannerValue==value && !node.scannerInitial)return 0;
        node.scannerValue=value;node.scannerInitial=0;
        return value || config.scanner==ScannerKind::DigitalUpOrDown?PackedEvent(node,value):0;
    }
    case ScannerKind::DoublePress: {
        const bool down=DigitalValue(config,current,previous)!=0;
        int32_t phase=node.scannerPhase;
        bool emit=false;
        if (phase<50) {
            if (!down)phase=static_cast<uint32_t>(phase-2)<10?50:0;
            if (phase<=48)++phase;
        } else if (phase<100) {
            if (down)phase=static_cast<uint32_t>(phase-52)<10?100:0;
            if (phase<99)++phase;else if (phase==99)phase=0;
        } else if (phase<150) {
            if (!down) {emit=static_cast<uint32_t>(phase-102)<10;phase=0;}
            if (phase<=148)++phase;
        }
        node.scannerPhase=phase;
        return emit?PackedEvent(node,0):0;
    }
    case ScannerKind::Repeat: {
        const bool down=DigitalValue(config,current,previous)!=0;
        if (node.scannerInitial) {node.scannerInitial=0;node.scannerValue=0;}
        if (!down) {
            if (!node.scannerValue)return 0;
            node.scannerValue=0;return PackedEvent(node,0);
        }
        if (node.scannerValue==1 || node.scannerValue==2) {
            if (!(node.scannerPhase<videoBlank))return 0;
            node.scannerValue=2;
            node.scannerPhase=static_cast<int32_t>(static_cast<uint32_t>(videoBlank)+6);
            return PackedEvent(node,2);
        }
        node.scannerValue=1;
        node.scannerPhase=static_cast<int32_t>(static_cast<uint32_t>(videoBlank)+25);
        return PackedEvent(node,1);
    }
    case ScannerKind::DigitalSteer: {
        const auto left=PadBits(current,f[2],f[0],1),right=PadBits(current,f[3],f[1],1);
        const uint32_t value=left?right?128:0:right?255:128;
        if (node.scannerValue==value && !node.scannerInitial)return 0;
        node.scannerInitial=0;node.scannerValue=value;return PackedEvent(node,value);
    }
    case ScannerKind::DigitalAnalog: {
        const uint32_t values[2]={PadBits(current,f[2],f[0],255),f[3]?PadBits(current,f[3],f[1],255):128};
        const auto old=node.scannerValue;
        node.scannerValue=values[0]|(values[1]<<8);
        if (node.scannerInitial) {node.scannerInitial=0;return 0;}
        for (unsigned axis=0;axis<2;++axis) {
            const auto before=(old>>(axis*8))&255,now=values[axis];
            bool pressed=false,released=false,repeating=false;
            if (f[4]==1) {pressed=before>8 && now<8;released=before<8 && now>8;repeating=before<8 && now<8;}
            else if (f[4]==2) {pressed=before<247 && now>=248;released=before>=248 && now<247;repeating=before>=248 && now>=248;}
            if (pressed) {node.scannerPhase=static_cast<int32_t>(static_cast<uint32_t>(videoBlank)+25);return PackedEvent(node,1);}
            if (released)return PackedEvent(node,0);
            if (repeating && node.scannerPhase<videoBlank) {
                node.scannerPhase=static_cast<int32_t>(static_cast<uint32_t>(videoBlank)+6);return PackedEvent(node,1);
            }
        }
        return 0;
    }
    case ScannerKind::Analog: {
        hp2::Require(f[5]<4,"Joystick scanner has an invalid final calibration axis");
        const auto& calibration=port.calibration[f[5]];
        const KeyedCurve graph{calibration.keys,4};
        uint32_t value=static_cast<uint8_t>(convertFloatToInt32(keyedCurveEvaluate(&graph,
            static_cast<float>(PadBits(current,f[2],f[0],255)))));
        if (f[4]==1) {value=value<127?(127-value)*2:0;if (value==254)value=255;}
        else if (f[4]==2) {value=value>128?(value-128)*2:0;if (value==254)value=255;}
        if (node.scannerInitial) {node.scannerInitial=0;node.scannerValue=value;return PackedEvent(node,value);}
        if (f[3] && !PadBits(current,f[3],f[1],1))return 0;
        const auto hysteresis=value==0 || value==128 || value==255?0:calibration.hysteresis;
        if (value+hysteresis<node.scannerValue || node.scannerValue+hysteresis<value) {
            node.scannerValue=value;return PackedEvent(node,value);
        }
        return 0;
    }
    }
    throw std::logic_error("Invalid native joystick scanner kind");
}

// PAL153A00 copies the callback before dispatch. Removing its EventNode
// cancels matching queued callbacks, while other queue positions stay intact.
void AddQueuedEvent(GameInputRouter& input,int32_t port,uint16_t event,const GameInputRegistration& node)
{
    if (input.queuedEventCount>=256)return;
    auto& queued=input.queuedEvents[input.queuedEventCount++];
    queued={event,static_cast<uint8_t>(port),static_cast<uint8_t>(node.event),node.target,1};
}

// A native focus owner consumes the opening chord before the ordinary
// scanners see it. Physical packets and the second player's route are never
// rewritten. Only actual camera/editor registrations receive the tools view.
CarFramePlayerState* NativeDebugPlayer(const GameInputRouter& input)
{
    if(!input.world)return nullptr;
    for(size_t i=0;i<input.count;++i) {
        const auto& node=input.registrations[i];
        if(node.target.kind<GAME_INPUT_PLAYER || node.target.kind>GAME_INPUT_PLAYER_CAMERA)continue;
        auto* player=node.target.owner.player;
        if(player && player->movement.seat==0 && player->registry && player->registry->world==input.world)
            return player;
    }
    return nullptr;
}
int32_t NextNativeDebugCamera(const CarFramePlayerState& player,int32_t kind)
{
    constexpr int32_t live[]{1,4,2,3},session[]{0,4,2};
    const auto* modes=player.inputMode==4?session:live;
    const int32_t count=player.inputMode==4?3:4;
    for(int32_t i=0;i<count;++i)if(modes[i]==kind)return modes[(i+1)%count];
    return kind;
}
bool QueueNativeDebugCycle(GameInputRouter& input,CarFramePlayerState& player)
{
    for(size_t i=0;i<input.count;++i) {
        const auto& node=input.registrations[i];
        if(node.target.kind!=GAME_INPUT_PLAYER_CAMERA || node.target.owner.player!=&player)continue;
        const auto before=input.queuedEventCount;
        AddQueuedEvent(input,node.device,PackedEvent(node,1),node);
        if(before==input.queuedEventCount)return false;
        input.nativeDebug.predictedCameraKind=NextNativeDebugCamera(player,input.nativeDebug.predictedCameraKind);
        return true;
    }
    return false;
}
void CloseNativeDebug(GameInputRouter& input,CarFramePlayerState* player)
{
    auto& focus=input.nativeDebug;
    if(focus.active && player) {
        // The same real cycle callbacks are logged for REC/Joylog. Predict
        // their result across multiple buffered samples before dispatch.
        for(int i=0;i<4 && focus.predictedCameraKind!=focus.originalCameraKind;++i) {
            if(QueueNativeDebugCycle(input,*player))continue;
            // Pause has already removed the cycle registration. Its Player
            // is still live, so retire the debug camera through its owner.
            carFramePlayerCycleCameraInput(player,1,0x2d,1);
            focus.predictedCameraKind=carFramePlayerCameraKind(player);
        }
    }
    focus.active=0;focus.startPending=0;focus.chordLatched=0;focus.waitNeutral=1;
    nativeDebugCaption=nullptr;
}
void NeutralNativeControls(JoystickControls& output,const JoystickControls& source)
{
    output=source;output.held=0;
    std::fill_n(output.sticks,4,std::uint8_t{128});std::fill_n(output.pressures,12,std::uint8_t{0});
}
bool NativeControlsNeutral(const JoystickControls* sample,int32_t type)
{
    if(type<0 || sample->held!=0)return false;
    // Release gating uses the native stick center, allowing tiny residual
    // noise within the original scanner deadzone. Triggers are buttons.
    for(const auto value:sample->sticks)if(value<112 || value>144)return false;
    return true;
}
void NativeDebugSample(GameInputRouter& input,int32_t type,const JoystickControls* raw,
    const JoystickControls* oldRaw,const ReplayManager* replay,JoystickControls& normal,JoystickControls& tools)
{
    auto& focus=input.nativeDebug;
    auto* player=NativeDebugPlayer(input);
    if(focus.context!=input.world) {
        focus={};focus.context=input.world;nativeDebugCaption=nullptr;
    }
    if(!focus.initialized) {
        focus.previousNormal=*oldRaw;NeutralNativeControls(focus.previousTools,*raw);
        focus.resetEpoch=nativeInputResetEpoch[0];focus.initialized=1;
        focus.predictedCameraKind=player?carFramePlayerCameraKind(player):1;
    }
    const bool replaying=replay && (replay->restoring || replay->fancyReplayInProgress);
    bool hasCycle=false;
    for(size_t i=0;i<input.count;++i)
        if(input.registrations[i].target.kind==GAME_INPUT_PLAYER_CAMERA)hasCycle=true;
    const bool eligible=applicationDebugToolsEnabled && player && player->registry->count==1 &&
        (player->inputMode==2 || player->inputMode==4) && hasCycle && !replaying && type>=0;
    if(focus.resetEpoch!=nativeInputResetEpoch[0] || type<0 || (focus.active && !eligible)) {
        CloseNativeDebug(input,player);focus.resetEpoch=nativeInputResetEpoch[0];
    }
    normal=*raw;NeutralNativeControls(tools,*raw);
    if(focus.waitNeutral) {
        NeutralNativeControls(normal,*raw);
        if(NativeControlsNeutral(raw,type))focus.waitNeutral=0;
        return;
    }
    if(!eligible) {focus.startPending=0;focus.chordLatched=0;return;}
    constexpr uint16_t start=uint16_t{1}<<3,select=uint16_t{1};
    const bool startDown=(raw->held&start)!=0,selectDown=(raw->held&select)!=0;
    const bool oldStartDown=(oldRaw->held&start)!=0,oldSelectDown=(oldRaw->held&select)!=0;
    bool startPulse=false;
    if(startDown && !oldStartDown)focus.startPending=1;
    if(startDown && selectDown && !oldSelectDown && focus.startPending && !focus.chordLatched) {
        if(focus.active)CloseNativeDebug(input,player);
        else {
            focus.originalCameraKind=focus.predictedCameraKind;
            focus.active=1;
            if(QueueNativeDebugCycle(input,*player))nativeDebugCaption=NativeDebugCameraCaption;
            else CloseNativeDebug(input,player);
        }
        focus.startPending=0;focus.chordLatched=1;focus.waitNeutral=1;
        NeutralNativeControls(normal,*raw);return;
    }
    if(!startDown && focus.startPending) {focus.startPending=0;startPulse=true;}
    if(focus.active) {
        NeutralNativeControls(normal,*raw);tools=*raw;
        tools.held&=static_cast<uint16_t>(~start);
        if(startPulse)tools.held|=start;
        nativeDebugCaption=NativeDebugCameraCaption;
    } else {
        // Delay standalone Start until release only in the opted-in live
        // single-player context. It still reaches the real Pause callback.
        normal.held&=static_cast<uint16_t>(~start);
        if(startPulse)normal.held|=start;
    }
    if(!startDown && !selectDown)focus.chordLatched=0;
}

// PAL153DA0 ignores its last nominal argument in the actual released body.
void AddLoggedEvent(GameInputRouter& input,uint32_t event)
{
    const auto port=event>>16;
    hp2::Require(port<2,"Logged joystick port is outside the released two-port extent");
    for (size_t i=0;i<input.count;++i) {
        const auto& node=input.registrations[i];
        if (node.device==static_cast<int32_t>(port) && node.event==static_cast<int32_t>(event&255)) {
            AddQueuedEvent(input,static_cast<int32_t>(port),static_cast<uint16_t>(event),node);return;
        }
    }
}

uint32_t LoggedEvent(const GameInputQueuedEvent& event)
{
    return event.packedEvent|(static_cast<uint32_t>(event.port)<<16);
}

int GenerateFromScanners(GameInputRouter& input,int32_t port,int32_t type,
    const JoystickControls* current,const JoystickControls* previous,const ReplayManager* replay,int32_t videoBlank,
    const JoystickControls* debugCurrent=nullptr,const JoystickControls* debugPrevious=nullptr)
{
    auto& scanner=input.scanners[port];
    if (type!=scanner.currentType) {
        if (type!=scanner.pendingType) {scanner.pendingType=type;scanner.stableSamples=0;return 0;}
        if (++scanner.stableSamples<32)return 0;
        scanner.currentType=type;
        for (int axis=0;axis<4;++axis)ChooseResponse(scanner.calibration[axis],type,axis);
        for (size_t i=0;i<input.count;++i)if (ScannerPort(input.registrations[i])==port)ConnectScanner(input,input.registrations[i]);
    }
    int generated=0;
    for (size_t i=0;i<input.count;++i) {
        auto& node=input.registrations[i];
        if (ScannerPort(node)!=port || node.scannerConfiguration<0)continue;
        const auto& config=buttonMappings[node.scannerConfiguration];
        if (replay && replay->restoring && !config.fields[6])continue;
        const bool debug=applicationDebugToolsEnabled && NativeDebugTarget(node) && debugCurrent && debugPrevious;
        if (const auto event=ScanEvent(node,scanner,config,debug?debugCurrent:current,debug?debugPrevious:previous,videoBlank)) {
            AddQueuedEvent(input,node.device,event,node);++generated;
            if(debug && node.target.kind==GAME_INPUT_PLAYER_CAMERA)
                input.nativeDebug.predictedCameraKind=NextNativeDebugCamera(*node.target.owner.player,input.nativeDebug.predictedCameraKind);
        }
    }
    return generated;
}
} // namespace

void joystickSetCalibration(GameInputRouter* input,int32_t port,int32_t axis,uint8_t minimum,uint8_t maximum)
{
    hp2::Require(input && port>=0 && port<2 && axis>=0 && axis<4,"Joystick calibration has no actual port/axis");
    auto& calibration=input->scanners[port].calibration[axis];
    calibration.keys[0].coordinate=static_cast<float>(minimum);
    calibration.keys[3].coordinate=static_cast<float>(maximum);
}

void joystickGetCalibration(const GameInputRouter* input,int32_t port,int32_t axis,uint8_t* minimum,uint8_t* maximum)
{
    hp2::Require(input && minimum && maximum && port>=0 && port<2 && axis>=0 && axis<4,"Joystick calibration has no actual port/axis/output");
    const auto& calibration=input->scanners[port].calibration[axis];
    *minimum=static_cast<uint8_t>(convertFloatToInt32(calibration.keys[0].coordinate));
    *maximum=static_cast<uint8_t>(convertFloatToInt32(calibration.keys[3].coordinate));
}

uint8_t joystickGetDeadzone(const GameInputRouter* input,int32_t port,int32_t axis)
{
    hp2::Require(input && port>=0 && port<2 && axis>=0 && axis<4,"Joystick deadzone has no actual port/axis");
    return static_cast<uint8_t>(convertFloatToInt32(input->scanners[port].calibration[axis].keys[2].coordinate-128.0f));
}

GameInputResult joystickGenerateEvents(GameInputRouter& input,JoystickSystem& samples,
    ReplayIntegerLog& log,ReplayManager* replay,const GameTime& clock,DemoDiscManager& demo)
{
    hp2::Require(!input.generatingEvents && !input.dispatching,"Recursive joystick generation is not a source operation");
    hp2::Require(log.clock==&clock,"Joystick and Joylog must borrow the same actual game clock");
    input.queuedEventCount=0;
    if(auto* player=NativeDebugPlayer(input))input.nativeDebug.predictedCameraKind=carFramePlayerCameraKind(player);
    if (log.replaying) {
        const auto count=replayIntegerLogRead(&log,8,1);
        for (uint32_t i=0;i<count;++i)AddLoggedEvent(input,replayIntegerLogRead(&log,19,1));
    } else if (!replay || !replay->fancyReplayInProgress) {
        joystickPrepareFrame(&samples);
        while (joystickHasFrame(&samples)) {
            for (int port=0;port<2;++port) {
                int32_t type;const JoystickControls* previous;
                const auto* current=joystickCurrentData(&samples,port,&type,&previous);
                JoystickControls normal{},tools{};
                const JoystickControls* debugCurrent=nullptr;const JoystickControls* debugPrevious=nullptr;
                if(port==0) {
                    NativeDebugSample(input,type,current,previous,replay,normal,tools);
                    current=&normal;previous=&input.nativeDebug.previousNormal;
                    debugCurrent=&tools;debugPrevious=&input.nativeDebug.previousTools;
                }
                if (GenerateFromScanners(input,port,type,current,previous,replay,
                    static_cast<int32_t>(platformVideoBlankCount()),debugCurrent,debugPrevious))demo.NoteInputTime(clock.realTimer);
                if(port==0) {
                    input.nativeDebug.previousNormal=normal;
                    input.nativeDebug.previousTools=tools;
                }
            }
            joystickAdvanceFrame(&samples);
        }
        const auto count=input.queuedEventCount;
        replayIntegerLogWrite(&log,count,8,1);
        for (uint32_t i=0;i<count;++i)replayIntegerLogWrite(&log,LoggedEvent(input.queuedEvents[i]),19,1);
    }
    if (replay) {
        if (replay->fancyJoyEventNumber<replay->fancyJoyEventCount) {
            hp2::Require(replay->fancyJoyEventNumber>=0 && replay->fancyJoyEventNumber<32,"Replay fancy event queue is outside its source extent");
            const auto event=replay->fancyJoyEvents[replay->fancyJoyEventNumber++];
            if (event)AddLoggedEvent(input,static_cast<uint32_t>(event)+256);
        }
        if (replay->restoring && !replay->replayPausedThisFrame) {
            const auto count=replayManagerRead(replay,8,0);
            for (uint32_t i=0;i<count;++i)AddLoggedEvent(input,replayManagerRead(replay,19,0));
        } else {
            const auto count=input.queuedEventCount;
            replayManagerWrite(replay,count,8,0);
            for (uint32_t i=0;i<count;++i)replayManagerWrite(replay,LoggedEvent(input.queuedEvents[i]),19,0);
        }
    }
    input.generatingEvents=1;
    try {
        for (uint32_t i=0;i<input.queuedEventCount;++i) {
            const auto event=input.queuedEvents[i];
            if (!event.active)continue;
            input.dispatching=1;input.dispatchDevice=event.port;input.dispatchEvent=event.packedEvent&255;input.dispatchCancelled=0;
            const auto result=DispatchTarget(&input,event.target,event.port,event.packedEvent&255,static_cast<uint8_t>(event.packedEvent>>8));
            input.dispatching=0;
            if (result!=GAME_INPUT_OK) {input.generatingEvents=0;return result;}
        }
    } catch (...) {input.dispatching=0;input.generatingEvents=0;throw;}
    input.generatingEvents=0;
    return input.result=GAME_INPUT_OK;
}

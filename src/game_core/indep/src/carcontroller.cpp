#include "indep/inc/CarController.hpp"
#include "indep/inc/Car.hpp"
#include "bware/inc/bWare.hpp"

int32_t carControllerSuperEasyAI=0; // Final330A58 initial word, actual193350 consumer.

// PAL1C0090: construction stores only the real Car reference. Native ownership
// replaces the PS2 vtable/deleting-thunk ABI, without a second Car or state.
void carControllerInitialize(CarController* controller,Car* car)
{
    controller->car=car;
}

// PAL1C0100 already belonged to the1856 accepted bodies as this native
// movement API. Its exact body now lives in the proven CarController unit.
// Existing collision dispatch and tests use the same single implementation.
void carMovementDefaultCollisionNotify(CarMovement* movement)
{
    if (movement->mode != 1 && movement->mode != 4)
        (void)carMovementSetMode(movement, movement->bindings.player ? 1 : 4);
}

// PAL1C0140 and1C0160 affect CarState.performanceScale, independently of
// Car.frictionBoost. A null source Car intentionally has no effect.
void carControllerForceBoost(CarController* controller,float performanceScale)
{
    if (!controller->car)return;
    auto* movement=static_cast<CarMovement*>(controller->car->movement);
    hp2::Require(movement && movement->bindings.owner==controller->car && movement->bindings.state,
        "CarController boost has no canonical CarState");
    movement->bindings.state->performanceScale=performanceScale;
}

void carControllerReleaseBoost(CarController* controller)
{
    if (!controller->car)return;
    auto* movement=static_cast<CarMovement*>(controller->car->movement);
    hp2::Require(movement && movement->bindings.owner==controller->car && movement->bindings.state,
        "CarController release has no canonical CarState");
    movement->bindings.state->performanceScale=1.0f;
}

#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "indep/inc/TrackedController.hpp"
#include "indep/inc/CopDriver.hpp"
#include "indep/inc/PsychoDriver.hpp"
#include "indep/inc/AICarController.hpp"
#include <stdlib.h>

// Same native dispatch/lifetime adapter from src/game/CarDriver.c;0 additional ELF bodies.
CarDriverResult carDriverThink(CarDriver* driver,float elapsed)
{
    switch(driver->kind){
    case CAR_DRIVER_REAL:
        realControllerThink(driver->owner.real,elapsed);
        return driver->result=driver->owner.real->base.result;
    case CAR_DRIVER_AI:
        aiDriverThink(driver->owner.ai,elapsed);
        return driver->result=aiDriverGetResult(driver->owner.ai);
    case CAR_DRIVER_TRAFFIC:
        trafficDriverThink(driver->owner.traffic,elapsed);
        return driver->result=trafficDriverGetResult(driver->owner.traffic);
    case CAR_DRIVER_COP:
        copDriverThink(driver->owner.cop,elapsed);
        return driver->result=driver->owner.cop->base.result;
    case CAR_DRIVER_NULL:return driver->result;
    }
    return driver->result=CAR_DRIVER_INVALID_BINDING;
}

static void thinkControls(void* context,float elapsed)
{
    CarDriver* driver=(CarDriver*)context;
    switch(driver->kind){
    case CAR_DRIVER_REAL:
        realDriverReadInput(&driver->owner.real->input,elapsed);
        driver->result=driver->owner.real->input.result;
        break;
    case CAR_DRIVER_AI:
        aiDriverUpdateControls(driver->owner.ai,elapsed);
        driver->result=aiDriverGetResult(driver->owner.ai);
        break;
    case CAR_DRIVER_TRAFFIC:
        trafficDriverUpdateControls(driver->owner.traffic,elapsed);
        driver->result=trafficDriverGetResult(driver->owner.traffic);
        break;
    case CAR_DRIVER_NULL:case CAR_DRIVER_COP:
        /* These classes use the original empty NullDriver read hook. */
        break;
    }
}

static void thinkController(void* context,float elapsed)
{
    carDriverThink((CarDriver*)context,elapsed);
}

CarDriverResult carDriverAttach(CarDriver* driver,Car* car)
{
    if(!driver||!car)return CAR_DRIVER_INVALID_BINDING;
    switch(driver->kind){
    case CAR_DRIVER_REAL:
        if(!driver->owner.real)return driver->result=CAR_DRIVER_INVALID_BINDING;
        driver->movementView=&driver->owner.real->base.recovery;
        break;
    case CAR_DRIVER_COP:
        if(!driver->owner.cop)return driver->result=CAR_DRIVER_INVALID_BINDING;
        driver->movementView=&driver->owner.cop->base.recovery;
        break;
    case CAR_DRIVER_AI:
        if(!driver->owner.ai||!driver->movementView)return driver->result=CAR_DRIVER_INVALID_BINDING;
        break;
    case CAR_DRIVER_TRAFFIC:
        if(!driver->owner.traffic||!driver->movementView)return driver->result=CAR_DRIVER_INVALID_BINDING;
        break;
    case CAR_DRIVER_NULL:break;
    default:return driver->result=CAR_DRIVER_INVALID_BINDING;
    }
    car->controller=driver;
    car->controllerThink=thinkController;
    car->controls=driver;
    car->updateControls=thinkControls;
    return driver->result;
}

CarDriverResult carDriverSetInputEnabled(CarDriver* driver,int32_t enabled,int32_t port)
{
    if(driver->kind==CAR_DRIVER_REAL){
        RealDriver* input=&driver->owner.real->input;
        if(enabled){
            input->port=port;
            return driver->result=realDriverActivateInput(input);
        }
        return driver->result=realDriverDeactivateInput(input);
    }
    /* The other concrete input tables contain actual empty enable hooks. */
    return driver->result;
}

int32_t carDriverHornActive(const CarDriver* driver)
{
    switch(driver->kind){
    case CAR_DRIVER_AI:return aiDriverHornActive(driver->owner.ai);
    case CAR_DRIVER_TRAFFIC:return trafficDriverHornActive(driver->owner.traffic);
    case CAR_DRIVER_REAL:case CAR_DRIVER_COP:case CAR_DRIVER_NULL:return 0;
    }
    return 0;
}

CarDriverResult carDriverSetHorn(CarDriver* driver,int32_t enabled)
{
    switch(driver->kind){
    case CAR_DRIVER_REAL:
        return driver->result=enabled?realDriverStartHorn(&driver->owner.real->input):realDriverStopHorn(&driver->owner.real->input);
    case CAR_DRIVER_AI:return driver->result=aiDriverSetHorn(driver->owner.ai,enabled);
    case CAR_DRIVER_TRAFFIC:return driver->result=trafficDriverSetHorn(driver->owner.traffic,enabled);
    case CAR_DRIVER_COP:case CAR_DRIVER_NULL:return driver->result;
    }
    return driver->result=CAR_DRIVER_INVALID_BINDING;
}

// Same native dispatch/lifetime adapter from src/game/CarDriverLifecycle.c;0 additional ELF bodies.
void carDriverDispose(CarDriver* driver)
{
    if (!driver || !driver->ownsOwner) return;
    switch (driver->kind) {
    case CAR_DRIVER_REAL:if (driver->owner.real) realControllerDispose(driver->owner.real);free(driver->owner.real);break;
    case CAR_DRIVER_AI:if (driver->owner.ai) aiDriverDispose(driver->owner.ai);free(driver->owner.ai);break;
    case CAR_DRIVER_TRAFFIC:if (driver->owner.traffic) trafficDriverDispose(driver->owner.traffic);free(driver->owner.traffic);break;
    case CAR_DRIVER_COP:free(driver->owner.cop);break;
    case CAR_DRIVER_NULL:free(driver->nullOwner);break;
    }
    CarMovement* movement=driver->environment->movement;
    movement->bindings.driver=NULL;movement->bindings.belt=NULL;
    Car* car=movement->bindings.owner;
    if (car->controller==driver) {
        car->controller=NULL;car->controllerThink=NULL;car->controls=NULL;car->updateControls=NULL;
    }
    driver->owner.ai=NULL;driver->movementView=NULL;driver->nullOwner=NULL;
    driver->ownsOwner=0;driver->kind=CAR_DRIVER_NULL;
}

CarDriverResult carDriverChange(CarDriver* driver,Car* car,CarDriverKind kind)
{
    if (!driver || !car || kind<CAR_DRIVER_NULL || kind>CAR_DRIVER_COP || !driver->environment)
        return CAR_DRIVER_INVALID_BINDING;
    const CarDriverEnvironment* environment=driver->environment;
    CarMovement* movement=environment->movement;
    if (!movement || movement->bindings.owner!=car || !movement->bindings.controls || !movement->bindings.ownerMotion ||
        !environment->physics || environment->physics->movement!=movement ||
        (!driver->ownsOwner && (driver->owner.ai || driver->nullOwner)))
        return driver->result=CAR_DRIVER_INVALID_BINDING;
    if (kind==CAR_DRIVER_REAL && !environment->real) return driver->result=CAR_DRIVER_INVALID_BINDING;
    if (kind==CAR_DRIVER_COP && !environment->pursuit) return driver->result=CAR_DRIVER_MISSING_PURSUIT;
    if ((kind==CAR_DRIVER_AI || kind==CAR_DRIVER_TRAFFIC) && (!environment->ai || !environment->clock))
        return driver->result=CAR_DRIVER_INVALID_BINDING;
    carDriverDispose(driver);
    driver->kind=kind;driver->ownsOwner=1;
    movement->bindings.ownerMotion->inputCategory=(int32_t)kind;
    CarDriverResult result=CAR_DRIVER_OK;
    switch (kind) {
    case CAR_DRIVER_NULL:
        driver->nullOwner=(NullController*)calloc(1,sizeof(*driver->nullOwner));
        if (!driver->nullOwner) {result=CAR_DRIVER_ALLOCATION_FAILURE;break;}
        carControllerInitialize(&driver->nullOwner->base,car);
        driver->nullOwner->movement=movement;driverCommandInitialize(&driver->nullOwner->command);
        driver->movementView=&driver->nullOwner->recovery;
        movement->bindings.controls->steeringAngle=0;movement->bindings.controls->throttle=0;
        movement->bindings.controls->brake=0;movement->bindings.controls->handbrake=0;
        movement->bindings.controls->gearRequest=1;movement->bindings.controls->nitroRequest=0;
        driverCommandAdvance(&driver->nullOwner->command,NULL,environment->clock->worldTimer,0.0f);
        break;
    case CAR_DRIVER_REAL: {
        // PAL RealController::Think23AAC8..23AADC reads the live Car+18
        // Player owner. Snapshot replacement retains that Car/Player pair;
        // refresh the native borrowed alias when rebuilding its controller.
        RealDriverBindings input=*environment->real;
        if (movement->frame)input.player=movement->frame->environment.player;
        driver->owner.real=(RealController*)calloc(1,sizeof(*driver->owner.real));
        if (!driver->owner.real) {result=CAR_DRIVER_ALLOCATION_FAILURE;break;}
        result=realControllerInitialize(driver->owner.real,movement,environment->physics,&input);
        driver->movementView=&driver->owner.real->base.recovery;
        break;
    }
    case CAR_DRIVER_AI:
        driver->owner.ai=(AIDriver*)calloc(1,sizeof(*driver->owner.ai));
        if (!driver->owner.ai) {result=CAR_DRIVER_ALLOCATION_FAILURE;break;}
        result=aiDriverInitialize(driver->owner.ai,movement,environment->physics,environment->ai,environment->clock,environment->sound);
        if (result==CAR_DRIVER_OK) driver->movementView=&driver->owner.ai->base.currentRoad->recovery;
        break;
    case CAR_DRIVER_TRAFFIC:
        driver->owner.traffic=(TrafficDriver*)calloc(1,sizeof(*driver->owner.traffic));
        if (!driver->owner.traffic) {result=CAR_DRIVER_ALLOCATION_FAILURE;break;}
        result=trafficDriverInitialize(driver->owner.traffic,movement,environment->physics,environment->ai,environment->clock,environment->sound);
        if (result==CAR_DRIVER_OK) driver->movementView=&driver->owner.traffic->base.currentRoad->recovery;
        break;
    case CAR_DRIVER_COP:
        driver->owner.cop=(CopDriver*)calloc(1,sizeof(*driver->owner.cop));
        if (!driver->owner.cop) {result=CAR_DRIVER_ALLOCATION_FAILURE;break;}
        result=copDriverInitialize(driver->owner.cop,movement,environment->pursuit);
        driver->movementView=&driver->owner.cop->base.recovery;
        break;
    }
    if (result!=CAR_DRIVER_OK) {
        carDriverDispose(driver);return driver->result=result;
    }
    movement->bindings.driver=driver->movementView;
    driver->result=CAR_DRIVER_OK;
    return carDriverAttach(driver,car);
}

// Same native dispatch/lifetime adapter from src/game/CarDriverMetrics.c;0 additional ELF bodies.
float carDriverRaceSpeedMetric(const CarDriver* driver)
{
    switch(driver->kind){
    case CAR_DRIVER_AI:return driver->owner.ai->input.targetSpeed;
    case CAR_DRIVER_TRAFFIC:return driver->owner.traffic->input.targetSpeed;
    case CAR_DRIVER_REAL:case CAR_DRIVER_COP:case CAR_DRIVER_NULL:return 0.0f;
    }
    return 0.0f;
}

static DriverCommand* commandForDriver(const CarDriver* driver)
{
    if(!driver)return NULL;
    switch(driver->kind){
    case CAR_DRIVER_REAL:return driver->owner.real?&driver->owner.real->input.command:NULL;
    case CAR_DRIVER_AI:return driver->owner.ai?&driver->owner.ai->input.command:NULL;
    case CAR_DRIVER_TRAFFIC:return driver->owner.traffic?&driver->owner.traffic->input.command:NULL;
    case CAR_DRIVER_COP:return driver->owner.cop?&driver->owner.cop->command:NULL;
    case CAR_DRIVER_NULL:return driver->nullOwner?&driver->nullOwner->command:NULL;
    }
    return NULL;
}

const DriverCommand* carDriverCommand(const CarDriver* driver)
{
    return commandForDriver(driver);
}

DriverCommand* carDriverMutableCommand(CarDriver* driver)
{
    return commandForDriver(driver);
}

float carDriverLateralExcess(const CarDriver* driver)
{
    switch(driver->kind){
    case CAR_DRIVER_AI:return driverAILateralExcess(&driver->owner.ai->base);
    case CAR_DRIVER_TRAFFIC:return driverAILateralExcess(&driver->owner.traffic->base);
    case CAR_DRIVER_REAL:return driverRouteLateralExcess(&driver->owner.real->base);
    case CAR_DRIVER_COP:return driverRouteLateralExcess(&driver->owner.cop->base);
    case CAR_DRIVER_NULL:return 0.0f;
    }
    return 0.0f;
}

// Same native dispatch/lifetime adapter from src/game/CarDriverRoute.c;0 additional ELF bodies.
const CarRecoveryGroup* carDriverCurrentGroup(const CarDriver* driver,int32_t refreshIfMissing)
{
    switch(driver->kind){
    case CAR_DRIVER_REAL:return driverRouteCurrentGroup(&driver->owner.real->base);
    case CAR_DRIVER_AI:return aiDriverCurrentGroup(driver->owner.ai,refreshIfMissing);
    case CAR_DRIVER_TRAFFIC:return trafficDriverCurrentGroup(driver->owner.traffic,refreshIfMissing);
    case CAR_DRIVER_COP:return driverRouteCurrentGroup(&driver->owner.cop->base);
    case CAR_DRIVER_NULL:return NULL;
    }
    return NULL;
}

const CarRecoveryPose* carDriverCurrentPose(const CarDriver* driver,int32_t refreshIfMissing)
{
    switch(driver->kind){
    case CAR_DRIVER_REAL:return driver->owner.real->base.recovery.recoveryPose;
    case CAR_DRIVER_AI:return aiDriverCurrentPose(driver->owner.ai,refreshIfMissing);
    case CAR_DRIVER_TRAFFIC:return trafficDriverCurrentPose(driver->owner.traffic,refreshIfMissing);
    case CAR_DRIVER_COP:return driver->owner.cop->base.recovery.recoveryPose;
    case CAR_DRIVER_NULL:return NULL;
    }
    return NULL;
}

// Same native dispatch/lifetime adapter from src/game/CarDriverPlacement.c;0 additional ELF bodies.
static CarDriverResult refreshRoute(DriverRoute* route)
{
    carMovementRefreshRecoveryPose(route->movement);
    if(route->movement->result!=CAR_MOVEMENT_OK)
        route->result=CAR_DRIVER_MOVEMENT_FAILURE;
    return route->result;
}

CarDriverResult carDriverOnCarPlaced(CarDriver* driver)
{
    if(!driver)return CAR_DRIVER_INVALID_BINDING;
    switch(driver->kind){
    case CAR_DRIVER_REAL:
        if(!driver->owner.real)return driver->result=CAR_DRIVER_INVALID_BINDING;
        return driver->result=refreshRoute(&driver->owner.real->base);
    case CAR_DRIVER_COP:
        if(!driver->owner.cop)return driver->result=CAR_DRIVER_INVALID_BINDING;
        return driver->result=refreshRoute(&driver->owner.cop->base);
    case CAR_DRIVER_AI:
        if(!driver->owner.ai)return driver->result=CAR_DRIVER_INVALID_BINDING;
        driverAIRefreshRoad(&driver->owner.ai->base);
        if(driver->owner.ai->base.result==CAR_DRIVER_OK)
            driverAIResetAfterPlacement(&driver->owner.ai->base);
        return driver->result=aiDriverGetResult(driver->owner.ai);
    case CAR_DRIVER_TRAFFIC:
        if(!driver->owner.traffic)return driver->result=CAR_DRIVER_INVALID_BINDING;
        driverAIRefreshRoad(&driver->owner.traffic->base);
        if(driver->owner.traffic->base.result==CAR_DRIVER_OK)
            trafficDriverResetAfterPlacement(driver->owner.traffic);
        return driver->result=trafficDriverGetResult(driver->owner.traffic);
    case CAR_DRIVER_NULL:
        /* Both original inactive-controller placement hooks are empty. */
        return driver->result;
    }
    return driver->result=CAR_DRIVER_INVALID_BINDING;
}

// ResetCore132E38..132E6C calls the input vtable independently of the
// controller's later JustReset. PsychoDriver2AEC50 clears both angle filters
// at139B28 and selects automatic gear at13ACD8. RealDriver2AEE30 has an
// empty steering hook136C38 and its own gear-mode rule13B3D0. NullDriver
// 2AE7C0 (also used by CopController) has two actual empty source hooks.
CarDriverResult carDriverClearSteeringMemory(CarDriver* driver)
{
    if(!driver)return CAR_DRIVER_INVALID_BINDING;
    switch(driver->kind) {
    case CAR_DRIVER_AI:
        if(!driver->owner.ai)return driver->result=CAR_DRIVER_INVALID_BINDING;
        aiControlsClearAngleFilters(&driver->owner.ai->input);
        return driver->result=aiDriverGetResult(driver->owner.ai);
    case CAR_DRIVER_TRAFFIC:
        if(!driver->owner.traffic)return driver->result=CAR_DRIVER_INVALID_BINDING;
        aiControlsClearAngleFilters(&driver->owner.traffic->input);
        return driver->result=trafficDriverGetResult(driver->owner.traffic);
    case CAR_DRIVER_REAL:case CAR_DRIVER_COP:case CAR_DRIVER_NULL:return driver->result;
    }
    return driver->result=CAR_DRIVER_INVALID_BINDING;
}

CarDriverResult carDriverSelectResetGear(CarDriver* driver)
{
    if(!driver)return CAR_DRIVER_INVALID_BINDING;
    switch(driver->kind) {
    case CAR_DRIVER_REAL:
        if(!driver->owner.real)return driver->result=CAR_DRIVER_INVALID_BINDING;
        realDriverSelectDefaultGear(&driver->owner.real->input);
        return driver->result=driver->owner.real->input.result;
    case CAR_DRIVER_AI:
        if(!driver->owner.ai)return driver->result=CAR_DRIVER_INVALID_BINDING;
        aiControlsNeutralGear(&driver->owner.ai->input);
        return driver->result=aiDriverGetResult(driver->owner.ai);
    case CAR_DRIVER_TRAFFIC:
        if(!driver->owner.traffic)return driver->result=CAR_DRIVER_INVALID_BINDING;
        aiControlsNeutralGear(&driver->owner.traffic->input);
        return driver->result=trafficDriverGetResult(driver->owner.traffic);
    case CAR_DRIVER_COP:case CAR_DRIVER_NULL:return driver->result;
    }
    return driver->result=CAR_DRIVER_INVALID_BINDING;
}

// Car::Reset (PAL132FE4..133018) invokes SetCurrentRoute then JustReset.
// Native dispatch preserves those two calls on the same concrete controller.
// AI vt2AE328/2B4D90 uses190670 +190850; Traffic vt2B4730 uses190670
// +18CB10. Real/Cop inherit23A178 plus the empty23A2F8 reset hook.
CarDriverResult carDriverResetOnRoad(CarDriver* driver,const CarRecoveryGroup* group,
                                    const CarRecoveryPose* pose)
{
    if(!driver || !group || !pose)return CAR_DRIVER_INVALID_BINDING;
    switch(driver->kind) {
    case CAR_DRIVER_REAL:
        if(!driver->owner.real)return driver->result=CAR_DRIVER_INVALID_BINDING;
        driverRouteSetPose(&driver->owner.real->base,pose);
        return driver->result=driver->owner.real->base.result;
    case CAR_DRIVER_COP:
        if(!driver->owner.cop)return driver->result=CAR_DRIVER_INVALID_BINDING;
        driverRouteSetPose(&driver->owner.cop->base,pose);
        return driver->result=driver->owner.cop->base.result;
    case CAR_DRIVER_AI:
        if(!driver->owner.ai)return driver->result=CAR_DRIVER_INVALID_BINDING;
        driverAISetRoad(&driver->owner.ai->base,group,pose);
        if(driver->owner.ai->base.result==CAR_DRIVER_OK)
            driverAIResetAfterPlacement(&driver->owner.ai->base);
        return driver->result=aiDriverGetResult(driver->owner.ai);
    case CAR_DRIVER_TRAFFIC:
        if(!driver->owner.traffic)return driver->result=CAR_DRIVER_INVALID_BINDING;
        driverAISetRoad(&driver->owner.traffic->base,group,pose);
        if(driver->owner.traffic->base.result==CAR_DRIVER_OK)
            trafficDriverResetAfterPlacement(driver->owner.traffic);
        return driver->result=trafficDriverGetResult(driver->owner.traffic);
    case CAR_DRIVER_NULL:
        // The two corresponding NullController entries are empty.
        return driver->result;
    }
    return driver->result=CAR_DRIVER_INVALID_BINDING;
}

#include "indep/inc/Replay.hpp"
#include "indep/inc/PsychoDriver.hpp"
// PAL1C00D0..1C00FC is the controller's virtual call to its real input
// object's DoSnapshot. This native dispatch preserves that single owner.
void carControllerSnapshot(CarMovement* movement,ReplaySnapshot* snapshot)
{
    CarDriver* driver=static_cast<CarDriver*>(movement->bindings.owner->controller);
    hp2::Require(driver&&driver->environment&&driver->environment->movement==movement,
        "CarController snapshot has no actual controller/input binding");
    switch(driver->kind) {
    case CAR_DRIVER_REAL:realDriverDoSnapshot(&driver->owner.real->input,snapshot);break;
    case CAR_DRIVER_AI:aiControlsSnapshot(&driver->owner.ai->input,snapshot);break;
    case CAR_DRIVER_TRAFFIC:aiControlsSnapshot(&driver->owner.traffic->input,snapshot);break;
    case CAR_DRIVER_COP:carDriverInputSnapshot(movement,&driver->owner.cop->command,snapshot);break;
    case CAR_DRIVER_NULL:carDriverInputSnapshot(movement,&driver->nullOwner->command,snapshot);break;
    }
}

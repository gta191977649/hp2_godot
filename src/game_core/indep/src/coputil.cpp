#include "Chopper.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/TrackPath.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>

#include "indep/inc/CarInfo.hpp"
#include "indep/inc/Pursuit.hpp"

// PAL230F00..230FF0. Each segment is a point plus its displacement. Keep
// the source's cross-product tests, including its collinear acceptance.
int32_t pursuitMotionSegmentsIntersect(const float first[2],const float firstMotion[2],
    const float second[2],const float secondMotion[2])
{
    const float secondEndX=second[0]+secondMotion[0];
    const float secondEndY=second[1]+secondMotion[1];
    const float firstX=second[0]-first[0],firstY=second[1]-first[1];
    const float endX=secondEndX-first[0],endY=secondEndY-first[1];
    const float a=firstX*firstMotion[1],b=firstY*firstMotion[0];
    const float c=endX*firstMotion[1],d=endY*firstMotion[0];
    const float side0=a-b,side1=c-d;
    if(!(side0*side1<=0.0f))return 0;
    const float firstEndX=first[0]+firstMotion[0];
    const float firstEndY=first[1]+firstMotion[1];
    const float otherX=first[0]-second[0],otherY=first[1]-second[1];
    const float otherEndX=firstEndX-second[0],otherEndY=firstEndY-second[1];
    const float e=otherX*secondMotion[1],f=otherY*secondMotion[0];
    const float g=otherEndX*secondMotion[1],h=otherEndY*secondMotion[0];
    const float side2=e-f,side3=g-h;
    return side2*side3<=0.0f;
}

// PAL230E18..230F00. This query tests the prospective point against ALL
// world cars. It is distinct from CanTeleport's current-position scan.
WorldServicesResult pursuitPositionClear(const World* world,const float position[2],int32_t* clear)
{
    if(!world || !position || !clear || world->totalCars<0 || world->totalCars>WORLD_MAX_CARS)
        return WORLD_SERVICES_INVALID_BINDING;
    *clear=0;
    if(worldGetDistanceToClosestPlayer2D(world,position)<250.0f)return WORLD_SERVICES_OK;
    for(int32_t i=0;i<world->totalCars;++i) {
        const Car* car=world->carTable[i];if(!car)return WORLD_SERVICES_INVALID_BINDING;
        const float dx=position[0]-car->position[0],dy=position[1]-car->position[1];
        const float y2=dy*dy,x2=dx*dx;
        if(sqrtf(x2+y2)<15.0f)return WORLD_SERVICES_OK;
    }
    *clear=1;return WORLD_SERVICES_OK;
}

// PAL231338..2313DC. The acceleration samples and mutable physical info are
// borrowed from the one model catalogue; no eager table/cache rebuild occurs.
WorldServicesResult pursuitCarTopSpeed(const CarMovement* movement,
    const hp2::CarInfoResource* resources,float* output)
{
    if(!movement || !resources || !output || !resources->CarsLoaded() ||
        !movement->bindings.owner || !movement->bindings.owner->driverInfo)
        return WORLD_SERVICES_INVALID_BINDING;
    const int32_t model=movement->bindings.owner->driverInfo->appearance.modelCode;
    if(model<0 || static_cast<size_t>(model)>=resources->Cars().size())
        return WORLD_SERVICES_INVALID_BINDING;
    const auto& definition=resources->Car(static_cast<size_t>(model));
    const PhysicsCarInfo& info=movement->info;
    if(info.body!=&definition.rigidBodyInfo || info.aerodynamics!=&definition.aerodynamicsInfo)
        return WORLD_SERVICES_INVALID_BINDING;
    const SampledCurve acceleration=resources->AccelerationCurve(static_cast<size_t>(model));
    float speed=27.77777862548828f;
    const float step=1.388888955116272f;
    while(speed<111.11111450195312f){
        const float available=sampledCurveEvaluate(&acceleration,speed);
        const float squared=speed*speed;
        const float drag=squared*info.aerodynamics->drag;
        const float resistance=finiteRatio(drag,info.body->mass);
        if(available<resistance)break;
        speed=speed+step;
    }
    *output=speed-step;
    return WORLD_SERVICES_OK;
}

float chopperWrapDegrees(float degrees)
{
    while (degrees>180.0f) degrees=bScalarAdd(degrees,-360.0f);
    while (degrees< -180.0f) degrees=bScalarAdd(degrees,360.0f);
    return degrees;
}

#include "indep/inc/Car.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Collision.hpp"

#include <math.h>

namespace {

// Native form of the original local contact iterator shared by the two
// queries. The index and output borrow the actual collision system; there
// is no retained contact list, copied participant or replacement callback.
CarDriverResult nextCollidingCar(CarMovement* movement, int32_t* index, CarMovement** other)
{
    if (!movement || !movement->bindings.owner || !movement->bindings.ownerMotion ||
        !movement->bindings.ownerMotion->collision ||
        !movement->bindings.ownerMotion->collision->shape)
        return CAR_DRIVER_INVALID_BINDING;
    const CollisionShape* shape = movement->bindings.ownerMotion->collision->shape;
    *other = nullptr;
    while (*index < shape->contactCount) {
        if (!movement->collisionSystem) return CAR_DRIVER_INVALID_BINDING;
        const CollisionImpact* impact = collisionShapeContactAt(movement->collisionSystem, shape, *index);
        ++*index;
        if (!impact) return CAR_DRIVER_INVALID_BINDING;
        if (!collisionImpactIsCarPair(impact)) continue;
        Car* first = nullptr;
        Car* second = nullptr;
        collisionImpactGetCars(impact, &first, &second);
        const Car* own = movement->bindings.owner;
        if (!first || !second || (first != own && second != own)) return CAR_DRIVER_INVALID_BINDING;
        Car* car = first == own ? second : first;
        CarMovement* candidate = static_cast<CarMovement*>(car->movement);
        if (!candidate || candidate->bindings.owner != car || !candidate->bindings.state)
            return CAR_DRIVER_INVALID_BINDING;
        *other = candidate;
        return CAR_DRIVER_OK;
    }
    return CAR_DRIVER_OK;
}

}

CarDriverResult pursuitCanTeleport(const World* world, const CarMovement* movement,
    const float position[2], int32_t* clear)
{
    if (!world || !movement || !movement->bindings.owner || !movement->bindings.state ||
        !movement->bindings.state->position || !position || !clear ||
        world->totalCars < 0 || world->totalCars > WORLD_MAX_CARS)
        return CAR_DRIVER_INVALID_BINDING;
    *clear = 0;
    const float* current = movement->bindings.state->position;
    if (worldGetDistanceToClosestPlayer2D(world, current) < 250.0f) return CAR_DRIVER_OK;
    if (worldGetDistanceToClosestPlayer2D(world, position) < 250.0f) return CAR_DRIVER_OK;
    for (int32_t index = 0; index < world->totalCars; ++index) {
        const Car* car = world->carTable[index];
        if (!car) return CAR_DRIVER_INVALID_BINDING;
        if (car == movement->bindings.owner) continue;
        // Source checks other cars around the CURRENT position here, not
        // around the prospective teleport point used by the prior query.
        const float x = current[0] - car->position[0];
        const float y = current[1] - car->position[1];
        const float xx = x * x;
        const float yy = y * y;
        if (sqrtf(xx + yy) < 15.0f) return CAR_DRIVER_OK;
    }
    *clear = 1;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitCarCausedRecentCollision(CarMovement* movement, CarMovement** other)
{
    if (!movement || !movement->bindings.state || !other) return CAR_DRIVER_INVALID_BINDING;
    int32_t index = 0;
    for (;;) {
        CarMovement* candidate = nullptr;
        const CarDriverResult result = nextCollidingCar(movement, &index, &candidate);
        if (result != CAR_DRIVER_OK) return result;
        if (!candidate) {
            *other = nullptr;
            return CAR_DRIVER_OK;
        }
        const VehicleState* state = candidate->bindings.state;
        const float speed = state->forwardSpeed;
        if (speed == 0.0f ||
            (driverCarInHeadingCone(movement->bindings.state, state, 0x2000u) &&
             0.0f <= speed && speed < movement->bindings.state->forwardSpeed)) {
            *other = candidate;
            return CAR_DRIVER_OK;
        }
    }
}

CarDriverResult pursuitCarsCollided(CarMovement* first, const CarMovement* second, int32_t* collided)
{
    if (!first || !second || !collided) return CAR_DRIVER_INVALID_BINDING;
    int32_t index = 0;
    for (;;) {
        CarMovement* other = nullptr;
        const CarDriverResult result = nextCollidingCar(first, &index, &other);
        if (result != CAR_DRIVER_OK) return result;
        if (!other) {
            *collided = 0;
            return CAR_DRIVER_OK;
        }
        if (other == second) {
            *collided = 1;
            return CAR_DRIVER_OK;
        }
    }
}

// RotateVector PAL2313E0..231474 belongs to coputil.cpp.
void chopperRotate2(float output[2],const float input[2],float degrees)
{
    const int32_t quantized=convertFloatToInt32(degrees*65536.0f)/360;
    float sine,cosine;vectorSineCosineAngle(&sine,&cosine,(uint16_t)quantized);
    const float x=input[0],y=input[1];
    const float ys=y*cosine,xs=x*sine,xc=x*cosine,yc=y*sine;
    output[1]=ys-xs;output[0]=xc+yc;
}

#include "indep/inc/Collision.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Smokeable.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/Munitions.hpp"
#include <limits.h>
#include "indep/inc/Contact.hpp"
#include <stddef.h>
#include "indep/inc/VoronoiClip.hpp"
#include "indep/inc/Contact.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Contact.hpp"

#include <math.h>
#include <string.h>

void collisionPointVelocity(float output[4],const float linear[4],const float angular[4],const float lever[4])
{
    float rotational[4];collisionResponseCross3(rotational,angular,lever);
    for(int lane=0;lane<3;++lane) output[lane]=linear[lane]+rotational[lane];
    output[3]=0.0f;
}

void collisionSystemDetachContact(CollisionQueuedContact* contact)
{
    contact->previous->next=contact->next;
    contact->next->previous=contact->previous;
    contact->occupied=0;
}

void collisionQueueRemoveShape(CollisionSystem* system, const CollisionShape* shape)
{
    CollisionQueuedContact* contact=system->contactRoot.next;
    while (contact!=&system->contactRoot) {
        CollisionQueuedContact* next=contact->next;
        if (contact->view.first==shape || contact->view.second==shape) collisionSystemDetachContact(contact);
        contact=next;
    }
}

float collisionAngularInverseMass(const float inverseInertia[16],const float lever[4],const float normal[4])
{
    float crossed[4],transformed[4],velocity[4];
    collisionResponseCross3(crossed,lever,normal);
    vectorMatrixTransformPoint(transformed,inverseInertia,crossed);
    collisionResponseCross3(velocity,transformed,lever);
    const float x=normal[0]*velocity[0],y=normal[1]*velocity[1],z=normal[2]*velocity[2];
    return (x+y)+z;
}

float collisionContactFindTime(CollisionSystem* system, CollisionQueuedContact* contact)
{
    CollisionShape* first=contact->view.first;
    CollisionShape* second=contact->view.second;
    const float firstTime=first->active ? first->previousTime : 1000.0f;
    const float secondTime=second->active ? second->previousTime : 1000.0f;
    float upper=minimumFinite(firstTime,secondTime), lower=0.0f, midpoint=0.0f;
    int32_t result=0;
    memset(contact->displacement,0,sizeof(contact->displacement));
    for (int iteration=0;iteration<8;++iteration) {
        const float half=(upper-lower)*0.5f;
        midpoint=lower+half;
        float firstPosition[4],secondPosition[4],firstFrame[16],secondFrame[16];
        collisionResponseInterpolatePose(first,firstPosition,firstFrame,midpoint);
        collisionResponseInterpolatePose(second,secondPosition,secondFrame,midpoint);
        result=collisionWitnessQuery(&contact->pair->witness,firstFrame,firstPosition,
                                     secondFrame,secondPosition,system->randomState);
        if (result==2) lower=midpoint;
        else {
            if (contact->pair->witness.distance<system->tuning->contactDistance) return midpoint;
            upper=midpoint;
        }
    }
    if (result==2) collisionContactSeparate(system,contact,midpoint);
    return midpoint;
}

static void contactPlaneNormalAtPosition(float normal[4], const CollisionShape* planeShape,
    const float position[4], const CollisionContactTuning* tuning)
{
    const CollisionFeature* plane=&planeShape->geometry->faces[0];
    const float projection=collisionResponseDot3(plane->vector,position);
    const float limit=plane->offset-tuning->planeBias;
    const int positive=limit<projection;
    for(int lane=0;lane<3;++lane) normal[lane]=positive ? plane->vector[lane] : 0.0f-plane->vector[lane];
    normal[3]=0.0f;
}

void collisionContactSeparate(CollisionSystem* system, CollisionQueuedContact* contact, float rewind)
{
    const CollisionShape* first=contact->view.first;
    const CollisionShape* second=contact->view.second;
    float firstPosition[4],secondPosition[4],firstFrame[16],secondFrame[16],normal[4]={};
    collisionResponseInterpolatePose(first,firstPosition,firstFrame,rewind);
    collisionResponseInterpolatePose(second,secondPosition,secondFrame,rewind);
    // Native continuous-contact correction: the source PAL separates these
    // interpolated poses but chooses the plane side using the end-of-step
    // center. A thin body crossing that center plane can consequently be
    // pushed through the floor. Select the side at this same rewind pose;
    // the original bounded separation search, impulse solve and residual
    // advance remain responsible for the actual contact response.
    if (first->kind==3) contactPlaneNormalAtPosition(normal,first,secondPosition,&system->tuning->contactOrder);
    else if (second->kind==3) {
        contactPlaneNormalAtPosition(normal,second,firstPosition,&system->tuning->contactOrder);
        for (int lane=0;lane<3;++lane) normal[lane]=0.0f-normal[lane];
    } else {
        for (int lane=0;lane<3;++lane) normal[lane]=secondPosition[lane]-firstPosition[lane];
        const float inverseLength=finiteRatio(1.0f,sqrtf(collisionResponseDot3(normal,normal)));
        for (int lane=0;lane<3;++lane) normal[lane]=normal[lane]*inverseLength;
    }
    float distance=0.0f,step=system->tuning->separationInitialStep;
    for (int iteration=0;iteration<20;++iteration) {
        distance=distance+step;
        if (step<system->tuning->separationMaximumStep) step=step+system->tuning->separationStepIncrease;
        float displacement[4]={},adjustedFirst[4],adjustedSecond[4];
        for (int lane=0;lane<3;++lane) displacement[lane]=normal[lane]*distance;
        const float limit=system->tuning->separationVerticalLimit;
        if (limit<displacement[2]) displacement[2]=limit;
        else if (displacement[2]<-limit) displacement[2]=-limit;
        for (int lane=0;lane<3;++lane) {
            adjustedFirst[lane]=firstPosition[lane]-(collisionParticipantMovable(contact->pair->first) ? displacement[lane] : 0.0f);
            adjustedSecond[lane]=secondPosition[lane]+(collisionParticipantMovable(contact->pair->second) ? displacement[lane] : 0.0f);
        }
        adjustedFirst[3]=adjustedSecond[3]=1.0f;
        if (collisionWitnessQuery(&contact->pair->witness,firstFrame,adjustedFirst,
                                   secondFrame,adjustedSecond,system->randomState)!=2) {
            memcpy(contact->displacement,displacement,sizeof(displacement));
            return;
        }
    }
}

void collisionContactFrame(const CollisionSystem* system, const CollisionQueuedContact* contact,
                            float point[4], float normal[4])
{
    const CollisionShape* first=contact->view.first;
    const CollisionShape* second=contact->view.second;
    float firstPoint[4],secondPoint[4];
    vectorMatrixTransformPoint(firstPoint,collisionResponseShapeFrame(first),contact->pair->witness.firstPoint);
    vectorMatrixTransformPoint(secondPoint,collisionResponseShapeFrame(second),contact->pair->witness.secondPoint);
    for (int lane=0;lane<3;++lane) {
        firstPoint[lane]=firstPoint[lane]+collisionResponseShapePosition(first)[lane];
        secondPoint[lane]=secondPoint[lane]+collisionResponseShapePosition(second)[lane];
        point[lane]=(firstPoint[lane]+secondPoint[lane])*0.5f;
    }
    point[3]=1.0f;
    normal[3]=0.0f;
    if (!first->active) collisionContactPlaneNormal(normal,first,second,&system->tuning->contactOrder);
    else if (!second->active) {
        collisionContactPlaneNormal(normal,second,first,&system->tuning->contactOrder);
        for (int lane=0;lane<3;++lane) normal[lane]=0.0f-normal[lane];
    } else {
        for (int lane=0;lane<3;++lane) normal[lane]=secondPoint[lane]-firstPoint[lane];
        float length=sqrtf(collisionResponseDot3(normal,normal));
        if (length<system->tuning->coincidentPointDistance) {
            for (int lane=0;lane<3;++lane) normal[lane]=collisionResponseShapePosition(second)[lane]-collisionResponseShapePosition(first)[lane];
            length=sqrtf(collisionResponseDot3(normal,normal));
        }
        const float inverseLength=finiteRatio(1.0f,length);
        for (int lane=0;lane<3;++lane) normal[lane]=normal[lane]*inverseLength;
    }
}

void collisionContactProjectPoint(const CollisionQueuedContact* contact, float point[4])
{
    float between[4]={},upTangent[4]={};
    const float* firstPosition=collisionResponseShapePosition(contact->view.first);
    const float* secondPosition=collisionResponseShapePosition(contact->view.second);
    for (int lane=0;lane<3;++lane) between[lane]=secondPosition[lane]-firstPosition[lane];
    const float inverseLength=finiteRatio(1.0f,sqrtf(collisionResponseDot3(between,between)));
    for (int lane=0;lane<3;++lane) between[lane]=between[lane]*inverseLength;
    const float verticalProjection=-between[2];
    for (int lane=0;lane<3;++lane) upTangent[lane]=between[lane]*verticalProjection;
    upTangent[2]=upTangent[2]+1.0f;
    const float inverseTangentLength=finiteRatio(1.0f,sqrtf(collisionResponseDot3(upTangent,upTangent)));
    for (int lane=0;lane<3;++lane) upTangent[lane]=upTangent[lane]*inverseTangentLength;
    float lever[4]={};
    for (int lane=0;lane<3;++lane) lever[lane]=point[lane]-firstPosition[lane];
    const float projection=-collisionResponseDot3(lever,upTangent);
    for (int lane=0;lane<3;++lane) point[lane]=point[lane]+upTangent[lane]*projection;
    point[3]=1.0f;
}

void collisionContactSolve(CollisionSystem* system, CollisionQueuedContact* contact)
{
    CollisionParticipant* first=contact->pair->first;
    CollisionParticipant* second=contact->pair->second;
    float point[4],normal[4],firstLever[4]={},secondLever[4]={};
    collisionContactFrame(system,contact,point,normal);
    /* The impact is retained even when this contact is already separating. */
    collisionPairImpact(system,contact->pair,point);
    if (system->result!=COLLISION_SYSTEM_OK) return;
    for (int lane=0;lane<3;++lane) {
        firstLever[lane]=point[lane]-collisionResponseShapePosition(first->shape)[lane];
        secondLever[lane]=point[lane]-collisionResponseShapePosition(second->shape)[lane];
    }
    for (;;) {
        float firstLinear[4],firstAngular[4],secondLinear[4],secondAngular[4];
        float firstVelocity[4],secondVelocity[4],relative[4]={},tangent[4]={};
        collisionParticipantVelocity(first,firstLinear,firstAngular);
        collisionParticipantVelocity(second,secondLinear,secondAngular);
        collisionPointVelocity(firstVelocity,firstLinear,firstAngular,firstLever);
        collisionPointVelocity(secondVelocity,secondLinear,secondAngular,secondLever);
        for (int lane=0;lane<3;++lane) relative[lane]=secondVelocity[lane]-firstVelocity[lane];
        float normalSpeed=collisionResponseDot3(relative,normal);
        for (int lane=0;lane<3;++lane) tangent[lane]=normal[lane]*(-normalSpeed)+relative[lane];
        if (0.0f<=normalSpeed) return;
        const float restitution=maximumFinite(collisionParticipantRestitution(system,first),
                                               collisionParticipantRestitution(system,second));
        const float firstInverseMass=collisionParticipantInverseMass(system,first);
        const float secondInverseMass=collisionParticipantInverseMass(system,second);
        float inertia[16];
        float inverseMass=firstInverseMass+secondInverseMass;
        collisionParticipantInverseInertia(system,first,inertia);
        inverseMass=inverseMass+collisionAngularInverseMass(inertia,firstLever,normal);
        collisionParticipantInverseInertia(system,second,inertia);
        inverseMass=inverseMass+collisionAngularInverseMass(inertia,secondLever,normal);
        const float maximumInverseMass=maximumFinite(firstInverseMass,secondInverseMass);
        const float minimumImpulse=finiteRatio(system->tuning->normalImpulseMinimum,maximumInverseMass);
        const float numerator=-(restitution+1.0f)*normalSpeed;
        float magnitude=finiteRatio(numerator,inverseMass);
        if (magnitude<minimumImpulse) magnitude=minimumImpulse;
        float normalImpulse[4]={},negativeImpulse[4]={};
        for (int lane=0;lane<3;++lane) {
            normalImpulse[lane]=normal[lane]*magnitude;
            negativeImpulse[lane]=0.0f-normalImpulse[lane];
        }
        // Native resource/effect callbacks can fail. Preserve that first
        // error before a failed activation repeats this original solver loop.
        float secondFraction=collisionParticipantImpulseFraction(second,normalImpulse,secondLever);
        if (system->result!=COLLISION_SYSTEM_OK) return;
        float firstFraction=collisionParticipantImpulseFraction(first,negativeImpulse,firstLever);
        if (system->result!=COLLISION_SYSTEM_OK) return;
        if (firstFraction==-57575.0f || secondFraction==-57575.0f) return;
        int fractionEndsContact=1;
        if (firstFraction<0.0f) firstFraction=-firstFraction;
        else if (secondFraction<0.0f) secondFraction=-secondFraction;
        else fractionEndsContact=0;
        const float fraction=minimumFinite(firstFraction,secondFraction);
        magnitude=fraction*magnitude;
        const float firstFriction=collisionParticipantFriction(system,first);
        const float secondFriction=collisionParticipantFriction(system,second);
        float friction=maximumFinite(firstFriction,secondFriction);
        CollisionParticipant* wallCar=NULL;
        if (firstInverseMass==0.0f && secondFriction==0.3f && second->shape->kind==0
                && (double)collisionResponseShapeFrame(second->shape)[10]>0.7) wallCar=second;
        else if (secondInverseMass==0.0f && firstFriction==0.3f && first->shape->kind==0
                && (double)collisionResponseShapeFrame(first->shape)[10]>0.7) wallCar=first;
        if (wallCar) friction=0.1f;
        const float tangentSpeed=sqrtf(collisionResponseDot3(tangent,tangent));
        float frictionMagnitude=magnitude*friction;
        if (tangentSpeed<system->tuning->tangentSpeedScale)
            frictionMagnitude=frictionMagnitude*finiteRatio(tangentSpeed,system->tuning->tangentSpeedScale);
        // Zero tangential speed has zero friction impulse. Dividing 0/0
        // would manufacture a console saturation value, not a direction.
        const float tangentScale=tangentSpeed==0.0f ? 0.0f : finiteRatio(-frictionMagnitude,tangentSpeed);
        float secondImpulse[4]={},firstImpulse[4]={};
        for (int lane=0;lane<3;++lane) {
            const float tangentialImpulse=tangent[lane]*tangentScale;
            const float normalPart=normal[lane]*magnitude;
            secondImpulse[lane]=normalPart+tangentialImpulse;
            firstImpulse[lane]=0.0f-secondImpulse[lane];
        }
        if (wallCar) {
            const float* velocity=wallCar->shape->bodyMode ? wallCar->shape->body->linearVelocity : wallCar->shape->vehicle.velocity;
            const float speed=sqrtf(collisionResponseDot3(velocity,velocity));
            if (30.0f<speed) {
                float scale=-0.025f;
                if (speed*scale<-0.4f) scale=finiteRatio(-0.4f,speed);
                float correction[4]={};
                for (int lane=0;lane<3;++lane) correction[lane]=velocity[lane]*scale;
                correction[2]=0.0f;
                float* impulse=firstInverseMass==0.0f ? secondImpulse : firstImpulse;
                float* lever=firstInverseMass==0.0f ? secondLever : firstLever;
                const float impulseSpeed=sqrtf(collisionResponseDot3(impulse,impulse));
                for (int lane=0;lane<3;++lane) impulse[lane]=impulse[lane]+correction[lane];
                if (1.0f<speed+impulseSpeed) {
                    const float scaledSpeed=speed*0.5714285969734192f;
                    const float leverScale=finiteRatio(impulseSpeed,scaledSpeed+impulseSpeed);
                    for (int lane=0;lane<3;++lane) lever[lane]=lever[lane]*leverScale;
                }
            }
        }
        collisionParticipantApplyImpulse(system,second,secondImpulse,secondLever);
        if (system->result!=COLLISION_SYSTEM_OK) return;
        collisionParticipantApplyImpulse(system,first,firstImpulse,firstLever);
        if (system->result!=COLLISION_SYSTEM_OK) return;
        collisionImpactAccumulate(system,contact->pair,point,secondImpulse,tangentSpeed);
        if (system->result!=COLLISION_SYSTEM_OK) return;
        if (fraction==1.0f || fractionEndsContact) return;
    }
}

void collisionContactResolve(CollisionSystem* system, CollisionQueuedContact* contact)
{
    CollisionParticipant* first=contact->pair->first;
    CollisionParticipant* second=contact->pair->second;
    collisionParticipantNotify(first,second);
    if (system->result!=COLLISION_SYSTEM_OK) return;
    collisionParticipantNotify(second,first);
    if (system->result!=COLLISION_SYSTEM_OK) return;
    if (first->shape->active) collisionParticipantApplyStep(first,contact->view.time);
    if (second->shape->active) collisionParticipantApplyStep(second,contact->view.time);
    float firstPosition[4],secondPosition[4];
    for (int lane=0;lane<3;++lane) {
        firstPosition[lane]=collisionResponseShapePosition(first->shape)[lane]-(collisionParticipantMovable(first) ? contact->displacement[lane] : 0.0f);
        secondPosition[lane]=collisionResponseShapePosition(second->shape)[lane]+(collisionParticipantMovable(second) ? contact->displacement[lane] : 0.0f);
    }
    firstPosition[3]=secondPosition[3]=1.0f;
    if (first->shape->active) collisionShapeStorePosition(first->shape,firstPosition);
    if (second->shape->active) collisionShapeStorePosition(second->shape,secondPosition);
    collisionParticipantPrepare(first);
    collisionParticipantPrepare(second);
    collisionContactSolve(system,contact);
    if (system->result!=COLLISION_SYSTEM_OK) return;
    collisionParticipantPrepare(first);
    collisionParticipantPrepare(second);
    collisionParticipantCapture(system,first,contact->view.time);
    collisionParticipantCapture(system,second,contact->view.time);
}

void collisionContactPlaneNormal(float normal[4], const CollisionShape* planeShape,
                                  const CollisionShape* movingShape, const CollisionContactTuning* tuning)
{
    contactPlaneNormalAtPosition(normal,planeShape,collisionResponseShapePosition(movingShape),tuning);
}

float collisionContactProjection(const CollisionContact* contact, int32_t* valid,
                                 const CollisionContactTuning* tuning)
{
    const CollisionShape* plane;
    const CollisionShape* moving;
    if(contact->first->kind==3) {
        if(!contact->second->active) { *valid=0;return 0.0f; }
        plane=contact->first;moving=contact->second;
    } else if(contact->second->kind==3 && contact->first->active) {
        plane=contact->second;moving=contact->first;
    } else { *valid=0;return 0.0f; }
    float normal[4];
    collisionContactPlaneNormal(normal,plane,moving,tuning);
    *valid=1;
    return collisionResponseDot3(collisionResponseShapePosition(moving),normal);
}

int32_t collisionContactOrder(const CollisionContact* first, const CollisionContact* second,
                              const CollisionContactTuning* tuning)
{
    const float difference=first->time-second->time;
    if(fabsf(difference)<tuning->orderTolerance) {
        int32_t firstValid,secondValid;
        const float firstProjection=collisionContactProjection(first,&firstValid,tuning);
        const float secondProjection=collisionContactProjection(second,&secondValid,tuning);
        if(!firstValid || !secondValid) return 1;
        return !(secondProjection<firstProjection);
    }
    return !(second->time<first->time);
}

const CollisionContactTuning collisionContactDefaultTuning = {-5.000100135803223f, 0.0f};

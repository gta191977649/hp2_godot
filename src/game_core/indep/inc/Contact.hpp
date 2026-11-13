#ifndef NFSHP2_INDEP_CONTACT_HPP
#define NFSHP2_INDEP_CONTACT_HPP

#include "indep/inc/CollisionBody.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CollisionSystem CollisionSystem;
typedef struct CollisionQueuedContact CollisionQueuedContact;

typedef struct CollisionContact {
    float time;
    CollisionShape* first;
    CollisionShape* second;
} CollisionContact;

typedef struct CollisionContactTuning {
    float orderTolerance;
    float planeBias;
} CollisionContactTuning;
extern const CollisionContactTuning collisionContactDefaultTuning;

void collisionContactPlaneNormal(float normal[4], const CollisionShape* planeShape,
                                  const CollisionShape* movingShape, const CollisionContactTuning* tuning);
float collisionContactProjection(const CollisionContact* contact, int32_t* valid,
                                 const CollisionContactTuning* tuning);
int32_t collisionContactOrder(const CollisionContact* first, const CollisionContact* second,
                              const CollisionContactTuning* tuning);

/* Physical velocity is linear + angular cross lever in XYZ. The native
 * direction slot W is zero; console register padding is not velocity.
 * output may alias any whole input vector. */
void collisionPointVelocity(float output[4],const float linear[4],const float angular[4],const float lever[4]);

/* Uses the complete affine inertia transform, including its fourth column.
 * The return value uses XYZ only. Inputs are finite native vectors/matrices. */
float collisionAngularInverseMass(const float inverseInertia[16],const float lever[4],const float normal[4]);

float collisionContactFindTime(CollisionSystem* system, CollisionQueuedContact* contact);
void collisionContactSeparate(CollisionSystem* system, CollisionQueuedContact* contact, float rewind);
void collisionContactFrame(const CollisionSystem* system, const CollisionQueuedContact* contact,
                            float point[4], float normal[4]);
void collisionContactProjectPoint(const CollisionQueuedContact* contact, float point[4]);
void collisionContactSolve(CollisionSystem* system, CollisionQueuedContact* contact);
void collisionContactResolve(CollisionSystem* system, CollisionQueuedContact* contact);

static inline void collisionResponseCross3(float output[4],const float first[4],const float second[4])
{
    float result[4];
    for(int lane=0;lane<3;++lane) {
        const int next=(lane+1)%3,last=(lane+2)%3;
        const float positive=first[next]*second[last],negative=second[next]*first[last];
        result[lane]=positive-negative;
    }
    result[3]=0.0f;
    memcpy(output,result,sizeof(result));
}

static inline float collisionResponseDot3(const float first[4], const float second[4])
{
    const float x=first[0]*second[0], y=first[1]*second[1], z=first[2]*second[2];
    return (x+y)+z;
}

static inline const float* collisionResponseShapePosition(const CollisionShape* shape)
{
    return shape->bodyMode ? shape->body->motion.position : shape->vehicle.position;
}

static inline const float* collisionResponseShapeFrame(const CollisionShape* shape)
{
    return shape->bodyMode ? shape->body->motion.orientation : shape->vehicle.orientation;
}

static inline void collisionResponseInterpolatePose(const CollisionShape* shape, float position[4], float frame[16], float rewind)
{
    if (shape->active) collisionShapeInterpolatePose(shape,position,frame,rewind);
    else {
        memset(position,0,4*sizeof(float));
        position[3]=1.0f;
        memset(frame,0,16*sizeof(float));
        frame[0]=frame[5]=frame[10]=frame[15]=1.0f;
    }
}

#ifdef __cplusplus
}
#endif

#endif

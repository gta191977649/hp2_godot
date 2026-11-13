#ifndef NFSHP2_INDEP_INC_COLLISIONBODY_HPP
#define NFSHP2_INDEP_INC_COLLISIONBODY_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/Polyhedra.hpp"
#include "indep/inc/RigidBody.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CollisionShape CollisionShape;
typedef struct CollisionSpace CollisionSpace;
struct ReplaySnapshot;
/* The collision owner is shared with the solver. Flags stay in the actual
 * shape; the contact timestamp belongs to this owner, not a driver mirror. */
typedef struct CarCollisionPolicy {
    int32_t collisionDisabled;
    int32_t secondaryCollisionDisabled;
} CarCollisionPolicy;

typedef struct CarCollisionState {
    CollisionShape *shape;
    /* PAL CarCollisionBody+10C: opposing yaw-sensitive contact. */
    int32_t lastCollisionTimeTicks;
    const CarCollisionPolicy *policy;
    /* PAL+104/+108 remain independent live timestamps, including snapshots. */
    int32_t collisionTicks;
    int32_t suppressedCollisionTicks;
} CarCollisionState;



int32_t collisionShapeOverlapsBounds(const CollisionShape* shape, const float minimum[2], const float maximum[2], float margin);
int32_t collisionShapesOverlap(const CollisionShape* first, const CollisionShape* second, float margin);
CollisionShape* collisionBodyById(const CollisionSpace* space, int16_t id);

/* A vehicle view refers to actual native motion storage. During bodyMode the
 * same queries read the attached RigidBody. No callbacks or synthetic contact
 * provider are involved. Position/velocity have four lanes, frames 16 floats. */
typedef struct CollisionMotionView {
    float* position;
    float* velocity;
    float* orientation;
} CollisionMotionView;
typedef struct CollisionShape CollisionShape;
typedef struct CollisionIdPool {
    int32_t size;
    uint8_t* occupied;
    int32_t nextFree;
} CollisionIdPool;
struct CollisionShape {
    CollisionShape* next;
    CollisionShape* previous;
    int8_t kind, active;
    int16_t id;
    uint8_t pairCount, locks;
    int8_t contactCount, dirty;
    float minimum[2], maximum[2];
    char label[40];
    void* userData;
    CollisionGeometry* geometry;
    const void* resourceData;
    RigidBody* body;
    void* prepareOwner;
    CollisionMotionView vehicle;
    int32_t bodyMode;
    float previousTime;
    float previousFrame[16], previousPosition[4], previousVelocity[4];
    float negativeExtentZ, positiveExtentZ, radialRadius, originRadius;
    float localMinimum[3], localMaximum[3];
};
/* Final PAL StaticCollisionBody.GetVelocity1D9190 returns the constant
 * zero vector319EE0. Native active distinguishes that static body from the
 * original dynamic/car motion owners; a static placement has no velocity
 * pointer to borrow. Missing motion on a dynamic body remains an error. */
static inline const float* collisionShapeVelocity(const CollisionShape* shape)
{
    if (!shape->active) {
        static const float stationary[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        return stationary;
    }
    if (shape->bodyMode) return shape->body ? shape->body->linearVelocity : NULL;
    return shape->vehicle.velocity;
}

enum { COLLISION_ID_COUNT = 206, COLLISION_PAIR_BYTES = (206 * 206 + 7) / 8 };
typedef struct CollisionSpace {
    CollisionShape root;
    float maximumWidth;
    CollisionShape* byId[COLLISION_ID_COUNT];
    uint8_t occupied[(COLLISION_ID_COUNT + 7) / 8];
    uint8_t pairBits[COLLISION_PAIR_BYTES];
    CollisionIdPool ids;
    int32_t shapeCount, peakShapeCount, dynamicCount, peakDynamicCount;
    int32_t carCount,peakCarCount;
} CollisionSpace;

/* Native context setup replaces original static storage initialization and is
 * deliberately not counted as an original method. IDs passed to allocation
 * operations are in range, storage has ceil(size/8) bytes. Graphs are valid;
 * bound updates operate on members of this space's intrusive list. */
void collisionSpaceInitialize(CollisionSpace* space);
void collisionIdMark(CollisionIdPool* pool, int32_t id);
void collisionIdRelease(CollisionIdPool* pool, int32_t id);
int32_t collisionIdAcquire(CollisionIdPool* pool);
uint32_t collisionPairMask(const CollisionSpace* space, const CollisionShape* first, const CollisionShape* second);
CollisionShape* collisionShapeInitialize(CollisionSpace* space, CollisionShape* shape, int32_t id, int8_t kind);
CollisionShape* collisionDynamicShapeInitialize(CollisionSpace* space, CollisionShape* shape, int32_t id,
                                               int8_t kind, const void* resourceData);
int32_t collisionShapeCompare(const CollisionShape* first, const CollisionShape* second);
void collisionSpaceUpdateWidth(CollisionSpace* space, const CollisionShape* shape, float width);
void collisionShapeSetBounds(CollisionSpace* space, CollisionShape* shape, const float minimum[2], const float maximum[2]);
int32_t collisionSpaceOverlaps(const CollisionSpace* space, const float minimum[2], const float maximum[2]);
void collisionShapeLock(CollisionShape* shape);
void collisionShapeUnlock(CollisionShape* shape);
void collisionShapeComputeLocalBounds(CollisionShape* shape);
void collisionShapeUpdateBounds(CollisionSpace* space, CollisionShape* shape);
void collisionShapeCapture(CollisionShape* shape, float timestep);
void collisionShapeInterpolatePose(const CollisionShape* shape, float position[4], float frame[16], float rewind);
void collisionShapeInterpolateVelocity(const CollisionShape* shape, float velocity[4], float rewind);
void collisionShapeApplyStep(CollisionShape* shape, float rewind);
void collisionShapeCopyVelocityPair(const CollisionShape* shape, float linear[4], float angular[4]);
void collisionShapeFindRange(const CollisionSpace* space, const CollisionShape* shape,
                             CollisionShape** before, CollisionShape** after, float margin);
int32_t collisionShapeCrossesPlane(const CollisionShape* shape, const float normal[3], float offset);
float collisionShapeImpulseEnergy(const CollisionShape* shape, const float impulse[4], const float lever[4]);
void collisionShapeStorePosition(CollisionShape* shape, const float position[4]);
/* Original snapshot transfers retain native owners; ID/bounds VerifyData
 * never replaces a live pointer or changes ordinary capture bytes. */
void collisionShapeSnapshot(CollisionShape* shape, struct ReplaySnapshot* snapshot);
void collisionBodiesSnapshotEverything(CollisionSpace* space, struct ReplaySnapshot* snapshot);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <memory>
struct Car;
struct VehicleState;
struct TerrainSurface;
struct GameTime;
struct CollisionSystem;
namespace hp2 {
struct CarDefinition;
class CarCollisionBody final {
public:
    CarCollisionBody(CollisionSystem& system,Car& car,VehicleState& vehicle,TerrainSurface& terrain,
        const CarDefinition& type,CollisionResource* const* resources,std::size_t count,const GameTime& clock);
    ~CarCollisionBody();
    CarCollisionBody(const CarCollisionBody&)=delete;
    CarCollisionBody& operator=(const CarCollisionBody&)=delete;
    CarCollisionState& State() noexcept {return state_;}
    CollisionShape& Shape() noexcept {return shape_;}
private:
    CollisionSystem& system_;
    Car& car_;
    CollisionShape shape_{};
    CarCollisionState state_{};
    std::unique_ptr<CollisionBox> fallback_;
};
}
#endif

#endif

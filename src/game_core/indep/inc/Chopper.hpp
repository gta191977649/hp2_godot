#ifndef NFSHP2_CHOPPER_H
#define NFSHP2_CHOPPER_H

#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/HeliSheet.hpp"
#include "bsound/include/bSample.h"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/TrackRoute.hpp"

#ifdef __cplusplus
namespace hp2 { struct ChopperConstructionBindings; }
extern "C" {
#endif
struct ChopperOwnedState;

typedef struct ChopperTarget {
    Car* car;
    CarMovement* movement;
    CarDriver* driver;
} ChopperTarget;

typedef struct ChopperAttackProfile {
    int32_t munitionKind;
    float leadSeconds,launchSpeed,lateralLaunchSpeed;
    int32_t projectilesPerDrop,dropLimit;
    float spread,dropDelay;
} ChopperAttackProfile;

typedef struct ChopperMunitionRegistry ChopperMunitionRegistry;
typedef struct ChopperPresentation ChopperPresentation;
typedef struct ChopperCommands ChopperCommands;
typedef struct ChopperDownwash ChopperDownwash;
#ifdef __cplusplus
class Sound3d;
#else
typedef struct Sound3d Sound3d;
#endif

typedef struct ChopperEnvironment {
    World* world;
    AerialNavigation* navigation;
    const CarRecoveryPointRegistry* road;
    TrackRoute* route;
    ChopperTarget* targets;
    size_t targetCount;
    const ChopperAttackProfile* profiles;
    size_t profileCount;
    ChopperMunitionRegistry* munitions;
    WorldParticleSystem* particles;
    ChopperPresentation* presentation;
} ChopperEnvironment;

/* One live owner contains navigation, movement and presentation pose state.
 * Targets and their actual controllers/movers remain borrowed owners. */
struct Chopper {
    ChopperEnvironment environment;
    TerrainSurface terrain;
    ChopperTarget* target;
    int32_t phase,movementMode;
    float planningCooldown;
    CarRecoverySelection current,departure,previousTarget;
    TrackRouteLocation routeLocation;
    float targetDistance;
    int32_t attackProfile,drops;
    float dropDelay;
    int32_t launchPattern;
    float attackSeconds,blockedSeconds;
    int32_t totalDrops;
    float position[4],frame[16];
    float horizontalSpeed,forwardAcceleration,yawRate;
    float roll,desiredRoll,yaw,pitch,desiredPitch,turnRate,verticalVelocity,groundHeight;
    int32_t downwashEnabled,forceFirstPlayerTarget,visible;
    WorldServicesResult result;
    ChopperCommands* commands;
    ChopperDownwash* downwash;
    /* Source+0: one actual child produced by CreateChopperSound. It borrows
     * its shared bank sample; that sample is not owned by this pointer. */
    Sound3d* sound;
    struct ChopperOwnedState* ownedState
#ifdef __cplusplus
        = nullptr
#endif
        ;
#ifdef __cplusplus
    Chopper() noexcept = default;
    explicit Chopper(const hp2::ChopperConstructionBindings& bindings);
    ~Chopper();
    Chopper(const Chopper&) = delete;
    Chopper& operator=(const Chopper&) = delete;
#endif
};

const ChopperAttackProfile* chopperAttackProfiles(size_t* count);
void chopperRotate2(float output[2],const float input[2],float degrees);
float chopperWrapDegrees(float degrees);
void chopperApproach(float target,float* value,float maximumStep);
void chopperRotateYaw(Chopper* chopper,float degrees);
void chopperToLocal(const Chopper* chopper,float output[2],const float input[2]);
void chopperDestinationToLocal(const Chopper* chopper,float point[2]);
float chopperMinimumAcceleration(const Chopper* chopper);
float chopperResponseScale(float value);
void chopperChooseAcceleration(Chopper* chopper,const float desired[3]);
void chopperChooseYawRate(Chopper* chopper,const float desired[2],float error);
void chopperChooseTurnRate(Chopper* chopper,float desired,float scale);
int32_t chopperTryTurnInPlace(Chopper* chopper,const float desired[2]);
int32_t chopperNavigationProbe(Chopper* chopper,float forward,float lateral);
void chopperAvoidCorridorEdges(Chopper* chopper);
void chopperBuildFrame(Chopper* chopper);
int32_t chopperRefreshGround(Chopper* chopper);
int32_t chopperTargetBehind(const Chopper* chopper);
int32_t chopperShiftTarget(Chopper* chopper,ChopperTarget* target);
int32_t chopperStartAttack(Chopper* chopper,int32_t profile);
void chopperNotifyHorn(Chopper* chopper);
void chopperChooseAttackDeparture(Chopper* chopper,float distance);
void chopperAdvanceAttackDeparture(Chopper* chopper);
void chopperRefreshAttackDeparture(Chopper* chopper);
int32_t chopperMove(Chopper* chopper,float elapsed);
int32_t chopperUpdateSound(Chopper* chopper);
int32_t chopperMuteSound(Chopper* chopper,int32_t muted);
void chopperPlace(Chopper* chopper,const float point[2],float height);
void chopperHide(Chopper* chopper);
void chopperDoSnapshot(Chopper* chopper,struct ReplaySnapshot* snapshot);
int32_t chopperEnterCorridor(Chopper* chopper,float goal[3],int32_t corridorFound,
    uint32_t flags,float corridorHeight);
int32_t chopperLeaveCorridor(Chopper* chopper,float goal[3]);
void chopperFollowGoal(Chopper* chopper,float goal[3],int32_t corridorFound,uint32_t flags);

void chopperResetCurrentRoad(Chopper* chopper);
void chopperRefreshCurrentRoad(Chopper* chopper);
void chopperSetGoalSpeed(const Chopper* chopper, float goal[3]);
int32_t chopperBacktrackToBranch(const Chopper* chopper, CarRecoverySelection* selection, int32_t forward);
int32_t chopperFindNearbyAlternative(const Chopper* chopper, CarRecoverySelection* output,
                                    const CarRecoverySelection* current, int32_t forward,
                                    float maximumDistance);
void chopperSetCurrentGoal(const Chopper* chopper, float goal[3]);
float chopperPlanningLookahead(const Chopper* chopper);
void chopperPlanPathGoal(Chopper* chopper, float goal[3], const CarRecoverySelection* destination,
                          int32_t forward);
void chopperConstrainGoalToCorridor(const Chopper* chopper, float goal[3],
                                    const CarRecoverySelection* road);
void chopperConstrainCurrentGoal(const Chopper* chopper, float goal[3]);
void chopperChooseRepositionGoal(Chopper* chopper, float goal[3],
                                 const CarRecoverySelection* first);
int32_t chopperKeepGoalClearance(Chopper* chopper, float goal[3]);
int32_t chopperIsGroupBehind(const Chopper* chopper, const CarRecoveryGroup* group);


// Native declarations of this same Chopper owner; no additional state owner.

/* The sound controller's active flag is independent of whether its retained
 * voice is currently playing. Position/frame/visibility remain on Chopper. */
struct ChopperPresentation {
    uint32_t modelKey;
    WorldAudioRequestQueue* audio;
    WorldAudioInstance* engineVoice;
    const int32_t* audioEnabled;
    int32_t audioActive;
    float volume;
    float spatialParameter;
    uint16_t panningAngle;
};



void chopperAttackGoal(Chopper* chopper, float goal[3]);
int32_t chopperPlanGoal(Chopper* chopper, float goal[3]);



struct ChopperCommands {
    int32_t toggleTarget;
    int32_t dropSmokeAtFirstPlayer;
};

/* These persistent clones borrow the same effect pool/resources as the
 * transient detonation list. They have a separate actual owning list. */
struct ChopperDownwash {
    ChopperMunitionEffects* effects;
    ChopperEffectInstance* first;
    ChopperEffectInstance* last;
};

WorldServicesResult chopperDownwashInitialize(ChopperDownwash* downwash, ChopperMunitionEffects* effects);
void chopperDownwashDispose(ChopperDownwash* downwash);
void chopperUpdateDownwash(Chopper* chopper, float elapsed);
void chopperProcessCommands(Chopper* chopper);
void chopperUpdateGuidance(Chopper* chopper);



/* The concrete munition registry owns the created physical actors. They
 * advance through the collision participant pass, not through Chopper. */
void chopperDropMunitions(Chopper* chopper);
void chopperBombLaunchVelocity(const Chopper* chopper, float output[4], float spread);
void chopperRocketLaunchVelocity(const Chopper* chopper, float output[4], const float origin[4]);
void chopperCreateMunition(Chopper* chopper, const float frame[16], float spread);


#ifdef __cplusplus
}
struct WorldObjectRegistry;
struct ChopperMunitionEffects;
struct CarSoundBindings;
struct RaceParameters;
namespace hp2 {
struct ChopperConstructionBindings {
    ChopperEnvironment environment{};
    TerrainTopology* terrain{};
    WorldObjectRegistry* objects{};
    ChopperMunitionEffects* effects{};
    const CarSoundBindings* sound{};
    const RaceParameters* parameters{};
};
void ChopperSynchronizeTargets(Chopper& chopper);
}
void chopperClearVariables(Chopper* chopper);
#endif
#endif

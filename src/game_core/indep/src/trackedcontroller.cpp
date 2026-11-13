#include "TrackedController.hpp"
#include "bware/inc/bMath.hpp"

CarDriverResult driverRouteInitialize(DriverRoute* driver, CarMovement* movement,
                                      CarDriverKind kind)
{
    if (!driver || !movement || !movement->bindings.state ||
            !movement->bindings.recoveryPoints ||
            (kind != CAR_DRIVER_REAL && kind != CAR_DRIVER_COP))
        return CAR_DRIVER_INVALID_BINDING;
    driver->movement = movement;
    driver->recovery.profile = (int32_t)kind;
    driver->recovery.recoveryPose = NULL;
    driver->recovery.recoveryDelay = 0.0f;
    driver->acceleration = 0.0f;
    driver->recovery.previousSpeed = 0.0f;
    driver->result = CAR_DRIVER_OK;
    movement->bindings.driver = &driver->recovery;
    carMovementRefreshRecoveryPose(movement);
    if (movement->result != CAR_MOVEMENT_OK)
        driver->result = CAR_DRIVER_MOVEMENT_FAILURE;
    return driver->result;
}

void driverRouteAdvance(DriverRoute* driver, float elapsed)
{
    const float remaining = driver->recovery.recoveryDelay - elapsed;
    driver->recovery.recoveryDelay = remaining;
    if (0.0f < remaining) return;
    const float speedChange = driver->movement->bindings.state->speed - driver->recovery.previousSpeed;
    const float duration = 0.10000000149011612f - remaining;
    driver->acceleration = finiteRatio(speedChange, duration);
    carMovementRefreshRecoveryPose(driver->movement);
    if (driver->movement->result != CAR_MOVEMENT_OK)
        driver->result = CAR_DRIVER_MOVEMENT_FAILURE;
}

void driverRouteSetPose(DriverRoute* driver, const CarRecoveryPose* pose)
{
    driver->recovery.recoveryDelay = 0.10000000149011612f;
    driver->recovery.recoveryPose = pose;
    driver->recovery.previousSpeed = driver->movement->bindings.state->speed;
}

const CarRecoveryGroup* driverRouteCurrentGroup(const DriverRoute* driver)
{
    const CarRecoveryPose* pose = driver->recovery.recoveryPose;
    if (!pose) return NULL;
    const CarRecoveryPointRegistry* registry = driver->movement->bindings.recoveryPoints;
    for (size_t index = 0; index < registry->groupCount; ++index)
        if (registry->groups[index].id == pose->groupId) return &registry->groups[index];
    return NULL;
}

float driverRouteLateralExcess(const DriverRoute* driver)
{
    const CarRecoveryPose* pose = driver->recovery.recoveryPose;
    if (!pose) return 0.0f;
    const float* position = driver->movement->bindings.state->position;
    const float x = position[0] - pose->position[0];
    const float y = position[1] - pose->position[1];
    const float first = x * pose->direction[1];
    const float second = y * pose->direction[0];
    const float side = first - second;
    return 0.0f < side ? side - pose->halfWidths[1] : -(side + pose->halfWidths[0]);
}

#include "indep/inc/Replay.hpp"
#include "indep/inc/CarController.hpp"
#include "indep/inc/TrackRoute.hpp"

// Full PAL23A0F0..23A174. The base call serializes the actual input owner,
// followed by this controller's sole authored-road pose and speed history.
void driverRouteSnapshot(DriverRoute* driver,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"TrackedController");
    carControllerSnapshot(driver->movement,snapshot);
    carRecoverySnapshotPose(driver->movement->bindings.recoveryPoints,snapshot,&driver->recovery.recoveryPose);
    // PAL23A13C..23A160 visits +0x10, +0x14, +0x18 in that order.
    // Advance23A088 stores acceleration at +0x14; SetPose23A178 stores
    // previousSpeed at +0x18. Host member layout must not reorder the stream.
    replaySnapshotMoveFloat(snapshot,&driver->recovery.recoveryDelay);
    replaySnapshotMoveFloat(snapshot,&driver->acceleration);
    replaySnapshotMoveFloat(snapshot,&driver->recovery.previousSpeed);
    replaySnapshotEndSection(snapshot);
}

// Complete FindWhereIAm, Beta233F10..234040 / PAL23A1C8..23A2F8.
// This tracked controller owns the recovery pose, refresh timer and prior speed.
void carMovementRefreshRecoveryPose(CarMovement* movement)
{
    CarMovementDriver* driver = movement->bindings.driver;
    const CarRecoveryPointRegistry* registry = movement->bindings.recoveryPoints;
    if (!driver || driver->profile == 0) return;
    if (!registry) { movement->result = CAR_MOVEMENT_MISSING_RECOVERY_DATA; return; }
    const float* position = movement->bindings.state->position;
    if (driver->recoveryPose) {
        const CarRecoveryGroup* group = NULL;
        for (size_t index = 0; index < registry->groupCount; ++index)
            if (registry->groups[index].id == driver->recoveryPose->groupId) {
                group = &registry->groups[index];
                break;
            }
        if (!group) { movement->result = CAR_MOVEMENT_INVALID_BINDING; return; }
        driver->recoveryPose = carRecoveryFindInGroup(registry, group, position, 24.0f,
                                                      movement->routeLocation.distance, 34u);
        const CarRecoveryPose* pose = driver->recoveryPose;
        if (pose) {
            const float x = position[0] - pose->position[0];
            const float y = position[1] - pose->position[1];
            const float first = x * pose->direction[1];
            const float second = y * pose->direction[0];
            const float side = first - second;
            const float outside = 0.0f < side ? side - pose->halfWidths[1] : -(side + pose->halfWidths[0]);
            if (5.0f < outside) driver->recoveryPose = NULL;
        }
    }
    if (!driver->recoveryPose) {
        CarRecoverySelection selection;
        if (carRecoverySelectPose(&selection, registry, position, 0, 34u, movement->routeLocation.distance))
            driver->recoveryPose = selection.pose;
    }
    driver->recoveryDelay = 0.10000000149011612f;
    driver->previousSpeed = movement->bindings.state->speed;
}

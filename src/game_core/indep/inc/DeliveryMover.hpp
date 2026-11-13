#ifndef NFSHP2_DELIVERYMOVER_HPP
#define NFSHP2_DELIVERYMOVER_HPP

// Native declarations use the PC runtime layout.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CarMovement;
typedef struct DeliveryMover {
    struct CarMovement* movement;
    float startOrientation[4];
    float targetOrientation[4];
    float startPosition[4];
    float positionDelta[4];
    float startVelocity[4];
    float velocityDelta[4];
    float duration;
    float elapsed;
} DeliveryMover;

void deliveryMoverInitialize(DeliveryMover* mover, struct CarMovement* movement);
/* The target must be assigned before this mover is advanced. */
void deliveryMoverSetTarget(DeliveryMover* mover, const float position[2], uint16_t heading,
                            float speed, float duration);
float deliveryMoverMove(DeliveryMover* mover, float elapsed);

#ifdef __cplusplus
}
#endif
#endif

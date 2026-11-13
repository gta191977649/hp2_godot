#ifndef NFSHP2_SOUND3D_HPP
#define NFSHP2_SOUND3D_HPP

#include <stdint.h>

struct CarFramePlayerRegistry;
struct CarCinematicServices;
struct GameTime;

#ifdef __cplusplus
extern "C" {
#endif

float physicsMaximum(float first, float second);
float physicsMinimum(float first, float second);
float physicsClamp(float value, float lower, float upper);
float physicsClampDelta(float center, float value, float delta);

/* One spatial-sound component, embedded by the actual vehicle sound owner.
 * Backend voices and listener transforms belong to their existing owners. */
typedef struct Sound3dState {
    int32_t active;
    int32_t dopplerActive;
    float volume;
    float pitch;
    uint16_t azimuth;
    float relativeVelocity;
    float distance;
} Sound3dState;

typedef struct Sound3dEnvelope {
    float volume;
    float maximumDistance;
    float inverseDistance;
} Sound3dEnvelope;

typedef struct Sound3dBindings {
    const struct CarFramePlayerRegistry* players;
    const struct CarCinematicServices* cameras;
    const struct GameTime* clock;
    const float* speedOfSound;
    const uint16_t* multiplayerAzimuth;
} Sound3dBindings;

typedef enum Sound3dResult {
    SOUND3D_OK,
    SOUND3D_INVALID_BINDING
} Sound3dResult;

void sound3dInitialize(Sound3dState* sound);
Sound3dResult sound3dDistanceToClosestCamera(const Sound3dBindings* bindings,
    const float position[4], float* distance);
Sound3dResult sound3dUpdateRelativeMotion(Sound3dState* sound,
    const Sound3dBindings* bindings, const float position[4]);
Sound3dResult sound3dUpdate(Sound3dState* sound, const Sound3dBindings* bindings,
    const float position[4], const Sound3dEnvelope* envelope);
Sound3dResult sound3dComputeAzimuth(const Sound3dBindings* bindings,
    const float position[4], uint16_t* azimuth);
Sound3dResult sound3dComputeVolume(const Sound3dBindings* bindings,
    const float position[4], float maximumDistance, float* volume);
float sound3dAzimuthToPan(uint16_t azimuth);
uint16_t sound3dPanToAzimuth(float pan);

#ifdef __cplusplus
}

#include "bsound/include/bSample.h"

/* Native virtual methods are the concrete game sound implementations.
 * No application callback supplies a missing engine or tire algorithm. */
class Sound3d {
public:
    Sound3d();
    virtual ~Sound3d();
    WorldAudioResult Activate(int32_t active);
    virtual WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) = 0;
    virtual WorldAudioResult Pause(int32_t paused) = 0;
    Sound3dState spatial;

protected:
    virtual WorldAudioResult ActivateSound() = 0;
    virtual WorldAudioResult DeactivateSound() = 0;
};
#endif
#endif

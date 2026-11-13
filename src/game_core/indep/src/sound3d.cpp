#include "Sound3d.hpp"
#include "Sound3dSample.hpp"

#include "CameraMover.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Timer.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"

#include <cmath>

namespace {

/* Native binding of the actual GetActiveCameras consumer. Its source cache
 * contains these same view pointers; it owns no separate camera transform. */
const CarCinematicView* activeView(const Sound3dBindings* bindings, int32_t index)
{
    if (!bindings->players->players[index]) return nullptr;
    return bindings->cameras->screens[index + 1].view;
}

bool validListeners(const Sound3dBindings* bindings)
{
    return bindings && bindings->players && bindings->cameras &&
        bindings->players->count >= 0 && bindings->players->count <= 2 &&
        bindings->cameras->screens[1].view;
}

float squaredDistance(const float first[4], const float second[4])
{
    const float x = first[0] - second[0];
    const float y = first[1] - second[1];
    const float z = first[2] - second[2];
    return (x * x + y * y) + z * z;
}

} // namespace

void sound3dInitialize(Sound3dState* sound)
{
    sound->active = 0;
    sound->dopplerActive = 1;
    sound->pitch = 1.0f;
    sound->volume = 1.0f;
    sound->azimuth = 0;
    sound->relativeVelocity = 0.0f;
    sound->distance = 0.0f;
}

Sound3dResult sound3dDistanceToClosestCamera(const Sound3dBindings* bindings,
    const float position[4], float* distance)
{
    if (!validListeners(bindings) || !position || !distance)
        return SOUND3D_INVALID_BINDING;

    float closest = 8999999488.0f;
    for (int32_t index = 0; index != 2; ++index) {
        const CarCinematicView* view = activeView(bindings, index);
        if (bindings->players->players[index] && !view)
            return SOUND3D_INVALID_BINDING;
        if (view) {
            const float candidate = std::sqrt(squaredDistance(position, view->current.eye));
            if (candidate < closest) closest = candidate;
        }
    }
    if (closest == 8999999488.0f) {
        const CarCinematicView* view = bindings->cameras->screens[1].view;
        closest = std::sqrt(squaredDistance(position, view->current.eye));
    }
    *distance = closest;
    return SOUND3D_OK;
}

Sound3dResult sound3dUpdateRelativeMotion(Sound3dState* sound,
    const Sound3dBindings* bindings, const float position[4])
{
    if (!sound || !bindings || !bindings->clock) return SOUND3D_INVALID_BINDING;
    float distance;
    const Sound3dResult result = sound3dDistanceToClosestCamera(bindings, position, &distance);
    if (result != SOUND3D_OK) return result;
    const float velocity = finiteRatio(distance - sound->distance, bindings->clock->worldTimeElapsed);
    if (velocity < 80.0f && -80.0f < velocity) sound->relativeVelocity = velocity;
    sound->distance = distance;
    return SOUND3D_OK;
}

Sound3dResult sound3dUpdate(Sound3dState* sound, const Sound3dBindings* bindings,
    const float position[4], const Sound3dEnvelope* envelope)
{
    if (!sound || !bindings || !bindings->speedOfSound) return SOUND3D_INVALID_BINDING;
    const Sound3dResult result = sound3dUpdateRelativeMotion(sound, bindings, position);
    if (result != SOUND3D_OK) return result;
    if (sound->dopplerActive) {
        sound->pitch = 1.0f - finiteRatio(sound->relativeVelocity, *bindings->speedOfSound);
    }
    if (envelope) {
        float volume;
        if (envelope->maximumDistance < sound->distance) {
            volume = 0.0f;
        } else {
            const float doubledDistance = sound->distance + sound->distance;
            const float coordinate = sound->distance * envelope->inverseDistance;
            const float doubledCoordinate = doubledDistance * envelope->inverseDistance;
            const float square = coordinate * coordinate;
            const float curve = std::sqrt(std::fabs(doubledCoordinate - square));
            volume = envelope->volume * (1.0f - curve);
        }
        sound->volume = volume;
        if (1.0f < volume) sound->volume = 1.0f;
    } else {
        sound->volume = 1.0f;
    }
    return sound3dComputeAzimuth(bindings, position, &sound->azimuth);
}

Sound3dResult sound3dComputeAzimuth(const Sound3dBindings* bindings,
    const float position[4], uint16_t* azimuth)
{
    if (!validListeners(bindings) || !bindings->multiplayerAzimuth || !position || !azimuth)
        return SOUND3D_INVALID_BINDING;

    if (bindings->players->count >= 2) {
        float closest = 8999999488.0f;
        int32_t selected = -1;
        for (int32_t index = 0; index < bindings->players->count; ++index) {
            const CarCinematicView* view = activeView(bindings, index);
            if (!view) return SOUND3D_INVALID_BINDING;
            const float x = view->current.eye[0] - position[0];
            const float y = view->current.eye[1] - position[1];
            const float distance = x * x + y * y;
            if (distance < closest) {
                closest = distance;
                selected = index;
            }
        }
        const uint16_t offset = *bindings->multiplayerAzimuth;
        *azimuth = selected == 0 ? static_cast<uint16_t>(0u - offset) : offset;
        return SOUND3D_OK;
    }

    const CarCinematicViewFrame& view = bindings->cameras->screens[1].view->current;
    float y = position[1] - view.eye[1];
    float x = position[0] - view.eye[0];
    const float squaredLength = x * x + y * y;
    const float length = std::sqrt(squaredLength);
    x = finiteRatio(x, length);
    y = finiteRatio(y, length);
    const float first = x * view.forward[0];
    const float second = y * view.forward[1];
    const uint16_t direction = vectorArcSineAngle(first + second);
    uint16_t angle = static_cast<uint16_t>(16384u - direction);
    const float oppositeY = -y;
    const float crossFirst = x * view.forward[1];
    const float crossSecond = oppositeY * view.forward[0];
    const float cross = crossSecond + crossFirst;
    if (cross < 0.0f) angle = static_cast<uint16_t>(0u - angle);
    *azimuth = angle;
    return SOUND3D_OK;
}

Sound3dResult sound3dComputeVolume(const Sound3dBindings* bindings,
    const float position[4], float maximumDistance, float* volume)
{
    if (!volume) return SOUND3D_INVALID_BINDING;
    float distance;
    const Sound3dResult result = sound3dDistanceToClosestCamera(bindings, position, &distance);
    if (result != SOUND3D_OK) return result;
    const float inverse = finiteRatio(1.0f, maximumDistance);
    if (maximumDistance < distance) {
        *volume = 0.0f;
    } else {
        const float coordinate = distance * inverse;
        const float doubledDistance = distance + distance;
        const float square = coordinate * coordinate;
        const float curve = std::sqrt(std::fabs(doubledDistance * inverse - square));
        *volume = 1.0f - curve;
    }
    return SOUND3D_OK;
}

float sound3dAzimuthToPan(uint16_t azimuth)
{
    const uint32_t angle = azimuth;
    const uint32_t remainder = angle % 16383u;
    float pan = static_cast<float>(remainder) * 0.00006103515625f;
    if (angle >= 16383u) {
        if (angle - 16383u < 16385u) pan = 1.0f - pan;
        else if (static_cast<uint16_t>(angle - 32768u) < 16385u) pan = -pan;
        else pan = pan - 1.0f;
    }
    return pan * 0.5f;
}

uint16_t sound3dPanToAzimuth(float pan)
{
    if (pan < 0.0f) {
        const float coordinate = (pan + 0.5f) + (pan + 0.5f);
        const float scaled = coordinate * 16383.0f;
        return static_cast<uint16_t>(convertFloatToInt32(scaled + 49152.0f));
    }
    if (0.0f < pan) {
        const float doubled = pan + pan;
        return static_cast<uint16_t>(convertFloatToInt32(doubled * 16383.0f));
    }
    return 0;
}

Sound3d::Sound3d()
{
    sound3dInitialize(&spatial);
}

Sound3d::~Sound3d() = default;

WorldAudioResult Sound3d::Activate(int32_t active)
{
    WorldAudioResult result = WORLD_AUDIO_OK;
    if (!active) {
        // PAL bSample::Stop274740..274778 synchronously emits event8.
        // Its positional owner can delete this object. The callback and
        // destructor do not read active, so commit this store before that
        // one concrete deletion path; preserve ordering for other sounds.
        const auto* positional = dynamic_cast<const Sound3dSample*>(this);
        if (positional && positional->StopCanDeleteOwner()) {
            spatial.active = active;
            return DeactivateSound();
        }
        result = DeactivateSound();
    }
    else if (!spatial.active) result = ActivateSound();
    spatial.active = active;
    return result;
}

#include "bsound/include/bSample.h"
#include "bware/inc/bMath.hpp"

#include <cmath>

/* The released method never reads its console driver receiver. The native
 * queue is its backend submission capability, with no copied DSP state.
 * No unobserved bDriver.h or console class layout is introduced. */
class bDriver {
public:
    static WorldAudioResult PlatSetEffectVolume(WorldAudioRequestQueue* queue,
        uint32_t mask,uint32_t route,float volume,float pan);
};

WorldAudioResult bDriver::PlatSetEffectVolume(WorldAudioRequestQueue* queue,
    uint32_t mask,uint32_t route,float volume,float pan)
{
    (void)mask;
    (void)route;
    const float scaledVolume=volume*16383.0f;
    const float scaledPan=pan*255.0f;
    if (!queue || !std::isfinite(scaledVolume) || !std::isfinite(scaledPan) ||
        scaledVolume<-2147483648.0f || 2147483648.0f<=scaledVolume ||
        scaledPan<-2147483648.0f || 2147483648.0f<=scaledPan)
        return WORLD_AUDIO_INVALID_BINDING;
    if (worldAudioAvailableRequests(queue)<4) return WORLD_AUDIO_CAPACITY;

    const uint16_t level=static_cast<uint16_t>(convertFloatToInt32(scaledVolume));
    const uint16_t panWord=static_cast<uint16_t>(convertFloatToInt32(scaledPan));
    int32_t signedPan=panWord&255;
    if (signedPan>=128) signedPan-=256;
    // PAL25D560..25D5AC submits both hardware cores in this exact order.
    auto result=worldAudioSetCoreEffectVolume(queue,0,level);
    if (result!=WORLD_AUDIO_OK) return result;
    result=worldAudioSetCoreEffectPan(queue,0,static_cast<int8_t>(signedPan));
    if (result!=WORLD_AUDIO_OK) return result;
    result=worldAudioSetCoreEffectVolume(queue,1,level);
    if (result!=WORLD_AUDIO_OK) return result;
    return worldAudioSetCoreEffectPan(queue,1,static_cast<int8_t>(signedPan));
}

/* Same public game/SDK entry, now through the actual original-file CPU
 * implementation. Old opaque MIX-only transport is no parallel algorithm. */
WorldAudioResult worldAudioSetEffectMix(WorldAudioRequestQueue* queue,
    int32_t mask,int32_t route,float mix,float pan)
{
    return bDriver::PlatSetEffectVolume(queue,static_cast<uint32_t>(mask),
        static_cast<uint32_t>(route),mix,pan);
}

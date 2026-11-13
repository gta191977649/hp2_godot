#include "bsound/include/bSample.h"

// No unproved bEffect.h is created. The released class is an effect-control
// singleton; its native capability is the existing library's device queue.
// bSound::Bind already rejects a second owner for that queue. Zero-initialized
// queue state replaces PAL270370's singleton counter/initialization flag.
namespace {
class bEffect final {
public:
    explicit bEffect(WorldAudioRequestQueue& queue):queue_(queue) {}

    WorldAudioResult SetMode(uint32_t mode)
    {
        return SetType(mode); // PAL2703E8
    }

    WorldAudioResult SetType(uint32_t type)
    {
        // PAL270420 compares this cell before transforming the argument.
        // It never stores the transformed6 into the compared-type cell.
        if (queue_.effectConfigured && type==queue_.effectComparedType)
            return WORLD_AUDIO_OK;
        const size_t required=queue_.effectConfigured ? 14u : 12u;
        if (worldAudioAvailableRequests(&queue_)<required ||
            queue_.nextSequence>UINT64_MAX-required)
            return queue_.result=WORLD_AUDIO_CAPACITY;
        if (queue_.effectConfigured) {
            for (int32_t core=0;core<2;++core) {
                const auto result=worldAudioSubmitEffectControl(&queue_,core,0,0);
                if (result!=WORLD_AUDIO_OK)return result;
            }
        }
        // Both released switch arms choose6, including input5. These are
        // exact driver attributes per core, not invented DSP approximations.
        constexpr int32_t values[5]={6,16383,127,0,0};
        for (int32_t core=0;core<2;++core) {
            for (int32_t attribute=1;attribute<=5;++attribute) {
                const auto result=worldAudioSubmitEffectControl(&queue_,core,
                    attribute,values[attribute-1]);
                if (result!=WORLD_AUDIO_OK)return result;
            }
        }
        for (int32_t core=0;core<2;++core) {
            const auto result=worldAudioSubmitEffectControl(&queue_,core,0,1);
            if (result!=WORLD_AUDIO_OK)return result;
        }
        queue_.effectConfigured=1;
        return queue_.result=WORLD_AUDIO_OK;
    }
private:
    WorldAudioRequestQueue& queue_;
};
}

WorldAudioResult worldAudioSetEffectPreset(WorldAudioRequestQueue* queue,int32_t preset)
{
    if (!queue)return WORLD_AUDIO_INVALID_BINDING;
    return bEffect(*queue).SetMode(static_cast<uint32_t>(preset));
}

// PAL2705C0 SetParameter has no loads/stores/calls after saving arguments.
// Its actual no-op is eliminated at the sole SoundScape consumer; no empty
// method, success wrapper, or invented SET_EFFECT_PARAMETER remains live.

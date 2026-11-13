#ifndef NFSHP2_BREGION_H
#define NFSHP2_BREGION_H

#include "bsound/include/bSample.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>

/* One SDK bank's actual shared samples. Immutable bank metadata belongs to
 * the audio registry and outlives every retained request. This class owns
 * the samples in the existing queue pool, never private sample copies. */
class bRegion final {
public:
    static WorldAudioResult Initialize(size_t capacity);
    bRegion(WorldAudioRequestQueue& queue, const WorldAudioBank& bank);
    ~bRegion();
    bRegion(const bRegion&) = delete;
    bRegion& operator=(const bRegion&) = delete;

    WorldAudioVoiceRef Sample(const char* name) const;

    void removeSample(WorldAudioVoiceRef sample)
    {
        const auto found = std::find_if(samples_.begin(), samples_.end(),
            [sample](WorldAudioVoiceRef item) {
                return item.instance == sample.instance && item.generation == sample.generation;
            });
        if (found != samples_.end()) samples_.erase(found);
    }

    void addSample(WorldAudioVoiceRef sample)
    {
        if (worldAudioValidateVoice(queue_, sample) != WORLD_AUDIO_OK ||
            sample.instance->bank != bank_ || !sample.instance->bankSample)
            throw std::invalid_argument("Region sample does not belong to this live SDK bank");
        samples_.push_back(sample);
    }

    void SetName(const char* name)
    {
        if (!name) throw std::invalid_argument("Region name is absent");
        // PAL asserts on length>32, then retains the first32 bytes and NUL.
        const size_t size = std::min(std::strlen(name), size_t{32});
        std::copy_n(name, size, name_.data());
        name_[size] = 0;
    }

    const char* GetName() const noexcept
    {
        return name_.data();
    }

    /* Checked Windows shutdown: preserve STOP-before-UNLOAD order. A caller
     * must retain this owner on capacity failure and retry before destruction.
     * Metadata remains borrowed until the same queue acknowledges all work. */
    WorldAudioResult Dispose();
    const WorldAudioBank& Bank() const noexcept { return *bank_; }

private:
    std::array<char, 33> name_{};
    std::vector<WorldAudioVoiceRef> samples_;
    WorldAudioRequestQueue* queue_;
    const WorldAudioBank* bank_;
    bool disposed_{};
};

#endif

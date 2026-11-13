#include "bsound/include/bRegion.h"

#include <exception>
#include <memory>
#include <new>

namespace {

/* Native allocation bookkeeping for the one PAL GP-3610 region pool. The
 * pointed-to bRegion objects own their original sample list; this contains
 * no copied bank state, playing flags, voices or resource data. */
struct RegionAllocation {
    explicit RegionAllocation(size_t limit) : capacity(limit) { live.reserve(limit); }
    size_t capacity;
    std::vector<bRegion*> live;
};

std::unique_ptr<RegionAllocation> allocation;

} // namespace

WorldAudioResult bRegion::Initialize(size_t capacity)
{
    if (allocation) return WORLD_AUDIO_OK;
    if (!capacity || capacity > SIZE_MAX / sizeof(bRegion*))
        return WORLD_AUDIO_INVALID_BINDING;
    try {
        allocation = std::make_unique<RegionAllocation>(capacity);
        return WORLD_AUDIO_OK;
    } catch (const std::bad_alloc&) {
        return WORLD_AUDIO_CAPACITY;
    }
}

bRegion::bRegion(WorldAudioRequestQueue& queue, const WorldAudioBank& bank)
    : queue_(&queue), bank_(&bank)
{
    if (!allocation || allocation->live.size() >= allocation->capacity ||
        !queue.instances || !bank.name || (!bank.cues && bank.cueCount))
        throw std::invalid_argument("Region has no initialized allocation/audio bank owner");
    for (const auto* region : allocation->live) {
        if (&region->Bank() == &bank)
            throw std::invalid_argument("SDK bank already has its live region owner");
    }
    for (size_t index = 0; index < queue.instanceCapacity; ++index) {
        const auto* stored = worldAudioInstanceAt(&queue,index);
        if (!stored) throw std::invalid_argument("Region sample storage is not bound");
        const auto& sample = *stored;
        if (sample.occupied && sample.bank == &bank && sample.bankSample)
            throw std::invalid_argument("Region cannot adopt a second lazy bank-sample owner");
    }
    const auto capacity=worldAudioEnsureFreeInstances(&queue,bank.cueCount);
    if (capacity==WORLD_AUDIO_CAPACITY) throw std::bad_alloc();
    if (capacity!=WORLD_AUDIO_OK) throw std::invalid_argument("Region sample storage is not bound");
    SetName(bank.name);
    samples_.reserve(bank.cueCount);
    // PAL271770..27180C allocates/initializes every SDK master sample in
    // bank order before the region becomes available to name lookups.
    try {
        for (size_t index = 0; index < bank.cueCount; ++index) {
            WorldAudioVoiceRef sample{};
            if (!bank.cues[index].name || worldAudioAcquireBankSample(&queue,
                &bank, &bank.cues[index], &sample) != WORLD_AUDIO_OK)
                throw std::invalid_argument("Region master sample cannot be initialized");
            addSample(sample);
        }
        allocation->live.push_back(this);
    } catch (...) {
        for (const auto sample : samples_)
            if (worldAudioRetireVoice(&queue, sample) != WORLD_AUDIO_OK)
                std::terminate();
        throw;
    }
}

bRegion::~bRegion()
{
    if (Dispose() != WORLD_AUDIO_OK) std::terminate();
    if (allocation) {
        const auto found = std::find(allocation->live.begin(), allocation->live.end(), this);
        if (found != allocation->live.end()) allocation->live.erase(found);
    }
}

WorldAudioVoiceRef bRegion::Sample(const char* name) const
{
    if (!name || disposed_)
        throw std::invalid_argument("Region lookup has no live name/owner");
    const auto* cue = worldAudioFindInBank(bank_, name);
    if (!cue) return {};
    for (const auto sample : samples_) {
        if (worldAudioValidateVoice(queue_, sample) != WORLD_AUDIO_OK)
            throw std::invalid_argument("Region lookup contains a stale sample");
        if (sample.instance->cue == cue) return sample;
    }
    return {};
}

WorldAudioResult bRegion::Dispose()
{
    if (disposed_) return WORLD_AUDIO_OK;
    size_t needed = 1; // SDK unload follows every retained sample's deletion.
    for (const auto sample : samples_) {
        const auto status = worldAudioValidateVoice(queue_, sample);
        if (status != WORLD_AUDIO_OK) return status;
        if (sample.instance->hasHandle) ++needed;
    }
    if (worldAudioAvailableRequests(queue_) < needed) return WORLD_AUDIO_CAPACITY;
    while (!samples_.empty()) {
        const auto sample = samples_.front();
        removeSample(sample);
        const auto status = worldAudioRetireVoice(queue_, sample);
        if (status != WORLD_AUDIO_OK) return status;
    }
    const auto status = worldAudioUnloadBank(queue_, bank_, GetName());
    if (status == WORLD_AUDIO_OK) disposed_ = true;
    return status;
}

#ifndef NFSHP2_BVOICE_H
#define NFSHP2_BVOICE_H

#include "bsound/include/bSample.h"
#include <array>

// The48 physical voice bindings borrow the existing logical sample pool.
// Backend observations arrive with the exact queue/voice operation identity;
// neither this owner nor Acknowledge manufactures playback observations.
struct bVoiceStatus {
    WorldAudioVoiceRef sample{};
    uint64_t operationSequence{};
    bool playing{};
    bool prebuffered{};
    uint32_t channels{};
};
using bVoiceStatusSnapshot=std::array<bVoiceStatus,48>;

class bVoice final {
public:
    bVoice()=default; // PAL27C6F8: null sample; native invalid id has no use.
    uint16_t GetId() const noexcept { return id_; } // PAL275FF8
    WorldAudioVoiceRef GetSample() const noexcept { return sample_; } // PAL27A938
    uint64_t OperationSequence() const noexcept { return operationSequence_; }
    void Release() noexcept { sample_={};operationSequence_=0; } // PAL27C5C0
    void Set(uint16_t id,WorldAudioVoiceRef sample,uint64_t sequence) noexcept
    { id_=id;sample_=sample;operationSequence_=sequence; } // PAL27C5E8
private:
    uint16_t id_{};
    WorldAudioVoiceRef sample_{};
    uint64_t operationSequence_{};
};

class bVoiceMgr final {
public:
    bVoiceMgr();
    bVoiceMgr(const bVoiceMgr&)=delete;
    bVoiceMgr& operator=(const bVoiceMgr&)=delete;
    WorldAudioResult Bind(WorldAudioRequestQueue& queue);
    WorldAudioResult Unbind();
    void Initialize() noexcept;
    bVoice* GetVoice(uint16_t id,WorldAudioVoiceRef sample,uint64_t sequence);
    void ReleaseVoice(bVoice& voice) noexcept;
    void ReleaseSample(WorldAudioVoiceRef sample) noexcept;
    // PAL2638A0 locks contiguous channels from47 downward before bSample
    // publishes PLAY. Existing logical handles are the sole reservation
    // owners; no second sample table or physical-ready state is created.
    WorldAudioResult SelectChannels(WorldAudioVoiceRef sample,uint16_t& first,
        uint16_t& channels,bool& available) const;
    WorldAudioResult Assign(const WorldAudioRequest& operation,uint16_t deviceId);
    // Called after the backend really stops/releases this retained command.
    // This changes the device association, never acknowledges its transport.
    WorldAudioResult CompleteStop(const WorldAudioRequest& operation);
    WorldAudioResult Update(const bVoiceStatusSnapshot& status,bool* operationStarted=nullptr);
    WorldAudioResult CompleteUpdate(const WorldAudioRequest& operation,
        const bVoiceStatusSnapshot& status);
    void PrintVoiceAllocsAndReleases(const char* tag,uint32_t id,const char* name);
    WorldAudioResult PrintVoiceUsage(const bVoiceStatusSnapshot& status) const;
    const std::array<bVoice,48>& Voices() const noexcept { return voices_; }
private:
    std::array<bVoice,48> voices_{};
    std::array<char,145> diagnosticLine_{};
    uint32_t diagnosticRows_{};
    WorldAudioRequestQueue* queue_{};
    WorldAudioResult ReservationMap(std::array<bool,48>& used,size_t& channels) const;

};

#endif

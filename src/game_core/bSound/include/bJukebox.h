#ifndef NFSHP2_BJUKEBOX_H
#define NFSHP2_BJUKEBOX_H

#include "bsound/include/bSample.h"
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

namespace hp2 { class GameResources; }
struct bSound;

class cJukeboxCartridge {
public:
    struct Tune {
        std::string name;
        bool enabled{true};
        uint32_t size{},offset{},syncSize{},id{};
    };
    struct Group {
        std::string name;
        std::vector<size_t> tunes;
        bool enabled{true};
        uint32_t cursor{};
    };
    const char* GetTuneName(uint32_t group,uint32_t track) const;
    uint32_t GetTuneOffset(uint32_t group,uint32_t track) const;
    uint32_t GetSyncOffset(uint32_t group,uint32_t track) const;
    uint32_t GetTuneSize(uint32_t group,uint32_t track) const;
    uint32_t GetSyncSize(uint32_t group,uint32_t track) const;
    uint32_t GetTuneID(uint32_t group,uint32_t track) const;
    uint32_t AddCategory(const char* name);
    uint32_t AddTune(const char* group,const char* name,uint32_t offset,
        uint32_t size,uint32_t syncSize,uint32_t id);
    const Tune& Track(uint32_t group,uint32_t track) const;
    std::vector<Group> groups;
    std::vector<Tune> tunes;
};

/* The one actual music player. This is native ownership of the touched
 * source cells, not the console class layout. The SDK music loader/update
 * must publish its real current sample into this same owner. */
struct cJukeboxPlayer final : cJukeboxCartridge {
public:
    WorldAudioRequestQueue* audio{};
    WorldAudioVoiceRef currentSample{};
    uint32_t category{UINT32_MAX};
    uint32_t state{},repeat{},currentGroup{},currentTrack{},lastCategory{};
    int32_t enabled{1},pendingPlay{},paused{};
    float volumeScale{1.0f};

    WorldAudioResult Bind(bSound& library,const hp2::GameResources& files);
    // Native terminal detach after Close and the real audio owner shutdown.
    WorldAudioResult UnbindAfterAudioShutdown(bSound& library,WorldAudioRequestQueue& queue);
    bool IsCartridgeLoaded() const noexcept;
    WorldAudioResult LoadCartridge(const char* path,bool unused);
    WorldAudioResult Close();
    WorldAudioResult Start(uint32_t repeatMode);
    WorldAudioResult Start(uint32_t group,uint32_t track,uint32_t repeatMode);
    WorldAudioResult Start(const char* name,uint32_t repeatMode);
    WorldAudioResult Stop();
    WorldAudioResult FadeOut(uint32_t milliseconds);
    WorldAudioResult FadeOutAdvance(uint32_t milliseconds);
    WorldAudioResult Advance();
    WorldAudioResult Update();
    WorldAudioResult SetOn(int32_t on);
    WorldAudioResult SampleCallback(uint32_t event,WorldAudioVoiceRef voice);
    WorldAudioResult StopPlaying();
    WorldAudioResult Pause(int32_t pause);
    WorldAudioResult StartFading(uint32_t milliseconds);
    WorldAudioResult StartPlaying(uint32_t group,uint32_t track);
    WorldAudioResult StartPlaying();
    WorldAudioResult SetVolume(float volume);
    WorldAudioResult SetTuneOn(uint32_t group,uint32_t track);
    WorldAudioResult SetTuneOff(uint32_t group,uint32_t track);
    bool GetTuneOn(uint32_t group,uint32_t track) const;
    WorldAudioResult SetCategoryOn(uint32_t group);
    WorldAudioResult SetCategoryOff(uint32_t group);
    bool PickNextTune(uint32_t& group,uint32_t& track);
    bool FindTuneByName(uint32_t& group,uint32_t& track,const char* name);
    bool IsDonePlaying() const noexcept { return currentSample.instance && state==4; }
    bool IsDoneFading() const noexcept { return currentSample.instance && state==6; }
    bool IsFading()
    {
        const auto result=Update();
        if (result!=WORLD_AUDIO_OK) throw std::runtime_error("Jukebox update failed");
        return state==5;
    }
    bool IsPlaying()
    {
        const auto result=Update();
        if (result!=WORLD_AUDIO_OK) throw std::runtime_error("Jukebox update failed");
        return state==2 || IsFading();
    }
    uint32_t GetSampleRate() const noexcept { return sampleRate_; }

    WorldAudioResult SetCategory(uint32_t value)
    {
        category=value;
        if (!currentSample.instance) return WORLD_AUDIO_OK;
        if (!audio) return WORLD_AUDIO_INVALID_BINDING;
        return worldAudioSetVoiceCategory(audio,currentSample,value);
    }

private:
    bSound* library_{};
    const hp2::GameResources* files_{};
    std::unique_ptr<WorldAudioStreamSource> stream_;
    uint32_t sampleRate_{};
    bool closing_{};
};

extern cJukeboxPlayer soundJukeboxPlayer;

#endif

#ifndef NFSHP2_NATIVE_AUDIO_PROFILE_VALIDATION_HPP
#define NFSHP2_NATIVE_AUDIO_PROFILE_VALIDATION_HPP

#include "Fred.hpp"
#include "CarSounds.hpp"
#include "bsound/include/bJukebox.h"
#include <cmath>

namespace hp2 {

// Validate the native profile's actual audio fields before committing FRED
// or touching the live mixer. The optional catalogue is the loaded jukebox,
// never a guessed list of shipped tracks. Inactive rows still have valid
// domains: the original favourite-song paths also address fixed row slots.
inline bool nativeAudioProfileValid(const FrontendMusicSettings& settings,
    const cJukeboxCartridge* catalogue=nullptr) noexcept
{
    if(settings.count<0 || settings.count>static_cast<std::int32_t>(settings.rows.size()) ||
       settings.savedAudioMode<0 || settings.savedAudioMode>2)return false;
    // Profiles store mixer gains, while sliders display a normalized value.
    // Invert the same balance/default scale as FlailerSlider: music and
    // speech can legitimately exceed unity at the top of their sliders.
    // Preserve the already supported 0..1 profile range for other channels.
    constexpr std::array<std::int32_t,7> channels{{5,4,0,1,2,3,6}};
    const SoundVolumeSettings defaults;
    for(std::size_t index=0;index<settings.savedVolumes.size();++index) {
        const float displayScale=defaults.balance/GetVolumeDefault(defaults,channels[index]);
        const float sliderMaximum=1.0f/displayScale;
        const float maximum=sliderMaximum>1.0f?sliderMaximum:1.0f;
        const float volume=settings.savedVolumes[index];
        if(!std::isfinite(volume) || volume<0.0f || volume>maximum)return false;
    }
    for(const auto& row:settings.rows)
        if(row.group<0 || row.track<0 || row.status<0 || row.status>=6 ||
           row.genreBit<0 || row.genreBit>=6)return false;
    if(catalogue) {
        // Startup may have no cartridge and a wholly empty/default playlist.
        // A loaded catalogue validates all32 slots: favourite-song and menu
        // navigation consumers address fixed rows independently of count.
        const bool empty=catalogue->groups.empty() && catalogue->tunes.empty();
        if(empty && settings.count!=0)return false;
        for(const auto& row:settings.rows) {
            const auto group=static_cast<std::size_t>(row.group);
            const auto track=static_cast<std::size_t>(row.track);
            if(empty) {if(group!=0 || track!=0)return false;continue;}
            if(group>=catalogue->groups.size() || track>=catalogue->groups[group].tunes.size() ||
               catalogue->groups[group].tunes[track]>=catalogue->tunes.size())return false;
        }
    }
    // enabledGenres is an eight-bit persisted mask; source defaults are255,
    // including the two unused high bits. They are deliberately preserved.
    return true;
}

} // namespace hp2
#endif

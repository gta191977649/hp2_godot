#pragma once
// Port-side binding of original sound assets and PlayerEngineSound slices.
// E-0082-A01/A02; reverse/task0082/engine.md. No PS2 mixer/SPU ABI claim.
#include "game_core/indep/src/sound.hpp"
#include "memory.hpp"
#include <array>
#include <functional>
#include <string>

namespace port::driving {
using AudioAssetReader=std::function<std::vector<std::uint8_t>(const std::string&)>;
struct VehicleAudioSample {
    std::string name;
    hp2::port::audio::PcmSound pcm;
    bool loop=false;
    std::uint32_t loop_begin=0,loop_end=0; // PCM frames; original ADPCM flags.
    std::uint16_t patch_volume=0x3fff;
};
struct VehicleSoundBank {
    std::string basename,prefix;
    std::array<VehicleAudioSample,12> engine;
    VehicleAudioSample horn;
    std::array<std::vector<std::uint8_t>,2> definitions;
    std::array<std::int16_t,12> detune{};
    std::array<float,12> volume{};
    std::array<std::int16_t,4> timbre{};
    std::int16_t bend_range=0,bass_add=0,redline_offset=0,topgear_offset=0;
    std::uint16_t idle_offset=0;
    float horn_pitch=1;
};
VehicleSoundBank load_vehicle_sound_bank(const AudioAssetReader&,const std::array<std::uint8_t,0x560>& config);
// CalcPitch0026BA98 + bSnd_SetVoicePitch0025C7A8/SOUND_HzToSPU:
// original signed-octave/table quantization, relative to the decoded PCM rate.
float vehicle_sample_pitch_scale(const Memory&,std::uint32_t sample_rate,float octaves);
float vehicle_sample_gain_scale(const Memory&,std::uint16_t patch_volume,float requested_volume,float category_volume=1);
struct EngineAudioVoice {bool playing=false;float gain=0,pitch_scale=1;};
struct EngineAudioFrame {
    std::array<EngineAudioVoice,12> voices{};
    float current_rpm=0,redline_sync_rpm=0,gear_shift_scale=1;
    int current_throttle=0,controller_level=0;
};
class EngineAudioMixer {
public:
    void reset(){initialized_=false;throttle_=0;shifting_up_=false;shift_scale_=duck_scale_=1;duck_time_=0;volumes_={};playing_={};}
    // Bound outside-view, non-replay/playerless fixture. distance_volume and
    // music_volume explicitly supply original services; no invented RPM loops.
    EngineAudioFrame update(const VehicleSoundBank&,const Memory&,std::uint32_t car,
        std::uint32_t physics_car,float timestep,float distance_volume=1,int pov=0,float music_volume=0);
private:
    bool initialized_=false,shifting_up_=false;
    int throttle_=0;
    float current_rpm_=0,shift_scale_=1,duck_time_=0,duck_scale_=1;
    std::array<int,12> volumes_{};
    std::array<bool,12> playing_{};
};
}

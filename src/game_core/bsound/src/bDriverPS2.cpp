#include "bsound/include/bSound.h"
#include "bware/inc/bMath.hpp"

#include <stdexcept>
#include <cstdlib>
#include <cstring>

namespace hp2 {
namespace {
// Exact final-PAL322830..322A2E pan coefficients. The active SDK mode is
// borrowed from the command consumer; this table is immutable authored data.
constexpr std::array<uint16_t,255> panVolumes{{
    16383,16382,16381,16380,16378,16375,16371,16367,16363,16358,16352,16345,16338,16330,16322,
    16313,16304,16293,16283,16271,16259,16247,16233,16220,16205,16190,16174,16158,16141,16124,
    16106,16087,16068,16048,16027,16006,15984,15962,15939,15916,15892,15867,15841,15816,15789,
    15762,15734,15706,15677,15648,15618,15587,15556,15524,15491,15458,15425,15391,15356,15321,
    15285,15248,15211,15174,15135,15097,15057,15017,14977,14936,14894,14852,14810,14766,14722,
    14678,14633,14588,14542,14495,14448,14400,14352,14303,14254,14204,14154,14103,14052,14000,
    13947,13894,13841,13787,13732,13677,13621,13565,13509,13452,13394,13336,13277,13218,13158,
    13098,13038,12977,12915,12853,12790,12727,12664,12600,12535,12470,12405,12339,12273,12206,
    12139,12071,12003,11934,11865,11795,11725,11655,11584,11513,11441,11369,11296,11223,11150,
    11076,11002,10927,10852,10776,10700,10624,10547,10470,10393,10315,10237,10158,10079,9999,
    9920,9839,9759,9678,9597,9515,9433,9351,9268,9185,9101,9018,8934,8849,8764,
    8679,8594,8508,8422,8336,8249,8162,8075,7987,7899,7811,7722,7634,7545,7455,
    7365,7276,7185,7095,7004,6913,6822,6730,6639,6547,6454,6362,6269,6176,6083,
    5989,5896,5802,5708,5613,5519,5424,5329,5234,5139,5043,4947,4851,4755,4659,
    4562,4466,4369,4272,4175,4078,3980,3883,3785,3687,3589,3491,3393,3294,3196,
    2726,2639,2551,2464,2090,2013,1936,1859,1344,1286,1228,1169,838,794,750,
    705,333,311,289,266,123,112,100,89,39,33,28,22,8,5,
}};
}

// PAL266F08 / 266F60: attack and release fields of the authored ADSR word.
uint16_t GetAttack(uint32_t envelope) noexcept
{
    return static_cast<uint16_t>((envelope&0x7F000000u)>>24);
}
uint16_t GetRelease(uint32_t envelope) noexcept
{
    return static_cast<uint16_t>(envelope&0x1Fu);
}

// PAL267F58 / 267FA8. Channel code is zero-based, not a PCM channel count.
uint8_t GetChannelNum(uint8_t flags) noexcept { return flags>>4; }
uint8_t GetLoop(uint8_t flags) noexcept { return flags&1u; }

// PAL2697E0: preserve the four integer quadrants and truncation toward zero.
int8_t GetPanFromAzimuth(uint16_t azimuth) noexcept
{
    const int32_t value=azimuth;
    int32_t pan;
    if (value<=16384) pan=value*127/16384;
    else if (value>=49152) pan=(65536-value)*-127/16384;
    else if (value>=32768) pan=(value-32768)*-127/16384;
    else pan=(32768-value)*127/16384;
    return static_cast<int8_t>(pan);
}

// PAL269938 and the actual binary32 literals at32F21C/32F220. The two
// multiply/subtract roundings must not be replaced with integer ratios.
uint16_t GetAzimuthFromPan(int8_t pan)
{
    const float input=static_cast<float>(pan);
    const float scaled=input*(pan<0 ? -129.00787353515625f:129.00787353515625f);
    const float result=pan<0 ? 65536.0f-scaled:scaled;
    return static_cast<uint16_t>(convertFloatToInt32(result));
}

// PAL2671A8. Mode0/1 selects centered gain; mode2/3 uses the same table.
// The upstream PAL caller resolves its -128 sentinel before this function.
std::array<uint16_t,2> GetVolumesFromPan(uint16_t volume,int8_t pan,uint8_t mode)
{
    if (mode<2) pan=0;
    if (pan==-128) throw std::invalid_argument("Unresolved authored pan sentinel");
    const int32_t left=static_cast<int32_t>(pan)+127;
    const int32_t right=127-static_cast<int32_t>(pan);
    return {{static_cast<uint16_t>((uint32_t{volume}*panVolumes[static_cast<size_t>(left)])>>14),
        static_cast<uint16_t>((uint32_t{volume}*panVolumes[static_cast<size_t>(right)])>>14)}};
}

// Complete PAL269A00..269CFC. Preserve the released integer phase-history
// decisions, including its equality boundaries; modes other than3 do nothing.
void GetVolumesFromAzimuth(std::array<int32_t,2>& volume,
    const std::array<int32_t,2>& previous,int32_t azimuth,uint8_t mode) noexcept
{
    if (mode!=3)return;
    const auto negate=[](int32_t value) noexcept {
        const uint32_t bits=0u-static_cast<uint32_t>(value);
        int32_t result;std::memcpy(&result,&bits,sizeof(result));return result;
    };
    int prior;
    if ((previous[0]<0)==(previous[1]<0))
        prior=previous[0]<previous[1] ? 1:2;
    else {
        const int32_t left=previous[0]<0 ? negate(previous[0]):previous[0];
        const int32_t right=previous[1]<0 ? negate(previous[1]):previous[1];
        prior=right<left ? 4:3;
    }
    int flip=0;
    if (azimuth>=0x4000 && azimuth<=0xBFFF) {
        if (prior<3)flip=volume[0]<volume[1] && prior==2 ? 1:2;
    } else if (prior>=3) {
        flip=volume[0]<volume[1] && prior==3 ? 1:2;
    }
    if (previous[0]<0)volume[0]=negate(volume[0]);
    if (previous[1]<0)volume[1]=negate(volume[1]);
    if (flip)volume[static_cast<size_t>(flip-1)]=negate(volume[static_cast<size_t>(flip-1)]);
}

namespace {
void requireNativeMix(uint16_t channels,uint8_t mode)
{
    if ((channels!=1 && channels!=2) || mode<1 || mode>3)
        throw std::invalid_argument("Invalid native channel mix format or mode");
}
template<class Gain>
std::array<float,2> nativeCoefficients(const std::array<Gain,2>& gain) noexcept
{
    return {{static_cast<float>(gain[0])/16383.0f,
             static_cast<float>(gain[1])/16383.0f}};
}
NativeVoicePhase capturePhase(const std::array<int32_t,2>& gain) noexcept
{
    NativeVoicePhase result;
    result.inverted={{gain[0]<0,gain[1]<0}};
    if(result.inverted[0]==result.inverted[1])
        result.comparison=static_cast<uint8_t>(gain[0]<gain[1] ? 1:2);
    else {
        const auto left=std::abs(static_cast<int64_t>(gain[0]));
        const auto right=std::abs(static_cast<int64_t>(gain[1]));
        result.comparison=static_cast<uint8_t>(right<left ? 4:3);
    }
    return result;
}
void applyPhase(std::array<int32_t,2>& gain,const NativeVoicePhase& previous,
    uint16_t azimuth,uint8_t mode) noexcept
{
    if(mode!=3)return;
    const auto prior=previous.comparison;
    int flip=0;
    if(azimuth>=0x4000 && azimuth<=0xBFFF) {
        if(prior<3)flip=gain[0]<gain[1] && prior==2 ? 1:2;
    } else if(prior>=3)flip=gain[0]<gain[1] && prior==3 ? 1:2;
    for(size_t channel=0;channel<2;++channel)
        if(previous.inverted[channel])gain[channel]=-gain[channel];
    if(flip)gain[static_cast<size_t>(flip-1)]=-gain[static_cast<size_t>(flip-1)];
}
NativeVoicePhase phaseAfterAzimuth(const std::array<int32_t,2>& gain) noexcept
{
    // The original SetChannelAzimuth retains a different history than PLAY:
    // inversion resets and a negative channel sorts after a positive one.
    // Record those decisions directly, without constructing SPU register bits
    // and interpreting those bits again on the next native audio operation.
    NativeVoicePhase result;
    if((gain[0]<0)!=(gain[1]<0))result.comparison=static_cast<uint8_t>(gain[0]<0 ? 2:1);
    else result.comparison=static_cast<uint8_t>(gain[0]<gain[1] ? 1:2);
    return result;
}
}

// Native representation of the source's resident/stream channel parameters.
// No device state or playback completion is fabricated by these calculations.
void NativeVoiceMix::Initialize(uint16_t sourceChannels,bool residentSample,
    uint16_t gain,int8_t requestedPan,uint8_t mode)
{
    requireNativeMix(sourceChannels,mode);
    *this={};channels=sourceChannels;volume=gain;resident=residentSample;
    // bSample.Play calls260CE8: convert the captured pan to an angle first.
    //261274 converts it back for the resident/stream construction branches.
    const uint16_t angle=GetAzimuthFromPan(requestedPan);
    const int8_t monoPan=GetPanFromAzimuth(angle);
    for (size_t channel=0;channel<channels;++channel) {
        pan[channel]=channels==2 ? static_cast<int8_t>(channel ? 127:-127):monoPan;
        azimuth[channel]=resident ? angle:GetAzimuthFromPan(pan[channel]);
        // Stream mono start261BE8 centers the initial output. Its retained
        // pan/angle still drive subsequent ordinary volume/mode requests.
        const auto raw=GetVolumesFromPan(volume,!resident && channels==1 ? 0:pan[channel],mode);
        std::array<int32_t,2> values{{raw[0],raw[1]}};
        if (resident) {
            if (angle)GetVolumesFromAzimuth(values,values,angle,mode);
            //2622F0/262318 keep signed phase results on resident PLAY.
            phaseHistory[channel]=capturePhase(values);
        }
        output[channel]=nativeCoefficients(values);
    }
}

void NativeVoiceMix::SetChannelAzimuth(size_t channel,uint16_t angle,uint8_t mode)
{
    pan[channel]=GetPanFromAzimuth(angle);azimuth[channel]=angle;
    const auto raw=GetVolumesFromPan(volume,pan[channel],mode);
    std::array<int32_t,2> values{{raw[0],raw[1]}};
    if (mode!=1)applyPhase(values,phaseHistory[channel],angle,mode);
    output[channel]=nativeCoefficients(values);
    phaseHistory[channel]=phaseAfterAzimuth(values);
}

void NativeVoiceMix::SetVolume(uint16_t gain,uint8_t mode)
{
    requireNativeMix(channels,mode);
    if (volume==gain)return; //2640A8
    volume=gain;
    if (mode==3 && resident) {
        SetAzimuth(azimuth[0],mode);return; //2640E8,2642DC
    }
    for (size_t channel=0;channel<channels;++channel)
        output[channel]=nativeCoefficients(GetVolumesFromPan(volume,pan[channel],mode));
    // Ordinary volume2641EC updates volume, not the signed phase cache.
}

void NativeVoiceMix::Mute(bool muted,uint16_t& savedVolume,uint8_t mode)
{
    // PAL2747A0 saves the current ordinary hardware gain then uses the
    // ordinary gain setter. Later SetVolume is allowed to override zero.
    if (muted)savedVolume=volume;
    SetVolume(muted ? 0 : savedVolume,mode);
}

void NativeVoiceMix::SetAzimuth(uint16_t angle,uint8_t mode)
{
    requireNativeMix(channels,mode);
    if (!resident)return; //263F84: stream voices have no spatial-azimuth action.
    for (size_t channel=0;channel<channels;++channel)SetChannelAzimuth(channel,angle,mode);
}

void NativeVoiceMix::SetMode(uint8_t mode)
{
    requireNativeMix(channels,mode);
    for (size_t channel=0;channel<channels;++channel) {
        if (mode==1 && pan[channel]==0)continue; //25F1C4 keeps already centered channels.
        if (mode==3)SetChannelAzimuth(channel,azimuth[channel],mode);
        else {
            //25F158/25F248 select ordinary volume-pan, including streams.
            pan[channel]=mode==1 ? 0:GetPanFromAzimuth(azimuth[channel]);
            output[channel]=nativeCoefficients(GetVolumesFromPan(volume,pan[channel],mode));
        }
    }
}

std::array<float,4> NativeVoiceMix::Coefficients() const
{
    if (channels!=1 && channels!=2)throw std::invalid_argument("Uninitialized native channel matrix");
    std::array<float,4> result{};
    for (size_t source=0;source<channels;++source)
        for (size_t destination=0;destination<2;++destination)
            // XAudio2's destination-major order: SourceChannels*destination+source.
            result[channels*destination+source]=output[source][destination];
    return result;
}

}

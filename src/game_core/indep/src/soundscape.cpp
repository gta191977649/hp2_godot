#include "CarSounds.hpp"
#include "Car.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include "CameraMover.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Timer.hpp"
#include "Player.hpp"
#include "Track.hpp"
#include "WeatherMan.hpp"
#include "Race.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/PhysicsCar.hpp"
#include "bsound/include/bSound.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

SoundScapeControls soundScapeControls = {
    0.0f, 0.0f, 2,
    {0.6499999761581421f, 1.0f, 0.8500000238418579f, 0.75f}
};

namespace {

using AmbientCameras = std::array<const CarCinematicView*, 2>;

struct AmbientAudioFailure { WorldAudioResult result; };

void requireAmbientAudio(WorldAudioResult result)
{
    if (result != WORLD_AUDIO_OK) throw AmbientAudioFailure{result};
}

AmbientCameras activeCameras(const CarSoundBindings& bindings)
{
    AmbientCameras result{};
    for (size_t index = 0; index < result.size(); ++index)
        if (bindings.spatial.players->players[index])
            result[index] = bindings.spatial.cameras->screens[index+1].view;
    // PAL1CF2A8 increments s2 in an ordinary BEQ delay slot on BOTH paths.
    // After the two iterations s2 is2, so1CF2DC always skips the view fallback.
    return result;
}

void requireExtent(size_t cursor, size_t count, size_t stride, size_t end)
{
    if (cursor > end || count > (end-cursor)/stride)
        throw std::invalid_argument("SoundScape record exceeds its retained resource");
}

float authoredFloat(const std::vector<uint8_t>& bytes, size_t offset)
{
    const uint32_t bits = hp2::ReadU32(bytes.data()+offset);
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

struct SoundArea {
    std::vector<CarViewRegionVertex> vertices;
    std::vector<std::array<uint32_t, 3>> triangles;

    size_t Initialize(const std::vector<uint8_t>& bytes, size_t cursor, size_t end);
    bool IsPointInArea(const float point[2]) const;
    AmbientCameras AreCamerasInArea(const AmbientCameras& cameras, int32_t& count) const;
};

size_t SoundArea::Initialize(const std::vector<uint8_t>& bytes, size_t cursor, size_t end)
{
    requireExtent(cursor, 1, 4, end);
    const uint32_t vertexCount = hp2::ReadU32(bytes.data()+cursor);
    cursor += 4;
    requireExtent(cursor, vertexCount, 8, end);
    vertices.resize(vertexCount);
    for (auto& vertex : vertices) {
        vertex.x = authoredFloat(bytes, cursor);
        vertex.y = authoredFloat(bytes, cursor+4);
        cursor += 8;
    }
    requireExtent(cursor, 1, 4, end);
    const uint32_t triangleCount = hp2::ReadU32(bytes.data()+cursor);
    cursor += 4;
    requireExtent(cursor, triangleCount, 12, end);
    triangles.resize(triangleCount);
    for (auto& triangle : triangles) {
        for (auto& index : triangle) {
            index = hp2::ReadU32(bytes.data()+cursor);
            cursor += 4;
            if (index >= vertices.size())
                throw std::invalid_argument("SoundScape triangle has no authored vertex");
        }
    }
    return cursor;
}

bool SoundArea::IsPointInArea(const float point[2]) const
{
    for (const auto& indices : triangles) {
        const CarViewRegionVertex triangle[3] = {
            vertices[indices[0]], vertices[indices[1]], vertices[indices[2]]};
        if (carViewPointInPolygon(point, triangle, 3)) return true;
    }
    return false;
}

AmbientCameras SoundArea::AreCamerasInArea(const AmbientCameras& cameras, int32_t& count) const
{
    AmbientCameras selected{};
    count = 0;
    for (const auto* camera : cameras)
        if (camera && IsPointInArea(camera->current.eye)) selected[static_cast<size_t>(count++)] = camera;
    return selected;
}

const char* authoredName(const std::vector<uint8_t>& bytes, size_t offset, size_t end)
{
    requireExtent(offset, 1, 32, end);
    const char* name = reinterpret_cast<const char*>(bytes.data()+offset);
    if (!std::memchr(name, 0, 32)) throw std::invalid_argument("SoundScape sample name is unterminated");
    return name;
}

struct LoopingAmbient {
    const char* name = nullptr;
    const char* sampleName = nullptr;
    uint32_t fadeInMilliseconds = 0;
    uint32_t fadeOutMilliseconds = 0;
    int32_t wasInside = 0;
    int32_t directional = 0;
    int32_t useSphere = 0;
    WorldAudioVoiceRef voice{};
    SoundArea geometry;
    float position[3]{};
    float radius = 0.0f;

    void Initialize(const std::vector<uint8_t>& bytes, const hp2::Chunk& chunk);
    [[maybe_unused]] float ComputeVolume(const CarCinematicView& camera) const;
    uint16_t ComputeAzimuth(const CarCinematicView& camera) const;
    void UpdateUsingArea(const CarSoundBindings& bindings, const AmbientCameras& cameras);
    void UpdateUsingSoundSphere(const CarSoundBindings& bindings, const AmbientCameras& cameras);
    void Update(const CarSoundBindings& bindings, const AmbientCameras& cameras);
    void Mute(WorldAudioRequestQueue* audio, int32_t muted);
};

void LoopingAmbient::Initialize(const std::vector<uint8_t>& bytes, const hp2::Chunk& chunk)
{
    const size_t start = chunk.payloadOffset, end = start+chunk.size;
    requireExtent(start, 1, 0x58, end);
    name = authoredName(bytes, start, end);
    sampleName = authoredName(bytes, start+32, end);
    const int32_t fadeIn = convertFloatToInt32(authoredFloat(bytes, start+0x40)*1000.0f);
    const int32_t fadeOut = convertFloatToInt32(authoredFloat(bytes, start+0x44)*1000.0f);
    fadeInMilliseconds = static_cast<uint32_t>(fadeIn);
    fadeOutMilliseconds = static_cast<uint32_t>(fadeOut);
    directional = hp2::ReadU32(bytes.data()+start+0x48) != 0;
    for (size_t lane = 0; lane < 3; ++lane) position[lane] = authoredFloat(bytes, start+0x4C+lane*4);
    geometry.Initialize(bytes, start+0x58, end);
    wasInside = useSphere = 0;
    voice = {};
    radius = 0.0f;
    if (directional && geometry.IsPointInArea(position)) {
        useSphere = 1;
        for (const auto& vertex : geometry.vertices) {
            const float x = position[0]-vertex.x, y = position[1]-vertex.y;
            radius = std::max(radius, std::sqrt(x*x+y*y));
        }
    }
}

float LoopingAmbient::ComputeVolume(const CarCinematicView& camera) const
{
    const float x = position[0]-camera.current.eye[0], y = position[1]-camera.current.eye[1];
    const float distance = std::sqrt(x*x+y*y);
    const float scaled = distance*position[2];
    return (1.0f-scaled)*0.699999988079071f+0.30000001192092896f;
}

uint16_t LoopingAmbient::ComputeAzimuth(const CarCinematicView& camera) const
{
    float x = position[0]-camera.current.eye[0], y = position[1]-camera.current.eye[1];
    const float length = std::sqrt(x*x+y*y);
    x = finiteRatio(x, length);
    y = finiteRatio(y, length);
    const float first = x*camera.current.forward[0], second = y*camera.current.forward[1];
    uint16_t result = static_cast<uint16_t>(0x4000-vectorArcSineAngle(first+second));
    const float side = -y*camera.current.forward[0]+x*camera.current.forward[1];
    if (side < 0.0f) result = static_cast<uint16_t>(-result);
    return result;
}

void LoopingAmbient::UpdateUsingArea(const CarSoundBindings& bindings, const AmbientCameras& cameras)
{
    int32_t count;
    const auto selected = geometry.AreCamerasInArea(cameras, count);
    if (wasInside) {
        if (count > 0) {
            if (directional) {
                int32_t total = 0;
                for (int32_t index = 0; index < count; ++index)
                    total += ComputeAzimuth(*selected[static_cast<size_t>(index)]);
                if (count <= 0) throw std::invalid_argument("Ambient azimuth requires positive camera count");
                requireAmbientAudio(worldAudioSetVoiceControlCode(bindings.audio, voice, static_cast<uint16_t>(total/count)));
            }
        } else {
            if (!fadeOutMilliseconds) requireAmbientAudio(worldAudioStopInstance(bindings.audio, voice.instance));
            else requireAmbientAudio(worldAudioFadeVoiceVolume(bindings.audio, voice, 0.0f, fadeOutMilliseconds));
            wasInside = 0;
        }
    } else if (count > 0) {
        if (bindings.globals->policy.audioSystemEnabled) {
            const float gain = fadeInMilliseconds ? 0.0f : 0.8399999737739563f;
            requireAmbientAudio(worldAudioPlayVoice(bindings.audio, voice, gain, 0.0f, 0.0f));
            if (fadeInMilliseconds)
                requireAmbientAudio(worldAudioFadeVoiceVolume(bindings.audio, voice, 0.8399999737739563f, fadeInMilliseconds));
        }
        wasInside = 1;
    } else if (voice.instance->playing) {
        float volume;
        requireAmbientAudio(worldAudioGetVoiceVolume(bindings.audio, voice, &volume));
        if (volume == 0.0f) requireAmbientAudio(worldAudioStopInstance(bindings.audio, voice.instance));
    }
}

void LoopingAmbient::UpdateUsingSoundSphere(const CarSoundBindings& bindings, const AmbientCameras& cameras)
{
    AmbientCameras selected{};
    int32_t count = 0;
    for (const auto* camera : cameras) {
        if (!camera) continue;
        const float x = camera->current.eye[0]-position[0], y = camera->current.eye[1]-position[1];
        if (std::sqrt(x*x+y*y) < radius) selected[static_cast<size_t>(count++)] = camera;
    }
    if (count > 0) {
        float distance = 0.0f;
        int32_t totalAzimuth = 0;
        for (int32_t index = 0; index < count; ++index) {
            const auto& camera = *selected[static_cast<size_t>(index)];
            totalAzimuth += ComputeAzimuth(camera);
            const float x = camera.current.eye[0]-position[0], y = camera.current.eye[1]-position[1];
            distance = distance+std::sqrt(x*x+y*y);
        }
        if (count <= 0) throw std::invalid_argument("Ambient sphere requires positive camera count");
        const uint16_t azimuth = static_cast<uint16_t>(totalAzimuth/count);
        distance = distance/static_cast<float>(count);
        const float inverse = finiteRatio(1.0f, radius);
        float volume;
        if (radius < distance) volume = 0.0f;
        else {
            const float scaled = distance*inverse;
            const float squared = scaled*scaled;
            const float doubled = (distance+distance)*inverse;
            const float root = std::sqrt(doubled-squared);
            const float remaining = 1.0f-root;
            volume = remaining+remaining;
        }
        if (!voice.instance->playing) {
            if (bindings.globals->policy.audioSystemEnabled)
                requireAmbientAudio(worldAudioPlayVoice(bindings.audio, voice, volume, 0.0f, 0.0f));
        } else requireAmbientAudio(worldAudioSetVoiceVolume(bindings.audio, voice, volume));
        requireAmbientAudio(worldAudioSetVoiceControlCode(bindings.audio, voice, azimuth));
    } else if (voice.instance->playing) {
        requireAmbientAudio(worldAudioStopInstance(bindings.audio, voice.instance));
    }
}

void LoopingAmbient::Update(const CarSoundBindings& bindings, const AmbientCameras& cameras)
{
    if (!voice.instance) {
        if (!bindings.trackBank) return;
        const auto* cue = worldAudioFindInBank(bindings.trackBank, sampleName);
        if (!cue) return;
        requireAmbientAudio(worldAudioInitializeVoiceFromCue(bindings.audio, bindings.trackBank, cue, &voice));
        requireAmbientAudio(worldAudioSetVoiceCategory(bindings.audio, voice, 0));
    }
    if (useSphere) UpdateUsingSoundSphere(bindings, cameras);
    else UpdateUsingArea(bindings, cameras);
}

void LoopingAmbient::Mute(WorldAudioRequestQueue* audio, int32_t muted)
{
    if (voice.instance) requireAmbientAudio(worldAudioPauseVoice(audio, voice, muted));
}

struct OneShotAmbient {
    const char* name = nullptr;
    const char* sampleName = nullptr;
    float interval = 0.0f, intervalRange = 0.0f;
    float pitch = 0.0f, pitchRange = 0.0f, angle = 0.0f;
    int32_t wasInside = 0;
    float remaining = 0.0f;
    WorldAudioVoiceRef sharedVoice{}, secondVoice{};
    SoundArea geometry;

    void Initialize(const std::vector<uint8_t>& bytes, const hp2::Chunk& chunk);
    void Play(const CarSoundBindings& bindings);
    void Update(const CarSoundBindings& bindings, const AmbientCameras& cameras);
    void Mute(WorldAudioRequestQueue* audio, int32_t muted);
};

void OneShotAmbient::Initialize(const std::vector<uint8_t>& bytes, const hp2::Chunk& chunk)
{
    const size_t start = chunk.payloadOffset, end = start+chunk.size;
    requireExtent(start, 1, 0x54, end);
    name = authoredName(bytes, start, end);
    sampleName = authoredName(bytes, start+32, end);
    interval = authoredFloat(bytes, start+0x40);
    intervalRange = authoredFloat(bytes, start+0x44);
    pitch = authoredFloat(bytes, start+0x48);
    pitchRange = authoredFloat(bytes, start+0x4C);
    angle = authoredFloat(bytes, start+0x50);
    geometry.Initialize(bytes, start+0x54, end);
    remaining = 0.0f;
    wasInside = 0;
    sharedVoice = {};
    // The embedded second bSample is a global-lifetime member. Initialize
    // does not recreate or reset it when the same slot receives another area.
}

void OneShotAmbient::Play(const CarSoundBindings& bindings)
{
    if (!sharedVoice.instance) return;
    float selectedPitch = pitch;
    if (pitchRange != 0.0f) {
        const float random = vectorRandomFloat(&worldAudioRuntime.randomState, pitchRange);
        selectedPitch = (pitch-pitchRange*0.5f)+random;
    }
    uint16_t azimuth = 0;
    if (angle == 360.0f) azimuth = 0x4000;
    else if (angle != 0.0f)
        azimuth = static_cast<uint16_t>(vectorRandomBounded(65536, &worldAudioRuntime.randomState));
    if (bindings.globals->policy.audioSystemEnabled) {
        requireAmbientAudio(worldAudioPlayVoice(bindings.audio, sharedVoice, 0.8399999737739563f, 0.0f, 0.0f));
        requireAmbientAudio(worldAudioSetVoiceControlCode(bindings.audio, sharedVoice, azimuth));
        requireAmbientAudio(worldAudioSetVoicePitchOffset(bindings.audio, sharedVoice, selectedPitch));
        if (angle == 360.0f) {
            requireAmbientAudio(worldAudioPlayVoice(bindings.audio, secondVoice, 0.8399999737739563f, 0.0f, 0.0f));
            requireAmbientAudio(worldAudioSetVoiceControlCode(bindings.audio, secondVoice, 0xC000));
            requireAmbientAudio(worldAudioSetVoicePitchOffset(bindings.audio, secondVoice, selectedPitch));
        }
    }
    if (0.0f < interval) {
        float delay = interval;
        if (intervalRange != 0.0f) {
            const float random = vectorRandomFloat(&worldAudioRuntime.randomState, intervalRange);
            delay = (interval-intervalRange*0.5f)+random;
        }
        remaining = delay;
    }
}

void OneShotAmbient::Update(const CarSoundBindings& bindings, const AmbientCameras& cameras)
{
    if (!sharedVoice.instance) {
        if (!bindings.trackBank) return;
        const auto* cue = worldAudioFindInBank(bindings.trackBank, sampleName);
        if (!cue) return;
        requireAmbientAudio(worldAudioAcquireBankSample(bindings.audio, bindings.trackBank, cue, &sharedVoice));
        requireAmbientAudio(worldAudioSetVoiceCategory(bindings.audio, sharedVoice, 0));
        if (angle == 360.0f) {
            requireAmbientAudio(worldAudioCopyVoice(bindings.audio, sharedVoice, &secondVoice));
            requireAmbientAudio(worldAudioSetVoiceCategory(bindings.audio, secondVoice, 0));
        }
    }
    int32_t count;
    geometry.AreCamerasInArea(cameras, count);
    if (wasInside) {
        if (count <= 0) wasInside = 0;
        else if (0.0f < interval) {
            remaining = remaining-bindings.clock->worldTimeElapsed;
            if (remaining < 0.0f) Play(bindings);
        }
    } else if (count > 0) {
        Play(bindings);
        wasInside = 1;
    }
}

void OneShotAmbient::Mute(WorldAudioRequestQueue* audio, int32_t muted)
{
    if (sharedVoice.instance) requireAmbientAudio(worldAudioPauseVoice(audio, sharedVoice, muted));
}

struct ReverbAreaSample {
    const char* name;
    float firstParameter;
    float secondParameter;
};

struct ReverbArea {
    const uint8_t* record = nullptr;
    uint32_t parameter20 = 0;
    SoundArea geometry;
    std::vector<ReverbAreaSample> samples;

    void Initialize(const std::vector<uint8_t>& bytes, const hp2::Chunk& chunk);
};

void ReverbArea::Initialize(const std::vector<uint8_t>& bytes, const hp2::Chunk& chunk)
{
    const size_t start = chunk.payloadOffset;
    const size_t end = start+chunk.size;
    requireExtent(start, 1, 0x24, end);
    record = bytes.data()+start;
    parameter20 = hp2::ReadU32(record+0x20);
    size_t cursor = geometry.Initialize(bytes, start+0x24, end);
    requireExtent(cursor, 1, 4, end);
    const uint32_t count = hp2::ReadU32(bytes.data()+cursor);
    cursor += 4;
    // Original ReverbArea has exactly eight12-byte sample descriptors in
    // its124-byte native-console record. Reject an invalid resource extent;
    // do not clamp it or invent adjacent descriptors.
    if (count > 8) throw std::invalid_argument("SoundScape reverb sample count exceeds its actual owner");
    requireExtent(cursor, count, 40, end);
    samples.reserve(count);
    for (uint32_t index = 0; index < count; ++index) {
        samples.push_back({reinterpret_cast<const char*>(bytes.data()+cursor),
            authoredFloat(bytes, cursor+32), authoredFloat(bytes, cursor+36)});
        cursor += 40;
    }
}

// These are the actual current-resource view and GP-46A8/-46A4/-46A0 state.
// The source current-area word is initialized-1; there is no tunnel bool.
struct SoundScapeState {
    const CarSoundBindings* bindings = nullptr;
    const std::vector<uint8_t>* resource = nullptr;
    std::vector<ReverbArea> reverbAreas;
    std::array<LoopingAmbient, 32> looping;
    std::array<OneShotAmbient, 48> oneShots;
    size_t loopingCount = 0, oneShotCount = 0;
    WorldAudioVoiceRef weatherVoice{};
    int32_t dataJustLoaded = 0;
    int32_t currentReverbAreaIndex = -1;
    int32_t reverbActive = 0;
    float currentMix = 0.0f;
    float reflectionDistance = 7.0f; // actual mutable GP-4660, separate from the sample envelopes
    int32_t currentPreset = 2;
    std::array<float, 4> currentParameters = {0.6499999761581421f, 1.0f, 0.8500000238418579f, 0.75f};

    bool WillSoundRight(int32_t area, const CarMovement* car) const;
    int32_t FindGoodReverbArea(const CarMovement* car) const;
    int32_t IsInsideReverbArea(const CarMovement* first, const CarMovement* second);
    void SetReverbVolume(float volume, CarMovement* car);
    void UpdateReverbAreas();
    void UpdateLoopingAmbient();
    void UpdateOneShotAmbient();
    void UpdateWeather();
    void UpdateReflectionReverb(const std::array<CarMovement*, 2>& cars);
    void Update();
    void Mute(int32_t muted);
};

SoundScapeState soundScape;

bool SoundScapeState::WillSoundRight(int32_t area, const CarMovement* car) const
{
    if (area >= static_cast<int32_t>(reverbAreas.size())) return false;
    if (area < 0 || !car || !car->bindings.owner || !car->bindings.state ||
        car->bindings.state->position != car->bindings.owner->position)
        throw std::invalid_argument("SoundScape has no actual Car position or a corrupt area index");
    return reverbAreas[static_cast<size_t>(area)].geometry.IsPointInArea(car->bindings.state->position);
}

int32_t SoundScapeState::FindGoodReverbArea(const CarMovement* car) const
{
    for (size_t index = 0; index < reverbAreas.size(); ++index)
        if (WillSoundRight(static_cast<int32_t>(index), car)) return static_cast<int32_t>(index);
    return -1;
}

int32_t SoundScapeState::IsInsideReverbArea(const CarMovement* first, const CarMovement* second)
{
    int32_t selectedPlayer = 0;
    bool refresh = false;
    if (currentReverbAreaIndex == -1 || dataJustLoaded) {
        refresh = true;
        dataJustLoaded = 0;
    } else if (!WillSoundRight(currentReverbAreaIndex, first)) {
        refresh = true;
    } else if (second && !WillSoundRight(currentReverbAreaIndex, second)) {
        refresh = true;
        selectedPlayer = 1;
    }
    if (refresh) {
        int32_t area = FindGoodReverbArea(first);
        if (area == -1 && second) {
            selectedPlayer = 1;
            area = FindGoodReverbArea(second);
        }
        currentReverbAreaIndex = area;
    }
    return selectedPlayer;
}

void SoundScapeState::SetReverbVolume(float volume, CarMovement* car)
{
    if (bindings->race->services->settings->mode < 2) soundScapeControls.desiredMix = volume;
    else if (car) {
        if (!car->frame || car->frame->movement != car)
            throw std::invalid_argument("SoundScape Car has no same frame sound owner");
        if (car->frame->environment.sound)
            requireAmbientAudio(car->frame->environment.sound->ScaleRoadNoise(volume+1.0f));
    }
}

void SoundScapeState::UpdateReverbAreas()
{
    const float step = currentMix < soundScapeControls.desiredMix ? 0.3330000042915344f : 0.05000000074505806f;
    const float updated = physicsClampDelta(currentMix, soundScapeControls.desiredMix, step);
    if (updated != currentMix) {
        currentMix = updated;
        requireAmbientAudio(worldAudioSetEffectMix(bindings->audio, -1, 1, currentMix, soundScapeControls.pan));
    }
    if (soundScapeControls.preset != currentPreset) {
        static constexpr int32_t presets[13] = {0, 1, 2, 3, 4, 5, 6, 7, 101, 102, 103, 104, 105};
        if (soundScapeControls.preset < 0 || soundScapeControls.preset >= 13)
            throw std::invalid_argument("SoundScape has no actual317C48 DSP preset");
        SetReverbVolume(0.0f, nullptr);
        currentPreset = soundScapeControls.preset;
        requireAmbientAudio(worldAudioSetEffectPreset(bindings->audio, presets[static_cast<size_t>(soundScapeControls.preset)]));
        reverbActive = 0;
    }
    for (size_t index = 0; index < soundScapeControls.parameters.size(); ++index) {
        if (soundScapeControls.parameters[index] == currentParameters[index]) continue;
        currentParameters[index] = soundScapeControls.parameters[index];
        // Final PAL2705C0 bEffect::SetParameter performs no operation.
        // Keep the real caller's cache writes; eliminate that empty callee
        // instead of submitting an invented physical DSP command.
    }

    std::array<CarMovement*, 2> cars{};
    bool any = false;
    for (size_t index = 0; index < cars.size(); ++index) {
        const auto* player = bindings->spatial.players->players[index];
        if (!player) continue;
        cars[index] = player->car;
        if (!cars[index] || !cars[index]->bindings.owner || !cars[index]->bindings.state)
            throw std::invalid_argument("SoundScape player has no actual Car position");
        any = true;
    }
    if (!any) return;
    if (!cars[0]) throw std::invalid_argument("SoundScape requires the source creation-order first player");
    const int32_t selected = IsInsideReverbArea(cars[0], cars[1]);
    if (currentReverbAreaIndex == -1) {
        if (reverbActive) {
            for (auto* car : cars) if (car) SetReverbVolume(0.0f, car);
            reverbActive = 0;
        }
        return;
    }
    CarMovement* car = cars[static_cast<size_t>(selected)];
    if (!car || !car->bindings.environment || !car->bindings.environment->contact.terrain)
        throw std::invalid_argument("SoundScape selected Car has no same terrain-surface owner");
    auto* surface = car->bindings.environment->contact.terrain;
    float point[4];
    std::copy_n(car->bindings.state->position, 4, point);
    const float ground = terrainSurfaceElevation(surface, point, nullptr, nullptr, nullptr);
    point[2] = 99999.0f;
    const float upper = terrainSurfaceElevation(surface, point, nullptr, nullptr, nullptr);
    point[2] = upper-2.0f;
    const float lower = terrainSurfaceElevation(surface, point, nullptr, nullptr, nullptr);
    const bool elevated = ground < upper;
    const bool enclosed = ground <= lower;
    if (!reverbActive) {
        if (elevated || enclosed) {
            soundScapeControls.pan = 0.0f;
            SetReverbVolume(0.8500000238418579f, car);
            reverbActive = 1;
        }
    } else if (!elevated && !enclosed) {
        SetReverbVolume(0.0f, car);
        reverbActive = 0;
    }
}

void SoundScapeState::UpdateLoopingAmbient()
{
    for (size_t index = 0; index < loopingCount; ++index)
        looping[index].Update(*bindings, activeCameras(*bindings));
}

void SoundScapeState::UpdateOneShotAmbient()
{
    for (size_t index = 0; index < oneShotCount; ++index)
        oneShots[index].Update(*bindings, activeCameras(*bindings));
}

void SoundScapeState::UpdateWeather()
{
    if (!weatherVoice.instance) {
        if (worldAudioRuntime.disabled != 0) return;
        const auto* cue = worldAudioFindInGame(bindings->audio, "Rain");
        if (!cue) return;
        requireAmbientAudio(worldAudioAcquireBankSample(bindings->audio, bindings->audio->inGameBank,
            cue, &weatherVoice));
    }
    const int32_t mode = bindings->race->services->settings->mode;
    if (mode < 0 || mode > 2)
        throw std::invalid_argument("Weather audio exceeds the actual two-camera span");
    const auto cameras = activeCameras(*bindings);
    float intensity = 0.0f;
    for (int32_t index = 0; index < mode; ++index) {
        const auto* camera = cameras[static_cast<size_t>(index)];
        if (!camera) continue;
        // PAL passes a local query record. Initialize native scratch storage
        // so partial override writes have defined Windows memory; it is not
        // a persistent precipitation default or another weather owner.
        WeatherQuery query{};
        if (weatherManQuery(&query, camera->current.eye) != WEATHER_MAN_OK)
            throw std::invalid_argument("Weather audio has no actual WeatherMan query owner");
        intensity = intensity+query.precipitation;
    }
    if (0.0f < intensity) {
        if (!weatherVoice.instance->playing) {
            if (bindings->globals->policy.audioSystemEnabled)
                requireAmbientAudio(worldAudioPlayVoice(bindings->audio, weatherVoice,
                    intensity*0.8399999737739563f, 0.0f, 0.0f));
        } else requireAmbientAudio(worldAudioSetVoiceVolume(bindings->audio, weatherVoice, intensity));
    } else if (weatherVoice.instance->playing) {
        requireAmbientAudio(worldAudioStopInstance(bindings->audio, weatherVoice.instance));
    }
}

float calculateDistToWall(const float point[4], const TerrainSurface& surface,
    const float*& selectedPoint)
{
    float closest = 999999.0f;
    for (size_t index = 0; index < surface.barrierCount; ++index) {
        const size_t offset = surface.groundCount+index;
        if (offset >= surface.polygonCount || !surface.polygons[offset])
            throw std::invalid_argument("SoundScape reflection has an invalid same-owner terrain cache");
        const auto& polygon = *surface.polygons[offset];
        for (const size_t vertex : {size_t(0), size_t(2)}) {
            const float* endpoint = polygon.vertices[vertex];
            const float x = endpoint[0]-point[0], y = endpoint[1]-point[1];
            const float distance = std::sqrt(x*x+y*y);
            if (distance < closest) { closest = distance; selectedPoint = endpoint; }
        }
    }
    return closest;
}

void SoundScapeState::UpdateReflectionReverb(const std::array<CarMovement*, 2>& cars)
{
    const auto cameras = activeCameras(*bindings);
    const int32_t mode = bindings->race->services->settings->mode;
    if (mode < 0 || mode > 2) throw std::invalid_argument("Reflection reverb exceeds its actual two-car span");
    float volume = 0.0f;
    soundScapeControls.pan = 0.0f;
    const float* wall = nullptr;
    for (int32_t index = 0; index < mode; ++index) {
        const size_t slot = static_cast<size_t>(index);
        if (!cameras[slot]) continue;
        const auto* car = cars[slot];
        if (!car) throw std::invalid_argument("Reflection reverb has no actual current car");
        if (!car->bindings.player) continue;
        if (!car->frame || car->frame->movement != car || !car->frame->environment.sound)
            throw std::invalid_argument("Reflection reverb has no same Player sound owner");
        const auto* playerSound = dynamic_cast<const BasePlayerCarSound*>(car->frame->environment.sound);
        if (!playerSound || !playerSound->ReflectionSurface() || !car->bindings.state)
            throw std::invalid_argument("Reflection reverb has no actual Player topology owner");
        const float distance = calculateDistToWall(car->bindings.state->position,
            *playerSound->ReflectionSurface(), wall);
        float strength = 1.0f-finiteRatio(distance, reflectionDistance);
        if (strength < 0.0f) strength = 0.0f;
        if (1.0f < strength) strength = 1.0f;
        volume = volume+strength;
    }
    if (0.0f < volume && wall) {
        const float point[4] = {wall[0], wall[1], 0.0f, 0.0f};
        uint16_t azimuth;
        if (sound3dComputeAzimuth(&bindings->spatial, point, &azimuth) != SOUND3D_OK)
            throw std::invalid_argument("Reflection reverb has no actual listener azimuth");
        soundScapeControls.pan = sound3dAzimuthToPan(azimuth);
    }
    SetReverbVolume(volume, nullptr);
}

void SoundScapeState::Update()
{
    // GP-56C8 is the current GameSession pointer. The source frame tests its
    // presence, while consumers such as1AE1BC separately read session+0 state.
    if (!bindings->session) return;
    UpdateReverbAreas();
    UpdateLoopingAmbient();
    UpdateOneShotAmbient();
}

void SoundScapeState::Mute(int32_t muted)
{
    for (size_t index = 0; index < loopingCount; ++index) looping[index].Mute(bindings->audio, muted);
    for (size_t index = 0; index < oneShotCount; ++index) oneShots[index].Mute(bindings->audio, muted);
    if (weatherVoice.instance && weatherVoice.instance->playing)
        requireAmbientAudio(worldAudioMuteVoice(bindings->audio, weatherVoice, muted));
    if (muted) {
        requireAmbientAudio(worldAudioSetEffectMix(bindings->audio, -1, 1, 0.0f, 0.0f));
        reverbActive = 0;
        currentReverbAreaIndex = -1;
    }
}

} // namespace

int32_t soundScapeIsInsideTunnel()
{
    return soundScape.currentReverbAreaIndex != -1;
}

WorldAudioResult soundScapeLoadResources(const CarSoundBindings& bindings,
    const hp2::TrackScenarioData& track)
{
    if (!track.CurrentInfo() || bindings.trackInfo != track.CurrentInfo() || soundScape.resource)
        return WORLD_AUDIO_INVALID_BINDING;
    const WorldAudioResult bind = soundScapeBindServices(bindings);
    if (bind != WORLD_AUDIO_OK) return bind;
    const auto& bytes = track.Authored();
    try {
        std::vector<ReverbArea> areas;
        std::vector<hp2::Chunk> loopingChunks, oneShotChunks;
        bool found = false;
        const auto visit = [&](const auto& self, size_t offset, size_t extent) -> void {
            for (const auto& chunk : hp2::ReadChunks(bytes, offset, extent)) {
                if (chunk.id == 0x80038000u) {
                    if (found) throw std::invalid_argument("Multiple live SoundScape38000 owners");
                    found = true;
                    for (const auto& child : hp2::ReadChunks(bytes, chunk.payloadOffset, chunk.size)) {
                        if (child.id == 0x38010u) {
                            ReverbArea area;
                            area.Initialize(bytes, child);
                            areas.push_back(std::move(area));
                        } else if (child.id == 0x38020u) loopingChunks.push_back(child);
                        else if (child.id == 0x38030u) oneShotChunks.push_back(child);
                    }
                } else if (chunk.IsContainer()) {
                    self(self, chunk.payloadOffset, chunk.size);
                }
            }
        };
        visit(visit, 0, bytes.size());
        if (!found) return WORLD_AUDIO_INVALID_BINDING;
        if (areas.size() > 32 || loopingChunks.size() > soundScape.looping.size() ||
            oneShotChunks.size() > soundScape.oneShots.size()) return WORLD_AUDIO_INVALID_BINDING;
        for (size_t index = 0; index < loopingChunks.size(); ++index)
            soundScape.looping[index].Initialize(bytes, loopingChunks[index]);
        for (size_t index = 0; index < oneShotChunks.size(); ++index)
            soundScape.oneShots[index].Initialize(bytes, oneShotChunks[index]);
        soundScape.resource = &bytes;
        soundScape.reverbAreas = std::move(areas);
        soundScape.loopingCount = loopingChunks.size();
        soundScape.oneShotCount = oneShotChunks.size();
        soundScape.currentReverbAreaIndex = -1;
        soundScape.dataJustLoaded = 1;
        return WORLD_AUDIO_OK;
    } catch (const std::exception&) {
        return WORLD_AUDIO_INVALID_BINDING;
    }
}

WorldAudioResult soundScapeBindServices(const CarSoundBindings& bindings)
{
    // Track resource chunks arrive before RaceCoordinator creates Race.
    // Decoding does not read Race; the live update requires it explicitly.
    if (!bindings.audio || !bindings.globals || !bindings.clock ||
        !bindings.spatial.players || !bindings.spatial.cameras || bindings.spatial.clock != bindings.clock ||
        (soundScape.bindings && soundScape.bindings != &bindings)) return WORLD_AUDIO_INVALID_BINDING;
    if (weatherManBindClock(bindings.clock) != WEATHER_MAN_OK) return WORLD_AUDIO_INVALID_BINDING;
    soundScape.bindings = &bindings;
    return WORLD_AUDIO_OK;
}

WorldAudioResult soundScapeUnloadResources(const hp2::TrackScenarioData& track)
{
    if (soundScape.resource != &track.Authored() || !soundScape.bindings) return WORLD_AUDIO_INVALID_BINDING;
    try {
        soundScape.currentReverbAreaIndex = -1;
        soundScape.reverbAreas.clear();
        soundScape.oneShotCount = 0;
        soundScape.weatherVoice = {};
        for (size_t index = 0; index < soundScape.loopingCount; ++index) {
            auto& voice = soundScape.looping[index].voice;
            if (!voice.instance) continue;
            requireAmbientAudio(worldAudioStopInstance(soundScape.bindings->audio, voice.instance));
            requireAmbientAudio(worldAudioRetireVoice(soundScape.bindings->audio, voice));
            voice = {};
        }
        soundScape.loopingCount = 0;
        soundScape.resource = nullptr;
        return WORLD_AUDIO_OK;
    } catch (const AmbientAudioFailure& failure) {
        return failure.result;
    }
}

WorldAudioResult soundScapeUpdate()
{
    if (!soundScape.bindings) return WORLD_AUDIO_INVALID_BINDING;
    // PAL1CEBF0 returns at its actual nullable GameSession gate before
    // touching Race. Native validation must not make startup require one.
    if(soundScape.bindings->session && (!soundScape.bindings->race || !soundScape.bindings->race->services ||
        !soundScape.bindings->race->services->settings)) return WORLD_AUDIO_INVALID_BINDING;
    try { soundScape.Update(); return WORLD_AUDIO_OK; }
    catch (const AmbientAudioFailure& failure) { return failure.result; }
    catch (const std::exception&) { return WORLD_AUDIO_INVALID_BINDING; }
}

namespace {
WorldAudioResult RetireAmbientVoice(WorldAudioRequestQueue* queue,WorldAudioVoiceRef& voice)
{
    if(!voice.instance)return WORLD_AUDIO_OK;
    auto result=worldAudioValidateVoice(queue,voice);
    if(result==WORLD_AUDIO_STALE_REFERENCE) {voice={};return WORLD_AUDIO_OK;}
    if(result!=WORLD_AUDIO_OK)return result;
    result=worldAudioRetireVoice(queue,voice);
    if(result==WORLD_AUDIO_CAPACITY && queue->library) {
        // Allowed only when the real backend has closed and joined. An open
        // device rejects cancellation; no playback completion is fabricated.
        const auto cancelled=queue->library->CancelDeviceRequestsAfterClose();
        if(cancelled==WORLD_AUDIO_OK)result=worldAudioRetireVoice(queue,voice);
    }
    if(result==WORLD_AUDIO_OK || result==WORLD_AUDIO_STALE_REFERENCE)voice={};
    return result==WORLD_AUDIO_STALE_REFERENCE?WORLD_AUDIO_OK:result;
}
}

std::exception_ptr soundScapeShutdownResources(const hp2::TrackScenarioData& track) noexcept
{
    std::exception_ptr first;
    const auto attempt=[&](auto&& action) noexcept {
        try {action();}catch(...) {if(!first)first=std::current_exception();}
    };
    if(soundScape.resource && soundScape.resource!=&track.Authored()) {
        attempt([]{throw std::runtime_error("Soundscape retirement encountered a different track owner");});
        return first;
    }
    const auto retire=[&](WorldAudioVoiceRef& voice) {
        const auto result=RetireAmbientVoice(soundScape.bindings?soundScape.bindings->audio:nullptr,voice);
        if(result!=WORLD_AUDIO_OK)throw std::runtime_error("Soundscape voice retirement failed: "+
            std::to_string(static_cast<int>(result)));
    };
    for(auto& area:soundScape.looping) {
        attempt([&]{retire(area.voice);});
        area.name=nullptr;area.sampleName=nullptr;
        area.geometry.vertices.clear();area.geometry.triangles.clear();area.wasInside=0;
    }
    for(auto& area:soundScape.oneShots) {
        attempt([&]{retire(area.secondVoice);});
        // Shared bank samples belong to bRegion, not this resource view.
        area.sharedVoice={};area.name=nullptr;area.sampleName=nullptr;
        area.geometry.vertices.clear();area.geometry.triangles.clear();area.wasInside=0;
    }
    soundScape.weatherVoice={};soundScape.reverbAreas.clear();
    soundScape.currentReverbAreaIndex=-1;soundScape.loopingCount=0;soundScape.oneShotCount=0;
    soundScape.dataJustLoaded=0;soundScape.resource=nullptr;
    return first;
}

WorldAudioResult soundScapeDispose()
{
    if(soundScape.resource)return WORLD_AUDIO_INVALID_BINDING;
    if(!soundScape.bindings)return WORLD_AUDIO_OK;
    auto first=WORLD_AUDIO_OK;
    const auto retire=[&](WorldAudioVoiceRef& voice) {
        const auto result=RetireAmbientVoice(soundScape.bindings->audio,voice);
        if(first==WORLD_AUDIO_OK && result!=WORLD_AUDIO_OK)first=result;
    };
    for(auto& area:soundScape.looping)retire(area.voice);
    for(auto& area:soundScape.oneShots) {retire(area.secondVoice);area.sharedVoice={};}
    if(first==WORLD_AUDIO_OK)soundScape.bindings=nullptr;
    return first;
}

WorldAudioResult soundScapeMute(int32_t muted)
{
    if (!soundScape.bindings) return WORLD_AUDIO_INVALID_BINDING;
    try { soundScape.Mute(muted); return WORLD_AUDIO_OK; }
    catch (const AmbientAudioFailure& failure) { return failure.result; }
}

WorldAudioResult soundScapeUpdateWeather()
{
    if (!soundScape.bindings) return WORLD_AUDIO_INVALID_BINDING;
    try { soundScape.UpdateWeather(); return WORLD_AUDIO_OK; }
    catch (const AmbientAudioFailure& failure) { return failure.result; }
    catch (const std::exception&) { return WORLD_AUDIO_INVALID_BINDING; }
}

WorldAudioResult soundScapeUpdateReflectionReverb(CarMovement* first, CarMovement* second)
{
    if (!soundScape.bindings) return WORLD_AUDIO_INVALID_BINDING;
    try { soundScape.UpdateReflectionReverb({first, second}); return WORLD_AUDIO_OK; }
    catch (const AmbientAudioFailure& failure) { return failure.result; }
    catch (const std::exception&) { return WORLD_AUDIO_INVALID_BINDING; }
}

WorldAudioResult soundScapeSelectReverbArea(CarMovement* first, CarMovement* second,
    int32_t* selectedPlayer)
{
    if (!first || !selectedPlayer || !soundScape.resource) return WORLD_AUDIO_INVALID_BINDING;
    try {
        *selectedPlayer = soundScape.IsInsideReverbArea(first, second);
        return WORLD_AUDIO_OK;
    } catch (const std::exception&) {
        return WORLD_AUDIO_INVALID_BINDING;
    }
}

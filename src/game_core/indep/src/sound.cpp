#include <cstdlib>
#include "indep/inc/RaceCoordinator.hpp"
#include "FMenus.hpp"
#include "bware/inc/bWare.hpp"
#include "Sound3dSample.hpp"
#include "CarSounds.hpp"
#include "Player.hpp"
#include "GameFlow.hpp"
#include "PhysicsCar.hpp"
#include "indep/src/speech/mgr.hpp"
#include "bsound/include/bRegion.h"
#include "bsound/include/bJukebox.h"
#include "bsound/include/bSound.h"
#include "indep/src/speech/SEDWedge.hpp"
#include "DemoDisc.hpp"
#include "Fred.hpp"
#include "NativeAudioProfileValidation.hpp"
#include "Flags.hpp"
#include "Joylog.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bFile.hpp"
#include "SpeechNoise.hpp"
#include "Collision.hpp"
#include "Chopper.hpp"
#include "CarInfo.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/Car.hpp"
#include <thread>
#include <chrono>

extern "C" uint32_t platformVideoBlankCount(void);

// PAL1AF420. The optional collision owner is absent only before a World has
// registered impact voices; that is the original empty global instance list.
WorldAudioResult soundUnloadRegions(const CarSoundBindings& bindings,WorldImpactAudio* collisions)
{
    if (!bindings.globals)return WORLD_AUDIO_INVALID_BINDING;
    if (!bindings.globals->policy.audioSystemEnabled)return WORLD_AUDIO_OK;
    if (!bindings.audio || !bindings.audio->library ||
        !bindings.audio->positionalSamples)return WORLD_AUDIO_INVALID_BINDING;
    if (collisions && collisions->registered) {
        const auto result=collisionAudioUnregister(collisions);
        if (result!=WORLD_AUDIO_OK)return result;
    }
    auto result=SpeechNoise::getInstance()->UnLoadSamples();
    if (result!=WORLD_AUDIO_OK)return result;
    if (worldSpeechFlush(worldSpeechGetInstance())!=WORLD_SPEECH_OK)
        return WORLD_AUDIO_INVALID_BINDING;
    result=bindings.audio->positionalSamples->Close();
    if (result!=WORLD_AUDIO_OK)return result;
    for (std::size_t index=soundDriverState.regions.size();index>0;--index) {
        auto& region=soundDriverState.regions[index-1];
        if (!region.name[0])continue;
        result=bindings.audio->library->UnloadRegion(region.name.data());
        if (result!=WORLD_AUDIO_OK)return result;
        region.name[0]=0;region.loaded=0;
    }
    soundDriverState.frontendRegionCache=nullptr;
    soundDriverState.delayedJukeboxSeconds=0.0f;
    // PAL1AF420 preserves GP-4A60. Only the real next LoadRegions and
    // Mute/MuteReplay boundaries change the world-audio suppression gate.
    return WORLD_AUDIO_OK;
}

// Final PAL static values for the two independent sound globals. Native
// audio borrowers use this one record; source variable names are not claimed.
WorldAudioRuntimeState worldAudioRuntime={0,0,1};
SoundDriverState soundDriverState={0,1.0f,1.0f,-1.0f};
cJukeboxPlayer soundJukeboxPlayer;

namespace {

int32_t soundJukeboxFavouriteTrack(const RaceParameters& parameters)
{
    if (soundJukeboxPlayer.category==5) {
        if (!soundDriverState.favouriteEligible[0]) return -1;
        soundDriverState.favouriteEligible[0]=0;
        return 5;
    }
    if (soundJukeboxPlayer.category!=4) return -1;
    const size_t index=parameters.policeDriverCount>0 ? 1u : 2u;
    if (!soundDriverState.favouriteEligible[index]) return -1;
    soundDriverState.favouriteEligible[index]=0;
    return index==1 ? 14 : 7;
}

} // namespace

WorldAudioResult soundJukeboxStart(const CarSoundBindings& bindings,
    const hp2::FrontendPlayerManager& frontend,const RaceParameters& parameters,
    const hp2::ApplicationFrontendFlow& flow,bool unused)
{
    (void)unused; // Both complete released callers pass a bool ignored by PAL.
    if (!bindings.globals) return WORLD_AUDIO_INVALID_BINDING;
    if (!bindings.globals->policy.audioSystemEnabled || !applicationLaunchOptions.musicEnabled)
        return WORLD_AUDIO_OK;
    if (!bindings.audio || !bindings.volumes || soundJukeboxPlayer.audio!=bindings.audio ||
        !soundJukeboxPlayer.IsCartridgeLoaded()) return WORLD_AUDIO_INVALID_BINDING;
    auto result=soundJukeboxPlayer.Update();
    if (result!=WORLD_AUDIO_OK) return result;
    try {
        if (soundJukeboxPlayer.state==2 || soundJukeboxPlayer.IsFading()) return WORLD_AUDIO_OK;
        const int32_t favourite=soundJukeboxFavouriteTrack(parameters);
        bool selectNext=true;
        if (favourite>=0) {
            // Source reads a fixed row even if the current UI count is lower.
            const auto& row=frontend.musicSettings.rows.at(static_cast<size_t>(favourite));
            const uint32_t bit=uint32_t{1}<<(static_cast<uint32_t>(row.genreBit)&31);
            if ((static_cast<uint32_t>(frontend.musicSettings.enabledGenres)&bit) && row.status) {
                result=soundJukeboxPlayer.Start(static_cast<uint32_t>(static_cast<int32_t>(row.group)),
                    static_cast<uint32_t>(static_cast<int32_t>(row.track)),1);
                if (result!=WORLD_AUDIO_OK) return result;
                selectNext=false;
            }
        }
        if (selectNext) {
            if (soundDriverState.initialJukeboxShuffle) {
                const auto& group=soundJukeboxPlayer.groups.at(soundJukeboxPlayer.currentGroup);
                if (group.tunes.empty()) return WORLD_AUDIO_INVALID_BINDING;
                uint32_t seed=platformVideoBlankCount();
                const uint32_t count=vectorRandomBounded(static_cast<uint32_t>(group.tunes.size()),&seed);
                // The delay slot decrements the source loop counter before
                // each Advance returns; every positive count still calls it.
                // Start(1) performs the following additional advance.
                for (uint32_t remaining=count;remaining>0;--remaining) {
                    result=soundJukeboxPlayer.Advance();
                    if (result!=WORLD_AUDIO_OK) return result;
                }
                soundDriverState.initialJukeboxShuffle=0;
            }
            result=soundJukeboxPlayer.Start(1);
            if (result!=WORLD_AUDIO_OK) return result;
        }
        // These are two ordered released decisions, including both explicit
        // unpause branches. Do not collapse them into a guessed music flag.
        const bool pauseInGame=soundGetCategoryVolume(*bindings.volumes,4)==0.0f && flow.currentState!=3;
        result=soundJukeboxPlayer.Pause(pauseInGame ? 1 : 0);
        if (result!=WORLD_AUDIO_OK) return result;
        const bool pauseFrontend=soundGetCategoryVolume(*bindings.volumes,5)==0.0f && flow.currentState==3;
        return soundJukeboxPlayer.Pause(pauseFrontend ? 1 : 0);
    } catch (const std::exception&) {
        return bindings.audio->result==WORLD_AUDIO_OK ? WORLD_AUDIO_INVALID_BINDING : bindings.audio->result;
    }
}

WorldAudioResult soundJukeboxStop(const CarSoundBindings& bindings)
{
    // PAL1AEC90: the music enable cell alone gates the original Stop.
    if (!applicationLaunchOptions.musicEnabled) return WORLD_AUDIO_OK;
    if (!bindings.audio || soundJukeboxPlayer.audio!=bindings.audio)
        return WORLD_AUDIO_INVALID_BINDING;
    return soundJukeboxPlayer.Stop();
}

// Full PAL1AEC88..1AEC90: one live sound-module timer and the real Stop
// tail. The timer is consumed by soundUpdate, never by a second RC clock.
WorldAudioResult soundJukeboxDelayedStart(const CarSoundBindings& bindings,float seconds)
{
    soundDriverState.delayedJukeboxSeconds=seconds;
    return soundJukeboxStop(bindings);
}

// PAL1AECB0. Gameplay suppresses the chyron when the actual music category
// volume is zero; frontend state3 only consults the same jukebox player.
bool soundJukeboxIsPlaying(const CarSoundBindings& bindings,
    const hp2::ApplicationFrontendFlow& flow)
{
    if(!soundJukeboxPlayer.IsPlaying())return false;
    if(flow.currentState==3)return true;
    hp2::Require(bindings.volumes,"Jukebox status has no actual music-volume owner");
    return soundGetCategoryVolume(*bindings.volumes,4)>0.0f;
}
std::int32_t soundJukeboxGetCurrentTrack()
{
    return static_cast<std::int32_t>(soundJukeboxPlayer.currentTrack);
}
namespace {
// PAL1AED90: the original one retained metadata split. String allocation is
// a host ownership change; the cache key, separators and sixteen fields are
// the released sound owner, not a frontend mirror of the music cartridge.
const char* JukeboxMetadataPart(const CarSoundBindings& bindings,std::int32_t group,
    std::int32_t track,std::size_t part)
{
    hp2::Require(bindings.globals,"Jukebox metadata has no actual audio policy");
    if(!bindings.globals->policy.audioSystemEnabled)return nullptr;
    if(group==-1)group=static_cast<std::int32_t>(soundJukeboxPlayer.currentGroup);
    if(track==-1)track=static_cast<std::int32_t>(soundJukeboxPlayer.currentTrack);
    const auto key=(static_cast<std::uint32_t>(group)<<16)+static_cast<std::uint32_t>(track);
    auto& state=soundDriverState;
    if(key!=state.jukeboxMetadataKey) {
        const auto* name=soundJukeboxPlayer.GetTuneName(static_cast<std::uint32_t>(group),static_cast<std::uint32_t>(track));
        hp2::Require(name && std::strlen(name)<256,"Jukebox metadata exceeds its actual256-byte record");
        for(auto& text:state.jukeboxMetadata)text.clear();
        state.jukeboxMetadataCount=1;state.jukeboxMetadataKey=key;
        const char* first=name;
        for(const char* cursor=name;;++cursor) {
            if(*cursor!='-' && *cursor)continue;
            state.jukeboxMetadata[state.jukeboxMetadataCount-1].assign(first,cursor);
            if(!*cursor || !cursor[1])break;
            hp2::Require(state.jukeboxMetadataCount<state.jukeboxMetadata.size(),
                "Jukebox metadata has more than the source sixteen fields");
            ++state.jukeboxMetadataCount;first=cursor+1;
        }
    }
    return part<state.jukeboxMetadataCount?state.jukeboxMetadata[part].c_str():nullptr;
}
}
const char* soundJukeboxGetTrackName(const CarSoundBindings& bindings,std::int32_t group,std::int32_t track)
{return JukeboxMetadataPart(bindings,group,track,1);}
const char* soundJukeboxGetTrackArtist(const CarSoundBindings& bindings,std::int32_t group,std::int32_t track)
{return JukeboxMetadataPart(bindings,group,track,0);}
const char* soundJukeboxGetTrackLabel(const CarSoundBindings& bindings,std::int32_t group,std::int32_t track)
{return JukeboxMetadataPart(bindings,group,track,2);}
const char* soundJukeboxGetTrackGenre(const CarSoundBindings& bindings,std::int32_t group,std::int32_t track)
{return JukeboxMetadataPart(bindings,group,track,3);}
std::int32_t soundJukeboxGetTrackStatus(const CarSoundBindings& bindings,std::int32_t group,std::int32_t track)
{
    const auto* value=JukeboxMetadataPart(bindings,group,track,4);
    return value && *value ? static_cast<std::int32_t>(std::strtol(value,nullptr,10)) : 0;
}

// PAL1AF6F8: walk the actual World intrusive car list in its current order.
// A source car without a sound child is skipped; missing native owner links
// are rejected instead of being interpreted as a silent car.
WorldAudioResult soundMuteCarSounds(const WorldSimulation* simulation,bool muted)
{
    if (!simulation || !simulation->world)return WORLD_AUDIO_INVALID_BINDING;
    auto* world=simulation->world;
    for (Car* car=world->carList.next;car!=&world->carList;car=car->next) {
        if (!car || !car->movement || !simulation->frames ||
            car->tableIndex>=simulation->frameCount)return WORLD_AUDIO_INVALID_BINDING;
        auto* movement=static_cast<CarMovement*>(car->movement);
        auto* frame=simulation->frames[car->tableIndex];
        if (!frame || movement->frame!=frame || frame->movement!=movement ||
            movement->bindings.owner!=car)return WORLD_AUDIO_INVALID_BINDING;
        if (frame->environment.sound) {
            const auto result=frame->environment.sound->Pause(muted);
            if (result!=WORLD_AUDIO_OK)return result;
        }
    }
    return WORLD_AUDIO_OK;
}

// PAL1AF850..1AF940. This is the game's ordered owner-level mute, not a
// device-wide suspension. In particular music with a nonpositive category
// volume remains paused even on a request to unmute.
WorldAudioResult soundMute(const CarSoundBindings& bindings,
    const WorldSimulation* simulation,bool muted)
{
    if (!bindings.globals)return WORLD_AUDIO_INVALID_BINDING;
    if (!bindings.globals->policy.audioSystemEnabled)return WORLD_AUDIO_OK;
    soundDriverState.suppressWorldAudio=muted;
    if (!bindings.audio || !bindings.audio->library || !simulation ||
        !simulation->world || !simulation->collisions ||
        !simulation->collisions->contactAudio || !bindings.audio->positionalSamples)
        return WORLD_AUDIO_INVALID_BINDING;
    if (applicationLaunchOptions.musicEnabled) {
        if (soundJukeboxPlayer.audio!=bindings.audio)return WORLD_AUDIO_INVALID_BINDING;
        const float volume=bindings.audio->library->GetCategoryVolume(soundJukeboxPlayer.category);
        const auto result=soundJukeboxPlayer.Pause(muted || !(0.0f<volume));
        if (result!=WORLD_AUDIO_OK)return result;
    }
    if (soundDriverState.speechEnabled) {
        auto* manager=worldSpeechGetInstance();
        if (!manager)return WORLD_AUDIO_CAPACITY;
        if (manager->session!=bindings.session)return WORLD_AUDIO_INVALID_BINDING;
        const auto speech=worldSpeechMute(manager,muted);
        if (speech!=WORLD_SPEECH_OK)return WORLD_AUDIO_BACKEND_FAILURE;
        const auto result=SpeechNoise::getInstance()->Mute(muted);
        if (result!=WORLD_AUDIO_OK)return result;
    }
    auto result=soundScapeMute(muted);
    if (result!=WORLD_AUDIO_OK)return result;
    result=soundMuteCarSounds(simulation,muted);
    if (result!=WORLD_AUDIO_OK)return result;
    result=bindings.audio->positionalSamples->Pause(muted);
    if (result!=WORLD_AUDIO_OK)return result;
    result=collisionAudioPause(simulation->collisions->contactAudio,muted);
    if (result!=WORLD_AUDIO_OK)return result;
    if (simulation->chopper && !chopperMuteSound(simulation->chopper,muted))
        return WORLD_AUDIO_BACKEND_FAILURE;
    return WORLD_AUDIO_OK;
}

// PAL1AF940..1AF9A0. The replay path has a different order and deliberately
// leaves music, speech and speech noise untouched.
WorldAudioResult soundMuteReplay(const CarSoundBindings& bindings,
    const WorldSimulation* simulation,bool muted)
{
    if (!bindings.globals)return WORLD_AUDIO_INVALID_BINDING;
    if (!bindings.globals->policy.audioSystemEnabled)return WORLD_AUDIO_OK;
    soundDriverState.suppressWorldAudio=muted;
    if (!bindings.audio || !bindings.audio->positionalSamples || !simulation ||
        !simulation->collisions || !simulation->collisions->contactAudio)
        return WORLD_AUDIO_INVALID_BINDING;
    auto result=soundMuteCarSounds(simulation,muted);
    if (result!=WORLD_AUDIO_OK)return result;
    result=bindings.audio->positionalSamples->Pause(muted);
    if (result!=WORLD_AUDIO_OK)return result;
    result=collisionAudioPause(simulation->collisions->contactAudio,muted);
    if (result!=WORLD_AUDIO_OK)return result;
    result=soundScapeMute(muted);
    if (result!=WORLD_AUDIO_OK)return result;
    if (simulation->chopper && !chopperMuteSound(simulation->chopper,muted))
        return WORLD_AUDIO_BACKEND_FAILURE;
    return WORLD_AUDIO_OK;
}

WorldAudioResult soundJukeboxSetSelection(const CarSoundBindings& bindings,
    const hp2::FrontendPlayerManager& frontend,
    const RaceParameters& parameters,int32_t selection)
{
    if (!applicationLaunchOptions.musicEnabled) return WORLD_AUDIO_OK;
    if (!bindings.audio || (soundJukeboxPlayer.currentSample.instance &&
        soundJukeboxPlayer.audio!=bindings.audio)) return WORLD_AUDIO_INVALID_BINDING;
    // Reject every invalid row before SetCategory or SetTuneOn/Off can
    // mutate the actual music owner. Domain and catalogue checks are shared
    // with the non-mutating native profile validation pass.
    if ((selection!=4 && selection!=5) || soundJukeboxPlayer.audio!=bindings.audio ||
        !hp2::nativeAudioProfileValid(frontend.musicSettings,&soundJukeboxPlayer))
        return WORLD_AUDIO_INVALID_BINDING;
    const auto category=soundJukeboxPlayer.SetCategory(static_cast<uint32_t>(selection));
    if (category!=WORLD_AUDIO_OK) return category;

    int32_t column=-1;
    if (selection==4) column=parameters.policeDriverCount>0 ? 2 : 1;
    else if (selection==5) column=0;
    const auto& settings=frontend.musicSettings;
    if (settings.count>static_cast<int32_t>(settings.rows.size())) return WORLD_AUDIO_INVALID_BINDING;
    // Exact final PAL table2B6300. Neither beta policy nor a guessed genre
    // default supplies the selected track set.
    static constexpr uint8_t allowed[18]={0,0,0,0,1,0,0,0,1,1,0,0,1,1,0,1,1,1};
    for (int32_t index=0;index<settings.count;++index) {
        const auto& row=settings.rows[static_cast<size_t>(index)];
        const int32_t cell=static_cast<int32_t>(row.status)*3+column;
        if (cell<0 || cell>=18) return WORLD_AUDIO_INVALID_BINDING;
        const uint32_t bit=uint32_t{1}<<static_cast<uint32_t>(row.genreBit);
        const int32_t included=allowed[cell] && (static_cast<uint32_t>(settings.enabledGenres)&bit);
        const auto result=worldAudioSetJukeboxTrackIncluded(bindings.audio,row.group,row.track,included);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    return WORLD_AUDIO_OK;
}

float soundGetCategoryVolume(const hp2::SoundVolumeSettings& settings,
    int32_t category)
{
    switch (category) {
    case 0: return settings.speechRegionVolumes[0];
    case 1: return settings.speechRegionVolumes[1];
    case 2: return settings.speechRegionVolumes[2];
    case 5: return settings.speechRegionVolumes[3];
    case 4: return settings.speechRegionVolumes[4];
    default: return 0.0f;
    }
}

int32_t soundIsNisPlaying(const CarFramePlayerRegistry& players)
{
    hp2::Require(players.count >= 0 && players.count <= 2,
        "Sound NIS query has no valid creation-order Player registry");
    for (int32_t index = 0; index < players.count; ++index) {
        const auto* player = players.players[index];
        if (player && carFramePlayerReplayActive(player)) return 1;
    }
    return 0;
}

WorldAudioResult soundScaleCategoryVolumes(const CarSoundBindings& bindings,
    int32_t includeSpeech, float scale)
{
    // The source cache suppresses even a change to includeSpeech when the
    // scale itself is unchanged. Preserve that order and exact comparison.
    if (scale == soundDriverState.lastCategoryScale) {
        soundDriverState.lastCategoryScale = scale;
        return WORLD_AUDIO_OK;
    }
    if (!bindings.audio || !bindings.manager || !bindings.volumes ||
        !bindings.manager->channelControls ||
        bindings.manager->channelControls != bindings.manager->channelVolumes ||
        bindings.manager->channelVolumeCount <= 6)
        return WORLD_AUDIO_INVALID_BINDING;
    if (worldAudioAvailableRequests(bindings.audio) < 6)
        return WORLD_AUDIO_CAPACITY;

    static constexpr int32_t categories[5] = {0,1,2,5,4};
    for (size_t index = 0; index < 5; ++index) {
        const float volume = bindings.volumes->speechRegionVolumes[index] * scale;
        const auto result = worldAudioSetChannelVolume(bindings.audio,
            bindings.manager, categories[index], volume);
        if (result != WORLD_AUDIO_OK) return result;
    }
    const float speechVolume = includeSpeech ?
        bindings.volumes->speechCategorySixVolume * scale :
        bindings.volumes->speechCategorySixVolume;
    const auto result = worldAudioSetChannelVolume(bindings.audio,
        bindings.manager, 6, speechVolume);
    if (result == WORLD_AUDIO_OK) soundDriverState.lastCategoryScale = scale;
    return result;
}

WorldAudioResult soundUpdateVolumeMuffler(const CarSoundBindings& bindings,
    const hp2::ApplicationFrontendFlow& flow)
{
    const auto* players = bindings.spatial.players;
    if (!players || players->count < 0 || players->count > 2)
        return WORLD_AUDIO_INVALID_BINDING;
    bool transition = false;
    float transitionStrength = -1.0f;
    for (int32_t index = 0; index < players->count; ++index) {
        const auto* player = players->players[index];
        if (!player) return WORLD_AUDIO_INVALID_BINDING;
        transition = transition ||
            (player->transitionAudio != 0 && player->movement.steeringFeedbackBusy != 0);
        if (transition && transitionStrength == -1.0f)
            transitionStrength = player->transitionAudioStrength;
    }

    float maximumChange = 0.10000000149011612f;
    if (!soundDriverState.suppressWorldAudio && bindings.session &&
        bindings.session->state == 7) {
        // This is 0.5 in the final PAL; beta used 0.550000012.
        soundDriverState.desiredMuffler = 0.5f;
    } else if (!soundDriverState.suppressWorldAudio && transition) {
        maximumChange = 0.05000000074505806f;
        soundDriverState.desiredMuffler = transitionStrength;
    } else if (!soundDriverState.suppressWorldAudio) {
        const auto* speech = worldSpeechGetInstance();
        if (!speech) return WORLD_AUDIO_CAPACITY;
        if (speech->speaking) {
            soundDriverState.desiredMuffler = flow.currentState == 3 ?
                0.30000001192092896f : 0.949999988079071f;
        } else {
            maximumChange = 0.20000000298023224f;
            soundDriverState.desiredMuffler = 1.0f;
        }
    } else {
        maximumChange = 0.20000000298023224f;
        soundDriverState.desiredMuffler = 1.0f;
    }

    const float updated = physicsClampDelta(soundDriverState.currentMuffler,
        soundDriverState.desiredMuffler, maximumChange);
    if (updated == soundDriverState.currentMuffler) return WORLD_AUDIO_OK;
    soundDriverState.currentMuffler = updated;
    return soundScaleCategoryVolumes(bindings, 0, updated);
}

WorldAudioResult soundLoadRegion(const CarSoundBindings& bindings,const char* name)
{
    if (!bindings.audio || !bindings.files || !name || std::strlen(name)>32)
        return WORLD_AUDIO_INVALID_BINDING;
    const std::string path=std::string("sound\\")+name+".msh";
    try {
        // The original checks file existence before asking the SDK to load.
        // Retaining/decoding the actual bank is the backend registry's job.
        (void)bindings.files->OpenPhysical(path);
    } catch (const std::exception&) {
        // PAL displays Cannot Find Sound Region. Missing resources do not
        // become a loaded bank or a successful frontend completion.
        return WORLD_AUDIO_INVALID_BINDING;
    }
    return worldAudioLoadRegion(bindings.audio,name);
}

WorldAudioResult soundLoadRegionsFrontend(const CarSoundBindings& bindings,
    const hp2::FrontendPlayerManager& frontend,
    const RaceParameters& parameters,hp2::ApplicationFrontendFlow* completionFlow)
{
    if (!bindings.globals) return WORLD_AUDIO_INVALID_BINDING;
    soundDriverState.completion=completionFlow ? SoundRegionCompletion::FrontendReady :
        SoundRegionCompletion::None;
    soundDriverState.completionFlow=completionFlow;
    soundDriverState.completionBindings=nullptr;
    worldAudioRuntime.disabled=1;
    soundDriverState.loadStarted=std::chrono::steady_clock::now();
    if (!bindings.globals->policy.audioSystemEnabled) return WORLD_AUDIO_OK;
    if (!bindings.audio) return WORLD_AUDIO_INVALID_BINDING;
    bindings.audio->inGameBank=nullptr;
    soundDriverState.frontendRegionCache=nullptr;
    soundDriverState.delayedJukeboxSeconds=0.0f;
    const auto selection=soundJukeboxSetSelection(bindings,frontend,parameters,5);
    if (selection!=WORLD_AUDIO_OK) return selection;
    for (auto& record:soundDriverState.regions) {
        record.name[0]=0;
        record.loaded=0;
    }
    static constexpr char name[]="FrontEnd";
    std::memcpy(soundDriverState.regions[0].name.data(),name,sizeof(name));
    const auto result=soundLoadRegion(bindings,soundDriverState.regions[0].name.data());
    soundDriverState.suppressWorldAudio=0;
    return result;
}

// PAL1AE798..1AE99C. Regions belong to the sound driver; track and selected
// engine-bank names come from the actual released resource records. The
// original audio-disabled gate is retained without fabricating loaded banks.
WorldAudioResult soundLoadRegionsInGame(const CarSoundBindings& bindings,
    const hp2::FrontendPlayerManager& frontend,const RaceParameters& parameters,
    WorldImpactAudio* collisions,CollisionSystem* collisionSystem,
    hp2::ApplicationFrontendFlow& flow,const hp2::GameFlowBindings& gameFlow)
{
    if (!bindings.globals)return WORLD_AUDIO_INVALID_BINDING;
    soundDriverState.completion=SoundRegionCompletion::TrackReady;
    soundDriverState.completionFlow=&flow;soundDriverState.completionBindings=&gameFlow;
    worldAudioRuntime.disabled=1;
    soundDriverState.loadStarted=std::chrono::steady_clock::now();
    if (!bindings.globals->policy.audioSystemEnabled)return WORLD_AUDIO_OK;
    if (!bindings.audio || !bindings.cars || !bindings.audio->positionalSamples ||
        !collisions || !collisionSystem)return WORLD_AUDIO_INVALID_BINDING;
    bindings.audio->inGameBank=nullptr;
    soundDriverState.frontendRegionCache=nullptr;
    soundDriverState.delayedJukeboxSeconds=0.0f;
    soundDriverState.suppressWorldAudio=0;
    auto result=soundJukeboxSetSelection(bindings,frontend,parameters,4);
    if (result!=WORLD_AUDIO_OK)return result;
    for (auto& record:soundDriverState.regions) {record.name[0]=0;record.loaded=0;}
    const auto assign=[](SoundRegionRecord& record,const char* name) {
        const auto length=std::strlen(name);
        if (length>=record.name.size())return false;
        std::memcpy(record.name.data(),name,length+1);return true;
    };
    if (!assign(soundDriverState.regions[0],"InGame"))return WORLD_AUDIO_INVALID_BINDING;
    std::size_t next=1;
    if (bindings.trackInfo) {
        if (!std::memchr(bindings.trackInfo->tireBankName,0,sizeof(bindings.trackInfo->tireBankName)) ||
            !assign(soundDriverState.regions[next++],bindings.trackInfo->tireBankName))return WORLD_AUDIO_INVALID_BINDING;
    }
    for (int32_t number=0;number<2;++number) {
        const RaceDriverInfo* driver=nullptr;
        for (int32_t i=0;i<parameters.driverCount;++i)
            if (parameters.drivers[i].driverNumber==number) {driver=&parameters.drivers[i];break;}
        if (!driver || driver->eliminated>0)continue;
        const auto& name=bindings.cars->Car(static_cast<std::size_t>(driver->appearance.modelCode)).engineSound;
        if (name.empty())continue;
        bool found=false;
        for (const auto& record:soundDriverState.regions)
            if (name==record.name.data()) {found=true;break;}
        if (!found) {
            if (next>=soundDriverState.regions.size() || !assign(soundDriverState.regions[next++],name.c_str()))
                return WORLD_AUDIO_INVALID_BINDING;
        }
    }
    result=soundLoadRegion(bindings,soundDriverState.regions[0].name.data());
    if (result!=WORLD_AUDIO_OK)return result;
    result=collisionAudioRegister(collisions,collisionSystem,&bindings.spatial,&parameters);
    if (result!=WORLD_AUDIO_OK)return result;
    SpeechNoise::getInstance()->RequestSamples();
    return bindings.audio->positionalSamples->Initialize();
}

WorldAudioResult soundDriverCallback(const CarSoundBindings& bindings,
    uint32_t event,const bRegion* region)
{
    if (event!=2) return WORLD_AUDIO_OK;
    if (!region || !bindings.manager || !bindings.audio)
        return WORLD_AUDIO_INVALID_BINDING;
    // This existing manager field is the legacy native name for the SDK's
    // CommonRegion borrow. A separately loaded FrontEnd row is not that bank.
    if (&region->Bank()==bindings.manager->frontendBank)
        soundDriverState.commonRegionLoaded=1;
    for (auto& record:soundDriverState.regions) {
        if (std::strcmp(region->GetName(),record.name.data())==0) record.loaded=1;
    }
    for (const auto& record:soundDriverState.regions) {
        if (record.name[0] && !record.loaded) {
            const auto result=worldAudioSetRegionPath(bindings.audio,"sound\\");
            if (result!=WORLD_AUDIO_OK) return result;
            return soundLoadRegion(bindings,record.name.data());
        }
    }
    return WORLD_AUDIO_OK;
}

int32_t soundRealRegionLoadingComplete(const CarSoundBindings& bindings)
{
    hp2::Require(bindings.globals,"Sound region query has no live audio policy");
    if (!bindings.globals->policy.audioSystemEnabled) return 1;
    for (const auto& record:soundDriverState.regions)
        if (record.name[0] && !record.loaded) return 0;
    return 1;
}

int32_t soundRegionLoadingInProgress()
{
    return worldAudioRuntime.disabled;
}

int32_t soundRegionLoadingComplete()
{
    return soundRegionLoadingInProgress()==0;
}

WorldAudioResult soundServiceRegionLoading(const CarSoundBindings& bindings,
    ReplayIntegerLog* joylog)
{
    if (!soundRegionLoadingInProgress()) return WORLD_AUDIO_OK;
    if (!joylog || !bindings.audio) return WORLD_AUDIO_INVALID_BINDING;
    int32_t ready=0;
    if (joylog->replaying) {
        ready=static_cast<int32_t>(replayIntegerLogRead(joylog,1,5));
        if (ready) {
            // The recorded source wait cannot return before the real banks
            // exist. Native async completions require the owner-thread device
            // pump AND bSound publication; waiting for another presentation
            // here starves both and cannot make forward progress.
            while (!soundRealRegionLoadingComplete(bindings)) {
                if (!bindings.audio->library || !bindings.servicePhysicalAudio)
                    return WORLD_AUDIO_INVALID_BINDING;
                auto result=bindings.servicePhysicalAudio(bindings.physicalProgressOwner);
                if (result!=WORLD_AUDIO_OK) return result;
                // A pending global bank load may follow an unfinished
                // stream. Keep pumping the real work, without duplicating
                // a driver update that is still queued or in flight.
                bool updatePending=false;
                for (size_t index=0;index<bindings.audio->capacity;++index) {
                    const auto& request=bindings.audio->requests[index];
                    if (request.kind==WORLD_AUDIO_DRIVER_UPDATE &&
                        (request.deliveryState==WORLD_AUDIO_REQUEST_QUEUED ||
                         request.deliveryState==WORLD_AUDIO_REQUEST_INFLIGHT)) {
                        updatePending=true;break;
                    }
                }
                if (!updatePending && worldAudioAvailableRequests(bindings.audio)) {
                    result=bindings.audio->library->Update();
                    if (result!=WORLD_AUDIO_OK) return result;
                }
                std::this_thread::yield();
            }
        }
    } else {
        ready=soundRealRegionLoadingComplete(bindings);
        replayIntegerLogWrite(joylog,static_cast<uint32_t>(ready),1,5);
    }
    const auto elapsed=std::chrono::duration<float>(
        std::chrono::steady_clock::now()-soundDriverState.loadStarted).count();
    if (20.0f<elapsed) {
        // PAL's console background-colour diagnostic is retired. Its
        // logical timeout/completion behavior remains; no bank is marked
        // loaded and no hardware/PCM success is fabricated.
        ready=1;
    }
    if (ready) {
        // PAL1AF3FC is the delay slot of the optional callback test: this
        // gate clears on every completed load, including completion=None.
        worldAudioRuntime.disabled=0;
        if (soundDriverState.completion!=SoundRegionCompletion::None) {
            const auto completion=soundDriverState.completion;
            auto* flow=soundDriverState.completionFlow;
            const auto* gameFlow=soundDriverState.completionBindings;
            if (!flow || (completion==SoundRegionCompletion::TrackReady && !gameFlow))
                return WORLD_AUDIO_INVALID_BINDING;
            soundDriverState.completion=SoundRegionCompletion::None;
            soundDriverState.completionFlow=nullptr;soundDriverState.completionBindings=nullptr;
            if (completion==SoundRegionCompletion::FrontendReady)
                hp2::GameFlowLoadingFrontEndPart2(*flow);
            else hp2::GameFlowLoadingTrackSpecificPart2(*flow,*gameFlow);
        }
    }
    return WORLD_AUDIO_OK;
}

// Complete PAL1AF4C0..1AF634 outer ordering. All bindings are borrowed from
// their actual owners; the only new native field is the SDK ticker's host
// steady-clock representation. No phase silently succeeds for a missing
// dependency, and no device state is inferred from submitting an update.
WorldAudioResult soundUpdate(const CarSoundBindings& bindings,
    const WorldSimulation* simulation,WorldStreamAudio& stream,
    const hp2::FrontendPlayerManager& frontend,const RaceParameters& parameters,
    const hp2::ApplicationFrontendFlow& flow,ReplayIntegerLog* joylog,
    const bVoiceStatusSnapshot* observedDevice)
{
    auto result=soundServiceRegionLoading(bindings,joylog);
    if (result!=WORLD_AUDIO_OK)return result;
    if (!bindings.globals)return WORLD_AUDIO_INVALID_BINDING;
    if (!bindings.globals->policy.audioSystemEnabled)return WORLD_AUDIO_OK;
    const auto now=std::chrono::steady_clock::now();
    const float elapsedMilliseconds=std::chrono::duration<float,std::milli>(
        now-soundDriverState.lastUpdate).count();
    if (!(8.0f<elapsedMilliseconds))return WORLD_AUDIO_OK;
    soundDriverState.lastUpdate=std::chrono::steady_clock::now();
    if (!bindings.audio || !bindings.audio->library || !bindings.clock)
        return WORLD_AUDIO_INVALID_BINDING;
    auto* speech=worldSpeechGetInstance();
    if (!speech)return WORLD_AUDIO_CAPACITY;
    if (speech->session!=bindings.session ||
        (speech->streamedAudio && speech->streamedAudio!=&stream))return WORLD_AUDIO_INVALID_BINDING;
    const auto spoken=worldSpeechUpdate(speech);
    if (spoken!=WORLD_SPEECH_OK && spoken!=WORLD_SPEECH_SUPPRESSED)
        return spoken==WORLD_SPEECH_ALLOCATION_FAILURE ? WORLD_AUDIO_CAPACITY:WORLD_AUDIO_BACKEND_FAILURE;
    const auto sentence=worldStreamAudioUpdate(&stream);
    if (sentence!=WORLD_STREAM_AUDIO_OK)return WORLD_AUDIO_BACKEND_FAILURE;
    result=soundJukeboxPlayer.Update();
    if (result!=WORLD_AUDIO_OK)return result;
    if (!hp2::IsGameFlowPaused(bindings.session) && flow.currentState!=3 &&
        !soundDriverState.suppressWorldAudio) {
        if (!simulation || simulation->clock!=bindings.clock || !bindings.audio->positionalSamples)
            return WORLD_AUDIO_INVALID_BINDING;
        // CarSound::UpdateAll18EA28 and collision sound1E51B8 have real
        // absent-World branches during startup/loading. The native service
        // record persists then; a track-only collision binding need not.
        if(simulation->world && (!simulation->collisions || !simulation->collisions->contactAudio))
            return WORLD_AUDIO_INVALID_BINDING;
        result=carSoundUpdateAll(simulation);
        if (result!=WORLD_AUDIO_OK)return result;
        if (0.0f<soundDriverState.delayedJukeboxSeconds)
            soundDriverState.delayedJukeboxSeconds-=bindings.clock->worldTimeElapsed;
        if (soundDriverState.delayedJukeboxSeconds<0.0f) {
            result=soundJukeboxStart(bindings,frontend,parameters,flow,false);
            if (result!=WORLD_AUDIO_OK)return result;
            soundDriverState.delayedJukeboxSeconds=0.0f;
        }
        if(simulation->world) {
            result=collisionAudioUpdate(simulation->collisions->contactAudio);
            if (result!=WORLD_AUDIO_OK)return result;
        }
        result=bindings.audio->positionalSamples->Update();
        if (result!=WORLD_AUDIO_OK)return result;
        result=soundScapeUpdate();
        if (result!=WORLD_AUDIO_OK)return result;
        result=SpeechNoise::getInstance()->Update();
        if (result!=WORLD_AUDIO_OK)return result;
        if (simulation->chopper && !chopperUpdateSound(simulation->chopper))
            return WORLD_AUDIO_BACKEND_FAILURE;
    }
    result=bindings.audio->library->Update();
    if (result!=WORLD_AUDIO_OK)return result;
    result=soundUpdateVolumeMuffler(bindings,flow);
    if (result!=WORLD_AUDIO_OK)return result;

    if (soundDriverState.printVoicesRequested) {
        if (!observedDevice)return WORLD_AUDIO_INVALID_BINDING;
        result=bindings.audio->library->Voices().PrintVoiceUsage(*observedDevice);
        if (result!=WORLD_AUDIO_OK)return result;
        soundDriverState.printVoicesRequested=0;
    }
    return WORLD_AUDIO_OK;
}

namespace hp2 {

float GetVolumeBalance(const SoundVolumeSettings& settings)
{
    return settings.balance;
}

float GetVolumeDefault(const SoundVolumeSettings& settings,std::int32_t channel)
{
    if (channel>=0 && channel<6) return settings.defaults[static_cast<std::size_t>(channel)];
    if (channel==6) return 0.75f;
    return -1.0f;
}

void SaveVolumesForSpeech(FlailerSoundState& state)
{
    Require(state.manager && state.volumes && state.manager->channelVolumes &&
        state.manager->channelVolumeCount>6,
        "Speech-volume refresh has no persistent settings or seven live mixer categories");
    static constexpr std::size_t categories[5]={0,1,2,5,4};
    for (std::size_t index=0;index<5;++index)
        state.volumes->speechRegionVolumes[index]=state.manager->channelVolumes[categories[index]];
    state.volumes->speechCategorySixVolume=state.manager->channelVolumes[6];
}

// Complete PAL1AFAB8..1AFB38. The final SoundInit tail calls this owner,
// which publishes all seven defaults and then captures those same live
// categories for the speech muffler. A default-only startup loop leaves
// that retained capture zero and the first speech transition mutes gameplay.
void SetVolumeDefaults(FlailerSoundState& state,const DemoDiscManager& demo)
{
    Require(state.audio && state.manager && state.volumes,
        "Default audio volumes have no canonical mixer/settings owner");
    const float scale=demo.ActivationWord() ?
        static_cast<float>(demo.Parameters().masterVolumeScale)*0.10000000149011612f : 1.0f;
    for(std::int32_t category=0;category<7;++category)
        Require(worldAudioSetChannelVolume(state.audio,state.manager,category,
            GetVolumeDefault(*state.volumes,category)*scale)==WORLD_AUDIO_OK,
            "Default audio category publication failed");
    SaveVolumesForSpeech(state);
}

void PlayCommonRegionSample(FlailerSoundState& state,const char* name,float gain)
{
    Require(state.enabled,"Common audio sample has no actual audio-enable owner");
    if (!*state.enabled) return;
    Require(state.audio && state.manager && state.manager->frontendBank && name,
        "Common audio sample has no live common-bank owner");
    const auto* cue=worldAudioFindInBank(state.manager->frontendBank,name);
    if (!cue) return; // The SDK's missing region handle is a silent lookup.
    Require(worldAudioAvailableRequests(state.audio)>=2,
        "Common audio sample stop/play exceeds backend command capacity");
    WorldAudioVoiceRef voice{};
    Require(worldAudioAcquireBankSample(state.audio,state.manager->frontendBank,cue,&voice)==WORLD_AUDIO_OK,
        "Common audio sample cannot borrow its actual shared bank sample");
    Require(worldAudioStopInstance(state.audio,voice.instance)==WORLD_AUDIO_OK,
        "Common audio sample stop failed");
    Require(worldAudioPlayVoice(state.audio,voice,gain,0.0f,0.0f)==WORLD_AUDIO_OK,
        "Common audio sample playback failed");
}

} // namespace hp2

// Native audio command/voice ownership replaces the console SDK boundary.
// These adapters are not additional original ELF methods.
#include <string.h>
#include <cmath>

/* Native identities belong to the serialized audio owner, not the SDK or
 * authored sound data. They are never reused during this process lifetime. */
static uint64_t nextIdentity = 1;

static uint64_t allocateIdentity(void)
{
    if (nextIdentity == UINT64_MAX) return 0;
    return nextIdentity++;
}

static int cueNamesEqual(const char* first, const char* second)
{
    unsigned char left, right;
    do {
        left = (unsigned char)*first++;
        right = (unsigned char)*second++;
        if (left >= 'A' && left <= 'Z') left = (unsigned char)(left + ('a' - 'A'));
        if (right >= 'A' && right <= 'Z') right = (unsigned char)(right + ('a' - 'A'));
        if (left != right) return 0;
    } while (left);
    return 1;
}

static uint64_t nativeAudioMonotonicNanoseconds(void*) noexcept
{
    using Clock=std::chrono::steady_clock;
    static const auto origin=Clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now()-origin).count());
}

WorldAudioResult worldAudioReadNativeClock(WorldAudioRequestQueue* queue,uint64_t* nanoseconds)
{
    if (!queue || !nanoseconds || !queue->generation || !queue->nativeClockRead)
        return WORLD_AUDIO_INVALID_BINDING;
    const uint64_t raw=queue->nativeClockRead(queue->nativeClockOwner);
    if (raw<queue->nativeClockOrigin ||
        raw-queue->nativeClockOrigin>UINT64_MAX-queue->nativeClockOffset)
        return queue->result=WORLD_AUDIO_INVALID_BINDING;
    *nanoseconds=queue->nativeClockOffset+(raw-queue->nativeClockOrigin);
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioBindNativeClock(WorldAudioRequestQueue* queue,
    void* owner,WorldAudioNativeClockRead read)
{
    if (!queue || !queue->generation || (!read && owner))return WORLD_AUDIO_INVALID_BINDING;
    if (!read)read=nativeAudioMonotonicNanoseconds;
    if (queue->nativeClockRead==read && queue->nativeClockOwner==owner)
        return queue->result=WORLD_AUDIO_OK;
    // Custom clocks cannot change a live fade's native timing policy.
    // Returning a terminal host to the default steady clock only translates
    // epochs below; it does not alter any voice, gain, slope or fade anchor.
    if (read!=nativeAudioMonotonicNanoseconds) {
        for (size_t i=0;i<queue->instanceCapacity;++i) {
            const auto* sample=worldAudioInstanceAt(queue,i);
            if (!sample)return queue->result=WORLD_AUDIO_INVALID_BINDING;
            if (sample->occupied && !sample->retiring && sample->hasHandle && sample->fadeStep!=0.0f)
                return queue->result=WORLD_AUDIO_INVALID_BINDING;
        }
    }
    uint64_t elapsed=0;
    const auto result=worldAudioReadNativeClock(queue,&elapsed);
    if (result!=WORLD_AUDIO_OK)return result;
    const uint64_t origin=read(owner);
    queue->nativeClockRead=read;queue->nativeClockOwner=owner;
    queue->nativeClockOrigin=origin;queue->nativeClockOffset=elapsed;
    return queue->result=WORLD_AUDIO_OK;
}

void worldAudioInitialize(WorldAudioRequestQueue* queue,
    const WorldAudioBank* banks, size_t bankCount,
    WorldAudioRequest* requests, size_t capacity)
{
    if (!queue) return;
    memset(queue, 0, sizeof(*queue));
    queue->banks = banks;
    queue->bankCount = bankCount;
    queue->requests = requests;
    queue->capacity = capacity;
    queue->generation = allocateIdentity();
    queue->nativeClockRead=nativeAudioMonotonicNanoseconds;
    queue->nativeClockOrigin=nativeAudioMonotonicNanoseconds(nullptr);
    if ((!banks && bankCount) || (!requests && capacity) ||
        capacity > SIZE_MAX / sizeof(*requests))
        queue->result = WORLD_AUDIO_INVALID_BINDING;
    else if (!queue->generation)
        queue->result = WORLD_AUDIO_CAPACITY;
    else if (capacity)
        memset(requests, 0, capacity * sizeof(*requests));
}

const WorldAudioCue* worldAudioFindInGame(WorldAudioRequestQueue* queue, const char* name)
{
    size_t index;
    if (!queue || !name || (!queue->banks && queue->bankCount)) {
        if (queue) queue->result = WORLD_AUDIO_INVALID_BINDING;
        return NULL;
    }
    queue->result = WORLD_AUDIO_OK;
    if (!queue->inGameBank) {
        for (index = 0; index < queue->bankCount; ++index) {
            if (queue->banks[index].name && strcmp(queue->banks[index].name, "InGame") == 0) {
                queue->inGameBank = &queue->banks[index];
                break;
            }
        }
    }
    if (!queue->inGameBank) return NULL;
    if (!queue->inGameBank->cues && queue->inGameBank->cueCount) {
        queue->result = WORLD_AUDIO_INVALID_BINDING;
        return NULL;
    }
    for (index = 0; index < queue->inGameBank->cueCount; ++index) {
        const WorldAudioCue* cue = &queue->inGameBank->cues[index];
        if (cue->name && cueNamesEqual(cue->name, name)) return cue;
    }
    return NULL;
}

const WorldAudioCue* worldAudioFindInBank(const WorldAudioBank* bank, const char* name)
{
    size_t index;
    if (!bank || !name || (!bank->cues && bank->cueCount)) return NULL;
    for (index = 0; index < bank->cueCount; ++index) {
        const WorldAudioCue* cue = &bank->cues[index];
        if (cue->name && cueNamesEqual(cue->name, name)) return cue;
    }
    return NULL;
}

size_t worldAudioAvailableRequests(const WorldAudioRequestQueue* queue)
{
    if (!queue || !queue->generation || !queue->requests ||
        queue->capacity > SIZE_MAX / sizeof(*queue->requests) ||
        queue->retainedCount > queue->capacity) return 0;
    return queue->capacity - queue->retainedCount;
}

static WorldAudioResult reserveRequests(WorldAudioRequestQueue* queue, size_t count)
{
    if (!queue) return WORLD_AUDIO_INVALID_BINDING;
    if (worldAudioAvailableRequests(queue) < count ||
        count > UINT64_MAX - queue->nextSequence)
        return queue->result = WORLD_AUDIO_CAPACITY;
    return WORLD_AUDIO_OK;
}

static WorldAudioRequest* appendRequest(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue, WorldAudioInstance* instance,
    WorldAudioRequestKind kind)
{
    const size_t slot = (queue->retireIndex + queue->retainedCount) % queue->capacity;
    WorldAudioRequest* request = &queue->requests[slot];
    memset(request, 0, sizeof(*request));
    request->bank = bank;
    request->cue = cue;
    request->sequence = queue->nextSequence++;
    request->instance = instance;
    request->kind = kind;
    request->queueGeneration = queue->generation;
    request->instanceGeneration = instance ? instance->generation : 0;
    request->category = instance ? instance->category : 0;
    request->reservedVoiceId = instance ? instance->reservedVoiceId : 48;
    request->reservedVoiceChannels = instance ? instance->reservedVoiceChannels : 0;
    request->streamSource = instance ? instance->streamSource : nullptr;
    request->streamOffset = instance ? instance->streamOffset : 0;
    request->streamSize = instance ? instance->streamSize : 0;
    request->slot = slot;
    request->deliveryState = WORLD_AUDIO_REQUEST_QUEUED;

    if (instance) ++instance->requestReferences;
    ++queue->retainedCount;
    ++queue->count;
    return request;
}

WorldAudioResult worldAudioPlayInGame(WorldAudioRequestQueue* queue, const char* name,
    float gain, float pan, float variation)
{
    const WorldAudioCue* cue = worldAudioFindInGame(queue, name);
    if (!queue) return WORLD_AUDIO_INVALID_BINDING;
    if (!cue) return queue->result;
    return worldAudioPlayCue(queue, queue->inGameBank, cue, gain, pan, variation);
}

WorldAudioResult worldAudioPlayInGameIfIdle(WorldAudioRequestQueue* queue,
    const char* name, float gain, float pan, float variation)
{
    const WorldAudioCue* cue = worldAudioFindInGame(queue, name);
    WorldAudioVoiceRef voice = {NULL, 0};
    if (!queue) return WORLD_AUDIO_INVALID_BINDING;
    if (!cue) return queue->result;
    for (size_t index = 0; index < queue->instanceCapacity; ++index) {
        WorldAudioInstance* instance = worldAudioInstanceAt(queue,index);
        if (!instance) return queue->result=WORLD_AUDIO_INVALID_BINDING;
        if (instance->occupied && !instance->retiring && instance->bankSample && !instance->autoRelease &&
            instance->bank == queue->inGameBank && instance->cue == cue) {
            voice = worldAudioVoiceReference(instance);
            break;
        }
    }
    if (voice.instance && voice.instance->playing) return queue->result = WORLD_AUDIO_OK;
    if (reserveRequests(queue, 1) != WORLD_AUDIO_OK) return queue->result;
    if (!voice.instance && worldAudioAcquireBankSample(queue, queue->inGameBank, cue,
            &voice) != WORLD_AUDIO_OK) return queue->result;
    return worldAudioPlayVoice(queue, voice, gain, pan, variation);
}

WorldAudioResult worldAudioPlayCue(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue,
    float gain, float pan, float variation)
{
    if (!queue || !bank || !cue) return WORLD_AUDIO_INVALID_BINDING;
    if (queue->library && !queue->library->OwnsLoadedBank(bank))
        return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue, 1) != WORLD_AUDIO_OK) return queue->result;
    // PAL1AFBD4 and177FD4 play the shared bRegion::Sample, not a cue
    // without an instance. The loaded region already owns this master;
    // its generation and request count must survive through STOP/UNLOAD.
    WorldAudioVoiceRef voice{};
    const auto acquired=worldAudioAcquireBankSample(queue,bank,cue,&voice);
    if (acquired!=WORLD_AUDIO_OK)return acquired;
    return worldAudioPlayVoice(queue,voice,gain,pan,variation);
}

WorldAudioResult worldAudioSetChannelVolume(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager, int32_t channel, float value)
{
    if (!queue || !queue->library || manager!=&queue->library->Mixer() || channel<0)
        return WORLD_AUDIO_INVALID_BINDING;
    return queue->library->SetCategoryVolume(static_cast<uint32_t>(channel),value);
}

WorldAudioResult worldAudioSubmitChannelVolume(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager, int32_t channel, float value)
{
    WorldAudioRequest* request;
    if (!queue || !manager || !manager->channelControls ||
        manager->channelControls != manager->channelVolumes || channel < 0 ||
        (size_t)channel >= manager->channelVolumeCount) {
        if (queue) queue->result = WORLD_AUDIO_INVALID_BINDING;
        return WORLD_AUDIO_INVALID_BINDING;
    }
    if (reserveRequests(queue, 1) != WORLD_AUDIO_OK) return queue->result;
    request = appendRequest(queue, NULL, NULL, NULL, WORLD_AUDIO_SET_CHANNEL_VOLUME);
    request->channel = channel;
    request->parameterValue = value;
    manager->channelControls[channel] = value;
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSetMode(WorldAudioRequestQueue* queue, int32_t mode)
{
    if (!queue) return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue, 1) != WORLD_AUDIO_OK) return queue->result;
    WorldAudioRequest* request = appendRequest(queue, NULL, NULL, NULL, WORLD_AUDIO_SET_AUDIO_MODE);
    request->audioMode = mode;
    /* Source1AF9F4 submits the actual SDK operation before1AF9FC updates the
     * one game sound cell. Keep its signed32-bit value, not a stereo flag. */
    worldAudioRuntime.audioMode = mode;
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioUnloadBank(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank,const char* name)
{
    if (!queue || !bank || !name || std::strlen(name)>32)
        return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue, 1) != WORLD_AUDIO_OK) return queue->result;
    auto* request=appendRequest(queue, bank, nullptr, nullptr, WORLD_AUDIO_UNLOAD_BANK);
    std::memcpy(request->regionName,name,std::strlen(name)+1);
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSetRegionPath(WorldAudioRequestQueue* queue,const char* path)
{
    if (!queue || !queue->library) return WORLD_AUDIO_INVALID_BINDING;
    return queue->library->SetPath(path);
}

WorldAudioResult worldAudioSubmitRegionPath(WorldAudioRequestQueue* queue,const char* path)
{
    if (!queue || !path || std::strlen(path)>=sizeof(WorldAudioRequest::resourcePath))
        return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue,1)!=WORLD_AUDIO_OK) return queue->result;
    auto* request=appendRequest(queue,nullptr,nullptr,nullptr,WORLD_AUDIO_SET_REGION_PATH);
    std::memcpy(request->resourcePath,path,std::strlen(path)+1);
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioLoadRegion(WorldAudioRequestQueue* queue,const char* name)
{
    if (!queue || !queue->library) return WORLD_AUDIO_INVALID_BINDING;
    return queue->library->LoadRegion(name);
}

WorldAudioResult worldAudioSubmitRegionLoad(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank,const char* name)
{
    if (!queue || !bank || !name || std::strlen(name)>32) return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue,1)!=WORLD_AUDIO_OK) return queue->result;
    auto* request=appendRequest(queue,bank,nullptr,nullptr,WORLD_AUDIO_LOAD_REGION);
    std::memcpy(request->regionName,name,std::strlen(name)+1);
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSubmitDeviceInitialization(WorldAudioRequestQueue* queue)
{
    if (!queue) return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue,1)!=WORLD_AUDIO_OK) return queue->result;
    appendRequest(queue,nullptr,nullptr,nullptr,WORLD_AUDIO_INITIALIZE_DEVICE);
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSubmitSampleVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,uint16_t volume)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    if (voice.instance->requestReferences==SIZE_MAX || reserveRequests(queue,1)!=WORLD_AUDIO_OK)
        return queue->result=WORLD_AUDIO_CAPACITY;
    auto* request=appendRequest(queue,voice.instance->bank,voice.instance->cue,
        voice.instance,WORLD_AUDIO_SET_SAMPLE_VOLUME);
    request->sampleVolume=volume;
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSubmitStreamOpen(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,const WorldAudioStreamSource* source,uint64_t offset,uint64_t size)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    if (!source || voice.instance->requestReferences==SIZE_MAX || reserveRequests(queue,1)!=WORLD_AUDIO_OK)
        return queue->result=WORLD_AUDIO_CAPACITY;
    auto* request=appendRequest(queue,voice.instance->bank,voice.instance->cue,
        voice.instance,WORLD_AUDIO_OPEN_STREAM);
    request->streamSource=source;
    request->streamOffset=offset;
    request->streamSize=size;
    voice.instance->streamOffset=offset;
    voice.instance->streamSize=size;
    voice.instance->playSequence=request->sequence;
    voice.instance->prebuffered=0;
    voice.instance->playing=0;
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSubmitChainPart(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,const WorldAudioStreamSource* source,uint32_t offset,uint32_t bytes,
    int32_t last,uint32_t pitch,uint16_t volume,int8_t pan)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    if (!source || !source->file.Valid() || source->movie || voice.instance->sampleType!=1 ||
        voice.instance->streamSource!=source || offset>source->file.Size() ||
        bytes>source->file.Size()-offset || !voice.instance->hasHandle)
        return WORLD_AUDIO_INVALID_BINDING;
    if (voice.instance->requestReferences==SIZE_MAX || reserveRequests(queue,1)!=WORLD_AUDIO_OK)
        return queue->result=WORLD_AUDIO_CAPACITY;
    auto* request=appendRequest(queue,voice.instance->bank,voice.instance->cue,voice.instance,
        last?WORLD_AUDIO_OPEN_CHAIN:WORLD_AUDIO_APPEND_CHAIN);
    request->streamSource=source;request->streamOffset=offset;request->streamSize=bytes;
    request->samplePitch=pitch;request->sampleVolume=volume;request->samplePan=pan;
    if (last) {
        voice.instance->streamOffset=offset;voice.instance->streamSize=bytes;
        voice.instance->playSequence=request->sequence;voice.instance->prebuffered=0;
        voice.instance->playing=0;voice.instance->paused=0;
    }
    // The same retained queue carries every _CHAIN range and the final
    // _STREAM open. This does not acknowledge decoding or start a device.
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioUpdateDriver(WorldAudioRequestQueue* queue)
{
    if (!queue) return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue,1)!=WORLD_AUDIO_OK) return queue->result;
    appendRequest(queue,nullptr,nullptr,nullptr,WORLD_AUDIO_DRIVER_UPDATE);
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSetJukeboxTrackIncluded(WorldAudioRequestQueue* queue,
    int32_t group,int32_t track,int32_t included)
{
    if (!queue || soundJukeboxPlayer.audio!=queue || group<0 || track<0)
        return WORLD_AUDIO_INVALID_BINDING;
    return included ? soundJukeboxPlayer.SetTuneOn(static_cast<uint32_t>(group),static_cast<uint32_t>(track)) :
        soundJukeboxPlayer.SetTuneOff(static_cast<uint32_t>(group),static_cast<uint32_t>(track));
}

int32_t worldAudioGetMode(void)
{
    return worldAudioRuntime.audioMode;
}

WorldAudioResult worldAudioSetCoreEffectVolume(WorldAudioRequestQueue* queue,
    int32_t core,uint16_t volume)
{
    if (!queue || core<0 || core>1) return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue,1)!=WORLD_AUDIO_OK) return queue->result;
    auto* request=appendRequest(queue,nullptr,nullptr,nullptr,WORLD_AUDIO_SET_CORE_EFFECT_VOLUME);
    request->effectCore=core;
    request->effectCoreVolume=volume;
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSetCoreEffectPan(WorldAudioRequestQueue* queue,
    int32_t core,int8_t pan)
{
    if (!queue || core<0 || core>1) return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue,1)!=WORLD_AUDIO_OK) return queue->result;
    auto* request=appendRequest(queue,nullptr,nullptr,nullptr,WORLD_AUDIO_SET_CORE_EFFECT_PAN);
    request->effectCore=core;
    request->effectCorePan=pan;
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSubmitEffectControl(WorldAudioRequestQueue* queue,
    int32_t core,int32_t attribute,int32_t value)
{
    if (!queue || core<0 || core>1 || attribute<0 || attribute>5 ||
        (attribute==0 && value!=0 && value!=1))return WORLD_AUDIO_INVALID_BINDING;
    if (reserveRequests(queue,1)!=WORLD_AUDIO_OK)return queue->result;
    auto* request=appendRequest(queue,nullptr,nullptr,nullptr,attribute ?
        WORLD_AUDIO_SET_CORE_EFFECT_ATTRIBUTE:WORLD_AUDIO_SET_CORE_EFFECT_ENABLED);
    request->effectCore=core;
    request->effectParameter=attribute;
    request->effectPreset=value;
    return queue->result=WORLD_AUDIO_OK;
}

int worldAudioTake(WorldAudioRequestQueue* queue, WorldAudioRequest* request)
{
    WorldAudioRequest* stored;
    if (!queue || !request || !queue->count || !queue->requests || !queue->capacity) return 0;
    stored = &queue->requests[queue->readIndex];
    if (stored->deliveryState != WORLD_AUDIO_REQUEST_QUEUED) return 0;
    stored->deliveryState = WORLD_AUDIO_REQUEST_INFLIGHT;
    *request = *stored;
    queue->readIndex = (queue->readIndex + 1) % queue->capacity;
    --queue->count;
    return 1;
}

static void releaseRetiredInstance(WorldAudioInstance* instance)
{
    if (instance->retiring==1 && !instance->requestReferences) {
        const uint64_t generation = instance->generation;
        // Movie stream metadata outlives every real queued device request.
        // Detach the instance first; resetting its final transport pin can
        // destroy the source and its bank/cue. No device acknowledgement is
        // synthesized by this ownership cleanup.
        const auto* stream=instance->streamSource;
        memset(instance, 0, sizeof(*instance));
        instance->generation = generation;
        if (stream)const_cast<WorldAudioStreamSource*>(stream)->retainedByVoice.reset();
    }
}

static WorldAudioResult finishAudioRequest(WorldAudioRequestQueue* queue,
    const WorldAudioRequest* request,bool failed)
{
    WorldAudioRequest* stored;
    if (!queue || !request || !queue->requests || request->queueGeneration != queue->generation ||
        request->slot >= queue->capacity) return WORLD_AUDIO_STALE_REFERENCE;
    stored = &queue->requests[request->slot];
    if (stored->deliveryState != WORLD_AUDIO_REQUEST_INFLIGHT ||
        stored->sequence != request->sequence || stored->queueGeneration != request->queueGeneration ||
        stored->instance != request->instance || stored->instanceGeneration != request->instanceGeneration ||
        stored->kind != request->kind)
        return WORLD_AUDIO_STALE_REFERENCE;
    if (stored->instance) {
        WorldAudioInstance* instance = stored->instance;
        if (!instance->occupied || instance->generation != stored->instanceGeneration ||
            !instance->requestReferences) return WORLD_AUDIO_STALE_REFERENCE;
        --instance->requestReferences;
        releaseRetiredInstance(instance);
    }
    if (failed) {
        queue->lastFailedSequence=stored->sequence;
        queue->lastFailedKind=stored->kind;
    }
    stored->deliveryState = failed ? WORLD_AUDIO_REQUEST_FAILED:WORLD_AUDIO_REQUEST_ACKNOWLEDGED;
    while (queue->retainedCount &&
        (queue->requests[queue->retireIndex].deliveryState == WORLD_AUDIO_REQUEST_ACKNOWLEDGED ||
         queue->requests[queue->retireIndex].deliveryState == WORLD_AUDIO_REQUEST_FAILED)) {
        memset(&queue->requests[queue->retireIndex], 0, sizeof(*queue->requests));
        queue->retireIndex = (queue->retireIndex + 1) % queue->capacity;
        --queue->retainedCount;
    }
    // A closing movie may outlive a full device queue. State2 retains the
    // same voice and source until an actual acknowledgement frees a command
    // slot. Publish its real STOP here, after older commands; never forge an
    // acknowledgement, playing completion, or extra sample owner.
    for (size_t index=0;index<queue->instanceCapacity;++index) {
        auto* current=worldAudioInstanceAt(queue,index);
        if (!current) return queue->result=WORLD_AUDIO_INVALID_BINDING;
        auto& instance=*current;
        if (!instance.occupied || instance.retiring!=2)continue;
        if (reserveRequests(queue,1)!=WORLD_AUDIO_OK)break;
        auto* stop=appendRequest(queue,instance.bank,instance.cue,&instance,WORLD_AUDIO_STOP);
        stop->releaseDeviceVoice=1;
        instance.playing=0;
        instance.primed=0;
        instance.hasHandle=0;
        instance.retiring=1;
        if (queue->library)queue->library->Voices().ReleaseSample(worldAudioVoiceReference(&instance));
    }
    return queue->result = failed ? WORLD_AUDIO_BACKEND_FAILURE:WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioAcknowledge(WorldAudioRequestQueue* queue,
    const WorldAudioRequest* request)
{
    return finishAudioRequest(queue,request,false);
}

WorldAudioResult worldAudioReject(WorldAudioRequestQueue* queue,
    const WorldAudioRequest* request)
{
    // Only after the device has relinquished every borrowed payload. This
    // records a failed/cancelled operation, not a synthetic success ACK.
    return finishAudioRequest(queue,request,true);
}

void worldAudioBindInstances(WorldAudioRequestQueue* queue, WorldAudioInstance* instances,
    size_t capacity, const int32_t* enabled)
{
    size_t index;
    if (!queue) return;
    if ((!instances && capacity) || !enabled || queue->library || queue->retainedCount ||
        capacity > SIZE_MAX / sizeof(*instances)) {
        queue->result = WORLD_AUDIO_INVALID_BINDING;
        return;
    }
    for (index = 0; index < queue->instanceCapacity; ++index) {
        const auto* sample=worldAudioInstanceAt(queue,index);
        if (!sample || sample->occupied) {
            queue->result = WORLD_AUDIO_INVALID_BINDING;
            return;
        }
    }
    queue->instances = instances;
    queue->instanceCapacity = capacity;
    queue->spatialEnabled = enabled;
    if (capacity) memset(instances, 0, capacity * sizeof(*instances));
    queue->result = WORLD_AUDIO_OK;
}

WorldAudioVoiceRef worldAudioVoiceReference(const WorldAudioInstance* instance)
{
    WorldAudioVoiceRef result = {NULL, 0};
    if (instance && instance->occupied) {
        result.instance = (WorldAudioInstance*)instance;
        result.generation = instance->generation;
    }
    return result;
}

WorldAudioInstance* worldAudioInstanceAt(const WorldAudioRequestQueue* queue,size_t index)
{
    if (!queue || !queue->instances || index>=queue->instanceCapacity ||
        queue->instanceCapacity>SIZE_MAX/sizeof(WorldAudioInstance)) return nullptr;
    if (queue->library) {
        if (queue->library->Driver()!=queue || queue->instances!=queue->library->SampleAt(0) ||
            queue->instanceCapacity!=queue->library->SampleCapacity()) return nullptr;
        return queue->library->SampleAt(index);
    }
    return queue->instances+index;
}

WorldAudioResult worldAudioEnsureFreeInstances(WorldAudioRequestQueue* queue,size_t count)
{
    if (!queue || !queue->instances || queue->instanceCapacity>SIZE_MAX/sizeof(WorldAudioInstance))
        return WORLD_AUDIO_INVALID_BINDING;
    if (count>SIZE_MAX/sizeof(WorldAudioInstance)) return queue->result=WORLD_AUDIO_CAPACITY;
    size_t available=0;
    for (size_t index=0;index<queue->instanceCapacity;++index) {
        const auto* sample=worldAudioInstanceAt(queue,index);
        if (!sample) return queue->result=WORLD_AUDIO_INVALID_BINDING;
        available+=sample->occupied==0;
    }
    while (available<count) {
        if (!queue->library) return queue->result=WORLD_AUDIO_CAPACITY;
        const size_t before=queue->instanceCapacity;
        const auto result=queue->library->ExpandSampleStorage();
        if (result!=WORLD_AUDIO_OK) return queue->result=result;
        available+=queue->instanceCapacity-before;
    }
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioValidateVoice(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice)
{
    if (!queue || !voice.instance || !queue->instances ||
        queue->instanceCapacity>SIZE_MAX/sizeof(*voice.instance)) return WORLD_AUDIO_INVALID_BINDING;
    if (queue->library) {
        if (queue->library->Driver()!=queue || queue->instances!=queue->library->SampleAt(0) ||
            queue->instanceCapacity!=queue->library->SampleCapacity() ||
            !queue->library->OwnsSample(voice.instance)) return queue->result=WORLD_AUDIO_INVALID_BINDING;
    } else {
        const uintptr_t first=reinterpret_cast<uintptr_t>(queue->instances);
        const uintptr_t address=reinterpret_cast<uintptr_t>(voice.instance);
        const size_t bytes=queue->instanceCapacity*sizeof(*voice.instance);
        if (address<first || address-first>=bytes || (address-first)%sizeof(*voice.instance))
            return queue->result=WORLD_AUDIO_INVALID_BINDING;
    }
    if (!voice.generation || !voice.instance->occupied ||
        voice.instance->generation != voice.generation || voice.instance->retiring)
        return queue->result = WORLD_AUDIO_STALE_REFERENCE;
    if (!voice.instance->bank || !voice.instance->cue) {
        const auto& sample=*voice.instance;
        if (sample.bankSample || sample.sampleType || sample.hasHandle ||
            !sample.bankName[0] || !sample.sampleName[0])
            return queue->result = WORLD_AUDIO_INVALID_BINDING;
    }
    return WORLD_AUDIO_OK;
}

static void copyAudioSourceName(char (&destination)[33],const char* source)
{
    const size_t count=source ? std::min(std::strlen(source),size_t{32}) : 0;
    if (count)std::memcpy(destination,source,count);
    destination[count]=0;
}

WorldAudioResult worldAudioAcquireVoice(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue, int32_t tag,
    int32_t autoRelease, WorldAudioVoiceRef* voice)
{
    size_t index;
    uint64_t generation;
    WorldAudioInstance* instance = NULL;
    if (!queue || !bank || !cue || !voice || !queue->instances)
        return WORLD_AUDIO_INVALID_BINDING;
    for (index = 0; index < queue->instanceCapacity; ++index) {
        auto* candidate=worldAudioInstanceAt(queue,index);
        if (!candidate) return queue->result=WORLD_AUDIO_INVALID_BINDING;
        if (!candidate->occupied) {instance=candidate;break;}
    }
    if (!instance) {
        const size_t oldCapacity=queue->instanceCapacity;
        const auto result=worldAudioEnsureFreeInstances(queue,1);
        if (result!=WORLD_AUDIO_OK) return result;
        instance=worldAudioInstanceAt(queue,oldCapacity);
        if (!instance) return queue->result=WORLD_AUDIO_INVALID_BINDING;
    }
    generation = allocateIdentity();
    if (!generation)
        return queue->result = WORLD_AUDIO_CAPACITY;
    memset(instance, 0, sizeof(*instance));
    instance->bank = bank;
    instance->cue = cue;
    copyAudioSourceName(instance->bankName,bank->name);
    copyAudioSourceName(instance->sampleName,cue->name);
    instance->tag = tag;
    instance->autoRelease = autoRelease;
    instance->occupied = 1;
    instance->generation = generation;
    instance->playSequence = UINT64_MAX;
    instance->volumeScale = instance->pitchScale = 1.0f;
    instance->volume = 1.0f;
    voice->instance = instance;
    voice->generation = generation;
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioAcquireBankSample(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue, WorldAudioVoiceRef* voice)
{
    if (!queue || !bank || !cue || !voice || !queue->instances)
        return WORLD_AUDIO_INVALID_BINDING;
    for (size_t index = 0; index < queue->instanceCapacity; ++index) {
        WorldAudioInstance* instance = worldAudioInstanceAt(queue,index);
        if (!instance) return queue->result=WORLD_AUDIO_INVALID_BINDING;
        if (instance->occupied && !instance->retiring && instance->bankSample &&
            instance->bank == bank && instance->cue == cue) {
            *voice = worldAudioVoiceReference(instance);
            return queue->result = WORLD_AUDIO_OK;
        }
    }
    const WorldAudioResult result = worldAudioAcquireVoice(queue, bank, cue, -1, 0, voice);
    if (result == WORLD_AUDIO_OK) voice->instance->bankSample = 1;
    return result;
}

WorldAudioResult worldAudioAcquireInGameSample(WorldAudioRequestQueue* queue,
    const char* name, WorldAudioVoiceRef* voice)
{
    if (!queue || !name || !voice) return WORLD_AUDIO_INVALID_BINDING;
    const WorldAudioCue* cue = worldAudioFindInGame(queue, name);
    if (!cue) return queue->result == WORLD_AUDIO_OK ?
        (queue->result = WORLD_AUDIO_INVALID_BINDING) : queue->result;
    return worldAudioAcquireBankSample(queue, queue->inGameBank, cue, voice);
}

WorldAudioResult worldAudioInitializeVoiceFromCue(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue, WorldAudioVoiceRef* destination)
{
    if (!queue || !bank || !cue || !destination) return WORLD_AUDIO_INVALID_BINDING;
    if (!destination->instance)
        return worldAudioAcquireVoice(queue, bank, cue, -1, 0, destination);
    const WorldAudioResult result = worldAudioValidateVoice(queue, *destination);
    if (result != WORLD_AUDIO_OK) return result;
    if (destination->instance->bankSample) return queue->result = WORLD_AUDIO_INVALID_BINDING;
    destination->instance->bank = bank;
    destination->instance->cue = cue;
    copyAudioSourceName(destination->instance->bankName,bank->name);
    copyAudioSourceName(destination->instance->sampleName,cue->name);
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioCopyVoice(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef source, WorldAudioVoiceRef* destination)
{
    const WorldAudioResult result = worldAudioValidateVoice(queue, source);
    if (result != WORLD_AUDIO_OK) return result;
    return worldAudioInitializeVoiceFromCue(queue, source.instance->bank, source.instance->cue, destination);
}

static WorldAudioResult captureAudioParameters(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,float gain,float pan,float offset,
    uint32_t& pitch,uint16_t& volume,int8_t& balance)
{
    auto result=worldAudioCalcVoicePitch(queue,voice,offset,&pitch);
    if (result!=WORLD_AUDIO_OK)return result;
    result=worldAudioCalcVoicePan(pan,&balance);
    if (result!=WORLD_AUDIO_OK)return result;
    float scaled=0.0f;
    result=worldAudioCalcVoiceScaledVolume(queue,voice,&scaled);
    if (result!=WORLD_AUDIO_OK)return result;
    const float raw=(gain*scaled)*16383.0f;
    if (!std::isfinite(raw) || raw<-2147483648.0f || 2147483648.0f<=raw)
        return WORLD_AUDIO_INVALID_BINDING;
    volume=static_cast<uint16_t>(convertFloatToInt32(raw));
    return WORLD_AUDIO_OK;
}

static WorldAudioResult appendVoiceRequest(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice,
    WorldAudioRequestKind kind, float gain, float pan, float variation,bool releasePhysical=false)
{
    WorldAudioRequest* request;
    WorldAudioResult result = worldAudioValidateVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    uint32_t pitch=0;uint16_t volume=0;int8_t balance=0;
    uint16_t reservedId=48,reservedChannels=0;
    uint64_t fadeClock=0;
    if (kind==WORLD_AUDIO_PLAY && voice.instance->fadeStep!=0.0f) {
        const auto clock=worldAudioReadNativeClock(queue,&fadeClock);
        if (clock!=WORLD_AUDIO_OK)return clock;
        if (voice.instance->hasHandle && fadeClock<voice.instance->fadeLastNanoseconds)
            return queue->result=WORLD_AUDIO_INVALID_BINDING;
    }
    if (kind==WORLD_AUDIO_PLAY && queue->library) {
        bool available=false;
        result=queue->library->Voices().SelectChannels(voice,reservedId,reservedChannels,available);
        if(result!=WORLD_AUDIO_OK)return queue->result=result;
        if(!available) {
            queue->library->Printf(
                "***** OH OH ****** tried to get a voice but none were available! for sample %s\n",
                voice.instance->cue->name);
            // PAL274194..2741CC: failed LockChannels prints and returns a
            // NULL handle before gain/pan/pitch, playing or callbacks change.
            // This is a normal source outcome, not a successful device PLAY.
            // No request/ACK/READY is fabricated; later setters see no handle.
            return queue->result=WORLD_AUDIO_OK;
        }
        result=captureAudioParameters(queue,voice,gain,pan,variation,pitch,volume,balance);
        if (result!=WORLD_AUDIO_OK)return result;
    }
    if (voice.instance->requestReferences == SIZE_MAX || reserveRequests(queue, 1) != WORLD_AUDIO_OK)
        return queue->result = WORLD_AUDIO_CAPACITY;
    if(kind==WORLD_AUDIO_PLAY && queue->library) {
        voice.instance->reservedVoiceId=reservedId;
        voice.instance->reservedVoiceChannels=reservedChannels;
    }
    request = appendRequest(queue, voice.instance->bank, voice.instance->cue, voice.instance, kind);
    request->gain = gain;
    request->pan = pan;
    request->variation = variation;
    request->samplePitch=pitch;request->sampleVolume=volume;request->samplePan=balance;
    request->releaseDeviceVoice=kind==WORLD_AUDIO_STOP &&
        (releasePhysical || !voice.instance->primed);
    voice.instance->playing = kind == WORLD_AUDIO_PLAY;
    if (kind == WORLD_AUDIO_PLAY) {
        voice.instance->hasHandle = 1;
        voice.instance->paused = 0; // PAL2743C8: a new PLAY clears pause.
        voice.instance->volume = gain;
        if (voice.instance->fadeStep!=0.0f) {
            voice.instance->fadeOriginVolume=gain;
            voice.instance->fadeOriginNanoseconds=voice.instance->fadeLastNanoseconds=fadeClock;
            voice.instance->fadeDurationNanoseconds=0;
        }
        voice.instance->pitchOffset = variation;
        voice.instance->pan = pan;
        voice.instance->playSequence = request->sequence;
    } else if (kind == WORLD_AUDIO_STOP) {
        // A late ready event for an older OPEN must not restart a stopped
        // source. A subsequent actual OPEN installs its own fresh sequence.
        voice.instance->playSequence = UINT64_MAX;
    }
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioPlayVoice(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice,
    float gain, float pan, float variation)
{
    const auto valid=worldAudioResolveVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    if (queue->library && !voice.instance->sampleType &&
        !queue->library->OwnsLoadedBank(voice.instance->bank))
        return WORLD_AUDIO_INVALID_BINDING;
    if (voice.instance->sampleType==3 && (!voice.instance->streamSource || !voice.instance->prebuffered))
        return WORLD_AUDIO_INVALID_BINDING;
    if (voice.instance->sampleType==1 && (!voice.instance->streamSource || !voice.instance->prebuffered))
        return WORLD_AUDIO_INVALID_BINDING;
    return appendVoiceRequest(queue, voice, WORLD_AUDIO_PLAY, gain, pan, variation);
}

WorldAudioResult worldAudioPrimeVoice(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice)
{
    const WorldAudioResult result = worldAudioResolveVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    uint16_t reservedId=48,reservedChannels=0;
    if(queue->library) {
        bool available=false;
        const auto selected=queue->library->Voices().SelectChannels(voice,reservedId,reservedChannels,available);
        if(selected!=WORLD_AUDIO_OK)return queue->result=selected;
        // Source LockVoice asserts on exhaustion; its required reservation
        // is separate from ordinary Play's explicitly nullable result.
        if(!available)return queue->result=WORLD_AUDIO_CAPACITY;
    }
    if (voice.instance->requestReferences == SIZE_MAX || reserveRequests(queue, 1) != WORLD_AUDIO_OK)
        return queue->result = WORLD_AUDIO_CAPACITY;
    if(queue->library) {
        voice.instance->reservedVoiceId=reservedId;
        voice.instance->reservedVoiceChannels=reservedChannels;
    }
    appendRequest(queue, voice.instance->bank, voice.instance->cue, voice.instance, WORLD_AUDIO_PRIME);
    voice.instance->primed = 1;
    voice.instance->hasHandle = 1;
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioRetireVoice(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice)
{
    WorldAudioResult result = worldAudioValidateVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    if (voice.instance->hasHandle) {
        result = appendVoiceRequest(queue, voice, WORLD_AUDIO_STOP, 0, 0, 0,true);
        if (result != WORLD_AUDIO_OK) {
            const auto* source=voice.instance->streamSource;
            if (result!=WORLD_AUDIO_CAPACITY || !source || !source->retainedByVoice ||
                queue->nextSequence==UINT64_MAX)return result;
            // Only a self-retained movie provider can detach immediately;
            // its pending STOP still owns its actual bank/cue/voice metadata.
            voice.instance->completionSample=nullptr;
            voice.instance->completionKind=WORLD_AUDIO_COMPLETION_NONE;
            voice.instance->retiring=2;
            return queue->result=WORLD_AUDIO_OK;
        }
    }
    voice.instance->completionSample = nullptr;
    voice.instance->completionKind = WORLD_AUDIO_COMPLETION_NONE;
    voice.instance->primed = 0;
    voice.instance->hasHandle = 0;
    voice.instance->retiring = 1;
    if (queue->library)queue->library->Voices().ReleaseSample(voice);
    releaseRetiredInstance(voice.instance);
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioCompleteVoice(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, uint64_t playSequence)
{
    const WorldAudioResult result = worldAudioValidateVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    if (voice.instance->playSequence != playSequence || !voice.instance->playing)
        return WORLD_AUDIO_STALE_REFERENCE;
    Sound3dSample* positional = voice.instance->completionKind==WORLD_AUDIO_COMPLETION_POSITIONAL ?
        voice.instance->completionSample : nullptr;
    cJukeboxPlayer* jukebox = voice.instance->completionKind==WORLD_AUDIO_COMPLETION_JUKEBOX ?
        voice.instance->completionJukebox : nullptr;
    WorldStreamAudio* speech = voice.instance->completionKind==WORLD_AUDIO_COMPLETION_SPEECH ?
        voice.instance->completionSpeech : nullptr;
    if (positional && (!queue->positionalSamples ||
        queue->positionalSamples->Find(voice) != positional))
        return queue->result = WORLD_AUDIO_INVALID_BINDING;
    voice.instance->playing = 0;
    voice.instance->hasHandle = voice.instance->primed;
    if (!voice.instance->primed && queue->library)
        queue->library->Voices().ReleaseSample(voice);
    voice.instance->paused = 0;
    if (positional) {
        voice.instance->completionSample = nullptr;
        voice.instance->completionKind = WORLD_AUDIO_COMPLETION_NONE;
    }
    if (positional) return queue->result = positional->OnVoiceCompleted(voice);
    if (jukebox) return queue->result = jukebox->SampleCallback(8,voice);
    if (speech) return queue->result = worldStreamAudioSampleCallback(speech,8,voice);
    if (voice.instance->autoRelease) {
        voice.instance->retiring = 1;
        releaseRetiredInstance(voice.instance);
    }
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioPlayInstance(WorldAudioRequestQueue* queue, WorldAudioInstance* instance,
    float gain, float pan, float variation)
{
    return worldAudioPlayVoice(queue, worldAudioVoiceReference(instance), gain, pan, variation);
}

WorldAudioResult worldAudioStopInstance(WorldAudioRequestQueue* queue, WorldAudioInstance* instance)
{
    const WorldAudioVoiceRef voice = worldAudioVoiceReference(instance);
    WorldAudioResult result = worldAudioValidateVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    // Source Stop returns before its event8 path when sample+0C is NULL.
    if (!instance->hasHandle) return queue->result = WORLD_AUDIO_OK;
    Sound3dSample* positional = instance->completionKind==WORLD_AUDIO_COMPLETION_POSITIONAL ?
        instance->completionSample : nullptr;
    cJukeboxPlayer* jukebox = instance->completionKind==WORLD_AUDIO_COMPLETION_JUKEBOX ?
        instance->completionJukebox : nullptr;
    WorldStreamAudio* speech = instance->completionKind==WORLD_AUDIO_COMPLETION_SPEECH ?
        instance->completionSpeech : nullptr;
    if (positional && (!queue->positionalSamples || queue->positionalSamples->Find(voice) != positional))
        return queue->result = WORLD_AUDIO_INVALID_BINDING;
    result = appendVoiceRequest(queue, voice, WORLD_AUDIO_STOP, 0, 0, 0);
    if (result != WORLD_AUDIO_OK) return result;
    if (positional) {
        instance->completionSample = nullptr;
        instance->completionKind = WORLD_AUDIO_COMPLETION_NONE;
    }
    if (!instance->primed) {
        instance->hasHandle = 0;
        if (queue->library)queue->library->Voices().ReleaseSample(voice);
    }
    instance->paused = 0;
    if (positional) return queue->result = positional->OnVoiceCompleted(voice);
    if (jukebox) return queue->result = jukebox->SampleCallback(8,voice);
    if (speech) return queue->result = worldStreamAudioSampleCallback(speech,8,voice);
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSetVoiceParameters(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, uint16_t controlCode, float pitchOffset, float volume)
{
    WorldAudioResult result = worldAudioValidateVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    // All three original setters skip the driver when sample+0C is NULL;
    // SetPitch still stores sample+2C. No command capacity is needed then.
    const size_t needed=voice.instance->hasHandle ? 3u : 0u;
    if (needed && (voice.instance->requestReferences > SIZE_MAX - needed ||
        reserveRequests(queue,needed) != WORLD_AUDIO_OK))
        return queue->result = WORLD_AUDIO_CAPACITY;
    result=worldAudioSetVoiceControlCode(queue,voice,controlCode);
    if (result!=WORLD_AUDIO_OK) return result;
    result=worldAudioSetVoicePitchOffset(queue,voice,pitchOffset);
    if (result!=WORLD_AUDIO_OK) return result;
    return worldAudioSetVoiceVolume(queue,voice,volume);
}

WorldAudioResult worldAudioSetInstanceParameters(WorldAudioRequestQueue* queue,
    WorldAudioInstance* instance, uint16_t controlCode, float pitchOffset, float volume)
{
    return worldAudioSetVoiceParameters(queue, worldAudioVoiceReference(instance),
        controlCode, pitchOffset, volume);
}

WorldAudioResult worldAudioSetVoiceControlCode(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,uint16_t controlCode)
{
    WorldAudioResult result=worldAudioValidateVoice(queue,voice);
    if(result!=WORLD_AUDIO_OK)return result;
    // SetAzimuth PAL273E90..273EBC neither calls the driver nor stores an
    // azimuth when this sample has no allocated/queued voice handle.
    if(!voice.instance->hasHandle)return queue->result=WORLD_AUDIO_OK;
    if(voice.instance->requestReferences==SIZE_MAX || reserveRequests(queue,1)!=WORLD_AUDIO_OK)
        return queue->result=WORLD_AUDIO_CAPACITY;
    WorldAudioRequest* request=appendRequest(queue,voice.instance->bank,voice.instance->cue,
        voice.instance,WORLD_AUDIO_SET_CONTROL_CODE);
    request->controlCode=controlCode;
    voice.instance->controlCode=controlCode;
    return queue->result=WORLD_AUDIO_OK;
}

static WorldAudioResult appendVoiceParameter(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, WorldAudioRequestKind kind, float value)
{
    const WorldAudioResult result = worldAudioValidateVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    if (voice.instance->requestReferences == SIZE_MAX || reserveRequests(queue, 1) != WORLD_AUDIO_OK)
        return queue->result = WORLD_AUDIO_CAPACITY;
    WorldAudioRequest* request = appendRequest(queue, voice.instance->bank, voice.instance->cue,
        voice.instance, kind);
    request->parameterValue = value;
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSetVoicePitchOffset(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, float pitchOffset)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if(valid!=WORLD_AUDIO_OK)return valid;
    // SetPitch PAL27375C..2737F0 skips CalcPitch and the driver for a NULL
    // handle, but always stores the caller's original binary32 pitch offset.
    if(!voice.instance->hasHandle) {
        voice.instance->pitchOffset=pitchOffset;
        return queue->result=WORLD_AUDIO_OK;
    }
    uint32_t pitch=0;
    if (queue && queue->library) {
        const auto calculated=worldAudioCalcVoicePitch(queue,voice,pitchOffset,&pitch);
        if (calculated!=WORLD_AUDIO_OK)return calculated;
    }
    const WorldAudioResult result = appendVoiceParameter(queue, voice, WORLD_AUDIO_SET_PITCH_OFFSET, pitchOffset);
    if (result == WORLD_AUDIO_OK) {
        voice.instance->pitchOffset = pitchOffset;
        const size_t slot=(queue->retireIndex+queue->retainedCount-1)%queue->capacity;
        queue->requests[slot].samplePitch=pitch;
    }
    return result;
}


WorldAudioResult worldAudioFadeVoiceVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, float target, uint32_t milliseconds)
{
    const WorldAudioResult result = worldAudioValidateVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    // Fade PAL274888 tests the handle, including a primed idle sample.
    if (!voice.instance->hasHandle) return queue->result = WORLD_AUDIO_OK;
    if (!std::isfinite(target)) return queue->result = WORLD_AUDIO_INVALID_BINDING;
    float scaled=0.0f;
    const auto calculated=worldAudioCalcVoiceScaledVolume(queue,voice,&scaled);
    if(calculated!=WORLD_AUDIO_OK)return calculated;
    const float resolved=target*scaled; // PAL2748B4 /2748E0, before SetVolume/fade state.
    if (milliseconds <= 32) return worldAudioSetVoiceVolume(queue, voice, resolved);
    const float steps = static_cast<float>(milliseconds) * 0.059988003224134445f;
    const float step = (resolved - voice.instance->volume) / steps;
    if (!std::isfinite(resolved) || !std::isfinite(step))
        return queue->result=WORLD_AUDIO_INVALID_BINDING;
    uint64_t now=0;
    const auto clock=worldAudioReadNativeClock(queue,&now);
    if (clock!=WORLD_AUDIO_OK)return clock;
    if (voice.instance->fadeStep!=0.0f && now<voice.instance->fadeLastNanoseconds)
        return queue->result=WORLD_AUDIO_INVALID_BINDING;
    if (voice.instance->requestReferences == SIZE_MAX || reserveRequests(queue, 1) != WORLD_AUDIO_OK)
        return queue->result = WORLD_AUDIO_CAPACITY;
    WorldAudioRequest* request = appendRequest(queue, voice.instance->bank, voice.instance->cue,
        voice.instance, WORLD_AUDIO_FADE_VOLUME);
    request->parameterValue = target;
    request->durationMilliseconds = milliseconds;
    voice.instance->fadeTarget = resolved;
    voice.instance->fadeStep = step;
    voice.instance->fadeOriginVolume=voice.instance->volume;
    voice.instance->fadeOriginNanoseconds=voice.instance->fadeLastNanoseconds=now;
    voice.instance->fadeDurationNanoseconds=static_cast<uint64_t>(milliseconds)*1000000u;
    return queue->result = WORLD_AUDIO_OK;
}


WorldAudioResult worldAudioGetVoiceVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, float* volume)
{
    if (!volume) return WORLD_AUDIO_INVALID_BINDING;
    const WorldAudioResult result = worldAudioValidateVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    *volume = voice.instance->playing ? voice.instance->volume : 0.0f;
    return queue->result = WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioPauseVoice(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,int32_t paused)
{
    WorldAudioResult result=worldAudioValidateVoice(queue,voice);
    if(result!=WORLD_AUDIO_OK)return result;
    // PAL2744A8/2744F8 and274588/274598 require both a retained handle
    // and a playing driver voice. ReleaseVoice only clears the handle, so
    // a previous logical playing flag alone cannot authorize this command.
    if(!voice.instance->hasHandle || !voice.instance->playing)
        return queue->result=WORLD_AUDIO_OK;
    if(voice.instance->requestReferences==SIZE_MAX || reserveRequests(queue,1)!=WORLD_AUDIO_OK)
        return queue->result=WORLD_AUDIO_CAPACITY;
    WorldAudioRequest* request=appendRequest(queue,voice.instance->bank,voice.instance->cue,
        voice.instance,WORLD_AUDIO_SET_PAUSED);
    request->parameterValue=paused ? 1.0f:0.0f;
    voice.instance->paused=paused!=0;
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioMuteVoice(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, int32_t muted)
{
    const WorldAudioResult result = worldAudioValidateVoice(queue, voice);
    if (result != WORLD_AUDIO_OK) return result;
    if (!voice.instance->hasHandle || voice.instance->muted == muted)
        return queue->result = WORLD_AUDIO_OK;
    if (voice.instance->requestReferences == SIZE_MAX || reserveRequests(queue, 1) != WORLD_AUDIO_OK)
        return queue->result = WORLD_AUDIO_CAPACITY;
    WorldAudioRequest* request = appendRequest(queue, voice.instance->bank, voice.instance->cue,
        voice.instance, WORLD_AUDIO_SET_MUTED);
    request->parameterValue = static_cast<float>(muted);
    request->muted = muted;
    voice.instance->muted = muted;
    return queue->result = WORLD_AUDIO_OK;
}

WorldStreamAudioResult soundLoadLocalizedSpeech(WorldStreamAudio& stream,int32_t language)
{
    if (!stream.audioEnabled || !stream.speechEnabled)
        return stream.result=WORLD_STREAM_AUDIO_INVALID_BINDING;
    if (!*stream.audioEnabled || static_cast<uint32_t>(language)>=6u)
        return stream.result=WORLD_STREAM_AUDIO_OK;
    // Exact final PAL table2B6228: Swedish uses English speech. Active demo
    // policy selects DemoDisk, not English and not a beta asset directory.
    static constexpr const char* names[]={"English","French","German","Italian","Spanish","English"};
    const char* selected=TheDemoDiscManager.ActivationWord()!=0 ? "DemoDisk" : names[language];
    const std::string base=std::string("Sound\\Speech\\")+selected;
    const auto header=base+"HDR.pck";
    const auto banks=base+"BAN.pck";
    const auto data=base+"DAT.pck";
    if (!*stream.speechEnabled)return stream.result=WORLD_STREAM_AUDIO_OK;
    return worldStreamAudioInitialize(stream,"Sound\\Speech\\Events.evt",header.c_str(),data.c_str(),banks.c_str());
}

WorldStreamAudioResult soundUnloadLocalizedSpeech(WorldStreamAudio& stream)
{
    if (!stream.audioEnabled)return stream.result=WORLD_STREAM_AUDIO_INVALID_BINDING;
    if (!*stream.audioEnabled)return stream.result=WORLD_STREAM_AUDIO_OK;
    return worldStreamAudioCleanup(&stream);
}

// TriggerTimerWarningSound__FR5Timer, final PAL001AFB80..001AFBF0.
// The original shares the last whole second across local Player HUDs. A
// missing named sample is a real bank lookup result and does not repeat the
// same second's lookup. Playback uses the existing bSample request owner.
WorldAudioResult soundPlayTimerCountdown(WorldAudioRequestQueue* audio,
    const int32_t* remainingTicks)
{
    if (!audio || !remainingTicks)return WORLD_AUDIO_INVALID_BINDING;
    const int32_t seconds=convertFloatToInt32(
        static_cast<float>(*remainingTicks)*0.0002500000118743628f);
    if (seconds==soundDriverState.timerWarningSecond)return WORLD_AUDIO_OK;
    const WorldAudioCue* cue=worldAudioFindInGame(audio,"TimerCountdown");
    const WorldAudioResult result=cue ? worldAudioPlayCue(audio,audio->inGameBank,
        cue,1.0f,0.0f,0.0f) : audio->result;
    soundDriverState.timerWarningSecond=seconds;
    return result;
}

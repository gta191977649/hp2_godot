#include "bsound/include/bJukebox.h"
#include "bsound/include/bSound.h"
#include "bware/inc/bFile.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace {

// PAL GP-34EC is separate from the player's PickNextTune cursor+22A0.
uint32_t nameSearchCategoryCursor{};

uint32_t word(const std::vector<uint8_t>& bytes,size_t offset)
{
    if (offset>bytes.size() || bytes.size()-offset<4)
        throw std::invalid_argument("Truncated JBC metadata");
    return uint32_t{bytes[offset]} | uint32_t{bytes[offset+1]}<<8 |
        uint32_t{bytes[offset+2]}<<16 | uint32_t{bytes[offset+3]}<<24;
}

std::string text(const std::vector<uint8_t>& bytes,size_t offset,size_t capacity)
{
    if (offset>bytes.size() || capacity>bytes.size()-offset)
        throw std::invalid_argument("Truncated JBC name");
    const char* start=reinterpret_cast<const char*>(bytes.data()+offset);
    const char* end=std::find(start,start+capacity,'\0');
    if (end==start+capacity) throw std::invalid_argument("Unterminated JBC name");
    return std::string(start,end);
}

bool sameVoice(WorldAudioVoiceRef left,WorldAudioVoiceRef right)
{
    return left.instance==right.instance && left.generation==right.generation;
}

} // namespace

const cJukeboxCartridge::Tune& cJukeboxCartridge::Track(uint32_t group,uint32_t track) const
{
    return tunes.at(groups.at(group).tunes.at(track));
}

const char* cJukeboxCartridge::GetTuneName(uint32_t group,uint32_t track) const
{
    return Track(group,track).name.c_str();
}

uint32_t cJukeboxCartridge::GetTuneOffset(uint32_t group,uint32_t track) const
{
    const auto& tune=Track(group,track);
    const uint32_t sync=tune.syncSize ? ((tune.syncSize>>11)<<11)+2048u : 0u;
    return tune.offset+sync;
}

uint32_t cJukeboxCartridge::GetSyncOffset(uint32_t group,uint32_t track) const
{
    return Track(group,track).offset;
}

uint32_t cJukeboxCartridge::GetTuneSize(uint32_t group,uint32_t track) const
{
    return Track(group,track).size;
}

uint32_t cJukeboxCartridge::GetSyncSize(uint32_t group,uint32_t track) const
{
    return Track(group,track).syncSize;
}

uint32_t cJukeboxCartridge::GetTuneID(uint32_t group,uint32_t track) const
{
    return Track(group,track).id;
}

uint32_t cJukeboxCartridge::AddCategory(const char* name)
{
    if (!name) throw std::invalid_argument("Jukebox category name is absent");
    for (size_t index=0;index<groups.size();++index)
        if (groups[index].name==name) return static_cast<uint32_t>(index);
    if (groups.size()>=16) throw std::out_of_range("Jukebox category capacity");
    groups.push_back({name,{},true,0});
    return static_cast<uint32_t>(groups.size()-1);
}

uint32_t cJukeboxCartridge::AddTune(const char* group,const char* name,
    uint32_t offset,uint32_t size,uint32_t syncSize,uint32_t id)
{
    if (!group || !name) throw std::invalid_argument("Jukebox tune/category name is absent");
    size_t index=0;
    while (index<tunes.size() && tunes[index].name!=name) ++index;
    const bool newTune=index==tunes.size();
    if (newTune && index>=128) return UINT32_MAX;
    // Check the entire cartridge destination before modifying any old tune.
    // A rejected category must not change a track played by another bank.
    size_t categoryIndex=0;
    while (categoryIndex<groups.size() && groups[categoryIndex].name!=group) ++categoryIndex;
    const bool newCategory=categoryIndex==groups.size();
    if (newCategory && groups.size()>=16)
        throw std::out_of_range("Jukebox category capacity");
    if (!newCategory && groups[categoryIndex].tunes.size()>=128)
        throw std::out_of_range("Jukebox category track capacity");
    if (newTune) tunes.push_back({name,true,offset,size,syncSize,id});
    bool publishedCategory=false;
    try {
        if (newCategory) {
            groups.push_back({group,{},true,0});
            publishedCategory=true;
        }
        groups[categoryIndex].tunes.push_back(index);
        groups[categoryIndex].cursor=0;
    } catch (...) {
        if (publishedCategory) groups.pop_back();
        if (newTune) tunes.pop_back();
        throw;
    }
    auto& tune=tunes[index];
    tune.offset=offset;tune.size=size;tune.syncSize=syncSize;tune.id=id;
    tune.enabled=true;
    return static_cast<uint32_t>(index);
}

WorldAudioResult cJukeboxPlayer::Bind(bSound& library,const hp2::GameResources& files)
{
    if (stream_ || currentSample.instance || !library.Driver() || library.GetStatus()!=1)
        return WORLD_AUDIO_INVALID_BINDING;
    library_=&library;
    files_=&files;
    audio=library.Driver();
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::UnbindAfterAudioShutdown(bSound& library,WorldAudioRequestQueue& queue)
{
    if (library_!=&library || audio!=&queue || library.Driver() || library.GetStatus()!=0 ||
        queue.library || queue.banks || queue.bankCount || queue.inGameBank ||
        queue.instances || queue.instanceCapacity || queue.spatialEnabled ||
        queue.count || queue.retainedCount || currentSample.instance || stream_ || closing_ ||
        !groups.empty() || !tunes.empty())
        return WORLD_AUDIO_INVALID_BINDING;
    // Close has already retired the real stream and its metadata. The
    // static player must retain no callback or borrow into that application.
    library_=nullptr;files_=nullptr;audio=nullptr;
    sampleRate_=0;state=0;pendingPlay=0;paused=0;
    return WORLD_AUDIO_OK;
}

bool cJukeboxPlayer::IsCartridgeLoaded() const noexcept
{
    return stream_!=nullptr;
}

WorldAudioResult cJukeboxPlayer::LoadCartridge(const char* path,bool unused)
{
    (void)unused; // PAL's checked boolean branch has no operational body.
    if (!library_ || library_->GetStatus()!=1 || !audio || !files_ || !path || stream_)
        return WORLD_AUDIO_INVALID_BINDING;
    try {
        const auto resource=files_->OpenPhysical(path);
        if (resource.expanded || resource.range.Size()<32) return WORLD_AUDIO_RESOURCE_FAILURE;
        const auto header=resource.range.Read(0,32);
        const uint32_t rate=word(header,8);
        const uint32_t categoryCount=word(header,12);
        const uint32_t metadataSize=word(header,16);
        if (categoryCount>16 || metadataSize>resource.range.Size()-32)
            return WORLD_AUDIO_RESOURCE_FAILURE;
        const auto metadata=resource.range.Read(32,metadataSize);
        cJukeboxCartridge parsed;
        size_t at=0;
        for (uint32_t group=0;group<categoryCount;++group) {
            const uint32_t count=word(metadata,at);
            const auto groupName=text(metadata,at+4,60);
            if (at>metadata.size() || metadata.size()-at<64 || count>128)
                return WORLD_AUDIO_RESOURCE_FAILURE;
            at+=64;
            for (uint32_t track=0;track<count;++track) {
                if (at>metadata.size() || metadata.size()-at<96)
                    return WORLD_AUDIO_RESOURCE_FAILURE;
                const auto tuneName=text(metadata,at+12,84);
                if (parsed.AddTune(groupName.c_str(),tuneName.c_str(),word(metadata,at),
                    word(metadata,at+4),word(metadata,at+8),0)==UINT32_MAX)
                    return WORLD_AUDIO_RESOURCE_FAILURE;
                at+=96;
            }
        }
        auto stream=std::make_unique<WorldAudioStreamSource>();
        stream->file=resource.range;
        stream->path=path;
        stream->sampleRate=rate;
        const auto result=worldAudioCreateStreamVoice(audio,stream.get(),&currentSample);
        if (result!=WORLD_AUDIO_OK) return result;
        currentSample.instance->completionKind=WORLD_AUDIO_COMPLETION_JUKEBOX;
        currentSample.instance->completionJukebox=this;
        groups=std::move(parsed.groups);
        tunes=std::move(parsed.tunes);
        sampleRate_=rate;
        stream_=std::move(stream);
        return WORLD_AUDIO_OK;
    } catch (const std::bad_alloc&) {
        return WORLD_AUDIO_CAPACITY;
    } catch (const std::exception&) {
        return WORLD_AUDIO_RESOURCE_FAILURE;
    }
}

WorldAudioResult cJukeboxPlayer::Close()
{
    if (currentSample.instance) {
        const auto valid=worldAudioValidateVoice(audio,currentSample);
        if (valid!=WORLD_AUDIO_OK && !closing_) return valid;
        if (!closing_) {
            // Source teardown deletes the sample before releasing metadata.
            // Windows requests retain its file range until acknowledgement.
            // Retire clears the callback only after admitting its STOP.
            // A full queue leaves this live player unchanged for a retry.
            const auto retired=worldAudioRetireVoice(audio,currentSample);
            if (retired!=WORLD_AUDIO_OK) return retired;
            closing_=true;
            // A delivered ready event is deferred until Update. It belongs
            // to this retired sample, never to a later LoadCartridge voice.
            pendingPlay=0;
            state=0;
        }
        if (currentSample.instance->occupied &&
            currentSample.instance->generation==currentSample.generation)
            return WORLD_AUDIO_CAPACITY;
        currentSample={};
        closing_=false;
    }
    stream_.reset();
    groups.clear();
    tunes.clear();
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::Start(uint32_t repeatMode)
{
    repeat=repeatMode;
    return IsCartridgeLoaded() ? Advance() : WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::Start(uint32_t group,uint32_t track,uint32_t repeatMode)
{
    repeat=repeatMode;
    if (!IsCartridgeLoaded()) return WORLD_AUDIO_OK;
    if (IsPlaying() || IsFading()) {
        const auto stopped=StopPlaying();
        if (stopped!=WORLD_AUDIO_OK) return stopped;
    }
    return StartPlaying(group,track);
}

WorldAudioResult cJukeboxPlayer::Start(const char* name,uint32_t repeatMode)
{
    repeat=repeatMode;
    if (!IsCartridgeLoaded()) return WORLD_AUDIO_OK;
    if (IsPlaying() || IsFading()) {
        const auto stopped=StopPlaying();
        if (stopped!=WORLD_AUDIO_OK) return stopped;
    }
    uint32_t group=0,track=0;
    return FindTuneByName(group,track,name) ? StartPlaying(group,track) : WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::Stop()
{
    if (!IsCartridgeLoaded()) return WORLD_AUDIO_OK;
    if (IsPlaying() || IsFading()) {
        const auto stopped=StopPlaying();
        if (stopped!=WORLD_AUDIO_OK) return stopped;
    }
    repeat=0;
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::FadeOut(uint32_t milliseconds)
{
    if (!IsCartridgeLoaded()) return WORLD_AUDIO_OK;
    if (IsPlaying() && !IsFading()) {
        const auto faded=StartFading(milliseconds);
        if (faded!=WORLD_AUDIO_OK) return faded;
    }
    repeat=0;
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::FadeOutAdvance(uint32_t milliseconds)
{
    if (IsCartridgeLoaded() && IsPlaying() && !IsFading()) return StartFading(milliseconds);
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::Advance()
{
    if (!IsCartridgeLoaded()) return WORLD_AUDIO_OK;
    if (IsPlaying() || IsFading()) {
        const auto stopped=StopPlaying();
        if (stopped!=WORLD_AUDIO_OK) return stopped;
    }
    return StartPlaying();
}

WorldAudioResult cJukeboxPlayer::Update()
{
    if (pendingPlay) {
        if (state==5) state=0; // Source Cleanup is empty; no fake backend call.
        else if (!paused) {
            if (!audio || !currentSample.instance) return WORLD_AUDIO_INVALID_BINDING;
            const auto result=worldAudioPlayVoice(audio,currentSample,1.0f,0.0f,0.0f);
            if (result!=WORLD_AUDIO_OK) return result;
        }
        pendingPlay=0;
    }
    if (IsDoneFading()) state=0;
    if (IsDonePlaying()) {
        state=0;
        if (repeat==1) return StartPlaying();
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::SetOn(int32_t on)
{
    if (!on && enabled) {
        const auto result=Stop();
        if (result!=WORLD_AUDIO_OK) return result;
    }
    enabled=on;
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::SampleCallback(uint32_t event,WorldAudioVoiceRef voice)
{
    if (!sameVoice(voice,currentSample)) return WORLD_AUDIO_STALE_REFERENCE;
    switch (event) {
    case 4: pendingPlay=1; break;
    case 32: state=6; break;
    case 8: state=4; break;
    case 64: pendingPlay=1; break;
    default: break;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::StopPlaying()
{
    if (!audio || !currentSample.instance) return WORLD_AUDIO_INVALID_BINDING;
    if (state==2 || state==5) {
        const auto result=worldAudioStopInstance(audio,currentSample.instance);
        if (result!=WORLD_AUDIO_OK) return result;
        state=0;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::Pause(int32_t pause)
{
    if (!audio || !currentSample.instance) return WORLD_AUDIO_INVALID_BINDING;
    return state==2 || state==5 ? worldAudioPauseVoice(audio,currentSample,pause) : WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::StartFading(uint32_t milliseconds)
{
    if (!audio || !currentSample.instance) return WORLD_AUDIO_INVALID_BINDING;
    if (state!=2) return WORLD_AUDIO_OK;
    const auto result=worldAudioFadeVoiceVolume(audio,currentSample,0.0f,milliseconds);
    if (result==WORLD_AUDIO_OK) state=5;
    return result;
}

WorldAudioResult cJukeboxPlayer::StartPlaying(uint32_t group,uint32_t track)
{
    if (!audio || !currentSample.instance || !stream_ || closing_)
        return WORLD_AUDIO_INVALID_BINDING;
    if (!enabled) return WORLD_AUDIO_OK;
    try {
        const auto& tune=Track(group,track);
        const uint64_t offset=uint64_t{GetTuneOffset(group,track)}+0x30;
        if (offset>stream_->file.Size() || tune.size>stream_->file.Size()-offset)
            return WORLD_AUDIO_RESOURCE_FAILURE;
        const auto valid=worldAudioValidateVoice(audio,currentSample);
        if (valid!=WORLD_AUDIO_OK) return valid;
        // PAL opens the stream and sets its category/scale in one owner
        // call. The native retained queue must admit OPEN and SET_VOLUME
        // together before publishing a different current track.
        const size_t required=currentSample.instance->hasHandle ? 2u : 1u;
        if (worldAudioAvailableRequests(audio)<required ||
            required>UINT64_MAX-audio->nextSequence ||
            currentSample.instance->requestReferences>SIZE_MAX-required)
            return audio->result=WORLD_AUDIO_CAPACITY;
        currentGroup=group;
        currentTrack=track;
        // File lifetime is shared, but every command captures its own actual
        // range. Source shuffle can stop/open several tracks in one frame.
        const auto opened=worldAudioOpenStream(audio,currentSample,stream_.get(),offset,tune.size);
        if (opened!=WORLD_AUDIO_OK) return opened;
        const auto categorized=SetCategory(category);
        if (categorized!=WORLD_AUDIO_OK) return categorized;
        const auto scaled=worldAudioSetVoiceScaleVolume(audio,currentSample,volumeScale);
        if (scaled==WORLD_AUDIO_OK) state=2;
        return scaled;
    } catch (const std::out_of_range&) {
        return WORLD_AUDIO_INVALID_BINDING;
    }
}

WorldAudioResult cJukeboxPlayer::StartPlaying()
{
    if (!currentSample.instance) return WORLD_AUDIO_INVALID_BINDING;
    uint32_t group=0,track=0;
    return PickNextTune(group,track) ? StartPlaying(group,track) : WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::SetVolume(float volume)
{
    volumeScale=volume;
    return currentSample.instance ? worldAudioSetVoiceScaleVolume(audio,currentSample,volume) : WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::SetTuneOn(uint32_t group,uint32_t track)
{
    try { tunes.at(groups.at(group).tunes.at(track)).enabled=true; return WORLD_AUDIO_OK; }
    catch (const std::out_of_range&) { return WORLD_AUDIO_INVALID_BINDING; }
}

WorldAudioResult cJukeboxPlayer::SetTuneOff(uint32_t group,uint32_t track)
{
    try { tunes.at(groups.at(group).tunes.at(track)).enabled=false; return WORLD_AUDIO_OK; }
    catch (const std::out_of_range&) { return WORLD_AUDIO_INVALID_BINDING; }
}

bool cJukeboxPlayer::GetTuneOn(uint32_t group,uint32_t track) const
{
    return Track(group,track).enabled;
}

WorldAudioResult cJukeboxPlayer::SetCategoryOn(uint32_t group)
{
    if (group>=groups.size()) return WORLD_AUDIO_INVALID_BINDING;
    groups[group].enabled=true;
    return WORLD_AUDIO_OK;
}

WorldAudioResult cJukeboxPlayer::SetCategoryOff(uint32_t group)
{
    if (group>=groups.size()) return WORLD_AUDIO_INVALID_BINDING;
    groups[group].enabled=false;
    return WORLD_AUDIO_OK;
}

bool cJukeboxPlayer::PickNextTune(uint32_t& group,uint32_t& track)
{
    for (uint32_t groupStep=1;groupStep<=groups.size();++groupStep) {
        const auto selected=(lastCategory+groupStep)%static_cast<uint32_t>(groups.size());
        auto& categoryGroup=groups[selected];
        if (!categoryGroup.enabled) continue;
        for (uint32_t trackStep=1;trackStep<=categoryGroup.tunes.size();++trackStep) {
            const auto selectedTrack=(categoryGroup.cursor+trackStep)%static_cast<uint32_t>(categoryGroup.tunes.size());
            if (!tunes.at(categoryGroup.tunes[selectedTrack]).enabled) continue;
            group=selected;
            lastCategory=selected;
            track=selectedTrack;
            categoryGroup.cursor=selectedTrack;
            return true;
        }
    }
    return false;
}

bool cJukeboxPlayer::FindTuneByName(uint32_t& group,uint32_t& track,const char* name)
{
    if (!name) throw std::invalid_argument("Jukebox tune name is absent");
    for (uint32_t groupStep=1;groupStep<=groups.size();++groupStep) {
        const auto selected=(nameSearchCategoryCursor+groupStep)%static_cast<uint32_t>(groups.size());
        auto& categoryGroup=groups[selected];
        for (uint32_t trackStep=1;trackStep<=categoryGroup.tunes.size();++trackStep) {
            const auto selectedTrack=(categoryGroup.cursor+trackStep)%static_cast<uint32_t>(categoryGroup.tunes.size());
            if (tunes.at(categoryGroup.tunes[selectedTrack]).name!=name) continue;
            group=selected;
            nameSearchCategoryCursor=selected;
            track=selectedTrack;
            categoryGroup.cursor=selectedTrack;
            return true;
        }
    }
    return false;
}

#include "bsound/include/bSound.h"

#include "CarSounds.hpp"
#include "bware/inc/bFile.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <sstream>

namespace {

uint32_t readWord(const std::vector<uint8_t>& bytes,size_t offset)
{
    if (offset>bytes.size() || bytes.size()-offset<4)
        throw std::invalid_argument("Truncated sound-bank header");
    return uint32_t{bytes[offset]} | uint32_t{bytes[offset+1]}<<8 |
        uint32_t{bytes[offset+2]}<<16 | uint32_t{bytes[offset+3]}<<24;
}

} // namespace

struct bSound::BankResource {
    enum class Phase { Loading,Ready,Loaded,Failed,Retiring };
    std::array<char,33> name{};
    std::vector<uint8_t> metadata;
    std::vector<uint8_t> encodedSamples;
    std::vector<std::array<char,33>> names;
    std::vector<WorldAudioCue> cues;
    std::unique_ptr<bRegion> region;
    Phase phase{Phase::Loading};
    uint64_t loadSequence{};
};

bSound::bSound()
{
    categoryVolumes_.fill(1.0f);
    mixer_.channelVolumes=categoryVolumes_.data();
    mixer_.channelControls=categoryVolumes_.data();
    mixer_.channelVolumeCount=categoryVolumes_.size();
    // PAL reserves16 initially; native32-slot capacity is reserved up front
    // so publication cannot fail after a live eager region was constructed.
    regionOrder_.reserve(bankCapacity);
}

bSound::~bSound()
{
    ShutdownAfterOwnersRetired();
}

std::string bSound::FailureDescription() const
{
#ifdef _WIN32
    if (windowsFailure_.result!=WORLD_AUDIO_OK) {
        std::ostringstream message;
        message<<windowsFailure_.reason<<" [operation="<<static_cast<int>(windowsFailure_.kind)
            <<", sequence="<<windowsFailure_.sequence<<", HRESULT=0x"<<std::hex
            <<static_cast<uint32_t>(windowsFailure_.systemCode)<<']';
        return message.str();
    }
#endif
    return resourceFailure_;
}

void bSound::ClearSourceFailure() noexcept
{
    // This owner also stores real file-load failures. Clear only the source error
    // emitted below; never discard a pre-existing resource/backend failure.
    if (resourceFailure_.rfind("[source constructor] ",0)==0)resourceFailure_.clear();
}

void bSound::RecordSourceFailure(WorldAudioResult result,const char* stage,const char* reason) noexcept
{
    if (result==WORLD_AUDIO_OK || !resourceFailure_.empty()) return;
    try {
        resourceFailure_=std::string("[source constructor] ")+(stage ? stage : "Sound source")+
            " failed, audio result="+std::to_string(static_cast<int>(result))+
            "; "+(reason ? reason : "No source exception detail");
    } catch (...) {
        // Formatting a source error must not replace the original
        // result, throw across a factory boundary, or interfere with rollback.
    }
}

bSound* bSound::getInstance()
{
    static bSound instance;
    return &instance;
}


WorldAudioResult bSound::Bind(WorldAudioRequestQueue& queue,
    const hp2::GameResources& files,const int32_t& spatialEnabled)
{
    if (queue_ || queue.library || queue.banks || queue.bankCount ||
        queue.instances || queue.instanceCapacity || queue.retainedCount || !queue.generation)
        return WORLD_AUDIO_INVALID_BINDING;
    resourceFailure_.clear();
    terminalFailure_=WORLD_AUDIO_OK;
#ifdef _WIN32
    windowsFailure_={};
#endif
    queue_=&queue;
    files_=&files;
    worldAudioBindInstances(&queue,samples_.data(),samples_.size(),&spatialEnabled);
    if (queue.result!=WORLD_AUDIO_OK) {
        queue_=nullptr;
        files_=nullptr;
        return queue.result;
    }
    queue.library=this;
    queue.banks=banks_.data();
    queue.bankCount=banks_.size();
    return voices_.Bind(queue);
}

size_t bSound::SampleCapacity() const noexcept
{
    return samples_.size()+expandedSamples_.size()*sampleGrowthCapacity;
}

const WorldAudioInstance* bSound::SampleAt(size_t index) const noexcept
{
    if (index<samples_.size()) return &samples_[index];
    index-=samples_.size();
    for (const auto& block:expandedSamples_) {
        if (index<block.size()) return &block[index];
        index-=block.size();
    }
    return nullptr;
}

WorldAudioInstance* bSound::SampleAt(size_t index) noexcept
{
    return const_cast<WorldAudioInstance*>(static_cast<const bSound*>(this)->SampleAt(index));
}

bool bSound::OwnsSample(const WorldAudioInstance* sample) const noexcept
{
    const auto address=reinterpret_cast<uintptr_t>(sample);
    const auto contains=[address](const auto& block) {
        const auto first=reinterpret_cast<uintptr_t>(block.data());
        return address>=first && address-first<block.size()*sizeof(WorldAudioInstance) &&
            (address-first)%sizeof(WorldAudioInstance)==0;
    };
    if (contains(samples_)) return true;
    for (const auto& block:expandedSamples_) if (contains(block)) return true;
    return false;
}

WorldAudioResult bSound::ExpandSampleStorage()
{
    if (!queue_ || queue_->library!=this || queue_->instances!=samples_.data() ||
        queue_->instanceCapacity!=SampleCapacity()) return WORLD_AUDIO_INVALID_BINDING;
    if (SampleCapacity()>SIZE_MAX-sampleGrowthCapacity ||
        SampleCapacity()+sampleGrowthCapacity>SIZE_MAX/sizeof(WorldAudioInstance) ||
        expandedSamples_.size()==expandedSamples_.max_size()) return WORLD_AUDIO_CAPACITY;
    try {
        // PAL SlotPool starts at255, enables expansion (flags0x0F), and
        // links each64-slot block without moving any allocated old slot.
        // std::list value-initializes the new array before publishing it.
        expandedSamples_.emplace_back();
    } catch (const std::bad_alloc&) {
        return WORLD_AUDIO_CAPACITY;
    }
    queue_->instanceCapacity=SampleCapacity();
    return WORLD_AUDIO_OK;
}

WorldAudioResult bSound::BindSoundClient(const CarSoundBindings* client)
{
    if (client && (!queue_ || client->audio!=queue_ || client->manager!=&mixer_))
        return WORLD_AUDIO_INVALID_BINDING;
    client_=client;
    return WORLD_AUDIO_OK;
}

WorldAudioResult bSound::Initialize()
{
    if (!queue_ || !files_ || status_) return WORLD_AUDIO_INVALID_BINDING;
    const auto allocation=bRegion::Initialize(32);
    if (allocation!=WORLD_AUDIO_OK) return allocation;
    const auto result=worldAudioSubmitDeviceInitialization(queue_);
    if (result!=WORLD_AUDIO_OK) return result;
    status_=1;
    return WORLD_AUDIO_OK;
}

WorldAudioRequestQueue* bSound::Driver() const noexcept
{
    return queue_;
}

const WorldAudioManagerState& bSound::Mixer() const noexcept
{
    return mixer_;
}

bRegion* bSound::CommonRegion() const noexcept
{
    return common_;
}

int32_t bSound::GetStatus() const noexcept
{
    return status_;
}

bRegion* bSound::Region(const char* name) const
{
    if (!name) throw std::invalid_argument("Sound region name is absent");
    for (const size_t index:regionOrder_) {
        const auto& resource=*resources_[index];
        if (std::strcmp(resource.region->GetName(),name)==0) return resource.region.get();
    }
    return nullptr;
}

WorldAudioResult bSound::SetPath(const char* path)
{
    if (!queue_ || !path || std::strlen(path)>=path_.size())
        return WORLD_AUDIO_INVALID_BINDING;
    const auto result=worldAudioSubmitRegionPath(queue_,path);
    if (result!=WORLD_AUDIO_OK) return result;
    std::memcpy(path_.data(),path,std::strlen(path)+1);
    return WORLD_AUDIO_OK;
}

WorldAudioResult bSound::LoadRegion(const char* name)
{
    if (status_!=1 || !queue_ || !files_ || !name || !*name || std::strlen(name)>32)
        return WORLD_AUDIO_INVALID_BINDING;
    // Final PAL rejects another pending region. Keep a checked failure
    // instead of reproducing the source's preceding unsafe SDK submission.
    if (pendingCount_) return WORLD_AUDIO_CAPACITY;
    if (Region(name)) return WORLD_AUDIO_INVALID_BINDING;
    reclaimRetiredBanks();
    size_t index=0;
    while (index<resources_.size() && resources_[index]) ++index;
    if (index==resources_.size()) return WORLD_AUDIO_CAPACITY;
    if (worldAudioAvailableRequests(queue_)<1) return WORLD_AUDIO_CAPACITY;

    auto resource=std::make_unique<BankResource>();
    try {
        resource->metadata=files_->Load(std::string(path_.data())+name+".msh");
        resource->encodedSamples=files_->Load(std::string(path_.data())+name+".msb");
        const auto& bytes=resource->metadata;
        if (bytes.size()<12 || readWord(bytes,0)!=bytes.size())
            throw std::invalid_argument("MSH header size does not match the retained file");
        const size_t count=readWord(bytes,8);
        if (count>(bytes.size()-12)/52)
            throw std::invalid_argument("MSH master table exceeds the retained file");
        resource->names.reserve(count);
        resource->cues.reserve(count);
        // Sony metadata accessors264CF8/264E10 step authored52-byte records;
        // type267F58 is the high nibble of byte21. Region owns masters only.
        for (size_t sample=0;sample<count;++sample) {
            const size_t at=12+sample*52;
            if (bytes[at+0x21]>>4) continue;
            std::array<char,33> cueName{};
            std::memcpy(cueName.data(),bytes.data()+at,32);
            if (!cueName[0]) throw std::invalid_argument("MSH master has an empty cue name");
            resource->names.push_back(cueName);
            resource->cues.push_back({nullptr,0,bytes.data()+at,52});
        }
        for (size_t cue=0;cue<resource->cues.size();++cue)
            resource->cues[cue].name=resource->names[cue].data();
        std::memcpy(resource->name.data(),name,std::strlen(name)+1);
    } catch (const std::bad_alloc&) {
        return WORLD_AUDIO_CAPACITY;
    } catch (const std::exception& error) {
        resourceFailure_=std::string("Sound bank '")+path_.data()+name+"': "+error.what();
        return WORLD_AUDIO_RESOURCE_FAILURE;
    }

    WorldAudioBank& bank=banks_[index];
    // Metadata is available to the backend immediately, but game lookup
    // cannot discover this bank until genuine completion creates its region.
    bank.metadata=resource->metadata.data();
    bank.metadataSize=resource->metadata.size();
    bank.encodedSamples=resource->encodedSamples.data();
    bank.encodedSampleSize=resource->encodedSamples.size();
    resource->loadSequence=queue_->nextSequence;
    const auto result=worldAudioSubmitRegionLoad(queue_,&bank,name);
    if (result!=WORLD_AUDIO_OK) {
        bank={};
        return result;
    }
    resources_[index]=std::move(resource);

    return addPendingRegion(name);
}

WorldAudioResult bSound::addPendingRegion(const char* name)
{
    if (!name || pendingCount_==pending_.size() || std::strlen(name)>=pending_[0].size())
        return WORLD_AUDIO_CAPACITY;
    std::memcpy(pending_[pendingCount_++].data(),name,std::strlen(name)+1);
    return WORLD_AUDIO_OK;
}

void bSound::removePendingRegion(const char* name)
{
    (void)name; // PAL ignores the argument and clears the last pending slot.
    if (pendingCount_) pending_[--pendingCount_][0]=0;
}

WorldAudioResult bSound::CompleteRegion(const WorldAudioRequest& operation,bool loaded)
{
    if (!queue_ || operation.kind!=WORLD_AUDIO_LOAD_REGION ||
        operation.queueGeneration!=queue_->generation || operation.slot>=queue_->capacity)
        return WORLD_AUDIO_STALE_REFERENCE;
    const auto& live=queue_->requests[operation.slot];
    if (live.deliveryState!=WORLD_AUDIO_REQUEST_INFLIGHT ||
        live.sequence!=operation.sequence || live.queueGeneration!=operation.queueGeneration ||
        live.bank!=operation.bank || live.kind!=operation.kind)
        return WORLD_AUDIO_STALE_REFERENCE;
    for (size_t index=0;index<resources_.size();++index) {
        auto& resource=resources_[index];
        if (operation.bank!=&banks_[index] || !resource ||
            operation.sequence!=resource->loadSequence) continue;
        if (resource->phase!=BankResource::Phase::Loading)
            return WORLD_AUDIO_STALE_REFERENCE;
        resource->phase=loaded ? BankResource::Phase::Ready : BankResource::Phase::Retiring;

        if (!loaded) removePendingRegion(resource->name.data());
        return loaded ? WORLD_AUDIO_OK : WORLD_AUDIO_BACKEND_FAILURE;
    }
    return WORLD_AUDIO_STALE_REFERENCE;
}

WorldAudioResult bSound::notifyClient(uint32_t event,bRegion* region)
{
    if (!client_) return WORLD_AUDIO_OK; // Source callback is optional.
    return soundDriverCallback(*client_,event,region);
}

WorldAudioResult bSound::updatePendingRegions()
{
    for (size_t pending=0;pending<pendingCount_;++pending) {
        const char* name=getPendingRegion(static_cast<int32_t>(pending));
        for (size_t index=0;index<resources_.size();++index) {
            auto& resource=resources_[index];
            if (!resource || std::strcmp(resource->name.data(),name)!=0) continue;
            // The pending name belongs to the current load, not an older
            // same-name resource whose real UNLOAD is still draining.
            if (resource->phase==BankResource::Phase::Retiring)continue;
            if (resource->phase==BankResource::Phase::Failed) return WORLD_AUDIO_BACKEND_FAILURE;
            if (resource->phase!=BankResource::Phase::Ready) break;
            auto& bank=banks_[index];
            bank.name=resource->name.data();
            bank.cues=resource->cues.data();
            bank.cueCount=resource->cues.size();
            try {
                resource->region=std::make_unique<bRegion>(*queue_,bank);
                regionOrder_.push_back(index);
            } catch (const std::bad_alloc&) {
                bank.name=nullptr;
                return WORLD_AUDIO_CAPACITY;
            } catch (const std::exception&) {
                bank.name=nullptr;
                return WORLD_AUDIO_INVALID_BINDING;
            }
            resource->phase=BankResource::Phase::Loaded;

            // InitializeSoundDriver1AE6B8 passes final330EF8 "common".
            // Some retained callers use "Common". The physical bank name
            // is case insensitive; both refer to this same published bank.
            bool common=std::strlen(name)==6;
            constexpr char commonName[]="common";
            for(size_t c=0;common && c<6;++c) {
                const auto letter=static_cast<unsigned char>(name[c]);
                common=static_cast<unsigned char>(letter>='A' && letter<='Z'?letter+('a'-'A'):letter)==
                    static_cast<unsigned char>(commonName[c]);
            }
            if (common) {
                common_=resource->region.get();
                mixer_.frontendBank=&bank; // Existing native name for Common.
            }
            removePendingRegion(resource->region->GetName());
            const auto result=notifyClient(2,resource->region.get());
            if (result!=WORLD_AUDIO_OK) return result;
            break;
        }
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult bSound::Update()
{
    if (status_!=1) return WORLD_AUDIO_OK;
    const auto result=worldAudioUpdateDriver(queue_);
    if (result!=WORLD_AUDIO_OK) return result;
    // Device update/completions use this same transport and checked voice
    // generation. The console updateCompletedVoices method is truly empty.
    const auto pending=updatePendingRegions();
    reclaimRetiredBanks();
    return pending;
}

WorldAudioResult bSound::UnloadRegion(const char* name)
{
    if (!queue_ || !name) return WORLD_AUDIO_INVALID_BINDING;
    for (auto at=regionOrder_.begin();at!=regionOrder_.end();++at) {
        const size_t index=*at;
        auto& resource=*resources_[index];
        if (std::strcmp(resource.region->GetName(),name)!=0) continue;

        const auto result=resource.region->Dispose();
        if (result!=WORLD_AUDIO_OK) return result;
        // Original region destruction deletes masters, then BankUnload
        // stops the physical bank. Private bSamples retain their names and
        // identity; they do not veto the original region lifecycle.
        for (size_t slot=0;slot<SampleCapacity();++slot) {
            auto& sample=*SampleAt(slot);
            if (!sample.occupied || sample.bankSample || sample.bank!=&banks_[index])continue;
            sample.hasHandle=0;sample.playing=0;sample.paused=0;sample.prebuffered=0;
            sample.playSequence=UINT64_MAX;
        }
        if (queue_->inGameBank==&banks_[index]) queue_->inGameBank=nullptr;
        if (common_==resource.region.get()) {
            common_=nullptr;
            mixer_.frontendBank=nullptr;
        }
        resource.region.reset();
        regionOrder_.erase(at);
        resource.phase=BankResource::Phase::Retiring;
        banks_[index].name=nullptr;
        reclaimRetiredBanks();
        return WORLD_AUDIO_OK;
    }
    return WORLD_AUDIO_INVALID_BINDING;
}

bool bSound::OwnsLoadedBank(const WorldAudioBank* bank) const noexcept
{
    for (const size_t index:regionOrder_) if (bank==&banks_[index]) return true;
    return false;
}

bool bSound::bankBorrowed(const WorldAudioBank* bank) const noexcept
{
    for (size_t index=0;index<SampleCapacity();++index) {
        const auto& sample=*SampleAt(index);
        if (sample.occupied && sample.bank==bank) return true;
    }
    for (size_t index=0;index<queue_->capacity;++index) {
        const auto& request=queue_->requests[index];
        if (request.deliveryState!=WORLD_AUDIO_REQUEST_FREE && request.bank==bank) return true;
    }
    return false;
}

void bSound::reclaimRetiredBanks()
{
    if (!queue_) return;
    for (size_t index=0;index<resources_.size();++index) {
        const auto& resource=resources_[index];
        if (!resource || resource->phase!=BankResource::Phase::Retiring)continue;
        bool transportBorrow=false;
        for (size_t request=0;request<queue_->capacity;++request) {
            const auto& operation=queue_->requests[request];
            if (operation.deliveryState!=WORLD_AUDIO_REQUEST_FREE && operation.bank==&banks_[index]) {
                transportBorrow=true;break;
            }
        }
        if (transportBorrow)continue;
        // Real consumer completion releases PCM/metadata. A dormant source
        // object still owns its original names, not every earlier MSB.
        for (size_t slot=0;slot<SampleCapacity();++slot) {
            auto& sample=*SampleAt(slot);
            if (sample.occupied && !sample.bankSample && sample.bank==&banks_[index] &&
                !sample.hasHandle && !sample.requestReferences) {
                sample.bank=nullptr;sample.cue=nullptr;
            }
        }
        if (!bankBorrowed(&banks_[index])) {

            resources_[index].reset();
            banks_[index]={};
        }
    }
}

float bSound::GetCategoryVolume(uint32_t category) const noexcept
{
    return category<categoryVolumes_.size() ? categoryVolumes_[category] : 1.0f;
}

WorldAudioResult bSound::SetCategoryVolume(uint32_t category,float volume)
{
    if (category>=categoryVolumes_.size()) return WORLD_AUDIO_OK;
    if (!queue_) return WORLD_AUDIO_INVALID_BINDING;
    size_t needed=1;
    for (size_t index=0;index<SampleCapacity();++index) {
        const auto& sample=*SampleAt(index);
        if (sample.occupied && !sample.retiring && sample.hasHandle && sample.category==category)
            ++needed;
    }
    if (worldAudioAvailableRequests(queue_)<needed) return WORLD_AUDIO_CAPACITY;
    const auto result=worldAudioSubmitChannelVolume(queue_,&mixer_,static_cast<int32_t>(category),volume);
    if (result!=WORLD_AUDIO_OK) return result;
    // This is the native device's live-voice view, replacing the console's
    // fixed48 hardware voice slots. No copied mixer/sample array is created.
    for (size_t index=0;index<SampleCapacity();++index) {
        auto& sample=*SampleAt(index);
        if (!sample.occupied || sample.retiring || !sample.hasHandle || sample.category!=category) continue;
        const auto refreshed=worldAudioSetVoiceVolume(queue_,worldAudioVoiceReference(&sample),sample.volume);
        if (refreshed!=WORLD_AUDIO_OK) return refreshed;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult bSound::SetMode(int32_t mode)
{
    if (!queue_) return WORLD_AUDIO_INVALID_BINDING;
    if (mode<0 || mode>2) return WORLD_AUDIO_OK;
    return worldAudioSetMode(queue_,mode);
}


void bSound::Printf(const char* format,...) const
{
    if (diagnosticChannel_==-1 || !format) return;
    va_list arguments;
    va_start(arguments,format);
    std::vfprintf(stderr,format,arguments);
    va_end(arguments);
}

void bSound::EnablePrintf(uint32_t channel) noexcept
{
    diagnosticChannel_=static_cast<int32_t>(channel);
}

void bSound::DisablePrintf() noexcept
{
    diagnosticChannel_=-1;
}

WorldAudioResult bSound::Shutdown()
{
#ifdef _WIN32
    if (windowsDevice_) return WORLD_AUDIO_INVALID_BINDING;
#endif
    if (!queue_) return WORLD_AUDIO_OK;
    // A LOAD can have completed on the device immediately before startup
    // failed, without Update constructing the public CPU region yet.
    for (auto& resource:resources_) {
        if (resource && resource->phase==BankResource::Phase::Ready) {
            resource->phase=BankResource::Phase::Retiring;
            removePendingRegion(resource->name.data());
        }
    }
    if (pendingCount_) return WORLD_AUDIO_INVALID_BINDING;
    while (!regionOrder_.empty()) {
        const auto index=regionOrder_.back();
        const auto result=UnloadRegion(resources_[index]->name.data());
        if (result!=WORLD_AUDIO_OK) return result;
    }
    reclaimRetiredBanks();
    for (const auto& resource:resources_) if (resource) return WORLD_AUDIO_CAPACITY;
    if (queue_->retainedCount) return WORLD_AUDIO_CAPACITY;
    for (size_t index=0;index<SampleCapacity();++index)
        if (SampleAt(index)->occupied) return WORLD_AUDIO_INVALID_BINDING;
    const auto voicesReleased=voices_.Unbind();
    if (voicesReleased!=WORLD_AUDIO_OK)return voicesReleased;
    queue_->banks=nullptr;
    queue_->bankCount=0;
    queue_->inGameBank=nullptr;
    queue_->instances=nullptr;
    queue_->instanceCapacity=0;
    expandedSamples_.clear(); // All sample/request/device borrows ended above.
    queue_->spatialEnabled=nullptr;
    queue_->library=nullptr;
    queue_=nullptr;
    files_=nullptr;
    client_=nullptr;
    status_=0;
    return WORLD_AUDIO_OK;
}

WorldAudioResult bSound::ShutdownAfterOwnersRetired() noexcept
{
    const auto remember=[this](WorldAudioResult result) {
        if (result!=WORLD_AUDIO_OK && terminalFailure_==WORLD_AUDIO_OK)
            terminalFailure_=result;
        if (queue_ && terminalFailure_!=WORLD_AUDIO_OK)queue_->result=terminalFailure_;
        return terminalFailure_;
    };
    if (!queue_)return terminalFailure_;
    try {
#ifdef _WIN32
        if (windowsDevice_) {
            const auto closed=CloseWindowsDevice();
            if (closed!=WORLD_AUDIO_OK)return remember(closed);
        }
#endif
        // This rejects a live physical voice association. No payload is
        // released until the real consumer and all retained requests end.
        const auto cancelled=CancelDeviceRequestsAfterClose();
        if (cancelled!=WORLD_AUDIO_OK)return remember(cancelled);
        for (size_t index=0;index<SampleCapacity();++index) {
            auto& sample=*SampleAt(index);
            if (!sample.occupied || sample.bankSample)continue;
            // Normal cancellation also drains the deferred movie STOP. An
            // occupied retiring sample here is an unresolved invariant,
            // not permission to zero a sample or erase its transport pins.
            remember(worldAudioRetireVoice(queue_,worldAudioVoiceReference(&sample)));
        }
        for (;;) {
            const auto result=Shutdown();
            if (result!=WORLD_AUDIO_CAPACITY || !queue_ || !queue_->retainedCount)
                return remember(result);
            // Unloading real banks can fill a small queue. Cancel those
            // actual commands after the consumer joined, then resume the
            // same bank ledger. Every successful drain makes progress.
            const auto drained=CancelDeviceRequestsAfterClose();
            if (drained!=WORLD_AUDIO_OK)return remember(drained);
        }
    } catch (const std::exception& error) {
        RecordSourceFailure(WORLD_AUDIO_BACKEND_FAILURE,"Terminal retirement",error.what());
        return remember(WORLD_AUDIO_BACKEND_FAILURE);
    } catch (...) {
        RecordSourceFailure(WORLD_AUDIO_BACKEND_FAILURE,"Terminal retirement","Unknown cleanup exception");
        return remember(WORLD_AUDIO_BACKEND_FAILURE);
    }
}

WorldAudioResult bSound::CancelDeviceRequestsAfterClose()
{
#ifdef _WIN32
    if (windowsDevice_) return WORLD_AUDIO_INVALID_BINDING;
#endif
    if (!queue_) return WORLD_AUDIO_OK;
    for (const auto& voice:voices_.Voices())
        if (voice.GetSample().instance) return WORLD_AUDIO_INVALID_BINDING;
    // PRIME publishes a requested handle before it reaches the device. Once
    // the actual consumer is gone neither applied nor queued voices exist.
    for (size_t index=0;index<SampleCapacity();++index) {
        auto& sample=*SampleAt(index);
        if (!sample.occupied) continue;
        sample.hasHandle=0;
        sample.playing=0;
        sample.prebuffered=0;
    }
    const auto reject=[&](const WorldAudioRequest& request) {
        if (request.kind==WORLD_AUDIO_LOAD_REGION) {
            const auto result=CompleteRegion(request,false);
            if (result!=WORLD_AUDIO_BACKEND_FAILURE) return result;
        }
        return worldAudioReject(queue_,&request)==WORLD_AUDIO_BACKEND_FAILURE ?
            WORLD_AUDIO_OK:WORLD_AUDIO_STALE_REFERENCE;
    };
    // The failed operation was already taken; later operations are queued.
    for (size_t index=0;index<queue_->capacity;++index) {
        if (queue_->requests[index].deliveryState!=WORLD_AUDIO_REQUEST_INFLIGHT) continue;
        const auto operation=queue_->requests[index];
        const auto result=reject(operation);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    WorldAudioRequest operation{};
    while (worldAudioTake(queue_,&operation)) {
        const auto result=reject(operation);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    return queue_->retainedCount ? WORLD_AUDIO_CAPACITY:WORLD_AUDIO_OK;
}

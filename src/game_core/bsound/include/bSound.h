#ifndef NFSHP2_BSOUND_H
#define NFSHP2_BSOUND_H

#include "bsound/include/bRegion.h"
#include "bsound/include/bVoice.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include <memory>
#include <list>
#include <string>

namespace hp2 { class GameResources; }
struct CarSoundBindings;

namespace hp2 {
// Individually matched bDriverPS2 / bDriverLowPS2 routines. Bounds are
// native capabilities; these records never reproduce a console object.
uint16_t GetAttack(uint32_t envelope) noexcept;
uint16_t GetRelease(uint32_t envelope) noexcept;
uint8_t GetChannelNum(uint8_t flags) noexcept;
uint8_t GetLoop(uint8_t flags) noexcept;
int8_t GetPanFromAzimuth(uint16_t azimuth) noexcept;
uint16_t GetAzimuthFromPan(int8_t pan);
std::array<uint16_t,2> GetVolumesFromPan(uint16_t volume,int8_t pan,uint8_t mode);

void GetVolumesFromAzimuth(std::array<int32_t,2>& volume,
    const std::array<int32_t,2>& previous,int32_t azimuth,uint8_t mode) noexcept;
// Only the phase decisions needed by the authored surround policy survive.
// No direct-volume register words or console playback state are retained.
struct NativeVoicePhase {
    uint8_t comparison{2};
    std::array<bool,2> inverted{};
};
// The real Windows device owns signed floating-point channel coefficients.
// Authored gain/pan units are converted once when a command changes the mix.
struct NativeVoiceMix {
    uint16_t channels{},volume{0x3FFF};
    bool resident{};
    std::array<int8_t,2> pan{};
    std::array<uint16_t,2> azimuth{};
    std::array<NativeVoicePhase,2> phaseHistory{};
    std::array<std::array<float,2>,2> output{};
    void Initialize(uint16_t sourceChannels,bool residentSample,uint16_t gain,
        int8_t requestedPan,uint8_t mode);
    void SetVolume(uint16_t gain,uint8_t mode);
    void Mute(bool muted,uint16_t& savedVolume,uint8_t mode);
    void SetAzimuth(uint16_t angle,uint8_t mode);
    void SetMode(uint8_t mode);
    std::array<float,4> Coefficients() const;
private:
    void SetChannelAzimuth(size_t channel,uint16_t angle,uint8_t mode);
};
uint32_t SOUND_GetMSHSampleCount(const uint8_t* bytes,size_t size);
int32_t SOUND_GetMSHInfo(const uint8_t* bytes,size_t size,uint32_t index,
    std::array<uint32_t,7>& fields);
int32_t SOUND_GetMIHInfo(const uint8_t* bytes,size_t size,uint32_t index,
    std::array<uint32_t,7>& fields);
struct bSoundWaveHeader { uint32_t dataBytes{},dataOffset{},channelCode{}; };
void SOUND_GetWAVHeaderInfo(const uint8_t* bytes,size_t size,bSoundWaveHeader& header);
int32_t SOUND_ScaleVol(int32_t master,int32_t volume) noexcept;
std::array<int32_t,2> SOUND_MixVol(int32_t group,uint32_t channel,
    const std::array<int16_t,4>& masterLeft,const std::array<int16_t,4>& masterRight,
    const std::array<int16_t,48>& left,const std::array<int16_t,48>& right);

struct PcmWave {
    uint32_t sampleRate{};
    uint16_t channels{};
    std::vector<int16_t> samples;
    uint32_t loopStartFrame{},loopEndFrame{};
};
// Native Windows format boundary, not an original ELF body. Only an actual
// RIFF/WAVE PCM descriptor authorizes conversion; AVI01wb is not such input.
PcmWave DecodePcmWave(const uint8_t* bytes,size_t size);
// Native SPU-data conversion. Channel layout must come from the retained
// resource descriptor; a guessed stereo interleave is never accepted.
struct Ps2AdpcmHistory {
    std::array<std::array<int32_t,2>,2> channels{};
};
PcmWave DecodePs2Adpcm(const uint8_t* bytes,size_t size,uint32_t sampleRate,
    uint16_t channels,size_t interleaveBytes,bool stopAtEnd,Ps2AdpcmHistory* history=nullptr);
void DecodePs2AdpcmInto(const uint8_t* bytes,size_t size,uint32_t sampleRate,
    uint16_t channels,size_t interleaveBytes,bool stopAtEnd,Ps2AdpcmHistory* history,PcmWave& result);
// Read one complete final movie channel pair from the original MoviePlayer
// callback. False leaves both PCM and predictor history unchanged; it is not
// an EOF, device completion or permission to submit its zeroed short read.
bool ReadMoviePcmChunk(const WorldAudioStreamSource& source,Ps2AdpcmHistory& history,PcmWave& pcm);
PcmWave DecodeSoundStream(const uint8_t* bytes,size_t size,uint32_t sampleRate,
    uint16_t channels);
// Decode the original captured file request through the native format adapter.
PcmWave DecodeSoundStreamRange(const WorldAudioRequest& operation);
// Immutable asynchronous decode input. The worker owns the physical file
// range and format; it never borrows a mutable sample, player or request ring.
struct NativeStreamDecodeRange {
    hp2::FileRange file;
    uint64_t offset{},bytes{};
    uint32_t sampleRate{};
    uint16_t channels{};
    bool jukebox{};
};
NativeStreamDecodeRange CaptureStreamDecodeRange(const WorldAudioRequest& operation);
PcmWave DecodeStreamRangeSnapshot(const NativeStreamDecodeRange& source);
PcmWave DecodeSoundBankCue(const WorldAudioBank& bank,size_t metadataOffset);

// Native file/PCM policy for the actual JBC/VAGx stream. It retains only the
// immutable physical range, two channel predictors and one bounded chunk.
// Channel stride is proved from the declared extent and paired end markers;
// it is not a console FIFO, guessed interleave or whole-track PCM cache.
class NativeJukeboxStream final {
public:
    static constexpr size_t FramesPerChunk=28u*1024u;
    struct Chunk { PcmWave pcm;bool last{}; };
    explicit NativeJukeboxStream(NativeStreamDecodeRange source);
    Chunk Read();
    uint64_t TotalFrames() const noexcept { return totalFrames_; }
    uint64_t DecodedFrames() const noexcept { return decodedFrames_; }
    uint64_t BytesRead() const noexcept { return bytesRead_; }
    size_t MaximumReadBytes() const noexcept { return maximumReadBytes_; }
    size_t InterleaveBytes() const noexcept { return interleave_; }
    bool Finished() const noexcept { return decodedFrames_==totalFrames_; }
private:
    hp2::FileRange file_;
    uint32_t rate_{};
    size_t interleave_{},channelBytes_{},logicalBytes_{};
    uint64_t totalFrames_{},decodedFrames_{},bytesRead_{};
    size_t maximumReadBytes_{};
    Ps2AdpcmHistory history_{};
    void ReadBytes(uint64_t offset,void* destination,size_t count);
    uint64_t Physical(size_t logical,size_t channel) const noexcept;
};
struct NativeJukeboxStart {
    std::unique_ptr<NativeJukeboxStream> stream;
    NativeJukeboxStream::Chunk first;
};
NativeJukeboxStart PrepareJukeboxStream(const NativeStreamDecodeRange& source);

// Bank metadata and encoded bytes stay with the original bSound owner.
// Extents are indexed once; decoded resident PCM is shared only while a
// physical voice holds it. Weak cache entries cannot pin every idle cue.
class NativeSoundBankCache final {
public:
    explicit NativeSoundBankCache(const WorldAudioBank& bank);
    std::shared_ptr<const PcmWave> Acquire(size_t metadataOffset);
    size_t CueCount() const noexcept { return entries_.size(); }
    size_t DecodeCount() const noexcept { return decodeCount_; }
    size_t ResidentBytes() const;
private:
    struct Entry {
        size_t offset{},bytes{};
        uint32_t rate{};
        bool resident{};
        std::weak_ptr<const PcmWave> pcm;
    };
    const WorldAudioBank* bank_;
    std::vector<Entry> entries_;
    size_t decodeCount_{};
};

// Native consumer policy belongs to this individually proven audio owner.
// This portable implementation is shared with the real Windows device and
// the PCM regression fixture; it is not a console runtime or source ABI.
enum class NativeAudioApply { Blocked, Applied, Cancelled };

// Port-side device delegation. The request pump retains ordering and ACK ownership.
class NativeAudioVoiceOutput {
public:
    virtual ~NativeAudioVoiceOutput() = default;
    virtual NativeAudioApply Apply(bSound& sound, const WorldAudioRequest& operation) = 0;
    virtual WorldAudioResult Observe(bVoiceStatusSnapshot& status) = 0;
    virtual void SetMode(uint8_t mode) = 0;
    virtual void Close(bSound& sound) = 0;
};

// Owner-thread scheduling of the existing retained request ring. Tokens
// borrow its INFLIGHT records; payloads, instance references and retirement
// remain owned by that ring. An asynchronous file decode must not prevent
// unrelated live voices from consuming STOP, gain or completion observations.
class NativeAudioCommandPump {
public:
    const std::optional<WorldAudioRequest>& Current() const noexcept { return current_; }
    std::size_t PendingCount() const noexcept { return pending_.size(); }

    template<class Apply>
    WorldAudioResult Pump(WorldAudioRequestQueue& queue, std::size_t maximum, Apply&& apply) {
        // An acknowledged ring record may have released its final instance
        // borrow. Diagnostic context must never retain that raw owner into
        // a subsequent allocation, validation or callback.
        current_.reset();
        if (!queue.generation || !queue.requests || !queue.capacity) return WORLD_AUDIO_INVALID_BINDING;
        if (generation_ && generation_ != queue.generation) return WORLD_AUDIO_STALE_REFERENCE;
        generation_ = queue.generation;
        if (pending_.capacity() < queue.capacity) pending_.reserve(queue.capacity);
        std::size_t attempts = 0, index = 0;
        while (attempts < maximum) {
            if (index == pending_.size()) {
                WorldAudioRequest next{};
                if (!worldAudioTake(&queue, &next)) break;
                pending_.push_back({next.slot, next.sequence});
            }
            const auto token = pending_[index];
            if (token.slot >= queue.capacity) return WORLD_AUDIO_STALE_REFERENCE;
            const auto& retained = queue.requests[token.slot];
            if (retained.sequence != token.sequence || retained.queueGeneration != generation_ ||
                retained.deliveryState != WORLD_AUDIO_REQUEST_INFLIGHT) return WORLD_AUDIO_STALE_REFERENCE;

            // A global device/bank change cannot overtake any older borrow,
            // and nothing beyond it may overtake the global change itself.
            if (GlobalBarrier(retained.kind) && index != 0) break;
            bool depends = false;
            for (std::size_t older = 0; older < index && !depends; ++older) {
                const auto& before = queue.requests[pending_[older].slot];
                depends = SharesVoiceOrChannels(before, retained);
            }
            if (depends) { ++index; continue; }

            current_ = retained; // Capture before callback/ACK can retire a slot.
            ++attempts;
            const auto result = apply(*current_);
            if (result == NativeAudioApply::Blocked) {
                // Only an owned asynchronous voice or a global barrier can
                // wait. Unowned CPU publications must actually be applied.
                if (!retained.instance && !GlobalBarrier(retained.kind)) return WORLD_AUDIO_INVALID_BINDING;
                current_.reset();
                if (GlobalBarrier(retained.kind)) break;
                ++index;
                continue;
            }
            const auto completed = result == NativeAudioApply::Applied ?
                worldAudioAcknowledge(&queue, &*current_) : worldAudioReject(&queue, &*current_);
            if (completed != (result == NativeAudioApply::Applied ? WORLD_AUDIO_OK : WORLD_AUDIO_BACKEND_FAILURE))
                return completed;
            current_.reset();
            pending_.erase(pending_.begin() + static_cast<std::ptrdiff_t>(index));
            // Later ACKs may wait behind an earlier INFLIGHT slot. The ring
            // alone releases that contiguous retired prefix when it is safe.
        }
        return WORLD_AUDIO_OK;
    }

private:
    struct Token { std::size_t slot; std::uint64_t sequence; };
    std::uint64_t generation_{};
    std::vector<Token> pending_;
    std::optional<WorldAudioRequest> current_;

    static bool SharesVoiceOrChannels(const WorldAudioRequest& a, const WorldAudioRequest& b) noexcept {
        if (!a.instance || !b.instance) return false;
        if (a.instance == b.instance && a.instanceGeneration == b.instanceGeneration) return true;
        // CPU STOP can release a reservation before physical STOP is applied.
        // A later different owner of the same channel must wait for that STOP.
        const auto ac = a.reservedVoiceChannels, bc = b.reservedVoiceChannels;
        return ac && bc && a.reservedVoiceId < b.reservedVoiceId + bc &&
            b.reservedVoiceId < a.reservedVoiceId + ac;
    }
    static bool GlobalBarrier(WorldAudioRequestKind kind) noexcept {
        switch (kind) {
        case WORLD_AUDIO_INITIALIZE_DEVICE:
        case WORLD_AUDIO_LOAD_REGION:
        case WORLD_AUDIO_UNLOAD_BANK:
        case WORLD_AUDIO_SET_AUDIO_MODE:
        case WORLD_AUDIO_SET_CORE_EFFECT_VOLUME:
        case WORLD_AUDIO_SET_CORE_EFFECT_PAN:
        case WORLD_AUDIO_SET_CORE_EFFECT_ATTRIBUTE:
        case WORLD_AUDIO_SET_CORE_EFFECT_ENABLED:
            return true;
        default:
            // SET_REGION_PATH has already selected each retained FileRange.
            // SET_CHANNEL_VOLUME has already resolved the following per-voice
            // gain commands. DRIVER_UPDATE truthfully observes current device
            // cursors; bVoiceMgr rejects observations of newer source intents.
            return false;
        }
    }
};
}

/* One CPU audio-library owner. These are native C++ lifetimes/capabilities,
 * not the console class layout. All public bank/sample/mixer views refer to
 * this object's storage. The physical device consumes the existing queue. */
struct bSound final {
public:
    bSound();
    ~bSound();
    bSound(const bSound&)=delete;
    bSound& operator=(const bSound&)=delete;
    static bSound* getInstance();

    WorldAudioResult Bind(WorldAudioRequestQueue& queue,const hp2::GameResources& files,
        const int32_t& spatialEnabled);
    WorldAudioResult BindSoundClient(const CarSoundBindings* client);
    WorldAudioResult Initialize();
    WorldAudioResult Shutdown();
    // Final owner-thread retirement, after all CPU sample/callback owners
    // have closed. Joins the consumer, cancels real retained operations and
    // retires remaining private samples before detaching banks/the queue.
    // A repeated call preserves the first terminal failure, even if later
    // cleanup succeeds. Bind starts a fresh lifetime and clears that result.
    WorldAudioResult ShutdownAfterOwnersRetired() noexcept;
    // Terminal cancellation only after the physical consumer has joined.
    // Retained operations become FAILED, never successful device ACKs.
    WorldAudioResult CancelDeviceRequestsAfterClose();
    std::string FailureDescription() const;
    // Preserve the first typed source failure while
    // the existing factories keep their result/rollback contracts unchanged.
    void ClearSourceFailure() noexcept;
    void RecordSourceFailure(WorldAudioResult result,const char* stage,const char* reason) noexcept;
    WorldAudioResult Update();
    WorldAudioRequestQueue* Driver() const noexcept;
    // Native registry storage for the source's expandable sample pool.
    // Growth never moves a live sample, queued borrow or generation token.
    size_t SampleCapacity() const noexcept;
    WorldAudioInstance* SampleAt(size_t index) noexcept;
    const WorldAudioInstance* SampleAt(size_t index) const noexcept;
    bool OwnsSample(const WorldAudioInstance* sample) const noexcept;
    WorldAudioResult ExpandSampleStorage();
    bVoiceMgr& Voices() noexcept { return voices_; }
    const bVoiceMgr& Voices() const noexcept { return voices_; }
    const WorldAudioManagerState& Mixer() const noexcept;
    bRegion* CommonRegion() const noexcept;
    bRegion* Region(const char* name) const;
    WorldAudioResult LoadRegion(const char* name);
    WorldAudioResult UnloadRegion(const char* name);
    WorldAudioResult SetCategoryVolume(uint32_t category,float volume);
    float GetCategoryVolume(uint32_t category) const noexcept;
    WorldAudioResult SetPath(const char* path);
    const char* GetPath() const noexcept { return path_.data(); }
    int32_t GetStatus() const noexcept;
    WorldAudioResult SetMode(int32_t mode);

    /* Backend completion is not Acknowledge: it identifies the exact LOAD
     * operation and queue generation. Event2 is emitted later by Update,
     * after the actual eager bRegion has been constructed. */
    WorldAudioResult CompleteRegion(const WorldAudioRequest& operation,bool loaded);
    bool OwnsLoadedBank(const WorldAudioBank* bank) const noexcept;
    void Printf(const char* format,...) const;
    void EnablePrintf(uint32_t channel) noexcept;
    void DisablePrintf() noexcept;

#ifdef _WIN32
    struct WindowsAudioFailure {
        WorldAudioResult result{WORLD_AUDIO_OK};
        WorldAudioRequestKind kind{WORLD_AUDIO_INITIALIZE_DEVICE};
        uint64_t sequence{};
        int32_t systemCode{};
        std::string reason;
    };
    // Owner-thread API. Open consumes the retained INITIALIZE operation;
    // Pump applies the same queue. Neither method exists as a Linux stub.
    WorldAudioResult OpenWindowsDevice(hp2::NativeAudioVoiceOutput* speechOutput=nullptr);
    WorldAudioResult PumpWindowsDevice(size_t maximumOperations=256);
    WorldAudioResult CloseWindowsDevice();
    const WindowsAudioFailure& WindowsFailure() const noexcept { return windowsFailure_; }
#endif

    const char* getPendingRegion(int32_t index) const
    {
        if (index<0 || static_cast<size_t>(index)>=pendingCount_)
            throw std::out_of_range("Pending sound region index");
        return pending_[static_cast<size_t>(index)].data();
    }

    int32_t getPendingRegionsCount() const noexcept
    {
        return static_cast<int32_t>(pendingCount_);
    }

private:
#ifdef _WIN32
    struct NativeDevice;
    struct DeviceDeleter { void operator()(NativeDevice* device) const noexcept; };
    std::unique_ptr<NativeDevice,DeviceDeleter> windowsDevice_;
    WindowsAudioFailure windowsFailure_;
#endif
    struct BankResource;
    static constexpr size_t bankCapacity=32;
    WorldAudioResult notifyClient(uint32_t event,bRegion* region);
    WorldAudioResult updatePendingRegions();
    WorldAudioResult addPendingRegion(const char* name);
    void removePendingRegion(const char* name);
    void reclaimRetiredBanks();
    bool bankBorrowed(const WorldAudioBank* bank) const noexcept;

    bRegion* common_{};
    int32_t status_{};
    int32_t diagnosticChannel_{};
    std::string resourceFailure_;
    WorldAudioResult terminalFailure_{WORLD_AUDIO_OK};
    std::array<float,16> categoryVolumes_{};
    WorldAudioManagerState mixer_{};
    std::array<std::array<char,256>,1> pending_{};
    size_t pendingCount_{};
    std::array<char,64> path_{};
    std::array<WorldAudioBank,bankCapacity> banks_{};
    std::array<std::unique_ptr<BankResource>,bankCapacity> resources_{};
    static constexpr size_t sampleInitialCapacity=255;
    // PAL Malloc25815C reads initial block count+24. Expand257EE8 only
    // changes aggregate count+2C, so every added block has64 slots.
    static constexpr size_t sampleGrowthCapacity=(sampleInitialCapacity>>2)+1;
    std::array<WorldAudioInstance,sampleInitialCapacity> samples_{};
    std::list<std::array<WorldAudioInstance,sampleGrowthCapacity>> expandedSamples_;
    bVoiceMgr voices_;
    WorldAudioRequestQueue* queue_{};
    const hp2::GameResources* files_{};
    const CarSoundBindings* client_{};
    std::vector<size_t> regionOrder_;
};

#endif

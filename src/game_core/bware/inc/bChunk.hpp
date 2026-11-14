#ifndef NFSHP2_BCHUNK_HPP
#define NFSHP2_BCHUNK_HPP

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <vector>
#include "bware/inc/bWare.hpp"

struct CarFrameLiveReplay;
struct CarViewCameraResources;
struct PositionMarkerRegistry;
struct TerrainTopology;
struct EditorMarkerResources;
struct RaceParameters;
struct CarSoundBindings;
struct WorldParticleSystem;
struct GameTime;
struct CarReplayAnimationCatalogue;
class DemoDiscManager;

namespace hp2 {
// Checked native views of the original eight-byte chunk envelope. Offsets
// stay relative to the owning byte buffer; no host pointers or decoded layout
// are written back into the immutable resource bytes.
struct Chunk {
    std::uint32_t id = 0;
    std::uint32_t size = 0;
    std::size_t headerOffset = 0;
    std::size_t payloadOffset = 0;
    std::size_t endOffset = 0;
    bool IsContainer() const noexcept { return (id & 0x80000000u) != 0; }
};

inline std::vector<Chunk> ReadChunks(const std::vector<std::uint8_t>& data,
    std::size_t offset = 0, std::size_t length = static_cast<std::size_t>(-1))
{
    Require(offset <= data.size(), "Chunk list begins outside buffer");
    if (length == static_cast<std::size_t>(-1)) length = data.size() - offset;
    Require(length <= data.size() - offset, "Chunk list extends beyond buffer");
    const std::size_t end = offset + length;
    std::vector<Chunk> chunks;
    while (offset < end) {
        Require(end - offset >= 8, "Trailing incomplete EA chunk header");
        const auto id = ReadU32(data.data() + offset);
        const auto size = ReadU32(data.data() + offset + 4);
        const auto dataBegin = offset + 8;
        Require(size <= end - dataBegin, "EA chunk extends outside parent");
        const auto next = dataBegin + size;
        chunks.push_back(Chunk{id, size, offset, dataBegin, next});
        offset = next;
        Require(chunks.size() < 1'000'000u, "Too many EA chunks");
    }
    return chunks;
}

extern "C++" {
class EventTreeResources;
struct FrontendPlayerManager;
struct FlailerRenderManager;
struct ApplicationFrontendFlow;
struct GameFlowBindings;
}
class FileRange;
// The two original LZ procedures use the caller's output buffer. Compression
// needs inputBytes+50 writable bytes, including its temporary final token
// group before a possible stored/raw fallback.
std::uint32_t LZCompress(const std::uint8_t* input,std::uint32_t inputBytes,std::uint8_t* output);
std::uint32_t LZDecompress(const std::uint8_t* input,std::uint8_t* output);
// Checked physical-file adaptation around the same decompression kernel.
std::vector<std::uint8_t> DecodeComp(const FileRange& resource);
std::vector<std::uint8_t> DecodeComp(const std::vector<std::uint8_t>& input);
class GameResources;
class ResourceRegistry;
class CollisionObjectData;
class CarDamageData;
class TrackScenarioData;
class TerrainData;
class TrackRouteData;
class CarRecoveryData;
class SceneryResources;
class AerialNavigationData;
class WorldEventsData;
class ChopperEffectData;
class AcidFXRuntime;
class CarInfoResource;
class NisResources;
class CarDriverAppearanceResources;
class DriverBehaviorData;
class CarLoader;
enum class CarInfoAvailability;

// These are native callback roles, not console function addresses. The
// implemented NIS producer supplies either NULL or actual NisLoadedCallback.
enum class ResourceFileCompletion {
    None, NisLoaded, CarLoaded, FrontendLoaded,
    TrackCommonLoaded, TrackSpecificLoaded, TrackPreRaceLoaded,
    TrackLocationTexturesLoaded, TrackDriverTexturesLoaded
};
enum class ResourceFileKind { Nis, Car, Track, Global, Frontend, TrackCommon, TrackAuxiliary };
// The content policy must come from the real application/DemoDisc owner.
// There is no default FullGame fallback in a queued Global file.
struct ResourceGlobalBindings {
    AcidFXRuntime& effects;
    CarInfoAvailability availability;
};
struct ResourceFrontendBindings {
    FrontendPlayerManager& profiles;
    const DemoDiscManager& demo;
    FlailerRenderManager& renderer;
};
struct ResourceFileBindings {
    CarViewCameraResources* cameras{};
    CarFrameLiveReplay* nis{};
    std::int32_t cameraGroup{};
    ResourceFileKind kind{ResourceFileKind::Nis};
    ::PositionMarkerRegistry* positionMarkers{};
    ::TerrainTopology* terrain{};
    const ::RaceParameters* parameters{};
    ::CarSoundBindings* sound{};
    ::WorldParticleSystem* particles{};
    const ResourceGlobalBindings* global{};
    CarLoader* carLoader{};
    const ResourceFrontendBindings* frontend{};
    ApplicationFrontendFlow* flow{};
    const GameFlowBindings* gameFlow{};
    // Port-side scene resource consumer. Track weather and effects can run
    // on this clock independently of a gameplay audio session.
    const ::GameTime* sceneClock{};
};

// Read-only views of the file's actual decoded owners. The file owns their
// lifetime; consumers borrow these objects instead of loading another copy.
struct ResourceFileResources {
    ::EditorMarkerResources* models{};
    CollisionObjectData* collisionObjects{};
    CarDamageData* damage{};
    TrackScenarioData* track{};
    TerrainData* terrain{};
    TrackRouteData* route{};
    CarRecoveryData* recovery{};
    SceneryResources* scenery{};
    AerialNavigationData* navigation{};
    WorldEventsData* events{};
    ChopperEffectData* effects{};
    ::CarViewCameraResources* cameras{};
    CarInfoResource* cars{};
    NisResources* nis{};
    CarDriverAppearanceResources* characters{};
    DriverBehaviorData* driverBehavior{};
    EventTreeResources* frontendEvents{};
    const CarReplayAnimationCatalogue* animations{};
};

class ResourceLoader;
class QueuedFileService;
class ResourceFile final {
public:
    ~ResourceFile();
    ResourceFile(const ResourceFile&)=delete;
    ResourceFile& operator=(const ResourceFile&)=delete;
    const std::string& Filename() const noexcept;
    std::int32_t Type() const noexcept;
    std::uint32_t Flags() const noexcept;
    bool Loaded() const noexcept;
    std::int32_t TransfersPending() const noexcept;
    const ResourceFileResources& Resources() const;
    std::int32_t GetSize(std::uint32_t chunkKind,std::int32_t* chunks=nullptr) const;
    void ChangeFilenameForHotChunking(const char* filename);
private:
    friend class ResourceLoader;
    friend class QueuedFileService;
    void QueuedReadDone(bool failed);
    struct Storage;
    explicit ResourceFile(std::unique_ptr<Storage> storage);
    std::unique_ptr<Storage> storage_;
};

// One application-owned service is borrowed by both NIS managers. Actual
// physical bytes enter through GameResources; this object owns transfer,
// publication, callback and retirement order. It is not a GPU-ready signal.
class ResourceLoader final {
public:
    ResourceLoader(const GameResources& files,ResourceRegistry& registry,QueuedFileService& queued);
    ~ResourceLoader();
    ResourceLoader(const ResourceLoader&)=delete;
    ResourceLoader& operator=(const ResourceLoader&)=delete;
    ResourceFile* LoadResourceFile(const char* filename,std::int32_t type,
        std::uint32_t flags,const ResourceFileBindings& bindings,
        ResourceFileCompletion completion=ResourceFileCompletion::None);
    std::size_t ServiceResourceLoading();
    bool IsResourceLoadingComplete() const noexcept;
    bool HasPendingFileReads() const noexcept;
    // Stop admissions/continuations first; retain destinations and providers
    // until actual I/O and all consumer lifetimes have retired.
    void BeginShutdown() noexcept;
    std::exception_ptr Shutdown() noexcept;
    bool ShuttingDown() const noexcept;
    void CancelResourceFile(ResourceFile* file);
    // Terminal application retirement detaches only its flow continuations.
    // Physical transfers and decoded-owner retirement still complete normally.
    void DetachFlowCompletions(const ApplicationFrontendFlow& flow) noexcept;
    void UnloadResourceFile(ResourceFile* file);
    ResourceFile* FindResourceFile(const char* filename) const noexcept;
    ResourceFile* FindResourceFile(std::int32_t type) const noexcept;
    std::int32_t GetResourceFileSize(std::int32_t type,std::uint32_t chunkKind,
        std::int32_t* chunks=nullptr) const;
    const GameResources& Files() const noexcept;
    ResourceRegistry& Registry() noexcept;
private:
    void RetireResourceFile(ResourceFile* file,bool terminal);
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
#endif

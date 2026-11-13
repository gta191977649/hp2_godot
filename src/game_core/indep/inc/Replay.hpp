#ifndef NFSHP2_REPLAY_HPP
#define NFSHP2_REPLAY_HPP

#include "Joylog.hpp"
typedef struct RaceActionQueue RaceActionQueue;
#ifdef __cplusplus
class ReplayManagerOwner;
struct GameSession;
#endif
#include "indep/inc/RaceCoordinator.hpp"
struct World;
#include <stddef.h>
#include <stdint.h>
struct GameTime;
struct RaceParameters;
struct CarDriverAppearanceCatalogue;
struct GameInputRouter;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ReplaySpecialEventInfo {
    int32_t kind;
    int32_t worldLoopCounter;
    int32_t argument1;
    int32_t argument2;
} ReplaySpecialEventInfo;

typedef enum ReplaySynchronizationPolicy {
    REPLAY_SYNC_NATIVE_STRICT=0,
    REPLAY_SYNC_PAL_REFERENCE=1
} ReplaySynchronizationPolicy;

typedef struct ReplayManager {
    BitStream* stream;
    int32_t restoring;
    int32_t recording;
    int32_t exhausted;
    int32_t desynchronized;
    /* Imported PAL coordinate hashes are diagnostic references, not native
     * float tolerances. Their bytes remain unchanged. Native recordings and
     * instant replays retain strict native synchronization. */
    ReplaySynchronizationPolicy synchronizationPolicy;
    uint32_t sourceReferenceSamples;
    uint32_t sourceReferenceMismatchCount;
    int32_t firstSourceReferenceMismatchLoop;
    uint8_t firstSourceReferenceExpected;
    uint8_t firstSourceReferenceActual;
    int32_t stopRequested;
    /* Source+10 is PausedFlag, independent of CapturingFlag+04. The final
     * Pause.Activate170738 tests desynchronized+0C, not this paused flag.
     * Source+8C is
     * NumInstantReplaySnapshots; the full native snapshot allocator remains
     * an explicit producer dependency, not a synthetic can-replay switch. */
    int32_t paused;
    int32_t instantReplaySnapshotCount;
    /* PAL+50/+40B0/+40B8; stopRequested already owns PAL+58. */
    int32_t beginCapturingLoopCount;
    int32_t specialEventCount;
    ReplaySpecialEventInfo specialEvents[256];
    /* Individually related constructor stores, PAL13D448. The released
     * desynchronized word at+0C is absent in beta; later source members are
     * four bytes later. Native fields are deliberately not a layout overlay. */
    int32_t slowMotion;
    int32_t replayPausedThisFrame;
    int32_t nextFancyDebugTime;
    int32_t fancyReplayInProgress;
    int32_t fancyJoyEventNumber;
    int32_t fancyJoyEventCount;
    uint8_t fancyJoyEvents[32];
    int32_t nextInstantReplaySnapshotTime;
    int32_t endCapturingLoopCount;
    int32_t beginCapturingTime;
    int32_t endCapturingTime;
    int32_t maximumSnapshots;
    int32_t maximumInstantReplaySnapshots;
    int32_t maximumAutoReplaySnapshots;
    int32_t maximumLapSnapshots;
    int32_t maximumHighlightSnapshots;
    int32_t lapSnapshotCount;
    int32_t highlightSnapshotCount;
    float lapSnapshotSpacing;
    int32_t nextHighlightNumber;
    int32_t endCurrentHighlightTime;
    int32_t lastSnapshotTime;
    int32_t snapshotsTaken;
    int32_t lastSnapshotIndex;
    int32_t snapshotTimes[4096];
    int32_t lastSpecialEventNumber;
#ifdef __cplusplus
    ReplayManagerOwner* owner; /* The same allocation's native list owner. */
#else
    struct ReplayManagerOwner* owner;
#endif
} ReplayManager;

typedef enum WorldReplayResult {
    WORLD_REPLAY_OK,
    WORLD_REPLAY_INVALID_BINDING,
    WORLD_REPLAY_ACTION_CAPACITY,
    WORLD_REPLAY_STREAM_FAILURE,
    WORLD_REPLAY_INVALID_STATE
} WorldReplayResult;

typedef struct WorldReplay {
    ReplayIntegerLog* log;
    ReplayManager* manager;
    const GameTime* clock;
    const void* checksumContribution;
    RaceActionQueue* raceActions;
    WorldReplayResult result;
} WorldReplay;

uint32_t replayManagerRead(ReplayManager* replay, uint32_t bits, uint32_t channel);
uint32_t replayManagerReadUnsigned(ReplayManager* replay, uint32_t bits, uint32_t channel);
void replayManagerWrite(ReplayManager* replay, uint32_t value, uint32_t bits, uint32_t channel);
void replayManagerRequestStop(ReplayManager* replay);
void replayManagerAddHighlight(ReplayManager* replay,float intensity);
/* Complete PAL13E008..13E074. The end boundary uses the same live clocks
 * consumed by BeginFrame; this does not manufacture recording snapshots. */
void replayManagerEndFrame(ReplayManager* replay,const struct GameTime* clock);
WorldReplayResult worldReplaySynchronize(WorldReplay* replay, struct World* world);


enum { REPLAY_SNAPSHOT_MAX_SECTIONS=16, REPLAY_SNAPSHOT_WRITE_LIMIT=0x93ff };
typedef enum ReplaySnapshotResult {
    REPLAY_SNAPSHOT_OK,
    REPLAY_SNAPSHOT_INVALID_BINDING,
    REPLAY_SNAPSHOT_INVALID_TRANSFER,
    REPLAY_SNAPSHOT_CAPACITY,
    REPLAY_SNAPSHOT_READ_BOUNDS,
    REPLAY_SNAPSHOT_SECTION_BOUNDS,
    REPLAY_SNAPSHOT_NONCANONICAL_EXTENSION,
    REPLAY_SNAPSHOT_CHECKSUM_MISMATCH,
    REPLAY_SNAPSHOT_UNRESOLVED_REFERENCE
} ReplaySnapshotResult;

typedef struct ReplaySnapshotSection {
    const char* name;
    const void* object;
    uint16_t ordinal;
    uint8_t transferCount;
    uint8_t mismatchReported;
    int32_t byteCount;
} ReplaySnapshotSection;

/* One actual byte-stream owner. Storage is borrowed from its retained replay
 * resource/allocation; no generic game callbacks or second integer log exist.
 * The native positions below do not reconstruct the console object layout. */
enum { REPLAY_SNAPSHOT_NATIVE = 0, REPLAY_SNAPSHOT_PAL_V8 = 8 };

typedef struct ReplaySnapshot {
    char name[32];
    int32_t worldTimer;
    int32_t worldLoopCounter;
    uint8_t recording;
    uint8_t restoring;
    uint8_t recordingChecksums;
    uint8_t verifyingChecksums;
    int8_t sectionDepth;
    uint8_t overflowReported;
    uint16_t nextSectionOrdinal;
    int32_t debugDepth;
    int32_t debugLoopCounter;
    ReplaySnapshotSection sections[REPLAY_SNAPSHOT_MAX_SECTIONS];
    int32_t cursor;
    int32_t dataSize;
    uint8_t* bytes;
    size_t capacity;
    const struct GameTime* clock;
    ReplaySnapshotResult result;
    /* Authored header+2C/+34/+38/+14C; the existing worldTimer/loopCounter
     * already own BeginTime/+28 and BeginLoopTime/+30 respectively. */
    int32_t endTime;
    int32_t endLoopCounter;
    int32_t memoryImageLoaded;
    float totalInterestingValue;
    /* Explicit transport identity. PAL v8 stores a raw Player address;
     * native snapshots store a logical seat. These words are never host pointers.
     * A v8 address is resolved only by a file-specific, supplied relocation. */
    uint32_t wireFormat;
    /* File metadata version 10 selects the cumulative native timer; PAL8
     * and native9 retain per-step Timer rounding. Payload bytes are unchanged. */
    int32_t clockPolicy;
    uint32_t sourcePlayerTokens[2];
} ReplaySnapshot;

void replaySnapshotInitialize(ReplaySnapshot* snapshot,const char* name,const struct GameTime* clock);
uint32_t replaySnapshotChecksum(const ReplaySnapshot* snapshot,const void* values,int32_t bytes,int32_t ignored);
void replaySnapshotWriteData(ReplaySnapshot* snapshot,const void* values,int32_t bytes,int32_t maximumBytes,int32_t signedExtension);
void replaySnapshotReadData(ReplaySnapshot* snapshot,void* values,int32_t bytes,int32_t maximumBytes,int32_t signedExtension);
void replaySnapshotMoveData(ReplaySnapshot* snapshot,void* values,int32_t bytes,int32_t maximumBytes,int32_t signedExtension);
void replaySnapshotVerifyData(ReplaySnapshot* snapshot,void* values,int32_t bytes,int32_t maximumBytes,int32_t signedExtension);
void replaySnapshotBitStream(BitStream* stream,ReplaySnapshot* snapshot,int32_t writingEnd);
void replaySnapshotBeginSection(ReplaySnapshot* snapshot,const void* object,const char* name);
void replaySnapshotEndSection(ReplaySnapshot* snapshot);

/* Typed native argument adapters for original inline overloads. They add no
 * separate original-body count. Width is the serialized byte width, not an
 * integer element count. */
void replaySnapshotMoveInteger(ReplaySnapshot* snapshot,int32_t* value,int32_t bytes);
void replaySnapshotMoveUnsigned(ReplaySnapshot* snapshot,uint32_t* value,int32_t bytes);
void replaySnapshotMoveFloat(ReplaySnapshot* snapshot,float* value);
/* Final PAL matrix stream: one byte selects full, rotation, or affine data. */
void replaySnapshotMoveMatrix(ReplaySnapshot* snapshot,float matrix[16],int32_t bytes);

#ifdef __cplusplus
}
#endif
#include "World.hpp"

#ifdef __cplusplus
namespace hp2 {struct RaceScreenServices;}
int replayManagerAddSpecialEvent(ReplayManager& replay,hp2::RaceScreenServices& services,
    int32_t kind,int32_t argument1,int32_t argument2);
int replayManagerHandleSpecialEvent(ReplayManager& replay,hp2::RaceScreenServices& services,
    const ReplaySpecialEventInfo& event);

#include <memory>
#include <vector>
#include <array>
#include <cstdint>
namespace hp2 {class DemoModeReplay;}
// Native validation index, never serialized as a PS2 structure. Every entry
// is a complete timestep/input/synchronization boundary in the real bRepeat.
class ReplayStreamIndex final {
public:
    void Build(const BitStream& source);
    std::size_t FrameCount() const noexcept {return boundaries_.empty()?0:boundaries_.size()-1;}
    std::size_t FindFrame(const BitStream& reader) const noexcept;
    static constexpr std::size_t NoFrame=static_cast<std::size_t>(-1);
private:
    struct Channel {std::uint32_t repeats{},position{},value{};};
    struct Boundary {
        std::uint32_t position{},count{};
        std::array<Channel,4> channels{};
    };
    std::vector<Boundary> boundaries_;
    std::uint32_t writtenBits_{},writtenValues_{};
};
struct ReplayCaptureRange {std::int32_t beginLoop{},endLoop{};};
/* A source snapshot owns either its native allocation or its retained final
 * chunk payload. Moving this node never copies or reinterprets that payload. */
struct ReplaySnapshotStorage final {
    ReplaySnapshot snapshot{};
    std::vector<uint8_t> allocation;
    const uint8_t* authoredPayload{};
    std::size_t authoredPayloadBytes{}; // Original chunk payload bound, not snapshot.dataSize.
};
class ReplayManagerOwner final {
public:
    explicit ReplayManagerOwner(hp2::DemoModeReplay& demo);
    ~ReplayManagerOwner();
    ReplayManagerOwner(const ReplayManagerOwner&)=delete;
    ReplayManagerOwner& operator=(const ReplayManagerOwner&)=delete;
    ReplayManager& State() noexcept;
    const ReplayManager& State() const noexcept;
    // availableBytes is the actual allocator's remaining allocation budget,
    // replacing bCountFreeMemory(16); no constant success budget is implied.
    void MarkTheBeginningOfTime(const GameTime& clock,std::size_t availableBytes);
    void BindRace(GameSession& coordinator,RaceParameters& parameters,const GameTime& clock,
        const CarDriverAppearanceCatalogue& characters,GameInputRouter& input);
    // Persistent timeline predicate. EndFrame clears the transient restoring
    // flag, so native pacing must query this before the next BeginFrame.
    bool WillRestoreFrame(const GameTime& clock) const;
    void BeginFrame(const GameTime& clock,hp2::RaceScreenServices& services);
    void DoFancyJoylogDebugging(const GameTime& clock,ReplayIntegerLog& log);
    const RaceParameters& Parameters() const;
    void BeginRaceHighlights(const GameTime& clock);
    void BeginNextRaceHighlight(const GameTime& clock);
    ReplaySnapshot* FindSnapshot(const char* name,std::int32_t ordinal=0) noexcept;
    ReplaySnapshot* TakeSnapshot(const char* name);
    bool RestoreSnapshot(const char* name);
    void RestoreSnapshot(ReplaySnapshot& snapshot);
    void SortSnapshots(const char* name);
    void HandleSnapshot(ReplaySnapshot& snapshot);
    void DoEndSnapshot(ReplaySnapshot& snapshot);
    static ReplayCaptureRange ValidateStateSnapshot(const ReplaySnapshot& snapshot);
    void MaybeTakeInstantReplaySnapshot();
    void MaybeTakeLapSnapshot();
    void MaybeTakeDummySnapshot();
    void MaybeDoChecksumSnapshot();
    void DoChecksumSnapshot(const char* name);
    bool WasSnapshotTakenAtThisTime(std::int32_t worldLoop);
    bool DeleteSnapshot(const char* name);
    void DeleteSnapshot(ReplaySnapshot& snapshot);
    std::size_t FreeSnapshotCount() const noexcept;
    std::size_t ActiveSnapshotCount() const noexcept;
    ReplaySnapshot* ActiveSnapshot(std::size_t index) noexcept;
    const ReplaySnapshotStorage* ActiveSnapshotStorage(std::size_t index) const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
/* Actual final checksum schedule2F08D8, authored-1. GP-50BC is the
 * single ReplayIntegerLog::replaying owner, including Fancy transitions. */
extern int32_t replayChecksumSnapshotTimes[4];
#endif
#endif

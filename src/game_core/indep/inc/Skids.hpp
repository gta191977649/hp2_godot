#ifndef NFSHP2_SKIDS_HPP
#define NFSHP2_SKIDS_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/Wheel.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Renderer input remains native data. A revision announces a start, geometry
 * update, stop, or eviction. Detached trails retain their geometry until the
 * caller's bounded store reuses their slot. No graphics API is required here. */
typedef enum WheelTrailChange {
    WHEEL_TRAIL_STARTED, WHEEL_TRAIL_UPDATED, WHEEL_TRAIL_STOPPED
} WheelTrailChange;

typedef struct WheelTrailPoint {
    float position[3];
    int8_t width[3];
    uint8_t opacity;
} WheelTrailPoint;

struct WheelTrailClan;
struct WheelTrailClanOwner;

typedef struct WheelTrail {
    struct WheelTrail** attachment;
    const void* owner;
    struct WheelTrailClan* clan;
    struct WheelTrail* clanPrevious;
    struct WheelTrail* clanNext;
    uint64_t creation;
    uint64_t revision;
    WheelTrailChange change;
    int32_t materialIndex;
    int32_t variant;
    size_t pointCount;
    WheelTrailPoint points[8];
    float previousDirection[3];
    float distance;
    float minimum[3];
    float maximum[3];
    float center[3];
    float origin[3];
} WheelTrail;

typedef struct WheelTrailStore {
    WheelTrail* storage;
    size_t capacity;
    uint64_t revision;
    struct WheelTrailClanOwner* clans;
} WheelTrailStore;

/* Initialize before attaching channels. Capacity is at least one; storage and
 * store outlive the channels. Detach channels before unloading the store. */
void wheelTrailStoreInitialize(WheelTrailStore* store, WheelTrail* storage, size_t capacity);
void wheelTrailPackPoint(WheelTrailPoint* point, const float position[3], const float width[3]);
void wheelTrailUnpackPoint(const WheelTrailPoint* point, float position[3], float width[3]);
void wheelTrailEndpoints(const WheelTrailPoint* point, float first[3], float second[3]);
void wheelTrailStop(WheelTrailStore* store, WheelTrail* trail);
void wheelTrailDetach(WheelTrailStore* store, WheelTrail** attachment);
void wheelTrailDelete(WheelTrailStore* store, WheelTrail* trail);
int wheelTrailAppend(WheelTrailStore* store, WheelTrail* trail, const float position[4],
                     const float width[3], float intensity);
void wheelTrailUpdate(WheelTrailStore* store, WheelTrail** attachment, const void* owner,
                      const float ownerCenter[3], uint32_t flags, const float position[4],
                      const float width[3], int32_t materialIndex, float intensity);
void wheelEffectsTransformDirection(float output[3], const float matrix[16], const float input[3]);
void wheelEffectsUpdate(const Wheel* wheel, const WheelContactContext* contact,
                        WheelTrailStore* store, WheelTrail** attachment, const void* owner,
                        const float ownerCenter[3], uint32_t flags, uint8_t disabledWheelMask);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
#include <memory>
struct EditorMarkerTexture;
namespace hp2 {
class ResourceRegistry;
// InitSkids1AB118 / CloseSkids1AB1D0 own exactly128 live trail slots and
//33 borrowed material textures. CPU geometry is the existing canonical
//WheelTrailStore; the console pool and PS2 packets are not second owners.
class SkidResources final {
public:
    explicit SkidResources(ResourceRegistry& resources);
    ~SkidResources();
    SkidResources(const SkidResources&)=delete;
    SkidResources& operator=(const SkidResources&)=delete;
    WheelTrailStore& Trails() noexcept;
    const EditorMarkerTexture* MaterialTexture(std::size_t material) const;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
#endif

#endif

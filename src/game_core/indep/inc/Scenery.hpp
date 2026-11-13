#ifndef NFSHP2_SCENERY_HPP
#define NFSHP2_SCENERY_HPP
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/WorldObject.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SceneryInstance {
    int16_t minimum[3],maximum[3];
    int16_t modelIndex;
    uint16_t flags;
    float position[3];
    int16_t rotation[9];
} SceneryInstance;

typedef struct SceneryModel {
    EditorMarkerModel levels[3];
    int16_t minimumPixels[3];
    int8_t facade[3];
    float radius;
} SceneryModel;

/* The source publishes these24-byte authored records with each section.
 * Their rendering consumers assign further semantic field names. */
typedef struct SceneryRecord34104 {uint32_t fields[6];} SceneryRecord34104;

typedef struct ScenerySection {
    uint32_t number;
    SceneryModel* models;
    size_t modelCount;
    SceneryInstance* instances;
    size_t instanceCount;
    const SceneryRecord34104* records34104;
    size_t record34104Count;
    const uint8_t* authoredHeader;
    size_t authoredHeaderSize;
} ScenerySection;

typedef struct SceneryVisibleZone {
    struct SceneryVisibleZone* next;
    struct SceneryVisibleZone* previous;
    uint32_t authoredParameters[3];
    int32_t runtimeState14;
    int32_t adjacentCount;
    int16_t adjacentZones[64];
    int32_t polygonCount;
    float polygon[16][2];
} SceneryVisibleZone;

typedef struct Scenery {
    ScenerySection* const* sections;
    size_t sectionCount;
    ScenerySection* const* loadedSections;
    size_t loadedSectionCount;
    SceneryVisibleZone* firstVisibleZone;
    SceneryVisibleZone* lastVisibleZone;
} Scenery;
int32_t sceneryCurrentZoneNumber(void);

SceneryInstance* sceneryFindInstance(const Scenery* scenery,uint32_t key);
SceneryInstance* sceneryFindInstanceByIndex(const Scenery* scenery,int32_t section,int32_t instance);
void sceneryInstanceFrame(const SceneryInstance* instance,float frame[16]);
void sceneryFlipObjectsOnBackwardsTrack(ScenerySection* section);
/* Track-load preparation of a physical spawner, before render caches and
 * collision promotion borrow the canonical quantized orientation. */
void sceneryNormalizeSmokeableRotation(int16_t rotation[9]);
int32_t sceneryPointInVisibleZone(const float point[2],const float (*polygon)[2],int32_t count);
SceneryVisibleZone* sceneryFindVisibleZone(Scenery* scenery,const float point[2]);
int32_t sceneryVisibleZoneFindIndexTo(const SceneryVisibleZone* zone,int32_t target);
int32_t sceneryVisibleZoneFindNextFreeIndex(SceneryVisibleZone* zone);

/* Native interface to the individually identified fernflattener.cpp owner.
 * The released FernShaker implementation replaces a SceneryInstance with
 * a WorldObject and triggers AcidFX emitters. No original FernShaker header
 * is claimed: this bridge lives with the retained source scenery types. */
typedef struct FernShakerSystem FernShakerSystem;
FernShakerSystem* fernShakersCreate(Scenery* scenery,WorldObjectRegistry* objects,
    WorldParticleSystem* particles,const ChopperEffectGroup* groups,size_t groupCount,
    const GameTime* clock);
void fernShakersFlush(FernShakerSystem* system);
void fernShakersDispose(FernShakerSystem* system);
WorldServicesResult fernShakersShake(FernShakerSystem* system,uint32_t key,const float velocity[4]);
size_t fernShakersCount(const FernShakerSystem* system);

#ifdef __cplusplus
}

#include "indep/inc/Ecstasy.hpp"
#include <memory>
#include <vector>

namespace hp2 {
class VisibleZoneResources final {
public:
    explicit VisibleZoneResources(EditorMarkerResources& models);
    ~VisibleZoneResources();
    VisibleZoneResources(const VisibleZoneResources&)=delete;
    VisibleZoneResources& operator=(const VisibleZoneResources&)=delete;
    const EditorMarkerModel& BoundaryModel() const noexcept {return boundary_;}
private:
    EditorMarkerModel boundary_{};
};
// PC quality selection. PAL visibility radius and its original minimumPixels
// array remain unchanged; only the native presentation LOD is preferred.
// Native presentation: the shortest Euclidean distance from the camera to
// the original section's world-space instance envelope (not its origin).
// This deliberately does not change PAL culling or visibility-zone selection.
float sceneryNativeDistanceToInstanceSurface(const SceneryInstance& instance,
    const float eye[3]) noexcept;
// Squared horizontal distance to the original world-space footprint. Does
// not emulate console arithmetic and does not re-size source geometry.
float sceneryNativeDistanceSquaredXY(const SceneryInstance& instance,
    const float eye[3]) noexcept;
// The source section's integer AABB is not always aligned with the actual
// rendered LOD0 mesh. Precompute the native XY envelope of that real mesh
// using the scene transform once, without changing the source object/collision.
bool sceneryNativeModelFootprintXY(const SceneryModel& model,const float frame[16],
    float bounds[4]) noexcept;
float sceneryNativeDistanceSquaredFootprintXY(const float bounds[4],
    const float eye[3]) noexcept;
// Pure decision that can be different from model-level PAL threshold only
// for optional native near-field image quality. Retained bit is per-instance,
// not per-model; each material/model key and collision shape are unchanged.
bool sceneryNativeNearDetailState(float squaredXY,bool wasNear) noexcept;
// Direct PC billboard orientation. The original camera-facing direction is
// preserved without a console angle table or two 4x4 matrix products.
void sceneryNativeBillboardFacing(float output[16],const float towardEyeXY[2],
    const float origin[3]) noexcept;
std::size_t sceneryNativeSelectDetailLod(const SceneryModel& model,
    std::int32_t originalPixels,float distanceToSurface) noexcept;
// Compatibility for call sites that have only original projected pixels.
std::size_t sceneryNativeSelectDetailLod(const SceneryModel& model,
    std::int32_t originalPixels) noexcept;

struct SceneryNativeRenderStatistics {
    std::size_t sourceSections{},extendedSections{},duplicateExtendedInstances{},drawnInstances{};
};
class SceneryResources final {
public:
    explicit SceneryResources(const std::vector<std::uint8_t>& bundle,
        EditorMarkerResources& models,bool reverse);
    ~SceneryResources();
    SceneryResources(const SceneryResources&)=delete;
    SceneryResources& operator=(const SceneryResources&)=delete;
    Scenery& Native() noexcept { return native_; }
    EditorMarkerResources& Models() noexcept { return *models_; }
    const SceneryNativeRenderStatistics& NativeRenderStatistics() const noexcept { return nativeRenderStatistics_; }
    // Source visibility-zone selection, retained34104 tree traversal and
    // DrawAScenery LOD selection feed the actual model strip consumer.
    std::vector<std::int16_t> WhatSectionsShouldWeDraw(const CarCinematicScreen& view,
        std::size_t capacity=128);
    void Render(const CarCinematicScreen& view,EcstasyFrame& frame,std::uint32_t viewMask,
        std::int32_t worldFrames);
private:
    struct SectionStorage;

    // Render() is consumed serially by PAL's ten-view scene scheduler. Keep
    // transient CPU culling/draw lists attached to the live track instead of
    // allocating them again for every camera on every frame. No emitted
    // Ecstasy command borrows these arrays after the Render() call.
    struct RenderDraw {
        EditorMarkerModel* model{};
        std::array<float,16> matrix{};
        std::int32_t clip{};
        bool identity{};
        bool nativeExtended{};
        std::uint8_t nativeLod{};
    };
    struct RenderNode {std::size_t index{};std::int32_t clip{};};
    std::vector<RenderDraw> drawScratch_;
    std::vector<RenderNode> nodeScratch_;
    std::vector<std::int16_t> sectionScratch_;
    // Exact selected-solid / current native matrix / animation and material
    // bindings. A generation table reuses one allocation between extended
    // views; its capacity is bounded by this track's owned instance count.
    struct RenderIdentityBucket {
        std::array<std::uintptr_t,22> key{};
        std::uint32_t generation{};
    };
    std::vector<RenderIdentityBucket> renderIdentityScratch_;
    std::size_t nativeInstanceCapacity_{};
    std::uint32_t renderGeneration_{};
    SceneryNativeRenderStatistics nativeRenderStatistics_{};
    const SceneryVisibleZone* CollectSections(const CarCinematicScreen& view,
        std::vector<std::int16_t>& output,std::size_t capacity,std::size_t* sourceCount=nullptr);
    // Native journal cadence only; it does not participate in visibility.
    std::array<std::uint32_t,10> journalTicks_{};
    std::uint32_t journalViews_{};
    EditorMarkerResources* models_;
    std::vector<std::unique_ptr<SectionStorage>> sections_;
    std::vector<ScenerySection*> sectionIndex_;
    std::vector<SectionStorage*> sectionStorageByNumber_;
    std::vector<ScenerySection*> loadedSections_;
    std::vector<std::unique_ptr<SceneryVisibleZone>> visibleZones_;
    Scenery native_{};
};
}
#endif
#endif

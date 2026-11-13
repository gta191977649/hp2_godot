#pragma once

#include "indep/inc/World.hpp"
#include <stddef.h>
#include <stdint.h>
#include "indep/inc/CameraMover.hpp"

#ifdef __cplusplus
extern "C" {
#endif

float afxMatrixDeterminant(const float input[16]);
void afxMatrixInverse(float output[16],const float input[16]);

struct WorldParticleDefinition;
struct WorldParticleGroup;

/* These are decoded resource references. Packed file links and unspecified
 * particle/shader fields do not become native runtime object storage. */
typedef struct ChopperEffectTemplate {
    uint32_t key;
    uint32_t definitionKey;
    uint32_t renderKey;
    uint32_t groupKey;
    const char* name;
    float frame[16];
    const struct WorldParticleDefinition* particleDefinition;
} ChopperEffectTemplate;

typedef struct ChopperEffectGroup {
    uint32_t key;
    const ChopperEffectTemplate* const* members;
    size_t count;
} ChopperEffectGroup;

typedef struct ChopperEffectInstance {
    struct ChopperEffectInstance* next;
    struct ChopperEffectInstance* previous;
    const ChopperEffectTemplate* resource;
    float frame[16];
    uint64_t generation;
    int32_t emissionState;
    int32_t occupied;
    /* The handler and auto-emission pass are two borrowed traversal views of
     * this same emitter. Position, resource and lifetime are stored once. */
    struct ChopperEffectInstance* autoNext;
    struct ChopperEffectInstance* autoPrevious;
    WorldParticleSystem* particleSystem;
    const struct WorldParticleDefinition* particleDefinition;
    struct WorldParticleGroup* particleGroup;
    uint64_t particleGroupGeneration;
    uint32_t randomState;
    float cycleRemaining;
    float emissionRemainder;
    float carVelocity[4];
    int32_t suppressParticles;
    /* Native lifetime binding of the original handler list. All handlers
     * share the same actual emitter pool; this is never another particle. */
    const void* lifetimeOwner;
} ChopperEffectInstance;

typedef enum ChopperEffectRequestKind {
    CHOPPER_EFFECT_START,
    CHOPPER_EFFECT_RELEASE,
    CHOPPER_EFFECT_EMIT
} ChopperEffectRequestKind;

typedef struct ChopperEffectRequest {
    ChopperEffectRequestKind kind;
    const ChopperEffectTemplate* resource;
    ChopperEffectInstance* instance;
    uint64_t generation;
    uint64_t sequence;
    float frame[16];
    float elapsed;
} ChopperEffectRequest;

/* The handler owns emitter slots; the particle system borrows those same
 * instances. Requests record diagnostics. Live CPU cycles determine when
 * an emitter leaves the handler, independently of graphics submission. */
typedef struct ChopperMunitionEffects {
    ChopperEffectInstance* instances;
    size_t capacity;
    ChopperEffectInstance* first;
    ChopperEffectInstance* last;
    ChopperEffectRequest* requests;
    size_t requestCapacity;
    size_t readIndex;
    size_t requestCount;
    uint64_t nextSequence;
    uint64_t nextGeneration;
    const ChopperEffectGroup* groups;
    size_t groupCount;
    WorldServicesResult result;
    WorldParticleSystem* particles;
    size_t droppedRequests;
} ChopperMunitionEffects;

void chopperEffectsInitialize(ChopperMunitionEffects* effects,
    ChopperEffectInstance* instances, size_t capacity,
    ChopperEffectRequest* requests, size_t requestCapacity,
    const ChopperEffectGroup* groups, size_t groupCount);
WorldServicesResult chopperEffectsBindParticles(ChopperMunitionEffects* effects,
    WorldParticleSystem* particles);
WorldServicesResult chopperEffectsSpawnGroup(ChopperMunitionEffects* effects,
    uint32_t groupKey, const float frame[16]);
void chopperEffectsSweep(ChopperMunitionEffects* effects);
void chopperEffectsDispose(ChopperMunitionEffects* effects);
void chopperEffectsReleaseOwner(ChopperMunitionEffects* effects,const void* owner);
int32_t chopperEffectsTake(ChopperMunitionEffects* effects, ChopperEffectRequest* request);
void chopperEffectCompleted(ChopperMunitionEffects* effects,
    ChopperEffectInstance* instance, uint64_t generation);
WorldServicesResult chopperEffectsCreatePersistentGroup(ChopperMunitionEffects* effects,
    uint32_t groupKey, ChopperEffectInstance** first, ChopperEffectInstance** last);
void chopperEffectsReleasePersistentGroup(ChopperMunitionEffects* effects,
    ChopperEffectInstance** first, ChopperEffectInstance** last);
WorldServicesResult chopperEffectsEmit(ChopperMunitionEffects* effects,
    ChopperEffectInstance* instance, const float frame[16], float elapsed);

struct CarMovement;
typedef struct WorldParticleGroup WorldParticleGroup;

typedef struct WorldParticleCycle {
    float duration;
    float variation;
} WorldParticleCycle;

/* Decoded CPU parameters. Authored records are never overlaid on host
 * pointers or used as native linked-list nodes. */
typedef struct WorldParticleDefinition {
    uint32_t key;
    uint32_t textureKey;
    uint32_t groupKey;
    int8_t conicalEmission;
    int8_t atlasWidthPower;
    int8_t atlasHeightPower;
    int8_t animationRate;
    int8_t randomAtlasFrame;
    int8_t emissionMode;
    int8_t followsCar;
    WorldParticleCycle on;
    WorldParticleCycle off;
    float emissionRate;
    float emissionRateVariation;
    float lifetime;
    float lifetimeVariation;
    float speed;
    float speedVariation;
    float directionAngle;
    float directionSpread;
    float carMotionScale;
    float carMotionVariation;
    float spawnOffset[3];
    float spawnRange[3];
    float interestDistance;
    float gravity;
    float drag;
    float colorCurve[16];
    float sizeRotationCurve[16];
    int32_t carEffectPosition;
    // Final authored definition+8C, consumed by RenderEffects128DA0.
    float maximumPixelSize;
    // The same 35021 record contains editable controls at90..FC and its
    // baked coefficients at100..17C. PAL1288E8 refreshes them in frontend.
    float controlKnots[4];
    float controlColors[16];
    float controlSizes[4];
    float controlRotations[4];
    struct WorldParticleDefinition* nextDefinition;
} WorldParticleDefinition;

typedef struct WorldParticle {
    struct WorldParticle* next;
    struct WorldParticle* previous;
    int16_t remainingTicks;
    uint16_t rotation;
    uint16_t speed;
    uint8_t carMotionScale;
    uint8_t atlasFrame;
    int8_t direction[3];
    float position[3];
    int32_t occupied;
} WorldParticle;

struct WorldParticleGroup {
    WorldParticleGroup* next;
    WorldParticleGroup* previous;
    const WorldParticleDefinition* definition;
    ChopperEffectInstance* emitter;
    struct CarMovement* car;
    WorldParticle* first;
    WorldParticle* last;
    uint64_t generation;
    int32_t occupied;
};

/* The authoring view edits the same decoded definition used by live groups.
 * Game assets retain their baked curves without a second mutable catalogue. */
struct WorldParticleSystem {
    ChopperEffectInstance* firstEmitter;
    ChopperEffectInstance* lastEmitter;
    WorldParticleGroup* firstGroup;
    WorldParticleGroup* lastGroup;
    WorldParticle* particleStorage;
    size_t particleCapacity;
    WorldParticleGroup* groupStorage;
    size_t groupCapacity;
    uint32_t* randomState;
    const CarCinematicServices* cameras;
    float interestPoints[3][4];
    WorldParticleDefinition* const* definitions;
    const int32_t* gameFlowState;
    uint32_t emitterCount;
    uint32_t particleCount;
    uint32_t groupCount;
    uint64_t droppedParticles;
    uint64_t nextGeneration;
    float stepMilliseconds;
    WorldServicesResult result;
    // Original AcidEffectSystem+40/+4D editor filter; constructor127418
    // initializes both to zero. Selection is borrowed, never dereferenced.
    const ChopperEffectInstance* selectedEmitter;
    int32_t renderSelectedOnly;
};

void worldParticlesInitialize(WorldParticleSystem* system,
    WorldParticle* particles, size_t particleCapacity,
    WorldParticleGroup* groups, size_t groupCapacity,
    uint32_t* randomState, const CarCinematicServices* cameras);
WorldServicesResult worldParticlesAddEmitter(WorldParticleSystem* system, ChopperEffectInstance* emitter);
void worldParticlesRemoveEmitter(WorldParticleSystem* system, ChopperEffectInstance* emitter);
void worldParticlesUpdateInterestPoints(WorldParticleSystem* system);
void worldParticlesDisposeGroup(WorldParticleSystem* system, WorldParticleGroup* group);
/* PAL1276A8 AcidEffectSystem::KillAllActiveEffects. Active groups and their
 * particles are retired; authored auto emitters remain registered in FX. */
void worldParticlesKillAllActiveEffects(WorldParticleSystem* system);
WorldServicesResult worldParticlesTriggerEmitter(WorldParticleSystem* system,
    ChopperEffectInstance* emitter, float elapsed, struct CarMovement* car);
void worldParticleHermiteEvaluate(float output[4], const float coefficients[16], float parameter);
int32_t worldParticleHermiteBasis(float output[16], const float values[16], const float knots[4]);

#ifdef __cplusplus
}

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct EditorMarkerTexture;

namespace hp2 {
class ResourceRegistry;
class AcidFXRuntime;

/* Owns only decoded GLOBALB resources. Runtime instances belong to the
 * effect service, independently of this immutable authored catalog. */
class ChopperEffectData final {
public:
    explicit ChopperEffectData(const std::vector<std::uint8_t>& global,
        const ChopperEffectData* sharedDefinitions=nullptr);
    ~ChopperEffectData();
    ChopperEffectData(const ChopperEffectData&) = delete;
    ChopperEffectData& operator=(const ChopperEffectData&) = delete;
    const ChopperEffectGroup* Groups() const noexcept { return groups_.data(); }
    std::size_t GroupCount() const noexcept { return groups_.size(); }
    const ChopperEffectGroup* Find(std::uint32_t key) const noexcept;
    const WorldParticleDefinition* Definition(std::uint32_t key) const noexcept;
    WorldParticleDefinition* EditableDefinition(std::uint32_t key) noexcept;
    WorldParticleDefinition* const* DefinitionList() const noexcept {return &firstDefinition_;}
    bool Register(ChopperEffectData& root) noexcept;
    void Unregister() noexcept;
    WorldServicesResult BindParticles(WorldParticleSystem& system);
    WorldServicesResult UnbindParticles() noexcept;
    const WorldParticleSystem* Particles() const noexcept {return particleSystem_;}
    void UpdateTextures(ResourceRegistry& resources);
    const EditorMarkerTexture* Texture(const ChopperEffectTemplate& resource) const noexcept;
    const EditorMarkerTexture* Texture(const WorldParticleDefinition& definition) const noexcept;

private:
    std::vector<std::unique_ptr<std::string>> names_;
    std::vector<ChopperEffectTemplate> templates_;
    std::map<std::uint32_t, std::vector<const ChopperEffectTemplate*>> members_;
    std::vector<ChopperEffectGroup> groups_;
    std::map<std::uint32_t, WorldParticleDefinition> particleDefinitions_;
    WorldParticleDefinition* firstDefinition_{};
    WorldParticleDefinition* lastDefinition_{};
    // PAL127078 walks35021 definitions and refreshes their+14 texture cell.
    // Templates sharing one definition must share this one mutable cache.
    std::map<const WorldParticleDefinition*,const EditorMarkerTexture*> textures_;
    ChopperEffectData* nextCatalogue_{};
    ChopperEffectData* catalogueRoot_{};
    std::vector<ChopperEffectInstance> authoredEmitters_;
    WorldParticleSystem* particleSystem_{};
};

class EcstasyFrame;
struct WorldParticleRenderSummary {
    std::size_t groups{},particles{},drawnGroups{},drawnParticles{};
    std::size_t hiddenCarGroups{},missingTextures{},centerClipped{},sizeClipped{},distanceClipped{};
};
// Source RenderEffects128DA0. It borrows the one current particle/catalogue
// owner and copies its already-quantized draw arguments into this frame.
WorldParticleRenderSummary RenderWorldParticles(const WorldParticleSystem& system,
    const ChopperEffectData& catalogue,const CarCinematicScreen& view,EcstasyFrame& frame);

/* Application-lifetime Windows storage for the three afxInit pools. The
 * authored catalog and live handlers borrow these owners; packed PS2 sizes
 * never determine native pointer layouts. Construct before ResourceRegistry. */
class AcidFXRuntime final {
public:
    explicit AcidFXRuntime(const CarCinematicServices& cameras);
    ~AcidFXRuntime();
    AcidFXRuntime(const AcidFXRuntime&) = delete;
    AcidFXRuntime& operator=(const AcidFXRuntime&) = delete;
    WorldParticleSystem& Particles() noexcept;
    // PAL FrameEffects2EF5E0 and WorldEffects2EF640 have distinct live
    // lists but allocate from the same afxInit particle/group pools.
    WorldParticleSystem& FrameParticles() noexcept;
    void BindGameFlow(const std::int32_t& state) noexcept;
    ChopperEffectInstance* CreateFrameEmitter(std::uint32_t key,const void* owner);
    void ReleaseFrameEmitter(ChopperEffectInstance& emitter,const void* owner);
    void BindCatalogue(ChopperEffectData& catalogue);
    void ReleaseCatalogue(ChopperEffectData& catalogue);
    ChopperMunitionEffects& Emitters();
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

} // namespace hp2

#endif

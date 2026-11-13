#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

struct EditorMarkerTexture;
struct EditorMarkerResources;
struct EditorMarkerLightMaterialCatalogue;
struct WorldSpatialNode;
struct CarCinematicScreen;

namespace hp2 {
class ResourceRegistry;

struct SceneLight {
    std::array<char,32> name{};
    std::uint32_t key{};
    std::uint8_t kind{};
    std::uint8_t ambientMode{};
    std::uint32_t color{};
    std::array<float,3> position{};
    float radius{};
    std::array<float,3> direction{};
    float coefficient{};
    float innerRadius{};
};

class LightGroup final {
public:
    LightGroup();
    ~LightGroup();
    LightGroup(const LightGroup&)=delete;
    LightGroup& operator=(const LightGroup&)=delete;
    std::size_t LightCount() const noexcept;
    const SceneLight& LightAt(std::size_t index) const;
    std::size_t NodeCount() const noexcept;
    const WorldSpatialNode* QueryLeaf(const float position[3]) const;
private:
    friend class LightSceneResources;
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

struct LightFlare {
    std::uint32_t markerKey{};
    std::uint8_t active{1},kind{};
    std::uint16_t flags{};
    std::array<char,32> name{};
    std::array<float,4> position{},normal{};
};
struct LightFlarePack {std::vector<LightFlare> flares;};

struct SceneLightQuery {
    std::array<const SceneLight*,10> lights{};
    std::array<float,10> weights{};
    std::array<float,4> ambient{};
};

// Native storage of the released eLightContext. These are the local-space
// inputs consumed by the original normal/reflection/material operations;
// no console DMA state or substitute lighting model is retained here.
struct SceneLightContext {
    std::array<float,16> colorMatrix{};
    std::array<float,16> directionMatrix{};
    std::array<float,16> lightPositions{};
    std::array<float,4> localEye{};
    std::int32_t lightCount{};
    float sunlightIntensity{};
    std::array<float,16> environmentMatrix{};
};

struct FrontendLightPreset {
    std::array<std::array<float,4>,3> colors{};
    std::array<std::array<float,4>,3> positions{};
    std::array<float,4> ambient{};
};

class LightSceneRegistry final {
public:
    const std::vector<const LightGroup*>& Groups() const noexcept {return groups_;}
    const std::vector<const LightFlarePack*>& FlarePacks() const noexcept {return flares_;}
    const LightFlare* FindFlare(std::uint32_t key) const noexcept;
    std::int32_t Query(SceneLightQuery& output,const float position[3]) const;
    // Same writable source32FA54 query input, not an immutable data license.
    float ambientSearchLimitSquared;
    // Source eLight globals initialized by PAL117230..117270. Keep these
    // inputs with the one light owner, independently of the visual Sun mesh.
    std::array<float,3> ambientColor{};
    std::array<float,3> sunlightDirection{{-0.7400000095367432f,0.0f,0.550000011920929f}};
    std::array<float,3> sunlightColor{{80.0f,80.0f,80.0f}};
    // Original writable frontend light sets. The two player sets remain
    // zero until the source SelectCar constructor/Tick publishes them.
    FrontendLightPreset frontendLights;
    FrontendLightPreset selectCarBaseLights,selectCarAlternateLights;
    std::array<FrontendLightPreset,2> selectedCarLights{};
    LightSceneRegistry();
private:
    friend class LightSceneResources;
    std::vector<const LightGroup*> groups_;
    std::vector<const LightFlarePack*> flares_;
};

void SetSelectCarLighting(LightSceneRegistry& registry,std::int32_t view,float amount,std::int32_t alternatePreset);

// The in-race branch of PAL115978..116118. Frontend selectable-car light
// presets have a separate source caller and are not silently substituted.
SceneLightContext SetupRaceLightContext(const LightSceneRegistry& registry,
    const float localToWorld[16],const float worldToView[16],const float worldEye[3]);

// Full PAL115978 context entry; frontend presets are owned by the same
// light registry and are selected by the original frontend screen owner.
SceneLightContext SetupLightContext(const LightSceneRegistry& registry,
    const float localToWorld[16],const float worldToView[16],const float worldEye[3],
    const CarCinematicScreen& view,std::int32_t applicationState);

// Reuse an already calculated local eye when only the environment camera
// differs. The local context and localToWorld must describe the same object.
void SetLightContextEnvironment(SceneLightContext& context,
    const float localToWorld[16],const float worldToView[16]);

// Short-lived native scratch for one object's body, wheels and brakes.
// It borrows the same read-only light groups during this call only; no
// cross-frame cache, resource ownership or approximate spatial buckets.
class LightContextBatch final {
public:
    explicit LightContextBatch(const LightSceneRegistry& registry):registry_(registry) {}
    SceneLightContext Setup(const float localToWorld[16],const float worldToView[16],
        const float worldEye[3],const CarCinematicScreen& view,std::int32_t applicationState);
    SceneLightContext SetupRace(const float localToWorld[16],const float worldToView[16],const float worldEye[3]);
    std::size_t QueryCount() const noexcept {return queries_;}
    std::size_t QueryHits() const noexcept {return hits_;}
private:
    struct Sample {
        std::array<std::uint32_t,4> key{};
        SceneLightQuery query;
        std::int32_t count{};
    };
    const Sample& Query(const float position[3]);
    const LightSceneRegistry& registry_;
    std::array<Sample,9> samples_{};
    Sample overflow_;
    std::size_t used_{},queries_{},hits_{};
};

class EcstasyFrame;
std::size_t RenderLightFlare(const LightFlare& flare,const float localToWorld[16],
    float intensity,ResourceRegistry& resources,const CarCinematicScreen& view,EcstasyFrame& frame);
std::size_t RenderWorldLightFlares(const LightSceneRegistry& lights,ResourceRegistry& resources,
    const CarCinematicScreen& view,EcstasyFrame& frame,bool inGame);

// One retained file attaches the actual light groups/presets to the registry
// borrowed by ResourceLoader and Draw. Material3401E stays with its existing
// sole LightMaterialResources owner.
class LightSceneResources final {
public:
    LightSceneResources(const std::vector<std::uint8_t>& bundle,LightSceneRegistry& registry);
    ~LightSceneResources();
    LightSceneResources(const LightSceneResources&)=delete;
    LightSceneResources& operator=(const LightSceneResources&)=delete;
private:
    LightSceneRegistry& registry_;
    std::vector<std::uint8_t> authored_;
    std::vector<std::unique_ptr<LightGroup>> groups_;
    std::vector<std::unique_ptr<LightFlarePack>> flares_;
};

class LightMaterialResources final {
public:
    LightMaterialResources(const std::vector<std::uint8_t>& bundle,EditorMarkerResources& bank);
    ~LightMaterialResources();
    LightMaterialResources(const LightMaterialResources&)=delete;
    LightMaterialResources& operator=(const LightMaterialResources&)=delete;
private:
    std::vector<std::uint8_t> authored_;
    std::unique_ptr<EditorMarkerLightMaterialCatalogue> catalogue_;
};

// The three retained texture cells used by eLightUpdateTextures. These
// pointers borrow the one resource bank; no texture records are duplicated.
class LightTextureResources final {
public:
    void UpdateTextures(ResourceRegistry& resources);
    const EditorMarkerTexture* At(std::size_t index) const;
private:
    std::array<const EditorMarkerTexture*,3> textures_{};
};
}

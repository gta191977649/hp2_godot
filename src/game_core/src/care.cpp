#include "indep/inc/RaceCarModels.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/NativeRenderSettings.hpp"

#include "bware/inc/bWare.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/TerrainEffects.hpp"
#include "indep/inc/RaceEffects.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/CarDamage.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Texture.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/eEnvMap.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/TerrainType.hpp"
#include "indep/inc/FlailerChunks.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdlib>
#include <new>
#include <utility>


// Exact final source debug controls at32FC20/32FC24, initialized to zero.
std::int32_t carRenderForceHeadlightsOn=0;
std::int32_t carRenderForceBrakelightsOn=0;
float CarScaleMatrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
std::int32_t carRenderDriverVisibilityOverride=0;
// PAL32FC60/32FC64; individually matched beta CarE.obj globals. The
// released -1 sentinel leaves each independently selected mesh LOD intact.
std::int32_t ForceCarLOD=-1;
std::int32_t ForceTireLOD=-1;

uint32_t carDamageItemLevel(uint32_t levels, uint32_t item)
{
    return (levels >> (item * 2)) & 3u;
}

struct CarAppearance {
    CarAppearanceSelection* selection;
    const hp2::CarInfoResource* info;
    hp2::ResourceRegistry* textures;
    const hp2::CarTextureNamingPolicy* naming;
    hp2::CarUsedTextureInfo used;
    std::array<EditorMarkerReplacement,2> genericReplacements{};
    std::array<EditorMarkerReplacement,12> bodyReplacements{};
    std::array<EditorMarkerReplacement,1> dashboardReplacements{};
    std::array<EditorMarkerReplacement,3> tireReplacements{};
    std::array<EditorMarkerModel,26> models{};
    std::array<CarPartCuller,14> cullers{};
    std::vector<CarLightFlare> flares;
    const hp2::CarDefinition* definition{};
    const hp2::CarRenderBindings* bindings{};
    EditorMarkerTexture* shadowTexture{};
    EditorMarkerTexture* auxiliaryTexture{};
    const EditorMarkerLightMaterial* lightMaterial{};
    std::uint32_t brokenLightMask{},randomPhase{};
    float origin[4]{},minimum[4]{},maximum[4]{};
    bool visualOwned{};
    ~CarAppearance() {for (auto& model:models)editorMarkerModelDispose(&model);}
    WorldParticleSystem* particles = nullptr;
    TerrainEffectEmitters* const* terrain = nullptr;
    std::array<std::vector<ChopperEffectInstance>,26> emitters;
    hp2::CarEffectsResources* effectResources{};
    std::array<std::vector<hp2::CarEffectPosition*>,28> effectPositions;
};

std::size_t carRenderNativeDetailLod(const EditorMarkerModel* models,
    std::int32_t originalPixels) noexcept
{
    constexpr float minimumPixels[4]={120.0f,45.0f,20.0f,0.0f}; // PAL2AD0E8
    const auto detailPixels=hp2::nativeDetailLodPixels(originalPixels,
        hp2::nativeVehicleDetailDistanceFactor);
    std::size_t lod=0;
    while(lod<3 && static_cast<float>(detailPixels)<minimumPixels[lod] &&
        models[lod+1].solid)++lod;
    return lod;
}

namespace {
void carAppearanceApplySelection(CarAppearance* appearance, CarAppearanceSelection* selection)
{
    appearance->selection = selection;
    appearance->used = hp2::BuildCarTextureInfo(*appearance->info, *selection, 0, *appearance->naming);
    const auto& defaults = appearance->used.defaultSkinKeys;
    const auto& skins = appearance->used.skinKeys;
    appearance->genericReplacements[0].originalKey = defaults[0];
    appearance->genericReplacements[0].replacementKey = skins[0];
    appearance->genericReplacements[1].originalKey = defaults[1];
    appearance->genericReplacements[1].replacementKey = skins[1];
    appearance->bodyReplacements[0].originalKey = defaults[0];
    appearance->bodyReplacements[0].replacementKey = skins[0];
    appearance->bodyReplacements[1].originalKey = defaults[1];
    appearance->bodyReplacements[1].replacementKey = skins[1];
    appearance->tireReplacements[0].originalKey = defaults[0];
    appearance->tireReplacements[0].replacementKey = skins[0];
}

void carAppearanceUpdateTextureInfo(CarAppearance* appearance, EditorMarkerReplacement* replacement)
{
    replacement->texture = appearance->textures->ResolveTexture(replacement->replacementKey,true);
    hp2::Require(replacement->texture!=nullptr,"Car appearance has no actual loaded texture/default owner");
}

void carAppearanceUpdateSkinTextures(CarAppearance* appearance)
{
    for (auto& replacement : appearance->genericReplacements) carAppearanceUpdateTextureInfo(appearance, &replacement);
    for (std::size_t index=0;index<2;++index) carAppearanceUpdateTextureInfo(appearance,&appearance->bodyReplacements[index]);
    carAppearanceUpdateTextureInfo(appearance,&appearance->tireReplacements[0]);
}
}

void carAppearanceDispose(Car* car)
{
    if (car) {
        carAppearanceReleaseEffects(car);
        if (car->appearance && car->appearance->visualOwned) {
            carDriverVisualDestroy(car->visual);car->visual=nullptr;
        }
        delete car->appearance;
        car->appearance = nullptr;
    }
}

const hp2::CarUsedTextureInfo* carAppearanceTextures(const Car* car)
{
    return car && car->appearance ? &car->appearance->used : nullptr;
}

const EditorMarkerTexture* carAppearanceReplacement(const Car* car, std::size_t slot)
{
    if (!car || !car->appearance || slot>=5)return nullptr;
    const auto& appearance=*car->appearance;
    if (slot<2)return appearance.genericReplacements[slot].texture;
    if (slot<4)return appearance.bodyReplacements[slot-2].texture;
    return appearance.tireReplacements[0].texture;
}

CarAppearanceResult carSwitchSkin(Car* car, int32_t skinNumber)
{
    if (!car || !car->driverInfo || !car->appearance ||
        car->appearance->selection != &car->driverInfo->appearance)
        return CAR_APPEARANCE_INVALID_BINDING;
    const std::uint8_t byte = static_cast<std::uint8_t>(skinNumber);
    std::memcpy(&car->driverInfo->appearance.skinNumber, &byte, sizeof(byte));
    try {
        carAppearanceApplySelection(car->appearance, &car->driverInfo->appearance);
        carAppearanceUpdateSkinTextures(car->appearance);
        return CAR_APPEARANCE_OK;
    } catch (...) { return CAR_APPEARANCE_INVALID_SELECTION; }
}

namespace {
const char* const carEffectNames[26] = {
    "CARFX_NONE", "CARFX_DRIVE_OVER", "CARFX_DRIVE_OVER2", "CARFX_SKID_SMOKE",
    "CARFX_TIRE_SPEW", "CARFX_BOTTOM_OUT", "CARFX_DAM_RADIATOR", "CARFX_DAM_ENGINE",
    "CARFX_BLOWN_TIRE", "CARFX_DRIVE_ON_FLAT_TIRE", "CARFX_NITRO", "CARFX_EXHAUST_SMOKE",
    "CARFX_BREAK_SIDE_MIRROR_LEFT", "CARFX_BREAK_SIDE_MIRROR_RIGHT",
    "CARFX_BREAK_LICENSE_PLATE_FRONT", "CARFX_BREAK_LICENSE_PLATE_RIGHT",
    "CARFX_BREAK_HEADLIGHT_LEFT", "CARFX_BREAK_HEADLIGHT_RIGHT",
    "CARFX_BREAK_BRAKELIGHT_LEFT", "CARFX_BREAK_BRAKELIGHT_RIGHT",
    "CARFX_BREAK_WINDSHIELD", "CARFX_BREAK_WINDOW_REAR",
    "CARFX_BREAK_WINDOW_LEFT_FRONT", "CARFX_BREAK_WINDOW_LEFT_REAR",
    "CARFX_BREAK_WINDOW_RIGHT_FRONT", "CARFX_BREAK_WINDOW_RIGHT_REAR"
};

void appendCarPosition(CarAppearance& appearance,std::size_t list,const std::array<float,3>& value)
{
    auto* slot=appearance.effectResources->AcquirePosition(value);
    try { appearance.effectPositions[list].push_back(slot); }
    catch (...) {appearance.effectResources->ReleasePosition(slot);throw;}
}

void appendCarMarker(CarAppearance& appearance,uint32_t key,const std::array<float,3>& position)
{
    // Several body markers deliberately belong to two position lists.
    switch(key) {
    case 0x59A55DB5u: appendCarPosition(appearance,9,position); break;
    case 0xA2799C2Bu: case 0xA5B28001u: case 0xF21BE91Eu:
        appendCarPosition(appearance,10,position); break;
    case 0x68100815u: appendCarPosition(appearance,11,position); break;
    case 0x7B220DDFu: appendCarPosition(appearance,12,position); break;
    case 0x1B0763A0u: appendCarPosition(appearance,13,position); break;
    case 0xE7E4EF49u: appendCarPosition(appearance,14,position); break;
    case 0x4CDEBFCAu: appendCarPosition(appearance,15,position); break;
    case 0x60F8B13Cu: appendCarPosition(appearance,16,position); break;
    case 0x0AB88F5Du: appendCarPosition(appearance,17,position); break;
    case 0xA7E6EA53u:
        appendCarPosition(appearance,18,position);
        appendCarPosition(appearance,20,position); break;
    case 0xA532FC46u:
        appendCarPosition(appearance,18,position);
        appendCarPosition(appearance,21,position); break;
    case 0xD947F346u:
        appendCarPosition(appearance,19,position);
        appendCarPosition(appearance,22,position); break;
    case 0x02B52399u:
        appendCarPosition(appearance,19,position);
        appendCarPosition(appearance,23,position); break;
    case 0x05F63788u: appendCarPosition(appearance,24,position); break;
    case 0xC52BF01Bu: appendCarPosition(appearance,25,position); break;
    case 0x725758BFu: appendCarPosition(appearance,26,position); break;
    case 0x494ED280u: appendCarPosition(appearance,27,position); break;
    default: break;
    }
}
}

namespace hp2 {
struct CarEffectsResources::Storage {
    std::array<CarEffectPosition,Capacity> positions{};
    std::array<bool,Capacity> occupied{};
    std::array<std::uint32_t,26> keys{};
    std::array<float,16> identity{};
    std::array<std::uint32_t,12> material{};
    std::size_t used{};
};
CarEffectsResources::CarEffectsResources():storage_(std::make_unique<Storage>())
{
    for(std::size_t i=0;i<storage_->keys.size();++i)storage_->keys[i]=hashString(carEffectNames[i]);
    eIdentity(storage_->identity.data());
    // Exact final PAL2E0390 stores. Beta has a different parameter layout.
    for(std::size_t i=3;i<=5;++i)storage_->material[i]=0x3f800000u;
    storage_->material[8]=0x80808080u;storage_->material[10]=0x3f800000u;
}
CarEffectsResources::~CarEffectsResources()
{
    // CloseCarEffects checks the real slot pool before its release. A dangling
    // Car binding is a lifecycle error, not permission to manufacture slots.
    assert(storage_->used==0 && "CarEffects destroyed before its live Cars");
}
std::size_t CarEffectsResources::UsedPositions() const noexcept {return storage_->used;}
std::uint32_t CarEffectsResources::EffectKey(std::size_t effect) const
{
    Require(effect<storage_->keys.size(),"Car effect index outside final26-key table");
    return storage_->keys[effect];
}
const std::array<float,16>& CarEffectsResources::Identity() const noexcept {return storage_->identity;}
const std::array<std::uint32_t,12>& CarEffectsResources::MaterialWords() const noexcept {return storage_->material;}
CarEffectPosition* CarEffectsResources::AcquirePosition(const std::array<float,3>& position)
{
    Require(storage_->used<Capacity,"CarEmitterPositionSlotPool exhausted its350 source slots");
    for(std::size_t i=0;i<Capacity;++i)if(!storage_->occupied[i]) {
        storage_->occupied[i]=true;++storage_->used;
        storage_->positions[i].value=position;return &storage_->positions[i];
    }
    throw std::logic_error("CarEmitterPositionSlotPool occupancy is corrupt");
}
void CarEffectsResources::ReleasePosition(CarEffectPosition* position) noexcept
{
    if(!position)return;
    for(std::size_t i=0;i<Capacity;++i)if(position==&storage_->positions[i]) {
        assert(storage_->occupied[i]);
        storage_->occupied[i]=false;--storage_->used;return;
    }
    assert(false && "Position belongs to another CarEffects owner");
}
}

CarEffectResult carAppearanceBindEffects(Car* car,const EditorMarkerModel* body,
    const PositionMarkerRegistry* markers,WorldParticleSystem* particles,
    TerrainEffectEmitters* const* terrain,const ChopperEffectGroup* groups,std::size_t groupCount,
    hp2::CarEffectsResources* resources)
{
    if (!car || !car->appearance || car->appearance->particles || !body || !body->solid ||
        !markers || !particles || !terrain || !resources || (groupCount && !groups) ||
        (*terrain && terrainEffectEmittersParticles(*terrain)!=particles))
        return CAR_EFFECT_INVALID_BINDING;
    auto& appearance = *car->appearance;
    appearance.effectResources=resources;
    try {
        const auto& definition = appearance.info->Car(static_cast<std::size_t>(appearance.selection->modelCode));
        appendCarPosition(appearance,0,{0,0,0});
        const int pairs[4][2] = {{0,1},{3,2},{0,3},{1,2}};
        for (std::size_t index = 0; index < 4; ++index) {
            std::array<float,3> average{};
            for (std::size_t axis = 0; axis < 3; ++axis)
                average[axis] = (definition.wheels[pairs[index][0]].position[axis] +
                    definition.wheels[pairs[index][1]].position[axis]) * 0.5f;
            appendCarPosition(appearance,index+1,average);
            appendCarPosition(appearance,index+5,definition.wheels[index].position);
        }
        // PAL11DCFC hashes CarTypeInfo+20: shared geometry/marker name.
        // Reward variants retain their own model index and skin selection.
        const uint32_t groupKey = hashString(definition.baseName.c_str());
        const PositionMarker* marker = nullptr;
        while ((marker = positionMarkerFind(markers,0,groupKey,marker)) != nullptr) {
            std::array<float,3> position{};
            for (std::size_t axis = 0; axis < 3; ++axis)
                position[axis] = marker->position[axis] - body->solid->origin[axis];
            appendCarMarker(appearance,marker->key,position);
        }
        for (std::size_t effect = 0; effect < appearance.emitters.size(); ++effect) {
            const uint32_t key = resources->EffectKey(effect);
            for (std::size_t group = 0; group < groupCount; ++group) {
                if (groups[group].key != key) continue;
                if (groups[group].count && !groups[group].members)
                    throw std::runtime_error("Invalid car effect group");
                auto& instances = appearance.emitters[effect];
                instances.resize(groups[group].count);
                for (std::size_t index = 0; index < instances.size(); ++index) {
                    const auto* resource = groups[group].members[index];
                    if (!resource || resource->groupKey != key || !resource->particleDefinition ||
                        resource->particleDefinition->carEffectPosition < 0 ||
                        resource->particleDefinition->carEffectPosition >= 28)
                        throw std::runtime_error("Invalid car emitter position binding");
                    auto& instance = instances[index];
                    instance = {};
                    instance.resource = resource;
                    instance.particleDefinition = resource->particleDefinition;
                    std::memcpy(instance.frame,resource->frame,sizeof(instance.frame));
                    instance.occupied = 1;
                }
                break;
            }
        }
        appearance.terrain = terrain;
        appearance.particles = particles;
        return CAR_EFFECT_OK;
    } catch (...) {
        carAppearanceReleaseEffects(car);
        return CAR_EFFECT_INVALID_BINDING;
    }
}

void carAppearanceReleaseEffects(Car* car)
{
    if (!car || !car->appearance) return;
    auto& appearance = *car->appearance;
    for (auto& group : appearance.emitters) {
        for (auto& instance : group) {
            if (instance.particleSystem) worldParticlesRemoveEmitter(instance.particleSystem,&instance);
            if (appearance.particles)
                for (WorldParticleGroup* live = appearance.particles->firstGroup; live; live = live->next)
                    if (live->emitter == &instance) live->emitter = nullptr;
        }
        group.clear();
    }
    for (auto& positions : appearance.effectPositions) {
        for(auto* slot:positions)appearance.effectResources->ReleasePosition(slot);
        positions.clear();
    }
    appearance.effectResources=nullptr;
    appearance.particles = nullptr;
    appearance.terrain = nullptr;
}

const float* carAppearanceEffectPosition(const Car* car,std::size_t position,std::size_t index)
{
    if (!car || !car->appearance || position >= car->appearance->effectPositions.size()) return nullptr;
    const auto& positions = car->appearance->effectPositions[position];
    return index < positions.size() ? positions[index]->value.data() : nullptr;
}

CarEffectResult carTriggerEffect(CarMovement* movement,int32_t effect,float strength,int32_t key,const float* velocity)
{
    if (!movement || !movement->bindings.owner) return CAR_EFFECT_INVALID_BINDING;
    auto* appearance = movement->bindings.owner->appearance;
    if (!appearance || strength == 0.0f || effect == 0) return CAR_EFFECT_OK;
    if (!movement->bindings.ownerMotion || !movement->bindings.state) return CAR_EFFECT_INVALID_BINDING;
    if (movement->bindings.ownerMotion->wheelExclusionMask) return CAR_EFFECT_OK;
    if (!appearance->particles || !appearance->terrain || effect < 0 || effect >= 26)
        return CAR_EFFECT_INVALID_BINDING;
    auto& own = appearance->emitters[static_cast<std::size_t>(effect)];
    ChopperEffectInstance* emitters = own.data();
    std::size_t count = own.size();
    if (effect >= 1 && effect <= 5 && *appearance->terrain) {
        if(terrainEffectEmittersParticles(*appearance->terrain)!=appearance->particles)
            return CAR_EFFECT_INVALID_BINDING;
        const int32_t material = static_cast<int32_t>(static_cast<uint32_t>(key) & 0xFFFFu);
        if (material >= 33) return CAR_EFFECT_INVALID_BINDING;
        emitters = terrainEffectEmittersGroup(*appearance->terrain,effect-1,material,&count);
    }
    const int32_t positionOverride = (effect == 3 || effect == 4 || effect == 8 || effect == 9) ? key >> 16 : 0;
    if (positionOverride < 0 || positionOverride >= 28) return CAR_EFFECT_INVALID_BINDING;
    const auto& state = *movement->bindings.state;
    const float* center = positionOverride >= 1 && positionOverride < 9 ? state.position : state.centerPosition;
    if (!center) return CAR_EFFECT_INVALID_BINDING;
    float frame[16];
    std::memcpy(frame,state.orientation,sizeof(frame));
    std::memcpy(frame+12,center,3*sizeof(float));
    frame[15] = 1.0f;
    const float* inheritedVelocity = velocity ? velocity : state.velocity;
    for (std::size_t index = 0; index < count; ++index) {
        auto& emitter = emitters[index];
        const auto* definition = emitter.particleDefinition;
        if (!definition) return CAR_EFFECT_INVALID_BINDING;
        const int32_t position = positionOverride ? positionOverride : definition->carEffectPosition;
        if (position < 0 || position >= 28) return CAR_EFFECT_INVALID_BINDING;
        float saved[16],local[16];
        std::memcpy(saved,emitter.frame,sizeof(saved));
        std::memcpy(local,saved,sizeof(local));
        for (const auto& offset : appearance->effectPositions[static_cast<std::size_t>(position)]) {
            std::memcpy(local+12,offset->value.data(),3*sizeof(float));
            local[15] = 1.0f;
            vectorMatrixMultiply(emitter.frame,frame,local);
            std::memcpy(emitter.carVelocity,inheritedVelocity,3*sizeof(float));
            emitter.carVelocity[3] = 0.0f;
            const auto result = worldParticlesTriggerEmitter(appearance->particles,&emitter,-strength,movement);
            if (result != WORLD_SERVICES_OK) {
                std::memcpy(emitter.frame,saved,sizeof(saved));
                return CAR_EFFECT_PARTICLE_FAILURE;
            }
        }
        std::memcpy(emitter.frame,saved,sizeof(saved));
    }
    return CAR_EFFECT_OK;
}

// Final124528 static construction; no beta table or zero-filled ELF data.
CarPartCullingPlaneInfo carPartCullingPlanes[14] = {
    {0,3,{{-0.009999999776482582f,-1.0f,0.0f,0.0f},{0.07999999821186066f,0.15000000596046448f,1.0f,0.0f},{-1.0f,0.0f,2.0f,0.0f}},{0.10000000149011612f,0.0f,0.05000000074505806f,0.0f}},
    {0,3,{{-0.009999999776482582f,1.0f,0.0f,0.0f},{0.07999999821186066f,-0.15000000596046448f,1.0f,0.0f},{-1.0f,0.0f,2.0f,0.0f}},{0.10000000149011612f,0.0f,0.05000000074505806f,0.0f}},
    {0,3,{{0.009999999776482582f,1.0f,0.0f,0.0f},{-0.07999999821186066f,-0.15000000596046448f,1.0f,0.0f},{0.4472140073776245f,0.0f,0.33500000834465027f,0.0f}},{0.10000000149011612f,0.0f,0.05000000074505806f,0.0f}},
    {0,3,{{0.009999999776482582f,-1.0f,0.0f,0.0f},{-0.07999999821186066f,0.15000000596046448f,1.0f,0.0f},{0.4472140073776245f,0.0f,0.33500000834465027f,0.0f}},{0.10000000149011612f,0.0f,0.05000000074505806f,0.0f}},
    {1,1,{{0.0f,-1.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.0f,0.0f,0.0f,0.0f}},
    {1,1,{{0.0f,1.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.0f,0.0f,0.0f,0.0f}},
    {1,2,{{0.2199999988079071f,-1.0f,0.0f,0.0f},{-0.2199999988079071f,-1.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.0f,0.05000000074505806f,0.0f,0.0f}},
    {1,2,{{0.2199999988079071f,1.0f,0.0f,0.0f},{-0.2199999988079071f,1.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.0f,0.05000000074505806f,0.0f,0.0f}},
    {1,2,{{0.2199999988079071f,1.0f,0.0f,0.0f},{-0.2199999988079071f,1.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.0f,0.05000000074505806f,0.0f,0.0f}},
    {1,2,{{0.2199999988079071f,-1.0f,0.0f,0.0f},{-0.2199999988079071f,-1.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.0f,0.05000000074505806f,0.0f,0.0f}},
    {0,1,{{-1.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.0f,0.0f,0.0f,0.0f}},
    {0,1,{{1.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.0f,0.0f,0.0f,0.0f}},
    {1,2,{{-0.07000000029802322f,0.0f,1.0f,0.0f},{0.07000000029802322f,0.0f,1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.05000000074505806f,0.05000000074505806f,0.0f,0.0f}},
    {1,1,{{0.0f,0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},{0.0f,0.0f,0.0f,0.0f}},
};
std::int32_t carPartCullingNormalizeOnQuery=0;

namespace {
float carDot3(const float* a,const float* b)
{
    const float x=a[0]*b[0],y=a[1]*b[1],z=a[2]*b[2];
    return (x+y)+z;
}
void normalizeCarVector(float value[4])
{
    const float scale=finiteRatio(1.0f,std::sqrt(carDot3(value,value)));
    for (std::size_t axis=0;axis<4;++axis)value[axis]*=scale;
}
CarDamage* carRenderDamage(Car* car)
{
    auto* movement=car ? static_cast<CarMovement*>(car->movement) : nullptr;
    return movement ? movement->bindings.damage : nullptr;
}
}

void carPartCullerInitialize(CarPartCuller* parts,std::int32_t part,const float position[4])
{
    // PAL11D580 normalizes the shared definition, then writes this instance.
    hp2::Require(parts && position && part>=0 && part<14,"Invalid CarPartCuller binding");
    auto& info=carPartCullingPlanes[part];
    for (std::int32_t plane=0;plane<info.count;++plane) {
        auto& normal=info.normals[plane];
        if (std::fabs(1.0f-std::sqrt(carDot3(normal,normal)))>0.009999999776482582f)
            normalizeCarVector(normal);
    }
    std::memcpy(parts[part].position,position,4*sizeof(float));
}

void carPartCullerCull(CarPartCuller* parts,const float eye[4])
{
    // Complete PAL11D980, including both plane-combination modes, mutable
    // per-query normalization switch and separate visible/culled counters.
    for (std::size_t part=0;part<14;++part) {
        auto& state=parts[part];const auto& info=carPartCullingPlanes[part];
        std::int32_t culled=1;
        if (info.count>0) {
            float direction[4];
            for (std::size_t axis=0;axis<4;++axis)direction[axis]=eye[axis]-state.position[axis];
            if (info.combine==0 || info.combine==1) {
                std::int32_t plane=0;
                for (;plane<info.count;++plane) {
                    float normal[4];std::memcpy(normal,info.normals[plane],sizeof(normal));
                    if (carPartCullingNormalizeOnQuery)normalizeCarVector(normal);
                    const float distance=carDot3(direction,normal)-info.offsets[plane];
                    if (info.combine==0 ? distance<0.0f : distance>=0.0f)break;
                }
                if (info.combine==0 ? plane==info.count : plane<info.count)culled=0;
            }
        }
        state.culled=culled;
        if (culled)++state.culledQueries;else ++state.visibleQueries;
    }
}

namespace {
void updateCarRenderReplacementTextures(CarAppearance& render,Car* car,bool reconnect=true)
{
    const auto* damage=carRenderDamage(car);
    const std::uint32_t levels=damage ? damage->levels : 0;
    constexpr std::uint32_t windows[4]={0x834b8017u,0x0a155545u,0x0a155546u,0x0a155547u};
    for (std::uint32_t item=0;item<6;++item)
        render.bodyReplacements[item+2].replacementKey=windows[carDamageItemLevel(levels,item+8)];
    render.brokenLightMask=0;
    for (std::uint32_t light=0;light<4;++light) {
        auto& replacement=render.bodyReplacements[light+8];
        if (carDamageItemLevel(levels,light+4)>=2) {
            replacement.replacementKey=render.used.lightingKeys[light<2 ? 2:5];
            render.brokenLightMask|=1u<<light;
        } else replacement.replacementKey=replacement.originalKey;
    }
    if ((render.brokenLightMask&12u)==12u)render.brokenLightMask|=16u;
    if(reconnect)for (std::size_t lod=0;lod<3;++lod)editorMarkerModelReconnectTextureTable(&render.models[lod]);
}

void updateCarRenderLightStateTextures(CarAppearance& render,
    const CarFrameState* state,std::int32_t driverClass,const CarControls* controls,
    const TrackScenarioDefinition* scenario,std::int32_t worldFrames,bool reconnect=true)
{
    // Complete PAL120130..120360. Final traffic blink inversion, timeOfDay4
    // and brake-only control differ from beta. Use the actual live state.
    constexpr std::uint8_t policeHeadlights[16]={1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0};
    const bool night=scenario && (scenario->timeOfDay==2 || scenario->timeOfDay==3 || scenario->timeOfDay==4);
    bool left=false,right=false;
    if (state) {
        auto visible=static_cast<std::uint32_t>(state->blinkVisible);
        if (driverClass==3)visible^=1u;
        if (visible)left=right=true;
        else if (state->signalMode) {
            const auto phase=(render.randomPhase+static_cast<std::uint32_t>(worldFrames))&15u;
            left=policeHeadlights[phase]!=0;right=!left;
        }
    }
    if (night || carRenderForceHeadlightsOn)left=right=true;
    const bool brake=carRenderForceBrakelightsOn || (state && controls && controls->brake>0.05000000074505806f);
    bool changed=false;
    for (std::size_t lamp=0;lamp<4;++lamp) {
        auto& replacement=render.bodyReplacements[lamp+8];
        const std::size_t base=lamp<2 ? 0:3;
        if (replacement.replacementKey==render.used.lightingKeys[base+2])continue;
        const bool enabled=lamp==0 ? left : lamp==1 ? right : brake;
        const auto key=render.used.lightingKeys[base+(enabled ? 1:0)];
        if (replacement.replacementKey!=key) {replacement.replacementKey=key;changed=true;}
    }
    if (changed && reconnect)for (std::size_t lod=0;lod<3;++lod)editorMarkerModelReconnectTextureTable(&render.models[lod]);
}

void updateCarRenderSceneTextures(CarAppearance& render,Car* car,
    const CarFrameState* state,std::int32_t driverClass,const CarControls* controls,
    const TrackScenarioDefinition* scenario,std::int32_t worldFrames)
{
    std::array<std::uint32_t,12> previous;
    for(std::size_t index=0;index<previous.size();++index)
        previous[index]=render.bodyReplacements[index].replacementKey;
    // Damage selects the base keys; live lamps then select the final keys.
    // No consumer observes their intermediate state. Publish only the final
    // changed table, avoiding an off/on reconnect for every visible car/view.
    // Explicit source APIs above retain their immediate reconnect contract;
    // resource fixup still reconnects every live model after owner changes.
    updateCarRenderReplacementTextures(render,car,false);
    updateCarRenderLightStateTextures(render,state,driverClass,controls,scenario,worldFrames,false);
    bool changed=false;
    for(std::size_t index=0;index<previous.size();++index)
        changed|=previous[index]!=render.bodyReplacements[index].replacementKey;
    // Resource packs can reconnect while keys stay identical. Check each
    // canonical final texture once, rather than relying on pointer lifetime
    // or repeating all 12 lookups and slot scans for every body LOD.
    bool hasReplacementTable=false;
    for(std::size_t lod=0;lod<3;++lod)hasReplacementTable|=render.models[lod].replacementCount>0;
    if(!changed && hasReplacementTable) {
        hp2::Require(render.textures,"Car texture publication has no canonical registry");
        for(const auto& replacement:render.bodyReplacements)
            changed|=replacement.texture!=render.textures->ResolveTexture(replacement.replacementKey,true);
    }
    if(changed)for(std::size_t lod=0;lod<3;++lod)
        editorMarkerModelReconnectTextureTable(&render.models[lod]);
}

void createCarRenderLightFlares(CarAppearance& render,const PositionMarkerRegistry* markers)
{
    hp2::Require(markers,"Car flares lack their real marker owner");
    if (!render.definition)return;
    constexpr float directions[6][4]={{1,0,0,0},{-1,0,0,0},{0,1,0,0},
        {0,-1,0,0},{1,0,0,0},{0,1,0,0}};
    // PAL120398 selects the same +20 geometry group as InitRenderInfo.
    const std::uint32_t group=hashString(render.definition->baseName.c_str());
    const PositionMarker* marker=nullptr;
    while ((marker=positionMarkerFind(markers,0,group,marker))!=nullptr) {
        int kind=-1;std::uint8_t flags=0;
        switch (marker->key) {
        case 0x4ae7f96fu:case 0xacec665cu:kind=0;break;
        case 0x34970013u:case 0x5f09c5e2u:case 0x7efcf06fu:kind=1;break;
        case 0x944e5339u:case 0xbea9aee6u:kind=2;break;
        case 0x1e4150b4u:kind=4;flags=1;break;
        case 0xe662c161u:kind=5;flags=1;break;
        case 0xb4348dbau:kind=6;break;
        case 0xa532fc46u:kind=7;break;
        case 0xa7e6ea53u:kind=8;break;
        default:break;
        }
        if (kind<0)continue;
        const auto direction=(marker->tags>>8)&255u;
        hp2::Require(direction<6,"Car flare direction is outside the final authored table");
        CarLightFlare flare{};flare.markerKey=marker->key;flare.kind=static_cast<std::uint8_t>(kind);flare.flags=flags;
        for (std::size_t axis=0;axis<3;++axis)flare.position[axis]=marker->position[axis]-render.origin[axis];
        std::memcpy(flare.normal,directions[direction],sizeof(flare.normal));
        render.flares.push_back(flare);
    }
}
}

void carRenderUpdateReplacementTextures(Car* car)
{
    hp2::Require(car && car->appearance,"Car texture update has no render owner");
    updateCarRenderReplacementTextures(*car->appearance,car);
}
void carRenderCreateLightFlares(Car* car,const PositionMarkerRegistry* markers)
{
    hp2::Require(car && car->appearance,"Car flares lack their real render owner");
    createCarRenderLightFlares(*car->appearance,markers);
}

namespace {
std::string carModelName(const hp2::CarDefinition& definition,const char* suffix)
{
    // PAL11EC3C retains CarTypeInfo+20 in S6 for every model-name format
    // in this constructor. Official HP reward cars share the base geometry.
    return definition.baseName+suffix;
}
void carRenderConstruct(Car* car,CarAppearanceSelection& ride,CarAppearance& render,
    const hp2::CarRenderBindings& binding)
{
    // PAL11E860..11FABC. The independent full InitRenderInfo caller licenses
    // this changed4700-byte constructor, rather than an appearance prefix.
    auto& resources=binding.resources->Models();
    render.info=binding.cars;render.textures=binding.resources;render.naming=binding.naming;render.bindings=&binding;
    render.selection=&ride;
    render.randomPhase=vectorDefaultRandomBounded(8);
    const auto& selection=*render.selection;
    const auto& definition=binding.cars->Car(static_cast<std::size_t>(selection.modelCode));
    render.definition=&definition;
    const auto* skin=binding.cars->FindSkin(selection.modelCode,selection.skinNumber,true);
    hp2::Require(skin!=nullptr,"Car render construction has no authored SkinInfo");
    carAppearanceApplySelection(&render,render.selection);
    constexpr std::uint32_t partKeys[6]={0x7b220ddfu,0x1b0763a0u,0xe7e4ef49u,0x4cdebfca,
        0x60f8b13cu,0x0ab88f5du};
    for (std::size_t index=0;index<6;++index)render.bodyReplacements[index+2].originalKey=partKeys[index];
    for (std::size_t index=0;index<2;++index) {
        render.bodyReplacements[index+8].originalKey=render.used.headlightSideKeys[index];
        render.bodyReplacements[index+10].originalKey=render.used.brakeSideKeys[index];
    }
    updateCarRenderReplacementTextures(render,car);
    render.tireReplacements[1].originalKey=render.used.defaultTireKey;
    render.tireReplacements[1].replacementKey=render.used.tireKey;
    render.tireReplacements[2].originalKey=render.used.defaultTireBlurKey;
    render.tireReplacements[2].replacementKey=render.used.tireBlurKey;
    render.dashboardReplacements[0].originalKey=render.used.defaultDashboardKey;
    render.dashboardReplacements[0].replacementKey=render.used.dashboardKey;
    render.lightMaterial=binding.resources->FindLightMaterial(skin->lightMaterialKey);
    const auto initialize=[&](std::size_t slot,const std::string& name) {
        editorMarkerModelInitialize(&render.models[slot],&resources,hashString(name.c_str()));
    };
    const auto attach=[&](std::size_t slot,auto& table,std::int32_t detail) {
        editorMarkerModelAttachReplacementTextures(&render.models[slot],table.data(),
            static_cast<std::int32_t>(table.size()),detail);
    };
    const auto material=[&](std::size_t slot) {
        editorMarkerModelReplaceLightMaterial(&render.models[slot],0xd6d6080au,skin->lightMaterialKey);
    };
    for (std::size_t lod=0;lod<4;++lod) {
        const char letter=static_cast<char>('A'+lod-(definition.bodyCategory==2 && lod ? 1:0));
        std::string name=definition.baseName+'_'+letter;
        if (selection.reserved && selection.variant && lod<2)name+=std::to_string(selection.variant);
        initialize(lod,name);
        if (lod>=2 && definition.bodyCategory!=2)
            (void)editorMarkerModelRenameTexture(&render.models[lod],render.used.defaultSkinKeys[0],render.used.defaultSkinKeys[1]);
        if (lod<3)attach(lod,render.bodyReplacements,static_cast<std::int32_t>(lod));
        else attach(lod,render.genericReplacements,0);
    }
    // Source initializes all three front handles, then all three rear handles.
    std::string frontNames[3];
    for (std::size_t lod=0;lod<3;++lod) {
        const char letter=static_cast<char>('A'+lod-(definition.bodyCategory==2 && lod==2 ? 1:0));
        frontNames[lod]=definition.baseName+"_TIRE_FRONT_"+letter;
        initialize(4+lod*2,frontNames[lod]);
    }
    for (std::size_t lod=0;lod<3;++lod) {
        const char letter=static_cast<char>('A'+lod-(definition.bodyCategory==2 && lod==2 ? 1:0));
        initialize(5+lod*2,definition.baseName+"_TIRE_REAR_"+letter);
    }
    for (std::size_t lod=0;lod<3;++lod)if (!render.models[5+lod*2].solid) {
        editorMarkerModelDispose(&render.models[5+lod*2]);initialize(5+lod*2,frontNames[lod]);
    }
    for (std::size_t side=0;side<2;++side)for (std::size_t lod=0;lod<3;++lod)
        attach(4+lod*2+side,render.tireReplacements,static_cast<std::int32_t>(side*3+lod));
    for (std::size_t side=0;side<2;++side)for (std::size_t lod=0;lod<3;++lod)material(4+lod*2+side);
    const char* paired[8]={"_BRAKE_FRONT","_BRAKE_REAR","_WIPER_LEFT","_WIPER_RIGHT",
        "_SIDE_MIRROR_LEFT","_SIDE_MIRROR_RIGHT","_LICENSE_PLATE_FRONT","_LICENSE_PLATE_REAR"};
    for (std::size_t pair=0;pair<4;++pair) {
        const auto first=12+pair*2;
        initialize(first,carModelName(definition,paired[pair*2]));
        initialize(first+1,carModelName(definition,paired[pair*2+1]));
        if (pair==0 && !render.models[first+1].solid) {
            editorMarkerModelDispose(&render.models[first+1]);initialize(first+1,carModelName(definition,paired[0]));
        }
        if (pair<3) {
            attach(first,render.genericReplacements,static_cast<std::int32_t>(first-11));
            attach(first+1,render.genericReplacements,static_cast<std::int32_t>(first-10));
        }
        material(first);material(first+1);
    }
    std::string scuffs=carModelName(definition,"_SCUFFS");
    if (selection.variant)scuffs+=std::to_string(selection.variant);
    initialize(20,scuffs);
    initialize(21,carModelName(definition,"_DASH"));
    initialize(23,carModelName(definition,"_DASH_WHEEL"));
    initialize(22,carModelName(definition,"_DASH_WINDOW"));
    initialize(24,carModelName(definition,"_SPEED_NEEDLE"));
    initialize(25,carModelName(definition,"_TACH_NEEDLE"));
    attach(21,render.dashboardReplacements,0);attach(23,render.dashboardReplacements,1);
    material(21);material(23);material(22);
    initialize(10,carModelName(definition,"_WHEEL_BLUR"));
    initialize(11,carModelName(definition,"_WHEEL_BLUR"));
    for (std::size_t side=0;side<2;++side)if (!render.models[10+side].solid) {
        editorMarkerModelDispose(&render.models[10+side]);
        initialize(10+side,carModelName(definition,side ? "_WHEEL_BLUR_R":"_WHEEL_BLUR_F"));
    }
    attach(10,render.tireReplacements,6);attach(11,render.tireReplacements,7);
    const bool frontend=*binding.applicationState==3;
    const auto shadow=carModelName(definition,frontend ? "_SHADOWFE":"_SHADOWIG");
    render.shadowTexture=binding.resources->ResolveTexture(hashString(shadow.c_str()),false);
    render.auxiliaryTexture=binding.resources->ResolveTexture(0xbadb4475u,false);
    if (render.shadowTexture) {
        auto* texture=hp2::textureInfoForDescriptor(render.shadowTexture);
        hp2::Require(texture && texture->pack,"Car shadow descriptor has no physical metadata owner");
        texture->pack->SetDescriptorByte29(render.shadowTexture,frontend ? 5:3);
    }
    // The final constructor is shared with FrontEndRenderingCar, which
    // supplies a null Car at11E7BC. Only a race car owns an animated driver.
    if(car) {
        carAssignAnimatedDriver(car,&binding);
        render.visualOwned=car->visual!=nullptr;
    }
    if (auto* damage=carRenderDamage(car)) {
        auto& body=damage->body;
        const auto nameA=carModelName(definition,"_A");
        bodyDamageSetALevelInfo(&body,nameA.c_str());
        if (selection.reserved) {
            bodyDamageAddModel(&body,&render.models[0],nameA.c_str(),0);
            const auto nameB=carModelName(definition,"_B"),nameC=carModelName(definition,"_C");
            bodyDamageAddModel(&body,&render.models[1],nameB.c_str(),0);
            bodyDamageAddModel(&body,&render.models[2],nameC.c_str(),0);
        }
        const auto baseScuffs=carModelName(definition,"_SCUFFS");
        bodyDamageAddModel(&body,&render.models[20],baseScuffs.c_str(),selection.reserved ? 0:1);
    }
    if (render.models[0].solid)std::memcpy(render.origin,render.models[0].solid->origin,3*sizeof(float));
    createCarRenderLightFlares(render,binding.markers);
    vectorBoundsInitialize3(render.minimum,render.maximum);
    float minimum[4]{},maximum[4]{};
    for (const auto slot:{0u,16u,17u}) {
        editorMarkerModelBounds(&render.models[slot],minimum,maximum);
        vectorBoundsExpandBounds3(render.minimum,render.maximum,minimum,maximum);
    }
    for (std::size_t axis=0;axis<3;++axis) {render.minimum[axis]-=0.10000000149011612f;render.maximum[axis]+=0.10000000149011612f;}
    float wheel[4][4]{};
    for (std::size_t index=0;index<4;++index)std::memcpy(wheel[index],definition.wheels[index].position.data(),3*sizeof(float));
    for (std::int32_t index=0;index<4;++index)carPartCullerInitialize(render.cullers.data(),index,wheel[index]);
    float sides[2][4]{},center[4]{},cross[4]{},first[4]{},second[4]{};
    for (std::size_t axis=0;axis<4;++axis) {
        sides[0][axis]=(wheel[0][axis]+wheel[3][axis])*0.5f;
        sides[1][axis]=(wheel[1][axis]+wheel[2][axis])*0.5f;
        center[axis]=(sides[0][axis]+sides[1][axis])*0.5f;
        first[axis]=wheel[0][axis]-wheel[1][axis];second[axis]=wheel[1][axis]-wheel[2][axis];
    }
    carPartCullerInitialize(render.cullers.data(),4,sides[0]);carPartCullerInitialize(render.cullers.data(),5,sides[1]);
    for (std::int32_t index=0;index<4;++index)carPartCullerInitialize(render.cullers.data(),index+6,wheel[index]);
    carPartCullerInitialize(render.cullers.data(),10,wheel[0]);carPartCullerInitialize(render.cullers.data(),11,wheel[2]);
    carPartCullerInitialize(render.cullers.data(),12,center);
    for (std::size_t axis=0;axis<3;++axis)
        cross[axis]=first[(axis+1)%3]*second[(axis+2)%3]-second[(axis+1)%3]*first[(axis+2)%3];
    normalizeCarVector(cross);
    for (std::size_t axis=0;axis<4;++axis)center[axis]+=cross[axis]*definition.wheels[0].radius;
    carPartCullerInitialize(render.cullers.data(),13,center);
}
}

void carInitRenderInfo(Car* car,const hp2::CarRenderBindings* bindings)
{
    hp2::Require(car && car->driverInfo && !car->appearance && bindings && bindings->cars &&
        bindings->resources && bindings->naming && bindings->markers && bindings->particles &&
        bindings->terrain && bindings->applicationState && bindings->characters && bindings->sharedEffectTexture &&
        bindings->carEffects,
        "InitRenderInfo lacks canonical resource or world owners");
    auto* damage=carRenderDamage(car);
    const auto previousCount=damage ? damage->body.modelCount:0;
    const auto* previousInfo=damage ? damage->body.info:nullptr;
    auto appearance=std::make_unique<CarAppearance>();
    car->appearance=appearance.get();
    try {
        carRenderConstruct(car,car->driverInfo->appearance,*appearance,*bindings);
        hp2::Require(carAppearanceBindEffects(car,&appearance->models[0],bindings->markers,bindings->particles,
            bindings->terrain,bindings->effectGroups,bindings->effectGroupCount,bindings->carEffects)==CAR_EFFECT_OK,
            "InitRenderInfo could not bind its actual world effect groups");
        *bindings->sharedEffectTexture=bindings->resources->ResolveTexture(0x2340u,true);
        hp2::Require(*bindings->sharedEffectTexture!=nullptr,"Car effects have no loaded texture/default owner");
    } catch (...) {
        carAppearanceReleaseEffects(car);
        if (appearance->visualOwned) {carDriverVisualDestroy(car->visual);car->visual=nullptr;}
        if (damage) {
            while (damage->body.modelCount>previousCount)
                damage->body.models[--damage->body.modelCount]={};
            damage->body.info=previousInfo;
        }
        car->appearance=nullptr;throw;
    }
    (void)appearance.release();
}

const EditorMarkerModel* carRenderModel(const Car* car,std::size_t slot)
{return car && car->appearance && slot<26 ? &car->appearance->models[slot]:nullptr;}
const CarPartCuller* carRenderPartCullers(const Car* car)
{return car && car->appearance ? car->appearance->cullers.data():nullptr;}
const CarLightFlare* carRenderLightFlares(const Car* car,std::size_t* count)
{
    if (count)*count=car && car->appearance ? car->appearance->flares.size():0;
    return car && car->appearance ? car->appearance->flares.data():nullptr;
}
void carRenderBounds(const Car* car,float minimum[4],float maximum[4])
{
    hp2::Require(car && car->appearance,"Car bounds have no render owner");
    std::memcpy(minimum,car->appearance->minimum,4*sizeof(float));
    std::memcpy(maximum,car->appearance->maximum,4*sizeof(float));
}
std::uint32_t carRenderBrokenLightMask(const Car* car)
{return car && car->appearance ? car->appearance->brokenLightMask:0;}

namespace {
template<class Submit>
std::size_t visitCarLightFlares(CarAppearance& render,Car* car,
    std::int32_t worldFrames,std::uint32_t lightFlags,Submit submit)
{
    // PAL121540..121A44: canonical frame/controls and authored marker keys.
    auto* nativeFrame=car ? car->Frame():nullptr;
    const auto* state=nativeFrame ? &nativeFrame->state:nullptr;
    const auto* movement=nativeFrame ? nativeFrame->movement:nullptr;
    const auto* controls=movement ? movement->bindings.controls:nullptr;
    const bool traffic=car && car->driverInfo && car->driverInfo->driverClass==3;
    const auto* scenario=render.bindings->scenario;
    float rearBase=traffic ? 0.699999988079071f:0.0f;
    float frontBase=!traffic && scenario &&
        (scenario->timeOfDay==2 || scenario->timeOfDay==3 || scenario->timeOfDay==4) ? 0.75f:0.0f;
    if(carRenderForceHeadlightsOn)lightFlags|=1u;
    if(carRenderForceBrakelightsOn)lightFlags|=2u;
    float frontRight=frontBase,frontLeft=frontBase;
    float rearCenter=rearBase,rearRight=rearBase,rearLeft=rearBase,reverseRight=0.0f,reverseLeft=0.0f;
    if((lightFlags&1u) || (state && (traffic ? !state->blinkVisible:state->blinkVisible)))
        frontRight=frontLeft=frontBase+1.0f;
    if(lightFlags&2u)rearCenter=rearRight=rearLeft=rearBase+1.0f;
    else if(controls && controls->brake>0.05000000074505806f)
        rearCenter=rearRight=rearLeft=rearBase+controls->brake*16.0f;
    if((lightFlags&4u) || (movement && movement->bindings.state && movement->bindings.state->gear==0))
        reverseRight=reverseLeft=1.0f;
    const bool police=(lightFlags&8u) || (state && state->signalMode);
    const auto broken=render.brokenLightMask;
    if(broken&1u)frontLeft=0;
    if(broken&2u)frontRight=0;
    if(broken&4u)rearLeft=reverseLeft=0;
    if(broken&8u)rearRight=reverseRight=0;
    if(broken&16u)rearCenter=0;
    constexpr std::uint8_t normalPatterns[3][16]={
        {1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1},
        {0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0}};
    constexpr std::uint8_t alternatePatterns[3][8]={
        {1,1,1,1,0,0,0,0},{0,0,1,1,1,1,0,0},{1,1,1,1,0,0,0,0}};
    const auto phase=render.randomPhase+static_cast<std::uint32_t>(worldFrames);
    std::size_t count=0;
    for(const auto& source:render.flares) {
        float intensity=0;
        switch(source.markerKey) {
        case 0xacec665cu:intensity=frontLeft;break;
        case 0x4ae7f96fu:intensity=frontRight;break;
        case 0x7efcf06fu:intensity=rearLeft;break;
        case 0x34970013u:intensity=rearCenter;break;
        case 0x5f09c5e2u:intensity=rearRight;break;
        case 0xbea9aee6u:intensity=reverseLeft;break;
        case 0x944e5339u:intensity=reverseRight;break;
        case 0xa7e6ea53u:if(police && (phase&15u)<8u && !(broken&1u))intensity=1;break;
        case 0xa532fc46u:if(police && (phase&15u)>=8u && !(broken&2u))intensity=1;break;
        default:
            if(police) {
                const int index=source.markerKey==0x1e4150b4u ? 0:source.markerKey==0xe662c161u ? 1:
                    source.markerKey==0xb4348dbau ? 2:-1;
                if(index>=0)intensity=state && state->signalMode==2 ?
                    alternatePatterns[index][phase&7u]:normalPatterns[index][(phase>>1)&15u];
            }
        }
        if(intensity<=0.0f)continue;
        hp2::LightFlare flare;flare.markerKey=source.markerKey;flare.active=source.active;
        flare.kind=source.kind;flare.flags=source.flags;
        std::copy(source.position,source.position+4,flare.position.begin());
        std::copy(source.normal,source.normal+4,flare.normal.begin());
        count+=submit(flare,std::min(intensity,1.0f));
    }
    return count;
}

std::size_t renderCarLightFlares(CarAppearance& render,Car* car,const CarCinematicScreen& view,
    hp2::EcstasyFrame& frame,const float transform[16],std::int32_t worldFrames,std::uint32_t lightFlags)
{
    return visitCarLightFlares(render,car,worldFrames,lightFlags,[&](const auto& flare,float intensity) {
        return hp2::RenderLightFlare(flare,transform,intensity,*render.textures,view,frame);
    });
}

using CarShadowPoint=std::array<float,4>;
// Collision hulls are validated at <=64 vertices; smoothing emits exactly
// three points per retained hull corner. Keep these short-lived work arrays
// on the native stack instead of four heap allocations per car/view/frame.
template<std::size_t Capacity> struct CarShadowPoints {
    std::array<CarShadowPoint,Capacity> values{};
    std::size_t count{};
    std::size_t size() const noexcept {return count;}
    bool empty() const noexcept {return count==0;}
    const CarShadowPoint* begin() const noexcept {return values.data();}
    const CarShadowPoint* end() const noexcept {return values.data()+count;}
    const CarShadowPoint& operator[](std::size_t index) const noexcept {return values[index];}
    const CarShadowPoint& front() const noexcept {return values[0];}
    const CarShadowPoint& back() const noexcept {return values[count-1];}
    void push_back(const CarShadowPoint& point) {
        if(count>=Capacity)hp2::Require(false,"Native car shadow exceeded its validated hull capacity");
        values[count++]=point;
    }
};

// PAL120D60: the shadow direction uses SunInfo+10, not the separate
// +04 direction vector used by the visual sun/flare pass.
std::array<float,4> carShadowSunDirection(const float position[3],const float* sunPosition)
{
    constexpr float defaultSun[3]={-16384.0f,0.0f,7000.0f};
    const auto* sun=sunPosition ? sunPosition:defaultSun;
    std::array<float,4> direction{};
    for(std::size_t axis=0;axis<3;++axis)direction[axis]=position[axis]-sun[axis];
    const float horizontal=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]);
    if(vectorAtan2Angle(horizontal,-direction[2])<0x11f8u) {
        float sine=0.0f,cosine=0.0f;vectorSineCosineAngle(&sine,&cosine,0x11f8u);
        const float length=std::sqrt((direction[0]*direction[0]+direction[1]*direction[1])+
            direction[2]*direction[2]);
        direction[2]=-length*sine;
        const float divisor=std::fabs(direction[0])+std::fabs(direction[1]);
        direction[0]=((direction[0]/divisor)*length)*cosine;
        direction[1]=((direction[1]/divisor)*length)*cosine;
    }
    direction[3]=direction[2]!=0.0f ? 1.0f/direction[2]:0.0f;
    return direction;
}

CarShadowPoint carShadowProjectVertex(const float vertex[3],const float transform[16],
    float carHeight,const std::array<float,4>& sun)
{
    // PAL121EA8..121F44, including the original XY scale and local Z lift.
    const float local[3]={vertex[0]*0.8500000238418579f,vertex[1]*0.8500000238418579f,
        vertex[2]+0.2750000059604645f};
    CarShadowPoint point{};eMulVector3(point.data(),transform,local);
    const float amount=-(point[2]-carHeight)*sun[3];
    point[0]+=amount*sun[0];point[1]+=amount*sun[1];
    return point;
}

int carShadowCompareLower(const void* first,const void* second)
{
    const auto& a=**static_cast<const CarShadowPoint* const*>(first);
    const auto& b=**static_cast<const CarShadowPoint* const*>(second);
    float difference=a[0]-b[0];
    if(difference>0.0f)return 1;
    if(difference<0.0f)return -1;
    difference=b[1]-a[1];
    return difference>0.0f ? 1:difference<0.0f ? -1:0;
}
int carShadowCompareUpper(const void* first,const void* second)
{
    return carShadowCompareLower(second,first);
}
std::size_t carShadowMakeChain(const CarShadowPoint** points,std::size_t count,
    int (*compare)(const void*,const void*))
{
    // PAL120850: the source swaps unused pointers rather than erasing
    // them; the second chain consumes that exact retained permutation.
    std::qsort(points,count,sizeof(*points),compare);
    std::size_t last=1;
    for(std::size_t next=2;next<count;++next) {
        std::size_t current=last;
        while(current>0) {
            const auto& a=*points[next];const auto& b=*points[current];const auto& c=*points[current-1];
            const float cross=(a[0]-b[0])*(c[1]-b[1])-(a[1]-b[1])*(c[0]-b[0]);
            if(cross>0.0f)break;
            --current;
        }
        last=current+1;std::swap(points[last],points[next]);
    }
    return last;
}
CarShadowPoints<64> carShadowHull(const CarShadowPoints<64>& points)
{
    hp2::Require(points.size()>=2 && points.size()<=64,"Car shadow hull exceeds its original64-vertex input");
    std::array<const CarShadowPoint*,65> pointers{};
    for(std::size_t i=0;i<points.size();++i)pointers[i]=&points[i];
    const auto lower=carShadowMakeChain(pointers.data(),points.size(),carShadowCompareLower);
    pointers[points.size()]=pointers[0];
    const auto count=lower+carShadowMakeChain(pointers.data()+lower,points.size()-lower+1,carShadowCompareUpper);
    CarShadowPoints<64> result;
    for(std::size_t i=0;i<count;++i)result.push_back(*pointers[i]);
    return result;
}
CarShadowPoints<192> carShadowSmooth(const CarShadowPoints<64>& hull)
{
    // PAL120C00 emits three points per retained corner, using0.333 rather
    // than an exact one-third. All work storage is local to this render call.
    CarShadowPoints<192> result;
    if(hull.empty())return result;
    constexpr float third=0.3330000042915344f;
    CarShadowPoint previous{};
    for(std::size_t axis=0;axis<4;++axis)previous[axis]=(hull.front()[axis]-hull.back()[axis])*third;
    for(std::size_t i=0;i<hull.size();++i) {
        const auto& current=hull[i];const auto& next=hull[(i+1)%hull.size()];
        CarShadowPoint delta{},before{},middle{},after{};
        for(std::size_t axis=0;axis<4;++axis) {
            delta[axis]=(next[axis]-current[axis])*third;
            before[axis]=current[axis]-previous[axis];
            middle[axis]=(delta[axis]-previous[axis])*third+current[axis];
            after[axis]=current[axis]+delta[axis];
        }
        result.push_back(before);result.push_back(middle);result.push_back(after);previous=delta;
    }
    return result;
}

template<class Emit>
std::size_t renderCarSunShadow(CarAppearance& render,Car* car,const CarCinematicScreen& view,
    Emit emit,const float transform[16],const hp2::SceneLightContext& lighting,
    bool frontend,std::size_t lod,std::int32_t pixels)
{
    if(lod>=3 || frontend || !car || !render.models[0].solid || !render.auxiliaryTexture || view.id==3)return 0;
    auto* movement=car->Movement();
    const auto* collision=movement && movement->bindings.ownerMotion ? movement->bindings.ownerMotion->collision:nullptr;
    if(!collision || !collision->shape)return 0;
    const auto* geometry=collision->shape->geometry;
    auto* terrain=movement->bindings.environment ? movement->bindings.environment->contact.terrain:nullptr;
    hp2::Require(geometry && geometry->vertices && geometry->vertexCount>=2 && terrain && render.textures,
        "Car sun shadow lost its collision hull, terrain or resource owner");
    const auto* texture=hp2::textureInfoForDescriptor(render.auxiliaryTexture);
    hp2::Require(texture,"Car sun shadow has no authored gradient texture");
    const auto& sunInfo=render.textures->Sun();float authoredSun[3]{};const float* sunPosition=nullptr;
    if(sunInfo.Info()) {
        hp2::Require(sunInfo.InfoSize()>=28,"Car sun shadow position is outside its authored SunInfo");
        for(std::size_t axis=0;axis<3;++axis) {
            const auto word=hp2::ReadU32(sunInfo.Info()+16+axis*4);std::memcpy(&authoredSun[axis],&word,4);
        }
        sunPosition=authoredSun;
    }
    const auto sun=carShadowSunDirection(transform+12,sunPosition);
    hp2::Require(geometry->vertexCount<=64,"Car shadow hull exceeds its validated native input capacity");
    CarShadowPoint center{{transform[12],transform[13],transform[14],0.0f}},normal{};
    center[2]=terrainSurfaceElevation(terrain,center.data(),nullptr,normal.data(),nullptr);
    CarShadowPoints<64> projected;
    for(std::int32_t i=0;i<geometry->vertexCount;++i)
        projected.push_back(carShadowProjectVertex(geometry->vertices[i].vector,transform,transform[14],sun));
    auto hull=carShadowHull(projected);
    constexpr float lift=0.05999999865889549f;
    const float referenceHeight=center[2]+lift;
    CarShadowPoints<64> grounded;
    for(auto point:hull) {
        point[2]=referenceHeight;bool found=false;
        point[2]=terrainSurfaceElevation(terrain,point.data(),nullptr,nullptr,&found)+lift;
        const float separation=((point[0]-center[0])*normal[0]+(point[1]-center[1])*normal[1])+
            (point[2]-center[2])*normal[2];
        if(found && std::fabs(point[2]-referenceHeight)<=4.0f && std::fabs(separation)<1.5f) {
            grounded.push_back(point);
        }
    }
    const auto smooth=carShadowSmooth(grounded);const auto count=smooth.size();
    if(count<3)return 0;
    for(std::size_t axis=0;axis<4;++axis)center[axis]=(smooth[0][axis]+smooth[count/2][axis])*0.5f;
    float alpha=90.0f,feather=1.100000023841858f;
    const float intensity=lighting.sunlightIntensity;
    if(intensity<0.4000000059604645f) {alpha=0.0f;feather=1.399999976158142f;}
    else if(intensity<=1.0f) {
        const float amount=(intensity-0.4000000059604645f)/0.6000000238418579f;
        feather=amount*-0.2999999523162842f+1.399999976158142f;alpha*=amount;
    }
    if(lod==2)alpha*=std::clamp((static_cast<float>(pixels)-20.0f)/(45.0f-20.0f),0.0f,1.0f);
    const auto alphaByte=std::min(convertFloatToInt32(alpha),255);
    const auto color=0x00808080u|(static_cast<std::uint32_t>(alphaByte)<<24);
    const auto vertex=[&](const CarShadowPoint& point,float u) {
        hp2::FlailerPolyVertex v;std::copy(point.begin(),point.begin()+3,v.position.begin());
        v.textureCoordinate={u,0.0f,1.0f};v.color=color;return v;
    };
    constexpr float edgeU=0.1550000011920929f;std::size_t submitted=0;
    for(std::size_t i=0;i+1<(count&~std::size_t{1});i+=2) {
        emit({vertex(smooth[i],edgeU),vertex(center,0.0f),
            vertex(smooth[i+1],edgeU),vertex(smooth[(i+2)%count],edgeU)},*texture);++submitted;
    }
    if(count&1u) {
        emit({vertex(smooth.back(),edgeU),vertex(center,0.0f),vertex(smooth.front(),edgeU)},*texture);++submitted;
    }
    const auto third=count/3;
    std::vector<hp2::FlailerPolyVertex> strip;strip.reserve((third+3)*2);
    for(std::size_t group=0;group<3;++group) {
        const auto begin=group*third,end=group==2 ? count:begin+third;
        strip.clear();
        for(std::size_t i=begin;i<=end;++i) {
            const auto& point=smooth[i%count];auto outer=point;
            outer[0]=feather*(point[0]-center[0])+center[0];outer[1]=feather*(point[1]-center[1])+center[1];
            strip.push_back(vertex(point,edgeU));strip.push_back(vertex(outer,1.0f));
        }
        emit(strip,*texture);++submitted;
    }
    return submitted;
}

template<class Emit>
std::size_t renderCarRoadShadow(CarAppearance& render,Car* car,Emit emit,const float transform[16],
    const hp2::SceneLightContext* lighting,bool frontend,bool groundVisible)
{
    // Released independent regular branch122454..122B88. The sunlight
    // shadow is submitted first by the original121D70..122454 branch.
    if(!render.models[0].solid || !render.shadowTexture)return 0;
    auto* nativeFrame=car ? car->Frame():nullptr;
    if(nativeFrame && nativeFrame->movement && nativeFrame->movement->bindings.state &&
        nativeFrame->movement->bindings.state->activeContactCount==4 && !groundVisible)return 0;
    const auto* texture=hp2::textureInfoForDescriptor(render.shadowTexture);
    hp2::Require(texture && render.definition,"Car shadow lost its real texture/definition owner");
    auto* terrain=nativeFrame && nativeFrame->movement && nativeFrame->movement->bindings.environment ?
        nativeFrame->movement->bindings.environment->contact.terrain:nullptr;
    constexpr std::size_t grid=4;
    const std::size_t pointCount=grid*grid;
    constexpr float step=0.3333333432674408f;
    const auto central=[&](std::size_t index) {
        const auto row=index/grid,column=index%grid;
        return row>0 && row+1<grid && column>0 && column+1<grid;
    };
    const float sx=frontend ? 1.1150000095367432f:0.949999988079071f;
    const float sy=frontend ? 1.0f:0.949999988079071f;
    const float minX=render.minimum[0]*sx,maxX=render.maximum[0]*sx;
    const float minY=render.minimum[1]*sy,maxY=render.maximum[1]*sy;
    std::array<std::array<float,4>,16> points{};
    std::array<std::array<float,2>,16> uv{};
    float y=minY,v=0.0f;
    for(std::size_t row=0;row<grid;++row) {
        float x=minX,u=0.0f;
        for(std::size_t column=0;column<grid;++column) {
            const auto index=row*grid+column;const float local[3]={x,y,0.0f};
            eMulVector3(points[index].data(),transform,local);points[index][2]=transform[14];
            uv[index]={u,v};x+=(maxX-minX)*step;u+=step;
        }
        y+=(maxY-minY)*step;v+=step;
    }
    std::array<std::size_t,4> invalidRows{};
    if(car && terrain && !frontend) {
        float center[4]={transform[12],transform[13],transform[14],1.0f},centerNormal[4]{};
        center[2]=terrainSurfaceElevation(terrain,center,nullptr,centerNormal,nullptr);
        std::array<std::array<float,4>,4> wheelPositions{},wheelPlanes{};
        std::array<bool,4> wheelGround{};
        for(std::size_t wheel=0;wheel<4;++wheel) {
            eMulVector3(wheelPositions[wheel].data(),transform,render.definition->wheels[wheel].position.data());
            wheelPositions[wheel][2]=terrainSurfaceElevation(terrain,wheelPositions[wheel].data(),
                nullptr,wheelPlanes[wheel].data(),&wheelGround[wheel]);
            const float factor=-1.0f/wheelPlanes[wheel][2];
            for(auto& lane:wheelPlanes[wheel])lane*=factor;
        }
        // The authored four quadrants select the nearest wheel for the 4x4 grid.
        float previousHeight=transform[14];
        for(std::size_t i=0;i<pointCount;++i) {
            auto& point=points[i];bool found=false;
            point[2]=terrainSurfaceElevation(terrain,point.data(),nullptr,nullptr,&found);
            const std::size_t wheel=i/grid<grid/2 ? (i%grid<grid/2 ? 2u:1u) : (i%grid<grid/2 ? 3u:0u);
            if(!found && wheelGround[wheel]) {
                point[2]=wheelPositions[wheel][2]+(point[0]-wheelPositions[wheel][0])*wheelPlanes[wheel][0];
                point[2]+=(point[1]-wheelPositions[wheel][1])*wheelPlanes[wheel][1];found=true;
            }
            if(central(i))point[2]+=0.07999999821186066f;
            float separation=0.0f;
            for(std::size_t axis=0;axis<3;++axis)separation+=(point[axis]-center[axis])*centerNormal[axis];
            if(found && std::fabs(point[2]-center[2])<=4.0f && std::fabs(separation)<1.5f) {
                previousHeight=point[2];point[2]+=0.03999999910593033f;
            } else {point[2]=previousHeight+0.03999999910593033f;++invalidRows[i/grid];}
        }
    } else {
        // Released32FC34 floor height and32DAEC central lift.
        for(std::size_t i=0;i<pointCount;++i) {
            points[i][2]=0.009999999776482582f;
            if(central(i))points[i][2]+=0.07999999821186066f;
        }
    }
    const float sun=lighting ? lighting->sunlightIntensity:0.0f;
    const float alpha=frontend ? 92.0f:sun<0.4000000059604645f ? 70.0f:
        sun>1.0f ? 140.0f:70.0f+70.0f*((sun-0.4000000059604645f)/0.6000000238418579f);
    const auto alphaByte=std::min(static_cast<std::uint32_t>(convertFloatToInt32(alpha)),255u);
    const auto color=0x00808080u|(alphaByte<<24);
    std::size_t submitted=0;
    std::vector<hp2::FlailerPolyVertex> strip;strip.reserve(grid*2);
    for(std::size_t row=0;row+1<grid;++row) {
        if(invalidRows[row]+invalidRows[row+1]>=grid)continue;
        strip.clear();
        for(std::size_t column=0;column<grid;++column)for(std::size_t side=0;side<2;++side) {
            const auto index=(row+side)*grid+column;hp2::FlailerPolyVertex vertex;
            std::copy(points[index].begin(),points[index].begin()+3,vertex.position.begin());
            vertex.textureCoordinate={uv[index][0],uv[index][1],1.0f};vertex.color=color;strip.push_back(vertex);
        }
        emit(strip,*texture);++submitted;
    }
    return submitted;
}


std::size_t renderCarAppearanceScene(CarAppearance& render,Car* car,
    const CarCinematicScreen& view,hp2::EcstasyFrame& frame,
    const float position[3],const float orientation[16],const float wheels[4][16],
    const float brakes[4][16],std::int32_t worldFrames,std::int32_t presentationLayout,
    std::int32_t forcedLod,std::uint32_t sourceFlags,std::uint32_t lightFlags)
{
    // PAL120EF8..12356C geometry/driver branches. Car projected shadows and
    // light-volume modifiers remain separate source dependencies.
    hp2::Require(view.view && view.matricesCalculated,
        "Car render requires the canonical appearance and calculated view");
    auto pixels=ecstasyViewSourcePositionPixelSize(&view,position,3.0f);
    if (presentationLayout==3)pixels=convertFloatToInt32(static_cast<float>(pixels)*0.699999988079071f);
    auto visibilityPixels=pixels;
    const float distanceScale=hp2::nativeViewDistanceScale(view.id);
    if(distanceScale!=1.0f) {
        visibilityPixels=ecstasyViewNativePositionPixelSize(&view,position,3.0f,1);
        if(presentationLayout==3)visibilityPixels=convertFloatToInt32(static_cast<float>(visibilityPixels)*0.699999988079071f);
    }
    if (visibilityPixels<4)return 0;
    // PAL multiplies by the fixed identity matrix initialized by
    // InitCarEffects. It has no per-car state and frontend rendering does
    // not instantiate a World merely to access the same identity constant.
    // PAL121120..12114C multiplies the source orientation by this exact
    // shared matrix before replacing its translation. Modes1/11 change the
    // three diagonal cells; collision/vehicle dimensions remain untouched.
    float transform[16];eMulMatrix(transform,CarScaleMatrix,orientation);
    for(std::size_t axis=0;axis<3;++axis)transform[12+axis]=position[axis];
    transform[15]=1.0f;
    float toClip[16];eMulMatrix(toClip,transform,view.matrices.worldToClip);
    if (!ecstasyClassifyBounds(render.minimum,render.maximum,toClip))return 0;
    auto* nativeFrame=car ? car->Frame():nullptr;
    hp2::Require(!car || (nativeFrame && nativeFrame->movement &&
        nativeFrame->movement->bindings.owner==car && nativeFrame->movement->bindings.state &&
        nativeFrame->movement->bindings.controls && car->driverInfo),
        "Car light-state render lacks its actual frame/movement/selection owners");
    updateCarRenderSceneTextures(render,car,nativeFrame ? &nativeFrame->state:nullptr,
        car ? car->driverInfo->driverClass:0,
        nativeFrame ? nativeFrame->movement->bindings.controls:nullptr,render.bindings->scenario,worldFrames);
    // Explicit PC-quality presentation adaptation. Retain the PAL pixel
    // radius for culling, flares, damage, driver and lighting decisions;
    // apply the native distance preference to body/wheel mesh LOD only.
    std::size_t lod=carRenderNativeDetailLod(render.models.data(),pixels);
    std::size_t wheelLod=lod;
    // PAL121338..121368: Car+3BC is CarState+37C, the live ground-contact
    // count. Appearance+8F8 is culler12's +18 predicate (760+12*20+18).
    // The released selector reads the PRECEDING CullParts result; its next
    // cull occurs later at121538. Only the body changes from D to C, while
    // the wheel selector keeps its original detail and is clamped below.
    if(lod>=3 && car && nativeFrame->movement->bindings.state->activeContactCount==0 &&
        render.cullers[12].culled)lod=2;
    // PAL121370..121394 applies separate overrides after that body repair.
    // The frontend's two authored {0,1} tables supply the same forcedLod;
    // passing their values directly avoids temporary process-global writes.
    const auto bodyOverride=forcedLod>=0 ? forcedLod:ForceCarLOD;
    const auto wheelOverride=forcedLod>=0 ? forcedLod:ForceTireLOD;
    if(forcedLod>=0) {
        hp2::Require(forcedLod<4,"Frontend forced LOD is outside the final table");
    }
    hp2::Require(bodyOverride>=-1 && bodyOverride<4 && wheelOverride>=-1,
        "Car/tire LOD override is outside its authored mesh domain");
    if(bodyOverride!=-1)lod=static_cast<std::size_t>(bodyOverride);
    if(wheelOverride!=-1)wheelLod=static_cast<std::size_t>(wheelOverride);
    float inverse[16],localEye[4];vectorMatrixInverseAffine(inverse,transform);
    eMulVector3(localEye,inverse,view.view->current.eye);
    carPartCullerCull(render.cullers.data(),localEye);
    // This is the source raw +18 predicate. Its historical native name
    // 'culled' is retained to avoid changing already-audited owner layout.
    const auto part=[&](std::size_t index){return render.cullers[index].culled!=0;};
    float drawTransform[16];std::memcpy(drawTransform,transform,sizeof(drawTransform));
    drawTransform[14]+=0.009999999776482582f;
    float distanceSquared=0.0f;
    for(std::size_t axis=0;axis<3;++axis) {
        const float d=view.view->current.eye[axis]-position[axis];distanceSquared+=d*d;
    }
    const float distance=std::sqrt(distanceSquared);
    hp2::Require(distance>0.0f,"Original Car depth bias is undefined at its camera origin");
    ecstasyViewBiasMatrixForZSorting(&view,drawTransform,distance/distance);
    const bool frontend=render.bindings && render.bindings->applicationState &&
        *render.bindings->applicationState==3;
    hp2::Require(render.textures,"Car lighting has no canonical light registry");
    // PAL1211EC uses the unbiased model matrix and eEnvMap's one body
    // matrix2D70F0. The same elSetupLightContext owns frontend presets.
    const auto applicationState=render.bindings && render.bindings->applicationState ?
        *render.bindings->applicationState:0;
    hp2::LightContextBatch lightContexts(render.textures->Lights());
    const auto bodyLighting=lightContexts.Setup(transform,
        frame.EnvironmentMatrix(),view.view->current.eye,view,applicationState);
    const std::uint32_t environmentFlags=
        hp2::environmentMappingEnabled && pixels>=18 ? 0u:1u;
    const auto draw=[&](std::size_t slot,const float* matrix,std::uint32_t flags=0u,float alpha=1.0f) {
        std::optional<hp2::SceneLightContext> partLighting;
        const hp2::SceneLightContext* context=nullptr;
        if(slot<4 || (slot>=16 && slot<=19))context=&bodyLighting;
        else if(slot>=4 && slot<=13) {
            // PAL122EE8..1233EC: each wheel/brake owns its final matrix.
            partLighting=lightContexts.Setup(matrix,
                view.matrices.worldToView,view.view->current.eye,view,applicationState);
            context=&*partLighting;
        }
        return frame.RenderModel(render.models[slot],matrix,view,flags|sourceFlags,alpha,worldFrames,context);
    };
    std::size_t submitted=renderCarLightFlares(render,car,view,frame,transform,worldFrames,lightFlags);
    // PAL121A84..121D10: use the same appearance/character owner, original
    // pixel thresholds, passenger-seat offset and front-facing detail rule.
    if(car && car->driverVisible && car->visual && car->driverInfo && view.id!=3 && !car->visual->replaySuppressed) {
        auto& driver=*car->visual;
        hp2::Require(driver.bindings.vehicle && render.definition && render.textures,
            "Car driver rendering lost its authored appearance owners");
        const float minimum=driver.bindings.vehicle->driverAnimationCode ? 50.0f:
            (presentationLayout==3 ? 100.0f:75.0f);
        if(carRenderDriverVisibilityOverride || static_cast<float>(pixels)>minimum) {
            float driverTransform[16],rotation[16],offset[4];
            std::memcpy(driverTransform,drawTransform,sizeof(driverTransform));
            std::memcpy(rotation,drawTransform,sizeof(rotation));
            rotation[12]=rotation[13]=rotation[14]=0.0f;
            eMulVector3(offset,rotation,render.definition->driverAnimationOffset.data());
            for(std::size_t axis=0;axis<3;++axis)driverTransform[12+axis]+=offset[axis];
            driverTransform[15]=1.0f;
            float front[4],toEye[4];const float localFront[4]={-1.0f,0.0f,0.0f,0.0f};
            eMulVector4(front,rotation,localFront);
            for(std::size_t axis=0;axis<3;++axis)toEye[axis]=(view.view->current.eye[axis]-position[axis])/distance;
            const float facing=front[0]*toEye[0]+front[1]*toEye[1]+front[2]*toEye[2];
            const bool active=carDriverVisualMaximumRemaining(&driver)!=0;
            std::int32_t detail=2;
            if(facing>0.8999999761581421f) {
                if(pixels>999 || (pixels>300 && active))detail=1;
            } else if(pixels>300 || (pixels>150 && active))detail=1;
            auto lighting=bodyLighting;
            if(frontend)lighting=lightContexts.SetupRace(transform,
                view.matrices.worldToView,view.view->current.eye);
            else hp2::SetLightContextEnvironment(lighting,transform,view.matrices.worldToView);
            submitted+=hp2::RenderDriverVisual(driver,driverTransform,&lighting,view,frame,0,detail);
        }
    }
    const auto shadowLod=distanceScale>1.0f ?
        std::min(lod,carRenderNativeDetailLod(render.models.data(),visibilityPixels)):lod;
    const auto emitShadow=[&](const std::vector<hp2::FlailerPolyVertex>& strip,const hp2::TextureInfo& texture) {
        frame.RenderWorldStrip(strip,texture,eMathIdentityMatrix,view);
    };
    submitted+=renderCarSunShadow(render,car,view,emitShadow,transform,bodyLighting,frontend,shadowLod,visibilityPixels);
    submitted+=renderCarRoadShadow(render,car,emitShadow,transform,&bodyLighting,frontend,part(13));
    submitted+=draw(lod,drawTransform,environmentFlags);
    const auto* damage=carRenderDamage(car);
    const auto intact=[&](std::uint32_t item) {return !damage || carDamageItemLevel(damage->levels,item)<2;};
    if(lod<2)for(std::size_t side=0;side<2;++side)if(intact(static_cast<std::uint32_t>(side)))
        submitted+=draw(16+side,drawTransform,environmentFlags);
    if(lod<3)for(std::size_t side=0;side<2;++side)
        if(part(10+side) && intact(static_cast<std::uint32_t>(2+side)))submitted+=draw(18+side,drawTransform,environmentFlags);
    if(lod<2 && damage && bodyDamageVisibleTotal(&damage->body)>0.019999999552965164f)
        submitted+=draw(20,drawTransform);

    // Body D has no wheel pass. Front/rear A/B/C meshes and the actual
    // suspension rotation/translation are selected by the original loops.
    if(lod<3) {
        wheelLod=std::min(wheelLod,std::size_t{2});
        float flipped[16];eIdentity(flipped);motionMatrixRotateYaw(flipped,flipped,0x8000u);
        std::array<std::array<float,16>,4> wheelWorld{};
        std::array<hp2::SceneLightContext,4> wheelLighting{};
        for(std::size_t wheel=0;wheel<4;++wheel)if(frontend || part(wheel)) {
            float local[16];auto& world=wheelWorld[wheel];
            if(wheel==0 || wheel==3)eMulMatrix(local,flipped,wheels[wheel]);
            else std::memcpy(local,wheels[wheel],sizeof(local));
            eMulMatrix(world.data(),local,drawTransform);
            wheelLighting[wheel]=lightContexts.Setup(world.data(),view.matrices.worldToView,
                view.view->current.eye,view,applicationState);
            submitted+=frame.RenderModel(render.models[4+wheelLod*2+(wheel>=2 ? 1:0)],world.data(),
                view,environmentFlags|sourceFlags,1.0f,worldFrames,&wheelLighting[wheel]);
        }
        // PAL123024..1231B0: blur is an additional authored wheel model,
        // not a substitute mesh. Reuse each wheel's calculated matrix/light
        // context and preserve the original left-side, right-side order.
        if(wheelLod<2 && !frontend && car) {
            const auto* movement=car->Movement();
            hp2::Require(movement && movement->bindings.state,"Wheel blur lost its physical wheel state");
            constexpr std::size_t order[4]={0,3,1,2};
            for(const auto wheel:order)if(part(wheel) && part((wheel==0 || wheel==3) ? 4:5)) {
                const float amount=std::clamp(std::fabs(movement->bindings.state->wheelAngularVelocity[wheel])*
                    0.019999999552965164f,0.0f,1.0f)*90.0f;
                if(amount<=0.10000000149011612f)continue;
                auto context=wheelLighting[wheel];context.colorMatrix[15]=amount;
                submitted+=frame.RenderModel(render.models[wheel<2 ? 10:11],wheelWorld[wheel].data(),
                    view,sourceFlags|1u,1.0f,worldFrames,&context);
            }
        }
        if(lod<2) {
            float reversed[16];eIdentity(reversed);reversed[5]=-1.0f;
            for(std::size_t wheel=0;wheel<4;++wheel)if(frontend || part(wheel+6)) {
                float local[16],world[16];
                if(wheel==0 || wheel==3)eMulMatrix(local,reversed,brakes[wheel]);
                else std::memcpy(local,brakes[wheel],sizeof(local));
                eMulMatrix(world,local,drawTransform);
                submitted+=draw(wheel<2 ? 12:13,world,environmentFlags);
            }
        }
    }
    // Source wiper animation uses the same frame timer window and encoded
    // rigid animation start; RenderModel evaluates its real animation tree.
    if(pixels>40 && car)if(auto* carFrame=car->Frame()) {
        const auto& state=carFrame->state;
        const bool active=state.signalWindowLocked ||
            (worldFrames>=state.signalWindowStart && worldFrames<state.signalWindowEnd);
        if(active)for(std::size_t slot=14;slot<16;++slot) {
            auto& model=render.models[slot];
            if(!editorMarkerModelRigidAnimation(&model))continue;
            model.rigidAnimationStart=static_cast<std::uint16_t>((state.signalWindowStart&0x3fffu)|0xc000u);
            // PAL1234DC..123504: authored eSolid+60 pivot precedes the
            // rigid frame matrix. The source wiper has no light context.
            float wiperTransform[16];
            vectorMatrixMultiply(wiperTransform,drawTransform,model.solid->positionMatrix);
            submitted+=draw(slot,wiperTransform);
        }
    }
    return submitted;
}
}

std::size_t carRenderScene(Car& car,const CarCinematicScreen& view,hp2::EcstasyFrame& frame,
    const float position[3],const float orientation[16],const float wheels[4][16],
    const float brakes[4][16],std::int32_t worldFrames,std::int32_t presentationLayout)
{
    hp2::Require(car.appearance,"Car rendering has no canonical appearance owner");
    return renderCarAppearanceScene(*car.appearance,&car,view,frame,position,orientation,
        wheels,brakes,worldFrames,presentationLayout,-1,0,0);
}

std::size_t RenderDashboards(const CarCinematicScreen& view,hp2::EcstasyFrame& frame,
    std::int32_t worldFrames)
{
    // PAL1242A8..1244A4 admits the attached kind1 camera through exactly
    // these two published camera cells. The ordinary car pass hides it.
    if(!view.view || !view.view->hiddenCar || !view.view->visibilityFlags)return 0;
    const auto* car=view.view->hiddenCar;
    auto* render=car->appearance;
    if(!render || !render->models[21].solid)return 0;
    const auto* movement=static_cast<const CarMovement*>(car->movement);
    hp2::Require(movement && movement->bindings.state && movement->bindings.controls &&
        render->definition && render->textures && view.matricesCalculated,
        "Dashboard lost its canonical car, controls, resource or camera owner");
    const auto& state=*movement->bindings.state;
    const auto& type=*render->definition;
    const std::size_t row=type.modelCode==0x17 ? 0:type.modelCode==0x19 ? 1:2;
    // Final PAL2AD088..2AD0E8, read as six independent arrays. They are
    // presentation offsets/scales and do not alter the physical car pose.
    constexpr float offset[3][3]={
        {-0.25581398606300354f,-0.10769200325012207f,-0.30799999833106995f},
        {-0.25581398606300354f,-0.15897400677204132f,-0.40697601437568665f},
        {-0.2512820065021515f,-0.17948800325393677f,-0.3230760097503662f}};
    constexpr float scale[3][3]={
        {0.8999999761581421f,1.7034879922866821f,1.697674036026001f},
        {0.8999999761581421f,2.0f,2.0f},
        {0.8999999761581421f,1.7034879922866821f,1.697674036026001f}};
    std::size_t materialCount{};const auto* materials=terrainDefaultMaterials(&materialCount);
    hp2::Require(materialCount>2 && state.position,"Dashboard road-noise inputs are unavailable");
    float point[4]={state.position[0],state.position[1],state.position[2],1.0f};
    float displacement[4]={offset[row][0],offset[row][1],offset[row][2],1.0f};
    displacement[0]+=terrainMaterialDisplacement(&materials[2],point)*0.04800000041723251f;
    point[1]-=1.0f;
    displacement[1]+=terrainMaterialDisplacement(&materials[2],point)*0.04800000041723251f;
    point[1]+=2.0f;
    displacement[2]+=terrainMaterialDisplacement(&materials[2],point)*0.04800000041723251f;
    float transform[16];eIdentity(transform);
    transform[0]=scale[row][0];transform[5]=scale[row][1];transform[10]=scale[row][2];
    eMulMatrix(transform,transform,state.orientation);
    float position[4];eMulVector3(position,transform,displacement);
    for(std::size_t axis=0;axis<3;++axis)transform[12+axis]=state.centerPosition[axis]+position[axis];
    transform[15]=1.0f;
    const auto applicationState=render->bindings && render->bindings->applicationState ?
        *render->bindings->applicationState:0;
    const auto lighting=hp2::SetupLightContext(render->textures->Lights(),transform,
        view.matrices.worldToView,view.view->current.eye,view,applicationState);
    struct ProjectionScope {
        hp2::EcstasyFrame& frame;bool previous;
        ~ProjectionScope(){frame.SetDashboardProjection(previous);}
    } scope{frame,frame.SetDashboardProjection(true)};
    std::size_t submitted=frame.RenderModel(render->models[21],transform,view,0,1.0f,worldFrames,&lighting);
    if(render->models[23].solid) {
        float local[16],world[16];eIdentity(local);
        // eRotateX11B888, not the eRotateZ11BA18 wheel-yaw operation.
        motionMatrixRotateX(local,local,static_cast<std::uint16_t>(0u-movement->bindings.controls->steeringAngle));
        eMulMatrix(local,local,render->models[23].solid->positionMatrix);
        vectorMatrixMultiply(world,transform,local);
        submitted+=frame.RenderModel(render->models[23],world,view,0,1.0f,worldFrames,&lighting);
    }
    const auto needle=[&](std::size_t slot,float normalized,std::uint16_t start,float span) {
        auto& model=render->models[slot];if(!model.solid)return std::size_t{0};
        hp2::Require(std::isfinite(normalized),"Dashboard gauge has a nonfinite authored range");
        const auto angle=static_cast<std::uint16_t>(static_cast<std::uint32_t>(start)+
            static_cast<std::uint32_t>(convertFloatToInt32(std::clamp(normalized,0.0f,1.0f)*span)));
        float local[16],world[16];eIdentity(local);motionMatrixRotateX(local,local,angle);
        std::copy_n(model.solid->origin,3,local+12);
        vectorMatrixMultiply(world,transform,local);
        return frame.RenderModel(model,world,view,0,1.0f,worldFrames);
    };
    const auto& range=type.gauges.speedometerRange[0];
    hp2::Require(range[1]>range[0] && type.engineInfo.maximumRpm>0.0f,
        "Dashboard gauge range is not positive");
    submitted+=needle(24,(state.speed*2.236999988555908f-range[0])/(range[1]-range[0]),0x7df8u,49000.0f);
    const std::uint16_t tachStart=type.modelCode==0x19 ? 0x7530u:type.modelCode==0x15 ? 0x733cu:0x7d00u;
    const float tachSpan=type.modelCode==0x19 ? 51200.0f:type.modelCode==0x15 ? 53500.0f:41900.0f;
    submitted+=needle(25,state.engineRpm/type.engineInfo.maximumRpm,tachStart,tachSpan);
    submitted+=frame.RenderModel(render->models[22],transform,view,0,1.0f,worldFrames);
    return submitted;
}

namespace hp2 {
void CollectCarLightEffects(Car& car,const float transform[16],std::int32_t worldFrames,
    std::vector<RaceLightEffect>& output)
{
    if (!car.appearance) return;
    auto& appearance = *car.appearance;
    visitCarLightFlares(appearance,&car,worldFrames,0,[&](const auto& flare,float intensity) {
        CollectLightEffect(flare,transform,intensity,*appearance.textures,output);
        return 0u;
    });
}

struct FrontEndRenderingCar::Storage {
    CarRenderBindings bindings;
    std::unique_ptr<CarAppearance> appearance;
    FrontEndRenderingCar* previous{};
    FrontEndRenderingCar* next{};
    static FrontEndRenderingCar* first;
    static FrontEndRenderingCar* last;
    explicit Storage(const CarRenderBindings& supplied):bindings(supplied){}
};
FrontEndRenderingCar* FrontEndRenderingCar::Storage::first{};
FrontEndRenderingCar* FrontEndRenderingCar::Storage::last{};

FrontEndRenderingCar::FrontEndRenderingCar(const CarRenderBindings& bindings,
    const CarAppearanceSelection* selection,std::int32_t view)
    :viewNumber(view),storage_(std::make_unique<Storage>(bindings))
{
    Require(bindings.cars && bindings.resources && bindings.naming && bindings.markers &&
        bindings.applicationState,"Frontend car lacks its canonical resource owners");
    Require(view>=0 && view<10,"Frontend car view is outside the original view bank");
    ride.modelCode=-1;
    ReInit(selection);
    eIdentity(orientation);
    for(std::size_t i=0;i<4;++i) {eIdentity(wheels[i]);eIdentity(brakes[i]);}
    storage_->previous=Storage::last;
    if(Storage::last)Storage::last->storage_->next=this;else Storage::first=this;
    Storage::last=this;
}
FrontEndRenderingCar::~FrontEndRenderingCar()
{
    // The native presenter is synchronous: a returned SubmitFrame has
    // waited for its real fence. Source eWaitUntilRenderingDone therefore
    // precedes this lifetime transition through the application frame owner.
    storage_->appearance.reset();
    if(overrideModel) {editorMarkerModelDispose(overrideModel);delete overrideModel;overrideModel=nullptr;}
    if(storage_->previous)storage_->previous->storage_->next=storage_->next;
    else Storage::first=storage_->next;
    if(storage_->next)storage_->next->storage_->previous=storage_->previous;
    else Storage::last=storage_->previous;
}
void FrontEndRenderingCar::ReInit(const CarAppearanceSelection* selection)
{
    storage_->appearance.reset();
    if(!selection)return;
    ride=*selection;
    auto appearance=std::make_unique<CarAppearance>();
    carRenderConstruct(nullptr,ride,*appearance,storage_->bindings);
    storage_->appearance=std::move(appearance);
}

void CollectFrontendCarModels(FrontendFrame& frame,RaceEffectsFrame& effects,std::int32_t worldFrames)
{
    for(auto* car=FrontEndRenderingCar::Storage::first;car;car=car->storage_->next) {
        auto* render=car->storage_->appearance.get();
        if(!render || !car->enabled || car->reflected)continue;
        const auto add=[&](const EditorMarkerModel* model,const float* matrix) {
            if(!model || !model->solid)return;
            FrontendModelRequest request{};
            request.model=model;request.viewNumber=car->viewNumber;
            std::copy_n(matrix,16,request.transform.begin());
            frame.models.push_back(request);
        };
        if(car->overrideModel) {
            add(car->overrideModel,car->orientation);
            continue;
        }
        // Match the normal frontend draw's authored lamp texture selection.
        updateCarRenderSceneTextures(*render,nullptr,nullptr,0,nullptr,nullptr,0);
        float minimum[4]{},maximum[4]{},body[16];
        editorMarkerModelBounds(&render->models[0],minimum,maximum);
        eMulMatrix(body,CarScaleMatrix,car->orientation);
        std::copy_n(car->position,3,body+12);
        body[12]-=(minimum[0]+maximum[0])*0.5f;
        body[15]=1.0f;
        const auto flags=(car->headlights ? 3u:0u)|(car->policeLights ? 8u:0u);
        visitCarLightFlares(*render,nullptr,worldFrames,flags,[&](const auto& flare,float intensity) {
            const auto first=effects.lights.size();
            CollectLightEffect(flare,body,intensity,*render->textures,effects.lights);
            for (auto i=first;i<effects.lights.size();++i) effects.lights[i].view=car->viewNumber;
            return 0u;
        });
        for(const auto slot:{0,16,17,18,19})add(&render->models[slot],body);
        float flipped[16];motionMatrixRotateYaw(flipped,eMathIdentityMatrix,0x8000u);
        for(int i=0;i<4;++i) {
            float local[16],world[16];
            if(i==0 || i==3)eMulMatrix(local,flipped,car->wheels[i]);
            else std::copy_n(car->wheels[i],16,local);
            eMulMatrix(world,local,body);
            add(&render->models[i>=2?5:4],world);
        }
    }
}

void RenderFrontEndCars(const CarCinematicScreen& view,EcstasyFrame& frame,
    std::int32_t worldFrames,std::int32_t presentationLayout,
    bool normal,bool reflectedPass,bool opaque,bool transparent)
{
    // PAL123D28..12405C. The shared CarRenderInfo geometry uses the source
    // null Car branch. Frontend forced body/wheel LOD tables both contain
    // {0,1}; passing the value avoids temporary duplicated global overrides.
    for(auto* car=FrontEndRenderingCar::Storage::first;car;car=car->storage_->next) {
        auto* render=car->storage_->appearance.get();
        if(!render || !car->enabled || car->viewNumber!=view.id)continue;
        if(car->reflected ? (presentationLayout!=1 || !reflectedPass):!normal)continue;
        std::uint32_t flags=car->reflected?0x11u:0u;
        if(!opaque)flags|=8u;
        if(!transparent)flags|=16u;
        if(car->overrideModel) {
            frame.RenderModel(*car->overrideModel,car->orientation,view,flags,1.0f,worldFrames);
            continue;
        }
        float minimum[4]{},maximum[4]{},position[4];
        editorMarkerModelBounds(&render->models[0],minimum,maximum);
        std::memcpy(position,car->position,sizeof(position));
        position[0]-=minimum[0]+(maximum[0]-minimum[0])*0.5f;
        renderCarAppearanceScene(*render,nullptr,view,frame,position,car->orientation,
            car->wheels,car->brakes,worldFrames,presentationLayout,car->reflected?1:0,flags,
            car->reflected ? 0u:(car->headlights ? 3u:0u)|(car->policeLights ? 8u:0u));
        // Original headlight/police flare light-context dispatch remains
        // tracked with CarRenderInfo::Render; geometry is not full acceptance.
    }
}

void UpdateEnvironmentMapCameras(EnvironmentMap& environment,CarCinematicServices& views,
    World* world,const CarFramePlayerRegistry* players,std::int32_t applicationState,
    float realTimeElapsed)
{
    // PAL123BA0..123C90. The native mover list is reversed: its last node
    // corresponds to the original intrusive head and active virtual owner.
    const float* origin=nullptr;
    if(world && world->totalCars>0) {
        Car* car=carCinematicGetCarToFollow(views.screens[1].last);
        if(!car && players && players->playersByNumber[0] && players->playersByNumber[0]->car)
            car=players->playersByNumber[0]->car->bindings.owner;
        if(!car)car=world->carTable[0];
        Require(car && car!=&world->carList && car->Movement() && car->Movement()->bindings.state,
            "Environment camera has no canonical source car pose");
        origin=car->Movement()->bindings.state->centerPosition;
    }
    if(!origin && FrontEndRenderingCar::Storage::first)
        origin=FrontEndRenderingCar::Storage::first->position;
    if(!origin)return;
    Require(views.screens[1].view,"Environment camera has no primary Camera owner");
    float position[3];
    for(std::size_t axis=0;axis<3;++axis)position[axis]=origin[axis]+environment.eyeOffset[axis];
    environment.UpdateCameras(views.screens[1].view->current.eye,position,applicationState,realTimeElapsed);
}
}

void RenderCars(World& world,const CarCinematicScreen& view,hp2::EcstasyFrame& frame,
    const GameTime& clock,std::int32_t presentationLayout,
    const hp2::CarDamageDisplayBindings* damageDisplay,CarViewMoverRegistry* views)
{
    // PAL124060..1242A4: intrusive World list, hidden camera car, original
    // view-mask cells, physical wheel matrices and subsequent DamageAbove.
    hp2::Require(view.view && view.id>=0 && view.id<32 && damageDisplay,
        "RenderCars lacks its source camera or DamageAbove display owners");
    int budget=99;
    for(auto* car=world.carList.next;car && car!=&world.carList;car=car->next) {
        if(car==view.view->hiddenCar || budget<=0)continue;
        auto* mover=car->Movement();
        hp2::Require(mover && mover->bindings.ownerMotion && mover->bindings.state,
            "RenderCars encountered a car without its actual motion owner");
        const auto& motion=*mover->bindings.ownerMotion;
        if(!motion.actionEnabled)continue;
        bool visible=carMotionTransitionFlag(&motion)==0;
        if(carMotionWheelSelected(&motion,static_cast<std::uint32_t>(view.id),clock.worldTimer) &&
            std::fabs(clock.worldTimeElapsed)>0.0010000000474974513f) {
            // PAL124060 alternates its transient visibility at the source
            // display cadence. Presentation FPS is independent on PC.
            const std::uint64_t frequency=gameTimeGetVideoMode(&clock)==HP2_VIDEO_PAL ? 50u:60u;
            const auto phase=(static_cast<std::uint64_t>(static_cast<std::uint32_t>(clock.realTimer))*frequency)/4000u;
            visible=(phase&1u)!=0;
        }
        if(!((motion.wheelEffectsFlags>>view.id)&1u))visible=false;
        if(!visible)continue;
        --budget;
        float wheels[4][16],brakes[4][16];car->GetRenderingWheelAndBrakeMatrices(wheels,brakes,view.id);
        const bool overridePose=((motion.wheelExclusionMask>>view.id)&1u)!=0;
        const auto* presentation=overridePose && views ? carViewMoverPresentation(views,car):nullptr;
        hp2::Require(!overridePose || (presentation && presentation->motion==&motion),
            "NIS rendering override has no canonical replay presentation owner");
        // PAL1241B0 supplies Car+50 (COM-adjusted model origin), not Car+60.
        const auto* position=overridePose ? presentation->centerPosition:mover->bindings.state->centerPosition;
        const auto* orientation=overridePose ? presentation->orientation:mover->bindings.state->orientation;
        carRenderScene(*car,view,frame,position,orientation,wheels,brakes,clock.worldTimeFrames,presentationLayout);
        if(damageDisplay) {
            bool suppress=false;
            for(int32_t i=0;i<damageDisplay->players.count;++i)if(const auto* player=damageDisplay->players.players[i])
                if(player->movement.seat+1==view.id) {
                    const auto state=player->movement.steeringFeedbackBusy;suppress=state==6 || state==7;break;
                }
            if(!suppress)car->DamageAbove(view.view->current.eye,view.view->current.forward,*damageDisplay);
        }
    }
}

uint32_t physicsGetPacked2Bit(uint32_t value, uint32_t index)
{
    return (value >> ((index << 1) & 31u)) & 3u;
}

namespace hp2 {

void CollectCarShadowEffects(Car& car,const float transform[16],const CarCinematicScreen& view,
    int layout,LightContextBatch& lights,RaceEffectsFrame& output)
{
    if(!car.appearance || !view.matricesCalculated)return;
    auto& render=*car.appearance;
    if(!render.models[0].solid || (!render.shadowTexture && !render.auxiliaryTexture))return;
    const auto scalePixels=[layout](std::int32_t pixels) {
        return layout==3 ? convertFloatToInt32(static_cast<float>(pixels)*0.699999988079071f):pixels;
    };
    const auto pixels=scalePixels(ecstasyViewSourcePositionPixelSize(&view,transform+12,3.0f));
    const auto distanceScale=nativeViewDistanceScale(view.id);
    const auto visibilityPixels=distanceScale==1.0f ? pixels :
        scalePixels(ecstasyViewNativePositionPixelSize(&view,transform+12,3.0f,1));
    if(visibilityPixels<4)return;
    float toClip[16];
    eMulMatrix(toClip,transform,view.matrices.worldToClip);
    if(!ecstasyClassifyBounds(render.minimum,render.maximum,toClip))return;
    float inverse[16],localEye[4];
    vectorMatrixInverseAffine(inverse,transform);
    eMulVector3(localEye,inverse,view.view->current.eye);
    auto groundCuller=render.cullers;
    carPartCullerCull(groundCuller.data(),localEye);
    auto lod=carRenderNativeDetailLod(render.models.data(),pixels);
    const auto contacts=car.Movement()->bindings.state->activeContactCount;
    if(lod>=3 && contacts==0 && groundCuller[12].culled)lod=2;
    if(ForceCarLOD>=0)lod=static_cast<std::size_t>(ForceCarLOD);
    if(distanceScale>1.0f)
        lod=std::min(lod,carRenderNativeDetailLod(render.models.data(),visibilityPixels));
    const bool sunVisible=view.id!=3 && lod<3 && render.auxiliaryTexture;
    const bool roadVisible=render.shadowTexture && (contacts!=4 || groundCuller[13].culled);
    if(!sunVisible && !roadVisible)return;
    const auto lighting=lights.SetupRace(transform,view.matrices.worldToView,view.view->current.eye);
    const auto emit=[&](const std::vector<FlailerPolyVertex>& strip,const TextureInfo& texture) {
        if(!(strip.front().color>>24))return;
        output.shadows.push_back({texture.descriptor,output.shadowVertices.size(),strip.size(),view.id});
        output.shadowVertices.insert(output.shadowVertices.end(),strip.begin(),strip.end());
    };
    if(sunVisible && lighting.sunlightIntensity>0.4f)
        renderCarSunShadow(render,&car,view,emit,transform,lighting,false,lod,visibilityPixels);
    if(roadVisible)
        renderCarRoadShadow(render,&car,emit,transform,&lighting,false,true);
}

std::array<const EditorMarkerModel*, 7> RaceCarModels(Car& car, int worldFrames)
{
    if(!car.appearance)return {};
    auto& appearance=*car.appearance;
    auto* frame=car.Frame();
    auto* movement=car.Movement();
    Require(frame && movement && movement->bindings.controls && car.driverInfo && appearance.bindings,
        "Race car asset snapshot has incomplete appearance bindings");
    updateCarRenderSceneTextures(appearance,&car,&frame->state,car.driverInfo->driverClass,
        movement->bindings.controls,appearance.bindings->scenario,worldFrames);
    const auto& models=appearance.models;
    const EditorMarkerModel* body=nullptr;
    for(std::size_t level=0;level<4;++level) {
        if(models[level].solid) {
            body=&models[level];
            break;
        }
    }
    return {body,&models[16],&models[17],&models[18],&models[19],&models[4],&models[5]};
}

}

#include "indep/inc/AICarController.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/NativeRenderSettings.hpp"
#if defined(_M_X64) || defined(__SSE2__)
#include <xmmintrin.h>
#endif

#include "CameraMover.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/eLight.hpp"
#include "indep/inc/eEnvMap.hpp"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/Texture.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/WorldObject.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/NIS.hpp"
#include "indep/inc/Clans.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/WeatherMan.hpp"
#include "indep/inc/FMenus.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/Timer.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>
#include <atomic>
#include <exception>
#include <cstdio>
#include <sstream>
#include <iterator>
#include <list>
#include <memory_resource>
#include <type_traits>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <sched.h>
#endif

namespace {
const EditorMarkerSolid* findModelSolid(const EditorMarkerResources* root,uint32_t key)
{
    for(const EditorMarkerResources* resource=root;resource;resource=resource->nextCatalogue)
        for(size_t index=0;index<resource->solidCount;++index)
            if(resource->solids[index].key==key)return resource->solids+index;
    return nullptr;
}

EditorMarkerTexture* findModelTexture(EditorMarkerResources* root,uint32_t key)
{
    for(EditorMarkerResources* resource=root;resource;resource=resource->nextCatalogue)
        for(size_t index=0;index<resource->textureCount;++index)
            if(resource->textures[index].key==key)return resource->textures+index;
    return nullptr;
}
}

namespace hp2 {
unsigned NativeSceneEffectiveHardwareConcurrency() noexcept
{
    const unsigned reported=std::max(1u,std::thread::hardware_concurrency());
    std::uint64_t affinityMask=0;
#if defined(_WIN32)
    DWORD_PTR processMask=0,systemMask=0;
    if(GetProcessAffinityMask(GetCurrentProcess(),&processMask,&systemMask))
        affinityMask=static_cast<std::uint64_t>(processMask);
    GROUP_AFFINITY currentThread{};
    if(GetThreadGroupAffinity(GetCurrentThread(),&currentThread)) {
        const auto threadMask=static_cast<std::uint64_t>(currentThread.Mask);
        affinityMask=affinityMask ? affinityMask&threadMask:threadMask;
    }
#elif defined(__linux__)
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    if(sched_getaffinity(0,sizeof(allowed),&allowed)==0) {
        const auto count=static_cast<unsigned>(CPU_COUNT(&allowed));
        if(count)return std::min(reported,count);
    }
#endif
    if(affinityMask) {
        unsigned count=0;
        while(affinityMask) {affinityMask&=affinityMask-1;++count;}
        return std::min(reported,count);
    }
    // An unknown OS affinity is not permission to occupy reported host CPUs.
    return 1;
}

std::int32_t environmentMappingEnabled=1; // Released writable EnableEnvMap32F720.
std::array<float,2> textureAnimationScrollSpeed{};
std::int32_t textureAnimationScrollMode=1;
float textureAnimationScrollInterval=1.0f;
EcstasyNativeRenderPolicy nativeRenderPolicy;
std::uint32_t ecstasyCaptureNativeRenderEggState() noexcept
{
    return (nativeRenderPolicy.texturing ? EcstasyRenderEggTexturing:0u) |
        (nativeRenderPolicy.wireframe ? EcstasyRenderEggWireframe:0u) |
        (nativeRenderPolicy.pixelClampOverride ? EcstasyRenderEggClampTopLeft:0u);
}
struct ResourceRegistry::Storage {
    EditorMarkerResources& models;
    CarReplayAnimationRegistry& animations;
    ChopperEffectData& effects;
    FlailerResources& flailer;
    LightTextureResources light;
    SunResources sun;
    LightSceneRegistry lights;
    QueuedFileService* queuedFiles{};
    std::array<EditorMarkerTexture*,9> specialTextures{};
    Storage(EditorMarkerResources& bank,CarReplayAnimationRegistry& animationList,
        ChopperEffectData& effectData,FlailerResources& ui)
        :models(bank),animations(animationList),effects(effectData),flailer(ui)
    {}
};

ResourceRegistry::ResourceRegistry(EditorMarkerResources& models,CarReplayAnimationRegistry& animations,
    ChopperEffectData& effects,FlailerResources& flailer)
    :storage_(std::make_unique<Storage>(models,animations,effects,flailer))
{
    Require(!models.catalogueRoot,"Resource registry requires the canonical model bank");
    Require(!models.resourceRegistry,"Model bank already has its actual resource registry");
    models.resourceRegistry=this;
}
ResourceRegistry::~ResourceRegistry()
{
    ecstasyDeleteAllDuplicatedSolids(&storage_->models);
    if (storage_->models.resourceRegistry==this)storage_->models.resourceRegistry=nullptr;
}
EditorMarkerResources& ResourceRegistry::Models() noexcept { return storage_->models; }
CarReplayAnimationRegistry& ResourceRegistry::Animations() noexcept { return storage_->animations; }
ChopperEffectData& ResourceRegistry::Effects() noexcept { return storage_->effects; }
FlailerResources& ResourceRegistry::Flailer() noexcept { return storage_->flailer; }
SunResources& ResourceRegistry::Sun() noexcept {return storage_->sun;}
LightSceneRegistry& ResourceRegistry::Lights() noexcept {return storage_->lights;}
void ResourceRegistry::BindQueuedFiles(QueuedFileService& files)
{
    Require(!storage_->queuedFiles || storage_->queuedFiles==&files,
        "Resource registry cannot borrow a second queued-file service");
    storage_->queuedFiles=&files;
}
QueuedFileService* ResourceRegistry::QueuedFiles() const noexcept {return storage_->queuedFiles;}
const EditorMarkerTexture* ResourceRegistry::FindTexture(std::uint32_t key) const noexcept
{ return findModelTexture(&storage_->models,key); }
EditorMarkerTexture* ResourceRegistry::ResolveTexture(std::uint32_t key,bool fallback)
{
    if(auto* texture=texturePackRegistry.ResolveLoadedTexture(storage_->models,key))return texture->descriptor;
    // PAL1ABF48(a2=0) uses1AC3F0: pixels and any required palette must
    // already belong to the same record. Metadata alone is not drawable.
    // The separate a2=1 metadata lookup is not requested by these callers.
    return fallback && key!=0x19006835u ? ResolveTexture(0x19006835u,false) : nullptr;
}
const EditorMarkerLightMaterial* ResourceRegistry::FindLightMaterial(std::uint32_t key) const noexcept
{
    for (auto* catalogue=storage_->models.lightMaterialCatalogues;catalogue;catalogue=catalogue->next)
        for (const auto& material:catalogue->materials)if (material.key==key)return &material;
    return storage_->models.defaultLightMaterial;
}
bool ResourceRegistry::IsTextureAlive(const EditorMarkerTexture* texture) const noexcept
{
    if (!texture)return false;
    for (auto* resource=&storage_->models;resource;resource=resource->nextCatalogue)
        for (std::size_t index=0;index<resource->textureCount;++index)
            if (resource->textures+index==texture)return true;
    return false;
}
void ResourceRegistry::ResourceRefresh()
{
    auto& models=storage_->models;
    if (models.fixupsBlocked)return;
    if (models.modelsChanged || models.texturesChanged) {
        if (models.texturesChanged)
            for (auto* catalogue=&models;catalogue;catalogue=catalogue->nextCatalogue)
                for (std::size_t index=0;index<catalogue->solidCount;++index)
                    if (auto* geometry=catalogue->solids[index].geometry) {
                        geometry->FixTextureTable(*this);
                        geometry->FixStripSTs();
                        geometry->CalcAutoK();
                    }
        editorMarkerModelsReconnectAll(&models);
        carReplayAnimationReconnectAll(&storage_->animations);
        RefreshSpecialTextures();
        models.modelsChanged=0;models.texturesChanged=0;
        storage_->animations.reconnectRequired=0;
    }
    carReplayAnimationResourceEndFixup(&storage_->animations);
}
void ResourceRegistry::UpdateLightTextures() { storage_->light.UpdateTextures(*this); }
void ResourceRegistry::RefreshSpecialTextures()
{
    constexpr std::array<std::uint32_t,9> keys{0xdbd4u,0xcf29d0dfu,0x32283a11u,
        0x02c8711cu,0xbad886d4u,0xab9a6e21u,0x0184d1f8u,0xb9ab56e4u,0x4ecc769cu};
    for (std::size_t index=0;index<keys.size();++index)
        storage_->specialTextures[index]=ResolveTexture(keys[index],false);
    auto* descriptor=storage_->specialTextures[7];
    if (!descriptor)return;
    auto* texture=textureInfoForDescriptor(descriptor);
    Require(texture!=nullptr,"Special texture has no canonical pixel/palette owner");
    auto* palette=texture->palette.MutableData();
    if (!palette)return;
    Require(texture->paletteBytes>=0 && texture->palette.size>=static_cast<std::size_t>(texture->paletteBytes),
        "Special texture palette extent is truncated");
    CreateSpecialTexturePalette(palette,static_cast<std::size_t>(texture->paletteBytes)/4u);
}
void ResourceRegistry::CreateSpecialTexturePalette(std::uint8_t* palette,std::size_t count)
{
    Require(palette || !count,"Special palette has no output storage");
    for(std::size_t index=0;index<count;++index) {
        const float angle=static_cast<float>(index)/static_cast<float>(count)*1.5707499980926514f+
            1.5707964897155762f;
        const float intensity=std::max(rotationSineRadians(angle),0.009999999776482582f);
        // PAL103328..103360's float-word conversion preserves an intensity
        // near its input, not int(intensity) reinterpreted as a denormal.
        // Native PC evaluates the curve directly and retains the final byte
        // truncation. In particular intensity1 must produce128, never0.
        const auto lane=static_cast<std::uint8_t>(convertFloatToInt32(std::clamp(intensity*128.0f,0.0f,255.0f)));
        std::fill_n(palette+index*4u,4,lane);
    }
}

const EditorMarkerTexture* ResourceRegistry::LightTexture(std::size_t index) const
{ return storage_->light.At(index); }
void ResourceRegistry::RetireTextureCatalogue(const EditorMarkerResources& catalogue) noexcept
{
    // RefreshSpecialTextures borrows descriptors from their actual pack.
    // During original track/common retirement fixups remain blocked, but the
    // native RAII catalogue is freed immediately. Revoke only its aliases
    // before detaching; no lookup, rebinding, or fallback is performed here.
    for(auto*& texture:storage_->specialTextures)
        for(std::size_t index=0;texture && index<catalogue.textureCount;++index)
            if(texture==catalogue.textures+index)texture=nullptr;
}
const EditorMarkerTexture* ResourceRegistry::RearViewTexture(std::size_t index) const
{
    Require(index<2,"Rear-view special texture index is outside its source pair");
    return storage_->specialTextures[index];
}
const EditorMarkerTexture* ResourceRegistry::SpecialTexture(std::size_t index) const
{
    Require(index<storage_->specialTextures.size(),"Special texture index is outside the original table");
    return storage_->specialTextures[index];
}
}

namespace {
const EditorMarkerTexture& stripTexture(const EditorMarkerSolidGeometry& geometry,
    const EditorMarkerSolidGeometry::Strip& strip)
{
    hp2::Require(strip.textureIndex>=0 && static_cast<std::size_t>(strip.textureIndex)<geometry.textures.size(),
        "Solid strip texture index is outside its retained table");
    const auto* texture=geometry.textures[static_cast<std::size_t>(strip.textureIndex)].texture;
    hp2::Require(texture && texture->authoredRecord && texture->authoredRecordSize>=0x74,
        "Solid texture binding has no actual texture descriptor");
    return *texture;
}

float solidApproximateLog2(float value)
{
    std::uint32_t bits;std::memcpy(&bits,&value,4);
    const int exponent=static_cast<int>((bits>>23)&255u);
    const int power=exponent-127<1 ? exponent-128 : exponent-126;
    const float iterations=std::fabs(finiteRatio(127.0f,static_cast<float>(power)));
    const float factor=value;
    for (int i=1;i<convertFloatToInt32(iterations);++i)value*=factor;
    std::memcpy(&bits,&value,4);
    return finiteRatio(static_cast<float>(static_cast<int>((bits>>23)&255u)-127),iterations);
}

std::int16_t solidTextureK(const EditorMarkerTexture& texture,float ratio)
{
    const auto* record=texture.authoredRecord;
    int bias=hp2::ReadU16(record+0x72)&0xfff;
    if (bias&0x800)bias-=0x1000;
    if (ratio==0.0f)return 0;
    std::int8_t level;std::memcpy(&level,record+0x71,1);
    const std::uint32_t scaleBits=1u<<(static_cast<std::uint32_t>(level)&31u);
    std::int32_t scale;std::memcpy(&scale,&scaleBits,4);
    const float logarithm=solidApproximateLog2(finiteRatio(1.0f,std::sqrt(std::fabs(ratio))));
    const float difference=static_cast<float>(bias)*0.0625f-logarithm*static_cast<float>(scale);
    const auto packed=static_cast<std::uint16_t>(convertFloatToInt32(difference*16.0f));
    std::int16_t result;std::memcpy(&result,&packed,2);return result;
}
}

void EditorMarkerSolidGeometry::FixTextureTable(hp2::ResourceRegistry& resources)
{
    for (auto& binding:textures)binding.texture=resources.ResolveTexture(binding.key,true);
}
void EditorMarkerSolidGeometry::FixLightMaterialTable(hp2::ResourceRegistry& resources)
{
    lightMaterials.resize(lightMaterialKeys.size());
    for (std::size_t index=0;index<lightMaterialKeys.size();++index)
        lightMaterials[index]=resources.FindLightMaterial(lightMaterialKeys[index]);
}

void EditorMarkerSolidGeometry::Strip::PrepareNativeTopology()
{
    const std::size_t count=positions.size();
    hp2::Require(count<=48,"Native solid strip topology exceeds source vertex limit");
    nativeUniqueOrder.clear();nativeTriangleOrder.clear();
    nativeUniqueOrder.reserve(count);
    nativeTriangleOrder.reserve(count>2 ? (count-2)*3 : 0);
    std::uint64_t visited=0;
    for(std::size_t triangle=2;triangle<count;++triangle) {
        if(drawDisableMask&(std::uint64_t{1}<<triangle))continue;
        const std::size_t corners[]={triangle-2+(triangle&1u),
            triangle-1-(triangle&1u),triangle};
        for(const auto vertex:corners) {
            const auto bit=std::uint64_t{1}<<vertex;
            if(!(visited&bit)) {
                nativeUniqueOrder.push_back(static_cast<std::uint8_t>(vertex));
                visited|=bit;
            }
            nativeTriangleOrder.push_back(static_cast<std::uint8_t>(vertex));
        }
    }
    nativeTopologyCount=count;
    nativeTopologyMask=drawDisableMask;
}

void EditorMarkerSolidGeometry::FixStripSTs()
{
    for (auto& strip:strips) {
        const auto& texture=stripTexture(*this,strip);
        if (texture.key==0x19006835u)continue;
        const std::uint16_t dimensions[2]={hp2::ReadU16(texture.authoredRecord+0x24),
            hp2::ReadU16(texture.authoredRecord+0x26)};
        float ratios[2],inverses[2];bool changed[2];
        for (int axis=0;axis<2;++axis) {
            hp2::Require(dimensions[axis]>0 && dimensions[axis]<=32767,"Solid texture dimension is invalid");
            std::uint32_t power=1;while (power<dimensions[axis])power<<=1;
            ratios[axis]=static_cast<float>(dimensions[axis])/static_cast<float>(power);
            const auto scale=static_cast<std::uint16_t>(convertFloatToInt32(ratios[axis]*32768.0f));
            changed[axis]=scale!=strip.textureScales[axis];
            inverses[axis]=finiteRatio(1.0f,static_cast<float>(strip.textureScales[axis])*0.000030517578125f);
        }
        if (!changed[0] && !changed[1])continue;
        for (auto& st:strip.textureCoordinates)
            for (int axis=0;axis<2;++axis)if (changed[axis]) {
                st[axis]*=inverses[axis];st[axis]*=ratios[axis];
            }
        for (int axis=0;axis<2;++axis)if (changed[axis])
            strip.textureScales[axis]=static_cast<std::uint16_t>(convertFloatToInt32(ratios[axis]*32768.0f));
    }
}

void EditorMarkerSolidGeometry::CalcAutoK()
{
    for (auto& strip:strips) {
        const auto& texture=stripTexture(*this,strip);
        const float projection=static_cast<float>(strip.autoKProjection)*0.00006103515625f;
        const float perpendicular=std::sqrt(std::fabs(1.0f-projection*projection));
        const float cosine=maximumFinite(0.0f,rotationSineAngle(static_cast<std::uint16_t>(strip.autoKAngle+0x4000u)));
        const float coefficient=(cosine*0.4f+0.6f)*0.8f;
        const float denominator=maximumFinite((coefficient*perpendicular+1.0f)-coefficient,0.001f);
        const int width=static_cast<int>(hp2::ReadU16(texture.authoredRecord+0x24));
        const int height=static_cast<int>(hp2::ReadU16(texture.authoredRecord+0x26));
        strip.autoK=solidTextureK(texture,finiteRatio(strip.autoKScale*static_cast<float>(width*height),denominator));
        strip.mipLevelScale=texture.authoredRecord[0x71];
        // Source SetMipMapK writes the low byte to the strip attribute.
        // Vulkan will consume this CPU value, not an invented GS packet.
    }
}

int32_t editorMarkerResourcesAttach(EditorMarkerResources* root,EditorMarkerResources* catalogue)
{
    if(!root || !catalogue || catalogue->catalogueRoot ||
       catalogue->nextCatalogue || catalogue->first)return 0;
    if(root->catalogueRoot)root=root->catalogueRoot;
    if(root==catalogue)return 0;
    EditorMarkerResources* tail=root;
    while(tail->nextCatalogue)tail=tail->nextCatalogue;
    tail->nextCatalogue=catalogue;
    catalogue->catalogueRoot=root;
    if(catalogue->solidCount)root->modelsChanged=1;
    if(catalogue->textureCount)root->texturesChanged=1;
    ++root->bindingRevision;
    return 1;
}

void editorMarkerResourcesDetach(EditorMarkerResources* catalogue)
{
    if(!catalogue || !catalogue->catalogueRoot)return;
    EditorMarkerResources* root=catalogue->catalogueRoot;
    EditorMarkerResources* previous=root;
    while(previous->nextCatalogue && previous->nextCatalogue!=catalogue)previous=previous->nextCatalogue;
    if(previous->nextCatalogue!=catalogue)return;
    if(root->resourceRegistry)root->resourceRegistry->RetireTextureCatalogue(*catalogue);
    previous->nextCatalogue=catalogue->nextCatalogue;
    catalogue->nextCatalogue=nullptr;
    catalogue->catalogueRoot=nullptr;
    if(catalogue->solidCount)root->modelsChanged=1;
    if(catalogue->textureCount)root->texturesChanged=1;
    ++root->bindingRevision;
}

void editorMarkerModelInitialize(EditorMarkerModel* model,EditorMarkerResources* resources,uint32_t key)
{
    std::memset(model,0,sizeof(*model));
    if(resources->catalogueRoot)resources=resources->catalogueRoot;
    model->resources=resources;
    model->key=key;
    if(!key)return;
    editorMarkerModelReconnectSolid(model);
    model->previous=resources->last;
    if(resources->last)resources->last->next=model;
    else resources->first=model;
    resources->last=model;
}

void editorMarkerModelSetReplacement(EditorMarkerModel* model,EditorMarkerReplacement* replacement)
{
    // The two-argument native caller still requires the complete original
    // attach contract: publish replacements, apply bindings and repair STs.
    // Reconnecting the table alone leaves source geometry using old UVs.
    ++model->resources->bindingRevision;
    editorMarkerModelAttachReplacementTextures(model,replacement,replacement ? 1:0,
        model->detailLevel);
}

void editorMarkerModelAttachReplacementTextures(EditorMarkerModel* model,
    EditorMarkerReplacement* replacements,int32_t count,int32_t detailLevel)
{
    // PAL10C798. These are the original shared replacement cells. Each model
    // records its own detail index into them; geometry remains in its eSolid.
    model->replacement=replacements;
    const auto countByte=static_cast<std::uint8_t>(count);
    const auto detailByte=static_cast<std::uint8_t>(detailLevel);
    std::memcpy(&model->replacementCount,&countByte,1);
    std::memcpy(&model->detailLevel,&detailByte,1);
    editorMarkerModelReconnectTextureTable(model);
    if (model->solid) {
        editorMarkerModelApplyReplacementTextureTable(model);
        hp2::Require(model->solid->geometry!=nullptr,"Model has no decoded geometry owner");
        model->solid->geometry->FixStripSTs();
    }
}

int32_t editorMarkerModelRenameTexture(EditorMarkerModel* model,uint32_t originalKey,uint32_t replacementKey)
{
    // PAL10C7F0 changes the first matching key in the live solid table.
    if (!model->solid)return 0;
    hp2::Require(model->solid->geometry && model->resources && model->resources->resourceRegistry,
        "Model texture rename has no geometry/resource owner");
    for (auto& texture:model->solid->geometry->textures)if (texture.key==originalKey) {
        texture.key=replacementKey;
        texture.texture=model->resources->resourceRegistry->ResolveTexture(replacementKey,true);
        return 1;
    }
    return 0;
}

void editorMarkerSolidReplaceLightMaterial(const EditorMarkerSolid* solid,uint32_t originalKey,
    const EditorMarkerLightMaterial* replacement)
{
    // PAL10C4B0 replaces every matching material pointer, retaining the keys.
    hp2::Require(solid && solid->geometry,"Light material replacement has no solid geometry");
    auto& geometry=*solid->geometry;
    hp2::Require(geometry.lightMaterials.size()==geometry.lightMaterialKeys.size(),
        "Light material keys and references differ");
    for (std::size_t index=0;index<geometry.lightMaterialKeys.size();++index)
        if (geometry.lightMaterialKeys[index]==originalKey)geometry.lightMaterials[index]=replacement;
}

void editorMarkerSolidReplaceLightMaterialKey(const EditorMarkerSolid* solid,EditorMarkerResources* resources,
    uint32_t originalKey,uint32_t replacementKey)
{
    // PAL10C4E0 resolves the same global material catalogue used by the solid.
    hp2::Require(resources && resources->resourceRegistry,"Light material replacement has no registry");
    editorMarkerSolidReplaceLightMaterial(solid,originalKey,
        resources->resourceRegistry->FindLightMaterial(replacementKey));
}

void editorMarkerModelReplaceLightMaterial(EditorMarkerModel* model,uint32_t originalKey,uint32_t replacementKey)
{
    // PAL10CAC0 is the model-to-solid tail call, with the source null test.
    if (model->solid)editorMarkerSolidReplaceLightMaterialKey(
        model->solid,model->resources,originalKey,replacementKey);
}

const EditorMarkerAnimationTree* editorMarkerModelAnimationTree(const EditorMarkerModel* model)
{
    // Complete PAL10CA80: an existing solid owns a tree even when count is0.
    return model->solid ? &model->solid->animationTree : nullptr;
}

void editorMarkerModelReconnectSolid(EditorMarkerModel* model)
{
    if (!model->key)return;
    const auto* solid=findModelSolid(model->resources,model->key);
    if (solid==model->solid)return;
    model->solid=solid;
    model->rigidAnimationStart=0;
    if (!solid || !solid->rigidAnimation || solid->rigidAnimation->playbackType!=0)return;
    model->rigidAnimationStart=0xc000u;
    if (solid->resourceFlags&0x20u)
        model->rigidAnimationStart=static_cast<std::uint16_t>(model->rigidAnimationStart|vectorDefaultRandomBounded(256));
}

void editorMarkerModelReconnectTextureTable(EditorMarkerModel* model)
{
    if (model->replacementCount<=0)return;
    hp2::Require(model->replacement && model->detailLevel>=0 && model->detailLevel<16,
        "Model replacement array or detail level is invalid");
    auto* registry=model->resources->resourceRegistry;
    hp2::Require(registry!=nullptr,"Model replacement requires its actual resource registry");
    for (std::int32_t index=0;index<model->replacementCount;++index) {
        auto& replacement=model->replacement[index];
        replacement.texture=registry->ResolveTexture(replacement.replacementKey,true);
        auto& mapped=replacement.textureIndices[model->detailLevel];mapped=-1;
        if (model->solid && model->solid->geometry) {
            const auto& textures=model->solid->geometry->textures;
            for (std::size_t texture=0;texture<textures.size();++texture)
                if (textures[texture].key==replacement.originalKey) {
                    const auto textureByte=static_cast<std::uint8_t>(texture);
                    std::memcpy(&mapped,&textureByte,1);break;
                }
        }
    }
    editorMarkerModelApplyReplacementTextureTable(model);
}

void editorMarkerModelApplyReplacementTextureTable(EditorMarkerModel* model)
{
    // Actual10C878 is also called by RenderStrips. It changes the shared
    // solid texture table, rather than constructing a per-model copy.
    if (!model->solid || model->replacementCount<=0)return;
    hp2::Require(model->solid->geometry && model->replacement &&
        model->detailLevel>=0 && model->detailLevel<16,
        "Replacement application has an invalid geometry, array or detail binding");
    auto& bindings=model->solid->geometry->textures;
    for (std::int32_t index=0;index<model->replacementCount;++index) {
        const auto& replacement=model->replacement[index];
        const auto mapped=replacement.textureIndices[model->detailLevel];
        if (mapped>=0) {
            hp2::Require(static_cast<std::size_t>(mapped)<bindings.size(),
                "Model replacement binding is outside the solid");
            bindings[static_cast<std::size_t>(mapped)].texture=replacement.texture;
        }
    }
}

void editorMarkerModelsReconnectAll(EditorMarkerResources* resources)
{
    for (auto* model=resources->first;model;model=model->next) {
        editorMarkerModelReconnectSolid(model);
        editorMarkerModelReconnectTextureTable(model);
        if (model->solid && model->solid->geometry)model->solid->geometry->FixStripSTs();
    }
}

void editorMarkerModelDispose(EditorMarkerModel* model)
{
    if(!model->key)return;
    EditorMarkerResources* resources=model->resources;
    if(model->previous)model->previous->next=model->next;
    else resources->first=model->next;
    if(model->next)model->next->previous=model->previous;
    else resources->last=model->previous;
    model->solid=nullptr;
    model->key=0;
    model->replacement=nullptr;
    model->replacementCount=0;
    model->rigidAnimationStart=0;
    model->next=model->previous=nullptr;
}

struct EditorMarkerDuplicatedSolid {
    EditorMarkerSolid solid{};
    std::unique_ptr<EditorMarkerSolidGeometry> geometry;
    EditorMarkerResources catalogue{};
    EditorMarkerDuplicatedSolid* next{};
};

void ecstasySolidChangeName(EditorMarkerSolid* solid,const char* name)
{
    const auto size=std::strlen(name);
    hp2::Require(size<sizeof(solid->name),"Duplicated solid name exceeds its original field");
    std::memcpy(solid->name,name,size+1);
    solid->key=hashString(name);
}

void ecstasyDuplicateSolid(EditorMarkerResources* resources,const char* sourceName,const char* newName)
{
    if (resources->catalogueRoot)resources=resources->catalogueRoot;
    const auto* source=findModelSolid(resources,hashString(sourceName));
    if (!source)return;
    auto copy=std::make_unique<EditorMarkerDuplicatedSolid>();
    copy->solid=*source;
    hp2::Require(source->geometry!=nullptr,"Duplicated solid has no decoded vertex owner");
    copy->geometry=std::make_unique<EditorMarkerSolidGeometry>(*source->geometry);
    copy->solid.geometry=copy->geometry.get();
    copy->solid.resourceFlags=static_cast<std::uint16_t>(copy->solid.resourceFlags|4u);
    ecstasySolidChangeName(&copy->solid,newName);
    copy->catalogue.solids=&copy->solid;
    copy->catalogue.solidCount=1;
    hp2::Require(editorMarkerResourcesAttach(resources,&copy->catalogue)!=0,
        "Duplicated solid could not join its existing resource bank");
    auto** last=&resources->duplicatedSolids;
    while (*last)last=&(*last)->next;
    *last=copy.release();
}

void ecstasyDeleteAllDuplicatedSolids(EditorMarkerResources* resources)
{
    if (resources->catalogueRoot)resources=resources->catalogueRoot;
    auto* copy=resources->duplicatedSolids;
    while (copy) {
        auto* next=copy->next;
        editorMarkerResourcesDetach(&copy->catalogue);
        // Native references must not retain a freed solid. The same live
        // eModel list is used; reconnection selects the surviving catalogue.
        for (auto* model=resources->first;model;model=model->next)
            if (model->solid==&copy->solid)editorMarkerModelReconnectSolid(model);
        delete copy;
        copy=next;
    }
    resources->duplicatedSolids=nullptr;
}

int32_t ecstasyModelNumVertices(const EditorMarkerModel* model)
{
    if (!model->solid)return 0;
    hp2::Require(model->solid->geometry!=nullptr,"Model has no decoded solid geometry");
    return model->solid->geometry->vertexCount;
}

void ecstasySolidSetEnvironmentMapping(const EditorMarkerSolid* solid,uint32_t textureKey,int32_t enabled)
{
    hp2::Require(solid->geometry!=nullptr,"Environment mapping has no decoded solid");
    auto& geometry=*solid->geometry;
    for (auto& strip:geometry.strips) {
        hp2::Require(strip.textureIndex>=0 && static_cast<std::size_t>(strip.textureIndex)<geometry.textures.size(),
            "Environment-mapped strip has an invalid texture selector");
        if (geometry.textures[static_cast<std::size_t>(strip.textureIndex)].key!=textureKey)continue;
        if (enabled)strip.flags=static_cast<std::uint16_t>(strip.flags|0x0100u);
        else strip.flags=static_cast<std::uint16_t>(strip.flags&0xfeffu);
    }
}

void ecstasyModelSetEnvironmentMapping(EditorMarkerModel* model,uint32_t textureKey,int32_t enabled)
{
    if (model->solid)ecstasySolidSetEnvironmentMapping(model->solid,textureKey,enabled);
}

void ecstasyCallDamageVertexFunction(EditorMarkerModel* model,BodyDamageModel* damage,
    EcstasyDamageVertexFunction function)
{
    if (!model->solid)return;
    hp2::Require(model->solid->geometry!=nullptr,"Damage traversal has no live solid geometry");
    std::int32_t vertexIndex=0;
    for (auto& strip:model->solid->geometry->strips) {
        const auto damageFlag=static_cast<std::int32_t>(strip.flags&0x0800u);
        hp2::Require(strip.colors.size()==strip.positions.size(),
            "Damage traversal requires the original RGB555 channel for every vertex");
        std::int32_t damaged=0;
        for (std::size_t vertex=0;vertex<strip.positions.size();++vertex) {
            // PAL unpacks XYZ, calls the real function, then packs XYZ back.
            // The native decoded arrays are already its canonical storage.
            auto position=strip.positions[vertex];
            damaged|=function(damage,vertexIndex++,position.data(),&strip.colors[vertex],nullptr,damageFlag);
            strip.positions[vertex]=position;
        }
        if (damageFlag) {
            if (damaged)strip.flags=static_cast<std::uint16_t>(strip.flags&0xfbffu);
            else strip.flags=static_cast<std::uint16_t>(strip.flags|0x0400u);
        }
    }
}

int32_t carCinematicCurrentViewMode(const CarCinematicServices* services)
{
    return services->presentationLayout;
}

void ecstasyViewInitialize(CarCinematicScreen* screen)
{
    std::memset(screen,0,sizeof(*screen));
    screen->viewScale=1.0f;
}

void ecstasyViewsBind(CarCinematicServices* services,CarCinematicView cameras[4],
    EcstasyRenderTarget targets[8])
{
    auto& first=services->screens[0];
    first.id=0;first.target=&targets[0];first.auxiliaryTarget=&targets[3];
    first.enabled=1;first.view=&cameras[0];
    auto& playerOne=services->screens[1];
    playerOne.id=1;playerOne.target=&targets[1];playerOne.auxiliaryTarget=&targets[4];
    playerOne.view=&cameras[1];
    auto& playerTwo=services->screens[2];
    playerTwo.id=2;playerTwo.target=&targets[2];playerTwo.auxiliaryTarget=&targets[5];
    playerTwo.view=&cameras[2];
    auto& mirror=services->screens[3];
    mirror.id=3;mirror.target=&targets[7];mirror.view=&cameras[3];
}

CarCinematicScreen* ecstasyViewAt(CarCinematicServices* services,int32_t index)
{
    // The original bank has ten records. Reject an invalid native binding
    // instead of forming an out-of-range host pointer.
    return index>=0 && index<10 ? services->screens+index : nullptr;
}

CarCinematicScreen* ecstasyViewCurrent(const CarCinematicServices* services)
{
    return services->currentScreen;
}

void ecstasyViewSetCurrent(CarCinematicServices* services,CarCinematicScreen* screen)
{
    services->currentScreen=screen;
}

CarCinematicView* ecstasyViewCurrentCamera(const CarCinematicServices* services)
{
    return services->currentScreen ? services->currentScreen->view : nullptr;
}

void ecstasyViewsUpdateMode(CarCinematicServices* services)
{
    const bool one=services->screens[1].last!=nullptr;
    const bool two=services->screens[2].last!=nullptr;
    const bool mirror=services->screens[3].last!=nullptr;
    services->presentationLayout=one && two ? 3 : one && mirror ? 2 : one ? 1 : 0;
}

void ecstasyViewsApplyMode(CarCinematicServices* services)
{
    // Complete layout switch of PAL107DB0..108048, reached by the actual
    // eDisplayFrame1128F4. GPU retirement is guaranteed by the synchronous
    // native presenter before this next-frame viewport mutation.
    hp2::Require(services && services->screens[0].target,
        "View layout has no actual presentation extent");
    services->appliedPresentationLayout=services->presentationLayoutOverride ?
        services->presentationLayoutOverride:services->presentationLayout;
    auto& one=services->screens[1];auto& two=services->screens[2];auto& mirror=services->screens[3];
    hp2::Require(one.target && two.target && mirror.target,"Original view target bank is incomplete");
    auto& first=*one.target;auto& second=*two.target;auto& rear=*mirror.target;
    auto& firstAux=one.auxiliaryTarget ? *one.auxiliaryTarget:first;
    auto& secondAux=two.auxiliaryTarget ? *two.auxiliaryTarget:second;
    const auto width=services->screens[0].target->width,height=services->screens[0].target->height;
    const auto rectangle=[](EcstasyRenderTarget& target,int32_t left,int32_t top,int32_t w,int32_t h) {
        target.left=left;target.top=top;target.width=w;target.height=h;
    };
    switch(services->appliedPresentationLayout) {
    case 3:
        rectangle(first,0,0,width,height/2);rectangle(second,0,height/2,width,height/2);
        rectangle(firstAux,0,0,width,height/2);rectangle(secondAux,0,height/2,width,height/2);
        rectangle(rear,0,0,0,0);first.enabled=second.enabled=1;one.enabled=two.enabled=1;
        rear.enabled=0;mirror.enabled=0;break;
    case 4:
        rectangle(first,0,0,width/2,height);rectangle(second,width/2,0,width/2,height);
        rectangle(firstAux,0,0,width/2,height);rectangle(secondAux,width/2,0,width/2,height);
        rectangle(rear,0,0,0,0);first.enabled=second.enabled=1;one.enabled=two.enabled=1;
        rear.enabled=0;mirror.enabled=0;break;
    case 2:
        rectangle(first,0,0,width,height);rectangle(second,0,0,0,0);
        rectangle(firstAux,0,0,width,height);rectangle(secondAux,0,0,0,0);
        rectangle(rear,0,0,256,64);first.enabled=1;one.enabled=1;
        second.enabled=0;two.enabled=0;rear.enabled=1;mirror.enabled=1;break;
    case 0:case 1:
        rectangle(first,0,0,width,height);rectangle(second,0,0,0,0);
        rectangle(firstAux,0,0,width,height);rectangle(secondAux,0,0,0,0);
        rectangle(rear,0,0,0,0);first.enabled=1;one.enabled=1;
        second.enabled=0;two.enabled=0;rear.enabled=0;mirror.enabled=0;break;
    default:break; // Source unsigned mode>=5 returns without layout writes.
    }
}

void ecstasyViewAttachCameraMover(CarCinematicServices* services,
    CarCinematicScreen* screen,CarCinematicBase* camera)
{
    camera->previous=screen->last;camera->next=nullptr;
    if (screen->last)screen->last->next=camera;else screen->first=camera;
    screen->last=camera;
    ecstasyViewsUpdateMode(services);
}

void ecstasyViewDetachCameraMover(CarCinematicServices* services,
    CarCinematicScreen* screen,CarCinematicBase* camera)
{
    if (camera->previous)camera->previous->next=camera->next;else screen->first=camera->next;
    if (camera->next)camera->next->previous=camera->previous;else screen->last=camera->previous;
    // Source unlink does not clear the removed node's own links.
    ecstasyViewsUpdateMode(services);
}

void ecstasyViewLocalToScreenClip(const CarCinematicScreen* screen,float output[16],
    const float localToWorld[16])
{
    eMulMatrix(output,localToWorld,screen->matrices.worldToScreenClip);
}

void ecstasyViewLocalToClip(const CarCinematicScreen* screen,float output[16],
    const float localToWorld[16])
{
    eMulMatrix(output,localToWorld,screen->matrices.worldToClip);
}

void ecstasyViewBiasMatrixForZSorting(const CarCinematicScreen* screen,
    float matrix[16],float scale)
{
    if (!screen->view)return;
    float adjustment[16];eIdentity(adjustment);
    const auto& eye=screen->view->current.eye;
    for (int axis=0;axis<4;++axis)
        adjustment[12+axis]=(matrix[12+axis]-eye[axis])*scale+eye[axis];
    adjustment[0]=adjustment[5]=adjustment[10]=scale;adjustment[15]=1.0f;
    matrix[12]=matrix[13]=matrix[14]=0.0f;matrix[15]=1.0f;
    eMulMatrix(matrix,matrix,adjustment);
}

int32_t ecstasyViewPixelSize(const CarCinematicScreen* screen,float size,float distance)
{
    const float result=size*0.5f<distance ? size*screen->pixelScale/distance :
        screen->pixelScale+screen->pixelScale;
    return convertFloatToInt32(result);
}

namespace {
int32_t viewPositionPixelsAtScale(const CarCinematicScreen* screen,
    const float position[3],float radius,float pixelScale)
{
    const auto& camera=screen->view->current;
    const float x=position[0]-camera.eye[0],y=position[1]-camera.eye[1],z=position[2]-camera.eye[2];
    float facing=x*camera.forward[0]+y*camera.forward[1];
    facing=facing+z*camera.forward[2];
    if (facing < -radius)return 0;
    const float distance=std::sqrt((x*x+y*y)+z*z)-radius;
    const float result=radius*0.5f<distance ? radius*pixelScale/distance : pixelScale+pixelScale;
    return convertFloatToInt32(result);
}
}

int32_t ecstasyViewPositionPixelSize(const CarCinematicScreen* screen,
    const float position[3],float radius)
{
    // A discarded source ticker read has no native rendering side effect.
    // Do not issue QueryPerformanceCounter once per object per camera.
    return viewPositionPixelsAtScale(screen,position,radius,screen->pixelScale);
}

int32_t ecstasyViewSourcePositionPixelSize(const CarCinematicScreen* screen,
    const float position[3],float radius)
{
    hp2::Require(screen && screen->view && screen->matricesCalculated &&
        std::isfinite(screen->sourcePixelScale) && screen->sourcePixelScale>0.0f,
        "Authored projected size requires the calculated source focal scale");
    return viewPositionPixelsAtScale(screen,position,radius,screen->sourcePixelScale);
}

int32_t ecstasyViewNativePositionPixelSize(const CarCinematicScreen* screen,
    const float position[3],float radius,int32_t sourceFocal)
{
    hp2::Require(screen && screen->view && screen->matricesCalculated,
        "Native visibility requires a calculated view");
    const float focal=sourceFocal ? screen->sourcePixelScale:screen->pixelScale;
    return viewPositionPixelsAtScale(screen,position,radius,
        focal*hp2::nativeViewDistanceScale(screen->id));
}

int32_t ecstasyViewBoundsPixelSize(const CarCinematicScreen* screen,
    const float first[3],const float second[3])
{
    const float dx=first[0]-second[0],dy=first[1]-second[1],dz=first[2]-second[2];
    const float radius=std::sqrt((dx*dx+dy*dy)+dz*dz)*0.5f;
    const auto& camera=screen->view->current;
    const float x=(first[0]+second[0])*0.5f-camera.eye[0];
    const float y=(first[1]+second[1])*0.5f-camera.eye[1];
    const float z=(first[2]+second[2])*0.5f-camera.eye[2];
    float facing=x*camera.forward[0]+y*camera.forward[1];facing=facing+z*camera.forward[2];
    if (facing < -radius)return 0;
    const float distance=std::sqrt((x*x+y*y)+z*z)-radius;
    // This overload uses radius, rather than radius/2, in the near test.
    float result=screen->pixelScale;
    if (radius<distance)result=radius*result/distance;
    return convertFloatToInt32(result);
}

void ecstasyViewScreenPosition(const CarCinematicScreen* screen,float output[3],
    const float position[3])
{
    const float point[4]={position[0],position[1],position[2],1.0f};
    float transformed[4];eRotTransPers(transformed,screen->matrices.worldToScreen,point);
    const auto* target=screen->target;
    output[2]=transformed[2];
    output[0]=transformed[0]+static_cast<float>(target->width/2);
    output[1]=transformed[1]+static_cast<float>(target->height/2);
}

float ecstasyViewScreenWidth(const CarCinematicScreen* screen)
{
    return static_cast<float>(screen->target->width);
}

float ecstasyViewScreenHeight(const CarCinematicScreen* screen)
{
    return static_cast<float>(screen->target->height);
}

const char* ecstasyViewIdName(int32_t id)
{
    static const char* const names[]={"EVIEW_FLAILER","EVIEW_PLAYER1","EVIEW_PLAYER2",
        "EVIEW_PLAYER1_RVM","EVIEW_ENVMAP0F","EVIEW_ENVMAP0R","EVIEW_ENVMAP0B",
        "EVIEW_ENVMAP0L","EVIEW_ENVMAP0U","EVIEW_ENVMAP0D"};
    if (id==-1)return "EVIEW_NONE";
    return id>=0 && id<10 ? names[id] : "EVIEW_UNKNOWN(?)";
}

void ecstasyViewProjectionInitialize(EcstasyViewProjectionSettings* settings,
    int32_t horizontalResolution)
{
    settings->referenceFieldOfView=16384;settings->splitFieldOfView=9102;
    settings->horizontalResolution=horizontalResolution;
    settings->horizontalClipScale=0.5f;settings->verticalClipScale=0.5f;
    // Native projection uses centre-relative pixels; the GS raster-origin
    // bias is neither part of an asset nor needed by a PC rasterizer.
    settings->horizontalClipOffset=0.0f;settings->verticalClipOffset=0.0f;
    settings->nativeAspectCorrection=0;settings->nativeSourceVideoMode=0;
    // Released projection endpoint values, converted once from their source
    // representation into ordinary numeric depth units. PC rendering uses
    // these values directly and retains D32 interpolation precision.
    const float ranges[2][6]={{1.0f,65280.0f,4.0f,65535.0f,255.0f,65535.0f},
        {1.0f,524288.0f,4.0f,524800.0f,589824.0f,983040.0f}};
    for(std::size_t row=0;row<2;++row)std::copy_n(ranges[row],6,settings->depthRanges[row]);
}

namespace {
float halfAngleTangent(uint16_t fieldOfView)
{
    const auto half=static_cast<uint16_t>(fieldOfView>>1);
    return rotationSineAngle(half)/vectorCosineAngle(half);
}

void packProjection(EcstasyProjectionPair& output,const float matrix[16])
{
    float inverse[16];afxMatrixInverse(inverse,matrix);
    for (int axis=0;axis<3;++axis) {
        output.diagonal[axis]=matrix[axis*5];
        output.inverseDiagonal[axis]=inverse[axis*5];
    }
    for (int axis=0;axis<4;++axis) {
        output.translation[axis]=matrix[12+axis];
        output.inverseTranslation[axis]=inverse[12+axis];
    }
}

void updateTargetContexts(EcstasyRenderTarget& target,const EcstasyViewMatrices& matrices)
{
    const EcstasyProjectionPair* projections[]={&matrices.screenProjection,&matrices.nearProjection,
        &matrices.frustumProjection};
    target.viewMatrices=&matrices;
    for (int index=0;index<3;++index) {
        auto& context=target.contexts[index];
        std::memcpy(context.worldToClip,matrices.worldToClip,sizeof(context.worldToClip));
        context.projection=*projections[index];
        eTransposeMatrix(context.transposedWorldToView,matrices.worldToView);
    }
}
}

CarCinematicResult ecstasyViewCalculateMatrices(CarCinematicServices* services,
    CarCinematicScreen* screen,float nearOverride,float farOverride,float clipFarOverride)
{
    if (!services || !screen || !screen->view || !screen->target ||
        services->projection.horizontalResolution<=0)return CAR_CINEMATIC_INVALID_BINDING;
    const auto& settings=services->projection;
    const auto& camera=screen->view->current;
    auto& target=*screen->target;
    float nearPlane=camera.nearClip,farPlane=camera.farClip;
    float ratio=halfAngleTangent(settings.referenceFieldOfView)/halfAngleTangent(camera.fieldOfView);
    target.fieldOfViewScale=ratio;
    if (screen->auxiliaryTarget)screen->auxiliaryTarget->fieldOfViewScale=ratio;
    // Source GetFirst is the tail of the native oldest-to-newest stack.
    const auto* first=screen->last;
    const bool exempt=first && (first->kind==1 || first->kind==5);
    if ((screen->id==1 || screen->id==2) && !exempt) {
        const float span=farPlane-nearPlane;
        ratio=halfAngleTangent(settings.splitFieldOfView)/halfAngleTangent(camera.fieldOfView);
        nearPlane=nearPlane*ratio;farPlane=nearPlane+span;
    }
    if (nearOverride!=0.0f)nearPlane=nearOverride;
    if (farOverride!=0.0f)farPlane=farOverride;
    // Extend the actual world/mirror frustum, not the physical Camera or its
    // simulation state. Sky overrides and cubemap cameras keep their ranges.
    if(nearOverride==0.0f && farOverride==0.0f)
        farPlane*=hp2::nativeViewDistanceScale(screen->id);
    screen->nearClip=nearPlane;screen->farClip=farPlane;
    const float tangent=halfAngleTangent(camera.fieldOfView);
    screen->pixelScale=tangent!=0.0f ? static_cast<float>(settings.horizontalResolution/2)/tangent : 10.0f;
    screen->sourcePixelScale=screen->pixelScale;
    screen->fieldOfViewDegrees=static_cast<float>(camera.fieldOfView)*0.0054931640625f;
    float rasterScaleX=target.horizontalScale,rasterScaleY=target.verticalScale;
    const bool nativeProjection=settings.nativeAspectCorrection!=0 && screen->id>0;
    if(nativeProjection && tangent!=0.0f) {
        screen->sourcePixelScale=256.0f/tangent;
        // PAL103124..10316C publishes a512-wide source display. Its
        // SetScreenBuffers1064C8 uses a distinct TV-pixel vertical scale:
        // PAL1.285/512 high, NTSC1.0714286566/448 high. Preserve that
        // vertical framing on square PC pixels and reveal additional world
        // horizontally, instead of cropping it as the window gets wider.
        // Camera eye, authored FOV, chase distance and gameplay stay intact.
        if(screen->id==1 || screen->id==2) {
            const bool ntsc=settings.nativeSourceVideoMode==1;
            const float sourceHeight=ntsc?448.0f:512.0f;
            const float sourceScale=ntsc?1.071428656578064f:1.284999966621399f;
            screen->pixelScale=((256.0f/tangent)*sourceScale)*static_cast<float>(target.height)/sourceHeight;
            rasterScaleX=rasterScaleY=1.0f;
            // The source directional-strip shortcut must not reject the
            // additional horizontal field exposed by a widescreen display.
            const float effective=2.0f*std::atan(static_cast<float>(target.width)/(2.0f*screen->pixelScale))*57.29577951308232f;
            screen->fieldOfViewDegrees=std::max(screen->fieldOfViewDegrees,effective);
        } else {
            // Original256x64 mirror and64-square cubemap targets do not
            // grow with the PC window. Their source focal input stays512.
            screen->pixelScale=256.0f/tangent;
            rasterScaleX=screen->id==3?1.5f:1.0f;
            rasterScaleY=screen->id==3?1.0800000429153442f:1.0f;
        }
    }
    std::memcpy(screen->forward,camera.forward,sizeof(screen->forward));
    const float inversePixel=1.0f/screen->pixelScale;
    const auto* range=settings.depthRanges[target.format==0x31 ? 1 : 0];
    const float span=farPlane-nearPlane,product=farPlane*nearPlane;
    const float depthSlope=((farPlane*range[0]+(-range[1])*nearPlane)/span)/screen->pixelScale;
    const float depthOffset=(product*(range[1]-range[0])/span)/screen->pixelScale;
    const float horizontalSlope=((farPlane*range[2]+(-range[3])*nearPlane)/span)/screen->pixelScale;
    const float horizontalOffset=(product*(range[3]-range[2])/span)/screen->pixelScale;
    const float verticalSlope=((farPlane*range[4]+(-range[5])*nearPlane)/span)/screen->pixelScale;
    const float verticalOffset=(product*(range[5]-range[4])/span)/screen->pixelScale;
    const float xOffset=settings.horizontalClipOffset/screen->pixelScale;
    const float yOffset=settings.verticalClipOffset/screen->pixelScale;
    float screenProjection[16]={rasterScaleX,0,0,0,0,rasterScaleY,0,0,
        xOffset,yOffset,depthSlope,inversePixel,0,0,depthOffset,0};
    float nearProjection[16];std::memcpy(nearProjection,screenProjection,sizeof(nearProjection));
    nearProjection[10]=horizontalSlope;nearProjection[14]=horizontalOffset;
    float frustumProjection[16];std::memcpy(frustumProjection,screenProjection,sizeof(frustumProjection));
    frustumProjection[10]=verticalSlope;frustumProjection[14]=verticalOffset;
    const float width=static_cast<float>(target.width),height=static_cast<float>(target.height);
    const float xClip=width*settings.horizontalClipScale,yClip=height*settings.verticalClipScale;
    float xExtent=width*0.5f,yExtent=height*0.5f;
    if (xClip<xExtent)xExtent=xClip;
    if (yClip<yExtent)yExtent=yClip;
    const float nativeScaleX=nativeProjection?rasterScaleX:1.0f;
    const float nativeScaleY=nativeProjection?rasterScaleY:1.0f;
    const float nearX=(nearPlane*xClip)/(screen->pixelScale*nativeScaleX);
    const float nearY=(nearPlane*yClip)/(screen->pixelScale*nativeScaleY);
    float clip[16];eIdentity(clip);
    clip[0]=(nearPlane+nearPlane)/(nearX+nearX);
    clip[5]=(nearPlane+nearPlane)/(nearY+nearY);
    clip[10]=(farPlane+nearPlane)/span;clip[11]=1.0f;
    clip[14]=(product*-2.0f)/span;clip[15]=0.0f;
    float inverse[16];eIdentity(inverse);
    inverse[0]=1.0f/clip[0];inverse[5]=1.0f/clip[5];
    inverse[10]=0.0f;inverse[11]=1.0f/clip[14];
    inverse[14]=1.0f;inverse[15]=(-clip[10])/clip[14];
    float clipToScreen[16],clipToNear[16],clipToFrustum[16];
    eMulMatrix(clipToScreen,inverse,screenProjection);
    eMulMatrix(clipToNear,inverse,nearProjection);
    eMulMatrix(clipToFrustum,inverse,frustumProjection);
    float screenClip[16];eIdentity(screenClip);
    const float clipFar=clipFarOverride!=0.0f ? clipFarOverride : farPlane;
    const float clipX=nearPlane*xExtent/(screen->pixelScale*nativeScaleX);
    const float clipY=nearPlane*yExtent/(screen->pixelScale*nativeScaleY);
    screenClip[0]=(nearPlane+nearPlane)/(clipX+clipX);
    screenClip[5]=(nearPlane+nearPlane)/(clipY+clipY);
    screenClip[10]=(clipFar+nearPlane)/(clipFar-nearPlane);screenClip[11]=1.0f;
    screenClip[14]=((clipFar*nearPlane)*-2.0f)/(clipFar-nearPlane);screenClip[15]=0.0f;
    // Final10EBF4 multiplication is observable in its float rounding.
    eMulMatrix(screenProjection,clip,clipToScreen);
    auto& matrices=screen->matrices;
    std::memcpy(matrices.worldToView,camera.matrix,sizeof(matrices.worldToView));
    std::memcpy(matrices.clipToScreen,clipToScreen,sizeof(matrices.clipToScreen));
    eMulMatrix(matrices.worldToClip,matrices.worldToView,clip);
    eMulMatrix(matrices.worldToScreen,matrices.worldToView,screenProjection);
    eMulMatrix(matrices.worldToScreenClip,matrices.worldToView,screenClip);
    packProjection(matrices.screenProjection,clipToScreen);
    packProjection(matrices.nearProjection,clipToNear);
    packProjection(matrices.frustumProjection,clipToFrustum);
    updateTargetContexts(target,matrices);
    if (screen->auxiliaryTarget)updateTargetContexts(*screen->auxiliaryTarget,matrices);
    screen->matricesCalculated=1;
    return CAR_CINEMATIC_OK;
}

uint32_t hashBytes(const void* bytes, size_t length)
{
    const uint8_t* source = (const uint8_t*)bytes;
    uint32_t value = UINT32_MAX;
    for (size_t i = 0; i < length; ++i)
        value = value * 33u + source[i];
    return value;
}

CarCinematicScreen* screenRadialBlurScreen(ScreenRadialBlur* radialBlur, int32_t seat)
{
    if (!radialBlur || !radialBlur->cameras || seat < 0 || seat >= 3) return NULL;
    const GameSession* session = radialBlur->cameras->session;
    if (session && session->state == 4 && session->viewSelection) {
        if (seat != session->viewSelection->seat) return NULL;
        return &radialBlur->cameras->screens[1];
    }
    return &radialBlur->cameras->screens[seat + 1];
}

void screenRadialBlurStartScreen(ScreenRadialBlur* radialBlur, CarCinematicScreen* screen)
{
    if (!radialBlur || !radialBlur->cameras || !screen) return;
    const GameSession* session = radialBlur->cameras->session;
    if (session && session->state == 7) return;
    const ptrdiff_t index = screen - radialBlur->cameras->screens - 1;
    if (index < 0 || index >= 4) return;
    screen->radialBlurTexture = radialBlur->texture;
    radialBlur->firstAmount[index] = 0;
    radialBlur->secondAmount[index] = 160;
}

void screenRadialBlurStart(ScreenRadialBlur* radialBlur, int32_t seat)
{
    CarCinematicScreen* screen = screenRadialBlurScreen(radialBlur, seat);
    if (screen) screenRadialBlurStartScreen(radialBlur, screen);
}

void screenRadialBlurStop(ScreenRadialBlur* radialBlur, int32_t seat)
{
    CarCinematicScreen* screen = screenRadialBlurScreen(radialBlur, seat);
    if (screen) screenRadialBlurStopScreen(screen);
}

void screenRadialBlurSetAmounts(ScreenRadialBlur* radialBlur, int32_t seat, float first, float second)
{
    if (!radialBlur || seat < 0 || seat >= 4) return;
    radialBlur->firstAmount[seat] = (uint8_t)(int32_t)(first * 255.0f);
    radialBlur->secondAmount[seat] = (uint8_t)(int32_t)(second * 255.0f);
}

// PAL10F1A8..10F1B0: revoke the borrowed source texture when blur stops.
void screenRadialBlurStopScreen(CarCinematicScreen* screen)
{
    screen->radialBlurTexture = nullptr;
}

void editorMarkerSolidBounds(const EditorMarkerSolid* solid,float minimum[3],float maximum[3])
{
    const float minimumX=solid->minimum[0];
    const float minimumY=solid->minimum[1];
    const float minimumZ=solid->minimum[2];
    const float maximumZ=solid->maximum[2];
    const float maximumX=solid->maximum[0];
    const float maximumY=solid->maximum[1];
    minimum[2]=minimumZ;
    minimum[0]=minimumX;
    minimum[1]=minimumY;
    maximum[2]=maximumZ;
    maximum[0]=maximumX;
    maximum[1]=maximumY;
}

int32_t ecstasyClassifyBounds(const float minimum[3],const float maximum[3],const float matrix[16])
{
#if defined(_M_X64) || defined(__SSE2__)
    // Native x64 SIMD evaluates the same four component lanes together.
    // Keep the original base + X/Y/Z edge order (including negative W and
    // boundary rounding); replacing it with extracted planes is not equivalent.
    const auto mx=_mm_loadu_ps(matrix),my=_mm_loadu_ps(matrix+4),mz=_mm_loadu_ps(matrix+8);
    const auto first=_mm_add_ps(_mm_mul_ps(mx,_mm_set1_ps(minimum[0])),
        _mm_mul_ps(my,_mm_set1_ps(minimum[1])));
    const auto base=_mm_add_ps(_mm_add_ps(first,_mm_mul_ps(mz,_mm_set1_ps(minimum[2]))),
        _mm_loadu_ps(matrix+12));
    const auto xStep=_mm_mul_ps(mx,_mm_set1_ps(maximum[0]-minimum[0]));
    const auto yStep=_mm_mul_ps(my,_mm_set1_ps(maximum[1]-minimum[1]));
    const auto zStep=_mm_mul_ps(mz,_mm_set1_ps(maximum[2]-minimum[2]));
    const __m128 corners[4]={base,_mm_add_ps(base,xStep),
        _mm_add_ps(_mm_add_ps(base,xStep),yStep),_mm_add_ps(base,yStep)};
    const auto sign=_mm_set1_ps(-0.0f);
    std::uint32_t any=0,common=63;
    for(int layer=0;layer<2;++layer)for(int corner=0;corner<4;++corner) {
        const auto point=layer ? _mm_add_ps(corners[corner],zStep):corners[corner];
        const auto limit=_mm_andnot_ps(sign,_mm_shuffle_ps(point,point,_MM_SHUFFLE(3,3,3,3)));
        const auto positive=static_cast<std::uint32_t>(_mm_movemask_ps(_mm_cmpgt_ps(point,limit)))&7u;
        const auto negative=static_cast<std::uint32_t>(_mm_movemask_ps(_mm_cmplt_ps(point,_mm_xor_ps(limit,sign))))&7u;
        const auto flags=positive|(negative<<3u);
        any|=flags;common&=flags;
        // Once two corners disagree, later corners cannot restore a common
        // outside plane or remove an already-observed outside component.
        if(any && !common)return 1;
    }
    return any==0 ? 2:common==0 ? 1:0;
#else
    float base[4],xStep[4],yStep[4],zStep[4];
    const float x=maximum[0]-minimum[0],y=maximum[1]-minimum[1],z=maximum[2]-minimum[2];
    for (int axis=0;axis<4;++axis) {
        const float first=matrix[axis]*minimum[0]+matrix[4+axis]*minimum[1];
        base[axis]=(first+matrix[8+axis]*minimum[2])+matrix[12+axis];
        xStep[axis]=matrix[axis]*x;yStep[axis]=matrix[4+axis]*y;zStep[axis]=matrix[8+axis]*z;
    }
    // Source constructs a base corner and three edge deltas, then clips
    // the eight additions. Re-transforming eight positions changes rounding.
    float corners[4][4];
    for (int axis=0;axis<4;++axis) {
        corners[0][axis]=base[axis];
        corners[1][axis]=base[axis]+xStep[axis];
        corners[2][axis]=corners[1][axis]+yStep[axis];
        corners[3][axis]=base[axis]+yStep[axis];
    }
    uint32_t any=0,common=63;
    for (int layer=0;layer<2;++layer)for (int corner=0;corner<4;++corner) {
        float point[4];
        for (int axis=0;axis<4;++axis)point[axis]=layer ? corners[corner][axis]+zStep[axis] : corners[corner][axis];
        const float limit=std::fabs(point[3]);uint32_t flags=0;
        for (int axis=0;axis<3;++axis) {
            if (point[axis]>limit)flags|=1u<<(axis*2);
            if (point[axis]<-limit)flags|=2u<<(axis*2);
        }
        any|=flags;common&=flags;
    }
    return any==0 ? 2 : common==0 ? 1 : 0;
#endif
}

namespace {
bool stripFacingVisible(const EditorMarkerSolidGeometry::Strip& strip,const float facing[4],
    float halfFieldOfView,bool forceMode)
{
    const float x=static_cast<float>(strip.cullDirectionXY[0])*0.00006103515625f;
    const float y=static_cast<float>(strip.cullDirectionXY[1])*0.00006103515625f;
    const float z=static_cast<float>(strip.autoKProjection)*0.00006103515625f;
    float dot=x*facing[0]+y*facing[1];dot=dot+z*facing[2];
    const auto sineAngle=vectorArcSineAngle(dot);
    const auto angle=static_cast<uint16_t>(16384u-sineAngle);
    const float degrees=static_cast<float>(angle)*0.0054931640625f;
    const int32_t authoredAngle=strip.autoKAngle<32768u ? strip.autoKAngle :
        static_cast<int32_t>(strip.autoKAngle)-65536;
    // Actual10D1C8 scales the unsigned angle in mode2.10D2D8 instead
    // passes the signed lane without this scale in mode1/3.
    const float cone=forceMode ? static_cast<float>(strip.autoKAngle)*0.0054931640625f :
        static_cast<float>(authoredAngle);
    const float threshold=(90.0f-halfFieldOfView)-cone;
    return !(degrees<threshold);
}

void classifyStrips(std::vector<EcstasyVisibleStrip>& output,const EditorMarkerModel& model,
    const CarCinematicScreen& screen,const float localToClip[16],const float localToWorld[16],
    int32_t mode,bool angular)
{
    const auto& strips=model.solid->geometry->strips;
    float facing[4]{};
    if (angular) {
        // PAL10D0AC..10D0F0 transforms the world direction by the general
        // inverse. Scaled and quantized scenery matrices need not be orthogonal.
        float inverse[16];afxMatrixInverse(inverse,localToWorld);
        const float negativeForward[4]={-screen.forward[0],-screen.forward[1],-screen.forward[2],0};
        eMulVector4(facing,inverse,negativeForward);
    }
    if (mode!=1 && mode!=2 && mode!=3)return;
    const int32_t intersectClass=mode==3 ? 2 : mode;
    for (std::size_t index=0;index<strips.size();++index) {
        const auto& strip=strips[index];
        int32_t classification=2;
        if (mode!=2) {
            classification=ecstasyClassifyBounds(strip.minimum.data(),strip.maximum.data(),localToClip);
            if (classification==1)classification=intersectClass;
        }
        if (!classification)continue;
        if (angular && (strip.flags&0x1000u) &&
            !stripFacingVisible(strip,facing,screen.fieldOfViewDegrees*0.5f,mode==2))continue;
        // The final loop never assigns its previous-class register after
        // resetting it to-1. Valid classifications0/1/2 therefore publish
        // separate one-strip runs; merging them would change Source output.
        output.push_back({index,1,classification});
    }
}
}

bool ecstasyDetermineVisibleStrips(std::vector<EcstasyVisibleStrip>& output,
    const EditorMarkerModel& model,const CarCinematicScreen& screen,
    const float localToWorld[16],bool forceVisible,int32_t mode)
{
    output.clear();
    if (!model.solid)return false;
    hp2::Require(model.solid->geometry!=nullptr,"Visible model has no same-owner decoded geometry");
    const auto count=model.solid->geometry->strips.size();
    const auto all=[&](int32_t classification) {
        if (count)output.push_back({0,static_cast<uint16_t>(count),classification});
    };
    float screenClip[16],clip[16];
    if (mode==2) {
        if (model.solid->renderFlags&0x1000u) {
            ecstasyViewLocalToClip(&screen,clip,localToWorld);
            classifyStrips(output,model,screen,clip,localToWorld,2,true);
        } else all(2);
    } else {
        ecstasyViewLocalToScreenClip(&screen,screenClip,localToWorld);
        int32_t classification=ecstasyClassifyBounds(model.solid->minimum,model.solid->maximum,screenClip);
        if (!classification)return false;
        if (classification==1) {
            ecstasyViewLocalToClip(&screen,clip,localToWorld);
            classification=ecstasyClassifyBounds(model.solid->minimum,model.solid->maximum,clip);
        }
        if (forceVisible || classification==2)all(classification);
        else if (classification==1) {
            const bool angular=(model.solid->renderFlags&0x1000u)!=0;
            classifyStrips(output,model,screen,angular ? clip : screenClip,localToWorld,1,angular);
        }
    }
    return !output.empty();
}

void editorMarkerModelBounds(const EditorMarkerModel* model,float minimum[3],float maximum[3])
{
    if (model->solid) editorMarkerSolidBounds(model->solid,minimum,maximum);
}

const EditorMarkerRigidAnimation* editorMarkerModelRigidAnimation(const EditorMarkerModel* model)
{
    return model->solid ? model->solid->rigidAnimation : nullptr;
}

void editorMarkerModelSetRigidAnimationFrame(EditorMarkerModel* model,int32_t frame,int32_t worldFrames)
{
    const uint32_t start = (static_cast<uint32_t>(worldFrames) -
        static_cast<uint32_t>(frame) * 2u) & 0x3fffu;
    model->rigidAnimationStart = static_cast<uint16_t>(start | 0x8000u);
}

void editorMarkerModelSetupVertexAnimation(EditorMarkerModel* model,int32_t worldFrames)
{
    if(!model->solid || !model->solid->vertexAnimation)return;
    const auto& animation=*model->solid->vertexAnimation;
    hp2::Require(animation.count>0 && animation.count<=64 && animation.framesPerSecond>0,
        "Vertex animation has no authored frame sequence");
    // PAL10DB70..10DBCC selects discrete geometry at30fps from the60Hz
    // world clock. The low14-bit start is shared with rigid animation. Use
    // native64-bit time arithmetic rather than a console multiply/divider;
    // preserve the source unsigned remainder choice for a negative start.
    const auto ticks=static_cast<int64_t>(worldFrames)-static_cast<int64_t>(model->rigidAnimationStart&0x3fffu);
    const auto sample=(ticks*static_cast<int64_t>(animation.framesPerSecond))/60;
    const auto remainder=sample%static_cast<int64_t>(animation.count);
    const auto index=static_cast<uint32_t>(remainder)%animation.count;
    hp2::Require(animation.frames[index]!=nullptr,"Vertex animation frame is unowned");
    model->solid=animation.frames[index];
}

int32_t editorMarkerModelRigidAnimationTime(const EditorMarkerModel* model,int32_t worldFrames)
{
    const uint16_t start = model->rigidAnimationStart;
    if (!start) return -1;
    const EditorMarkerRigidAnimation* animation = editorMarkerModelRigidAnimation(model);
    if (!animation || animation->frameCount <= 0 || animation->frameDuration <= 0) return -1;
    const int32_t time = static_cast<int32_t>((static_cast<uint32_t>(worldFrames) - start) & 0x3fffu);
    const int32_t duration = animation->frameCount * animation->frameDuration;
    if (start & 0x4000u) return time % duration;
    return time < duration ? time : -1;
}

#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <map>
#include <string>

namespace hp2 {
namespace {
std::string text(const std::uint8_t* bytes,std::size_t capacity)
{
    const auto* end=std::find(bytes,bytes+capacity,std::uint8_t(0));
    Require(end!=bytes+capacity,"Editor asset name is not terminated");
    return {reinterpret_cast<const char*>(bytes),static_cast<std::size_t>(end-bytes)};
}

float real(const std::uint8_t* bytes)
{
    const std::uint32_t bits=ReadU32(bytes);
    float value;
    std::memcpy(&value,&bits,sizeof(value));
    Require(std::isfinite(value),"Editor model has a non-finite bound");
    return value;
}

std::uint64_t decodeStripDrawDisableMask(const std::uint8_t* packet,std::size_t count)
{
    // Final PAL vuiReferenceStripper2A76E0 and its two transform workers
    // transport header X/Z/W through RINIT/RNEXT into XYZF2.W bit15 (ADC).
    // Y belongs to texture state. A reseed follows three already computed
    // position-W lanes; the native format decoder preserves that ordering.
    Require(count>0 && count<=48,"Solid strip exceeds the original 48-vertex packet schedule");
    const auto padded=(count+3u)&~std::size_t(3u);
    Require(packet[5]==padded/4u-1u,"Solid strip packet loop count disagrees with its vertex extent");
    const auto prefix=padded%16u;
    const std::size_t firstLane=(padded<16u || prefix) ? 0u:(padded==32u ? 2u:(padded==48u ? 0u:3u));
    auto random=ReadU32(packet+12+firstLane*4)&0x7fffffu;
    std::uint64_t mask{};
    for (std::size_t vertex=0;vertex<count;++vertex) {
        if (padded>32u && vertex==(prefix ? prefix:16u)+3u)
            random=ReadU32(packet+20)&0x7fffffu;
        if (padded>16u && vertex==padded-16u+3u)
            random=ReadU32(packet+24)&0x7fffffu;
        random=((random<<1)^((random>>4)&1u)^((random>>22)&1u))&0x7fffffu;
        mask|=std::uint64_t((random>>15)&1u)<<vertex;
    }
    return mask;
}

std::unique_ptr<EditorMarkerSolidGeometry> decodeSolidGeometry(const std::vector<std::uint8_t>& bytes,
    const std::uint8_t* header,const Chunk& table,const Chunk& packets,const Chunk& textures,const Chunk& materials)
{
    auto geometry=std::make_unique<EditorMarkerSolidGeometry>();
    geometry->vertexCount=static_cast<std::int16_t>(ReadU16(header+0x38));
    Require(geometry->vertexCount>=0,"Solid vertex count is outside its signed extent");
    if (textures.id) {
        Require(textures.size%8==0,"Solid texture table has a partial binding");
        for (std::size_t offset=0;offset<textures.size;offset+=8)
            geometry->textures.push_back({ReadU32(bytes.data()+textures.payloadOffset+offset),nullptr});
    }
    const auto strips=ReadU16(header+0x34);
    Require(strips<=32767,"Solid strip count is outside its signed extent");
    if (!strips)return geometry;
    Require(table.id && packets.id,"Solid strips are missing their real packet streams");
    const auto tableBegin=(table.payloadOffset+15u)&~std::size_t(15u);
    const auto packetBegin=(packets.payloadOffset+15u)&~std::size_t(15u);
    Require(tableBegin<=table.endOffset && std::size_t(strips)*64<=table.endOffset-tableBegin,
        "Solid strip table is truncated");
    const auto read=[&](std::size_t offset) {
        Require(offset<=packets.endOffset && packets.endOffset-offset>=4,"Solid vertex stream is truncated");
        return real(bytes.data()+offset);
    };
    for (std::size_t index=0;index<strips;++index) {
        const auto* record=bytes.data()+tableBegin+index*64;
        EditorMarkerSolidGeometry::Strip strip{};
        std::memcpy(&strip.textureIndex,record,1);
        for (std::size_t axis=0;axis<3;++axis) {
            strip.minimum[axis]=real(record+0x10+axis*4);
            strip.maximum[axis]=real(record+0x20+axis*4);
        }
        strip.flags=ReadU16(record+0xe);
        for (std::size_t axis=0;axis<2;++axis) {
            const auto word=ReadU16(record+0x30+axis*2);
            std::memcpy(&strip.cullDirectionXY[axis],&word,sizeof(word));
        }
        strip.textureScales={ReadU16(record+4),ReadU16(record+6)};
        const auto projection=ReadU16(record+0x34);std::memcpy(&strip.autoKProjection,&projection,2);
        strip.autoKAngle=ReadU16(record+0x36);strip.autoKScale=real(record+0x38);
        const auto packet=packetBegin+ReadU32(record+8);
        Require(packet>=packetBegin && packet<=packets.endOffset && packets.endOffset-packet>=28,
            "Solid strip points outside its retained packet chunk");
        const std::size_t count=bytes[packet+4];
        Require(count==record[0x1d],"Strip and packet vertex counts disagree");
        strip.drawDisableMask=decodeStripDrawDisableMask(bytes.data()+packet,count);
        std::size_t emittedPolygons{};
        for (std::size_t vertex=2;vertex<count;++vertex)
            if (!(strip.drawDisableMask&(std::uint64_t(1)<<vertex)))++emittedPolygons;
        Require(emittedPolygons==record[0x1e],"Solid strip ADC topology disagrees with its authored polygon count");
        // RenderStrips10E01C reads the signed material selector stored in
        // the strip header's otherwise unused bounds-W lane, not VIFbyte7.
        std::memcpy(&strip.lightMaterialIndex,record+0x1f,1);
        strip.positions.resize(count);strip.textureCoordinates.resize(count);strip.colors.resize(count);
        const auto fullPositions=(count/4)*4;
        const auto stBegin=packet+28+count*12+4+(count>=4 && count%4 ? 4 : 0);
        const auto colorBegin=stBegin+count*8+8+(count>=2 && count%2 ? 4 : 0);
        Require(colorBegin<=packets.endOffset && count*2<=packets.endOffset-colorBegin,
            "Solid RGB555 stream is truncated");
        for (std::size_t vertex=0;vertex<count;++vertex) {
            auto xyz=packet+28;
            const auto lane=vertex%4;
            if (vertex<fullPositions) {
                xyz+=(vertex/4)*48+4;
                for (std::size_t axis=0;axis<3;++axis)
                    strip.positions[vertex][axis]=read(xyz+axis*16+lane*4);
            } else {
                if (count>=4)xyz+=(vertex/4)*48+4;
                xyz+=4;
                for (std::size_t axis=0;axis<3;++axis)
                    strip.positions[vertex][axis]=read(xyz+(axis*(count%4)+lane)*4);
            }
            std::size_t s,t;
            if (vertex<(count/2)*2) {
                const auto at=stBegin+(vertex/2)*16+4;
                s=at+(vertex%2 ? 12 : 0);t=at+(vertex%2 ? 8 : 4);
            } else {
                s=stBegin+(count>=2 ? (vertex/2)*16+4 : 0)+4;t=s+4;
            }
            strip.textureCoordinates[vertex]={read(s),read(t)};
            strip.colors[vertex]=ReadU16(bytes.data()+colorBegin+vertex*2);
        }
        if(strip.flags&0x1c0u) {
            const auto normalCommand=(colorBegin+count*2+3u)&~std::size_t(3u);
            Require(normalCommand<=packets.endOffset && packets.endOffset-normalCommand>=4,
                "Lit solid strip has no authored normal stream");
            const auto command=ReadU32(bytes.data()+normalCommand);
            const auto vectors=3u*((count+3u)/4u);
            Require((command&0xff00ffffu)==0x6e00805cu && ((command>>16)&255u)==vectors,
                "Solid normal stream differs from released signed V4_8 TOPS+5C");
            Require(vectors*4<=packets.endOffset-normalCommand-4,
                "Solid signed normal payload is truncated");
            strip.normals.resize(count);
            constexpr float normalScale=0.007874015718698502f; // PAL VU immediate3C010204
            for(std::size_t vertex=0;vertex<count;++vertex)
                for(std::size_t axis=0;axis<3;++axis) {
                    std::int8_t value;
                    std::memcpy(&value,bytes.data()+normalCommand+4+(vertex/4)*12+axis*4+vertex%4,1);
                    strip.normals[vertex][axis]=static_cast<float>(value)*normalScale;
                }
        }
        // Keep the original decoded strip immutable and prepare its native
        // index topology once, not once per camera, lighting or alpha pass.
        strip.PrepareNativeTopology();
        geometry->strips.push_back(std::move(strip));
    }
    std::size_t decodedVertices=0;
    for (const auto& strip:geometry->strips)decodedVertices+=strip.positions.size();
    Require(decodedVertices==static_cast<std::size_t>(geometry->vertexCount),
        "Solid vertex header does not match its decoded strips");
    const auto count=ReadU16(header+0xae);
    if (count) {
        Require(materials.id!=0,"Solid light-material table is missing");
        const auto start=(materials.payloadOffset+15u)&~std::size_t(15u);
        Require(start<=materials.endOffset && std::size_t(count)*8<=materials.endOffset-start,
            "Solid light-material bindings are truncated");
        for (std::size_t index=0;index<count;++index)
            geometry->lightMaterialKeys.push_back(ReadU32(bytes.data()+start+index*8));
    }
    return geometry;
}
} // namespace

TrackRouteEditorResources::TrackRouteEditorResources(const std::vector<std::uint8_t>& ingame,
    EditorMarkerResources* registry)
    : authored_(ingame)
{
    std::map<std::uint32_t,std::uint32_t> inheritedAnimations;
    std::vector<std::pair<std::uint32_t,std::size_t>> textureChunks;
    const auto visit=[&](const auto& self,std::size_t begin,std::size_t size,unsigned depth)->void {
        Require(depth<=32,"Editor asset nesting exceeds 32 levels");
        for(const auto& chunk:ReadChunks(authored_,begin,size)){
            if(chunk.id==0x80034002u){
                Chunk header{},rigid{},labels{},stripTable{},stripPackets{},textureTable{},materials{},tree{};
                for(const auto& child:ReadChunks(authored_,chunk.payloadOffset,chunk.size)) {
                    if(child.id==0x34003u){Require(!header.id,"Editor model repeats its header");header=child;}
                    else if(child.id==0x34012u){Require(!rigid.id,"Model repeats its rigid animation");rigid=child;}
                    else if(child.id==0x34011u){Require(!labels.id,"Model repeats its frame labels");labels=child;}
                    else if(child.id==0x34004u){Require(!stripTable.id,"Model repeats its strip table");stripTable=child;}
                    else if(child.id==0x34005u){Require(!stripPackets.id,"Model repeats its strip packets");stripPackets=child;}
                    else if(child.id==0x34006u){Require(!textureTable.id,"Model repeats its texture table");textureTable=child;}
                    else if(child.id==0x3401du){Require(!materials.id,"Model repeats its light-material table");materials=child;}
                    else if(child.id==0x34007u){Require(!tree.id,"Model repeats its animation tree");tree=child;}
                }
                Require(header.id!=0,"Editor model has no header");
                const std::size_t start=(header.payloadOffset+15u)&~std::size_t(15u);
                Require(start<=header.endOffset&&header.endOffset-start>=192,"Editor model header is truncated");
                const auto* record=authored_.data()+start;
                const auto name=text(record+16,24);
                const auto key=ReadU32(record+8);
                // Shipped scenery stores truncated display names. The
                // authored key, rather than a hash of that name, is the
                // lookup identity. Repeated keys retain source load order.
                (void)name;
                EditorMarkerSolid model{};
                model.key=key;
                std::memcpy(model.name,record+16,sizeof(model.name));
                model.resourceFlags=ReadU16(record+0x30);
                model.renderFlags=ReadU16(record+0x32);
                model.authoredGeometry=authored_.data()+chunk.payloadOffset;
                model.authoredGeometrySize=chunk.size;
                for(int axis=0;axis<3;++axis){model.minimum[axis]=real(record+64+axis*4);model.maximum[axis]=real(record+80+axis*4);}
                for(int axis=0;axis<3;++axis)model.origin[axis]=real(record+0x90+axis*4);
                for(int lane=0;lane<16;++lane)model.positionMatrix[lane]=real(record+0x60+lane*4);
                auto geometry=decodeSolidGeometry(authored_,record,stripTable,stripPackets,textureTable,materials);
                model.geometry=geometry.get();geometry_.push_back(std::move(geometry));
                // Actual LoaderSolidList10389C derives the embedded tree from
                // its aligned payload/96. The leaf10CA80 exposes this owner.
                // A static model without34007 retains its authored zero count.
                const auto authoredNodeCount=ReadU32(record+0xa0);
                Require(authoredNodeCount<=static_cast<std::uint32_t>(INT32_MAX),"Animation tree count exceeds signed range");
                model.animationTree.nodeCount=static_cast<std::int32_t>(authoredNodeCount);
                if (tree.id) {
                    const auto nodes=(tree.payloadOffset+15u)&~std::size_t(15u);
                    Require(nodes<=tree.endOffset,"Animation tree alignment exceeds payload");
                    const auto bytes=tree.endOffset-nodes;
                    Require(bytes/96<=static_cast<std::size_t>(INT32_MAX),"Animation tree node count exceeds signed range");
                    model.animationTree.nodeCount=static_cast<std::int32_t>(bytes/96);
                    model.animationTree.authoredNodes=authored_.data()+nodes;
                    model.animationTree.authoredBytes=bytes;
                } else Require(!authoredNodeCount,"Animation tree has no retained node payload");
                if (rigid.id) {
                    const std::size_t animationStart=(rigid.payloadOffset+15u)&~std::size_t(15u);
                    Require(animationStart<=rigid.endOffset && rigid.endOffset-animationStart>=48,"Rigid animation header is truncated");
                    const auto* data=authored_.data()+animationStart;
                    auto animation=std::make_unique<RigidAnimationStorage>();
                    const uint16_t count=ReadU16(data);
                    std::memcpy(&animation->native.frameCount,&count,sizeof(count));
                    std::memcpy(&animation->native.frameDuration,data+3,1);
                    std::memcpy(&animation->native.playbackType,data+2,1);
                    animation->native.key=ReadU32(data+4);
                    Require(animation->native.frameCount>0 && animation->native.frameDuration>0,
                        "Rigid animation dimensions must be positive");
                    const std::size_t lanes=std::size_t(animation->native.frameCount)*16u;
                    Require(lanes<=(rigid.endOffset-animationStart-48)/4,"Rigid animation matrix stream is truncated");
                    animation->matrices.resize(lanes);
                    for(std::size_t lane=0;lane<lanes;++lane)animation->matrices[lane]=real(data+48+lane*4);
                    if(labels.id){
                        Require(labels.size%2==0,"Rigid animation label table has a partial element");
                        animation->labels.resize(labels.size/2);
                        for(std::size_t i=0;i<animation->labels.size();++i)
                            animation->labels[i]=ReadU16(authored_.data()+labels.payloadOffset+i*2);
                    }
                    animation->native.matrices=animation->matrices.data();
                    animation->native.labels=animation->labels.data();
                    animation->native.labelCount=animation->labels.size();
                    model.rigidAnimation=&animation->native;
                    rigidAnimations_.push_back(std::move(animation));
                }
                if (ReadU32(record+40)) inheritedAnimations.emplace(key,ReadU32(record+40));
                solids_.push_back(model);
            }else if(chunk.id==0xb0300001u || chunk.id==0xb0300100u){
                textureChunks.emplace_back(chunk.id,chunk.headerOffset);
            }else if(chunk.IsContainer())self(self,chunk.payloadOffset,chunk.size,depth+1);
        }
    };
    visit(visit,0,authored_.size(),0);
    native_.solids=solids_.data();native_.solidCount=solids_.size();
    if(registry)Require(editorMarkerResourcesAttach(registry,&native_),"Model catalogue is already registered");
    try {
    // The real texture class owns each retained descriptor and palette.
    // Do not decode another flat descriptor copy in this model resource.
    for (const auto& [kind,offset]:textureChunks) {
        if(kind==0xb0300001u)
            texturePacks_.push_back(std::make_unique<TexturePack>(texturePackRegistry,
                registry?*registry:native_,authored_,offset));
        else {
            // LoaderTexturePack100910 accepts version1 only; other versions
            // remain an explicitly uninstantiated authored resource.
            Require(offset<=authored_.size() && authored_.size()-offset>=20,
                "Texture animation pack header is truncated");
            if(ReadU32(authored_.data()+offset+16)==1)
                textureAnimationPacks_.push_back(std::make_unique<TextureAnimPack>(texturePackRegistry,
                    registry?*registry:native_,authored_,offset));
        }
    }
    lightMaterials_=std::make_unique<LightMaterialResources>(authored_,registry?*registry:native_);
    auto* root=registry?registry:&native_;
    if (root->catalogueRoot)root=root->catalogueRoot;
    for (auto& solid:solids_) {
        auto& geometry=*solid.geometry;
        if (root->resourceRegistry)geometry.FixTextureTable(*root->resourceRegistry);
        geometry.lightMaterials.resize(geometry.lightMaterialKeys.size());
        for (std::size_t index=0;index<geometry.lightMaterialKeys.size();++index) {
            auto* found=root->defaultLightMaterial;
            for (auto* catalogue=root->lightMaterialCatalogues;catalogue;catalogue=catalogue->next) {
                bool matched=false;
                for (const auto& material:catalogue->materials)
                    if (material.key==geometry.lightMaterialKeys[index]) {found=&material;matched=true;break;}
                if (matched)break;
            }
            geometry.lightMaterials[index]=found;
        }
    }
    for (auto& solid:solids_) {
        const auto inherited=inheritedAnimations.find(solid.key);
        if(inherited==inheritedAnimations.end())continue;
        const auto* source=findModelSolid(registry ? registry:&native_,inherited->second);
        if(source)solid.rigidAnimation=source->rigidAnimation;
    }
    // PAL103950..1039EC: identical0x10 solid keys form one ordered sequence,
    // at30fps with at most64 members; later members share the first rigid
    // animation. Native groups stay within their actual resource lifetime.
    // Concurrent/reloaded catalogues must never retain each other's solids.
    std::map<std::uint32_t,EditorMarkerVertexAnimation*> variants;
    for(auto& solid:solids_)if(solid.resourceFlags&0x10u) {
        auto& group=variants[solid.key];
        if(!group) {
            auto sequence=std::make_unique<EditorMarkerVertexAnimation>();
            sequence->framesPerSecond=30;group=sequence.get();
            vertexAnimations_.push_back(std::move(sequence));
        }else solid.rigidAnimation=group->frames[0]->rigidAnimation;
        if(group->count<64)group->frames[group->count++]=&solid;
        solid.vertexAnimation=group;
    }
    native_.solids=solids_.data();native_.solidCount=solids_.size();
    } catch (...) {
        editorMarkerResourcesDetach(&native_);
        throw;
    }
}

TrackRouteEditorResources::~TrackRouteEditorResources()
{
    if (!native_.catalogueRoot)ecstasyDeleteAllDuplicatedSolids(&native_);
    editorMarkerResourcesDetach(&native_);
}

} // namespace hp2

// Native counterpart of the counter producer in PAL eVblCallback111F58.
// GS FIELD, DMA page swaps and debug timing are platform-specific and are not
// claimed as a completed native eVblCallback. The Vulkan producer is still a
// required integration gate; this counter advances only on an explicit event.
namespace { std::atomic<std::uint32_t> graphicsVideoBlankCounter{0}; }
std::uint32_t graphicsVBlankCount() noexcept {
    return graphicsVideoBlankCounter.load(std::memory_order_acquire);
}

namespace hp2 {
namespace {
// Final2C79C0, indexed by the same GameTime mode as source GetVideoMode.
constexpr std::array<std::array<std::int32_t,2>,2> defaultDisplayOffsets{{{{56,27}},{{0,0}}}};
std::array<std::array<std::int32_t,2>,2> displayOffsets=defaultDisplayOffsets;
std::size_t DisplayModeIndex(const GameTime& clock)
{
    const auto mode=gameTimeGetVideoMode(&clock);
    Require(mode==HP2_VIDEO_PAL || mode==HP2_VIDEO_NTSC,"Display calibration has an invalid actual video mode");
    return static_cast<std::size_t>(mode);
}
}
void eSetScreenDisplayOffsets(const GameTime& clock,std::int32_t x,std::int32_t y)
{
    // Original setter stores signed words without a menu clamp.
    displayOffsets[DisplayModeIndex(clock)]={{x,y}};
}
void eGetScreenDisplayOffsets(const GameTime& clock,std::int32_t& x,std::int32_t& y)
{
    const auto& source=displayOffsets[DisplayModeIndex(clock)];x=source[0];y=source[1];
}
void EcstasyFrame::CaptureDisplayOffsets(const GameTime& clock)
{
    Require(phase_==Phase::Building,"Cannot change display offsets in a sealed graphics frame");
    const auto mode=DisplayModeIndex(clock);std::int32_t x{},y{};eGetScreenDisplayOffsets(clock,x,y);
    sourceDisplayExtent_={{512u,mode==HP2_VIDEO_PAL ? 512u:448u}};
    // CreateTextureContext108D80..108D9C uses WorldTimer, in4000Hz ticks.
    textureTime_=static_cast<float>(clock.worldTimer)*0.0002500000118743628f;
    // Native full-raster fit: the centered client area corresponds to the
    // source default display origin. PAL eSetDisplaySystem1030E8 chooses
    // 512x512 or512x448; released DISPLAY MAGH4 is five VCK per source
    // pixel, MAGV0 is one raster per pixel. We do not emulate CRT blanking,
    // its extra DW+1 clock, or the original four-line overscan crop.
    // Convert before subtraction so any persisted signed word is safe.
    displayTranslation_[0]=(static_cast<double>(x)-defaultDisplayOffsets[mode][0])/(512.0*5.0);
    displayTranslation_[1]=(static_cast<double>(y)-defaultDisplayOffsets[mode][1])/
        (mode==HP2_VIDEO_PAL ? 512.0:448.0);
}
void EcstasyFrame::AdvanceTextureAnimationsAfterCompletion(ResourceRegistry& resources,const GameTime& clock)
{
    Require(phase_==Phase::Building && !textureAnimationResources_,
        "Texture animation update was scheduled outside its source frame boundary");
    textureAnimationResources_=&resources;
    textureAnimationWorldFrames_=clock.worldTimeFrames;textureAnimationRealFrames_=clock.realTimeFrames;
}
EcstasyFrame::EcstasyFrame()=default;
void EcstasyFrame::CaptureEnvironment(EnvironmentMap* environment,ResourceRegistry& resources,
    std::int32_t applicationState,std::uint32_t renderFrameCounter,bool inGame)
{
    Require(phase_==Phase::Building,"Cannot replace environment owners in a sealed frame");
    inGameMaterials_=inGame;
    environment_=environment;environmentApplicationState_=applicationState;renderFrameCounter_=renderFrameCounter;
    environmentUpdated_=false;
    // PAL112B28 downloads the actual CarSelectEnvMap descriptor into the
    // separate frontend texture slot used by AttachMicroCode10AF50/10B038.
    frontendEnvironmentTexture_=textureInfoForDescriptor(resources.SpecialTexture(8));
    specularFalloffTexture_=textureInfoForDescriptor(resources.SpecialTexture(6));
}
const float* EcstasyFrame::EnvironmentMatrix() const noexcept
{
    return environment_ ? environment_->BodyMatrix().data():nullptr;
}

SceneLightContext EcstasyFrame::SetupWorldLightContext(ResourceRegistry& resources,const CarCinematicScreen& view) const
{
    // PAL107900..107AB0. Source+0x70 is a transient context allocation;
    // native values are copied into the same frame's immutable draw packets.
    Require(view.view,"World light context has no original camera");
    float identity[16];eIdentity(identity);
    float eye[3];
    for(std::size_t axis=0;axis<3;++axis)
        eye[axis]=view.view->current.eye[axis]-view.view->current.matrix[axis*4+2]*10.0f;
    eye[2]+=4.0f;
    auto context=SetupLightContext(resources.Lights(),identity,EnvironmentMatrix(),eye,view,environmentApplicationState_);
    const auto& sun=resources.Sun().Vector();
    const float position[4]={sun[0],sun[1],sun[2],0},zero[4]{},up[4]{0,0,1,0};
    float sunMatrix[16];carCinematicLookAt(sunMatrix,position,zero,up);
    // Source107A84..107A90 replaces lightPositions(+80), not the separate
    // environmentMatrix(+E0). The subsequent scale matrix is identity.
    eMulMatrix(context.lightPositions.data(),identity,sunMatrix);
    return context;
}

void EcstasyFrame::RenderTargetStrip(const std::vector<FlailerPolyVertex>& vertices,
    const EcstasyRenderTarget& source,const EcstasyRenderTarget& destination)
{
    Require(phase_==Phase::Building && vertices.size()>=4 && vertices.size()<=128 && vertices.size()%2==0 && source.enabled && destination.enabled &&
        source.id>=8 && source.id<=13 && destination.id==14,
        "Environment remap strip has no original source/destination target");
    auto owned=std::make_unique<std::vector<FlailerPolyVertex>>(vertices);
    EcstasyDrawCommand command;command.kind=EcstasyDrawKind::RenderTargetStrip;
    command.worldStrip=owned.get();command.target=&destination;command.sampledTarget=&source;
    command.zWriteEnabled=false;command.rasterState={{0x01000100u,0,0,0}};
    worldStrips_.push_back(std::move(owned));
    try {commands_.push_back(command);}catch(...) {worldStrips_.pop_back();throw;}
    environmentUpdated_=true;
}
EcstasyFrame::~EcstasyFrame()
{
    // A submitted buffer can still be read by the device. Destroying it is
    // a host lifetime error, never an implicit fake GPU completion.
    if (phase_==Phase::Submitted)std::terminate();
    Release();
}

std::int32_t EcstasyFrame::SetZWriteState(std::int32_t enabled)
{
    Require(phase_==Phase::Building,"Cannot change depth writes in a sealed graphics frame");
    // Complete PAL1033C8..1033D8 eSetZWriteState: return the old state and
    // store enabled!=0. Frame-local storage transports the original packet
    // state into deferred Vulkan commands, without a device-global override.
    const auto previous=static_cast<std::int32_t>(zWriteEnabled_);
    zWriteEnabled_=enabled!=0;
    return previous;
}

bool EcstasyFrame::SetDashboardProjection(bool enabled)
{
    Require(phase_==Phase::Building,"Cannot change dashboard projection in a sealed frame");
    const bool previous=dashboardProjection_;dashboardProjection_=enabled;return previous;
}

std::array<float,2> ecstasyNativeDepthRange(const float projection[16],std::int32_t format)
{
    const double maximum=format==0x31 ? 16777215.0:65535.0;
    const auto endpoint=[&](double clip) {
        const double divisor=projection[11]*clip+projection[15];
        Require(std::isfinite(divisor) && divisor!=0.0,"World depth lacks its captured projection");
        const double source=(projection[10]*clip+projection[14])/divisor;
        // Matrix products may round an exact endpoint a few native float
        // ulps outside its interval. Clip the numeric depth at the physical
        // attachment boundary, without quantizing it to a console integer.
        const double tolerance=maximum*std::numeric_limits<float>::epsilon()*8.0;
        Require(std::isfinite(source) && source>=-tolerance && source<=maximum+tolerance,
            "Native depth endpoint exceeds its target interval");
        return static_cast<float>(1.0-std::clamp(source,0.0,maximum)/maximum);
    };
    const std::array<float,2> result{{endpoint(-1.0),endpoint(1.0)}};
    Require(result[0]<=result[1],"Native depth interval is reversed");
    return result;
}

void EcstasyFrame::AttachParticleGroup(const std::vector<EcstasyParticleQuad>& quads,
    const EditorMarkerTexture& descriptor,const CarCinematicScreen& view,float averageDepth)
{
    if(quads.empty())return;
    Require(phase_==Phase::Building && view.target && view.target->enabled &&
        view.target->width>0 && view.target->height>0 && view.matricesCalculated,
        "Particle group lacks its active view/target");
    const auto* texture=textureInfoForDescriptor(&descriptor);
    Require(texture && texture->pixels.Data(),"Particle group has no loaded texture owner");
    Require(std::isfinite(averageDepth),"Particle group depth is not finite");
    if(liveParticleGroups_==particleGroups_.size())
        particleGroups_.push_back(std::make_unique<std::vector<EcstasyParticleQuad>>());
    auto& owned=*particleGroups_[liveParticleGroups_];
    owned.assign(quads.begin(),quads.end());
    EcstasyDrawCommand command;command.kind=EcstasyDrawKind::ParticleGroup;
    command.particleGroup=&owned;command.texture=texture;command.view=&view;command.target=view.target;
    command.zWriteEnabled=zWriteEnabled_;command.depthSorted=texture->requiresDepthSort;
    command.sortDepth=averageDepth;command.alphaGroup=++nextAlphaGroup_;
    try {commands_.push_back(command);}catch(...) {owned.clear();throw;}
    ++liveParticleGroups_;
}

void EcstasyFrame::AddPolygon(const FlailerPoly& polygon,
    const EditorMarkerTexture& descriptor,const CarCinematicScreen& view,
    std::int32_t layer,float alphaScale,EcstasyUiPlacement placement)
{
    Require(phase_==Phase::Building,"Cannot append to a sealed graphics frame");
    Require(view.target && view.target->enabled && view.target->width>0 && view.target->height>0,
        "Flailer polygon has no active original render target");
    const auto* texture=textureInfoForDescriptor(&descriptor);
    Require(texture,"Flailer polygon texture pixels are not loaded: descriptor has no active texture owner");
    const auto* orderingTexture=texture;
    bool neutralTextureOperand=false;
    if(!texture->pixels.Data() || (texture->paletteBytes && !texture->palette.Data())) {
        // PAL19ECE0 retires the previous slideshow images while its next
        // bitmap remains visible with all four alpha values zero. The
        // retained descriptor can therefore legitimately have no CPU image.
        // Preserve this draw's alpha=0 write and original texture ordering:
        // for MODULATE and (Cs-Cd)*As+Cd with no Z write, sampled texels cannot
        // affect RGBA/depth. Borrow a live neutral operand only in that exact
        // PC transport case; a visible or otherwise different packet still
        // requires its requested image. This is not a source loading fallback.
        const auto* wire=descriptor.authoredRecord;
        if(texture->pack && wire && descriptor.authoredRecordSize>0x50 &&
            !texture->requiresDepthSort && wire[0x4e]==0 && wire[0x50]==0 &&
            polygon.rasterState[0]==0x01000100u && std::isfinite(alphaScale) &&
            std::all_of(polygon.vertices.begin(),polygon.vertices.end(),
                [](const FlailerPolyVertex& vertex){return (vertex.color>>24)==0;})) {
            auto* registry=texture->pack->CatalogueRoot().resourceRegistry;
            auto* neutral=registry ? textureInfoForDescriptor(registry->ResolveTexture(0x19006835u,false)):nullptr;
            if(neutral && neutral->pixels.Data() && (!neutral->paletteBytes || neutral->palette.Data()) &&
                neutral->descriptor && neutral->descriptor->authoredRecord &&
                neutral->descriptor->authoredRecordSize>0x4e &&
                !neutral->requiresDepthSort && neutral->descriptor->authoredRecord[0x4e]==0) {
                texture=neutral;neutralTextureOperand=true;
            }
        }
    }
    if(!texture->pixels.Data() || (texture->paletteBytes && !texture->palette.Data())) {
        // The registry lookup established the live owner. Do not inspect
        // an unregistered descriptor merely to enrich a failure message.
        std::ostringstream error;
        error<<"Flailer polygon texture pixels are not loaded: key=0x"<<std::hex<<texture->descriptor->key
            <<std::dec<<", pack="<<(texture->pack ? texture->pack->Name():"<none>")
            <<", pixel_bytes="<<texture->pixelBytes<<", palette_bytes="<<texture->paletteBytes
            <<", palette_loaded="<<(texture->palette.Data()!=nullptr)
            <<", pending_reads="<<(texture->pack && texture->pack->HasPendingReads());
        throw std::runtime_error(error.str());
    }
    // The original ePoly::Render10FE5C uses the texture's alpha-list lane
    // even for Flailer polygons. It suppresses that list in opaque-only
    // targets and stores layerDepth-MaxZ (or direct Z) as its sort key.
    if(texture->requiresDepthSort && view.target->opaqueOnly)return;
    EcstasyDrawCommand command;command.zWriteEnabled=neutralTextureOperand ? false:zWriteEnabled_;
    command.uiPlacement=placement;
    command.polygon=&polygon;command.texture=texture;command.view=&view;command.target=view.target;
    if(neutralTextureOperand)command.orderingTexture=orderingTexture;
    std::copy(view.matrices.clipToScreen,view.matrices.clipToScreen+16,command.clipToScreen.begin());
    command.rasterState=polygon.rasterState;command.layer=layer;command.alphaScale=alphaScale;
    command.depthSorted=texture->requiresDepthSort;
    if(command.depthSorted) {
        command.sortDepth=polygon.vertices[0].position[2];
        if(polygon.rasterState[2]&255u) {
            static constexpr float depths[29]={10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
                5,6,7,8,5,4,3,2,1,1,2,3,4};
            const auto index=convertFloatToInt32(command.sortDepth);
            Require(index>=0 && index<29,"Flailer alpha key has no authored layer");
            command.sortDepth=depths[index]-(view.target->format==0x31 ? 16777215.0f:65535.0f);
        }
    }
    commands_.push_back(command);
}

void EcstasyFrame::AttachSolidPolys()
{
    Require(phase_==Phase::Building,"Cannot attach solid lists in a sealed graphics frame");
    // PAL10BB20 walks only the solid texture chains. The alpha arena/list
    // survives this call, including the two display calls1134A4/1135B8
    // between scenery/particles, environment remap and the later cars.
    const auto count=static_cast<std::size_t>(std::count_if(
        commands_.begin()+static_cast<std::ptrdiff_t>(pendingAlphaBegin_),commands_.end(),
        [](const EcstasyDrawCommand& command){return command.depthSorted;}));
    pendingAlphaCommands_.reserve(pendingAlphaCommands_.size()+count);
    auto output=pendingAlphaBegin_;
    for(auto index=pendingAlphaBegin_;index<commands_.size();++index) {
        if(commands_[index].depthSorted)pendingAlphaCommands_.push_back(std::move(commands_[index]));
        else {
            if(output!=index)commands_[output]=std::move(commands_[index]);
            ++output;
        }
    }
    commands_.resize(output);
    if(output==pendingAlphaBegin_)return;
    // AddTexturesSorted PAL109358..109488 groups the first-use texture
    // list into priorities0..7. Linked textures insert immediately after
    // their parent's first anchor when both priorities match; later siblings
    // therefore precede earlier siblings. A stable priority sort alone is
    // different. These local anchors expire with this source flush, exactly
    // as DeleteDownloadedTextures109488 clears TextureInfo+9C.
    // The source uses temporary packet/list storage. Bulk native scratch
    // avoids allocating one heap node for every texture and list link.
    // Overflow still uses the ordinary allocator; no packet is truncated.
    std::array<std::byte,65536> localStorage;
    std::pmr::monotonic_buffer_resource scratch(localStorage.data(),localStorage.size());
    struct TextureChain {
        const TextureInfo* texture{};
        const EcstasyRenderTarget* sampled{};
        std::array<std::size_t,15> first,last;
        TextureChain(const TextureInfo* image,const EcstasyRenderTarget* target)
            :texture(image),sampled(target)
        {
            first.fill(std::numeric_limits<std::size_t>::max());last=first;
        }
    };
    constexpr auto noCommand=std::numeric_limits<std::size_t>::max();
    // Each command has exactly one target. Link its position once rather
    // than rescanning every texture's full command list for all15 targets.
    // These indices are scratch only; original frame/texture owners survive
    // until the ordered command values have been moved back below.
    std::pmr::vector<std::size_t> next(output-pendingAlphaBegin_,noCommand,&scratch);
    std::pmr::vector<TextureChain> chains{&scratch};
    std::pmr::map<std::pair<const TextureInfo*,const EcstasyRenderTarget*>,std::size_t> active{&scratch};
    // Source strips and their material passes frequently keep the same
    // ordering texture. This lookup is local to this exact solid flush;
    // only an already-created chain index is reused, never its draw order.
    const TextureInfo* previousTexture=nullptr;
    const EcstasyRenderTarget* previousSampled=nullptr;
    std::size_t previousChain=noCommand;
    for(auto index=pendingAlphaBegin_;index<commands_.size();++index) {
        const auto& command=commands_[index];
        const auto* texture=command.orderingTexture ? command.orderingTexture:command.texture;
        const auto* sampled=texture ? nullptr:command.sampledTarget;
        Require((texture || sampled) && command.target && command.target->id>=0 && command.target->id<15,
            "Solid texture chain lost its original image or target");
        std::size_t chainIndex=previousChain;
        if(previousChain==noCommand || texture!=previousTexture || sampled!=previousSampled) {
            const auto key=std::make_pair(texture,sampled);
            auto found=active.find(key);
            if(found==active.end()) {
                found=active.emplace(key,chains.size()).first;
                chains.emplace_back(texture,sampled);
            }
            chainIndex=found->second;
            previousTexture=texture;previousSampled=sampled;previousChain=chainIndex;
        }
        auto& chain=chains[chainIndex];
        const auto target=static_cast<std::size_t>(command.target->id);
        if(chain.last[target]==noCommand)chain.first[target]=index;
        else next[chain.last[target]-pendingAlphaBegin_]=index;
        chain.last[target]=index;
    }
    std::array<std::pmr::list<std::size_t>,8> priorities{{
        std::pmr::list<std::size_t>{&scratch},std::pmr::list<std::size_t>{&scratch},
        std::pmr::list<std::size_t>{&scratch},std::pmr::list<std::size_t>{&scratch},
        std::pmr::list<std::size_t>{&scratch},std::pmr::list<std::size_t>{&scratch},
        std::pmr::list<std::size_t>{&scratch},std::pmr::list<std::size_t>{&scratch}}};
    struct Anchor {std::uint8_t bucket{};std::pmr::list<std::size_t>::iterator position;};
    std::pmr::map<const TextureInfo*,Anchor> anchors{&scratch};
    const auto priority=[](const TextureInfo& texture) {
        Require(texture.descriptor && texture.descriptor->authoredRecord &&
            texture.descriptor->authoredRecordSize>=164,"Solid chain lost its TextureInfo priority");
        const auto value=texture.descriptor->authoredRecord[0x29];
        Require(value<8,"Solid texture priority exceeds the original eight lists");
        return value;
    };
    for(std::size_t index=0;index<chains.size();++index) {
        const auto* texture=chains[index].texture;
        if(!texture) {
            // UpdateGifTags105F1C builds generated-target TextureInfos from
            // cleared storage: priority0 and a self-linked download owner.
            priorities[0].push_back(index);continue;
        }
        const auto value=priority(*texture);
        const auto* parent=texture->linkedTexture;
        Require(parent,"Solid texture lacks its canonical linked download owner");
        const auto anchor=anchors.find(parent);
        if(anchor!=anchors.end() && priority(*parent)==value)
            priorities[anchor->second.bucket].insert(std::next(anchor->second.position),index);
        else {
            priorities[value].push_back(index);
            anchors[parent]={value,std::prev(priorities[value].end())};
        }
    }
    // Keep the source chain walk, but carry only source indices through
    // the permutation. Copying every large captured lighting/transform
    // value to a second command buffer and back is unnecessary. A cycle
    // moves each nonidentity command once plus one temporary per cycle.
    // No command or borrower can outlive this exact source flush.
    std::pmr::vector<std::size_t> order{&scratch};
    order.reserve(output-pendingAlphaBegin_);
    for(const auto& bucket:priorities)for(const auto chain:bucket) {
        // AttachSolidPolys10BC40..10BCC0 walks all15 target chains for
        // each texture. Material's base/specular/reflection packets remain
        // together inside their actual parent chain and original target.
        for(const auto first:chains[chain].first)
            for(auto index=first;index!=noCommand;index=next[index-pendingAlphaBegin_])
                order.push_back(index-pendingAlphaBegin_);
    }
    Require(order.size()==output-pendingAlphaBegin_,"Native solid command order lost a source command");
    static_assert(std::is_nothrow_move_constructible<EcstasyDrawCommand>::value &&
        std::is_nothrow_move_assignable<EcstasyDrawCommand>::value,
        "In-place native command ordering requires nonthrowing value moves");
    for(std::size_t start=0;start<order.size();++start) {
        if(order[start]==start)continue;
        auto saved=std::move(commands_[pendingAlphaBegin_+start]);
        auto destination=start;
        while(order[destination]!=start) {
            const auto source=order[destination];
            commands_[pendingAlphaBegin_+destination]=std::move(commands_[pendingAlphaBegin_+source]);
            order[destination]=destination;destination=source;
        }
        commands_[pendingAlphaBegin_+destination]=std::move(saved);
        order[destination]=destination;
    }
    pendingAlphaBegin_=output;
}

void EcstasyFrame::RenderScreenPolygon(const FlailerPoly& polygon,
    const TextureInfo* texture,const CarCinematicScreen& view)
{
    // PAL10FA68..10FF88. This value transport is the direct-Z ePoly path
    // used by Sun; its texture still owns the original TEST/ZBUF context.
    if(!texture)return;
    Require(phase_==Phase::Building && view.target && view.target->enabled &&
        view.matricesCalculated && texture->pixels.Data(),"Screen polygon lost its loaded texture or view");
    if(texture->requiresDepthSort && view.target->opaqueOnly)return;
    auto owned=std::make_unique<FlailerPoly>(polygon);
    if(owned->rasterState[1]&0x00ff0000u)owned->rasterState[1]&=~0xff000000u;
    EcstasyDrawCommand command;command.kind=EcstasyDrawKind::ScreenPolygon;
    command.polygon=owned.get();command.texture=texture;command.view=&view;command.target=view.target;
    command.rasterState=owned->rasterState;command.zWriteEnabled=zWriteEnabled_;
    command.depthSorted=texture->requiresDepthSort;command.sortDepth=polygon.vertices[0].position[2];
    std::copy(view.matrices.clipToScreen,view.matrices.clipToScreen+16,command.clipToScreen.begin());
    worldPolygons_.push_back(std::move(owned));
    try {commands_.push_back(command);}catch(...) {worldPolygons_.pop_back();throw;}
}

void EcstasyFrame::TransferZBufferRect(std::shared_ptr<SunDepthSamples> destination,
    const CarCinematicScreen& view,std::int32_t x,std::int32_t y,std::int32_t width,std::int32_t height,
    float nativeSampleScale)
{
    // PAL111C58 creates an active-stage request. The native device retains
    // its original deferred lifetime and reads actual preceding GPU depth.
    Require(phase_==Phase::Building && destination && view.target && view.target->enabled &&
        view.target->id>=0 && view.target->id<7 && view.matricesCalculated &&
        x>=0 && y>=0 && width>0 && height>0 &&
        std::isfinite(nativeSampleScale) && nativeSampleScale>0.0f &&
        (static_cast<double>(x)+width)*nativeSampleScale<=view.target->width &&
        (static_cast<double>(y)+height)*nativeSampleScale<=view.target->height &&
        static_cast<std::size_t>(width)*static_cast<std::size_t>(height)<=destination->values.size(),
        "Source depth transfer has no retained destination or valid main-target rectangle");
    EcstasyDepthTransfer request;request.destination=std::move(destination);
    request.x=x;request.y=y;request.width=width;request.height=height;
    request.targetWidth=view.target->width;request.targetHeight=view.target->height;
    request.targetLeft=view.target->left;request.targetTop=view.target->top;
    request.nativeSampleScale=nativeSampleScale;
    request.maximumDepth=view.target->format==0x31 ? 16777215u:65535u;
    std::copy(view.matrices.clipToScreen,view.matrices.clipToScreen+16,request.clipToScreen.begin());
    depthTransfers_.push_back(request);
}

void EcstasyFrame::AttachAlphaPolys()
{
    Require(phase_==Phase::Building,"Cannot attach alpha lists in a sealed graphics frame");
    // Finish any still-unattached native solid packets before the original
    // alpha list. Explicit earlier solid boundaries have already consumed
    // their ranges, so this never reorders across a source stage.
    AttachSolidPolys();
    // Earlier solid-only boundaries may have retained alpha packets. Insert
    // them before newer pending packets to preserve source insertion order,
    // including equal-key and grouped material-pass tie behavior below.
    if(!pendingAlphaCommands_.empty()) {
        commands_.insert(commands_.begin()+static_cast<std::ptrdiff_t>(pendingAlphaBegin_),
            std::make_move_iterator(pendingAlphaCommands_.begin()),
            std::make_move_iterator(pendingAlphaCommands_.end()));
        pendingAlphaCommands_.clear();
    }
    // PAL10B838 flushes one list per target after the solid lists. Its
    // SortAlphaKeys1055B0/RadixSortAlphaKeys1051D8 use integer float keys:
    // priority0..7, then far-to-near; the final reverse walk reverses equal
    // positive keys, while the signed radix pass retains negative ties.
    // Work only since the last source flush. Sky, world, mirror and HUD must
    // never enter one frame-wide ordering operation.
    struct Packet {
        std::size_t begin{},end{};
        std::int32_t target{};
        std::uint32_t priority{},key{},bits{};
    };
    std::array<std::byte,16384> localStorage;
    std::pmr::monotonic_buffer_resource scratch(localStorage.data(),localStorage.size());
    std::pmr::vector<Packet> alpha{&scratch};
    alpha.reserve(commands_.size()-pendingAlphaBegin_);
    for(auto index=pendingAlphaBegin_;index<commands_.size();++index) {
        const auto& command=commands_[index];
        if(!command.depthSorted)continue;
        const auto* texture=command.orderingTexture ? command.orderingTexture:command.texture;
        Require(texture && texture->descriptor && command.target &&
            texture->descriptor->authoredRecord && texture->descriptor->authoredRecordSize>0x29,
            "Alpha packet lost its actual texture priority or target");
        Packet packet;packet.begin=index;packet.end=index+1;packet.target=command.target->id;
        const auto priority=texture->descriptor->authoredRecord[0x29];
        packet.priority=priority<8 ? priority:7;
        std::memcpy(&packet.bits,&command.sortDepth,sizeof(packet.bits));
        packet.key=(packet.bits&0x80000000u) ? ~packet.bits:packet.bits^0x80000000u;
        if(command.alphaGroup) {
            while(packet.end<commands_.size() && commands_[packet.end].alphaGroup==command.alphaGroup) {
                const auto& member=commands_[packet.end];
                const auto* memberTexture=member.orderingTexture ? member.orderingTexture:member.texture;
                Require(member.depthSorted && memberTexture==texture && member.target==command.target,
                    "Alpha group no longer represents one original direct packet");
                ++packet.end;
            }
            index=packet.end-1;
        }
        alpha.push_back(packet);
    }
    if(!alpha.empty()) {
        std::sort(alpha.begin(),alpha.end(),[](const Packet& a,const Packet& b) {
            if(a.target!=b.target)return a.target<b.target;
            if(a.priority!=b.priority)return a.priority<b.priority;
            if(a.key!=b.key)return a.key>b.key;
            return (a.bits&0x80000000u) ? a.begin<b.begin:a.begin>b.begin;
        });
        auto& ordered=orderingScratch_;
        struct ClearScratch {std::vector<EcstasyDrawCommand>& value;~ClearScratch(){value.clear();}} clear{ordered};
        ordered.clear();
        ordered.reserve(commands_.size()-pendingAlphaBegin_);
        for(auto index=pendingAlphaBegin_;index<commands_.size();++index)
            if(!commands_[index].depthSorted)ordered.push_back(std::move(commands_[index]));
        for(const auto& packet:alpha)
            for(auto index=packet.begin;index<packet.end;++index)
                ordered.push_back(std::move(commands_[index]));
        std::move(ordered.begin(),ordered.end(),commands_.begin()+static_cast<std::ptrdiff_t>(pendingAlphaBegin_));
    }
    pendingAlphaBegin_=commands_.size();
}

void EcstasyFrame::OverlayRVMImages(const CarCinematicServices& views,ResourceRegistry& resources)
{
    // PAL10EC70..10EDC8 (exclusive). Both textures belong to the canonical special-texture
    // pair refreshed by the resource owner. The source writes mask byte4F
    // before checking view3, then draws the two authored ALPHA materials.
    const auto* info=resources.RearViewTexture(0);
    const auto* mask=resources.RearViewTexture(1);
    if(!info || !mask)return;
    auto* maskTexture=textureInfoForDescriptor(mask);
    Require(maskTexture && maskTexture->pack,"Rear-view mask has no retained texture pack");
    maskTexture->pack->SetDescriptorByte4F(maskTexture->descriptor,1);
    const auto& rear=views.screens[3];
    if(!rear.enabled)return;
    Require(rear.target,"Rear-view overlay lost its actual target");
    if(!rear.target->enabled)return;
    Require(phase_==Phase::Building && rear.matricesCalculated && rear.target->id==7 &&
        rear.target->width==256 && rear.target->height==64,
        "Rear-view overlay lacks its original calculated view and target");
    const float maximum=rear.target->format==0x31 ? 16777215.0f:65535.0f;
    const float positions[4][2]={{1,-14},{257,-14},{257,76},{1,76}};
    const float coordinates[4][2]={{1,0},{0,0},{0,1},{1,1}};
    for(const auto* descriptor:{info,mask}) {
        const auto* texture=textureInfoForDescriptor(descriptor);
        Require(texture && texture->pixels.Data(),"Rear-view overlay texture pixels are not loaded");
        auto polygon=std::make_unique<FlailerPoly>();
        polygon->rasterState={{0x01000100u,0x80u,0,0}};
        for(std::size_t index=0;index<4;++index) {
            auto& vertex=polygon->vertices[index];
            vertex.position={{positions[index][0],positions[index][1],maximum,0}};
            vertex.textureCoordinate={{coordinates[index][0],coordinates[index][1],1}};
            vertex.color=0x80808080u;
        }
        EcstasyDrawCommand command;command.kind=EcstasyDrawKind::MirrorOverlayPolygon;
        command.polygon=polygon.get();command.texture=texture;command.view=&rear;command.target=rear.target;
        std::copy(rear.matrices.clipToScreen,rear.matrices.clipToScreen+16,command.clipToScreen.begin());
        command.rasterState=polygon->rasterState;command.zWriteEnabled=zWriteEnabled_;
        command.depthSorted=texture->requiresDepthSort;command.sortDepth=maximum;
        worldPolygons_.push_back(std::move(polygon));
        try {commands_.push_back(command);}catch(...) {worldPolygons_.pop_back();throw;}
    }
}

// PC-only viewport/capture adaptation. PAL's rear projection, source
// polygon, textures and game-state logic remain fixed at their proven values.
// The source (512 x 448 NTSC or 512 x 512 PAL) is fitted into a centered
// uniform-scale canvas. The physical rear color/depth attachments are grown
// to native pixel density; the unchanged source quad is transformed once
// by the renderer's existing logical-to-physical viewport mapping.
EcstasyNativeMirrorPresentation ecstasyNativeMirrorPresentation(
    std::uint32_t targetWidth,std::uint32_t targetHeight,
    std::uint32_t sourceWidth,std::uint32_t sourceHeight,
    std::uint32_t physicalWidth,std::uint32_t physicalHeight,
    std::uint32_t maximumImageDimension)
{
    Require(targetWidth && targetHeight && sourceWidth==512u &&
        (sourceHeight==448u || sourceHeight==512u) && physicalWidth &&
        physicalHeight && maximumImageDimension>=256u,
        "Native mirror presentation lost its valid PAL canvas or physical target");
    // Source DisplayRVMs has the *opposite* U coordinates and 160..352
    // coordinates in a 512-wide logical display. The Vulkan rearSample
    // shader already maps that original canvas to the PC output viewport;
    // scaling these CPU vertices again would clip the mirror at 1080p.
    // Keep the PAL polygon precisely as authored. Increase only target7's
    // physical raster (never the source camera's logical 256x64).
    const auto captureScale=std::max(1.0,std::min(
        std::max(static_cast<double>(physicalWidth)/sourceWidth,
                 static_cast<double>(physicalHeight)/sourceHeight),
        static_cast<double>(maximumImageDimension)/256.0));
    EcstasyNativeMirrorPresentation result{};
    result.left=160.0f;result.top=20.0f;
    result.right=352.0f;result.bottom=84.0f;
    result.rasterWidth=static_cast<std::uint32_t>(std::min<double>(maximumImageDimension,
        std::ceil(256.0*captureScale)));
    result.rasterHeight=static_cast<std::uint32_t>(std::min<double>(maximumImageDimension,
        std::ceil(64.0*captureScale)));
    return result;
}

EcstasyNativeMirrorScissor ecstasyNativeMirrorScissor(
    double viewportX,double viewportY,double viewportWidth,double viewportHeight,
    std::uint32_t sourceWidth,std::uint32_t sourceHeight,
    std::uint32_t outputWidth,std::uint32_t outputHeight,
    double translationX,double translationY)
{
    Require(sourceWidth==512u && (sourceHeight==448u || sourceHeight==512u) &&
        outputWidth && outputHeight && std::isfinite(viewportX) &&
        std::isfinite(viewportY) && std::isfinite(viewportWidth) &&
        std::isfinite(viewportHeight) && viewportWidth>0 && viewportHeight>0 &&
        std::isfinite(translationX) && std::isfinite(translationY),
        "Rear-view native scissor has no valid original canvas or viewport");
    // Vulkan's rectangle is [left,right) / [top,bottom). Round for pixel
    // centres, not downwards across the author's frame/mask boundary.
    const auto integer=[&](double value,std::uint32_t limit) {
        return static_cast<std::int32_t>(std::ceil(std::clamp(value-0.5,0.0,
            static_cast<double>(std::min<std::uint32_t>(limit,0x7fffffffu)))));
    };
    return {
        integer(viewportX+viewportWidth*160.0/sourceWidth+translationX,outputWidth),
        integer(viewportY+viewportHeight*20.0/sourceHeight+translationY,outputHeight),
        integer(viewportX+viewportWidth*352.0/sourceWidth+translationX,outputWidth),
        integer(viewportY+viewportHeight*84.0/sourceHeight+translationY,outputHeight)
    };
}

float ecstasyNativeMirrorMaskedSampleU(float originalU) noexcept
{
    // PAL OverlayRVMImages uses 1..257 on the 256-wide offscreen target.
    // Source x<1 is *camera only*, never overwritten by RVM/RVM_MASK. The
    // later DisplayRVMs reverses U, exposing precisely this column on the
    // right edge. The authentic RVM_MASK has a second dotted/transparent
    // edge column at x=1, but its next column x=2 is opaque in the glass
    // interior. Two source pixels plus a half-texel inset avoids both the
    // uncovered camera-only lane and bilinear bleed of the dotted border.
    constexpr float safeSourceU=2.5f/256.0f;
    return safeSourceU+(1.0f-safeSourceU)*originalU;
}

std::int32_t ecstasyNativeMirrorCompositeAlphaTest() noexcept
{
    // The real RVM_MASK is paletted PSMT8 with transparent outside pixels;
    // the source target7 alpha contains its shape. Drop only low-alpha edge
    // leakage before native bilinear composition. No shader replacement.
    return 2; // Existing GPU TEST alpha >48/128; PAL data remains unchanged.
}

void EcstasyFrame::DisplayRVMs(const CarCinematicServices& views,const CarCinematicScreen& destination)
{
    // Complete released source selection and polygon inputs10EDC8..10EEEC.
    // The original loop visits view3 only and tests both enabled owners.
    const auto& rear=views.screens[3];
    if(!rear.enabled)return;
    Require(rear.target,"Rear-view screen lost its actual render target");
    if(!rear.target->enabled)return;
    Require(phase_==Phase::Building && destination.target && destination.target->enabled &&
        destination.matricesCalculated && rear.target!=destination.target &&
        rear.target->id==7 && rear.target->width==256 && rear.target->height==64 &&
        sourceDisplayExtent_[0]==512 && (sourceDisplayExtent_[1]==448 || sourceDisplayExtent_[1]==512),
        "Rear-view composition lacks its original source/destination target or captured display extent");
    auto polygon=std::make_unique<FlailerPoly>();
    // Original ePoly constructor: neutral128-unit color and source alpha
    // equation. DisplayRVMs alone clears ScaleCoordinates (byte97).
    polygon->rasterState={{0x01000100u,0x00000080u,0,0}};
    const float maximum=destination.target->format==0x31 ? 16777215.0f:65535.0f;
    // Keep PAL 160..352 / 20..84 exactly. Native target7 scaling is
    // performed once during Vulkan target allocation / viewport projection,
    // never in this original 10EDC8..10EEEC scene composition function.
    const float positions[4][2]={{160,20},{352,20},{352,84},{160,84}};
    const float coordinates[4][2]={{1,0},{0,0},{0,1},{1,1}};
    for(std::size_t index=0;index<4;++index) {
        auto& vertex=polygon->vertices[index];
        vertex.position={{positions[index][0],positions[index][1],maximum,0}};
        vertex.textureCoordinate={{coordinates[index][0],coordinates[index][1],1}};
        vertex.color=0x80808080u;
    }
    EcstasyDrawCommand command;command.kind=EcstasyDrawKind::RenderTargetPolygon;
    command.polygon=polygon.get();command.view=&destination;command.target=destination.target;
    std::copy(destination.matrices.clipToScreen,destination.matrices.clipToScreen+16,command.clipToScreen.begin());
    command.sampledTarget=rear.target;command.zWriteEnabled=zWriteEnabled_;
    command.rasterState=polygon->rasterState;
    worldPolygons_.push_back(std::move(polygon));
    try {commands_.push_back(command);}catch(...) {worldPolygons_.pop_back();throw;}
}

static std::array<hp2::FlailerPoly,4> radialBlurPolygons(
    const EcstasyRenderTarget& target,uint8_t inner,uint8_t outer)
{
    const float left=static_cast<float>(target.left),top=static_cast<float>(target.top);
    const float right=static_cast<float>(target.left+target.width);
    const float bottom=static_cast<float>(target.top+target.height);
    // Source signed integer midpoint before conversion to float.
    const float middleX=static_cast<float>(target.left+target.width/2);
    const float middleY=static_cast<float>(target.top+target.height/2);
    const float z=target.format==0x31 ? 16777215.0f : 65535.0f;
    const float coordinates[4][2]={{0,0},{1,0},{1,1},{0,1}};
    std::array<hp2::FlailerPoly,4> output{};
    hp2::FlailerPoly p;
    // byte90..93 = 0,0,0,1; byte94..97 = 128,0,1,0.
    // (Cs-Cs)*As + Cd writes mask alpha while preserving RGB.
    p.rasterState={{0x01000000u,0x00010080u,0,0}};
    const float positions[4][2]={{left,top},{middleX,top},{middleX,middleY},{left,middleY}};
    for(size_t i=0;i<4;++i) {
        p.vertices[i].position={{positions[i][0],positions[i][1],z,0}};
        p.vertices[i].textureCoordinate={{coordinates[i][0],coordinates[i][1],1}};
        p.vertices[i].color=0x00808080u|(uint32_t(i==2?inner:outer)<<24);
    }
    output[0]=p;
    p.vertices[0].position[0]=p.vertices[3].position[0]=right;output[1]=p;
    p.vertices[0].position[1]=p.vertices[1].position[1]=bottom;output[2]=p;
    p.vertices[0].position[0]=p.vertices[3].position[0]=left;output[3]=p;
    return output;
}

EcstasyNativeFeedbackRegion ecstasyNativeFeedbackRegion(const EcstasyRenderTarget& target,
    std::uint32_t width,std::uint32_t height,std::uint32_t logicalWidth,std::uint32_t logicalHeight,
    const std::array<double,2>& translation)
{
    Require(width && height && logicalWidth && logicalHeight && target.width>0 && target.height>0 &&
        width<=static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) &&
        height<=static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()),
        "Colour feedback lost its native or logical raster extent");
    const double sx=static_cast<double>(width)/logicalWidth,sy=static_cast<double>(height)/logicalHeight;
    const double spanX=target.width*sx,spanY=target.height*sy;
    const double x=target.left*sx+translation[0]*width,y=target.top*sy+translation[1]*height;
    Require(std::isfinite(x) && std::isfinite(y),"Colour feedback display translation is nonfinite");
    const double left=std::clamp(x,0.0,static_cast<double>(width));
    const double top=std::clamp(y,0.0,static_cast<double>(height));
    const double right=std::clamp(x+spanX,0.0,static_cast<double>(width));
    const double bottom=std::clamp(y+spanY,0.0,static_cast<double>(height));
    EcstasyNativeFeedbackRegion result;
    if(right<=left || bottom<=top)return result;
    result.viewport={{static_cast<float>(left),static_cast<float>(top),
        static_cast<float>(right-left),static_cast<float>(bottom-top)}};
    const auto l=static_cast<std::int32_t>(std::floor(left)),t=static_cast<std::int32_t>(std::floor(top));
    result.scissor={{l,t,static_cast<std::int32_t>(std::ceil(right))-l,static_cast<std::int32_t>(std::ceil(bottom))-t}};
    result.textureRectangle={{static_cast<float>((left-x)/spanX),static_cast<float>((top-y)/spanY),
        static_cast<float>((right-x)/spanX),static_cast<float>((bottom-y)/spanY)}};
    return result;
}

void EcstasyFrame::DisplayFilter(const CarCinematicServices& views,ResourceRegistry& resources)
{
    Require(phase_==Phase::Building,"Screen filter belongs to the current display frame");
    if(!nativeRenderPolicy.screenFilters[0].enabled && !nativeRenderPolicy.screenFilters[1].enabled)return;
    AttachAlphaPolys();
    const auto* descriptor=resources.SpecialTexture(3); // PAL103F1C/24: FilterGrey02C8711C.
    // PAL10F700/704 explicitly permits this optional development resource
    // to be absent. Retain that null gate; never manufacture a substitute.
    if(!descriptor)return;
    const auto* texture=textureInfoForDescriptor(descriptor);
    Require(texture && texture->pixels.Data(),"Screen filter lost its released FilterGrey resource");
    const auto group=++nextFeedbackGroup_;
    Require(group!=0,"Native colour-feedback group count overflowed");
    for(std::size_t index=1;index<3;++index) {
        const auto& view=views.screens[index];const auto& filter=nativeRenderPolicy.screenFilters[index-1];
        if(!filter.enabled || !view.target || !view.target->enabled)continue;
        // The released mode6/7 leaves select this exact destination equation
        // and TFX0. Reject a different descriptor rather than substituting it.
        Require(filter.blend==std::array<std::uint8_t,4>{{1,0,2,1}} && filter.textureFunction==0,
            "Screen filter has an unsupported original destination equation");
        EcstasyDrawCommand command;command.kind=EcstasyDrawKind::DestinationFilterPass;
        command.view=&view;command.target=view.target;command.texture=texture;
        command.screenFilter=filter;command.feedbackGroup=group;command.zWriteEnabled=false;
        commands_.push_back(command);
    }
    pendingAlphaBegin_=commands_.size();
}

void EcstasyFrame::DisplayRadialBlur(const CarCinematicServices& views)
{
    Require(phase_==Phase::Building,"Radial blur belongs to the current display frame");
    if(!views.radialBlur)return;
    for(std::size_t index=1;index<3;++index) {
        const auto& view=views.screens[index];
        if(!view.enabled || !view.radialBlurTexture || !view.target || !view.target->enabled)continue;
        Require(view.matricesCalculated && view.radialBlurTexture->pixels.Data(),
            "Radial blur lost its authored texture or calculated view");
        const auto polygons=radialBlurPolygons(*view.target,views.radialBlur->firstAmount[index-1],
            views.radialBlur->secondAmount[index-1]);
        for(const auto& polygon:polygons) {
            auto owned=std::make_unique<FlailerPoly>(polygon);
            EcstasyDrawCommand command;command.kind=EcstasyDrawKind::ScreenFilterPolygon;
            command.polygon=owned.get();command.texture=view.radialBlurTexture;
            command.view=&view;command.target=view.target;command.rasterState=polygon.rasterState;
            command.zWriteEnabled=zWriteEnabled_;
            std::copy(view.matrices.clipToScreen,view.matrices.clipToScreen+16,command.clipToScreen.begin());
            worldPolygons_.push_back(std::move(owned));
            try {commands_.push_back(command);}catch(...) {worldPolygons_.pop_back();throw;}
        }
        AttachSolidPolys(); // PAL10F4A0 calls10BB20: mask solid chain only.
    }
}

void EcstasyFrame::DisplayBlendPrevious(const CarCinematicServices& views)
{
    Require(phase_==Phase::Building,"History blend belongs to the current display frame");
    const auto* previous=views.screens[0].target;
    if(!previous || !previous->enabled)return;
    for(std::size_t index=1;index<4;++index) {
        const auto& view=views.screens[index];
        if(!view.enabled || !view.radialBlurTexture || !view.target || !view.target->enabled)continue;
        Require(view.matricesCalculated && previous->width>0 && previous->height>0 &&
            sourceDisplayExtent_[0] && sourceDisplayExtent_[1],"History blend lacks its display projection");
        const auto& target=*view.target;
        auto polygon=std::make_unique<FlailerPoly>();
        polygon->rasterState={{0x01010100u,0x00010080u,0,0}};
        const float left=static_cast<float>(target.left),top=static_cast<float>(target.top);
        const float right=left+static_cast<float>(target.width),bottom=top+static_cast<float>(target.height);
        const float maximum=target.format==0x31 ? 16777215.0f:65535.0f;
        const float positions[4][2]={{left,top},{right,top},{right,bottom},{left,bottom}};
        // PAL10F600..10F64C addresses a512x512 previous buffer, including
        // half a SOURCE pixel. Native history contains only the active raster,
        // so remove console padding and scale the same authored coordinates.
        // Deliberately preserve the source max=width/height, not right/bottom.
        const float u0=left/static_cast<float>(previous->width)+0.5f/static_cast<float>(sourceDisplayExtent_[0]);
        const float v0=top/static_cast<float>(previous->height)+0.5f/static_cast<float>(sourceDisplayExtent_[1]);
        const float u1=static_cast<float>(target.width)/static_cast<float>(previous->width)-0.5f/static_cast<float>(sourceDisplayExtent_[0]);
        const float v1=static_cast<float>(target.height)/static_cast<float>(previous->height)-0.5f/static_cast<float>(sourceDisplayExtent_[1]);
        const float coordinates[4][2]={{u0,v0},{u1,v0},{u1,v1},{u0,v1}};
        for(std::size_t vertex=0;vertex<4;++vertex) {
            polygon->vertices[vertex].position={{positions[vertex][0],positions[vertex][1],maximum,0}};
            polygon->vertices[vertex].textureCoordinate={{coordinates[vertex][0],coordinates[vertex][1],1}};
            polygon->vertices[vertex].color=0x80808080u;
        }
        EcstasyDrawCommand command;command.kind=EcstasyDrawKind::ScreenFilterPolygon;
        command.polygon=polygon.get();command.view=&view;command.target=view.target;
        command.sampledTarget=previous;command.sampledPreviousFrame=true;
        command.zWriteEnabled=zWriteEnabled_;command.rasterState=polygon->rasterState;
        std::copy(view.matrices.clipToScreen,view.matrices.clipToScreen+16,command.clipToScreen.begin());
        worldPolygons_.push_back(std::move(polygon));
        try {commands_.push_back(command);}catch(...) {worldPolygons_.pop_back();throw;}
        AttachSolidPolys(); // PAL10F670 calls10BB20: history solid chain only.
    }
}

namespace {
void CaptureMaterialProjection(std::array<float,16>& output,const TextureInfo& texture,
    const CarCinematicScreen& view)
{
    // PAL AttachSolidPolys10BC0C/10BC54..10BC78 and
    // AttachAlphaPolys10B9D8..10BA04 select the target's +8C/+90/+94
    // projection with the texture's byte75. This is independent of alpha
    // sorting and Z-write. The authored shadow/road/lava overlays use1.
    // Capture it with the issuing view: sky, world and auxiliary targets
    // may recalculate these same matrices before the native draw is gathered.
    if(!texture.descriptor || !texture.descriptor->authoredRecord || texture.descriptor->authoredRecordSize<164)
        throw std::runtime_error("World material lost its original projection selector");
    const auto mode=texture.descriptor->authoredRecord[0x75];
    if(mode!=1 && mode!=2) {
        std::copy(view.matrices.clipToScreen,view.matrices.clipToScreen+16,output.begin());
        return;
    }
    const auto& projection=mode==1 ? view.matrices.nearProjection:view.matrices.frustumProjection;
    output.fill(0.0f);
    for(std::size_t axis=0;axis<3;++axis)output[axis*5]=projection.diagonal[axis];
    std::copy(projection.translation,projection.translation+4,output.begin()+12);
}
}

void EcstasyFrame::RenderPolygon3D(const FlailerPoly& polygon,
    const TextureInfo* texture,const float localToWorld[16],const CarCinematicScreen& view)
{
    // PAL10FF88..110238. The final release uses the fixed0x200 draw flag;
    // beta's descriptor4F-dependent flag is absent from the release body.
    if (!texture)return;
    Require(phase_==Phase::Building,"Cannot append to a sealed graphics frame");
    Require(localToWorld && view.target && view.target->enabled &&
        view.target->width>0 && view.target->height>0 && texture->pixels.Data(),
        "World polygon lacks its actual target, matrix or loaded texture");
    auto owned=std::make_unique<FlailerPoly>(polygon);
    EcstasyDrawCommand command;command.zWriteEnabled=zWriteEnabled_;
    command.kind=EcstasyDrawKind::WorldPolygon;command.polygon=owned.get();
    command.textureQScale=view.pixelScale;
    CaptureMaterialProjection(command.clipToScreen,*texture,view);
    command.texture=texture;command.view=&view;command.target=view.target;
    std::copy(localToWorld,localToWorld+16,command.localToWorld.begin());
    eMulMatrix(command.localToClip.data(),localToWorld,view.matrices.worldToClip);
    command.rasterState=polygon.rasterState;command.drawFlags=0x200u;
    command.vertexOrder={0,1,3,2};command.depthSorted=texture->requiresDepthSort;
    if (command.depthSorted) {
        // Exact source alpha-sort key: vertex0 is transformed by worldToView
        // alone. Applying localToWorld here would change the original key.
        float depth[4];eMulVector3(depth,view.matrices.worldToView,polygon.vertices[0].position.data());
        command.sortDepth=depth[2];
    }
    worldPolygons_.push_back(std::move(owned));
    try {commands_.push_back(command);}catch (...) {worldPolygons_.pop_back();throw;}
}

void EcstasyFrame::RenderWorldStrip(const std::vector<FlailerPolyVertex>& vertices,
    const TextureInfo& texture,const float localToWorld[16],const CarCinematicScreen& view)
{
    Require(phase_==Phase::Building,"Cannot append world strip to a sealed graphics frame");
    Require(localToWorld && view.target && view.target->enabled && view.matricesCalculated &&
        texture.pixels.Data(),"World strip lacks its source target, matrix or texture");
    // PAL1222B8 emits up to2*(64+1) vertices for a convex car-shadow border.
    Require(vertices.size()>=3 && vertices.size()<=130,"World strip exceeds its original immediate extent");
    auto owned=std::make_unique<std::vector<FlailerPolyVertex>>(vertices);
    EcstasyDrawCommand command;command.kind=EcstasyDrawKind::WorldStrip;
    command.textureQScale=view.pixelScale;
    CaptureMaterialProjection(command.clipToScreen,texture,view);
    command.worldStrip=owned.get();command.texture=&texture;command.view=&view;command.target=view.target;
    command.zWriteEnabled=zWriteEnabled_;command.drawFlags=0x200u;
    std::copy(localToWorld,localToWorld+16,command.localToWorld.begin());
    eMulMatrix(command.localToClip.data(),localToWorld,view.matrices.worldToClip);
    command.depthSorted=texture.requiresDepthSort;
    if(command.depthSorted) {
        float depth[4];eMulVector3(depth,view.matrices.worldToView,vertices.front().position.data());
        command.sortDepth=depth[2];
    }
    worldStrips_.push_back(std::move(owned));
    try {commands_.push_back(command);}catch(...) {worldStrips_.pop_back();throw;}
}

void EcstasyFrame::RenderStrip(const EditorMarkerModel& model,
    const EditorMarkerSolidGeometry::Strip& strip,const TextureInfo& texture,
    const EditorMarkerLightMaterial* material,const float localToWorld[16],
    const CarCinematicScreen& view,std::int32_t clipClass,std::uint32_t flags,float alphaScale,
    const SceneLightContext* lightContext)
{
    RenderStrip(model,strip,texture,material,localToWorld,view,clipClass,flags,alphaScale,lightContext,nullptr);
}

void EcstasyFrame::RenderStrip(const EditorMarkerModel& model,
    const EditorMarkerSolidGeometry::Strip& strip,const TextureInfo& texture,
    const EditorMarkerLightMaterial* material,const float localToWorld[16],
    const CarCinematicScreen& view,std::int32_t clipClass,std::uint32_t flags,float alphaScale,
    const SceneLightContext* lightContext,const float preparedLocalToClip[16])
{
    Require(phase_==Phase::Building,"Cannot append model geometry to a sealed frame");
    Require(localToWorld && view.target && view.target->enabled && view.matricesCalculated &&
        texture.pixels.Data(),"Solid strip lacks its actual matrix, target or loaded texture");
    Require(strip.positions.size()==strip.textureCoordinates.size() &&
        strip.positions.size()==strip.colors.size(),"Solid strip channels have different extents");
    if (strip.positions.size()<3)return;
    EcstasyDrawCommand command;command.zWriteEnabled=zWriteEnabled_;
    command.kind=EcstasyDrawKind::SolidStrip;command.strip=&strip;command.model=&model;
    command.textureQScale=view.pixelScale;
    CaptureMaterialProjection(command.clipToScreen,texture,view);
    if(dashboardProjection_) {
        const auto& projection=view.matrices.frustumProjection;
        command.clipToScreen.fill(0.0f);
        for(std::size_t axis=0;axis<3;++axis)command.clipToScreen[axis*5]=projection.diagonal[axis];
        std::copy(projection.translation,projection.translation+4,command.clipToScreen.begin()+12);
    }
    if(material && inGameMaterials_ && material->inGameVariant)material=material->inGameVariant.get();
    command.texture=&texture;command.lightMaterial=material;command.view=&view;command.target=view.target;
    if(lightContext)command.lightContext=*lightContext;
    std::copy(localToWorld,localToWorld+16,command.localToWorld.begin());
    if(preparedLocalToClip)
        std::copy(preparedLocalToClip,preparedLocalToClip+16,command.localToClip.begin());
    else eMulMatrix(command.localToClip.data(),localToWorld,view.matrices.worldToClip);
    command.clipClass=clipClass;command.drawFlags=flags;command.alphaScale=alphaScale;
    command.depthSorted=texture.requiresDepthSort;
    if (command.depthSorted) {
        // Complete PAL10CBF8 sort-key calculation: centre of the strip bounds,
        // then the supplied model transform, then the actual world-to-view.
        float center[4],world[4],camera[4];
        for (std::size_t axis=0;axis<3;++axis)center[axis]=(strip.minimum[axis]+strip.maximum[axis])*0.5f;
        center[3]=1.0f;eMulVector4(world,localToWorld,center);
        eMulVector4(camera,view.matrices.worldToView,world);command.sortDepth=camera[2];
    }
    if(flags&0x140u) {
        Require(lightContext && material && strip.normals.size()==strip.positions.size(),
            "Reflected strip lacks its original normal/material/light context");
        command.orderingTexture=&texture;
        if(command.depthSorted)command.alphaGroup=++nextAlphaGroup_;
        commands_.push_back(command);
        // Source VU0618 updates the base alpha, then0348/0350 submits
        // that base packet. The separate0320 context reuses its ST/RGBA
        // with the real SpecularFalloff texture and destination-alpha ALPHA58.
        if(flags&0x40u) {
            Require(specularFalloffTexture_ && specularFalloffTexture_->pixels.Data(),
                "Specular material has no authored SpecularFalloff texture");
            EcstasyDrawCommand specular=command;specular.kind=EcstasyDrawKind::SolidSpecularStrip;
            specular.texture=specularFalloffTexture_;specular.rasterState={{0x01010200u,64,0,0}};
            commands_.push_back(specular);
        }
        if(!(flags&0x100u))return;
        EcstasyDrawCommand reflection=command;reflection.kind=EcstasyDrawKind::SolidReflectionStrip;
        // PAL10B08C..10B0F8: only the second reflection context outside
        // the frontend uses these five writable ALPHA selectors. Material
        // ALPHA and the first reflection context keep their own source state.
        const auto& blend=nativeRenderPolicy.reflectionBlend;
        const bool overrideReflection=(flags&4u) && environmentApplicationState_!=3;
        const auto reflectionAlpha=static_cast<std::uint32_t>(blend[0]) |
            (static_cast<std::uint32_t>(blend[1])<<8) |
            (static_cast<std::uint32_t>(blend[2])<<16) |
            (static_cast<std::uint32_t>(blend[3])<<24);
        reflection.rasterState={{overrideReflection ? reflectionAlpha:0x01000200u,
            static_cast<std::uint32_t>(environmentApplicationState_==3 ? 64:overrideReflection ? blend[4]:16),0,0}};
        if(environmentApplicationState_==3) {
            Require(frontendEnvironmentTexture_ && frontendEnvironmentTexture_->pixels.Data(),
                "Frontend reflected strip has no loaded CarSelectEnvMap texture");
            reflection.texture=frontendEnvironmentTexture_;
        } else {
            Require(environment_,"Reflected strip has no original environment-map owner");
            reflection.texture=nullptr;reflection.sampledTarget=&environment_->MappedTarget();
            reflection.sampledPreviousFrame=!environmentUpdated_;
        }
        commands_.push_back(reflection);
    } else commands_.push_back(command);
}

// Evaluated directly from authored exponent/amplitude pairs on the PC CPU.
// The per-material table avoids expensive powf calls for every lit vertex.
// The fallback covers programmatically supplied test/editor materials.
static float NativeSpecularResponse(const EditorMarkerLightMaterial& material,
    float base,bool alphaMaterial)
{
    const auto* parameters=material.NativeCoefficients();
    const float exponent=parameters[alphaMaterial ? 12:0];
    const float amplitude=parameters[alphaMaterial ? 13:1];
    base=std::max(base,parameters[2]);
    if(material.nativeSpecularReady && base>=0.1f && base<=1.0f) {
        constexpr float kScale=(EditorMarkerLightMaterial::kNativeResponseSamples-1)/0.9f;
        const float scaled=(base-0.1f)*kScale;
        const auto index=std::min(static_cast<std::size_t>(scaled),
            EditorMarkerLightMaterial::kNativeResponseSamples-2);
        const float fraction=scaled-static_cast<float>(index);
        const auto& values=material.nativeSpecularResponse[alphaMaterial ? 1:0];
        return values[index]+fraction*(values[index+1]-values[index]);
    }
    return amplitude*std::pow(base,exponent);
}

// A material/light pair is immutable while its sealed frame is gathered.
// Prepare these products once for a strip group; the vertex evaluator keeps
// the original diffuse/specular addition and quantization order unchanged.
struct NativeVertexLighting {
    std::array<float,4> ambient{};
    std::array<std::array<float,3>,4> diffuse{},specular{};
    bool materialAlpha{},litSpecular{},alphaSpecular{},needReflection{};
};
static NativeVertexLighting PrepareNativeVertexLighting(const SceneLightContext& context,
    const EditorMarkerLightMaterial& material,std::uint32_t flags)
{
    const auto* coefficients=material.NativeCoefficients();
    const bool materialAlpha=(flags&4u)!=0;
    const bool rgbSpecular=coefficients[8]!=0.0f || coefficients[9]!=0.0f ||
        coefficients[10]!=0.0f;
    const bool litSpecular=(flags&0x80u) && (rgbSpecular ||
        (materialAlpha && coefficients[15]!=0.0f));
    const bool alphaSpecular=(flags&0x40u) && coefficients[15]!=0.0f;
    const bool needReflection=litSpecular || alphaSpecular;
    NativeVertexLighting result;
    result.materialAlpha=materialAlpha;result.litSpecular=litSpecular;
    result.alphaSpecular=alphaSpecular;result.needReflection=needReflection;
    if(flags&0x80u)for(std::size_t axis=0;axis<4;++axis) {
        result.ambient[axis]=axis==3 && materialAlpha ? coefficients[14]:context.colorMatrix[12+axis];
        for(std::size_t light=0;light<3;++light) {
            result.diffuse[axis][light]=axis<3 ? context.colorMatrix[light*4+axis]*coefficients[4+axis]:
                (materialAlpha ? 0.0f:context.colorMatrix[light*4+axis]);
            result.specular[axis][light]=axis<3 ? context.colorMatrix[light*4+axis]*coefficients[8+axis]:
                (materialAlpha && light==0 ? coefficients[15]:0.0f);
        }
    }
    return result;
}
static std::array<float,4> EvaluateNativeVertexColor(const SceneLightContext& context,
    const EditorMarkerLightMaterial& material,const float position[3],const float normal[3],
    std::uint16_t color,std::uint32_t flags,const NativeVertexLighting& lighting,
    const float* preparedReflection=nullptr)
{
    // Native PC diffuse/specular evaluation. Retain authored lights/material
    // coefficients and the original channel accumulation/blend order.
    const auto* coefficients=material.NativeCoefficients();
    const bool materialAlpha=lighting.materialAlpha;
    const bool litSpecular=lighting.litSpecular;
    const bool alphaSpecular=lighting.alphaSpecular;
    const bool needReflection=lighting.needReflection;
    float reflection[3]{};
    if(needReflection && preparedReflection)std::copy(preparedReflection,preparedReflection+3,reflection);
    else if(needReflection) {
        float relative[3],twiceDot=0.0f;
        for(std::size_t axis=0;axis<3;++axis) {
            relative[axis]=position[axis]-context.localEye[axis];
            twiceDot+=relative[axis]*normal[axis];twiceDot+=relative[axis]*normal[axis];
        }
        float squared=0.0f;
        for(std::size_t axis=0;axis<3;++axis) {
            reflection[axis]=relative[axis]-normal[axis]*twiceDot;
            squared+=reflection[axis]*reflection[axis];
        }
        const float inverseLength=squared>0.0f ? 1.0f/std::sqrt(squared):0.0f;
        for(auto& axis:reflection)axis*=inverseLength;
    }
    std::array<float,4> output{{static_cast<float>(color&31u)/16.0f,
        static_cast<float>((color>>5)&31u)/16.0f,static_cast<float>((color>>10)&31u)/16.0f,1}};
    // Both released strip dispatchers gate VU1998 on bit0x80 (VU02B8
    // and04E0). Bit0x40 independently requests the alpha-only VU0618
    // pass below. Do not calculate a full three-light color only to replace
    // every channel with the unchanged authored color for that path.
    if(flags&0x80u) {
      std::array<float,3> diffuse{},specular{};
      for(std::size_t light=0;light<3;++light) {
        float normalDot=0.0f;
        for(std::size_t axis=0;axis<3;++axis)
            normalDot+=context.directionMatrix[axis*4+light]*normal[axis];
        diffuse[light]=std::max(normalDot,0.0f);
        if(litSpecular) {
            float reflectionDot=0.0f;
            for(std::size_t axis=0;axis<3;++axis)
                reflectionDot+=context.directionMatrix[axis*4+light]*reflection[axis];
            specular[light]=NativeSpecularResponse(material,reflectionDot,materialAlpha);
        }
      }
      for(std::size_t axis=0;axis<4;++axis) {
        float value=lighting.ambient[axis];
        for(std::size_t light=0;light<3;++light) {
            value+=lighting.diffuse[axis][light]*diffuse[light];
        }
        for(std::size_t light=0;light<3;++light) {
            value+=lighting.specular[axis][light]*specular[light];
        }
        if(axis<3) {
            const float authored=static_cast<float>(((color>>(axis*5))&31u)*8u);
            value*=1.0f+(authored-255.0f)*coefficients[3];
        }
        Require(std::isfinite(value),"Released material lighting produced a non-finite color");
        output[axis]=static_cast<float>(static_cast<std::int32_t>(std::clamp(value,0.0f,255.0f)))/128.0f;
      }
    }
    if(flags&0x40u) {
        // VU0618..09C0 is a subsequent alpha-only write: the RGB from
        // VU1998 (when enabled) or the authored V4_5 stream is unchanged.
        float dot=0;
        if(alphaSpecular)
            for(std::size_t axis=0;axis<3;++axis)dot+=context.directionMatrix[axis*4]*reflection[axis];
        const float specularAlpha=alphaSpecular ? NativeSpecularResponse(material,dot,true):0.0f;
        const float alpha=coefficients[14]+coefficients[15]*specularAlpha;
        Require(std::isfinite(alpha),"Released specular alpha is non-finite");
        output[3]=static_cast<float>(static_cast<std::int32_t>(std::clamp(alpha,0.0f,255.0f)))/128.0f;
    }
    return output;
}

std::array<float,4> EcstasyLitVertexColor(const SceneLightContext& context,
    const EditorMarkerLightMaterial& material,const float position[3],const float normal[3],
    std::uint16_t color,std::uint32_t flags)
{
    const auto lighting=PrepareNativeVertexLighting(context,material,flags);
    return EvaluateNativeVertexColor(context,material,position,normal,color,flags,lighting);
}

static EcstasyReflectionVertex ReflectNativeVertex(const SceneLightContext& context,
    const EditorMarkerLightMaterial& material,const float position[3],const float normal[3],
    std::uint16_t color,std::uint32_t flags,const NativeVertexLighting* lighting,
    bool reuseBaseColor=false)
{
    // Native PC sphere-map direction. No console reciprocal-root or VU
    // bit interpretation remains in the vertex hot path.
    const auto inverseLength=[](float squared) {
        return squared>0.0f ? 1.0f/std::sqrt(squared):0.0f;
    };
    float relative[3],reflection[3];float twiceDot=0.0f;
    for(std::size_t axis=0;axis<3;++axis) {
        relative[axis]=position[axis]-context.localEye[axis];
        twiceDot+=relative[axis]*normal[axis];twiceDot+=relative[axis]*normal[axis];
    }
    float squared=0.0f;
    for(std::size_t axis=0;axis<3;++axis) {
        reflection[axis]=relative[axis]-normal[axis]*twiceDot;
        squared+=reflection[axis]*reflection[axis];
    }
    const auto inverse=inverseLength(squared);
    for(auto& axis:reflection)axis*=inverse;
    float mapped[3];
    for(std::size_t axis=0;axis<3;++axis) {
        // The VU negates x,z,w of every matrix column before multiplication.
        const float sign=axis==1 ? 1.0f:-1.0f;
        mapped[axis]=(context.environmentMatrix[axis]*sign)*reflection[0];
        mapped[axis]+=(context.environmentMatrix[4+axis]*sign)*reflection[1];
        mapped[axis]+=(context.environmentMatrix[8+axis]*sign)*reflection[2];
    }
    const float z=mapped[2]+1.0f;
    squared=mapped[0]*mapped[0]+mapped[1]*mapped[1];squared+=z*z;
    const float sphereInverse=inverseLength(squared);
    EcstasyReflectionVertex output;
    output.uv={{0.5f+(mapped[0]*sphereInverse)*0.5f,0.5f+(mapped[1]*sphereInverse)*0.5f}};
    output.color={{1,1,1,1}};
    if((flags&4u) && !reuseBaseColor) {
        if(flags&0x80u) {
            Require(lighting!=nullptr,"Reflected vertex lost its prepared material lighting");
            output.color=EvaluateNativeVertexColor(context,material,position,normal,color,flags,*lighting,reflection);
        }
        else output.color={{static_cast<float>(color&31u)/16.0f,
            static_cast<float>((color>>5)&31u)/16.0f,static_cast<float>((color>>10)&31u)/16.0f,1}};
    }
    const auto* coefficients=material.NativeCoefficients();
    const float alpha=coefficients[7]+coefficients[11]*mapped[2];
    Require(std::isfinite(alpha),"Environment material produced non-finite alpha");
    output.color[3]=static_cast<float>(static_cast<std::uint32_t>(convertFloatToInt32(alpha))&255u)/128.0f;
    return output;
}

EcstasyReflectionVertex EcstasyReflectVertex(const SceneLightContext& context,
    const EditorMarkerLightMaterial& material,const float position[3],const float normal[3],
    std::uint16_t color,std::uint32_t flags)
{
    if((flags&4u) && (flags&0x80u)) {
        const auto lighting=PrepareNativeVertexLighting(context,material,flags);
        return ReflectNativeVertex(context,material,position,normal,color,flags,&lighting);
    }
    return ReflectNativeVertex(context,material,position,normal,color,flags,nullptr);
}

std::size_t EcstasyFrame::RenderModel(EditorMarkerModel& model,const float localToWorld[16],
    const CarCinematicScreen& view,std::uint32_t flags,float alphaScale,std::int32_t worldFrames,
    const SceneLightContext* lightContext,const std::vector<std::array<float,16>>* boneMatrices)
{
    // PAL10DC80 delegates rigid animation before culling and texture-table
    // replacement before strip dispatch. The source's temporary DMA memory
    // is replaced by frame-owned matrix values; all geometry stays borrowed.
    if (!model.solid)return 0;
    Require(localToWorld && model.solid->geometry,"Rendered solid has no decoded geometry owner");
    float animated[16];
    if (!boneMatrices && model.solid->rigidAnimation && model.rigidAnimationStart) {
        const auto time=editorMarkerModelRigidAnimationTime(&model,worldFrames);
        if (time<0)model.rigidAnimationStart=0;
        else {
            float sample[16];
            Require(editorMarkerRigidAnimationMatrixAtTime(model.solid->rigidAnimation,sample,time)!=0,
                "Rigid model animation has no authored frame");
            eMulMatrix(animated,sample,localToWorld);localToWorld=animated;
        }
    }
    editorMarkerModelSetupVertexAnimation(&model,worldFrames);
    auto& visible=visibleScratch_;
    struct ClearVisible {std::vector<EcstasyVisibleStrip>& value;~ClearVisible(){value.clear();}} clear{visible};
    if (!ecstasyDetermineVisibleStrips(visible,model,view,localToWorld,boneMatrices!=nullptr,
        boneMatrices ? 1:((flags&4u) ? 2:1)))return 0;
    if (model.replacement)editorMarkerModelApplyReplacementTextureTable(&model);
    const auto& geometry=*model.solid->geometry;
    const auto start=commands_.size();
    float localToClip[16];
    bool localToClipReady=false;
    for (const auto& run:visible)for (std::size_t index=run.first;index<run.first+run.count;++index) {
        Require(index<geometry.strips.size(),"Visible strip run leaves its actual model");
        const auto& strip=geometry.strips[index];
        if (strip.flags&0x400u)continue;
        const auto& descriptor=stripTexture(geometry,strip);
        const auto* texture=textureInfoForDescriptor(&descriptor);
        Require(texture,"Model texture is absent from the canonical TexturePack");
        // Final RenderStrips10DF1C..10DF50: target's alpha-only/opaque-only
        // passes are encoded by caller flags8/16; no second strip list.
        if ((flags&8u) && !texture->requiresDepthSort)continue;
        if (((flags&16u) || view.target->opaqueOnly) && texture->requiresDepthSort)continue;
        const EditorMarkerLightMaterial* material=nullptr;
        if (strip.lightMaterialIndex>=0 &&
            static_cast<std::size_t>(strip.lightMaterialIndex)<geometry.lightMaterials.size())
            material=geometry.lightMaterials[static_cast<std::size_t>(strip.lightMaterialIndex)];
        if (!material && model.resources) {
            const auto* root=model.resources->catalogueRoot ? model.resources->catalogueRoot:model.resources;
            material=root->defaultLightMaterial;
        }
        // Preserve the source strip flags and its texture4F/final74 predicate.
        auto draw=static_cast<std::uint32_t>(strip.flags)|(texture->drawFlag4Enabled ? 4u:0u);
        if (run.clipClass==1)draw|=0x200u;
        if ((flags&1u) || !environmentMappingEnabled)draw&=~0x100u;
        if (flags&2u)draw&=~0x80u;
        if (!lightContext)draw&=~0x1c0u; // released10DDE0 null-context mask
        const auto* submitted=&strip;
        if(boneMatrices) {
            // PAL10D778 DoBlendingOnStrip copies the actual strip packet and
            // changes XYZ only. Its three embedded low bytes are two bone
            // indices and an unscaled8-bit blend value, not extra channels.
            auto animatedStrip=std::make_unique<EditorMarkerSolidGeometry::Strip>(strip);
            for(auto& position:animatedStrip->positions) {
                std::array<std::uint32_t,3> encoded{};
                std::memcpy(encoded.data(),position.data(),sizeof(encoded));
                const auto first=encoded[0]&255u,second=encoded[1]&255u,weight=encoded[2]&255u;
                const auto transformVertex=[&](std::uint32_t bone,float output[4]) {
                    Require(bone<boneMatrices->size(),"Animated vertex bone exceeds its retained skeleton");
                    eMulVector3(output,(*boneMatrices)[bone].data(),position.data());
                };
                float posed[4];
                if(weight==0)transformVertex(first,posed);
                else if(weight==1)transformVertex(second,posed);
                else {
                    float other[4];transformVertex(first,posed);transformVertex(second,other);
                    const float blend=static_cast<float>(weight)*(1.0f/256.0f);
                    for(std::size_t axis=0;axis<3;++axis)
                        posed[axis]=posed[axis]*(1.0f-blend)+other[axis]*blend;
                }
                std::copy(posed,posed+3,position.begin());
            }
            submitted=animatedStrip.get();animatedStrips_.push_back(std::move(animatedStrip));
        }
        // Neither texture lookup, strip skinning nor RenderStrip mutates
        // this model transform or the view. Reuse the same multiplication
        // within this call, after its rigid animation has been applied.
        if(!localToClipReady) {
            eMulMatrix(localToClip,localToWorld,view.matrices.worldToClip);
            localToClipReady=true;
        }
        RenderStrip(model,*submitted,*texture,material,localToWorld,view,run.clipClass,draw,alphaScale,lightContext,localToClip);
    }
    return commands_.size()-start;
}

void EcstasyDisplayFrameRate(FlailerResources& resources,
    FlailerRenderManager& render,const CarCinematicScreen& view,EcstasyFrame& frame)
{
    // Independent PC overlays use the actual global font. The FPS toggle
    // never gates DEBUG, and DEBUG never supplies the displayed frame rate.
    const auto* debugCaption=applicationDebugToolsEnabled?joystickNativeDebugCaption():nullptr;
    if(!applicationShowFrameRate && !debugCaption)return;
    constexpr std::uint32_t fontKey=0x5C10A676u;
    const auto* font=resources.FindFont(fontKey);
    if(!font || !font->GetTextureInfo(resources))return;
    const auto drawCaption=[&](const char* caption,float y) {
        FlailerTextChunk text;
        text.kind=1;text.visible=1;text.fontKey=fontKey;text.formatOptions=2;
        text.topLeftX=8.0f;text.topLeftY=y;text.scaleX=text.scaleY=0.75f;
        const auto length=std::strlen(caption);
        Require(length<std::numeric_limits<std::uint32_t>::max(),"Native overlay text exceeds its owned capacity");
        text.color=0xffffffffu;text.textCapacity=static_cast<std::uint32_t>(length+1);
        text.ownedText.resize(text.textCapacity+1,0);text.text=text.ownedText.data();text.SetString(caption);
        try {
            render.RenderFlailerText(resources,text,nullptr,nullptr,nullptr,1.0f);
            render.RenderLayer(0,view,frame);
        } catch(...) {text.DeleteFlailerStrip();throw;}
        // The captured frame retains the actual strip until retirement.
        text.DeleteFlailerStrip();
    };
    if(applicationShowFrameRate) {
        char caption[48]{};
        if(applicationFrameTiming.frameRateAvailable)
            std::snprintf(caption,sizeof(caption),"FPS: %.1f",applicationFrameTiming.framesPerSecond);
        else std::snprintf(caption,sizeof(caption),"FPS: --");
        drawCaption(caption,8.0f);
    }
    if(debugCaption)drawCaption(debugCaption,applicationShowFrameRate?24.0f:8.0f);
}

void EcstasyDisplayFrame(const EcstasySceneBindings& scene,EcstasyFrame& frame)
{
    // PAL112858: retain original environment/world/car/filter/HUD phases.
    // Native packets capture the state that the console consumed immediately;
    // the GPU endpoint owns only images, dependencies and completion fences.
    Require(frame.State()==EcstasyFrame::Phase::Building && scene.clock &&
        scene.ui.resources && scene.ui.render,"eDisplayFrame lacks its live frame/UI owners");
    frame.SetPresentationAdvance(scene.advancePresentation);
    frame.CaptureDisplayOffsets(*scene.clock); // original ePutDisplayEnvironment reads both source offsets
    auto& views=scene.views;auto& render=*scene.ui.render;
    // PAL11293C..1129C8 writes the physical screen Z format before
    // UpdateGifTags105BC8..105C5C propagates it to logical targets0..5.
    // The rear target7 owns a separate PSMZ16S buffer (SetScreenBuffers
    //106B0C..106B78); environment targets8..14 keep their own PSMZ16.
    const auto screenDepthFormat=scene.flow && scene.flow->inGameFlag ? 0x31:0x3a;
    for(std::size_t index=0;index<3;++index) {
        auto& view=views.screens[index];
        if(view.target)view.target->format=screenDepthFormat;
        if(view.auxiliaryTarget)view.auxiliaryTarget->format=screenDepthFormat;
    }
    if(views.screens[3].target)views.screens[3].target->format=0x3a;
    auto* resources=scene.ui.resources->Models().resourceRegistry;
    Require(resources,"Display frame has no canonical resource owner");
    auto* simulation=scene.track ? scene.track->Simulation():nullptr;
    const auto applicationState=scene.applicationState ? *scene.applicationState:0;
    if(scene.advancePresentation && scene.environmentMap && scene.environmentMap->enabled)
        UpdateEnvironmentMapCameras(*scene.environmentMap,views,simulation ? simulation->world:nullptr,
            scene.track ? scene.track->Players():nullptr,applicationState,scene.clock->realTimeElapsed);
    frame.CaptureEnvironment(scene.environmentMap,*resources,applicationState,views.renderFrameCounter,
        scene.flow && scene.flow->inGameFlag!=0);
    ecstasyViewsUpdateMode(&views);
    ecstasyViewsApplyMode(&views);
    const auto active=[](const CarCinematicScreen& view) {
        return view.enabled && view.view && view.target && view.target->enabled;
    };
    for(auto& view:views.screens)if(active(view))
        Require(ecstasyViewCalculateMatrices(&views,&view,0,0,0)==CAR_CINEMATIC_OK,
            "Original view projection could not be calculated");
    if(scene.screens) {
        Require(scene.frontendFrame,"Screen model callbacks have no actual frontend frame owner");
        // Clear the previous frame before BeginFrame enqueues current UI.
        // CompleteSubmissions must not erase newly generated clipped text.
        scene.frontendFrame->Clear();
    }
    render.BeginFrame(*scene.ui.resources,scene.ui.language,
        static_cast<float>(scene.clock->realTimer)*0.0002500000118743628f);
    if(scene.screens)scene.screens->RenderModels(scene.advancePresentation);
    // PAL112C1C: opaque frontend cars precede the sky/background. Their
    // owner targets player view1/2; view0 remains the Flailer camera.
    if(scene.applicationState && *scene.applicationState==3)
        for(std::size_t i=1;i<3;++i)if(active(views.screens[i])) {
            ecstasyViewSetCurrent(&views,&views.screens[i]);
            RenderFrontEndCars(views.screens[i],frame,scene.clock->worldTimeFrames,
                views.presentationLayout,true,false,true,false);
        }
    frame.AttachAlphaPolys();
    auto& frontend=views.screens[0];
    const auto renderFlailerLayer=[&](std::int32_t layer) {
        // PAL112F78/113B58/113B98/113BE0: each nonempty UI layer
        // immediately flushes both source lists before the next layer.
        // Texture priorities apply within that boundary. Deferring all UI
        // until one final flush moves the priority7 preview fade after the
        // nearer logo/help/thumbnail layers and makes the whole shop blink.
        if(render.RenderLayer(layer,frontend,frame)) {
            frame.AttachSolidPolys();frame.AttachAlphaPolys();
        }
    };
    const auto* sky=scene.track ? scene.track->Sky():nullptr;
    // PAL112E38 disables Z writes for the sky packet group; PAL112F24
    // restores them after its flush. Depth testing remains enabled, so
    // opaque frontend cars already submitted can still occlude the sky.
    frame.SetZWriteState(0);
    if(sky)for(std::size_t i=1;i<10;++i) {
        auto& view=views.screens[i];if(!active(view))continue;
        Require(ecstasyViewCalculateMatrices(&views,&view,100.0f,50000.0f,0)==CAR_CINEMATIC_OK,
            "Original sky projection could not be calculated");
        ecstasyViewSetCurrent(&views,&view);sky->StuffSky(view,frame,scene.clock->worldTimeFrames);
    }
    frame.AttachAlphaPolys();
    frame.SetZWriteState(1);
    if(active(frontend))for(std::int32_t layer=24;layer>=20;--layer)
        renderFlailerLayer(layer);
    frame.AttachAlphaPolys();
    // PAL1130F0 reads the actual InGameFlag330540 before adding any
    // scenery views. A decoded track file alone does not enable drawing:
    // textures/fixups and enabled sound owners are still loading then.
    // EndGameFlowLoadingTrackSpecific owns publication of this same flag.
    Require(!scene.track || scene.flow,"Track drawing has no original GameFlow owner");
    auto* scenery=scene.flow && scene.flow->inGameFlag && scene.track &&
        scene.track->trackFile && scene.track->trackFile->Loaded() ?
        scene.track->trackFile->Resources().scenery:nullptr;
    for(std::size_t i=1;i<10;++i) {
        auto& view=views.screens[i];if(!active(view))continue;
        // PAL113030..11304C supplies32 only to the cubemap culling far
        // override. Raster/depth projection retains the actual camera range.
        const float clipFar=view.id>=4 && view.id<10 ? 32.0f:0.0f;
        Require(ecstasyViewCalculateMatrices(&views,&view,0,0,clipFar)==CAR_CINEMATIC_OK,
            "Original world projection could not be calculated");
        ecstasyViewSetCurrent(&views,&view);
        // PAL10F928 reads TheRaceParameters+0C (the retained reverse owner).
        // Its0/1 gates are independent of the auxiliary-view bits.
        std::uint32_t mask=(views.presentationLayout==3 || views.presentationLayout==4) ? 0x10u:0x11u;
        if(view.id>=4 && view.id<10)mask|=0x102u;
        if(scenery) {
            Require(scene.parameters,"Scenery source filter has no live RaceParameters");
            if(scene.parameters->reverse==0)mask|=8u;
            if(scene.parameters->reverse==1)mask|=4u;
            scenery->Render(view,frame,mask,scene.clock->worldTimeFrames);
        }
        // PAL113250 follows StuffScenery11321C and precedes the sun and
        // active camera mover. NIS actors retain their actual replay poses.
        if(i<4 && scene.track && scene.track->NisManagers())
            (void)DrawNisObjects(*scene.track->NisManagers(),view,frame);
        // PAL113284 follows NIS and precedes RenderClans. Only the source
        // transfer operation is gated by InGameFlag; glare owns its timer.
        if(i<4)resources->Sun().StuffSun(view,frame,*scene.clock,
            scene.flow && scene.flow->inGameFlag!=0);
        // PAL1132C4 -> RenderClans1AB810 consumes the original world's
        // live SkidSets on primary views1..3. The source1AACC0 polygons
        // already contain world positions and the authored material texture.
        if(i<4 && simulation && simulation->world) {
            std::vector<SkidRenderQuad> skidQuads;
            WorldClans(*simulation->world).PrepareRender(view.matrices.worldToClip,view,skidQuads);
            for(const auto& quad:skidQuads) {
                FlailerPoly polygon;polygon.rasterState=quad.rasterState;
                for(std::size_t vertex=0;vertex<quad.vertices.size();++vertex) {
                    polygon.vertices[vertex].position=quad.vertices[vertex].position;
                    polygon.vertices[vertex].textureCoordinate=quad.vertices[vertex].textureCoordinate;
                    polygon.vertices[vertex].color=quad.vertices[vertex].color;
                }
                frame.RenderPolygon3D(polygon,textureInfoForDescriptor(quad.texture),eMathIdentityMatrix,view);
            }
        }
        // PAL11333C consumes the active (native last) mover virtual Render.
        // ZonePreview emits into FrameEffects; ordinary camera Render is empty.
        if(i<4)Require(carCinematicRenderMovers(&view,&frame)==CAR_CINEMATIC_OK,
            "Camera mover rendering lost its canonical active owner");
        if(i<4 && scene.track && scene.track->Players())
            Require(carFramePlayerRenderPlayers(scene.track->Players(),&view,&frame)!=0,
                "Player visible driving line lost its original owner");
        // PAL11334C: the original WorldObject list follows scenery on the
        // primary views1..3 and precedes RenderFlailerModels. Fern shakers
        // hide their authored static placement while this very model lives.
        if(i<4 && simulation && simulation->world)
            (void)RenderWorldObjects(WorldObjects(*simulation->world),view,frame,
                scene.clock->worldTimeFrames);
        // PAL11335C follows world objects with the authored light-flare pack.
        if(i<4 && scene.track && scene.flow)
            (void)RenderWorldLightFlares(scene.track->files.Registry().Lights(),
                scene.track->files.Registry(),view,frame,scene.flow->inGameFlag!=0);
    }
    // The source RenderFlailerModels stage follows background/scenery and
    // precedes RenderCars. Each queued model retains its actual view; the
    // view0 Flailer projection must not replace the track/car preview camera.
    if(scene.frontendFrame && !scene.frontendFrame->models.empty()) {
        for(const auto& request:scene.frontendFrame->models) {
            Require(request.model && request.viewNumber>=0 && request.viewNumber<10,
                "Frontend model request lost its actual resource or view owner");
            auto& modelView=views.screens[request.viewNumber];
            Require(active(modelView),"Frontend model request targets a disabled original view");
            ecstasyViewSetCurrent(&views,&modelView);
            frame.RenderModel(*const_cast<EditorMarkerModel*>(request.model),request.transform.data(),
                modelView,0,1.0f,scene.clock->worldTimeFrames);
        }
        scene.frontendFrame->modelDrawCpuClosureRequired=false;
    }
    // PAL113378/113388: separate FrameEffects and WorldEffects lists share
    // allocation pools, not live particles. Both borrow the retained catalogue.
    if(scene.effects) {
        auto& catalogue=resources->Effects();
        for(std::size_t i=1;i<4;++i)if(active(views.screens[i])) {
            ecstasyViewSetCurrent(&views,&views.screens[i]);
            (void)RenderWorldParticles(scene.effects->FrameParticles(),catalogue,views.screens[i],frame);
            (void)RenderWorldParticles(scene.effects->Particles(),catalogue,views.screens[i],frame);
        }
    }
    // PAL1134A4 calls AttachSolidPolys10BB20, not AttachAlphaPolys10B838.
    // Keep the particles/scenery alpha list pending until after car solids.
    // The scenery reflected strips read the previous mapped image; later
    // cars read the current update. Capture that version in each packet.
    frame.AttachSolidPolys();
    if(scene.environmentMap && scene.environmentMap->enabled && scene.flow && scene.flow->inGameFlag &&
        active(frontend))scene.environmentMap->CreatePrimitives(frame);
    frame.AttachSolidPolys(); // PAL1135B8: remap solid chain only.
    if(simulation && simulation->world)for(std::size_t i=1;i<4;++i) {
        auto& view=views.screens[i];if(!active(view))continue;
        ecstasyViewSetCurrent(&views,&view);
        Require(scene.parameters && scene.applicationState && scene.track->Players(),
            "RenderCars damage pass lacks the original race/player owners");
        Require(scene.track->Session(),"Car damage display has no canonical RaceCoordinator");
        const CarDamageDisplayBindings display{scene.track->files.Registry(),*scene.parameters,
            scene.track->Session()->state,*scene.track->Players(),simulation->pursuit,view,frame};
        RenderCars(*simulation->world,view,frame,*scene.clock,views.presentationLayout,&display,scene.track->ViewMovers());
    }
    // PAL1136BC/1136D4: reflected frontend cars, then the transparent
    // normal pass, after the ordinary race-car pass on views1..3.
    for(std::size_t i=1;i<4;++i)if(active(views.screens[i])) {
        ecstasyViewSetCurrent(&views,&views.screens[i]);
        RenderFrontEndCars(views.screens[i],frame,scene.clock->worldTimeFrames,
            views.presentationLayout,false,true,true,true);
        RenderFrontEndCars(views.screens[i],frame,scene.clock->worldTimeFrames,
            views.presentationLayout,true,false,false,true);
    }
    frame.AttachSolidPolys(); // PAL113714 completes the car solid chains.
    // PAL1137CC flushes the shared world alpha list after particles AND cars.
    // OverlayRVMImages1137F4 has its own following solid/alpha flush, before
    // DisplayRVMs later samples the completed offscreen image.
    frame.AttachAlphaPolys();
    if(scene.track)frame.OverlayRVMImages(views,scene.track->files.Registry());
    frame.AttachAlphaPolys();
    // PAL113818..113880: cockpit models use the third captured projection,
    // then complete their own solid/alpha stage before the depth effects.
    for(std::size_t i=1;i<4;++i)if(active(views.screens[i])) {
        ecstasyViewSetCurrent(&views,&views.screens[i]);
        RenderDashboards(views.screens[i],frame,scene.clock->worldTimeFrames);
    }
    frame.AttachSolidPolys();frame.AttachAlphaPolys();
    if(scene.flow && scene.flow->inGameFlag) {
        frame.AttachFog(views);
        frame.AttachDepthOfField(views);
    }
    frame.AttachBlackAndWhite(views); // PAL113900, before DisplayFilter113A44.
    // PAL1139F0..113A8C: mask alpha precedes destination-alpha history blend.
    frame.SetZWriteState(0);
    if(resources)frame.DisplayFilter(views,*resources);
    frame.DisplayRadialBlur(views);
    frame.DisplayBlendPrevious(views);
    frame.SetZWriteState(1);
    // Source113A90..113D80: effect-free HUD packets, followed by resumption
    // of the selected mode. Each packet captures its issuing render policy;
    // later Gather cannot leak the resumed scene state into these HUD draws.
    SuspendRenderEggs();
    try {
        if(active(frontend)) {
            ecstasyViewSetCurrent(&views,&frontend);
            // PAL113AEC..113B48 disables Z writes around DisplayRVMs, flushes
            // that target-texture polygon, then restores writes before the HUD.
            frame.SetZWriteState(0);frame.DisplayRVMs(views,frontend);frame.AttachAlphaPolys();frame.SetZWriteState(1);
            for(std::int32_t layer=15;layer>=0;--layer)renderFlailerLayer(layer);
            for(std::int32_t layer=19;layer>=16;--layer)renderFlailerLayer(layer);
            for(std::int32_t layer=28;layer>=25;--layer)renderFlailerLayer(layer);
            // Final113C1C..113C50 visits pPlayersByIndex after the HUD layers
            // and calls Player::RenderTrackMap181C08 for every live Player.
            // TrackMap's native immediate polygons use the same layer transport;
            // flush those newly submitted polygons before retiring this frame.
            if(auto* players=scene.track ? scene.track->Players():nullptr) {
                Require(players->count>=0 && players->count<=2,"TrackMap player count is outside the original table");
                for(std::int32_t index=0;index<players->count;++index) {
                    Require(players->players[index],"TrackMap player table has no live owner");
                    carFramePlayerRenderTrackMap(players->players[index]);
                }
                for(std::int32_t layer=15;layer>=0;--layer)renderFlailerLayer(layer);
            }
        }
        if(active(frontend)) {
            EcstasyDisplayFrameRate(*scene.ui.resources,render,frontend,frame);
            frame.AttachAlphaPolys();
        }
        render.EndFrame();
    } catch(...) {
        // Native allocation/resource errors must not leave Stomper suspended
        // after an aborted UI frame. Normal resource ownership remains live.
        ResumeRenderEggs();
        throw;
    }
    ResumeRenderEggs();
    frame.AttachAlphaPolys();
    // PAL113FD8 is after every current-frame texture context. Native Gather
    // reads borrowed pixels later, so the equivalent update follows CPU capture.
    if(scene.advancePresentation && !scene.deferPresentationUpdates) {
        if(scene.effects)Require(worldParticlesAdvance(&scene.effects->FrameParticles(),scene.clock->realTimeElapsed)==WORLD_SERVICES_OK,
            "FrameEffects lost its original real-time update owner");
        frame.AdvanceTextureAnimationsAfterCompletion(*resources,*scene.clock);
    }
    ++views.renderFrameCounter; // PAL11405C/114064/114090 eFrameCounter, not VBlank/GameTime.
}

void EcstasyFrame::RetireStripAfterCompletion(FlailerStrip& strip)
{
    Require(phase_==Phase::Building && strip.pool && strip.retentionCount,
        "Graphics frame requires the actual retained Flailer strip");
    retainedStrips_.push_back(&strip);
}

void EcstasyFrame::Seal()
{
    Require(phase_==Phase::Building,"Graphics frame was already sealed");
    AttachAlphaPolys();phase_=Phase::Ready;
}
void EcstasyFrame::MarkSubmitted()
{
    Require(phase_==Phase::Ready,"Graphics frame is not ready for submission");phase_=Phase::Submitted;
}
void EcstasyFrame::Complete()
{
    Require(phase_==Phase::Submitted,"CPU capture completion has no submitted frame");
    auto* textures=textureAnimationResources_;
    const auto worldFrames=textureAnimationWorldFrames_,realFrames=textureAnimationRealFrames_;
    Release();phase_=Phase::Complete;
    if(textures)texturePackRegistry.UpdateAnimations(textures->Models(),worldFrames,realFrames);
}
void EcstasyFrame::CancelBeforeSubmit()
{
    Require(phase_==Phase::Building || phase_==Phase::Ready,"Submitted graphics frame cannot be cancelled");
    Release();phase_=Phase::Complete;
}
void EcstasyFrame::BeginNextFrame()
{
    Require(phase_==Phase::Complete,"Only a completed graphics frame can be reused");
    Require(commands_.empty() && pendingAlphaCommands_.empty() && orderingScratch_.empty() &&
        visibleScratch_.empty() && depthTransfers_.empty() &&
        retainedStrips_.empty() && worldPolygons_.empty() && worldStrips_.empty() &&
        animatedStrips_.empty() && liveParticleGroups_==0 && !textureAnimationResources_,
        "Completed graphics frame still owns a submitted borrower");
    zWriteEnabled_=true;dashboardProjection_=false;inGameMaterials_=false;pendingAlphaBegin_=0;nextAlphaGroup_=0;nextFeedbackGroup_=0;
    displayTranslation_={};sourceDisplayExtent_={};textureTime_=0;
    environment_=nullptr;frontendEnvironmentTexture_=nullptr;specularFalloffTexture_=nullptr;
    environmentApplicationState_=0;renderFrameCounter_=0;environmentUpdated_=false;advancesPresentation_=true;
    textureAnimationWorldFrames_=0;textureAnimationRealFrames_=0;
    phase_=Phase::Building;
}
void EcstasyFrame::Release() noexcept
{
    textureAnimationResources_=nullptr;
    commands_.clear();pendingAlphaCommands_.clear();orderingScratch_.clear();visibleScratch_.clear();depthTransfers_.clear();
    worldPolygons_.clear();
    for(std::size_t index=0;index<liveParticleGroups_;++index)particleGroups_[index]->clear();
    liveParticleGroups_=0;
    worldStrips_.clear();
    animatedStrips_.clear();
    for (auto* strip:retainedStrips_)strip->pool->Complete(*strip);
    retainedStrips_.clear();
}
std::vector<std::uint8_t> DecodeTextureMipRgba(const TextureInfo& texture,
    std::uint32_t level,std::size_t sourceOffset,std::size_t sourceBytes,
    std::uint32_t& width,std::uint32_t& height,bool repeatedPadding)
{
    const auto* descriptor=texture.descriptor;
    Require(descriptor && descriptor->authoredRecord && descriptor->authoredRecordSize>=164,
        "Vulkan upload lacks the final TextureInfo descriptor");
    const auto* wire=descriptor->authoredRecord;
    const std::uint32_t authoredWidth=ReadU16(wire+0x24)>>level,authoredHeight=ReadU16(wire+0x26)>>level;
    Require(wire[0x48]<=14 && wire[0x49]<=14,"Invalid authored texture log dimensions");
    Require(level<=wire[0x48] && level<=wire[0x49],"Authored mip exceeds its texture dimensions");
    width=(1u<<wire[0x48])>>level;height=(1u<<wire[0x49])>>level;
    Require(authoredWidth && authoredHeight && authoredWidth<=width && authoredHeight<=height,
        "Invalid authored texture dimensions");
    const auto count=static_cast<std::size_t>(authoredWidth)*authoredHeight;
    const auto* source=texture.pixels.Data();const auto* palette=texture.palette.Data();
    const auto psm=wire[0x4a],palettePsm=wire[0x4b];
    Require(source && sourceOffset<=texture.pixels.size && sourceBytes<=texture.pixels.size-sourceOffset,
        "Vulkan upload has no complete retained authored mip");
    source+=sourceOffset;
    // The ordinary PAL109990 downloader chooses a preconverted32-bit stream
    // when this mip's byte55+4*level is nonzero (109CF0..109D20). The static108BB8
    // helper does not take that branch. TEX0 retains the original PSM and
    // power-of-two dimensions (10ABC0). Decode directly into a native image;
    // no console memory, command interpreter or emulated graphics state.
    const std::size_t required=psm==0 || psm==1 ? count*4 : psm==0x13 ? count : psm==0x14 ? (count+1)/2 : 0;
    Require(required && required<=sourceBytes,
        "Vulkan texture format is unsupported or its final pixel extent is truncated");
    std::size_t paletteColors{};
    if (psm==0x13 || psm==0x14) {
        // PAL109B80..109BE0 transfers the authored byte count at+3C.
        // Shipped PSMT8 textures can use only32 RGBA colors (128 bytes).
        // Validate that actual palette and each referenced entry; unused
        // entries need no fabricated colors or full256-entry allocation.
        Require(palette && palettePsm==0 && texture.paletteBytes>0 &&
            !(texture.paletteBytes%4) && texture.palette.size>=static_cast<std::size_t>(texture.paletteBytes),
            "Vulkan indexed texture requires its authored RGBA32 palette");
        paletteColors=static_cast<std::size_t>(texture.paletteBytes)/4u;
    }
    const auto bufferWidth=wire[0x54+4*level];
    const bool converted=wire[0x55+4*level]!=0;
    if(converted)Require((psm==0x13 || psm==0x14) && bufferWidth>=2 &&
        !(bufferWidth&1u) && !(authoredWidth&1u) &&
        authoredHeight%(psm==0x14 ? 4u:2u)==0,
        "Unsupported authored preconverted texture extent");
    // Each table entry names a physical block, not mutable runtime state.
    // GS manual6.0 sections8.3.1/8.3.3/8.3.4 supply the pixel layout.
    static constexpr std::uint8_t blocks4[8][4]={
        {0,2,8,10},{1,3,9,11},{4,6,12,14},{5,7,13,15},
        {16,18,24,26},{17,19,25,27},{20,22,28,30},{21,23,29,31}};
    static constexpr std::uint8_t blocks8[4][8]={
        {0,1,4,5,16,17,20,21},{2,3,6,7,18,19,22,23},
        {8,9,12,13,24,25,28,29},{10,11,14,15,26,27,30,31}};
    // Authored UVs use TEX0's power-of-two extent even when the uploaded
    // rectangle is smaller. Preserve the retained texels before extending
    // only repeated axes below; clamp-only padding remains transparent.
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width)*height*4);
    for (std::size_t pixel=0;pixel<count;++pixel) {
        const std::uint8_t* color{};
        if (psm==0 || psm==1) color=source+pixel*4;
        else {
            std::size_t byteOffset=psm==0x13 ? pixel:pixel/2;
            unsigned shift=psm==0x13 ? 0u:static_cast<unsigned>(pixel&1u)*4u;
            if(converted) {
                const auto x=static_cast<std::uint32_t>(pixel%authoredWidth);
                const auto y=static_cast<std::uint32_t>(pixel/authoredWidth);
                const auto pages=static_cast<std::uint32_t>(bufferWidth)/2u;
                const auto pageHeight=psm==0x14 ? 128u:64u;
                const auto blockWidth=psm==0x14 ? 32u:16u;
                const auto page=x/128u+(y/pageHeight)*pages;
                const auto block=psm==0x14 ? blocks4[(y%128u)/16u][(x%128u)/32u]:
                    blocks8[(y%64u)/16u][(x%128u)/16u];
                const auto column=(y%16u)/4u,cx=x%blockWidth,cy=y%4u;
                const auto word=(cx&1u)+((cy&1u)<<1u)+((cx&2u)<<1u)+
                    ((((cx>>2u)&1u)^(cy>>1u)^(column&1u))<<3u);
                const auto lane=(cx/8u)*2u+cy/2u;
                // Invert the32-bit layout to address the retained export
                // stream directly. This is a format conversion, not VRAM.
                const auto blockX=(block&1u)|((block>>1u)&2u)|((block>>2u)&4u);
                const auto blockY=((block>>1u)&1u)|((block>>2u)&2u);
                const auto sourceX=(page%pages)*64u+blockX*8u+(word&1u)+((word>>1u)&6u);
                const auto sourceY=(page/pages)*32u+blockY*8u+column*2u+((word>>1u)&1u);
                byteOffset=(static_cast<std::size_t>(sourceY)*(authoredWidth/2u)+sourceX)*4u+
                    (psm==0x14 ? lane/2u:lane);
                shift=psm==0x14 ? (lane&1u)*4u:0u;
                Require(byteOffset<required,"Preconverted texture addresses outside its actual authored mip");
            }
            std::uint32_t index=(source[byteOffset]>>shift)&(psm==0x13 ? 255u:15u);
            // Imported PSMT8 palettes retain their on-disk CLUT layout;
            // native-generated movie palettes have no console permutation.
            if(psm==0x13 && !texture.palette.nativeLinearPalette)
                index=(index&~24u)|((index&8u)<<1)|((index&16u)>>1);
            Require(index<paletteColors,"Vulkan indexed pixel exceeds its authored palette");
            color=palette+index*4;
        }
        const auto destination=(pixel/authoredWidth*width+pixel%authoredWidth)*4;
        std::copy_n(color,3,rgba.data()+destination);
        // Retain the original alpha byte. Expanding it to UNORM unity here
        // rounds authored48 upward and loses all values above128. The shader
        // converts the sampled byte to the GS128-unit scale before TFX.
        rgba[destination+3]=psm==1 ? 128u:color[3];
    }
    // Native PC texture conversion: bilinear/mip sampling at a repeated
    // NPOT edge must not mix the last authored texel with fabricated RGBA0.
    // RIVER_TOP is384 wide in a512-wide TEX0 domain. Extend its repeat using
    // actual decoded texels, including their original alpha, at EVERY mip.
    // This replaces console allocation aliases with a defined native image;
    // it does not reconstruct GS memory or change UVs/filter/material state.
    const bool repeatS=authoredWidth<width && (wire[0x4c]&1u)!=0;
    const bool repeatT=authoredHeight<height && (wire[0x4c]&2u)!=0;
    if(repeatedPadding && (repeatS || repeatT))for(std::uint32_t y=0;y<height;++y)for(std::uint32_t x=0;x<width;++x) {
        const bool outsideS=x>=authoredWidth,outsideT=y>=authoredHeight;
        if((!outsideS && !outsideT) || (outsideS && !repeatS) || (outsideT && !repeatT))continue;
        const auto sourcePixel=(static_cast<std::size_t>(y%authoredHeight)*width+x%authoredWidth)*4u;
        const auto destination=(static_cast<std::size_t>(y)*width+x)*4u;
        std::copy_n(rgba.data()+sourcePixel,4,rgba.data()+destination);
    }
    return rgba;
}

} // namespace hp2
void graphicsNotifyPresentationComplete() noexcept {
    graphicsVideoBlankCounter.fetch_add(1,std::memory_order_release);
}
/* Individually qualified coordinator and cinematic view owners. */
int32_t carViewCrossesNearPlane(const CarCinematicView* view,CarMovement* movement,
    uint16_t projectionAngle)
{
    float localToWorld[16],combined[16],minimum[4],maximum[4];
    const VehicleState* state=movement->bindings.state;
    memcpy(localToWorld,state->orientation,sizeof(localToWorld));
    memcpy(localToWorld+12,state->position,3*sizeof(float));localToWorld[15]=1.0f;
    vectorMatrixMultiply(combined,view->current.matrix,localToWorld);
    DriverPhysicsVehicle physics{};physics.movement=movement;
    driverCollisionBounds(&physics,minimum,maximum);
    const uint16_t referenceHalf=(uint16_t)(projectionAngle>>1);
    const float referenceTangent=finiteRatio(rotationSineAngle(referenceHalf),vectorCosineAngle(referenceHalf));
    const uint16_t viewHalf=(uint16_t)(view->current.fieldOfView>>1);
    const float viewTangent=finiteRatio(rotationSineAngle(viewHalf),vectorCosineAngle(viewHalf));
    const float plane=finiteRatio(view->current.nearClip*referenceTangent,viewTangent);
    float previous=0.0f;
    for (int corner=0;corner<8;++corner) {
        const float point[4]={corner&4?maximum[0]:minimum[0],corner&2?maximum[1]:minimum[1],
            corner&1?maximum[2]:minimum[2],0};
        float transformed[4];vectorMatrixTransformPoint(transformed,combined,point);
        const float distance=transformed[2]-plane;
        if (corner>0 && distance*previous<0.0f) return 1;
        previous=distance;
    }
    return 0;
}

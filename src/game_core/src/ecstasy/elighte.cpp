#include "indep/inc/eLight.hpp"
#include "indep/inc/RaceEffects.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bChunk.hpp"
#include "indep/inc/EventManager.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/FlailerChunks.hpp"
#include "indep/inc/Texture.hpp"
#include "indep/inc/Fred.hpp"
#include "bware/inc/bMath.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace hp2 {

struct LightGroup::Storage {
    std::vector<WorldSpatialNode> nodes;
    std::vector<SceneLight> lights;
};

LightGroup::LightGroup():storage_(std::make_unique<Storage>()) {}
LightGroup::~LightGroup()=default;
std::size_t LightGroup::LightCount() const noexcept {return storage_->lights.size();}
std::size_t LightGroup::NodeCount() const noexcept {return storage_->nodes.size();}
const SceneLight& LightGroup::LightAt(std::size_t index) const {return storage_->lights.at(index);}
const WorldSpatialNode* LightGroup::QueryLeaf(const float position[3]) const
{
    if (storage_->nodes.empty())return nullptr;
    const WorldSpatialContainer tree{storage_->nodes.data(),storage_->nodes.size(),nullptr,0};
    return worldSpatialFind(&tree,position);
}

LightSceneRegistry::LightSceneRegistry():ambientSearchLimitSquared(std::numeric_limits<float>::max())
{
    // PAL117098..117464 original constructor; exact initialized constants
    // verified by executing its257 scalar instructions (no intercepted calls).
    frontendLights.positions={{{{0,-5,5,0}},{{4.300000190734863f,2.5f,2.5f,0}},{{-4.300000190734863f,2.5f,2.5f,0}}}};
    frontendLights.colors={{{{128,128,128,0}},{{64,64,64,0}},{{64,64,64,0}}}};
    selectCarBaseLights.colors={{{{80,80,80,0}},{{64,64,64,0}},{{128,128,180,0}}}};
    selectCarBaseLights.ambient={{32,32,32,0}};
    selectCarBaseLights.positions={{{{6.820000171661377f,-6.099999904632568f,-8.5600004196167f,0}},
        {{-5,-1,0,0}},{{1.2799999713897705f,-1.1699999570846558f,1.2799999713897705f,0}}}};
    selectCarAlternateLights.positions={{{{0,9.380000114440918f,-5,0}},{{-5,10,0,0}},
        {{0.3070000112056732f,10,2.819999933242798f,0}}}};
}

void SetSelectCarLighting(LightSceneRegistry& registry,std::int32_t view,float amount,std::int32_t alternatePreset)
{
    // PAL1157C0..115978: view2 alone selects the second player set. The
    // alternate preset copies its actual zero RGB values; brightness adds
    //30*amount only to the three normal lights, never to ambient.
    auto& output=registry.selectedCarLights[view==2 ? 1:0];
    const auto& input=alternatePreset ? registry.selectCarAlternateLights:registry.selectCarBaseLights;
    output=input;
    if(!alternatePreset) {
        const float addition=amount*30.0f;
        for(std::size_t light=0;light<3;++light)for(std::size_t axis=0;axis<3;++axis)
            output.colors[light][axis]=std::min(input.colors[light][axis]+addition,255.0f);
        for(std::size_t axis=0;axis<3;++axis)output.ambient[axis]=std::min(input.ambient[axis],255.0f);
    }
}

void SetLightContextEnvironment(SceneLightContext& context,
    const float localToWorld[16],const float worldToView[16])
{
    Require(localToWorld && worldToView,"Light environment has no live object/camera matrix");
    eMulMatrix(context.environmentMatrix.data(),localToWorld,worldToView);
    for(std::size_t axis=0;axis<3;++axis) {
        context.environmentMatrix[axis*4+3]=0.0f;
        context.environmentMatrix[12+axis]=context.localEye[axis];
    }
    context.environmentMatrix[15]=0.0f;
}

static SceneLightContext setupRaceLightContext(const LightSceneRegistry& registry,
    const float localToWorld[16],const float worldToView[16],const float worldEye[3],
    const SceneLightQuery& query,std::int32_t count)
{
    Require(localToWorld,"Race light context has no live object matrix");
    SceneLightContext result{};
    float inverseRotation[16];
    motionMatrixTransposeRotation(inverseRotation,localToWorld);
    std::array<std::array<float,4>,3> directions{};
    for(std::size_t axis=0;axis<3;++axis)
        directions[0][axis]=registry.sunlightDirection[axis]*10000.0f-localToWorld[12+axis];
    for(std::int32_t index=0;index<std::min(count,2);++index) {
        Require(query.lights[static_cast<std::size_t>(index)],"Light query returned an empty live light");
        for(std::size_t axis=0;axis<3;++axis)
            directions[static_cast<std::size_t>(index)+1][axis]=
                query.lights[static_cast<std::size_t>(index)]->position[axis]-localToWorld[12+axis];
    }
    for(std::size_t index=0;index<3;++index) {
        float local[4];eMulVector3(local,inverseRotation,directions[index].data());
        float squared=0.0f;
        for(std::size_t axis=0;axis<3;++axis) {
            result.lightPositions[index*4+axis]=local[axis];
            squared+=local[axis]*local[axis];
        }
        result.lightPositions[index*4+3]=1.0f;
        const float scale=squared>0.0f ? 1.0f/std::sqrt(squared):0.0f;
        for(std::size_t axis=0;axis<3;++axis)
            result.directionMatrix[axis*4+index]=local[axis]*scale;
    }
    const bool overrideAmbient=registry.ambientColor[0]!=0.0f ||
        registry.ambientColor[1]!=0.0f || registry.ambientColor[2]!=0.0f;
    for(std::size_t axis=0;axis<3;++axis)
        result.colorMatrix[12+axis]=overrideAmbient ? registry.ambientColor[axis]:query.ambient[axis];
    // This is the released zero-ambient branch115D74, not a native fallback.
    if(result.colorMatrix[12]==0.0f && result.colorMatrix[13]==0.0f && result.colorMatrix[14]==0.0f)
        result.colorMatrix[12]=result.colorMatrix[13]=result.colorMatrix[14]=85.0f;
    result.colorMatrix[15]=128.0f;
    result.sunlightIntensity=std::clamp(query.ambient[3],0.0f,1.0f);
    const float sunScale=std::max(result.sunlightIntensity,0.699999988079071f);
    for(std::size_t axis=0;axis<3;++axis)
        result.colorMatrix[axis]=sunScale*registry.sunlightColor[axis];
    for(std::int32_t index=0;index<std::min(count,2);++index) {
        const auto i=static_cast<std::size_t>(index);
        const auto packed=query.lights[i]->color;
        result.colorMatrix[(i+1)*4]=query.weights[i]*static_cast<float>(packed&255u);
        result.colorMatrix[(i+1)*4+1]=query.weights[i]*static_cast<float>((packed>>8)&255u);
        // PAL116010 explicitly clears blue; do not repair the source's choice.
        result.colorMatrix[(i+1)*4+2]=0.0f;
    }
    if(worldEye) {
        float relative[3];
        for(std::size_t axis=0;axis<3;++axis)relative[axis]=worldEye[axis]-localToWorld[12+axis];
        eMulVector3(result.localEye.data(),inverseRotation,relative);
        result.localEye[3]=1.0f;
    }
    // PAL116090..1160E4 permits the environment transform to be absent.
    // Ordinary lighting does not depend on an environment-map owner.
    if(worldToView && worldEye) {
        SetLightContextEnvironment(result,localToWorld,worldToView);
    }
    return result;
}

SceneLightContext SetupRaceLightContext(const LightSceneRegistry& registry,
    const float localToWorld[16],const float worldToView[16],const float worldEye[3])
{
    Require(localToWorld,"Race light context has no live object matrix");
    SceneLightQuery query{};const auto count=registry.Query(query,localToWorld+12);
    return setupRaceLightContext(registry,localToWorld,worldToView,worldEye,query,count);
}

static SceneLightContext setupFrontendLightContext(const LightSceneRegistry& registry,
    SceneLightContext context,const float localToWorld[16],const CarCinematicScreen& view,
    std::int32_t applicationState)
{
    if(applicationState!=3)return context;
    const bool selected=IsCarSelectScreenActive();
    const auto& lights=selected ? registry.selectedCarLights[view.id==2 ? 1:0]:registry.frontendLights;
    float inverseObject[16],inverseView[16];motionMatrixTransposeRotation(inverseObject,localToWorld);
    if(selected)motionMatrixTransposeRotation(inverseView,view.matrices.worldToView);
    for(std::size_t light=0;light<3;++light) {
        float direction[4],local[4];
        if(selected)eMulVector3(direction,inverseView,lights.positions[light].data());
        else std::copy(lights.positions[light].begin(),lights.positions[light].end(),direction);
        eMulVector3(local,inverseObject,direction);
        float squared=0;
        for(std::size_t axis=0;axis<3;++axis)squared+=local[axis]*local[axis];
        const float inverse=squared>0 ? 1.0f/std::sqrt(squared):0.0f;
        for(std::size_t axis=0;axis<3;++axis) {
            context.lightPositions[light*4+axis]=local[axis];
            context.directionMatrix[axis*4+light]=local[axis]*inverse;
            context.colorMatrix[light*4+axis]=lights.colors[light][axis];
        }
        context.lightPositions[light*4+3]=1;
        context.colorMatrix[light*4+3]=0;
    }
    for(std::size_t axis=0;axis<3;++axis)context.colorMatrix[12+axis]=lights.ambient[axis];
    context.colorMatrix[15]=128;context.sunlightIntensity=1;
    return context;
}

SceneLightContext SetupLightContext(const LightSceneRegistry& registry,
    const float localToWorld[16],const float worldToView[16],const float worldEye[3],
    const CarCinematicScreen& view,std::int32_t applicationState)
{
    return setupFrontendLightContext(registry,
        SetupRaceLightContext(registry,localToWorld,worldToView,worldEye),localToWorld,view,applicationState);
}

const LightContextBatch::Sample& LightContextBatch::Query(const float position[3])
{
    std::array<std::uint32_t,4> key;
    std::memcpy(key.data(),position,3*sizeof(float));
    std::memcpy(&key[3],&registry_.ambientSearchLimitSquared,sizeof(float));
    for(std::size_t index=0;index<used_;++index)if(samples_[index].key==key) {
        ++hits_;return samples_[index];
    }
    auto& sample=used_<samples_.size() ? samples_[used_++]:overflow_;
    sample.key=key;sample.query={};sample.count=registry_.Query(sample.query,position);++queries_;
    return sample;
}

SceneLightContext LightContextBatch::SetupRace(const float localToWorld[16],
    const float worldToView[16],const float worldEye[3])
{
    Require(localToWorld,"Race light context has no live object matrix");
    const auto& sample=Query(localToWorld+12);
    return setupRaceLightContext(registry_,localToWorld,worldToView,worldEye,sample.query,sample.count);
}

SceneLightContext LightContextBatch::Setup(const float localToWorld[16],const float worldToView[16],
    const float worldEye[3],const CarCinematicScreen& view,std::int32_t applicationState)
{
    return setupFrontendLightContext(registry_,SetupRace(localToWorld,worldToView,worldEye),
        localToWorld,view,applicationState);
}

const LightFlare* LightSceneRegistry::FindFlare(std::uint32_t key) const noexcept
{
    // PAL116418..116474 preserves pack/record insertion order.
    for(const auto* pack:flares_)for(const auto& flare:pack->flares)
        if(flare.markerKey==key)return &flare;
    return nullptr;
}

std::int32_t LightSceneRegistry::Query(SceneLightQuery& output,const float position[3]) const
{
    // QueryLightDatabase clears only the first result/weight and ambient,
    // then reads its input position. Preserve the untouched result tail.
    output.weights[0]=0.0f;output.ambient={};output.lights[0]=nullptr;
    const std::array<float,3> point{position[0],position[1],position[2]};
    std::array<std::array<float,4>,10> contributions{};
    std::array<float,4> base{0.0f,0.0f,0.0f,1.0f};
    float weightSum=0.0f,nearest=ambientSearchLimitSquared;
    std::size_t ambientCount=0,count=0;
    for (const auto* group:groups_) {
        if (count==10 || ambientCount==10)break;
        const auto* leaf=group->QueryLeaf(point.data());
        if (!leaf)continue;
        for (std::int32_t index=0;index<-leaf->count && count<10;++index) {
            const auto& light=group->LightAt(static_cast<std::size_t>(leaf->indices[index]));
            const float dx=std::fabs(point[0]-light.position[0]);
            const float dy=std::fabs(point[1]-light.position[1]);
            const float dz=std::fabs(point[2]-light.position[2]);
            const float xx=dx*dx,yy=dy*dy,zz=dz*dz;
            const float radiusSquared=light.radius*light.radius;
            const float distanceSquared=(xx+yy)+zz;
            if (!(distanceSquared<radiusSquared))continue;
            const float innerSquared=light.innerRadius*light.innerRadius;
            float weight=1.0f;
            if (innerSquared<distanceSquared)
                weight=1.0f-(distanceSquared-innerSquared)/(radiusSquared-innerSquared);
            if (light.kind==0) {
                const std::array<float,4> color{
                    static_cast<float>(light.color&255u),static_cast<float>((light.color>>8)&255u),
                    static_cast<float>((light.color>>16)&255u),light.coefficient};
                if (light.ambientMode==2) {
                    if (ambientCount==10)continue;
                    for (std::size_t lane=0;lane<4;++lane)contributions[ambientCount][lane]=color[lane]*weight;
                    if (weight>0.0f) {weightSum+=weight;++ambientCount;}
                } else if (distanceSquared<nearest) {base=color;nearest=distanceSquared;}
            } else if (light.kind==2) {
                output.lights[count]=&light;output.weights[count]=weight;++count;
            }
        }
    }
    float contributionScale=weightSum,baseScale=1.0f-weightSum;
    if (1.0f<weightSum) {contributionScale=1.0f/weightSum;baseScale=0.0f;}
    std::array<float,4> ambient{};
    for (std::size_t lane=0;lane<4;++lane)ambient[lane]=base[lane]*baseScale;
    for (std::size_t index=0;index<ambientCount;++index)
        for (std::size_t lane=0;lane<4;++lane)
            ambient[lane]+=contributions[index][lane]*contributionScale;
    output.ambient=ambient;
    return static_cast<std::int32_t>(count);
}

namespace {
float scalar(std::uint32_t word) { float value;std::memcpy(&value,&word,4);return value; }
// The authored pairs are (exponent, amplitude). Their PS2 encoding used
// integer reinterpretation of IEEE754 words as a fast approximation to pow.
// The PC port decodes the authored numeric intent once, then uses native math.
void convertMaterial(const std::uint8_t* source,float output[16])
{
    std::array<std::uint32_t,20> words{};
    for(std::size_t index=0;index<words.size();++index)words[index]=ReadU32(source+index*4);
    const auto p=[&](std::size_t index){return scalar(words[index]);};
    output[0]=p(0)*p(2);output[1]=p(1)*p(3);output[2]=0.1f;
    output[3]=p(18)*(1.0f/255.0f);
    for(std::size_t i=4;i<=6;++i)output[i]=p(i);
    const float halfSpan=(p(11)-p(7))*0.5f;
    output[7]=(p(7)+halfSpan)*128.0f;
    for(std::size_t i=8;i<=10;++i)output[i]=p(i);
    output[11]=-128.0f*halfSpan;
    output[12]=p(12)*p(14);output[13]=p(13)*p(15);
    output[14]=p(16);output[15]=p(17)-p(16);
}
void prepareNativeMaterial(EditorMarkerLightMaterial& material)
{
    constexpr float kMinimum=0.1f;
    constexpr float kSpan=0.9f;
    constexpr auto kCount=EditorMarkerLightMaterial::kNativeResponseSamples;
    for(std::size_t table=0;table<2;++table) {
        const auto* coefficients=material.NativeCoefficients();
        const float exponent=coefficients[table ? 12:0];
        const float amplitude=coefficients[table ? 13:1];
        Require(std::isfinite(exponent) && std::isfinite(amplitude),
            "Authored PC specular material has non-finite power parameters");
        for(std::size_t index=0;index<kCount;++index) {
            const float input=kMinimum+kSpan*(static_cast<float>(index)/static_cast<float>(kCount-1));
            material.nativeSpecularResponse[table][index]=amplitude*std::pow(input,exponent);
        }
    }
    material.nativeSpecularReady=true;
}

std::size_t alignedChunk(const Chunk& chunk,std::size_t minimum)
{
    const auto start=(chunk.payloadOffset+15u)&~std::size_t(15u);
    Require(start<=chunk.endOffset && minimum<=chunk.endOffset-start,"Aligned light chunk is truncated");
    return start;
}

float lightScalar(const std::uint8_t* source)
{
    const auto value=scalar(ReadU32(source));
    Require(std::isfinite(value),"Authored light contains a non-finite coefficient");
    return value;
}
}

LightSceneResources::LightSceneResources(const std::vector<std::uint8_t>& bundle,LightSceneRegistry& registry)
    :registry_(registry),authored_(bundle)
{
    for (const auto& outer:ReadChunks(authored_)) {
        if (outer.id==0x80034015u) {
            Chunk header{},tree{},lights{};
            for (const auto& child:ReadChunks(authored_,outer.payloadOffset,outer.size)) {
                if (child.id==0x34016u)header=child;
                else if (child.id==0x34017u)tree=child;
                else if (child.id==0x34018u)lights=child;
            }
            if (!header.id)continue;
            const auto headerStart=alignedChunk(header,24);
            if (ReadU32(authored_.data()+headerStart+8)!=1)continue;
            auto group=std::make_unique<LightGroup>();
            if (tree.id) {
                const auto start=alignedChunk(tree,16)+16;
                Require((tree.endOffset-start)%48==0,"Light AABB node array is partial");
                const auto count=(tree.endOffset-start)/48;
                group->storage_->nodes.resize(count);
                for (std::size_t index=0;index<count;++index) {
                    const auto* raw=authored_.data()+start+index*48;auto& node=group->storage_->nodes[index];
                    for (std::size_t lane=0;lane<3;++lane) {
                        node.center[lane]=lightScalar(raw+lane*4);
                        node.halfExtent[lane]=lightScalar(raw+16+lane*4);
                        Require(node.halfExtent[lane]>=0.0f,"Light AABB has a negative extent");
                    }
                    const auto encoded=ReadU16(raw+14);std::memcpy(&node.count,&encoded,2);
                    Require(node.count>=-10 && node.count<=10,"Light AABB exceeds its ten authored indices");
                    for (std::size_t item=0;item<10;++item) {
                        const auto value=ReadU16(raw+28+item*2);std::memcpy(&node.indices[item],&value,2);
                    }
                }
            }
            if (lights.id) {
                const auto start=alignedChunk(lights,0);
                Require((lights.endOffset-start)%96==0,"Light record array is partial");
                const auto count=(lights.endOffset-start)/96;
                group->storage_->lights.resize(count);
                for (std::size_t index=0;index<count;++index) {
                    const auto* raw=authored_.data()+start+index*96;auto& light=group->storage_->lights[index];
                    std::memcpy(light.name.data(),raw,32);light.key=ReadU32(raw+32);
                    light.kind=raw[36];light.ambientMode=raw[37];light.color=ReadU32(raw+44);
                    for (std::size_t lane=0;lane<3;++lane) {
                        light.position[lane]=lightScalar(raw+48+lane*4);
                        light.direction[lane]=lightScalar(raw+64+lane*4);
                    }
                    light.radius=lightScalar(raw+60);light.coefficient=lightScalar(raw+76);
                    light.innerRadius=lightScalar(raw+80);
                }
            }
            const auto& nodes=group->storage_->nodes;
            std::vector<std::uint8_t> state(nodes.size());
            const auto validate=[&](const auto& self,std::size_t index)->void {
                Require(index<nodes.size() && state[index]!=1,"Light AABB contains a cycle/outside node");
                if (state[index]==2)return;
                state[index]=1;const auto& node=nodes[index];
                const auto count=node.count<0 ? -node.count : node.count;
                for (std::int32_t item=0;item<count;++item) {
                    const auto child=node.indices[item];Require(child>=0,"Light AABB contains a negative index");
                    if (node.count>0)self(self,static_cast<std::size_t>(child));
                    else Require(static_cast<std::size_t>(child)<group->LightCount(),"Light leaf index exceeds its light array");
                }
                state[index]=2;
            };
            if (!nodes.empty())validate(validate,0);
            groups_.push_back(std::move(group));
        } else if (outer.id==0x80034019u) {
            Chunk header{},records{};
            for (const auto& child:ReadChunks(authored_,outer.payloadOffset,outer.size)) {
                if (child.id==0x3401au)header=child;
                else if (child.id==0x3401bu)records=child;
            }
            if (!header.id)continue;
            const auto start=alignedChunk(header,64);
            if (ReadU32(authored_.data()+start+8)!=1)continue;
            const auto count=ReadU32(authored_.data()+start+48);
            auto group=std::make_unique<LightFlarePack>();
            if (count) {
                Require(records.id!=0,"Light preset count has no actual records");
                const auto recordStart=alignedChunk(records,0);
                Require(count<=(records.endOffset-recordStart)/80,"Light preset array is truncated");
                group->flares.resize(count);
                for (std::size_t index=0;index<count;++index) {
                    const auto* raw=authored_.data()+recordStart+index*80;
                    auto& flare=group->flares[index];
                    flare.markerKey=ReadU32(raw+8);flare.active=raw[12];flare.kind=raw[13];
                    flare.flags=ReadU16(raw+14);std::memcpy(flare.name.data(),raw+16,32);
                    // RenderLightFlare explicitly clears authored padding W.
                    for(std::size_t axis=0;axis<3;++axis) {
                        flare.position[axis]=lightScalar(raw+48+4*axis);
                        flare.normal[axis]=lightScalar(raw+64+4*axis);
                    }
                }
            }
            flares_.push_back(std::move(group));
        }
    }
    // Publish only after all decoded storage is stable and checked. The
    // registry preserves the source's file/group/record insertion order.
    registry_.groups_.reserve(registry_.groups_.size()+groups_.size());
    registry_.flares_.reserve(registry_.flares_.size()+flares_.size());
    for (const auto& group:groups_)registry_.groups_.push_back(group.get());
    for (const auto& group:flares_)registry_.flares_.push_back(group.get());
}

LightSceneResources::~LightSceneResources()
{
    for (const auto& owned:groups_) {
        const auto found=std::find(registry_.groups_.begin(),registry_.groups_.end(),owned.get());
        if (found!=registry_.groups_.end())registry_.groups_.erase(found);
    }
    for (const auto& owned:flares_) {
        const auto found=std::find(registry_.flares_.begin(),registry_.flares_.end(),owned.get());
        if (found!=registry_.flares_.end())registry_.flares_.erase(found);
    }
}

LightMaterialResources::LightMaterialResources(const std::vector<std::uint8_t>& bundle,EditorMarkerResources& bank)
    :authored_(bundle),catalogue_(std::make_unique<EditorMarkerLightMaterialCatalogue>())
{
    for (const auto& chunk:ReadChunks(authored_))if (chunk.id==0x3401eu) {
        const auto start=(chunk.payloadOffset+15u)&~std::size_t(15u);
        Require(start<=chunk.endOffset && chunk.endOffset-start>=16,"Light material header is truncated");
        const auto* header=authored_.data()+start;
        if (ReadU32(header+8)!=1)continue;
        const auto count=ReadU32(header+12);
        Require(count<=(chunk.endOffset-start-16)/320,"Light material records are truncated");
        for (std::size_t index=0;index<count;++index) {
            const auto* record=header+16+index*320;
            EditorMarkerLightMaterial material{};material.key=ReadU32(record+8);material.authoredRecord=record;
            convertMaterial(record+0x20,material.coefficients[0]);
            convertMaterial(record+0x70,material.coefficients[1]);
            prepareNativeMaterial(material);
            auto inGame=std::make_shared<EditorMarkerLightMaterial>(material);
            inGame->nativeDomain=EditorMarkerLightMaterialDomain::InGame;
            prepareNativeMaterial(*inGame);
            material.inGameVariant=std::move(inGame);
            catalogue_->materials.push_back(std::move(material));
        }
    }
    auto* root=bank.catalogueRoot?bank.catalogueRoot:&bank;
    catalogue_->registry=root;
    auto** tail=&root->lightMaterialCatalogues;while (*tail)tail=&(*tail)->next;
    *tail=catalogue_.get();
    for (const auto& material:catalogue_->materials)
        if (material.key==0x2c420d64u)root->defaultLightMaterial=&material;
}

LightMaterialResources::~LightMaterialResources()
{
    auto* root=catalogue_->registry;
    auto** link=&root->lightMaterialCatalogues;
    while (*link && *link!=catalogue_.get())link=&(*link)->next;
    if (*link)*link=catalogue_->next;
    root->defaultLightMaterial=nullptr;
    for (auto* current=root->lightMaterialCatalogues;current;current=current->next)
        for (const auto& material:current->materials)
            if (material.key==0x2c420d64u)root->defaultLightMaterial=&material;
}

void LightTextureResources::UpdateTextures(ResourceRegistry& resources)
{
    constexpr std::array<std::uint32_t,3> keys{0x8868656fu,0x88d8e8a2u,0x3394fe62u};
    for (std::size_t index=0;index<keys.size();++index)
        textures_[index]=resources.ResolveTexture(keys[index],false);
}

const EditorMarkerTexture* LightTextureResources::At(std::size_t index) const
{
    Require(index<textures_.size(),"Light texture index is outside its three cells");
    return textures_[index];
}
}

namespace hp2 {
namespace {
struct LightFlareParameters {
    float minimumSize,maximumSize;
    std::array<float,4> minimumColor,maximumColor;
    float responseExponent,positionBias,minimumDistanceScale,maximumDistanceScale;
    std::size_t texture;
};
// PAL2D6080..2D629C and the ten-kind pointer table2D62D8.
const std::array<LightFlareParameters,9> lightFlareParameters{{
    {0,64,{0,0,0,0},{100,120,170,80},1.4500000476837158f,0.800000011920929f,0.009999999776482582f,0.014999999664723873f,0},
    {28,28,{0,0,0,0},{100,120,140,20},3,0.800000011920929f,0.009999999776482582f,0.014999999664723873f,1},
    {20,30,{0,0,0,0},{128,0,0,80},1,0.20000000298023224f,0.009999999776482582f,0.019999999552965164f,2},
    {20,20,{0,0,0,0},{255,255,255,128},1.2000000476837158f,0.20000000298023224f,0.004999999888241291f,0.009999999776482582f,2},
    {20,20,{0,0,0,0},{128,128,128,128},1,0.20000000298023224f,0.004999999888241291f,0.009999999776482582f,2},
    {30,40,{128,64,64,48},{128,64,64,128},1,1,0.009999999776482582f,0.019999999552965164f,0},
    {30,40,{64,64,128,48},{64,64,128,128},1,1,0.009999999776482582f,0.019999999552965164f,0},
    {30,40,{128,128,128,32},{128,128,128,128},1,1,0.009999999776482582f,0.019999999552965164f,0},
    {30,50,{110,100,75,96},{110,100,75,110},1,4,0.0010000000474974513f,0.10000000149011612f,2}
}};
constexpr std::array<std::array<std::int8_t,2>,10> lightFlareParameterIndices{{
    {{0,1}},{{2,-1}},{{3,-1}},{{4,-1}},{{5,-1}},{{6,-1}},{{7,-1}},{{0,1}},{{0,1}},{{8,-1}}
}};

std::size_t RenderSingleFlare(const LightFlareParameters& parameters,const float position[4],
    const float direction[4],const float right[3],const float down[3],float intensity,
    float facing,float distanceScale,ResourceRegistry& resources,const CarCinematicScreen& view,EcstasyFrame& frame)
{
    // Native PC response curve. The authored exponent is a numeric power,
    // not a request to run the PS2 integer-bit approximation on the CPU.
    const auto* descriptor=resources.LightTexture(parameters.texture);
    if(!descriptor)return 0;
    const auto* texture=textureInfoForDescriptor(descriptor);
    Require(texture && texture->pixels.Data(),"Light flare texture has no retained pixel owner");
    const float blend=std::pow(std::max(facing,0.009999999776482582f),
        parameters.responseExponent)*intensity;
    float size=parameters.minimumSize+(parameters.maximumSize-parameters.minimumSize)*blend;
    if(size<=0.0f)return 0;
    size*=std::clamp(distanceScale,parameters.minimumDistanceScale,parameters.maximumDistanceScale);
    FlailerPoly polygon;
    std::uint32_t color=0;
    for(std::size_t axis=0;axis<4;++axis) {
        const float channel=parameters.minimumColor[axis]+(parameters.maximumColor[axis]-parameters.minimumColor[axis])*blend;
        color|=(static_cast<std::uint32_t>(convertFloatToInt32(channel))&255u)<<(axis*8);
    }
    for(std::size_t axis=0;axis<3;++axis) {
        const float center=direction[axis]*parameters.positionBias+position[axis];
        const float r=right[axis]*size,d=down[axis]*size;
        polygon.vertices[0].position[axis]=(center-r)+d;
        polygon.vertices[1].position[axis]=(center+r)+d;
        polygon.vertices[2].position[axis]=(center+r)-d;
        polygon.vertices[3].position[axis]=(center-r)-d;
    }
    constexpr float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
    for(std::size_t i=0;i<4;++i) {
        polygon.vertices[i].textureCoordinate={uv[i][0],uv[i][1],1.0f};
        polygon.vertices[i].color=color;
    }
    frame.RenderPolygon3D(polygon,texture,eMathIdentityMatrix,view);
    return 1;
}
}

void CollectLightEffect(const LightFlare& flare,const float transform[16],float intensity,
    ResourceRegistry& resources,std::vector<RaceLightEffect>& output)
{
    if (!flare.active) return;
    Require(flare.kind<lightFlareParameterIndices.size(),"Light flare kind exceeds its released table");
    float world[4];
    eMulVector3(world,transform,flare.position.data());
    for (const auto index : lightFlareParameterIndices[flare.kind]) {
        if (index<0) continue;
        const auto& p=lightFlareParameters[static_cast<std::size_t>(index)];
        const auto* texture=resources.LightTexture(p.texture);
        if (!texture) continue;
        RaceLightEffect effect{texture};
        std::copy_n(world,3,effect.position.begin());
        for (std::size_t axis=0;axis<3;++axis)
            effect.normal[axis]=transform[axis]*flare.normal[0]+transform[4+axis]*flare.normal[1]+
                transform[8+axis]*flare.normal[2];
        effect.minimumColor=p.minimumColor;
        effect.maximumColor=p.maximumColor;
        effect.minimumSize=p.minimumSize;
        effect.maximumSize=p.maximumSize;
        effect.exponent=p.responseExponent;
        effect.bias=p.positionBias;
        effect.minimumScale=p.minimumDistanceScale;
        effect.maximumScale=p.maximumDistanceScale;
        effect.intensity=intensity;
        effect.twoSided=(flare.flags&1u)!=0;
        output.push_back(effect);
    }
}

std::size_t RenderLightFlare(const LightFlare& flare,const float localToWorld[16],
    float intensity,ResourceRegistry& resources,const CarCinematicScreen& view,EcstasyFrame& frame)
{
    // PAL1164F0..116778. Transform/clip the actual marker before any texture
    // submission; camera basis and view pixelScale are original inputs.
    if(!flare.active || !localToWorld || !view.view)return 0;
    Require(flare.kind<lightFlareParameterIndices.size(),"Light flare kind exceeds its released table");
    float world[4],normal[4],clip[4],direction[4]{};
    eMulVector3(world,localToWorld,flare.position.data());
    eMulVector3(clip,view.matrices.worldToClip,world);
    for(std::size_t axis=0;axis<3;++axis)if(clip[axis]>std::fabs(clip[3]) || clip[axis]<-std::fabs(clip[3]))return 0;
    float squared=0.0f;
    for(std::size_t axis=0;axis<3;++axis) {
        normal[axis]=localToWorld[axis]*flare.normal[0]+localToWorld[4+axis]*flare.normal[1]+localToWorld[8+axis]*flare.normal[2];
        direction[axis]=view.view->current.eye[axis]-world[axis];
        squared+=direction[axis]*direction[axis];
    }
    if(squared>0.0f)for(std::size_t axis=0;axis<3;++axis)direction[axis]*=1.0f/std::sqrt(squared);
    float facing=(normal[0]*direction[0]+normal[1]*direction[1])+normal[2]*direction[2];
    if(flare.flags&1u)facing=std::fabs(facing);
    float right[3],down[3],cameraPosition[4];
    for(std::size_t axis=0;axis<3;++axis) {
        right[axis]=view.matrices.worldToView[axis*4];down[axis]=view.matrices.worldToView[axis*4+1];
    }
    eMulVector3(cameraPosition,view.view->current.matrix,world);
    // PAL's distance clamp consumes CalculateH in source pixels. Using the
    // enlarged PC focal input here cancels native projection growth and
    // makes the flare retain a fixed pixel size as resolution increases.
    Require(view.matricesCalculated && std::isfinite(view.sourcePixelScale) &&
        view.sourcePixelScale>0.0f,"Light flare has no calculated source focal scale");
    const float distanceScale=std::fabs(cameraPosition[2])/view.sourcePixelScale;
    std::size_t count=0;
    for(const auto index:lightFlareParameterIndices[flare.kind])if(index>=0)
        count+=RenderSingleFlare(lightFlareParameters[static_cast<std::size_t>(index)],world,direction,
            right,down,intensity,facing,distanceScale,resources,view,frame);
    return count;
}

std::size_t RenderWorldLightFlares(const LightSceneRegistry& lights,ResourceRegistry& resources,
    const CarCinematicScreen& view,EcstasyFrame& frame,bool inGame)
{
    // PAL116778..116824 reads the same InGameFlag and intrusive pack order.
    if(!inGame)return 0;
    std::size_t count=0;
    for(const auto* pack:lights.FlarePacks())for(const auto& flare:pack->flares)
        count+=RenderLightFlare(flare,eMathIdentityMatrix,1.0f,resources,view,frame);
    return count;
}
}

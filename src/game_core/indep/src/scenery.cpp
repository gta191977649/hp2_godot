#include "indep/inc/Scenery.hpp"
#include "indep/inc/NativeRenderSettings.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/eLight.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <limits>
#include <set>

namespace { int32_t currentZoneNumber=-1; } // PAL3302B0.
int32_t sceneryCurrentZoneNumber(void) {return currentZoneNumber;} // PAL14F8E0.

SceneryInstance* sceneryFindInstanceByIndex(const Scenery* scenery,int32_t section,int32_t instance)
{
    if(!scenery || section<0 || instance<0 || static_cast<size_t>(section)>=scenery->sectionCount)return nullptr;
    ScenerySection* group=scenery->sections[section];
    if(!group || static_cast<size_t>(instance)>=group->instanceCount)return nullptr;
    return group->instances+instance;
}

SceneryInstance* sceneryFindInstance(const Scenery* scenery,uint32_t key)
{
    if(!scenery)return nullptr;
    for(size_t index=0;index<scenery->loadedSectionCount;++index){
        ScenerySection* section=scenery->loadedSections[index];
        for(size_t instance=0;instance<section->instanceCount;++instance){
            SceneryInstance* placement=section->instances+instance;
            if(section->models[placement->modelIndex].levels[0].key==key)return placement;
        }
    }
    return nullptr;
}

void sceneryInstanceFrame(const SceneryInstance* instance,float frame[16])
{
    for(int row=0;row<3;++row){
        for(int column=0;column<3;++column)
            frame[row*4+column]=static_cast<float>(instance->rotation[row*3+column])*0.000030517578125f;
        frame[row*4+3]=0.0f;
        frame[12+row]=instance->position[row];
    }
    frame[15]=1.0f;
}

void sceneryFlipObjectsOnBackwardsTrack(ScenerySection* section)
{
    for(size_t index=0;index<section->instanceCount;++index){
        SceneryInstance& instance=section->instances[index];
        if(!(instance.flags&0x400u))continue;
        // PAL1506C8 loads 0x38800000 (1/16384) for all nine
        // orientation lanes. The separate GetMatrix helper uses1/32768
        // for its own consumers and would halve these placements here.
        float frame[16]{};frame[15]=1.0f;
        for(int row=0;row<3;++row)for(int column=0;column<3;++column)
            frame[row*4+column]=static_cast<float>(instance.rotation[row*3+column])*0.00006103515625f;
        motionMatrixRotateYaw(frame,frame,0x8000u);
        for(int row=0;row<3;++row)for(int column=0;column<3;++column){
            const uint16_t bits=static_cast<uint16_t>(convertFloatToInt32(frame[row*4+column]*16384.0f));
            std::memcpy(instance.rotation+row*3+column,&bits,sizeof(bits));
        }
        // PAL1506E0 requantizes the rotated orientation using16384.
        // Position remains the same point during this half-turn.
    }
}

void sceneryNormalizeSmokeableRotation(int16_t rotation[9])
{
    // InitSmokeables21F270..21F500 rebuilds the actual spawner basis via
    //11BD90, then publishes nine signed lanes at1/16384. This is ordinary
    // native basis construction; no VU/register execution is retained.
    float input[16]{},output[16]{};
    for(int row=0;row<3;++row)for(int lane=0;lane<3;++lane)
        input[row*4+lane]=static_cast<float>(rotation[row*3+lane])/16384.0f;
    const double x=input[0],y=input[1],z=input[2];
    const double crossX=y*input[6]-z*input[5];
    const double crossY=z*input[4]-x*input[6];
    const double crossZ=x*input[5]-y*input[4];
    hp2::Require(x*x+y*y+z*z>0.0 && crossX*crossX+crossY*crossY+crossZ*crossZ>0.0,
        "Smokeable placement has a zero or collinear authored basis");
    motionMatrixOrthonormalize(output,input);
    int16_t rebuilt[9];
    for(int row=0;row<3;++row)for(int lane=0;lane<3;++lane) {
        const float encoded=output[row*4+lane]*16384.0f;
        hp2::Require(std::isfinite(encoded) && encoded>=-32768.0f && encoded<32768.0f,
            "Smokeable normalized orientation exceeds its authored lane");
        // The format carries a truncated signed fixed-point lane. Retain
        // that quantization, not an instruction-specific conversion model.
        rebuilt[row*3+lane]=static_cast<int16_t>(encoded);
    }
    std::memcpy(rotation,rebuilt,sizeof(rebuilt));
}

int32_t sceneryPointInVisibleZone(const float point[2],const float (*polygon)[2],int32_t count)
{
    int32_t inside=0,previous=count-1;
    for (int32_t current=0;current<count;++current) {
        const float* a=polygon[current];const float* b=polygon[previous];
        if ((a[1]<=point[1] && point[1]<b[1]) || (b[1]<=point[1] && point[1]<a[1])) {
            float intersection=(b[0]-a[0])*(point[1]-a[1]);
            intersection/=b[1]-a[1];intersection+=a[0];
            if (point[0]<intersection)inside=!inside;
        }
        previous=current;
    }
    return inside;
}

SceneryVisibleZone* sceneryFindVisibleZone(Scenery* scenery,const float point[2])
{
    for (auto* zone=scenery->firstVisibleZone;zone;zone=zone->next)
        if (sceneryPointInVisibleZone(point,zone->polygon,zone->polygonCount)) {
            if (zone->previous)zone->previous->next=zone->next;
            else scenery->firstVisibleZone=zone->next;
            if (zone->next)zone->next->previous=zone->previous;
            else scenery->lastVisibleZone=zone->previous;
            zone->next=scenery->firstVisibleZone;zone->previous=nullptr;
            if (zone->next)zone->next->previous=zone;
            else scenery->lastVisibleZone=zone;
            scenery->firstVisibleZone=zone;return zone;
        }
    return nullptr;
}

int32_t sceneryVisibleZoneFindIndexTo(const SceneryVisibleZone* zone,int32_t target)
{
    for (int32_t index=0;index<zone->adjacentCount;++index)
        if (zone->adjacentZones[index]==target)return index;
    return -1;
}

int32_t sceneryVisibleZoneFindNextFreeIndex(SceneryVisibleZone* zone)
{
    const int32_t free=sceneryVisibleZoneFindIndexTo(zone,-1);
    if (free>=0)return free;
    if (zone->adjacentCount==64)return -1;
    return zone->adjacentCount++;
}

namespace hp2 {
namespace {
std::int16_t sceneryHalf(const std::uint8_t* data)
{
    const auto bits=ReadU16(data);std::int16_t result;
    std::memcpy(&result,&bits,sizeof(result));return result;
}
float sceneryReal(const std::uint8_t* data)
{
    const auto bits=ReadU32(data);float result;
    std::memcpy(&result,&bits,sizeof(result));
    Require(std::isfinite(result),"Scenery position is not finite");return result;
}
}

struct SceneryResources::SectionStorage {
    ScenerySection native{};
    std::vector<SceneryModel> models;
    std::vector<SceneryInstance> instances;
    // The static source instance rotation is immutable after the reverse
    // track flip. Decode the physical render matrix once at track load;
    // facade billboards remain camera-facing and are evaluated each view.
    std::vector<std::array<float,16>> staticRenderFrames;
    // XY footprint of the actual level-0 model, cached at load time;
    // no borrowed geometry pointer survives a resource reload.
    std::vector<std::array<float,4>> staticFineFootprints;
    // A per-instance near-field state is written by the main view only.
    // Secondary/mirror cameras read their own eye for the near test and
    // cannot alter the main view's choice or render model identities.
    std::vector<std::uint8_t> nearDetailState;
    std::vector<SceneryRecord34104> records34104;
    // Derived once from the retained immutable authored tree. Source
    // records remain the one published resource representation.
    struct CullNode {
        std::array<float,3> minimum{},maximum{};
        std::array<std::int16_t,5> children{};
        std::int16_t count{};
    };
    std::vector<CullNode> cullNodes;
    std::vector<uint8_t> header;
};

SceneryResources::SceneryResources(const std::vector<std::uint8_t>& bundle,
    EditorMarkerResources& modelResources,bool reverse)
    :models_(&modelResources)
{
    // Only physical spawners receive InitSmokeables basis preparation.
    // Other scenery can intentionally contain scale. Collect addresses
    // before decoding sections because authored chunk order is independent.
    std::set<std::pair<std::int32_t,std::int32_t>> smokeableInstances;
    const auto collect=[&](const auto& self,std::size_t begin,std::size_t size,unsigned depth)->void {
        Require(depth<=32,"Smokeable scenery nesting exceeds32 levels");
        for(const auto& chunk:ReadChunks(bundle,begin,size)) {
            if(chunk.id==0x34027u) {
                Require(chunk.size%8u==0,"Smokeable scenery array has a partial record");
                for(auto cursor=chunk.payloadOffset;cursor<chunk.endOffset;cursor+=8u) {
                    const auto section=sceneryHalf(bundle.data()+cursor+4);
                    const auto instance=sceneryHalf(bundle.data()+cursor+6);
                    Require(section>=0 && instance>=0,"Smokeable scenery address is negative");
                    smokeableInstances.emplace(section,instance);
                }
            } else if(chunk.IsContainer())self(self,chunk.payloadOffset,chunk.size,depth+1);
        }
    };
    collect(collect,0,bundle.size(),0);
    const auto visit=[&](const auto& self,size_t begin,size_t size,unsigned depth)->void {
        Require(depth<=32,"Scenery nesting exceeds 32 levels");
        for(const auto& group:ReadChunks(bundle,begin,size)){
            if (group.id==0x34106u) {
                // LoaderScenery appends the complete authored VisibleZone
                // rows, then clears only their mutable source+14 field.
                const auto count=group.size/288u;
                for (std::size_t index=0;index<count;++index) {
                    const auto* data=bundle.data()+group.payloadOffset+index*288u;
                    auto zone=std::make_unique<SceneryVisibleZone>();
                    for (std::size_t lane=0;lane<3;++lane)zone->authoredParameters[lane]=ReadU32(data+8+lane*4);
                    zone->runtimeState14=0;
                    const auto adjacent=ReadU32(data+0x18),polygon=ReadU32(data+0x9c);
                    Require(adjacent<=64 && polygon<=16,"VisibleZone authored counts exceed their actual arrays");
                    zone->adjacentCount=static_cast<int32_t>(adjacent);zone->polygonCount=static_cast<int32_t>(polygon);
                    for (std::size_t lane=0;lane<64;++lane)zone->adjacentZones[lane]=sceneryHalf(data+0x1c+lane*2);
                    for (std::size_t vertex=0;vertex<16;++vertex)
                        for (std::size_t axis=0;axis<2;++axis) {
                            const auto bits=ReadU32(data+0xa0+vertex*8+axis*4);
                            std::memcpy(&zone->polygon[vertex][axis],&bits,4);
                            if (vertex<polygon)Require(std::isfinite(zone->polygon[vertex][axis]),"VisibleZone polygon is not finite");
                        }
                    auto* node=zone.get();node->previous=native_.lastVisibleZone;
                    if (node->previous)node->previous->next=node;
                    else native_.firstVisibleZone=node;
                    native_.lastVisibleZone=node;
                    visibleZones_.push_back(std::move(zone));
                }
                continue;
            }
            if(group.id!=0x80034100u){
                if(group.IsContainer())self(self,group.payloadOffset,group.size,depth+1);
                continue;
            }
            Chunk header{},models{},instances{},records34104{};
            for(const auto& child:ReadChunks(bundle,group.payloadOffset,group.size)){
                if(child.id==0x34101u){Require(!header.id,"Repeated scenery header");header=child;}
                else if(child.id==0x34102u){Require(!models.id,"Repeated scenery model table");models=child;}
                else if(child.id==0x34103u){Require(!instances.id,"Repeated scenery instance table");instances=child;}
                else if(child.id==0x34104u){Require(!records34104.id,"Repeated scenery34104 array");records34104=child;}
            }
            Require(header.id && models.id && instances.id,"Scenery section is incomplete");
            const size_t hs=(header.payloadOffset+15u)&~size_t(15u);
            const size_t first=(instances.payloadOffset+15u)&~size_t(15u);
            Require(hs<=header.endOffset && header.endOffset-hs>=12,"Scenery header is truncated");
            Require(first<=instances.endOffset && (instances.endOffset-first)%48u==0,"Scenery instance table is partial");
            Require(models.size%40u==0,"Scenery model table is partial");
            auto owned=std::make_unique<SectionStorage>();
            auto* storage=owned.get();sections_.push_back(std::move(owned));
            auto& section=storage->native;
            storage->header.assign(bundle.begin()+static_cast<std::ptrdiff_t>(hs),
                bundle.begin()+static_cast<std::ptrdiff_t>(header.endOffset));
            section.authoredHeader=storage->header.data();section.authoredHeaderSize=storage->header.size();
            section.number=ReadU32(bundle.data()+hs+8);
            Require(section.number<=65535u,"Scenery section exceeds its packed key");
            if(sectionIndex_.size()<=section.number) {
                sectionIndex_.resize(size_t(section.number)+1,nullptr);
                sectionStorageByNumber_.resize(size_t(section.number)+1,nullptr);
            }
            Require(!sectionIndex_[section.number],"Scenery section number repeats");
            storage->models.resize(models.size/40u);
            storage->instances.resize((instances.endOffset-first)/48u);
            for(size_t index=0;index<storage->models.size();++index){
                const auto* data=bundle.data()+models.payloadOffset+index*40u;
                for(size_t level=0;level<3;++level){
                    const auto key=ReadU32(data+level*4);
                    if(key)editorMarkerModelInitialize(&storage->models[index].levels[level],models_,key);
                    storage->models[index].minimumPixels[level]=sceneryHalf(data+0x0c+level*2);
                    std::memcpy(&storage->models[index].facade[level],data+0x20+level,1);
                }
                storage->models[index].radius=sceneryReal(data+0x24);
            }
            for(size_t index=0;index<storage->instances.size();++index){
                const auto* data=bundle.data()+first+index*48u;
                auto& instance=storage->instances[index];
                for(int axis=0;axis<3;++axis){
                    instance.minimum[axis]=sceneryHalf(data+axis*2);
                    instance.maximum[axis]=sceneryHalf(data+6+axis*2);
                    instance.position[axis]=sceneryReal(data+16+axis*4);
                }
                instance.modelIndex=sceneryHalf(data+12);
                instance.flags=ReadU16(data+14);
                Require(instance.modelIndex>=0 && static_cast<size_t>(instance.modelIndex)<storage->models.size(),
                    "Scenery instance has an invalid model index");
                for(int lane=0;lane<9;++lane)instance.rotation[lane]=sceneryHalf(data+28+lane*2);
            }
            section.models=storage->models.data();section.modelCount=storage->models.size();
            section.instances=storage->instances.data();section.instanceCount=storage->instances.size();
            if (records34104.id) {
                const auto count=records34104.size/24u;storage->records34104.resize(count);
                storage->cullNodes.resize(count);
                for (std::size_t index=0;index<count;++index)
                    for (std::size_t field=0;field<6;++field)
                        storage->records34104[index].fields[field]=ReadU32(bundle.data()+records34104.payloadOffset+index*24+field*4);
                for(std::size_t index=0;index<count;++index) {
                    const auto* data=bundle.data()+records34104.payloadOffset+index*24;
                    auto& node=storage->cullNodes[index];
                    for(std::size_t axis=0;axis<3;++axis) {
                        node.minimum[axis]=static_cast<float>(sceneryHalf(data+axis*2));
                        node.maximum[axis]=static_cast<float>(sceneryHalf(data+(axis+3)*2));
                    }
                    node.count=sceneryHalf(data+12);
                    for(std::size_t child=0;child<node.children.size();++child)
                        node.children[child]=sceneryHalf(data+14+child*2);
                }
                section.records34104=storage->records34104.data();section.record34104Count=count;
            }
            if(reverse)sceneryFlipObjectsOnBackwardsTrack(&section);
            // LoaderScenery performs the reverse flip before the track-load
            // completion calls InitSmokeables. Finalize that same generation
            // before either matrix or footprint cache is created.
            for(std::size_t index=0;index<storage->instances.size();++index)
                if(smokeableInstances.count({static_cast<std::int32_t>(section.number),
                    static_cast<std::int32_t>(index)}))
                    sceneryNormalizeSmokeableRotation(storage->instances[index].rotation);
            // Cache only invariant native transforms; do not reuse a pose
            // for a camera-facing billboard or a source view-only Z offset.
            storage->staticRenderFrames.resize(storage->instances.size());
            storage->staticFineFootprints.resize(storage->instances.size());
            storage->nearDetailState.assign(storage->instances.size(),0);
            for(std::size_t index=0;index<storage->instances.size();++index) {
                const auto& instance=storage->instances[index];
                auto& matrix=storage->staticRenderFrames[index];
                matrix.fill(0.0f);matrix[15]=1.0f;
                if(instance.flags&0x200u) {
                    matrix[0]=matrix[5]=matrix[10]=1.0f;
                } else {
                    for(std::size_t row=0;row<3;++row) {
                        for(std::size_t column=0;column<3;++column)
                            matrix[row*4+column]=static_cast<float>(instance.rotation[row*3+column])*0.00006103515625f;
                        matrix[12+row]=instance.position[row];
                    }
                }
                // A source instance may have bounds that do not agree with
                // its source mesh after the actual native transform. Cache
                // BOTH independently; a union of disjoint boxes would
                // incorrectly promote whole stretches of a track to LOD0.
                auto& footprint=storage->staticFineFootprints[index];
                footprint={std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()};
                const auto& model=storage->models[instance.modelIndex];
                if(!(instance.flags&0x200u) && !model.facade[0])
                    (void)sceneryNativeModelFootprintXY(model,matrix.data(),footprint.data());
            }
            sectionIndex_[section.number]=&section;
            sectionStorageByNumber_[section.number]=storage;
            if(section.number>=90u)loadedSections_.insert(loadedSections_.begin(),&section);
            else loadedSections_.push_back(&section);
        }
    };
    try {
        visit(visit,0,bundle.size(),0);
        for(const auto& storage:sections_) {
            Require(storage->instances.size()<=std::numeric_limits<std::size_t>::max()-nativeInstanceCapacity_,
                "Native scenery instance count exceeds its address range");
            nativeInstanceCapacity_+=storage->instances.size();
        }
    }
    catch (...) {
        for (const auto& section:sections_)for (auto& model:section->models)
            for (auto& level:model.levels)editorMarkerModelDispose(&level);
        native_.firstVisibleZone=native_.lastVisibleZone=nullptr;
        throw;
    }
    native_.sections=sectionIndex_.data();native_.sectionCount=sectionIndex_.size();
    native_.loadedSections=loadedSections_.data();native_.loadedSectionCount=loadedSections_.size();
}

SceneryResources::~SceneryResources()
{
    for(const auto& section:sections_)for(auto& model:section->models)
        for(auto& level:model.levels)editorMarkerModelDispose(&level);
    native_.firstVisibleZone=native_.lastVisibleZone=nullptr;
}

const SceneryVisibleZone* SceneryResources::CollectSections(const CarCinematicScreen& view,
    std::vector<std::int16_t>& output,std::size_t capacity,std::size_t* sourceCount)
{
    // PAL150950..150B0C. This now borrows a preallocated native scratch
    // buffer. The retained source zone LRU order and truncation are identical.
    Require(view.view,"Scenery view has no actual camera");
    output.clear();
    auto* zone=sceneryFindVisibleZone(&native_,view.view->current.eye);
    const auto put=[&](std::int32_t number) {
        if (number>=0 && static_cast<std::size_t>(number)<native_.sectionCount &&
            native_.sections[number] && output.size()<capacity)
            output.push_back(static_cast<std::int16_t>(number));
    };
    if (!zone) {
        for (std::size_t index=0;index<native_.loadedSectionCount;++index) {
            const auto number=native_.loadedSections[index]->number;
            if (number<100 && output.size()<capacity)output.push_back(static_cast<std::int16_t>(number));
        }
    } else {
        for (std::int32_t number=90;number<100;++number)put(number);
        for (std::int32_t index=0;index<zone->adjacentCount;++index)put(zone->adjacentZones[index]);
    }
    if(sourceCount)*sourceCount=output.size();
    const float distanceScale=nativeViewDistanceScale(view.id);
    if(zone && distanceScale>1.0f && output.size()<capacity) {
        // Authored PVS is a console draw-distance limit, not a resource-load
        // boundary: every section below is already owned by this live track.
        // Preserve the exact PVS prefix and append nearby physical sections in
        // load order. Layout/direction instance masks and tree culling still run.
        // Sections90..99 are global; >=100 are PVS proxies (often subsets
        // of ordinary cells). Render deduplicates appended instances against
        // that original prefix; it does not also append the proxy cells.
        double radiusSquared=128.0*128.0;
        for(const auto number:output)if(number>=0 && number<90) {
            const auto* storage=sectionStorageByNumber_[static_cast<std::size_t>(number)];
            if(!storage || storage->cullNodes.empty())continue;
            const auto& bounds=storage->cullNodes.front();double extent=0.0;
            for(std::size_t axis=0;axis<2;++axis) {
                const double eye=view.view->current.eye[axis];
                const double delta=std::max(std::fabs(bounds.minimum[axis]-eye),std::fabs(bounds.maximum[axis]-eye));
                extent+=delta*delta;
            }
            radiusSquared=std::max(radiusSquared,extent);
        }
        radiusSquared*=static_cast<double>(distanceScale)*distanceScale;
        for(const auto* section:loadedSections_) {
            if(section->number>=90u || output.size()>=capacity)continue;
            const auto number=static_cast<std::int16_t>(section->number);
            if(std::find(output.begin(),output.end(),number)!=output.end())continue;
            const auto* storage=sectionStorageByNumber_[section->number];
            if(!storage || storage->cullNodes.empty())continue;
            const auto& bounds=storage->cullNodes.front();double nearestSquared=0.0;
            for(std::size_t axis=0;axis<2;++axis) {
                const double eye=view.view->current.eye[axis];
                const double delta=eye<bounds.minimum[axis] ? bounds.minimum[axis]-eye :
                    eye>bounds.maximum[axis] ? eye-bounds.maximum[axis]:0.0;
                nearestSquared+=delta*delta;
            }
            if(nearestSquared<=radiusSquared)put(number);
        }
    }
    if(view.id==1)currentZoneNumber=zone?static_cast<std::int32_t>(zone->authoredParameters[0]):-1;
    return zone;
}

std::vector<std::int16_t> SceneryResources::WhatSectionsShouldWeDraw(
    const CarCinematicScreen& view,std::size_t capacity)
{
    // External tools receive their own vector by value; the production
    // Render() below uses the persistent scratch and avoids this allocation.
    std::vector<std::int16_t> result;
    CollectSections(view,result,capacity);
    return result;
}

float sceneryNativeDistanceToInstanceSurface(const SceneryInstance& instance,
    const float eye[3]) noexcept
{
    if(!eye)return std::numeric_limits<float>::infinity();
    // The source's signed 16-bit world bounds are already passed to the
    // real scenery culler. They are a conservative envelope for large assets.
    // A building's world center is not useful for near-field detail policy.
    double distanceSquared=0.0;
    for(std::size_t axis=0;axis<3;++axis) {
        const float value=eye[axis];
        if(!std::isfinite(value) || instance.minimum[axis]>instance.maximum[axis])
            return std::numeric_limits<float>::infinity();
        const double delta=value<instance.minimum[axis] ?
            static_cast<double>(instance.minimum[axis])-value :
            value>instance.maximum[axis] ?
            value-static_cast<double>(instance.maximum[axis]) : 0.0;
        distanceSquared+=delta*delta;
    }
    return static_cast<float>(std::sqrt(distanceSquared));
}

float sceneryNativeDistanceSquaredXY(const SceneryInstance& instance,
    const float eye[3]) noexcept
{
    if(!eye)return std::numeric_limits<float>::infinity();
    float distanceSquared=0.0f;
    for(std::size_t axis=0;axis<2;++axis) {
        const float value=eye[axis];
        if(!std::isfinite(value) || instance.minimum[axis]>instance.maximum[axis])
            return std::numeric_limits<float>::infinity();
        const float delta=value<instance.minimum[axis] ?
            static_cast<float>(instance.minimum[axis])-value :
            value>instance.maximum[axis] ?
            value-static_cast<float>(instance.maximum[axis]):0.0f;
        distanceSquared+=delta*delta;
    }
    return distanceSquared;
}

bool sceneryNativeModelFootprintXY(const SceneryModel& model,const float frame[16],
    float bounds[4]) noexcept
{
    if(!frame || !bounds || !model.levels[0].solid || model.facade[0])return false;
    const auto* fine=model.levels[0].solid;
    for(std::size_t axis=0;axis<3;++axis) {
        if(!std::isfinite(fine->minimum[axis]) || !std::isfinite(fine->maximum[axis]) ||
           fine->minimum[axis]>fine->maximum[axis])return false;
    }
    float values[4]={std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()};
    // Eight native transform corners, once when a real track section loads.
    // An AABB of the transformed source mesh is conservative for rotation.
    for(unsigned mask=0;mask<8;++mask) {
        const float p[3]={ (mask&1)?fine->maximum[0]:fine->minimum[0],
            (mask&2)?fine->maximum[1]:fine->minimum[1],
            (mask&4)?fine->maximum[2]:fine->minimum[2]};
        for(unsigned axis=0;axis<2;++axis) {
            const float world=((frame[axis]*p[0]+frame[4+axis]*p[1])+
                frame[8+axis]*p[2])+frame[12+axis];
            if(!std::isfinite(world))return false;
            values[axis]=std::min(values[axis],world);
            values[2+axis]=std::max(values[2+axis],world);
        }
    }
    std::copy_n(values,4,bounds);
    return true;
}

float sceneryNativeDistanceSquaredFootprintXY(const float bounds[4],
    const float eye[3]) noexcept
{
    if(!bounds || !eye)return std::numeric_limits<float>::infinity();
    float squared=0.0f;
    for(unsigned axis=0;axis<2;++axis) {
        if(!std::isfinite(eye[axis]) || !std::isfinite(bounds[axis]) ||
            !std::isfinite(bounds[axis+2]) || bounds[axis]>bounds[axis+2])
            return std::numeric_limits<float>::infinity();
        const float delta=eye[axis]<bounds[axis] ? bounds[axis]-eye[axis] :
            eye[axis]>bounds[axis+2] ? eye[axis]-bounds[axis+2]:0.0f;
        squared+=delta*delta;
    }
    return squared;
}

bool sceneryNativeNearDetailState(float distanceSquared,bool previous) noexcept
{
    if(!std::isfinite(distanceSquared) || distanceSquared<0.0f)return false;
    const float reach=previous ? nativeSceneryNearDetailExitXY : nativeSceneryNearDetailEnterXY;
    return distanceSquared<=reach*reach;
}

void sceneryNativeBillboardFacing(float output[16],const float towardEyeXY[2],
    const float origin[3]) noexcept
{
    if(!output || !towardEyeXY || !origin)return;
    std::fill_n(output,16,0.0f);
    output[10]=output[15]=1.0f;
    const float x=towardEyeXY[0],y=towardEyeXY[1];
    const float squared=x*x+y*y;
    if(std::isfinite(squared) && squared>1.0e-4f) {
        const float inverse=1.0f/std::sqrt(squared);
        const float cosine=x*inverse,sine=y*inverse;
        output[0]=cosine;output[1]=sine;
        output[4]=-sine;output[5]=cosine;
    } else {
        // PAL25A8C0..25A978 retains dominant axis and quadrant when its
        // explicit squared-distance threshold sets the angle ratio to zero.
        // Equal absolute X/Y selects Y; all signed-zero pairs select +90.
        float cosine=0.0f,sine=1.0f;
        if(std::isfinite(squared)) {
            if(std::fabs(y)<std::fabs(x)) {
                cosine=x<0.0f ? -1.0f:1.0f;
                sine=0.0f;
            } else if(y<0.0f)sine=-1.0f;
        }
        output[0]=cosine;output[1]=sine;
        output[4]=-sine;output[5]=cosine;
    }
    output[12]=origin[0];output[13]=origin[1];output[14]=origin[2];
}

std::size_t sceneryNativeSelectDetailLod(const SceneryModel& model,
    std::int32_t originalPixels,float distanceToSurface) noexcept
{
    // PAL14FFC4..150030 first finds an existing authored mesh at its source
    // pixel threshold. A PC detail preference must not make an invisible
    // single-LOD fine section overlap its separately authored distant section.
    std::size_t visibleLod=0;
    for(;visibleLod<3;++visibleLod)
        if(originalPixels>=model.minimumPixels[visibleLod] && model.levels[visibleLod].solid)break;
    if(visibleLod==3)return visibleLod;
    const auto detailPixels=nativeDetailLodPixels(originalPixels,
        nativeSceneryDetailDistanceFactor);
    std::size_t lod=0;
    for(;lod<visibleLod;++lod)
        if(detailPixels>=model.minimumPixels[lod] && model.levels[lod].solid)break;
    // B25: the B24 projected-pixel multiplier could not protect oversized
    // structures whose authored LOD threshold was still reached only a
    // meter from a pump, wall or building edge. Keep the REAL high-detail
    // solid while its world envelope is near; never fabricate a missing LOD.
    // PAL visibility cutoff has already run in the caller, unchanged.
    if(lod>0 && model.levels[0].solid && std::isfinite(distanceToSurface) &&
        distanceToSurface>=0.0f &&
        distanceToSurface<=nativeSceneryHighDetailSurfaceDistance)return 0;
    return lod;
}

std::size_t sceneryNativeSelectDetailLod(const SceneryModel& model,
    std::int32_t originalPixels) noexcept
{
    return sceneryNativeSelectDetailLod(model,originalPixels,
        std::numeric_limits<float>::infinity());
}

void SceneryResources::Render(const CarCinematicScreen& view,EcstasyFrame& frame,
    std::uint32_t viewMask,std::int32_t worldFrames)
{
    // PAL150D98/150B10/150378/14FE18/150EB8. Native vectors replace only the
    // original temporary cull-list allocation; their entries still borrow
    // the same models, placements, source trees and viewport camera.
    Require(view.view && view.matricesCalculated,"Scenery rendering requires current view matrices");
    Require(std::isfinite(view.sourcePixelScale) && view.sourcePixelScale>0.0f,
        "Scenery thresholds require the calculated source focal scale");
    const auto& camera=view.view->current;
    const float distanceScale=nativeViewDistanceScale(view.id);
    nativeRenderStatistics_={};
    const bool extendedRange=distanceScale>1.0f;
    if(extendedRange) {
        if(renderIdentityScratch_.empty()) {
            Require(nativeInstanceCapacity_<=std::numeric_limits<std::size_t>::max()/2,
                "Native scenery identity capacity overflow");
            std::size_t capacity=2;
            while(capacity<nativeInstanceCapacity_*2) {
                Require(capacity<=std::numeric_limits<std::size_t>::max()/2,
                    "Native scenery identity table overflow");
                capacity*=2;
            }
            renderIdentityScratch_.resize(capacity);
        }
        if(++renderGeneration_==0) {
            for(auto& bucket:renderIdentityScratch_)bucket.generation=0;
            renderGeneration_=1;
        }
    }
    bool extendedSection=false;
    bool journalSample=false;
    if(view.id>=0 && view.id<static_cast<std::int32_t>(journalTicks_.size())) {
        const auto index=static_cast<std::size_t>(view.id);
        const auto bit=std::uint32_t(1)<<index;
        const auto ticks=TicksNow();
        if(!(journalViews_&bit) || ticks-journalTicks_[index]>=kTickerFrequency*5u) {
            journalViews_|=bit;journalTicks_[index]=ticks;journalSample=true;
        }
    }
    // Count only the sampled view. No second visibility/LOD traversal and
    // no GPU readback is performed for this bounded process-journal row.
    std::array<std::size_t,3> journalLods{};
    std::size_t journalInstances=0,journalNodes=0;
    const auto commandBegin=journalSample ? frame.Commands().size():0;
    auto& draws=drawScratch_;
    draws.clear();
    const auto classify=[&](const std::int16_t* minimum,const std::int16_t* maximum) {
        float lo[3],hi[3];
        for (std::size_t axis=0;axis<3;++axis) {lo[axis]=static_cast<float>(minimum[axis]);hi[axis]=static_cast<float>(maximum[axis]);}
        // SetupSceneryCullInfo10F878 copies eView+0x50, the source
        // CalculateLocalToScreenClip matrix; TreeCull150410 and
        // DrawAScenery14FE84 both consume that retained cull matrix.
        return ecstasyClassifyBounds(lo,hi,view.matrices.worldToScreenClip);
    };
    const auto drawInstance=[&](SectionStorage& storage,std::size_t index,std::int32_t inheritedClip) {
        auto& section=storage.native;
        Require(index<section.instanceCount,"Scenery tree leaf leaves its retained instance array");
        const auto& instance=section.instances[index];
        const auto clip=inheritedClip==1 ? classify(instance.minimum,instance.maximum):inheritedClip;
        if (!clip || (instance.flags&viewMask&0xffu))return;
        Require(instance.modelIndex>=0 && static_cast<std::size_t>(instance.modelIndex)<section.modelCount,
            "Scenery instance has no actual model");
        auto& model=section.models[instance.modelIndex];
        const float delta[3]={instance.position[0]-camera.eye[0],instance.position[1]-camera.eye[1],
            instance.position[2]-camera.eye[2]};
        const float radius=model.radius+4.0f;
        const float facing=(delta[0]*camera.forward[0]+delta[1]*camera.forward[1])+delta[2]*camera.forward[2];
        if (facing < -radius)return;
        const float distance=std::sqrt((delta[0]*delta[0]+delta[1]*delta[1])+delta[2]*delta[2])-radius;
        // PAL150E04..150E14 copies eView+8 into CullInfo+C0; the thresholds
        // consume that source focal input, before native output enlargement.
        const float scale=radius<distance ? radius*view.sourcePixelScale/distance:view.sourcePixelScale;
        const auto pixels=convertFloatToInt32(scale);
        const auto visibilityPixels=distanceScale==1.0f ? pixels:convertFloatToInt32(scale*distanceScale);
        if (visibilityPixels<2)return;
        // Original pixels still choose existing near/medium model detail.
        // Extended visibility admits distant geometry at its coarsest actual
        // authored LOD; it does not multiply every nearby high-detail mesh.
        const float squaredXY=std::min(
            hp2::sceneryNativeDistanceSquaredXY(instance,camera.eye),
            hp2::sceneryNativeDistanceSquaredFootprintXY(
                storage.staticFineFootprints[index].data(),camera.eye));
        // A source AABB has sometimes poorly fitting vertical extents. Use
        // horizontal proximity, which is also the meaningful test for a
        // player entering a canopy or passing roadside props. This is a
        // squared native float test (no sqrt/console math per object).
        const bool wasNear=storage.nearDetailState[index]!=0;
        const bool near=sceneryNativeNearDetailState(squaredXY,view.id==1 && wasNear);
        if(view.id==1)storage.nearDetailState[index]=static_cast<std::uint8_t>(near);
        std::size_t lod=hp2::sceneryNativeSelectDetailLod(model,pixels,
            near?0.0f:std::numeric_limits<float>::infinity());
        const bool originalAdmission=pixels>=2 && lod<3;
        if(lod==3 && distanceScale>1.0f) {
            for(std::size_t level=3;level>0;--level)
                if(model.levels[level-1].solid && model.minimumPixels[level-1]<=visibilityPixels) {
                    lod=level-1;break;
                }
        }
        if (lod==3)return;
        RenderDraw draw{&model.levels[lod],{},clip,(instance.flags&0x200u)!=0};
        if (instance.flags&0x200u)draw.matrix=storage.staticRenderFrames[index];
        else {
            if (model.facade[lod]) {
                hp2::sceneryNativeBillboardFacing(draw.matrix.data(),delta,instance.position);
            } else {
                // Render-specific source transform was decoded once when
                // the ScenerySection loaded (not an interpreted PS2 matrix).
                draw.matrix=storage.staticRenderFrames[index];
            }
            if (instance.flags&viewMask&0x100u)draw.matrix[14]+=2.0f;
            // PAL1500D8..150208 keeps this authored instance matrix for
            // every solid LOD. Different mesh bounds do not rescale it.
        }
        draw.nativeExtended=extendedSection || !originalAdmission;
        draw.nativeLod=static_cast<std::uint8_t>(lod);
        ++nativeRenderStatistics_.drawnInstances;
        if(journalSample)++journalLods[lod];
        draws.push_back(draw);
    };
    auto& stack=nodeScratch_;
    // Optional extended visibility is not clipped by the console's 128-entry
    // temporary list. It remains bounded by owned sections plus the PVS prefix.
    const auto sectionCapacity=distanceScale>1.0f ?
        std::max<std::size_t>(128,loadedSections_.size()+74u):128u;
    const auto* selectedZone=CollectSections(view,sectionScratch_,sectionCapacity,&nativeRenderStatistics_.sourceSections);
    nativeRenderStatistics_.extendedSections=sectionScratch_.size()-nativeRenderStatistics_.sourceSections;
    std::size_t sectionOrdinal=0;
    for (const auto number:sectionScratch_) {
        extendedSection=sectionOrdinal++>=nativeRenderStatistics_.sourceSections;
        auto& section=*native_.sections[number];
        if(journalSample)journalInstances+=section.instanceCount;
        // Native list of owned track sections in authored extraction order.
        // The section's static cache belongs to the same SectionStorage.
        auto* storage=sectionStorageByNumber_[number];
        if (!section.records34104 || !section.record34104Count)continue;
        Require(storage && section.records34104==storage->records34104.data() &&
            section.record34104Count==storage->cullNodes.size(),
            "Scenery cull tree no longer belongs to its loaded section");
        // Reuse the transient traversal allocation between sections. The
        // source depth-first visitation order is unchanged.
        stack.clear();stack.push_back({0,1});
        std::size_t visited=0;
        while (!stack.empty()) {
            const auto item=stack.back();stack.pop_back();
            Require(item.index<section.record34104Count && ++visited<=section.record34104Count,
                "Scenery culling tree contains an invalid index or repeated cycle");
            const auto& node=storage->cullNodes[item.index];
            const auto clip=item.clip==1 ? ecstasyClassifyBounds(node.minimum.data(),node.maximum.data(),
                view.matrices.worldToScreenClip):item.clip;
            if (!clip)continue;
            Require(node.count>=0 && node.count<=5,"Scenery tree node exceeds its five authored children");
            for (std::int32_t child=0;child<node.count;++child) {
                const auto value=node.children[child];
                if (value>=0)drawInstance(*storage,static_cast<std::size_t>(value),clip);
                else stack.push_back({static_cast<std::size_t>(-static_cast<std::int32_t>(value)),clip});
            }
        }
        if(journalSample)journalNodes+=visited;
    }
    if(extendedRange) {
        const auto alreadyPresent=[&](const RenderDraw& draw) {
            // Proxy sections can carry different culling boxes/LOD thresholds
            // but render the same solid at the exact same native transform.
            // Compare the selected draw, not a lossy position or those boxes.
            std::array<std::uintptr_t,22> key{};
            key[0]=reinterpret_cast<std::uintptr_t>(draw.model->solid);
            key[1]=reinterpret_cast<std::uintptr_t>(draw.model->resources);
            key[2]=reinterpret_cast<std::uintptr_t>(draw.model->replacement);
            key[3]=draw.model->rigidAnimationStart;
            key[4]=static_cast<std::uint8_t>(draw.model->replacementCount) |
                (static_cast<std::uintptr_t>(static_cast<std::uint8_t>(draw.model->detailLevel))<<8u);
            key[5]=draw.identity ? 1u:0u;
            std::size_t hash=static_cast<std::size_t>(1469598103934665603ull);
            for(std::size_t lane=0;lane<16;++lane) {
                std::uint32_t bits;std::memcpy(&bits,&draw.matrix[lane],sizeof(bits));key[lane+6]=bits;
            }
            for(const auto word:key) {hash^=static_cast<std::size_t>(word);hash*=static_cast<std::size_t>(1099511628211ull);}
            const auto mask=renderIdentityScratch_.size()-1;
            auto slot=hash&mask;
            while(renderIdentityScratch_[slot].generation==renderGeneration_ &&
                renderIdentityScratch_[slot].key!=key)slot=(slot+1)&mask;
            auto& bucket=renderIdentityScratch_[slot];
            const bool present=bucket.generation==renderGeneration_;
            if(!present) {bucket.key=key;bucket.generation=renderGeneration_;}
            return present;
        };
        // Reserve every already-visible authored draw before admitting any
        // extension. A proxy first encountered earlier in the PVS must not
        // displace a real source draw that appears later in that same order.
        for(const auto& draw:draws)if(!draw.nativeExtended)(void)alreadyPresent(draw);
        std::size_t output=0;
        for(std::size_t index=0;index<draws.size();++index) {
            const auto& draw=draws[index];
            if(draw.nativeExtended && alreadyPresent(draw)) {
                ++nativeRenderStatistics_.duplicateExtendedInstances;
                --nativeRenderStatistics_.drawnInstances;
                if(journalSample)--journalLods[draw.nativeLod];
                continue;
            }
            if(output!=index)draws[output]=draw;
            ++output;
        }
        draws.resize(output);
    }
    // StuffScenery publishes in the exact leaf traversal order. Fully inside
    // entries set eModel flag4; intersecting entries use strip-level culling.
    auto* resources=models_->resourceRegistry;
    std::optional<SceneLightContext> worldContext;
    for(auto& draw:draws) {
        std::optional<SceneLightContext> localContext;
        const SceneLightContext* context=nullptr;
        if(draw.identity || (draw.model->solid && (draw.model->solid->renderFlags&0x1c0u))) {
            Require(resources,"Scenery light context lost its canonical resource registry");
            if(draw.identity) {
                if(!worldContext)worldContext=frame.SetupWorldLightContext(*resources,view);
                context=&*worldContext;
            } else {
                // PAL150F9C calls elSetupLightContext for IsLit models;
                // the instance matrix and actual environment orientation are
                // consumed together before the frame copies this context.
                localContext=SetupLightContext(resources->Lights(),draw.matrix.data(),
                    frame.EnvironmentMatrix(),camera.eye,view,frame.ApplicationState());context=&*localContext;
            }
        }
        frame.RenderModel(*draw.model,draw.matrix.data(),view,draw.clip==2 ? 4u:0u,1.0f,worldFrames,context);
    }
    if(journalSample) {
        char detail[1536]{};
        const auto written=std::snprintf(detail,sizeof(detail),
            "view=%d world_frame=%d eye=(%.3f,%.3f,%.3f) forward=(%.4f,%.4f,%.4f) "
            "fov=%.3f source_focal=%.3f output_focal=%.3f pvs_zone=%d pvs_fallback=%d "
            "loaded_sections=%zu selected_sections=%zu candidate_instances=%zu tree_nodes=%zu "
            "visible_instances=%zu lod=(%zu,%zu,%zu) commands=%zu sections=",
            view.id,worldFrames,static_cast<double>(camera.eye[0]),static_cast<double>(camera.eye[1]),
            static_cast<double>(camera.eye[2]),static_cast<double>(camera.forward[0]),
            static_cast<double>(camera.forward[1]),static_cast<double>(camera.forward[2]),
            static_cast<double>(view.fieldOfViewDegrees),static_cast<double>(view.sourcePixelScale),
            static_cast<double>(view.pixelScale),
            selectedZone?static_cast<std::int32_t>(selectedZone->authoredParameters[0]):-1,
            selectedZone?0:1,native_.loadedSectionCount,sectionScratch_.size(),journalInstances,journalNodes,
            draws.size(),journalLods[0],journalLods[1],journalLods[2],frame.Commands().size()-commandBegin);
        auto used=written>0 ? std::min(static_cast<std::size_t>(written),sizeof(detail)-1):0;
        for(const auto number:sectionScratch_) {
            const auto count=std::snprintf(detail+used,sizeof(detail)-used,"%d,",static_cast<int>(number));
            if(count<0 || static_cast<std::size_t>(count)>=sizeof(detail)-used)break;
            used+=static_cast<std::size_t>(count);
        }
        RecordSessionEvent("renderer.visibility",detail);
    }
}

// PAL14F848/14F898 allocate and release one real eModel. The original
// visible-zone polygons remain in SceneryResources, not in this owner.
VisibleZoneResources::VisibleZoneResources(EditorMarkerResources& models)
{
    editorMarkerModelInitialize(&boundary_,&models,hashString("MARKER_BOUNDARY"));
}
VisibleZoneResources::~VisibleZoneResources() {editorMarkerModelDispose(&boundary_);}
}

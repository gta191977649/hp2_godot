#include "indep/inc/WorldObject.hpp"
#include "bware/inc/bWare.hpp"
#include <cstring>
#include <limits>
#include <new>

struct WorldObjectPoolBlock {
    WorldObjectPoolBlock* next;
    WorldObject* storage;
    size_t count;
};

namespace {
bool containsSlot(const WorldObject* storage,size_t count,const WorldObject* object)
{
    const auto first=reinterpret_cast<std::uintptr_t>(storage);
    const auto address=reinterpret_cast<std::uintptr_t>(object);
    if(!storage || address<first)return false;
    const auto offset=address-first;
    return offset%sizeof(WorldObject)==0 && offset/sizeof(WorldObject)<count;
}

bool ownsSlot(const WorldObjectRegistry& registry,const WorldObject* object)
{
    if(containsSlot(registry.storage,registry.capacity,object))return true;
    for(auto* block=registry.expanded;block;block=block->next)
        if(containsSlot(block->storage,block->count,object))return true;
    return false;
}

void addFreeSlots(WorldObjectRegistry& registry,WorldObject* storage,size_t count)
{
    for(size_t index=count;index>0;--index) {
        storage[index-1].next=registry.freeObjects;
        registry.freeObjects=&storage[index-1];
    }
}

bool expandPool(WorldObjectRegistry& registry)
{
    // InitWorldObjects initially reserves128, not a hard maximum. The
    // unchecked bOMalloc in FernShaker1D9D04 expands through258130 by
    // (initialCount >> 2)+1. Every previously issued address remains valid.
    const size_t count=registry.capacity/4+1;
    if(count>std::numeric_limits<size_t>::max()/sizeof(WorldObject) ||
       count>std::numeric_limits<size_t>::max()-registry.totalCapacity)return false;
    auto* block=new(std::nothrow) WorldObjectPoolBlock{};
    if(!block)return false;
    block->storage=new(std::nothrow) WorldObject[count]{};
    if(!block->storage){delete block;return false;}
    block->count=count;
    block->next=registry.expanded;
    registry.expanded=block;
    addFreeSlots(registry,block->storage,count);
    registry.totalCapacity+=count;
    return true;
}
}

void worldObjectsInitialize(WorldObjectRegistry* registry,WorldObject* storage,
    size_t capacity,EditorMarkerResources* models)
{
    hp2::Require(registry && (storage || !capacity) &&
        capacity<=std::numeric_limits<size_t>::max()/sizeof(WorldObject),
        "WorldObject initial pool has invalid storage or byte extent");
    *registry={};
    registry->storage=storage;
    registry->capacity=capacity;
    registry->models=models;
    if(storage)std::memset(storage,0,capacity*sizeof(*storage));
    registry->totalCapacity=capacity;
    addFreeSlots(*registry,storage,capacity);
}

void worldObjectRegister(WorldObjectRegistry* registry,WorldObject* object,
    uint32_t key,const float frame[16],int32_t alternatePass)
{
    hp2::Require(registry && registry->models && object && !object->occupied,
        "WorldObject constructor has no canonical model/list owner or is already registered");
    const bool pooled=ownsSlot(*registry,object);
    if(pooled) {
        // Public construction also accepts an explicitly selected free pool
        // slot; the ordinary allocation path takes the head in constant time.
        auto** link=&registry->freeObjects;
        while(*link && *link!=object)link=&(*link)->next;
        hp2::Require(*link==object,"WorldObject pool slot is not available for construction");
        *link=object->next;
    }
    *object={};
    try {
        editorMarkerModelInitialize(&object->model,registry->models,key);
    } catch(...) {
        *object={};
        if(pooled){object->next=registry->freeObjects;registry->freeObjects=object;}
        throw;
    }
    if(frame){std::memcpy(object->frame,frame,sizeof(object->frame));object->hasFrame=1;}
    object->alternatePass=alternatePass;
    object->occupied=1;
    object->previous=registry->last;
    if(registry->last)registry->last->next=object;
    else registry->first=object;
    registry->last=object;
    ++registry->count;
}

WorldObject* worldObjectCreate(WorldObjectRegistry* registry,uint32_t key,
    const float frame[16],int32_t alternatePass)
{
    if(!registry || !registry->models)return nullptr;
    if(!registry->freeObjects && !expandPool(*registry))return nullptr;
    auto* object=registry->freeObjects;
    worldObjectRegister(registry,object,key,frame,alternatePass);
    return object;
}

void worldObjectDispose(WorldObjectRegistry* registry,WorldObject* object)
{
    if(!registry || !object || !object->occupied)return;
    editorMarkerModelDispose(&object->model);
    if(object->previous)object->previous->next=object->next;
    else registry->first=object->next;
    if(object->next)object->next->previous=object->previous;
    else registry->last=object->previous;
    object->occupied=0;
    object->next=object->previous=nullptr;
    --registry->count;
    // bOFree links the released slot at the free head. Embedded objects
    // belong to their physical owner and must never enter this pool.
    if(ownsSlot(*registry,object)) {
        object->next=registry->freeObjects;
        registry->freeObjects=object;
    }
}

void worldObjectsClose(WorldObjectRegistry* registry)
{
    if(!registry)return;
    while(registry->first)worldObjectDispose(registry,registry->first);
    while(registry->expanded) {
        auto* block=registry->expanded;
        registry->expanded=block->next;
        delete[] block->storage;
        delete block;
    }
    *registry={};
}

namespace hp2 {
WorldObjectDrawSummary RenderWorldObjects(WorldObjectRegistry& objects,
    const CarCinematicScreen& view,EcstasyFrame& frame,std::int32_t worldFrames)
{
    Require(objects.models && view.view && view.matricesCalculated,
        "WorldObject drawing lacks its live registry or calculated source view");
    WorldObjectDrawSummary observed{};
    // Final1CDB20 visits the same oldest-to-newest intrusive list and tests
    // +0C before Render. The native null terminator replaces its sentinel.
    for(auto* object=objects.first;object;object=object->next) {
        Require(++observed.visited<=objects.count && object->occupied,
            "WorldObject rendering encountered a retired owner or list cycle");
        const auto enabled=object->borrowedEnabled?*object->borrowedEnabled:object->hasFrame;
        if(!enabled)continue;
        const float* transform=object->borrowedFrame?object->borrowedFrame:object->frame;
        ++observed.enabled;
        // WorldObject::Render1CD998 first uses radius20 and cutoff2, then
        // eModel::IsVisible10CEA8 on the original (pre-animation) transform.
        if(ecstasyViewNativePositionPixelSize(&view,transform+12,20.0f,0)<2)continue;
        const auto* solid=object->model.solid;
        if(!solid)continue;
        float toScreenClip[16];
        ecstasyViewLocalToScreenClip(&view,toScreenClip,transform);
        if(!ecstasyClassifyBounds(solid->minimum,solid->maximum,toScreenClip))continue;
        // +10 requests the original local lighting context. It is independent
        // of alpha sorting and uses the same retained model and frame matrix.
        std::optional<SceneLightContext> lighting;
        if(object->alternatePass) {
            auto* root=objects.models->catalogueRoot ? objects.models->catalogueRoot:objects.models;
            Require(root->resourceRegistry,"Lit WorldObject has no canonical light registry");
            lighting=SetupRaceLightContext(root->resourceRegistry->Lights(),transform,
                view.matrices.worldToView,view.view->current.eye);
            ++observed.litObjects;
        }
        const auto strips=frame.RenderModel(object->model,transform,view,0,1.0f,worldFrames,
            lighting ? &*lighting:nullptr);
        if(strips)++observed.drawnObjects;
        observed.stripCommands+=strips;
    }
    Require(observed.visited==objects.count,"WorldObject registry count differs from its live list");
    return observed;
}
}

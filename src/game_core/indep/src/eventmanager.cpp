#include "indep/inc/EventManager.hpp"
#include <math.h>
#include <string.h>
#include <new>

struct WorldEventHandler {
    WorldEventHandler* next;
    WorldEventHandler* previous;
    WorldEventCallback callback;
    uint32_t mask;
    uint32_t references;
};

static void initializeList(WorldEvent* root)
{
    memset(root,0,sizeof(*root));root->next=root;root->previous=root;
}

static void append(WorldEvent* root,WorldEvent* event)
{
    event->previous=root->previous;event->next=root;
    root->previous->next=event;root->previous=event;
}

static void unlinkEvent(WorldEvent* event)
{
    event->previous->next=event->next;event->next->previous=event->previous;
}

void worldEventsInitialize(WorldEventDispatcher* dispatcher,WorldEvent* slots,size_t capacity,
                            const WorldSpatialContainer* containers,size_t containerCount)
{
    memset(dispatcher,0,sizeof(*dispatcher));
    initializeList(&dispatcher->queued);initializeList(&dispatcher->deferred);initializeList(&dispatcher->retained);
    dispatcher->slots=slots;dispatcher->capacity=capacity;
    dispatcher->containers=containers;dispatcher->containerCount=containerCount;
    if (capacity) memset(slots,0,capacity*sizeof(*slots));
}

void worldEventsDispose(WorldEventDispatcher* dispatcher)
{
    if (!dispatcher) return;
    while(dispatcher->firstHandler) {
        WorldEventHandler* next=dispatcher->firstHandler->next;
        delete dispatcher->firstHandler;
        dispatcher->firstHandler=next;
    }
    worldEventsInitialize(dispatcher,dispatcher->slots,dispatcher->capacity,NULL,0);
}

WorldEvent* worldEventsCreate(WorldEventDispatcher* dispatcher,uint32_t mask)
{
    for (size_t index=0;index<dispatcher->capacity;++index) {
        WorldEvent* event=&dispatcher->slots[index];
        if (event->occupied) continue;
        memset(event,0,sizeof(*event));event->occupied=1;event->mask=mask;
        append(dispatcher->dispatching ? &dispatcher->deferred : &dispatcher->queued,event);
        ++dispatcher->count;
        if (dispatcher->maximumCount<dispatcher->count) dispatcher->maximumCount=dispatcher->count;
        return event;
    }
    dispatcher->result=WORLD_SERVICES_CAPACITY;return NULL;
}

void worldEventsDelete(WorldEventDispatcher* dispatcher,WorldEvent* event)
{
    if (event==dispatcher->current) return;
    unlinkEvent(event);event->occupied=0;--dispatcher->count;
}

WorldEvent* const* worldEventsQuery(WorldEventDispatcher* dispatcher,const float point[3])
{
    size_t count=0;
    for (size_t index=0;index<dispatcher->containerCount && count<40;++index) {
        const WorldSpatialContainer* container=&dispatcher->containers[index];
        const WorldSpatialNode* leaf=worldSpatialFind(container,point);
        if (!leaf) continue;
        for (int32_t item=0;item<-leaf->count && count<40;++item) {
            const WorldSpatialTrigger* trigger=&container->triggers[leaf->indices[item]];
            const float x=fabsf(point[0]-trigger->position[0]);
            const float y=fabsf(point[1]-trigger->position[1]);
            const float z=fabsf(point[2]-trigger->position[2]);
            const float xx=x*x,yy=y*y,zz=z*z,rr=trigger->radius*trigger->radius;
            if (!((xx+yy)+zz<rr)) continue;
            WorldEvent* event=worldEventsCreate(dispatcher,trigger->mask);
            if (!event) { dispatcher->results[count]=NULL;return count ? dispatcher->results : NULL; }
            event->trigger=trigger;dispatcher->results[count++]=event;
        }
    }
    dispatcher->results[count]=NULL;return count ? dispatcher->results : NULL;
}

// PAL1BA2D0: genuine ordered handler owner, with native allocation replacing
// the fixed PS2 slot pool. No synthetic handler or source-empty callback exists.
int32_t worldEventsAddHandler(WorldEventDispatcher* dispatcher,WorldEventCallback callback,uint32_t mask)
{
    if(!dispatcher || !callback || !mask) return 0;
    for(WorldEventHandler* handler=dispatcher->firstHandler;handler;handler=handler->next) {
        if(handler->callback==callback) { ++handler->references; return 1; }
    }
    auto* handler=new(std::nothrow) WorldEventHandler{nullptr,dispatcher->lastHandler,callback,mask,1};
    if(!handler) { dispatcher->result=WORLD_SERVICES_CAPACITY; return 0; }
    if(dispatcher->lastHandler) dispatcher->lastHandler->next=handler;
    else dispatcher->firstHandler=handler;
    dispatcher->lastHandler=handler;
    ++dispatcher->handlerCount;
    if(dispatcher->maximumHandlerCount<dispatcher->handlerCount)
        dispatcher->maximumHandlerCount=dispatcher->handlerCount;
    return 1;
}

// PAL1BA3A8 removes only the matching registration, when its last user leaves.
void worldEventsRemoveHandler(WorldEventDispatcher* dispatcher,WorldEventCallback callback)
{
    if(!dispatcher) return;
    for(WorldEventHandler* handler=dispatcher->firstHandler;handler;handler=handler->next) {
        if(handler->callback!=callback) continue;
        if(--handler->references) return;
        if(handler->previous) handler->previous->next=handler->next;
        else dispatcher->firstHandler=handler->next;
        if(handler->next) handler->next->previous=handler->previous;
        else dispatcher->lastHandler=handler->previous;
        delete handler;
        --dispatcher->handlerCount;
        return;
    }
}

void worldEventsRegisterCarHandler(WorldEventDispatcher* dispatcher) { ++dispatcher->carHandlerReferences; }
void worldEventsUnregisterCarHandler(WorldEventDispatcher* dispatcher)
{
    if (dispatcher->carHandlerReferences) --dispatcher->carHandlerReferences;
}

WorldServicesResult worldEventsDispatch(WorldEventDispatcher* dispatcher)
{
    if (!dispatcher) return WORLD_SERVICES_INVALID_BINDING;
    dispatcher->dispatching=1;
    while (dispatcher->queued.next!=&dispatcher->queued) {
        WorldEvent* event=dispatcher->queued.next;dispatcher->current=event;
        // Actual registration order and source mask test. New events enter
        // deferred; current-event deletion remains protected by emRemoveEvent.
        // The source-proved empty Car handler stays retired. Other real
        // callbacks now execute instead of silently being omitted.
        for(WorldEventHandler* handler=dispatcher->firstHandler;handler;handler=handler->next)
            if(event->mask & handler->mask) handler->callback(event);
        unlinkEvent(event);dispatcher->current=NULL;
        if (event->references) append(&dispatcher->retained,event);
        else { event->occupied=0;--dispatcher->count; }
    }
    while (dispatcher->retained.next!=&dispatcher->retained) {
        WorldEvent* event=dispatcher->retained.next;unlinkEvent(event);append(&dispatcher->queued,event);
    }
    while (dispatcher->deferred.next!=&dispatcher->deferred) {
        WorldEvent* event=dispatcher->deferred.next;unlinkEvent(event);append(&dispatcher->queued,event);
    }
    dispatcher->dispatching=0;return dispatcher->result;
}

void worldEventsRetain(WorldEvent* event) { ++event->references; }
void worldEventsRelease(WorldEvent* event) { --event->references; }


// Event resources retain their parser and owned allocations.
#include "indep/inc/EventManager.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include <cmath>
#include <cstring>

namespace hp2 {
namespace {
float real(const std::uint8_t* bytes)
{
    const std::uint32_t bits=ReadU32(bytes);float value;std::memcpy(&value,&bits,sizeof(value));
    Require(std::isfinite(value),"Spatial event resource contains a non-finite value");return value;
}
void validateTree(const std::vector<WorldSpatialNode>& nodes,std::size_t index,
                   std::vector<std::uint8_t>& state,std::size_t triggerCount)
{
    Require(index<nodes.size(),"Spatial event child is outside its node array");
    Require(state[index]!=1,"Spatial event tree contains a cycle");
    if (state[index]==2) return;
    state[index]=1;const auto& node=nodes[index];
    Require(node.count>=-10 && node.count<=10,"Spatial event node exceeds ten authored indices");
    const int count=node.count<0 ? -node.count : node.count;
    for (int item=0;item<count;++item) {
        const auto child=node.indices[item];Require(child>=0,"Negative spatial event index");
        if (node.count>0) validateTree(nodes,static_cast<std::size_t>(child),state,triggerCount);
        else Require(static_cast<std::size_t>(child)<triggerCount,"Spatial trigger index is outside its array");
    }
    state[index]=2;
}
}

WorldEventsData::WorldEventsData(const std::vector<std::uint8_t>& bundle)
{
    for (const auto& group : ReadChunks(bundle)) {
        if (group.id!=0x80036000u) continue;
        Chunk header{},nodes{},triggers{};
        for (const auto& child : ReadChunks(bundle,group.payloadOffset,group.size)) {
            if (child.id==0x36001u) header=child;
            if (child.id==0x36002u) nodes=child;
            if (child.id==0x36003u) triggers=child;
        }
        Require(header.id && header.size>=24,"Spatial event resource has no complete header");
        const std::size_t headerStart=(header.payloadOffset+15u)&~std::size_t(15u);
        Require(headerStart+24<=header.endOffset,"Truncated aligned spatial event header");
        if (ReadU32(bundle.data()+headerStart+8)!=1u) continue;
        /* Version one is the source container format. An absent data array is
         * retained by the original inactive list and never queried. */
        if (!ReadU32(bundle.data()+headerStart+12) || !nodes.id || !triggers.id) continue;
        const std::size_t nodeStart=((nodes.payloadOffset+15u)&~std::size_t(15u))+16;
        const std::size_t triggerStart=(triggers.payloadOffset+15u)&~std::size_t(15u);
        Require(nodeStart<=nodes.endOffset && (nodes.endOffset-nodeStart)%48==0,
                "Spatial event node array is partial");
        Require(triggerStart<=triggers.endOffset && (triggers.endOffset-triggerStart)%32==0,
                "Spatial trigger array is partial");
        nodes_.emplace_back((nodes.endOffset-nodeStart)/48);
        triggers_.emplace_back((triggers.endOffset-triggerStart)/32);
        auto& nativeNodes=nodes_.back();auto& nativeTriggers=triggers_.back();
        Require(!nativeNodes.empty(),"Active spatial event container has no root");
        for (std::size_t index=0;index<nativeNodes.size();++index) {
            const auto* bytes=bundle.data()+nodeStart+index*48;auto& node=nativeNodes[index];
            for (int lane=0;lane<3;++lane) {
                node.center[lane]=real(bytes+lane*4);node.halfExtent[lane]=real(bytes+16+lane*4);
                Require(node.halfExtent[lane]>=0,"Spatial node has negative extent");
            }
            node.count=static_cast<std::int16_t>(ReadU16(bytes+14));
            for (int item=0;item<10;++item) node.indices[item]=static_cast<std::int16_t>(ReadU16(bytes+28+item*2));
        }
        for (std::size_t index=0;index<nativeTriggers.size();++index) {
            const auto* bytes=bundle.data()+triggerStart+index*32;auto& trigger=nativeTriggers[index];
            trigger.key=ReadU32(bytes);trigger.mask=ReadU32(bytes+4);trigger.value=ReadU32(bytes+8);
            for (int lane=0;lane<3;++lane) trigger.position[lane]=real(bytes+16+lane*4);
            trigger.radius=real(bytes+28);Require(trigger.radius>=0,"Spatial trigger has negative radius");
        }
        std::vector<std::uint8_t> state(nativeNodes.size());validateTree(nativeNodes,0,state,nativeTriggers.size());
        containers_.push_back({nativeNodes.data(),nativeNodes.size(),nativeTriggers.data(),nativeTriggers.size()});
    }
}
}

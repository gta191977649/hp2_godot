#ifndef NFSHP2_EVENT_MANAGER_HPP
#define NFSHP2_EVENT_MANAGER_HPP

#include "indep/inc/World.hpp"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Car;

typedef struct WorldSpatialTrigger {
    uint32_t key,mask,value;
    float position[3];
    float radius;
} WorldSpatialTrigger;

/* Positive counts name child nodes. Nonpositive counts name trigger indices.
 * Child order is authored and determines which overlapping leaf wins. */
typedef struct WorldSpatialNode {
    float center[3],halfExtent[3];
    int16_t count;
    int16_t indices[10];
} WorldSpatialNode;

typedef struct WorldSpatialContainer {
    const WorldSpatialNode* nodes;
    size_t nodeCount;
    const WorldSpatialTrigger* triggers;
    size_t triggerCount;
} WorldSpatialContainer;

typedef struct WorldEvent {
    struct WorldEvent* next;
    struct WorldEvent* previous;
    int32_t references;
    uint32_t mask;
    const WorldSpatialTrigger* trigger;
    struct Car* recipient;
    int32_t occupied;
} WorldEvent;

typedef void (*WorldEventCallback)(WorldEvent* event);
typedef struct WorldEventHandler WorldEventHandler;

struct WorldEventDispatcher {
    WorldEvent queued,deferred,retained;
    WorldEvent* slots;
    size_t capacity,count,maximumCount;
    const WorldSpatialContainer* containers;
    size_t containerCount;
    WorldEvent* current;
    WorldEvent* results[41];
    int32_t dispatching;
    uint32_t carHandlerReferences;
    WorldServicesResult result;
    WorldEventHandler* firstHandler;
    WorldEventHandler* lastHandler;
    size_t handlerCount,maximumHandlerCount;
};

/* Storage and immutable resource containers remain borrowed. */
void worldEventsInitialize(WorldEventDispatcher* dispatcher,WorldEvent* slots,size_t capacity,
                            const WorldSpatialContainer* containers,size_t containerCount);
void worldEventsDispose(WorldEventDispatcher* dispatcher);
int32_t worldSpatialNodeContains(const WorldSpatialNode* node,const float point[3]);
const WorldSpatialNode* worldSpatialFindChild(const WorldSpatialContainer* container,
                                               const WorldSpatialNode* parent,const float point[3]);
const WorldSpatialNode* worldSpatialFind(const WorldSpatialContainer* container,const float point[3]);
WorldEvent* worldEventsCreate(WorldEventDispatcher* dispatcher,uint32_t mask);
void worldEventsDelete(WorldEventDispatcher* dispatcher,WorldEvent* event);
void worldEventsRetain(WorldEvent* event);
void worldEventsRelease(WorldEvent* event);
WorldEvent* const* worldEventsQuery(WorldEventDispatcher* dispatcher,const float point[3]);
/* emAddHandler1BA2D0/emRemoveHandler1BA3A8: callback identity is the key.
 * Duplicate registrations increment the reference count; their later mask
 * is deliberately ignored. Registration nodes remain valid during callbacks.
 * A callback can retain/release the current event and enqueue deferred events. */
int32_t worldEventsAddHandler(WorldEventDispatcher* dispatcher,WorldEventCallback callback,uint32_t mask);
void worldEventsRemoveHandler(WorldEventDispatcher* dispatcher,WorldEventCallback callback);

void worldEventsRegisterCarHandler(WorldEventDispatcher* dispatcher);
void worldEventsUnregisterCarHandler(WorldEventDispatcher* dispatcher);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <cstdint>
#include <vector>

namespace hp2 {
class WorldEventsData {
public:
    explicit WorldEventsData(const std::vector<std::uint8_t>& bundle);
    WorldEventsData(const WorldEventsData&)=delete;
    WorldEventsData& operator=(const WorldEventsData&)=delete;
    const WorldSpatialContainer* Containers() const { return containers_.data(); }
    std::size_t ContainerCount() const { return containers_.size(); }
private:
    std::vector<std::vector<WorldSpatialNode>> nodes_;
    std::vector<std::vector<WorldSpatialTrigger>> triggers_;
    std::vector<WorldSpatialContainer> containers_;
};
}
#endif

#endif

#ifndef NFSHP2_WORLD_OBJECT_HPP
#define NFSHP2_WORLD_OBJECT_HPP
#include "indep/inc/Ecstasy.hpp"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct WorldObject {
    struct WorldObject* next;
    struct WorldObject* previous;
    EditorMarkerModel model;
    float frame[16];
    int32_t hasFrame;
    int32_t alternatePass;
    int32_t occupied;
    // Some original objects embed this matrix/enable flag in a larger
    // physical owner. Borrow those exact cells instead of duplicating them.
    const float* borrowedFrame;
    const int32_t* borrowedEnabled;
} WorldObject;

typedef struct WorldObjectRegistry {
    WorldObject* first;
    WorldObject* last;
    WorldObject* storage;
    size_t capacity;
    EditorMarkerResources* models;
    size_t count;
    // The borrowed initial slots and nonmoving expansion blocks implement
    // the source bOMalloc pool. Embedded WorldObjects only join the list.
    WorldObject* freeObjects;
    struct WorldObjectPoolBlock* expanded;
    size_t totalCapacity;
} WorldObjectRegistry;

void worldObjectsInitialize(WorldObjectRegistry* registry,WorldObject* storage,
    size_t capacity,EditorMarkerResources* models);
void worldObjectsClose(WorldObjectRegistry* registry);
// PAL1CD760 constructs both separately allocated and embedded WorldObjects.
// Standalone pool capacity is independent of the total live intrusive list.
void worldObjectRegister(WorldObjectRegistry* registry,WorldObject* object,
    uint32_t key,const float frame[16],int32_t alternatePass);
WorldObject* worldObjectCreate(WorldObjectRegistry* registry,uint32_t key,
    const float frame[16],int32_t alternatePass);
void worldObjectDispose(WorldObjectRegistry* registry,WorldObject* object);

#ifdef __cplusplus
}
namespace hp2 {
// Native observations of this one draw traversal; not another object owner.
struct WorldObjectDrawSummary {
    std::size_t visited{};
    std::size_t enabled{};
    std::size_t litObjects{};
    std::size_t drawnObjects{};
    std::size_t stripCommands{};
};
// PAL1CDB20 list/visibility and both lighting branches of1CD998.
WorldObjectDrawSummary RenderWorldObjects(WorldObjectRegistry& objects,
    const CarCinematicScreen& view,EcstasyFrame& frame,std::int32_t worldFrames);
}
#endif
#endif

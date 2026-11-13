#include "indep/inc/EventManager.hpp"

#include <math.h>
#include <string.h>

int32_t worldSpatialNodeContains(const WorldSpatialNode* node,const float point[3])
{
    const float x=fabsf(point[0]-node->center[0]);
    const float y=fabsf(point[1]-node->center[1]);
    const float z=fabsf(point[2]-node->center[2]);
    return !(node->halfExtent[0]<=x || node->halfExtent[1]<=y || node->halfExtent[2]<=z);
}

const WorldSpatialNode* worldSpatialFindChild(const WorldSpatialContainer* container,
                                               const WorldSpatialNode* parent,const float point[3])
{
    for (int32_t index=0;index<parent->count;++index) {
        const WorldSpatialNode* child=&container->nodes[parent->indices[index]];
        if (!worldSpatialNodeContains(child,point)) continue;
        if (child->count<=0) return child;
        const WorldSpatialNode* leaf=worldSpatialFindChild(container,child,point);
        if (leaf) return leaf;
    }
    return NULL;
}

const WorldSpatialNode* worldSpatialFind(const WorldSpatialContainer* container,const float point[3])
{
    const WorldSpatialNode* root=container->nodes;
    if (!root || !worldSpatialNodeContains(root,point)) return NULL;
    return root->count<=0 ? root : worldSpatialFindChild(container,root,point);
}

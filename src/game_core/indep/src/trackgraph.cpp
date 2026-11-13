#include "indep/inc/World.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/CarDriver.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
struct GraphVertex {
    CarRecoveryConnection* connection = nullptr;
    std::int32_t groupId = -1;
    std::int32_t poseIndex = -1;
    float routeDistance = 0;
    std::array<std::int32_t,2> outgoing{{-1,-1}};
    std::array<std::int32_t,2> incoming{{-1,-1}};
};

struct GraphEdge {
    std::int32_t firstVertex = -1;
    std::int32_t lastVertex = -1;
    std::int32_t groupId = -1;
    std::int32_t firstPose = -1;
    std::int32_t lastPose = -1;
    std::int32_t cloaked = 0;
    std::int32_t initiallyCloaked = 0;
    std::int32_t available = 1;
    std::vector<std::int32_t> roadBlocks;
    std::vector<std::int32_t> speedTraps;
};
}

struct TrackGraph {
    const CarRecoveryPointRegistry* road;
    std::vector<GraphVertex> vertices;
    std::vector<GraphEdge> edges;
    std::int32_t hasCloaked = 0;
};

namespace {
const CarRecoveryGroup* findGroup(const TrackGraph& graph, std::int32_t id)
{
    for (std::size_t index=0;index<graph.road->groupCount;++index)
        if (graph.road->groups[index].id==id) return &graph.road->groups[index];
    return nullptr;
}

void addVertex(TrackGraph& graph, const CarRecoveryGroup& group, std::int32_t index)
{
    if (graph.vertices.size()==100) throw std::runtime_error("Track graph exceeds its authored vertex capacity");
    const auto& pose=group.poses[index];
    const std::size_t connectionIndex=pose.flags&255u;
    if (connectionIndex>=graph.road->connectionCount)
        throw std::runtime_error("Track graph vertex references a missing road connection");
    GraphVertex vertex;
    vertex.connection=&graph.road->connections[connectionIndex];
    vertex.groupId=group.id;
    vertex.poseIndex=index;
    TrackRouteLocation location;
    trackRouteLocationAtPoint(graph.road->route,&location,pose.position);
    vertex.routeDistance=location.distance;
    graph.vertices.push_back(vertex);
}

std::int32_t addEdge(TrackGraph& graph, std::int32_t vertexIndex,
    bool followConnection, std::int32_t cloaked)
{
    if (graph.edges.size()==400) throw std::runtime_error("Track graph exceeds its authored edge capacity");
    const auto& vertex=graph.vertices.at(static_cast<std::size_t>(vertexIndex));
    const auto result=static_cast<std::int32_t>(graph.edges.size());
    graph.edges.emplace_back();
    auto& edge=graph.edges.back();
    edge.firstVertex=vertexIndex;
    edge.cloaked=edge.initiallyCloaked=cloaked;
    edge.groupId=followConnection ? vertex.connection->groupId : vertex.groupId;
    edge.firstPose=followConnection ? vertex.connection->poseIndex : vertex.poseIndex;
    const auto* group=findGroup(graph,edge.groupId);
    if (!group || edge.firstPose<0 || edge.firstPose>=group->poseCount)
        throw std::runtime_error("Track graph edge references a missing road pose");
    for (std::int32_t point=edge.firstPose;point<group->poseCount;++point) {
        const auto flags=group->poses[point].flags;
        if (flags&0x4000u) edge.available=0;
        const auto roadBlock=static_cast<std::int32_t>((flags>>8)&63u);
        const auto speedTrap=static_cast<std::int32_t>((flags>>22)&63u);
        if (roadBlock!=63) edge.roadBlocks.push_back(roadBlock);
        if (speedTrap!=63) edge.speedTraps.push_back(speedTrap);
        if (point!=edge.firstPose && (flags&255u)!=255u) {
            const std::int32_t found=trackGraphFindVertex(&graph,edge.groupId,point);
            if (found!=-1) {
                edge.lastVertex=found;
                edge.lastPose=point;
                break;
            }
        }
    }
    return result;
}

void initializeGraph(TrackGraph& graph)
{
    graph.vertices.reserve(100);
    graph.edges.reserve(400);
    for (std::int32_t id=0;id<49;++id) {
        const auto* group=findGroup(graph,id);
        if (!group) continue;
        for (std::int32_t point=0;point<group->poseCount;++point) {
            const auto connectionIndex=group->poses[point].flags&255u;
            if (connectionIndex==255u) continue;
            if (connectionIndex>=graph.road->connectionCount)
                throw std::runtime_error("Track graph pose references a missing connection");
            const auto flags=graph.road->connections[connectionIndex].flags;
            if ((flags&8u) && (flags&7u)!=3u) continue;
            if (flags&0x40u) graph.hasCloaked=1;
            addVertex(graph,*group,point);
        }
    }
    for (std::size_t index=0;index<graph.vertices.size();++index) {
        const auto flags=graph.vertices[index].connection->flags;
        const bool classThree=(flags&7u)==3u;
        if (classThree && !(flags&8u)) continue;
        if ((flags&7u)==2u || classThree) {
            const auto edge=addEdge(graph,static_cast<std::int32_t>(index),true,0);
            graph.vertices[index].outgoing[0]=edge;
        } else {
            const auto first=addEdge(graph,static_cast<std::int32_t>(index),false,0);
            graph.vertices[index].outgoing[0]=first;
            const auto second=addEdge(graph,static_cast<std::int32_t>(index),true,(flags>>6)&1u);
            graph.vertices[index].outgoing[1]=second;
        }
    }
    for (std::size_t index=0;index<graph.edges.size();++index) {
        const auto destination=graph.edges[index].lastVertex;
        if (destination<0) continue;
        auto& incoming=graph.vertices.at(static_cast<std::size_t>(destination)).incoming;
        incoming[incoming[0]==-1 ? 0 : 1]=static_cast<std::int32_t>(index);
    }
    for (auto& vertex:graph.vertices) {
        if (vertex.incoming[1]==-1 || vertex.outgoing[0]==-1) continue;
        const auto& incoming=graph.edges[static_cast<std::size_t>(vertex.incoming[1])];
        if (incoming.cloaked) continue;
        const auto& outgoing=graph.edges[static_cast<std::size_t>(vertex.outgoing[0])];
        if (outgoing.groupId==incoming.groupId) std::swap(vertex.incoming[0],vertex.incoming[1]);
    }
}
}

TrackGraph* trackGraphCreate(const CarRecoveryPointRegistry* road)
{
    if (!road || !road->route || !road->groups || (!road->connections && road->connectionCount)) return nullptr;
    try {
        std::unique_ptr<TrackGraph> graph(new TrackGraph);
        graph->road=road;
        initializeGraph(*graph);
        return graph.release();
    } catch (const std::exception&) {
        return nullptr;
    }
}

void trackGraphDispose(TrackGraph* graph)
{
    delete graph;
}

int32_t trackGraphFindVertex(const TrackGraph* graph, int32_t groupId, int32_t poseIndex)
{
    for (std::size_t index=0;index<graph->vertices.size();++index) {
        const auto& vertex=graph->vertices[index];
        if ((vertex.groupId==groupId && vertex.poseIndex==poseIndex) ||
            (vertex.connection->groupId==groupId && vertex.connection->poseIndex==poseIndex))
            return static_cast<std::int32_t>(index);
    }
    return -1;
}

int32_t trackGraphFindEdge(const TrackGraph* graph, int32_t groupId, int32_t poseIndex)
{
    for (std::size_t index=0;index<graph->edges.size();++index) {
        const auto& edge=graph->edges[index];
        if (!edge.cloaked && edge.groupId==groupId && edge.firstPose<=poseIndex && poseIndex<=edge.lastPose)
            return static_cast<std::int32_t>(index);
    }
    return -1;
}

void trackGraphUncloakAllEdges(TrackGraph* graph)
{
    for (auto& edge:graph->edges) {
        if (!edge.cloaked) continue;
        edge.cloaked=0;
        auto* connection=graph->vertices[static_cast<std::size_t>(edge.firstVertex)].connection;
        connection->flags=static_cast<std::uint8_t>(connection->flags&~0x40u);
    }
    graph->hasCloaked=0;
}

int32_t trackGraphHasCloakedEdges(const TrackGraph* graph) { return graph->hasCloaked; }

// Complete PAL24E318..24E3B4. Only edges authored as cloaked have a mutable
// replay bit. This method has no source BeginSection/EndSection calls and
// does not rewrite the separately owned road connection records.
void trackGraphDoSnapshot(TrackGraph* graph,ReplaySnapshot* snapshot)
{
    replaySnapshotMoveData(snapshot,&graph->hasCloaked,1,4,0);
    for(auto& edge:graph->edges)
        if(edge.initiallyCloaked)
            replaySnapshotMoveData(snapshot,&edge.cloaked,1,4,0);
}

size_t trackGraphVertexCount(const TrackGraph* graph) { return graph->vertices.size(); }
size_t trackGraphEdgeCount(const TrackGraph* graph) { return graph->edges.size(); }

int32_t trackGraphInspectVertex(const TrackGraph* graph, size_t index, int32_t values[8], float* distance)
{
    if (!graph || index>=graph->vertices.size() || !values || !distance) return 0;
    const auto& v=graph->vertices[index];
    values[0]=v.groupId;values[1]=v.poseIndex;values[2]=v.connection->groupId;values[3]=v.connection->poseIndex;
    values[4]=v.outgoing[0];values[5]=v.outgoing[1];values[6]=v.incoming[0];values[7]=v.incoming[1];
    *distance=v.routeDistance;
    return 1;
}

int32_t trackGraphInspectEdge(const TrackGraph* graph, size_t index, int32_t values[8],
    const int32_t** blocks, size_t* blockCount, const int32_t** traps, size_t* trapCount)
{
    if (!graph || index>=graph->edges.size() || !values || !blocks || !blockCount || !traps || !trapCount) return 0;
    const auto& e=graph->edges[index];
    values[0]=e.firstVertex;values[1]=e.lastVertex;values[2]=e.groupId;values[3]=e.firstPose;
    values[4]=e.lastPose;values[5]=e.cloaked;values[6]=e.initiallyCloaked;values[7]=e.available;
    *blocks=e.roadBlocks.data();*blockCount=e.roadBlocks.size();
    *traps=e.speedTraps.data();*trapCount=e.speedTraps.size();
    return 1;
}

// PAL24E880..24EA68. The source walks at most five actual edges, retains
// each authored annotation's order, and follows primary adjacency only.
WorldServicesResult trackGraphFindRoadBlock(const TrackGraph* graph,PursuitManager* manager,
    Car* car,float minimumDistance,float maximumDistance,int32_t forward,PursuitRoadBlock** result)
{
    if(!graph || !graph->road || !graph->road->route || !manager || !manager->world || !car || !result)
        return WORLD_SERVICES_INVALID_BINDING;
    *result=nullptr;
    CarMovement* movement=static_cast<CarMovement*>(car->movement);
    if(!movement || movement->bindings.owner!=car || !movement->frame || !movement->frame->environment.driver)
        return WORLD_SERVICES_INVALID_BINDING;
    const CarDriver* driver=movement->frame->environment.driver;
    const CarRecoveryGroup* group=carDriverCurrentGroup(driver,1);
    const CarRecoveryPose* pose=carDriverCurrentPose(driver,1);
    if(!group || !pose)return WORLD_SERVICES_OK;
    int32_t poseIndex=-1;
    for(int32_t i=0;i<group->poseCount;++i)if(&group->poses[i]==pose){poseIndex=i;break;}
    if(poseIndex<0)return WORLD_SERVICES_INVALID_BINDING;
    int32_t edgeIndex=trackGraphFindEdge(graph,group->id,poseIndex);
    if(edgeIndex<0)return WORLD_SERVICES_OK;
    const float targetDistance=movement->routeLocation.distance;
    for(int32_t hop=0;hop<5;++hop) {
        if(static_cast<size_t>(edgeIndex)>=graph->edges.size())return WORLD_SERVICES_INVALID_BINDING;
        const auto& edge=graph->edges[static_cast<size_t>(edgeIndex)];
        for(int32_t authoredIndex:edge.roadBlocks) {
            PursuitRoadBlock* block=pursuitManagerRoadBlockBySite(manager,authoredIndex);
            if(!block)continue;
            // Complete230DE8 helper: one forward wrap, not signed nearest
            // distance or a new clamp of invalid route positions.
            float distance=forward ? block->routeDistance-targetDistance : targetDistance-block->routeDistance;
            if(distance<0.0f)distance+=graph->road->route->length;
            if(!(distance<maximumDistance) || !(minimumDistance<=distance))continue;
            int32_t clear=0;const auto status=pursuitPositionClear(manager->world,block->groundPoint,&clear);
            if(status!=WORLD_SERVICES_OK)return status;
            if(clear){*result=block;return WORLD_SERVICES_OK;}
        }
        if(hop==4)break;
        const int32_t vertexIndex=forward ? edge.lastVertex : edge.firstVertex;
        if(vertexIndex<0 || static_cast<size_t>(vertexIndex)>=graph->vertices.size())return WORLD_SERVICES_INVALID_BINDING;
        const auto& vertex=graph->vertices[static_cast<size_t>(vertexIndex)];
        edgeIndex=forward ? vertex.outgoing[0] : vertex.incoming[0];
        if(edgeIndex==-1)break;
    }
    return WORLD_SERVICES_OK;
}

// PAL24EA68..24EC3C. Unlike the road-block query, the graph selects a trap
// solely by ordered annotation and route distance. Occupancy and physical
// placement clearance are checked afterwards by the real manager caller.
WorldServicesResult trackGraphFindSpeedTrap(const TrackGraph* graph,PursuitManager* manager,
    Car* car,float minimumDistance,float maximumDistance,int32_t forward,PursuitSpeedTrap** result)
{
    if(!graph || !graph->road || !graph->road->route || !manager || !car || !result)
        return WORLD_SERVICES_INVALID_BINDING;
    *result=nullptr;
    CarMovement* movement=static_cast<CarMovement*>(car->movement);
    if(!movement || movement->bindings.owner!=car || !movement->frame || !movement->frame->environment.driver)
        return WORLD_SERVICES_INVALID_BINDING;
    const CarDriver* driver=movement->frame->environment.driver;
    const CarRecoveryGroup* group=carDriverCurrentGroup(driver,1);
    const CarRecoveryPose* pose=carDriverCurrentPose(driver,1);
    if(!group || !pose)return WORLD_SERVICES_OK;
    int32_t poseIndex=-1;
    for(int32_t i=0;i<group->poseCount;++i)if(&group->poses[i]==pose){poseIndex=i;break;}
    if(poseIndex<0)return WORLD_SERVICES_INVALID_BINDING;
    int32_t edgeIndex=trackGraphFindEdge(graph,group->id,poseIndex);
    if(edgeIndex<0)return WORLD_SERVICES_OK;
    const float targetDistance=movement->routeLocation.distance;
    for(int32_t hop=0;hop<5;++hop) {
        if(static_cast<size_t>(edgeIndex)>=graph->edges.size())return WORLD_SERVICES_INVALID_BINDING;
        const auto& edge=graph->edges[static_cast<size_t>(edgeIndex)];
        for(int32_t authoredIndex:edge.speedTraps) {
            PursuitSpeedTrap* trap=pursuitManagerSpeedTrapBySite(manager,authoredIndex);
            if(!trap)continue;
            float distance=forward ? trap->routeDistance-targetDistance : targetDistance-trap->routeDistance;
            if(distance<0.0f)distance+=graph->road->route->length;
            if(distance<maximumDistance && minimumDistance<=distance){*result=trap;return WORLD_SERVICES_OK;}
        }
        if(hop==4)break;
        const int32_t vertexIndex=forward ? edge.lastVertex : edge.firstVertex;
        if(vertexIndex<0 || static_cast<size_t>(vertexIndex)>=graph->vertices.size())return WORLD_SERVICES_INVALID_BINDING;
        const auto& vertex=graph->vertices[static_cast<size_t>(vertexIndex)];
        edgeIndex=forward ? vertex.outgoing[0] : vertex.incoming[0];
        if(edgeIndex==-1)break;
    }
    return WORLD_SERVICES_OK;
}

#include "static_collisions.hpp"
#include "session.hpp"
#include "track_environment.hpp"
#include "game_core/indep/inc/Polyhedra.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/inc/Contact.hpp"
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <algorithm>
#include <stdexcept>

namespace port::driving {
port::track::VoronoiPose collision_pose(const Memory& m,std::uint32_t body){port::track::VoronoiPose pose;const auto position=m.vector(body+0x10);const auto matrix=m.matrix(body+0x20);for(unsigned j=0;j<3;++j){pose.position[j]=position[j];for(unsigned i=0;i<3;++i)pose.basis[i][j]=matrix[i*4+j];}return pose;}
void StaticCollisions::clear(){shape={};cylinder={};witnesses.clear();witness_order.clear();resolved_contacts=applied_impulses=0;last_impulse={};impulses_by_polygon.clear();}
void StaticCollisions::configure(Memory& m,const Config& config,const std::array<float,3>& cog,const std::vector<std::uint8_t>& geometry){
    // Debug001D4A80, E-0048-D05: code aliases share original BaseModelName collision volume.
    auto volumes=port::track::load_collision_volumes(geometry);const auto name=base_model_name(config)+"_CV";
    auto* volume=hp2::recovered::FindPipelinePolyhedron(volumes,hp2::frontend::bStringHash(name.c_str()),&cog);
    port::track::VoronoiPolyhedron candidate;
    if(volume)candidate=std::move(volume->shape);
    else hp2::recovered::RectPolyhedron::Sub_00212EB0(candidate,m.scalar(0x00325ae4),m.scalar(0x00325ae8),m.scalar(0x00325aec),m.scalar(0x00325af0),m.scalar(0x00325af4),m.scalar(0x00325af8));
    auto bounds=hp2::recovered::DynamicCollisionBody::BuildBoundingVolumes(m,candidate);
    clear();shape=std::move(candidate);cylinder=bounds;
}
std::vector<StaticCollisionEvent> StaticCollisions::contacts(Memory& m,std::uint32_t body,const port::track::Topology& topology,const CollisionMotion& motion){
    if(!ready())throw std::logic_error("Original car collision shape is not installed");
    const auto pose=collision_pose(m,body);auto query=cylinder;query.center=pose.position;query.axis=pose.basis[2];
    const std::array<float,2> low{{pose.position[0]-query.radius,pose.position[1]-query.radius}},high{{pose.position[0]+query.radius,pose.position[1]+query.radius}};
    for(auto it=witnesses.begin();it!=witnesses.end();){const auto& bounds=topology.polygons.at(it->first).bounds;if(bounds[0]>high[0]||bounds[1]>high[1]||bounds[2]<low[0]||bounds[3]<low[1])it=witnesses.erase(it);else ++it;}
    witness_order.erase(std::remove_if(witness_order.begin(),witness_order.end(),[&](auto index){return !witnesses.count(index);}),witness_order.end());
    // Original CreateTrackPolygonCollisionBodies001D46D8 caps new bodies48 then sorts original packed addresses.
    std::vector<std::size_t> create;for(auto index:topology.intersecting(low,high))if(!witnesses.count(index)&&hp2::recovered::DynamicCollisionBody::TrackPolygonCheck(query,topology.polygons[index])&&create.size()<48)create.push_back(index);
    std::sort(create.begin(),create.end());
    // Debug001D3788 +001D3888 order newly-created static bodies by BBoxMax.x, then allocation ID.
    std::stable_sort(create.begin(),create.end(),[&](auto a,auto b){return topology.polygons[a].bounds[2]<topology.polygons[b].bounds[2];});
    for(auto index:create){StaticCollisionWitness instance;hp2::recovered::WaferPolyhedron::Init(instance.wafer,topology.polygons[index],{{0,0,0}});witnesses.emplace(index,std::move(instance));witness_order.push_back(index);}
    const port::track::VoronoiPose fixed;
    std::vector<StaticCollisionEvent> result;
    for(auto index:witness_order){auto& instance=witnesses.at(index);auto& witness=instance.witness;
        if(hp2::recovered::VCWitness::VClip(witness,shape,pose,instance.wafer,fixed)!=2)continue;
        debug_terrain_from_retail(topology.polygons[index].terrain); // A detected unsupported physical interaction fails before response.
        const auto normal=hp2::recovered::FindWaferNormal(m,instance.wafer.faces[0].plane,pose.position);
        const ContactClip clip=[&](float backup,const std::array<float,3>& shift){auto sample=hp2::recovered::DynamicCollisionBody::Interpolate(m,body,motion,backup);for(unsigned j=0;j<3;++j)sample.position[j]+=shift[j];const auto status=hp2::recovered::VCWitness::VClip(witness,shape,sample,instance.wafer,fixed);return ContactQuery{status,witness.distance};};
        result.push_back({index,hp2::recovered::Contact::FindContactTime(m,motion.timestep,clip,normal)});
    }
    // Original ContactTimeSorting001D13A0 + bList::AddSorted002498C0, stable descending backup time.
    std::stable_sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.time.backup_time>b.time.backup_time;});return result;
}
void StaticCollisions::resolve(Memory& m,std::uint32_t body,std::uint32_t car,const StaticCollisionEvent& event,CollisionMotion& motion,const std::function<void(float)>& move,const std::function<void()>& recompute){
    ContactResolveServices services;
    services.backup=[&](float time){hp2::recovered::DynamicCollisionBody::Backup(m,body,motion,time);};
    services.displace=[&](const std::array<float,3>& shift){auto position=m.vector(body+0x10);for(unsigned j=0;j<3;++j)position[j]+=shift[j];m.vector(body+0x10,position);};
    services.recompute=recompute;
    services.impulse=[&]{auto& instance=witnesses.at(event.polygon);const port::track::VoronoiPose fixed;const auto pose=collision_pose(m,body);
        const auto contact=hp2::recovered::Contact::FindContactPointAndNormal(m,instance.witness,fixed,pose,instance.wafer.faces[0].plane);
        return hp2::recovered::Contact::CollisionImpulse(m,body,car,contact);};
    services.move=move;
    const auto impulse=hp2::recovered::Contact::Resolve(event.time,services);++resolved_contacts;
    if(impulse.applied){++applied_impulses;++impulses_by_polygon[event.polygon];last_impulse=impulse.impulse;}

}
void StaticCollisions::process(Memory& m,std::uint32_t body,std::uint32_t car,const port::track::Topology& topology,CollisionMotion& motion,const std::function<void(float)>& move,const std::function<void()>& recompute){hp2::recovered::World::DoTimestepCollisions(m,*this,body,car,topology,motion,move,recompute);}
}

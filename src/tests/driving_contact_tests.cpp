#include "game_core/indep/inc/Contact.hpp"
#include "game_core/indep/src/rigidbody.hpp"
#include "game_core/port/driving/original_image.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void math(){
    using namespace hp2::recovered;
    auto velocity=GetContactPointVelocity({{1,2,3}},{{0,0,2}},{{0,1,0}});check(velocity==std::array<float,3>{{-1,2,3}},"Contact point angular velocity");
    const std::array<float,16> identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    check(DenominatorTerm(identity,{{0,1,0}},{{1,0,0}})==1,"Contact angular denominator");
    std::vector<int> order;port::driving::ContactResolveServices services;
    services.backup=[&](float t){check(t==0.25f,"Original backup time");order.push_back(1);};
    services.displace=[&](const std::array<float,3>&){order.push_back(2);};
    services.recompute=[&]{order.push_back(3);};services.impulse=[&]{order.push_back(4);return port::driving::StaticImpulse{};};
    services.move=[&](float t){check(t==0.25f,"Original remaining movement time");order.push_back(5);};
    Contact::Resolve({0.25f,{}},services);check(order==std::vector<int>({1,2,3,4,3,5}),"Original contact backup/recompute/impulse/removement order");
}
}
int main(){try{
    math();std::filesystem::path elf;try{elf=port::driving::debug_image_path(".");}catch(const std::exception& e){std::cout<<e.what()<<'\n';return 77;}
    auto m=port::driving::read_debug_image(elf);const auto body=m.allocate(0x180),car=m.allocate(0x900);
    const std::array<float,16> identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    m.matrix(body+0x20,identity);m.matrix(body+0x120,identity);m.scalar(body+0x11c,1);
    m.vector(body+0x90,{{-10,0,0,0}});m.vector(body+0xa0,{{0,0,0,0}});
    port::driving::StaticContact contact;contact.normal={{-1,0,0}};
    auto opposite=m;
    auto result=hp2::recovered::Contact::CollisionImpulse(m,body,car,contact);
    check(result.applied&&result.impulse[0]>0&&std::isfinite(result.impulse[0])&&result.scrape_speed==0,"Static car impulse or zero scrape EE divide");
    check(m.scalar(body+0x60)>0,"Car collision did not enter original rigid momentum");
    auto reverse_order=contact;reverse_order.normal[0]=1;
    hp2::recovered::Contact::CollisionImpulse(opposite,body,car,reverse_order,false);
    check(m.vector(body+0x60)==opposite.vector(body+0x60)&&m.vector(body+0x70)==opposite.vector(body+0x70),"Original static/car orders changed physical momentum");
    const auto momentum=m.vector(body+0x60);m.vector(body+0x90,{{10,0,0,0}});
    result=hp2::recovered::Contact::CollisionImpulse(m,body,car,contact);
    check(!result.applied&&m.vector(body+0x60)==momentum,"Receding contact changed momentum");
    port::track::VoronoiPlane face;face.normal={{1,0,0}};face.distance=5;
    check(hp2::recovered::FindWaferNormal(m,face,{{6,0,0}})[0]==1&&hp2::recovered::FindWaferNormal(m,face,{{4,0,0}})[0]==-1,"Original wafer-side normal");
    port::track::VoronoiWitness witness;witness.point1={{1,2,3}};witness.point2={{4,5,6}};
    port::track::VoronoiPose first,second;second.position={{10,0,0}};
    const auto geometry=hp2::recovered::Contact::FindContactPointAndNormal(m,witness,first,second,face);
    check(geometry.point==std::array<float,3>{{7.5f,3.5f,4.5f}},"Original witness point average");
    unsigned queries=0;auto time=hp2::recovered::Contact::FindContactTime(m,1,[&](float backup,const std::array<float,3>&){++queries;return port::driving::ContactQuery{backup<0.6f?2:1,backup<0.6f?0:backup-0.6f};},{{1,0,0}});
    check(queries<=8&&time.backup_time>=0.6f&&time.backup_time<0.7f,"Original contact binary backup search");
    queries=0;time=hp2::recovered::Contact::FindContactTime(m,1,[&](float,const std::array<float,3>& shift){++queries;return port::driving::ContactQuery{shift[0]>0?1:2,0};},{{1,0,0}});
    check(queries==9&&time.crowbar[0]>0,"Original penetration crowbar branch");
    std::cout<<"DRIVING_CONTACT_OK: original static-car impulse/time/math projections; not original numerical equivalence\n";
}catch(const std::exception& error){std::cerr<<"DRIVING_CONTACT_FAILED: "<<error.what()<<'\n';return 1;}}

#pragma once
// Exact original Debug header marker ../indep/inc/Contact.hpp, E-0048-D04.
// These inline host bindings assert neither original inline placement nor object ABI;
// the original implementation translation unit has not been recovered.
#include "game_core/port/driving/collision.hpp"
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace hp2::recovered {
// Original free functions Debug001D1EA8/001D1ED8; E-0048-D04.
inline std::array<float,3> GetContactPointVelocity(const std::array<float,3>& velocity,const std::array<float,3>& angular,const std::array<float,3>& arm){
    const std::array<float,3> cross{{angular[1]*arm[2]-angular[2]*arm[1],angular[2]*arm[0]-angular[0]*arm[2],angular[0]*arm[1]-angular[1]*arm[0]}};
    return {{velocity[0]+cross[0],velocity[1]+cross[1],velocity[2]+cross[2]}};
}
inline float DenominatorTerm(const std::array<float,16>& inertia,const std::array<float,3>& arm,const std::array<float,3>& normal){
    const std::array<float,3> cross{{arm[1]*normal[2]-arm[2]*normal[1],arm[2]*normal[0]-arm[0]*normal[2],arm[0]*normal[1]-arm[1]*normal[0]}};
    std::array<float,3> m{};for(unsigned i=0;i<3;++i)m[i]=(inertia[i]*cross[0]+inertia[4+i]*cross[1])+inertia[8+i]*cross[2];
    const std::array<float,3> second{{m[1]*arm[2]-m[2]*arm[1],m[2]*arm[0]-m[0]*arm[2],m[0]*arm[1]-m[1]*arm[0]}};
    return (normal[0]*second[0]+normal[1]*second[1])+normal[2]*second[2];
}
// Debug001D15C0; E-0048-D04, listing001D1630 clears W only (not XYZ).
inline std::array<float,3> FindWaferNormal(port::driving::Memory& m,const port::track::VoronoiPlane& face,const std::array<float,3>& position){
        auto normal=face.normal;const float dot=(normal[0]*position[0]+normal[1]*position[1])+normal[2]*position[2];
        if(dot<=face.distance-m.scalar(0x003283a0))for(auto& f:normal)f=-f;
        return normal;
}
class Contact {
public:
    // Debug001D18D8; E-0048-D04. Fixed already-physics native car/immovable static body.
    // Static-body callbacks are exact empty branches. Host displacement normalizes body order.
    static port::driving::StaticImpulse Resolve(const port::driving::ContactTime& time,const port::driving::ContactResolveServices& services){
        if(!services.backup||!services.move||!services.displace||!services.recompute||!services.impulse)
            throw std::invalid_argument("Original Contact::Resolve service closure is incomplete");
        services.backup(time.backup_time);services.displace(time.crowbar);services.recompute();
        auto result=services.impulse();services.recompute();services.move(time.backup_time);return result;
    }
    // Debug001D1AE0; E-0048-D04. Both original wafer/car orders; actual witness order defaults carfirst.
    static port::driving::StaticContact FindContactPointAndNormal(port::driving::Memory& m,const port::track::VoronoiWitness& witness,const port::track::VoronoiPose& static_pose,const port::track::VoronoiPose& car_pose,const port::track::VoronoiPlane& wafer_face,bool car_first=true){
        port::driving::StaticContact out;
        const auto& first_pose=car_first?car_pose:static_pose;const auto& second_pose=car_first?static_pose:car_pose;
        for(unsigned i=0;i<3;++i){
            const float first=((first_pose.basis[0][i]*witness.point1[0]+first_pose.basis[1][i]*witness.point1[1])+first_pose.basis[2][i]*witness.point1[2])+first_pose.position[i];
            const float second=((second_pose.basis[0][i]*witness.point2[0]+second_pose.basis[1][i]*witness.point2[1])+second_pose.basis[2][i]*witness.point2[2])+second_pose.position[i];
            out.point[i]=(first+second)*0.5f;
        }out.normal=FindWaferNormal(m,wafer_face,car_pose.position);if(car_first)for(auto& f:out.normal)f=-f;return out;
    }
    // Debug001D1698; E-0048-D04. One immovable wafer/one car. Host clip accepts actual
    // car displacement; caller supplies wafer->car normal, normalizing original order sign.
    static std::array<float,3> Crowbar(port::driving::Memory& m,float backup,const port::driving::ContactClip& clip,const std::array<float,3>& normal){
        float distance=0,increment=m.scalar(0x00325aa4);
        for(unsigned iteration=0;iteration<20;++iteration){distance+=increment;if(increment<m.scalar(0x00325aa8))increment+=m.scalar(0x00325aac);
            std::array<float,3> move{{normal[0]*distance,normal[1]*distance,normal[2]*distance}};
            if(clip(backup,move).status!=2)return move;
        }return {};
    }
    // Debug001D1408; E-0048-D04. Query callback interpolates both original body poses.
    static port::driving::ContactTime FindContactTime(port::driving::Memory& m,float maximum,const port::driving::ContactClip& clip,const std::array<float,3>& wafer_normal){
        port::driving::ContactTime out;float lower=0,upper=maximum;
        for(unsigned iteration=0;iteration<8;++iteration){out.backup_time=lower+(upper-lower)*0.5f;
            auto query=clip(out.backup_time,{});
            if(query.status!=2){upper=out.backup_time;if(query.distance<m.scalar(0x00328398))return out;}
            else lower=out.backup_time;
            if(iteration==7){if(query.status==2)out.crowbar=Crowbar(m,out.backup_time,clip,wafer_normal);return out;}
        }return out;
    }
    // Debug001D1F58; E-0048-D04. Both original native Car/StaticCollisionBody orders:
    // both ConsiderImpulse functions return1, so original loop executes at most once.
    static port::driving::StaticImpulse CollisionImpulse(port::driving::Memory& m,std::uint32_t body,std::uint32_t car,const port::driving::StaticContact& contact,bool car_first=true){
        port::driving::StaticImpulse out;const auto p=m.vector(body+0x10),v=m.vector(body+0x90),w=m.vector(body+0xa0);
        std::array<float,3> arm{{contact.point[0]-p[0],contact.point[1]-p[1],contact.point[2]-p[2]}};
        auto velocity=GetContactPointVelocity({{v[0],v[1],v[2]}},{{w[0],w[1],w[2]}},arm);
        if(car_first)for(auto& f:velocity)f=-f;
        const auto& n=contact.normal;const float incoming=(velocity[0]*n[0]+velocity[1]*n[1])+velocity[2]*n[2];if(0<=incoming)return out;
        std::array<float,3> scrape{{velocity[0]-n[0]*incoming,velocity[1]-n[1]*incoming,velocity[2]-n[2]*incoming}};
        const auto inertia=CarCollisionBody::GetInertiaTensorInverse(m,body);const float inverse=CarCollisionBody::GetMassInverse(m,body);
        const float bounce=std::max(m.scalar(0x00325b1c),m.scalar(0x00325b14));
        const float denominator=inverse+DenominatorTerm(inertia,arm,n);
        float impulse=(-(bounce+1)*incoming)/denominator;
        const float minimum=m.scalar(0x003283ac)/inverse;if(impulse<minimum)impulse=minimum;
        const float car_friction=m.scalar(0x00325b10);float friction=std::max(m.scalar(0x00325b18),car_friction);
        const auto threshold_address=car_first?0x002b0128u:0x002b0120u;
        std::uint64_t bits=m.word(threshold_address)|(std::uint64_t(m.word(threshold_address+4))<<32);double upright;std::memcpy(&upright,&bits,8);
        const bool special=car_friction==m.scalar(car_first?0x00325abc:0x00325ab8)&&double(m.scalar(body+0x20+0x28))>upright;
        if(special)friction=m.scalar(0x00325ac0);
        const float speed=std::sqrt((scrape[0]*scrape[0]+scrape[1]*scrape[1])+scrape[2]*scrape[2]);
        float friction_magnitude=impulse*friction;if(speed<m.scalar(0x003283b8))friction_magnitude*=speed/m.scalar(0x003283b8);
        const float factor=speed==0?port::driving::ee_div_s_zero(-friction_magnitude,speed,"0x001D2450"):-friction_magnitude/speed;
        std::array<float,3> total;for(unsigned i=0;i<3;++i)total[i]=n[i]*impulse+scrape[i]*factor;
        if(car_first)for(auto& f:total)f=-f;
        if(special){const float linear_speed=std::sqrt((v[0]*v[0]+v[1]*v[1])+v[2]*v[2]);if(30<linear_speed){
            float scale=m.scalar(0x00325ac4);if(linear_speed*scale<m.scalar(0x00325ac8))scale=m.scalar(0x00325ac8)/linear_speed;
            const float magnitude=std::sqrt((total[0]*total[0]+total[1]*total[1])+total[2]*total[2]);
            total[0]+=v[0]*scale;total[1]+=v[1]*scale;
            if(1<linear_speed+magnitude){const float arm_scale=magnitude/(linear_speed*m.scalar(car_first?0x00325ad0:0x00325acc)+magnitude);for(auto& f:arm)f*=arm_scale;}
        }}
        CarCollisionBody::ApplyImpulse(m,body,car,{{total[0],total[1],total[2],0}},{{arm[0],arm[1],arm[2],0}});
        out.applied=true;out.impulse=total;out.arm=arm;out.scrape_speed=speed;return out;
    }
};
}

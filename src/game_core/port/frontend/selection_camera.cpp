// TASK-0098 E-0098-C01. Explicit host projection partition for typed camera/math
// adapters; original declaration/whole-entry source markers are unresolved here.
// This file path is port-owned, not evidence of original source ownership.
// Debug NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "selection_camera.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>

namespace hp2::debug {
// Debug0015A430, E-0098-C01; explicit Memory substitutes original math globals.
void SelectCarCameraMover::SetOrientationStats(port::driving::Memory& m,std::uint16_t h,std::uint16_t v){
    HAngleStats=h;VAngleStats=static_cast<std::uint16_t>(0u-v);
    auto orientation=hp2::recovered::bIdentity();
    hp2::recovered::eRotateX(m,orientation,orientation,0);
    hp2::recovered::eRotateY(m,orientation,orientation,0x8000);
    hp2::recovered::eRotateZ(m,orientation,orientation,h);
    bMatrixToQuaternion(OrientationStats,orientation);
}
// Debug0015A4C0, E-0098-C01. Audio transport preserves original sample/call order.
void SelectCarCameraMover::SetMode(SelCarCamMode mode,const port::selection::CameraServices& svc){
    Mode=mode;if(!svc.sound_enabled)return;
    if(!svc.audio)hp2::not_implemented("0x0015A4C0","original selection camera audio service missing");
    using port::selection::CameraAudioOp;
    if(mode==SCC_STATS){svc.audio({CameraAudioOp::play,"MenuCarDisplayOff",1});svc.audio({CameraAudioOp::stop,"MenuCarDisplayActive",0});}
    else if(mode==SCC_ORBIT){svc.audio({CameraAudioOp::play,"MenuCarDisplayActive",0});svc.audio({CameraAudioOp::play,"MenuCarDisplayOn",1});}
}
// Debug0015A668, E-0098-C01. Full source algorithm; presentation publication is
// an explicit port boundary, not an implementation of the entire original Camera.
void SelectCarCameraMover::Update(port::driving::Memory& m,float elapsed,const port::selection::CameraServices& svc){
    if(!svc.publish)hp2::not_implemented("0x0015A668","original selection camera publication service missing");
    if(svc.sound_enabled && !svc.audio)hp2::not_implemented("0x0015A668","original selection camera audio service missing");
    if(!std::isfinite(elapsed) || elapsed<0)hp2::not_implemented("0x0015A668","invalid host RealTimeElapsed binding");
    const auto approach=[&](float& speed,float desired,std::uint32_t increase,std::uint32_t decrease){
        if(speed<desired){speed=speed+elapsed*m.scalar(increase);if(desired<speed)speed=desired;}
        else if(desired<speed){speed=speed-elapsed*m.scalar(decrease);if(speed<desired)speed=desired;}
    };
    approach(HOrbitSpeed,DesiredHOrbitSpeed,0x325004,0x325008);
    approach(VOrbitSpeed,DesiredVOrbitSpeed,0x32500c,0x325010);
    approach(RadiusSpeed,DesiredRadiusSpeed,0x325014,0x325018);
    float t=hp2::recovered::bCos(m,static_cast<std::uint16_t>(port::driving::cvt_w_s(ModeParam*32768.f)));
    t=1.f-(t+1.f)*.5f;if(1.f<t)t=1.f;if(t<0)t=0;
    if(ModeParam==0){
        VAngleOrbit=0xc350;Radius=m.scalar(0x32501c);
        auto orientation=hp2::recovered::bIdentity();
        hp2::recovered::eRotateX(m,orientation,orientation,0);
        hp2::recovered::eRotateY(m,orientation,orientation,0x8000);
        hp2::recovered::eRotateZ(m,orientation,orientation,HAngleStats);
        bMatrixToQuaternion(OrientationOrbit,orientation);
    }
    if(Mode==SCC_ORBIT){
        auto rotate=hp2::recovered::bIdentity();
        hp2::recovered::eRotateZ(m,rotate,rotate,static_cast<std::uint16_t>(port::driving::cvt_w_s(HOrbitSpeed*elapsed)));
        bQuaternion increment;bMatrixToQuaternion(increment,rotate);
        MyQuaternionMultiply(OrientationOrbit,increment,OrientationOrbit);
        // Imported static quatmultcount.5400 at003274B8 belongs to the shared
        // original image, not a per-selector counter. First multiplication normalizes.
        const auto count=m.signed_word(0x3274b8);m.word(0x3274b8,m.word(0x3274b8)+1u);
        if(count%60==0)NormalizeQuaternion(OrientationOrbit);
    }
    bQuaternion interpolated;Slerp(m,interpolated,OrientationStats,OrientationOrbit,t);
    std::array<float,16> rotation;bQuaternionToMatrix(rotation,interpolated);
    auto matrix=hp2::recovered::bIdentity();std::array<float,3> offset{};
    for(int i=0;i<3;++i)offset[i]=(OffsetOrbit[i]-OffsetStats[i])*t+OffsetStats[i];
    hp2::recovered::eTranslate(matrix,matrix,offset);
    hp2::recovered::eMulMatrix(matrix,matrix,rotation);
    using port::selection::CameraAudioOp;
    const auto bounce=[&](float volume,float threshold){
        if(threshold<volume && svc.sound_enabled)svc.audio({CameraAudioOp::play,"MenuCarOrbitBump",volume});
    };
    if(Mode==SCC_ORBIT){
        VAngleOrbit=static_cast<std::uint16_t>(VAngleOrbit+port::driving::cvt_w_s(VOrbitSpeed*elapsed));
        if(VAngleOrbit>=0xebac){VAngleOrbit=0xebab;VOrbitSpeed=-VOrbitSpeed*m.scalar(0x325020);
            bounce(std::abs(VOrbitSpeed)*m.scalar(0x325024),m.scalar(0x325028));}
        if(VAngleOrbit<0xc350){VAngleOrbit=0xc350;VOrbitSpeed=-VOrbitSpeed*m.scalar(0x32502c);
            bounce(std::abs(VOrbitSpeed)*m.scalar(0x325030),m.scalar(0x325034));}
    }
    const float vertical=float(VAngleStats)+t*float(int(VAngleOrbit)-int(VAngleStats));
    hp2::recovered::eRotateX(m,matrix,matrix,static_cast<std::uint16_t>(port::driving::cvt_w_s(vertical)));
    if(Mode==SCC_ORBIT){
        Radius=Radius+RadiusSpeed*elapsed;
        if(15.5f<Radius){Radius=15.5f;RadiusSpeed=RadiusSpeed*m.scalar(0x325038);
            bounce(std::abs(RadiusSpeed)*m.scalar(0x32503c),m.scalar(0x325040));}
        if(Radius<m.scalar(0x325044)){Radius=m.scalar(0x325044);RadiusSpeed=RadiusSpeed*m.scalar(0x325048);
            bounce(std::abs(RadiusSpeed)*m.scalar(0x32504c),m.scalar(0x325050));}
        if(svc.sound_enabled){
            const float horizontal=std::abs(HOrbitSpeed)*m.scalar(0x325054);
            const float vertical_volume=std::abs(VOrbitSpeed)*m.scalar(0x325058);
            const float radius_volume=std::abs(RadiusSpeed)*m.scalar(0x32505c);
            svc.audio({CameraAudioOp::volume,"MenuCarDisplayActive",std::max(std::max(horizontal,vertical_volume),radius_volume)});
            svc.audio({CameraAudioOp::pitch,"MenuCarDisplayActive",
                (((Radius-m.scalar(0x325060))*m.scalar(0x325064))*m.scalar(0x325068)+m.scalar(0x32506c))-1.f});
        }
    }
    const float distance=RadiusStats+t*(Radius-RadiusStats);
    hp2::recovered::eTranslate(matrix,matrix,{{0,0,distance}});
    if(Mode==SCC_STATS){ModeParam=ModeParam-m.scalar(0x325070);if(ModeParam<0)ModeParam=0;}
    else if(Mode==SCC_ORBIT){ModeParam=ModeParam+m.scalar(0x325074);if(1.f<ModeParam)ModeParam=1;}
    svc.publish(matrix,elapsed,4500,distance);
}
// Debug002532D8, exact symbol bQuaternionToMatrix; thunk MyQuaternionToMatrix001A6680.
// E-0098-C01. Preserve row-major source signs/order, not host quaternion conventions.
void bQuaternionToMatrix(std::array<float,16>& matrix,const bQuaternion& q){
    const float x2=q.x+q.x,y2=q.y+q.y,z2=q.z+q.z,w2=q.w+q.w;
    const float yy=y2*q.y,yz=y2*q.z,one_xx=1.f-x2*q.x,zz=z2*q.z;
    matrix={{(1.f-yy)-zz,x2*q.y-w2*q.z,x2*q.z+w2*q.y,0,
        x2*q.y+w2*q.z,one_xx-zz,yz-w2*q.x,0,
        x2*q.z-w2*q.y,yz+w2*q.x,one_xx-yy,0,0,0,0,1}};
}
// Debug00253398, exact symbol bMatrixToQuaternion; thunk001A6688. E-0098-C01.
void bMatrixToQuaternion(bQuaternion& q,const std::array<float,16>& m){
    const float trace=(m[0]+m[5])+m[10];
    if(trace>0){
        const float root=std::sqrt(trace+1.f),scale=.5f/root;
        q.w=root*.5f;q.x=(m[6]-m[9])*scale;q.y=(m[8]-m[2])*scale;q.z=(m[1]-m[4])*scale;
    }else{
        // Original table002B8220, captured independently:1,2,0.
        constexpr int next[3]={1,2,0};int i=m[0]<m[5]?1:0;
        if(m[i*5]<m[10])i=2;
        const int j=next[i],k=next[j];
        float root=std::sqrt((m[i*5]-(m[j*5]+m[k*5]))+1.f);
        std::array<float,4> result{};result[i]=root*.5f;
        if(root!=0)root=.5f/root;
        result[3]=(m[j*4+k]-m[k*4+j])*root;
        result[j]=(m[i*4+j]+m[j*4+i])*root;
        result[k]=(m[i*4+k]+m[k*4+i])*root;
        q={result[0],result[1],result[2],result[3]};
    }
}
// Debug001A4A60, E-0098-C01. Inputs are MUTABLE in the original.
void Slerp(port::driving::Memory& m,bQuaternion& out,bQuaternion& first,bQuaternion& second,float t){
    float dot=((first.w*second.w+first.x*second.x)+first.y*second.y)+first.z*second.z;
    if(dot<0){first.x=-first.x;first.y=-first.y;first.z=-first.z;first.w=-first.w;dot=-dot;}
    float a;
    if(m.scalar(0x325798)<dot+1.f){
        if(1.f-dot<m.scalar(0x325798))a=1.f-t;
        else{
            const auto angle=static_cast<std::uint16_t>(0x4000u-hp2::recovered::bASin(m,dot));
            const float denominator=hp2::recovered::bSin(m,angle);
            a=hp2::recovered::bSin(m,static_cast<std::uint16_t>(port::driving::cvt_w_s(float(angle)*(1.f-t))))/denominator;
            t=hp2::recovered::bSin(m,static_cast<std::uint16_t>(port::driving::cvt_w_s(float(angle)*t)))/denominator;
        }
    }else{
        second.x=-first.y;second.y=first.x;second.z=-first.w;second.w=first.z;
        const float radians=m.scalar(0x32579c),scale=m.scalar(0x3257a0);
        a=hp2::recovered::bSin(m,static_cast<std::uint16_t>(port::driving::cvt_w_s(((.5f-t)*radians)*scale)));
        t=hp2::recovered::bSin(m,static_cast<std::uint16_t>(port::driving::cvt_w_s((t*radians)*scale)));
    }
    // Inputs may alias the destination. Original w,x,y,z write order is retained.
    out.w=a*first.w+t*second.w;out.x=a*first.x+t*second.x;
    out.y=a*first.y+t*second.y;out.z=a*first.z+t*second.z;
}
}

namespace port::selection {
CameraFrame camera_frame_from_view(const std::array<float,16>& view,int fov,float distance){
    CameraFrame frame;frame.view=view;frame.fov_bangle=fov;frame.target_distance=distance;
    for(int i=0;i<3;++i)frame.position[i]=-((view[12]*view[i*4]+view[13]*view[i*4+1])+view[14]*view[i*4+2]);
    const float norm=std::sqrt((view[2]*view[2]+view[6]*view[6])+view[10]*view[10]);
    if(!(norm>0) || !std::isfinite(norm))hp2::not_implemented("0x00156868","invalid original camera direction projection");
    for(int i=0;i<3;++i){frame.target[i]=frame.position[i]+(view[i*4+2]/norm)*distance;frame.up[i]=-view[i*4+1];}
    return frame;
}
}

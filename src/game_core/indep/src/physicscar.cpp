// TASK-0044: canonical Debug PhysicsCar, original source ../indep/src/physicscar.cpp.
// NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/indep/inc/PhysicsCar.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/port/driving/projection_support.hpp"
namespace hp2::recovered {
// Debug 001375B0; E-0044-P01/P02.
void PhysicsCar::Calibrate() {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00137100_runtime;
    const auto config=m.word(object+0x4c),body=m.word(object+0x18);
    m.copy(body+0x160,config+0x110,16);
    init::CalculateInertiaTensor(m.view(body,0x180),m.view(m.word(body+0xc),0x50));
    float offset=m.scalar(config+0x110);if(!m.word(global_address(0xa704)))offset=0;
    const float numerator=m.scalar(config+0x120)-offset;
    const float split=(numerator/(numerator-(m.scalar(config+0x180)-offset)))*0.5f;
    float total=m.scalar(body+0x118)*global(m,0x83d0);
    const float additional=GetDownforceAtSpeed(m,m.word(object+0x48),m.scalar(m.word(object+0x54)+0x3c0));
    total=total+m.scalar(config+0x2fc)*additional;
    const float first=total*(0.5f-split),second=total*split;
    for(unsigned i=0;i<4;++i){const auto controller=m.word(object+0x20+4*i);m.scalar(controller+0x78,i<2?first:second);refresh(m,controller);}
    for(unsigned i=0;i<4;++i)m.word(m.word(object+0x30+4*i)+0x24,m.word(global_address(0xb3b0)));

}

// Debug 00138C80; E-0044-P01/P02.
void PhysicsCar::InitializeStateFromCar() {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00137100_runtime;
    const auto car=m.word(object+0x50);
    Sub_0019CC88(m,m.word(object+0x18),car+0x20,car+0x50,car+0x30,car+0x90);
    for(unsigned i=0;i<4;++i){
        init::Sub_001A5FB0(m.view(m.word(object+0x20+4*i),0xa0));
        init::SetState(m.view(m.word(object+0x30+4*i),0x184),m.scalar(car+0x2c0+4*i),m.scalar(car+0x2b0+4*i));
    }
    float value=global(m,0xb834);if(value==100.0f)value=m.scalar(car+0x390);
    m.scalar(m.word(object+0x40)+0xc,value*global(m,0x844c));
    const auto drive=m.word(object+0x44),word=m.word(car+0x388);
    m.word(drive+0x2c,word);m.word(drive+0x38,3);m.scalar(drive+0xc,1);m.word(drive+0x4c,0);m.word(drive+0x30,word);

}

// Debug 00137798; E-0044-P01/P02.
DampingSuitability PhysicsCar::CheckDampingSuitability() {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00137100_runtime;
    const auto car=m.word(object+0x54),body=m.word(object+0x18);const auto mode=m.signed_word(car+0x3bc);
    if(!(m.scalar(m.word(object)+0x214)<=global(m,0x83d4))&&mode>=1)return DS_NO_DAMP;
    const auto speed=length(m.vector(body+0x90)),angular=length(m.vector(body+0xa0));
    if(!(speed<=global(m,0x83d8))||!(angular<=global(m,0x83d8))||!(std::abs(m.scalar(body+0x28))<=0.25f)||!(std::abs(m.scalar(body+0x38))<=0.25f))return DS_NO_DAMP;
    if(IsBlinking(m,car)||(mode==0&&m.byte(m.word(car+0x34)+0x12)==0))return DS_NO_DAMP;
    return speed<=0.25f&&angular<=0.25f?DS_DOUSE:DS_DAMP;

}

// Debug 001378C0; E-0044-P01/P02.
void PhysicsCar::BottomOut(port::driving::native::InitServices& services) {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00137100_runtime;
    bool changed=false;const auto body=m.word(object+0x18),car=m.word(object+0x54);
    for(unsigned i=0;i<4;++i){const auto wheel=m.word(object+0x20+4*i);
        if(m.scalar(wheel+0x14)==m.scalar(m.word(wheel+0xc)+0x14)){changed=true;const float result=ApplyGroundPenaltyImpulse(m,wheel);const auto target=m.word(object+0x50)+0x2f0+4*i;m.scalar(target,m.scalar(target)+result);}}
    const auto mode=CheckDampingSuitability();
    if(mode==1){const auto factor=global(m,0x83e0);for(auto offset:{0x60,0x70}){auto v=m.vector(body+offset);for(auto& lane:v)lane=lane*factor;m.vector(body+offset,v);}
        for(unsigned i=0;i<4;++i){const auto wheel=m.word(object+0x30+4*i);m.scalar(wheel+0x20,m.scalar(wheel+0x20)*factor);}
        changed=true;if(global(m,0x83e4)<length(m.vector(body+0x90)))m.word(object+0x6c,0);m.byte(m.word(car+0x34)+0x13,1);
    }else if(mode==0){if(global(m,0x83dc)<length(m.vector(body+0x90)))m.word(object+0x6c,0);m.byte(m.word(car+0x34)+0x13,1);
    }else if(mode==2){
        if(!services.Field_00137938_stack_w)hp2::not_implemented("0x001378C0","Explicit original temporary W projection is absent");
        for(auto offset:{0x60,0x70}){for(unsigned i=0;i<3;++i)m.word(body+offset+4*i,0);m.word(body+offset+12,*services.Field_00137938_stack_w);}
        if(m.signed_word(car+0x3bc)>0)for(unsigned i=0;i<4;++i)m.word(m.word(object+0x30+4*i)+0x20,0);
        changed=true;
        if(m.scalar(body+0x48)<global(m,0x83e8)){
            const auto start=m.word(object+0x6c);if(!start||start==0x7fffffff)m.word(object+0x6c,m.word(global_address(0xb388)));
            else if(1.0f<float(signed_bits(m.word(global_address(0xb388))-start))*global(m,0x83ec))PleaseResetMeLater(m,car,services);
        }else m.word(object+0x6c,0);
        m.byte(m.word(car+0x34)+0x13,0);
    }
    if(m.word(global_address(0xa708))&&m.word(car+0x20)==1){
        if(!services.Field_00137938_stack_w)hp2::not_implemented("0x001378C0","Explicit original temporary W projection is absent");
        std::array<float,4> point{{0,0,0,0}};std::memcpy(&point[3],&*services.Field_00137938_stack_w,4);
        auto input=point;input[2]=25;
        m.word(global_address(0xa708),0);ApplyImpulse(m,body,input,point);changed=true;
    }
    if(changed){m.word(global_address(0xb408),0);PhysicsSystem::ComputeAuxiliaries(m,object+8,services);}

}

// Debug00137748 E-0044-P01/P02.
void PhysicsCar::ComputeAuxiliaries(port::driving::native::InitServices& services){
 auto& m=m_;const auto object=address_;
 port::driving::native::PhysicsSystem::ComputeAuxiliaries(m,object+8,services);BottomOut(services);
 const auto table=m.word(object+0x14),target=m.word(table+0x1c),adjusted=object+8+static_cast<std::int32_t>(m.signed_half(table+0x18));
 if(target!=0x00138d90)hp2::not_implemented("0x00137748","Unknown final virtual target");
 if(!services.ResolveForces)hp2::not_implemented("0x00138D90","Final constructor synchronization service is absent");
 services.ResolveForces(m,adjusted);
}
// Debug 001370C0; E-0044-P01/P02.
void PhysicsCar::Initialize(std::uint32_t config,std::uint32_t controls,std::uint32_t car,port::driving::native::InitServices& services) {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00137100_runtime;
    m.word(object+4,0x002a67d0);init::Sub_0019C170(m.view(object+8,0x10),object+8);m.word(object+0x14,0x002a6b18);
    m.word(object,controls);m.word(object+0x58,0);m.word(object+0x5c,0);m.word(object+0x6c,0);m.word(object+0x4c,config);m.word(object+0x54,car);
    m.word(object+0x14,0x002a6ad8);m.word(object+4,0x002a6af8);m.word(object+0x50,car+0x40);m.scalar(object+0x70,1);
    // Port-owned capacities are bounds for recovered offsets, not recovered pool sizes.
    const auto body=m.allocate(0x180);init::Sub_0019C9B0(m.view(body,0x180),config+0x1e0,m.view(config+0x1e0,0x50));m.word(object+0x18,body);AddComponent(m,object+8,body);
    m.word(body+0x174,m.word(config+0x320));m.word(body+0x114,m.byte(config+0xe4)!=0);
    const auto steering=m.allocate(0x90);m.word(steering,config);m.word(steering+8,0x002adad8);m.word(steering+4,steering+0x10);m.word(object+0x1c,steering);
    for(unsigned i=0;i<4;++i){const auto axle=(i/2)*0x20;const auto wheel=m.allocate(0x184);
        init::Sub_001A3F10(m.view(wheel,0x184),config+0x120+i*0x20,config+0x1a0+axle,config+0x300,car+0x40);m.word(object+0x30+4*i,wheel);
        const auto controller=m.allocate(0xb0);init::Sub_001A5EA0(m.view(controller,0xb0),i,body,wheel,i<2?steering:0,config+0x230+axle);m.word(object+0x20+4*i,controller);
        AddComponent(m,object+8,wheel);AddComponent(m,object+8,controller);}
    for(unsigned i=0;i<4;++i)m.word(m.word(object+0x20+4*i)+0x10,m.word(object+0x20+4*(i^1)));
    const auto engine=m.allocate(0x60);port::driving::Sub_0018A7E8(m,engine,config+0x2b0,car+0x40);m.word(object+0x40,engine);AddComponent(m,object+8,engine);m.word(engine+0x40,m.byte(config+0xe4)!=0);
    const auto drive=m.allocate(0xa0);port::driving::Sub_0018AE70(m,drive,config+0x270,engine,object+0x30,car+0x40);m.word(object+0x44,drive);AddComponent(m,object+8,drive);
    const auto helper=m.allocate(0x18);m.scalar(helper+8,1);m.scalar(helper+0xc,1);m.word(helper+0x10,0);m.word(helper+0x14,0);m.word(object+0x48,helper);m.word(helper+4,body);m.word(helper,config+0x2f0);
    m.word(global_address(0xb408),0);Calibrate();InitializeStateFromCar();ComputeAuxiliaries(services);
    m.scalar(object+0x74,1);m.word(object+0x64,1);m.word(object+0x68,2);m.word(object+0x60,1);return;

}

// Debug 00139418; E-0044-P01/P02.
void PhysicsCar::SetBoost(float value) {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00137d90;
    const auto car=m.word(s+0x54);float f6=value;
    m.scalar(m.word(s+0x40)+0x58,f6);m.scalar(m.word(s+0x18)+0x178,value);
    const auto helper=m.word(s+0x48);m.scalar(helper+8,value);m.scalar(helper+0xc,1.0f-m.scalar(car+0x458));
    for(unsigned i=0;i<4;++i)m.scalar(m.word(s+0x30+4*i)+0x17c,value);

}

// Debug 00137D18; E-0044-P01/P02.
float PhysicsCar::Move(float dt,port::driving::native::DrivingServices& hooks) {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00137d90;
    const auto car=m.word(s+0x54);
    const auto mode_address=m.word(ga(0xa900));
    if(m.signed_word(mode_address)==4 && m.word(ga(0xa758))!=0)hp2::not_implemented("0x00137D18","Excluded original Race4 diagnostic position-restore branch");
    const auto start=invoke(hooks.Sub_00250F38,"0x0024A340");
    SetBoost(m.scalar(car+0x3e0));Calibrate();
    const auto controls=m.word(s),body=m.word(s+0x18),state=m.word(s+0x50),config=m.word(s+0x4c);
    const auto engine=m.word(s+0x40),drive=m.word(s+0x44);
    if(m.scalar(controls+0x218)<m.scalar(controls+0x214) && m.scalar(controls+0x21c)<m.scalar(controls+0x214)) {
        if(m.scalar(controls+0x218)<0.5f)m.word(controls+0x218,0);
        if(m.scalar(controls+0x21c)<0.5f)m.word(controls+0x21c,0);
    }else if(m.scalar(controls+0x214)<0.5f)m.word(controls+0x214,0);
    const auto steering=m.word(s+0x1c),table=m.word(steering+8);
    if(m.word(table+0x14)!=0x001a3540)hp2::not_implemented("0x00137D18","Unrecovered steering virtual target");
    Sub_001A5D60(m,steering+static_cast<std::int32_t>(m.signed_half(table+0x10)),m.half(controls+0x210));
    m.word(engine+0x44,m.word(controls+0x214));
    SetSportGear(m,drive,m.signed_word(controls+0x224));
    if(m.word(controls+0x228)!=0){m.word(engine+0x50,1);m.scalar(engine+0x54,static_cast<float>(m.signed_word(ga(0xb388)))*g(m,0x83f4));}
    DecrementNitro(m,engine);m.word(controls+0x228,0);
    m.word(controls+0x220,static_cast<std::uint32_t>(SetDesiredGear(m,drive,m.signed_word(controls+0x220))));
    m.word(controls+0x224,m.word(drive+0x48));
    const bool active=g(m,0x83f8)<m.scalar(controls+0x214) ||
        (m.word(state+0x37c)==4 && g(m,0x83fc)<std::fabs(m.scalar(state+0x58)));
    m.word(body+0x110,active?0:1);
    float f24=m.scalar(s+0x70);if(m.word(state+0x39c)==0)f24=f24*g(m,0xa724);
    for(unsigned i=0;i<4;++i)m.word(state+0x2fc-4*i,0);
    const float f25=m.scalar(state+0x380)*g(m,0x8400);
    // Debug00137F60..00138294; E-0046-P01. REAL_CONTROL burnout state,
    // original source ../indep/src/physicscar.cpp. These are Debug globals,
    // not a Retail address delta; preserve instruction comparison/order.
    if(m.word(car+0x20)==1) {
        if(!hooks.IsJoystickTypeWheel)hp2::not_implemented("0x0011D720","Joystick classification service is absent");
        if(!hooks.IsJoystickTypeWheel(m.signed_word(controls+0x234))) {
            if(m.word(state+0x3a4)!=0) {
                const float remaining=m.scalar(state+0x3a8)-m.scalar(0x00327d08);
                m.scalar(state+0x3a8,remaining);
                if(remaining<0)m.word(state+0x3a4,0);
                else if(m.scalar(0x003270a4)<std::fabs(GetYaw(m,state)))m.word(state+0x3a4,0);
                else {
                    bool below=true;
                    for(unsigned i=0;i<4;++i)if(!(m.scalar(m.word(s+0x30+4*i)+0x108)<m.scalar(0x0032708c))){below=false;break;}
                    if(below){const float allow=m.scalar(state+0x3b0)+m.scalar(0x00327d08);m.scalar(state+0x3b0,allow);
                        if(m.scalar(0x0032709c)<allow)m.word(state+0x3a4,0);
                    }else m.word(state+0x3b0,0);
                }
            } else if(m.word(state+0x39c)==1 && m.half(controls+0x210)==0 &&
                      0<f25 && f25<m.scalar(0x00327090) && std::fabs(GetYaw(m,state))<m.scalar(0x003270a0)) {
                const float threshold=m.scalar(0x00327084);
                const float rear_left=m.scalar(m.word(s+0x38)+0x108),rear_right=m.scalar(m.word(s+0x3c)+0x108);
                int selected=-1;
                if(threshold<rear_left || threshold<rear_right){
                    if(rear_right<rear_left){selected=2;m.word(state+0x3a4,1);}
                    else{selected=3;m.word(state+0x3a4,2);}
                }else{
                    const float front_left=m.scalar(m.word(s+0x30)+0x108),front_right=m.scalar(m.word(s+0x34)+0x108);
                    if(threshold<front_left || threshold<front_right){
                        if(front_left<front_right){selected=1;m.word(state+0x3a4,1);}
                        else{selected=0;m.word(state+0x3a4,2);}
                    }
                }
                if(selected>=0 && !IsBlinking(m,car)){
                    float torque=(m.scalar(m.word(s+0x30+4*selected)+0x108)-threshold)/(m.scalar(0x00327088)-threshold);
                    if(1<torque)torque=1;
                    const float speed=f25/m.scalar(0x00327090);
                    float traction=m.scalar(0x00327094);
                    const auto terrain2=m.word(m.word(s+0x38)+0x150),terrain3=m.word(m.word(s+0x3c)+0x150);
                    if(m.scalar(0x003102e8+0x90*terrain2+0x20)!=1 || m.scalar(0x003102e8+0x90*terrain3+0x20)!=1)traction=traction*1.25f;
                    m.word(state+0x3a8,m.word(0x00327098));
                    float result=speed*(1.0f-torque);result=result*(1.0f-traction);result=traction+result;
                    m.scalar(state+0x3ac,result);
                }
            }
        }
    }
    float f23=m.scalar(s+0x74);
    if(0.5f<=m.scalar(controls+0x21c)){f23=f23-dt*20.0f;if(f23<0.5f)f23=0.5f;}
    else {f23=f23+dt;if(1.0f<f23)f23=1.0f;}
    m.scalar(s+0x74,f23);
    for(unsigned i=0;i<4;++i) {
        const auto wheel=m.word(s+0x30+4*i);
        if(m.signed_word(mode_address)==4 || (m.word(s+0x60)!=0 && i<2)) {
            float f0;
            if(g(m,0x8404)<m.scalar(controls+0x214) && 0.5f<m.scalar(controls+0x218) && f25<10.0f && i>=2) f0=f25*g(m,0x8408);
            else f0=m.scalar(controls+0x218)*m.scalar(0x2e7d90+4*i);
            m.scalar(wheel+0x170,f0);
        }else m.word(wheel+0x170,0);
        float f20=m.scalar(controls+0x21c)*m.scalar(0x2e7da0+4*i);
        bool add;
        if(m.word(state+0x39c)==0)add=g(m,0x840c)<f20;
        else add=g(m,0x8410)<f20 && g(m,0x8414)<std::fabs(GetYaw(m,state)) && f25<80.0f;
        if(add)f20=f20+0.5f;
        m.scalar(wheel+0x174,f20);
        if(i==2) {
            float a1=m.scalar(body+0x28);
            const float factor=port::driving::YawFrictionBoost(m,GetYaw(m,state),f20,m.scalar(state+0x384),
                m.scalar(config+0x310),m.scalar(config+0x314),GetPostCollisionYawControlBoost(m,car),m.signed_word(state+0x39c),a1);
            float f0=factor-1.0f;f0=f23*f0;f0=f0+1.0f;f24=f24*f0;
        }
        m.scalar(wheel+0x178,f24);
        const auto selector=m.word(state+0x3a4);
        if(((i==0 || i==3)&&selector==1) || ((i==1 || i==2)&&selector==2))m.scalar(wheel+0x178,f24*m.scalar(state+0x3ac));
        m.scalar(wheel+0x16c,((m.signed_byte(car+0x848)>>i)&1)?1.0f:0.0f);
    }
    if(m.scalar(car+0x460)!=0 && m.signed_byte(car+0x848)==0)SetBoost(m.scalar(car+0x460));
    const auto helper=m.word(s+0x48);m.word(helper+0x10,m.word(controls+0x214));
    m.scalar(helper+0x14,static_cast<float>(m.signed_word(state+0x37c))*0.25f);
    if(m.word(state+0x39c)==0)m.word(body+0x170,m.word(config+0x318));
    else {
        float f20=m.scalar(config+0x31c);
        if(f20==0){const float angle=GetYaw(m,state);if(angle*m.scalar(body+0x78)<0 && g(m,0x8418)<std::fabs(angle))f20=20;}
        m.scalar(body+0x170,f20);
    }
    float scale=1;
    if(m.word(car+0x844)!=0)scale=1.0f-invoke(hooks.Sub_001B50C8,"0x001B50C8",m,m.word(car+0x844)+8)*g(m,0x841c);
    if(m.word(engine+0x50)!=0)m.word(body+0x174,0);
    else if(m.signed_byte(car+0x848)!=0)m.scalar(body+0x174,60);
    else m.scalar(body+0x174,m.scalar(config+0x320)*scale);
    const auto steps=sb(static_cast<std::uint32_t>(port::driving::cvt_w_s(dt/g(m,0xa728)))+1);
    const float subdt=dt/static_cast<float>(steps);
    for(std::int32_t i=0;i<steps;++i) {
        if(m.word(ga(0xa758))!=0 || m.signed_word(mode_address)!=4)m.copy(0x2e7db0,body+0x10,0x50);
        m.scalar(ga(0xb408),subdt);
        const auto begin=invoke(hooks.Sub_00250F38,"0x0024A340");
        invoke(hooks.IntegrateEuler,"0x00199BB8",m,s+8,subdt);
        invoke(hooks.eOrthonormalizeMatrix,"0x0011C108",m,body+0x20);
        const auto end=invoke(hooks.Sub_00250F38,"0x0024A340");invoke(hooks.Sub_00250F48,"0x00250F48",begin,end);
        if(m.word(ga(0xa6fc))!=0)invoke(hooks.Sub_0019D980,"0x0019D980",m,body);
        if(m.word(ga(0xa700))!=0)invoke(hooks.Sub_0018C030,"0x0018C030",m,drive);
        const auto mode=m.signed_word(mode_address);
        if(mode==4) {
            // ELF00138B9C..00138BD4; E-0038-E13. Race state4 is the ordinary
            // running branch when the separate A758 diagnostic flag is clear.
            if(m.word(ga(0xa758))!=0)hp2::not_implemented("0x00137D18","Excluded original Race4 diagnostic position-restore branch");
            m.scalar(ga(0xa784),static_cast<float>(m.signed_word(ga(0xb388)))*g(m,0x8448));
            m.scalar(ga(0xa79c),1);m.word(ga(0xa788),0);m.scalar(ga(0xa794),1);
            m.scalar(ga(0xa798),1);m.word(ga(0xa790),0);m.word(ga(0xa78c),0);
        } else if(mode>=2) {
            const float tickscale=g(m,0x8420);
            if(static_cast<float>(m.signed_word(s+0x58))*tickscale<static_cast<float>(sb(m.word(ga(0xb388))-m.word(s+0x5c)))*tickscale)m.word(s+0x60,1);
            if(0.5f<m.scalar(controls+0x214)) {
                if(m.word(s+0x64)!=0 && m.word(s+0x68)!=0) {
                    float f0=bRandom(m,g(m,0x8424));f0=f0+g(m,0x8428);f0=f0*4000;f0=f0+0.5f;
                    m.word(s+0x60,0);m.word(s+0x64,0);m.word(s+0x58,static_cast<std::uint32_t>(port::driving::cvt_w_s(f0)));
                    m.word(s+0x68,m.word(s+0x68)-1);
                    f0=static_cast<float>(m.signed_word(ga(0xb388)))*tickscale;f0=f0*4000;f0=f0+0.5f;
                    m.word(s+0x5c,static_cast<std::uint32_t>(port::driving::cvt_w_s(f0)));
                }
            }else{m.word(s+0x60,1);m.word(s+0x64,1);}
        }
        if(mode!=4 && m.word(s+0x60)!=0) {
            if(!hooks.Field_001389A4_stack_w)hp2::not_implemented("0x00137D18","Explicit original temporary W projection is absent");
            for(unsigned j=0;j<3;++j)m.word(body+0x60+4*j,0);
            m.word(body+0x6c,*hooks.Field_001389A4_stack_w);
        }
    }
    if(m.word(ga(0xa758))!=0)for(auto off:{0xa788,0xa78c,0xa790})if(0<g(m,off))m.scalar(ga(off),g(m,off)-g(m,0xb398));
    invoke(hooks.UpdateCar,"0x00139110",m,s,dt);
    if(ShouldVisualEffectsBeGenerated(m,car))for(unsigned i=0;i<4;++i)invoke(hooks.MaybeMakeSkids,"0x001A2CF8",m,m.word(s+0x30+4*i),car,dt);
    const auto final=invoke(hooks.Sub_00250F38,"0x0024A340");invoke(hooks.Sub_00250F48,"0x00250F48",start,final);
    return dt;

}

// Debug 00139478; E-0044-P01/P02.
void PhysicsCar::JumpStabilizer() {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00138e08;
    if(m.signed_word(m.word(mover+0x54)+0x3bc)>=2)return;
    const auto body=m.word(mover+0x18);const auto v=m.vector(body+0xa0);const float mass=m.scalar(body+0x118);
    const float f=port::driving::Graph::GetValue(m,0x003270c0,len(v));
    const auto damping=mul(v,-(mass*f));auto old=m.vector(body+0x100);for(unsigned i=0;i<4;++i)old[i]=old[i]+damping[i];m.vector(body+0x100,old);

}

// Debug 00138EC0; E-0044-P01/P02.
void PhysicsCar::IterateForces() {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00138e08;
    const auto drive=m.word(mover+0x44);GuessClutchTorque(m,drive);
    const auto ai=m.signed_byte(m.word(m.word(mover+0x54)+0x34)+0x12);
    for(unsigned i=0;i<4;++i)port::driving::GuessGroundForce(m,m.word(mover+0x30+4*i),0,ai);
    const auto body=m.word(mover+0x18);const auto base=transform(m,transpose(m.matrix(body+0x20)),m.vector(body+0xf0));
    for(unsigned iteration=0;iteration<5;++iteration){
        auto sum=base;float moment=0;
        for(unsigned i=0;i<4;++i){const auto a=m.word(mover+0x30+4*i),s=m.word(mover+0x20+4*i);Vector value{};port::driving::GetHorizontalGroundForceInCarFrame(wheel(m,a),value);
            for(unsigned j=0;j<4;++j)sum[j]=sum[j]+value[j];
            const float y=m.scalar(s+0x80)*value[1],x=m.scalar(s+0x84)*value[0];moment=moment+y;moment=moment-x;
        }
        const float x=sum[0]/m.scalar(body+0x118),y=sum[1]/m.scalar(body+0x118);const float divisor=1.0f/m.scalar(body+0x148);moment=moment/divisor;
        for(unsigned i=0;i<4;++i){const auto a=m.word(mover+0x30+4*i),s=m.word(mover+0x20+4*i);if(m.byte(a+0x154)==0&&m.word(a+0x10c)==0){auto w=wheel(m,a);const float f1=moment*m.scalar(s+0x80),f2=moment*m.scalar(s+0x84);port::driving::ModifyGroundForce(w,{{x-f2,y+f1}});m.scalar(a+0x114,w.Field_0x114);}}
        ModifyClutchTorque(m,drive);
    }
    for(std::int32_t i=0;i<4;++i)port::driving::FinalizeGroundForce(m,m.word(mover+0x30+4*i),i);

}

// Debug 00138D90; E-0044-P01/P02.
void PhysicsCar::ResolveForces() {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00138e08;
    Memory::Scratch frame(m);const auto scratch=m.allocate(16);const auto body=m.word(mover+0x18);
    const auto table=m.word(body+8);if(m.word(table+0x34)!=0x0019b2a0)hp2::not_implemented("0x00138D90","unknown body pre-force virtual entry");
    m.word(0x003270b8,m.word(0x003270b8)+1u);Aerodynamics::ResolveForces(m,m.word(mover+0x48),scratch);
    const float request=m.word(0x003270b4)==0 ? 0 : m.scalar(scratch)*0.25f;
    for(unsigned i=0;i<4;++i)suspension_request(m,m.word(mover+0x20+4*i),request);
    IterateForces();
    for(unsigned i=0;i<4;++i){const auto a=m.word(mover+0x20+4*i);if(m.word(a+0x70)!=0){const auto w=m.word(a+0x90),b=m.word(a+0x94);const auto p=m.vector(w+0x130),q=m.vector(b+0x10);Vector delta;for(unsigned j=0;j<4;++j)delta[j]=p[j]-q[j];if(m.word(0x00327d90)==0)ApplyForce(m,b,m.vector(w+0x120),delta);}}
    JumpStabilizer();

}

// Debug 00139110; E-0044-P01/P02.
void PhysicsCar::UpdateCar(float /*dt*/) {
    auto& m=m_; [[maybe_unused]] const auto object=address_,s=address_,mover=address_;
    using namespace port::driving; using namespace port::driving::native;

    using namespace detail_group_00138e08;
    const auto body=m.word(mover+0x18),car=m.word(mover+0x50),config=m.word(car);
    m.vector(car+0x20,m.vector(body+0x10));m.matrix(car+0x50,m.matrix(body+0x20));
    const auto center=transform(m,m.matrix(car+0x50),m.vector(config+0x110));auto position=m.vector(car+0x20);for(unsigned i=0;i<4;++i)position[i]=position[i]-center[i];m.vector(car+0x10,position);
    m.vector(car+0x30,m.vector(body+0x90));m.scalar(car+0x384,len(m.vector(body+0x90)));m.word(car+0x400,0);m.vector(car+0x90,m.vector(body+0xa0));
    m.half(car+0x364,static_cast<std::uint16_t>(port::driving::bFixATan(m,cvt_w_s(m.scalar(car+0x50)*65536.0f),cvt_w_s(m.scalar(car+0x54)*65536.0f))));
    m.half(car+0x366,m.half(m.word(mover)+0x210));m.scalar(car+0x380,dot(m.vector(car+0x30),m.vector(car+0x50)));
    for(unsigned i=0;i<4;++i){const auto w=m.word(mover+0x30+4*i),s=m.word(mover+0x20+4*i),cfg=m.word(s+0xc);
        m.word(car+0x2b0+i*4,m.word(w+0x20));m.word(car+0x2c0+i*4,m.word(w+0x1c));
        const float limit=m.scalar(cfg+0x1c),value=m.scalar(s+0x14);m.scalar(car+0x300+i*4,value+(limit<=value?limit-value:0.0f));
        const auto now=m.byte(w+0x155);hp2::recovered::CarState(m,car).SetWheelTouchingGround(static_cast<std::int32_t>(i),static_cast<char>(now));
        hp2::recovered::CarState(m,car).SetWheelTerrain(static_cast<std::int32_t>(i),m.signed_word(w+0x150));
        m.word(car+0x354+i*4,m.word(w+0x168));m.word(car+0x334+i*8,m.word(w+0x100));m.word(car+0x338+i*8,m.word(w+0x104));m.word(car+0x2d0+i*4,m.word(w+0x158));m.word(car+0x2e0+i*4,now?m.word(w+0x15c):0);
    }
    const auto engine=m.word(mover+0x40),drive=m.word(mover+0x44);m.scalar(car+0x38c,m.scalar(engine+0x44)*m.scalar(engine+0x4c));m.word(car+0x388,m.word(drive+0x30));m.word(car+0x398,m.word(drive+0x38)==3);m.scalar(car+0x390,m.scalar(engine+0xc)*g(m,0x8450));
    const auto w0=m.word(mover+0x30),w1=m.word(mover+0x34),w2=m.word(mover+0x38),w3=m.word(mover+0x3c);
    float a=std::abs(m.scalar(w0+0x160)),b=std::abs(m.scalar(w1+0x160));const auto c=std::abs(m.scalar(w2+0x160)),d=std::abs(m.scalar(w3+0x160));if(a<d)a=d;if(b<c)b=c;a=a/g(m,0xa76c);b=b/g(m,0xa76c);if(1<a)a=1;if(1<b)b=1;m.scalar(car+0x374,a);m.scalar(car+0x378,b);
    const float sum=((m.scalar(w0+0x168)+m.scalar(w1+0x168))+m.scalar(w2+0x168))+m.scalar(w3+0x168);m.scalar(car+0x370,sum*0.25f);
    for(std::int32_t i=0;i<4;++i)port::driving::CalculateWheelMatrices(m,car,i);

}
// Debug001395D0 E-0044-P01, original single store.
void PhysicsCar::SetFrictionBoost(float boost){m_.scalar(address_+0x70,boost);}
// Debug00137CC0 E-0044-P01, double promotion retained in rear expression.
float PhysicsCar::GetBrakeSettings(std::int32_t pos){const float f=m_.scalar(m_.word(address_+0x4c)+0x30c);return pos<2?f+f:static_cast<float>((1.0-static_cast<double>(f))+(1.0-static_cast<double>(f)));}
// Debug00137C10 E-0044-P02. Memory binds original mutable globals.
float YawFrictionBoost(port::driving::Memory& m,float yaw,float ebrake,float speed,float control,float nfs,int mode,float& grade,float post){
 grade=std::fabs(grade);const float base=grade+1.0f;if(mode!=0)nfs=post*nfs;
 if(0.5f<ebrake&&std::fabs(yaw)<m.scalar(0x00324dd0))return base;
 const float ratio=(speed-m.scalar(0x00327068))/(m.scalar(0x0032706c)-m.scalar(0x00327068));
 const float range=m.scalar(0x00327064)-m.scalar(0x00327060);const float boost=m.scalar(0x00327060)+range*ratio;
 float result=std::fabs(yaw)*(control+nfs);result=result*boost;
 if(m.scalar(0x00327070)<result)result=m.scalar(0x00327070);
 return base+result;
}
}
namespace port::driving::native {
void Calibrate(Memory& m,std::uint32_t object){hp2::recovered::PhysicsCar(m,object).Calibrate();}
void InitializeStateFromCar(Memory& m,std::uint32_t object){hp2::recovered::PhysicsCar(m,object).InitializeStateFromCar();}
std::uint32_t CheckDampingSuitability(Memory& m,std::uint32_t object){return hp2::recovered::PhysicsCar(m,object).CheckDampingSuitability();}
void BottomOut(Memory& m,std::uint32_t object,InitServices& services){hp2::recovered::PhysicsCar(m,object).BottomOut(services);}
void PhysicsCar::ComputeAuxiliaries(Memory& m,std::uint32_t object,InitServices& services){hp2::recovered::PhysicsCar(m,object).ComputeAuxiliaries(services);}
std::uint32_t Sub_00137100(Memory& m,std::uint32_t object,std::uint32_t config,std::uint32_t controls,std::uint32_t car,InitServices& services){hp2::recovered::PhysicsCar(m,object).Initialize(config,controls,car,services);return object;}
void SetBoost(Memory& m,std::uint32_t s,float value) {hp2::recovered::PhysicsCar(m,s).SetBoost(value);}
float Move(Memory& m,std::uint32_t s,float dt,DrivingServices& hooks) {return hp2::recovered::PhysicsCar(m,s).Move(dt,hooks);}
void JumpStabilizer(Memory& m,std::uint32_t mover){hp2::recovered::PhysicsCar(m,mover).JumpStabilizer();}
void IterateForces(Memory& m,std::uint32_t mover){hp2::recovered::PhysicsCar(m,mover).IterateForces();}
void ResolveForces(Memory& m,std::uint32_t mover){hp2::recovered::PhysicsCar(m,mover).ResolveForces();}
void UpdateCar(Memory& m,std::uint32_t mover,float dt){hp2::recovered::PhysicsCar(m,mover).UpdateCar(dt);}
}
namespace port::driving {
float YawFrictionBoost(Memory& m,float a,float b,float c,float d,float e,float post,int mode,float& grade){return hp2::recovered::YawFrictionBoost(m,a,b,c,d,e,mode,grade,post);}
}
namespace port::driving {
// TASK-0044: obsolete typed composite retired; canonical PhysicsCar::IterateForces owns the algorithm.

}



#include "game_core/src/bMath.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/table.hpp"

// Debug 0x00139510; E-0044-M02; source ownership E-0042-S001.
namespace hp2::recovered {
void Sub_00139510(Memory& m) {
    m.word(0x002e7e00,0);m.word(0x002e7e04,0);m.word(0x002e7e08,m.word(0x00324e34));m.word(0x002e7e0c,0);
    m.scalar(0x002e7e10,2);m.scalar(0x002e7e14,20);m.word(0x00327080,m.word(0x00324e38));
    hp2::recovered::Graph(m,0x003270c0).Sub_0018D9E8(0x002e7e00,3);
}
}

// Preserved unrelated historical Retail destructor gap; no Debug supersession asserted.
#include "game_core/indep/inc/ComponentPhysics.hpp"
namespace hp2::recovered {
// ELF 0x00139678; E-0042-U030; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00139678.md.
void PhysicsVehicle::Sub_00139678() {
    hp2::not_implemented("0x00139678", "PhysicsVehicle::Sub_00139678 (signature and full body unresolved)");
}
} // namespace hp2::recovered

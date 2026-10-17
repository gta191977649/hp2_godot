#include "Sub_00137100_runtime.hpp"
#include "Sub_0018F808.hpp"
#include "Sub_001A4930.hpp"
#include "Sub_0019D4D0.hpp"
#include "port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace port::driving::native {
namespace {
constexpr std::uint32_t gp=0x003358f0;
float global(Memory& m,std::uint16_t offset){return m.scalar(gp+static_cast<std::int16_t>(offset));}
std::uint32_t global_address(std::uint16_t offset){return gp+static_cast<std::int16_t>(offset);}
float length(const std::array<float,4>& v){return std::sqrt((v[0]*v[0]+v[1]*v[1])+v[2]*v[2]);}
std::array<float,4> cross(const std::array<float,4>& a,const std::array<float,4>& b){return {{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0],a[3]}};}
std::int32_t signed_bits(std::uint32_t bits){std::int32_t result;std::memcpy(&result,&bits,4);return result;}
void Sub_0019CF10(Memory& m,std::uint32_t body,const std::array<float,4>& input,const std::array<float,4>& point){
    // ELF0019CF10, E-0038-I28. Cross uses the capped input (unlike0019CD90).
    auto value=input;const auto magnitude=length(value);
    if(1000.0f<magnitude)for(auto& lane:value)lane=lane*(1000.0f/magnitude);
    auto linear=m.vector(body+0x60);for(unsigned i=0;i<4;++i)linear[i]=linear[i]+value[i];m.vector(body+0x60,linear);
    auto angular=cross(point,value);const auto size=length(angular);
    if(10.0f<size)for(auto& lane:angular)lane=lane*(10.0f/size);
    const auto old=m.vector(body+0x70);for(unsigned i=0;i<4;++i)angular[i]=old[i]+angular[i];m.vector(body+0x70,angular);
}
void reset_dispatch(Memory& m,std::uint32_t code,std::uint32_t object,InitServices& services){
    switch(code){
    case 0x0019d848:Sub_0019D848(m,object);return;
    case 0x001a4358:Sub_001A4358(m,object);return;
    case 0x001a5fc0:
        if(!services.contact)hp2::not_implemented("0x001A5FC0","Contact provider is absent");
        Sub_001A5FC0(m,object,*services.contact);return;
    case 0x0018a988:port::driving::Sub_0018A988(m,object);return;
    case 0x0018b4a0:port::driving::Sub_0018B4A0(m,object);return;
    default:hp2::not_implemented("0x0019C808","Unknown component virtual target at slot +0x44");
    }
}
void refresh(Memory& m,std::uint32_t object){
    const auto body=m.word(object+0x94),wheel=m.word(object+0x90);
    init::Sub_001A5F88(m.view(object,0xa0),m.scalar(object+0x78),m.vector(m.word(wheel+0xc)),m.vector(body+0x160));
}
}
void Sub_0019C238(Memory& m,std::uint32_t head,std::uint32_t object){
    const auto last=m.word(head+4),table=m.word(object+8);
    m.word(last,object);m.word(head+4,object);m.word(object+4,last);m.word(object,head);
    const auto adjusted=object+static_cast<std::int32_t>(m.signed_half(table+0x10));
    (void)adjusted; // These exact constant-return implementations do not read this.
    std::uint32_t count;
    switch(m.word(table+0x14)){
    case 0x0019dcc8:count=32;break;
    case 0x001a5c28:case 0x001a6660:count=2;break;
    case 0x0018c368:case 0x0018c1c8:count=1;break;
    default:hp2::not_implemented("0x0019C238","Unknown component virtual count target at slot +0x14");
    }
    m.word(head+8,m.word(head+8)+count);
}
void Sub_0019C808(Memory& m,std::uint32_t head,InitServices& services){
    if(!services.Sub_00250F38)hp2::not_implemented("0x00250F38","Original performance-counter service is absent");
    const auto start=services.Sub_00250F38();auto object=m.word(head);
    while(object!=head){
        const auto table=m.word(object+8);
        reset_dispatch(m,m.word(table+0x44),object+static_cast<std::int32_t>(m.signed_half(table+0x40)),services);
        object=m.word(object);
    }
    const auto end=services.Sub_00250F38();
    if(!services.Sub_00250F48)hp2::not_implemented("0x00250F48","Original performance-counter delta service is absent");
    m.word(global_address(0xa780),services.Sub_00250F48(start,end));
}
float Sub_001ADFB8(Memory& m,std::uint32_t object,float f12){
    const auto config=m.word(object);return std::max(0.0f,f12)*(m.scalar(config+4)+m.scalar(config+8));
}
void Sub_001375F0(Memory& m,std::uint32_t object){
    const auto config=m.word(object+0x4c),body=m.word(object+0x18);
    m.copy(body+0x160,config+0x110,16);
    init::Sub_0019CC58(m.view(body,0x180),m.view(m.word(body+0xc),0x50));
    float offset=m.scalar(config+0x110);if(!m.word(global_address(0xa704)))offset=0;
    const float numerator=m.scalar(config+0x120)-offset;
    const float split=(numerator/(numerator-(m.scalar(config+0x180)-offset)))*0.5f;
    float total=m.scalar(body+0x118)*global(m,0x83d0);
    const float additional=Sub_001ADFB8(m,m.word(object+0x48),m.scalar(m.word(object+0x54)+0x3c0));
    total=total+m.scalar(config+0x2fc)*additional;
    const float first=total*(0.5f-split),second=total*split;
    for(unsigned i=0;i<4;++i){const auto controller=m.word(object+0x20+4*i);m.scalar(controller+0x78,i<2?first:second);refresh(m,controller);}
    for(unsigned i=0;i<4;++i)m.word(m.word(object+0x30+4*i)+0x24,m.word(global_address(0xb3b0)));
}
void Sub_00138CF8(Memory& m,std::uint32_t object){
    const auto car=m.word(object+0x50);
    Sub_0019CC88(m,m.word(object+0x18),car+0x20,car+0x50,car+0x30,car+0x90);
    for(unsigned i=0;i<4;++i){
        init::Sub_001A5FB0(m.view(m.word(object+0x20+4*i),0xa0));
        init::Sub_001A4158(m.view(m.word(object+0x30+4*i),0x184),m.scalar(car+0x2c0+4*i),m.scalar(car+0x2b0+4*i));
    }
    float value=global(m,0xb834);if(value==100.0f)value=m.scalar(car+0x390);
    m.scalar(m.word(object+0x40)+0xc,value*global(m,0x844c));
    const auto drive=m.word(object+0x44),word=m.word(car+0x388);
    m.word(drive+0x2c,word);m.word(drive+0x38,3);m.scalar(drive+0xc,1);m.word(drive+0x4c,0);m.word(drive+0x30,word);
}
void Sub_001A4358(Memory& m,std::uint32_t object){
    m.matrix(object+0x30,init::Sub_0025AC68());
    const auto angle=m.scalar(object+0x1c);
    const float cosine=port::driving::Sub_0025A5E0(m,angle+m.scalar(0x0032e628));
    const float sine=port::driving::Sub_0025A5E0(m,angle);
    m.scalar(object+0x58,cosine);m.scalar(object+0x38,-sine);m.scalar(object+0x30,cosine);m.scalar(object+0x50,sine);
    for(auto offset:{0x114,0x118,0x108,0x160})m.word(object+offset,0);
    const auto wrap=m.scalar(0x0032e62c);
    if(wrap<angle){float next=angle-wrap;if(m.scalar(0x0032e630)<next)next=0;m.scalar(object+0x1c,next);}
    else if(angle<m.scalar(0x0032e634)){float next=angle+wrap;if(next<m.scalar(0x0032e638))next=0;m.scalar(object+0x1c,next);}
    const auto mode=m.word(object+0x110);float speed=m.scalar(object+0x20);
    if((mode==0&&speed<0)||(mode==2&&0<speed)){speed=0;m.word(object+0x20,0);}
    m.word(object+0x110,m.scalar(0x0032e63c)<speed?0:(m.scalar(0x0032e640)<=speed?1:2));
}
void Sub_001A5FC0(Memory& m,std::uint32_t object,ContactProvider& contact){
    Memory::Scratch scratch(m);
    const auto body=m.word(object+0x94),wheel=m.word(object+0x90),config=m.word(object+0xc);
    auto world=transform(m,m.matrix(body+0x20),m.vector(object+0x80));
    const auto position=m.vector(body+0x10);for(unsigned i=0;i<4;++i)world[i]=world[i]+position[i];
    const auto temporary=m.allocate(16);m.vector(temporary,world);
    const float hit=port::driving::Sub_001A4580(m,wheel,temporary,body+0x20,contact);
    const float value=hit+m.scalar(m.word(wheel+0xc)+0xc),old=m.scalar(object+0x14);
    float extra=0;
    if(m.scalar(config+0x14)<value){extra=value-m.scalar(config+0x14);m.scalar(object+0x14,m.scalar(config+0x14));m.word(object+0x70,1);if(global(m,0xb414)<extra)extra=0;}
    else if(value<m.scalar(config+0x18)){m.scalar(object+0x14,m.scalar(config+0x18));m.word(object+0x70,0);}
    else {m.scalar(object+0x14,value);m.word(object+0x70,1);}
    const float dt=global(m,0xb408);
    if(global(m,0x8dcc)<dt)m.scalar(object+0x18,(m.scalar(object+0x14)-old)/dt);
    m.copy(object+0x20,object+0x80,16);m.scalar(object+0x28,m.scalar(object+0x28)+m.scalar(object+0x14));
    auto matrix=init::Sub_0025AC68();const float steer=m.scalar(wheel+0x16c);
    if(steer!=0){auto rotation=init::Sub_0025AC68();const float a=(global(m,0xb41c)*steer)*port::driving::Sub_0025A5E0(m,m.scalar(wheel+0x1c));
        const float c=port::driving::Sub_0025A5E0(m,a+global(m,0x8dd0)),s=port::driving::Sub_0025A5E0(m,a);
        rotation[0]=c;rotation[1]=s;rotation[4]=-s;rotation[5]=c;matrix=Sub_0025AE18(matrix,rotation);}
    const auto local=m.vector(object+0x20);for(unsigned i=0;i<3;++i)matrix[12+i]=local[i];matrix[15]=1;
    m.matrix(object+0x30,matrix);
    if(m.word(object+0x98)){const auto other=m.word(object+0x98);matrix=Sub_0025AE18(matrix,m.matrix(m.word(other+4)+m.word(object+0x9c)*0x40));}
    m.matrix(wheel+0xb0,matrix);
    const auto relative=transform(m,m.matrix(body+0x20),m.vector(object+0x20));
    const auto velocity=Sub_001A63D0(m.vector(body+0x90),m.vector(body+0xa0),relative);m.vector(wheel+0xf0,velocity);
    m.matrix(wheel+0x70,Sub_0025AE18(m.matrix(body+0xb0),matrix));
    if(m.word(object+0x70)){
        const float displacement=m.scalar(object+0x14),rate=m.scalar(object+0x18);
        float response=std::max(displacement,0.0f)*m.scalar(config);response=response+1;response=m.scalar(config+4)*response;
        response=displacement*response;response=m.scalar(object+0x78)+response;
        response=response+rate*m.scalar(config+8+(rate<=0?4:0));m.scalar(object+0x1c,response);
        response=response+m.scalar(config+0x10)*(displacement-m.scalar(m.word(object+0x10)+0x14));
        response=response+extra*global(m,0xb410);m.scalar(object+0x1c,response);
        if(!(response<0))return;
    }
    m.word(object+0x1c,0);
}
std::array<float,4> Sub_001A63D0(const std::array<float,4>& linear,const std::array<float,4>& angular,const std::array<float,4>& point){
    auto result=cross(angular,point);for(unsigned i=0;i<4;++i)result[i]=linear[i]+result[i];return result;
}
float Sub_001A6408(Memory& m,std::uint32_t object){
    const auto body=m.word(object+0x94),wheel=m.word(object+0x90);
    const auto point=transform(m,m.matrix(body+0x20),m.vector(object+0x20));
    const auto velocity=Sub_001A63D0(m.vector(body+0x90),m.vector(body+0xa0),point),normal=m.vector(wheel+0x140);
    const float dot=(velocity[0]*normal[0]+velocity[1]*normal[1])+velocity[2]*normal[2];float result=0;
    if(0<=-dot){result=(-dot*global(m,0xb40c))*m.scalar(body+0x118);auto input=normal;for(auto& lane:input)lane=lane*result;
        if(m.word(global_address(0xb420))==0)Sub_0019CF10(m,body,input,point);}
    return result;
}
bool Sub_00133130(Memory& m,std::uint32_t object){
    const auto start=m.word(object+0x860);
    return start&&start!=0x7fffffffu&&float(signed_bits(m.word(global_address(0xb388))-start))*global(m,0x8344)<m.scalar(object+0x864);
}
void Sub_00133070(Memory& m,std::uint32_t object,InitServices& services){
    bool set=true;
    if(m.word(0x0031ab0c)&&m.signed_word(0x0031aaf4)>=1&&m.word(m.word(object+0x14)+8)==2){
        if(!services.Sub_00227438)hp2::not_implemented("0x00227438","External predicate service is absent");
        set=services.Sub_00227438(m.word(global_address(0xc2fc)),object)==0;
    }
    if(set)m.word(object+0x858,1);
}
std::uint32_t Sub_001377D8(Memory& m,std::uint32_t object){
    const auto car=m.word(object+0x54),body=m.word(object+0x18);const auto mode=m.signed_word(car+0x3bc);
    if(!(m.scalar(m.word(object)+0x214)<=global(m,0x83d4))&&mode>=1)return 0;
    const auto speed=length(m.vector(body+0x90)),angular=length(m.vector(body+0xa0));
    if(!(speed<=global(m,0x83d8))||!(angular<=global(m,0x83d8))||!(std::abs(m.scalar(body+0x28))<=0.25f)||!(std::abs(m.scalar(body+0x38))<=0.25f))return 0;
    if(Sub_00133130(m,car)||(mode==0&&m.byte(m.word(car+0x34)+0x12)==0))return 0;
    return speed<=0.25f&&angular<=0.25f?2:1;
}
void Sub_00137938(Memory& m,std::uint32_t object,InitServices& services){
    bool changed=false;const auto body=m.word(object+0x18),car=m.word(object+0x54);
    for(unsigned i=0;i<4;++i){const auto wheel=m.word(object+0x20+4*i);
        if(m.scalar(wheel+0x14)==m.scalar(m.word(wheel+0xc)+0x14)){changed=true;const float result=Sub_001A6408(m,wheel);const auto target=m.word(object+0x50)+0x2f0+4*i;m.scalar(target,m.scalar(target)+result);}}
    const auto mode=Sub_001377D8(m,object);
    if(mode==1){const auto factor=global(m,0x83e0);for(auto offset:{0x60,0x70}){auto v=m.vector(body+offset);for(auto& lane:v)lane=lane*factor;m.vector(body+offset,v);}
        for(unsigned i=0;i<4;++i){const auto wheel=m.word(object+0x30+4*i);m.scalar(wheel+0x20,m.scalar(wheel+0x20)*factor);}
        changed=true;if(global(m,0x83e4)<length(m.vector(body+0x90)))m.word(object+0x6c,0);m.byte(m.word(car+0x34)+0x13,1);
    }else if(mode==0){if(global(m,0x83dc)<length(m.vector(body+0x90)))m.word(object+0x6c,0);m.byte(m.word(car+0x34)+0x13,1);
    }else if(mode==2){
        if(!services.Field_00137938_stack_w)hp2::not_implemented("0x00137938","Explicit original temporary W projection is absent");
        for(auto offset:{0x60,0x70}){for(unsigned i=0;i<3;++i)m.word(body+offset+4*i,0);m.word(body+offset+12,*services.Field_00137938_stack_w);}
        if(m.signed_word(car+0x3bc)>0)for(unsigned i=0;i<4;++i)m.word(m.word(object+0x30+4*i)+0x20,0);
        changed=true;
        if(m.scalar(body+0x48)<global(m,0x83e8)){
            const auto start=m.word(object+0x6c);if(!start||start==0x7fffffff)m.word(object+0x6c,m.word(global_address(0xb388)));
            else if(1.0f<float(signed_bits(m.word(global_address(0xb388))-start))*global(m,0x83ec))Sub_00133070(m,car,services);
        }else m.word(object+0x6c,0);
        m.byte(m.word(car+0x34)+0x13,0);
    }
    if(m.word(global_address(0xa708))&&m.word(car+0x20)==1){
        if(!services.Field_00137938_stack_w)hp2::not_implemented("0x00137938","Explicit original temporary W projection is absent");
        std::array<float,4> point{{0,0,0,0}};std::memcpy(&point[3],&*services.Field_00137938_stack_w,4);
        auto input=point;input[2]=25;
        m.word(global_address(0xa708),0);Sub_0019CF10(m,body,input,point);changed=true;
    }
    if(changed){m.word(global_address(0xb408),0);Sub_0019C808(m,object+8,services);}
}
void Sub_00137788(Memory& m,std::uint32_t object,InitServices& services){
    Sub_0019C808(m,object+8,services);Sub_00137938(m,object,services);
    const auto table=m.word(object+0x14),target=m.word(table+0x1c),adjusted=object+8+static_cast<std::int32_t>(m.signed_half(table+0x18));
    if(target!=0x00138e08)hp2::not_implemented("0x00137788","Unknown final virtual target");
    if(!services.ResolveForces)hp2::not_implemented("0x00138E08","Final constructor synchronization service is absent");
    services.ResolveForces(m,adjusted);
}
std::uint32_t Sub_00137100(Memory& m,std::uint32_t object,std::uint32_t config,std::uint32_t controls,std::uint32_t car,InitServices& services){
    m.word(object+4,0x002ae7a0);init::Sub_0019C170(m.view(object+8,0x10),object+8);m.word(object+0x14,0x002aeae8);
    m.word(object,controls);m.word(object+0x58,0);m.word(object+0x5c,0);m.word(object+0x6c,0);m.word(object+0x4c,config);m.word(object+0x54,car);
    m.word(object+0x14,0x002aeaa8);m.word(object+4,0x002aeac8);m.word(object+0x50,car+0x40);m.scalar(object+0x70,1);
    // Port-owned capacities are bounds for recovered offsets, not recovered pool sizes.
    const auto body=m.allocate(0x180);init::Sub_0019C9B0(m.view(body,0x180),config+0x1e0,m.view(config+0x1e0,0x50));m.word(object+0x18,body);Sub_0019C238(m,object+8,body);
    m.word(body+0x174,m.word(config+0x320));m.word(body+0x114,m.byte(config+0xe4)!=0);
    const auto steering=m.allocate(0x90);m.word(steering,config);m.word(steering+8,0x002b5d18);m.word(steering+4,steering+0x10);m.word(object+0x1c,steering);
    for(unsigned i=0;i<4;++i){const auto axle=(i/2)*0x20;const auto wheel=m.allocate(0x184);
        init::Sub_001A3F10(m.view(wheel,0x184),config+0x120+i*0x20,config+0x1a0+axle,config+0x300,car+0x40);m.word(object+0x30+4*i,wheel);
        const auto controller=m.allocate(0xb0);init::Sub_001A5EA0(m.view(controller,0xb0),i,body,wheel,i<2?steering:0,config+0x230+axle);m.word(object+0x20+4*i,controller);
        Sub_0019C238(m,object+8,wheel);Sub_0019C238(m,object+8,controller);}
    for(unsigned i=0;i<4;++i)m.word(m.word(object+0x20+4*i)+0x10,m.word(object+0x20+4*(i^1)));
    const auto engine=m.allocate(0x60);port::driving::Sub_0018A7E8(m,engine,config+0x2b0,car+0x40);m.word(object+0x40,engine);Sub_0019C238(m,object+8,engine);m.word(engine+0x40,m.byte(config+0xe4)!=0);
    const auto drive=m.allocate(0xa0);port::driving::Sub_0018AE70(m,drive,config+0x270,engine,object+0x30,car+0x40);m.word(object+0x44,drive);Sub_0019C238(m,object+8,drive);
    const auto helper=m.allocate(0x18);m.scalar(helper+8,1);m.scalar(helper+0xc,1);m.word(helper+0x10,0);m.word(helper+0x14,0);m.word(object+0x48,helper);m.word(helper+4,body);m.word(helper,config+0x2f0);
    m.word(global_address(0xb408),0);Sub_001375F0(m,object);Sub_00138CF8(m,object);Sub_00137788(m,object,services);
    m.scalar(object+0x74,1);m.word(object+0x64,1);m.word(object+0x68,2);m.word(object+0x60,1);return object;
}
}

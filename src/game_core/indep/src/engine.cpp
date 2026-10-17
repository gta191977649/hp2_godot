// TASK-0044 E-0044-E01. Debug NFSHP2.ELF behavior authority:
// SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Raw read-only decompilation/listing queries and hashes: local/task0044/engine/manifest.json.
#include "game_core/indep/src/engine.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/port/driving/projection_support.hpp"
namespace hp2::recovered {
using namespace port::driving;
std::int32_t DriveTrain::SetSportGear(DrivetrainProjection& s,std::int32_t value) {
    using namespace detail_group_0018bce0;
    if(value>1 && value<=add_one(s.config_Field_0x18)) {
        if(s.Field_0x48==1) s.Field_0x44=0;
        s.Field_0x48=value;
    }
    return s.Field_0x48;
}
std::int32_t DriveTrain::SetDesiredGear(DrivetrainProjection& s,std::int32_t request,const DrivetrainGlobals& g) {
    using namespace detail_group_0018bce0;
    const auto current=s.Field_0x30;
    if(request==-1) {
        s.Field_0x48=1;
        if(current<2) { s.Field_0x2c=2; return -1; }
        if(s.Field_0x38!=3) return -1;
        const float f3=s.engine_Field_0xc*g.Field_0032e388;
        float f2;
        if(current<3) f2=s.engine_Field_0x44<g.Field_0032e38c ? s.engine_config_Field_0x0+100.0f : 0.0f;
        else f2=s.Field_0x74.at(index(current));
        bool increase=s.Field_0x50.at(index(current))<f3 &&
            current<add_one(s.config_Field_0x18);
        for(const auto& w:s.wheels) increase=increase && w.Field_0x160<4.0f && w.Field_0x155!=0;
        if(increase) s.Field_0x2c=current+1;
        else if(f3<f2) s.Field_0x2c=current-1;
        return -1;
    }
    if(request!=-2) { s.Field_0x2c=request; return request; }
    s.Field_0x44=s.Field_0x44+g.Field_00330c88;
    if(s.Field_0x38!=3) return -2;
    const float f5=s.engine_config_Field_0x8, f4=s.engine_config_Field_0x0;
    const float f3=s.engine_Field_0xc*g.Field_0032e390;
    float f0;
    if(current<3) f0=s.engine_Field_0x44<g.Field_0032e394 ? f4+100.0f : 0.0f;
    else {
        f0=f5*g.Field_003308c4;
        f0=f0*s.config_Field_0x20.at(index(current));
        f0=f0/s.config_Field_0x20.at(index(current-1));
    }
    if(s.Field_0x2c==s.Field_0x48) {
        if(10.0f<s.Field_0x44) return -1;
        if(s.Field_0x34<current && f5-300.0f<f3) return -1;
    } else {
        s.Field_0x34=current;
        if(s.Field_0x48<s.Field_0x2c) {
            if(f0<f3) {
                s.Field_0x2c=current; s.Field_0x44=0; s.Field_0x48=current;
                return -2;
            }
            s.Field_0x44=0; s.Field_0x2c=current-1; return -2;
        }
        if(s.Field_0x2c<s.Field_0x48) {
            s.Field_0x44=0; s.Field_0x2c=current+1; return -2;
        }
    }
    if(f3<f4+100.0f && current>2) { s.Field_0x2c=current-1; return -1; }
    return -2;
}
float DriveTrain::GetDifferentialAngularAcceleration(const DrivetrainProjection& s,const DrivetrainGlobals& g) {
    using namespace detail_group_0018bce0;
    const auto lane=[](const DrivetrainWheelProjection& w) {
        float f=w.Field_0x114*w.config_Field_0xc;
        f=w.Field_0x108-f; return f/w.Field_0x24;
    };
    float f=0;
    if(g.Field_0032e358<=s.config_Field_0x10) {
        f=lane(s.wheels[2])+lane(s.wheels[3]); f=f*0.5f; f=f*s.config_Field_0xc;
    }
    if(s.config_Field_0x10<=g.Field_0032e35c) {
        float a=lane(s.wheels[0])+lane(s.wheels[1]); a=a*0.5f; a=a*s.config_Field_0x8;
        f=f+a;
        if(g.Field_0032e358<=s.config_Field_0x10) f=f*0.5f;
    }
    return f*s.config_Field_0x20.at(index(s.Field_0x30));
}
void DriveTrain::ApplyClutchTorque(float value,DrivetrainProjection& s,const DrivetrainGlobals& g) {
    using namespace detail_group_0018bce0;
    if(s.Field_0x30==1) return;
    s.engine_Field_0x48=s.engine_Field_0x48-value;
    value=value*s.config_Field_0x20.at(index(s.Field_0x30));
    float rear=value*s.config_Field_0x10; rear=rear*s.config_Field_0xc;
    float front=value*(1.0f-s.config_Field_0x10); front=front*s.config_Field_0x8;
    const auto split=[&](const DrivetrainWheelProjection& a,const DrivetrainWheelProjection& b,bool rear_axle) {
        float bound=0.5f;
        if(a.Field_0x155!=0 && b.Field_0x155!=0) bound=g.Field_003308b8;
        if(s.car_Field_0x3a4==1) return 1.0f-bound;
        if(s.car_Field_0x3a4==2) return bound;
        const float sum=a.Field_0x20+b.Field_0x20;
        const float numerator=rear_axle ? a.Field_0x20 : b.Field_0x20;
        // Original DIV.S0018993C/00189A48 can see zero wheel values at rest.
        // Project only that EE exceptional operation; no epsilon/balanced substitute.
        float ratio=sum==0.0f ? ee_div_s_zero(numerator,sum,rear_axle?"0x00189A48":"0x0018993C") : numerator/sum;
        if(ratio<bound) ratio=1.0f-g.Field_003308b8*0.5f;
        else if(1.0f-bound<ratio) ratio=g.Field_003308b8*0.5f;
        return ratio;
    };
    const float a=split(s.wheels[0],s.wheels[1],false);
    if(s.wheels[0].Field_0x10c==0) s.wheels[0].Field_0x108=s.wheels[0].Field_0x108+front*(1.0f-a);
    if(s.wheels[1].Field_0x10c==0) s.wheels[1].Field_0x108=s.wheels[1].Field_0x108+front*a;
    const float b=split(s.wheels[2],s.wheels[3],true);
    if(s.wheels[2].Field_0x10c==0) s.wheels[2].Field_0x108=s.wheels[2].Field_0x108+rear*b;
    if(s.wheels[3].Field_0x10c==0) s.wheels[3].Field_0x108=s.wheels[3].Field_0x108+rear*(1.0f-b);
}
void DriveTrain::GuessClutchTorque(DrivetrainProjection& s,const DrivetrainGlobals& g,const EngineEvaluation& engine) {
    using namespace detail_group_0018bce0;
    if(s.Field_0x38==3) { ApplyClutchTorque(evaluate(engine),s,g); return; }
    if(s.Field_0x38!=0 && s.Field_0x38!=2) return;
    float f4=s.engine_Field_0xc-s.Field_0x40;
    if(g.Field_003308bc<f4) f4=g.Field_003308bc;
    else if(f4< -g.Field_003308bc) f4= -g.Field_003308bc;
    const float f0=f4*s.config_Field_0x14;
    ApplyClutchTorque(f0*s.Field_0xc,s,g);
}
void DriveTrain::ModifyClutchTorque(DrivetrainProjection& s,const DrivetrainGlobals& g,const EngineEvaluation& engine) {
    using namespace detail_group_0018bce0;
    if(s.Field_0x38!=3) return;
    float f20=evaluate(engine)+s.engine_Field_0x48;
    f20=f20/s.engine_config_Field_0xc;
    const float f12=f20-GetDifferentialAngularAcceleration(s,g);
    float f5=1.0f/s.engine_config_Field_0xc;
    const float f6=s.config_Field_0x10;
    if(f6<=g.Field_0032e374) {
        float f2=s.config_Field_0x8*s.config_Field_0x20.at(index(s.Field_0x30));
        float f1=f2*(1.0f/s.wheels[0].Field_0x24);
        float f0=(1.0f-f6)*0.5f; f0=f0*f1; f0=f0*f2; f5=f5+f0;
    }
    if(g.Field_0032e378<=f6) {
        float f2=s.config_Field_0xc*s.config_Field_0x20.at(index(s.Field_0x30));
        float f1=f2*(1.0f/s.wheels[2].Field_0x24);
        float f0=f6*0.5f; f0=f0*f1; f0=f0*f2; f5=f5+f0;
    }
    ApplyClutchTorque(f12/f5,s,g);
}
void Engine::DecrementNitro(EngineTimerProjection& s,std::int32_t ticks,float scale) {
    using namespace detail_group_0018bce0;
    if(s.Field_0x50!=0 && s.Field_0x54+4.0f < static_cast<float>(ticks)*scale) s.Field_0x50=0;
}
void CalculateShiftPoints(Memory& m,std::uint32_t config,std::uint32_t engineconfig,
                  std::uint32_t out50,std::uint32_t out74,float f26) {
    using namespace detail_group_0018f808;
    const float lower=m.scalar(engineconfig),upper=m.scalar(engineconfig+8);
    const float step=8.0f/(upper-lower);
    const auto evaluate=[&](float value) {
        return interpolate(value,lower,upper,step,9,[&](std::int32_t i) {
            if(i<0 || i>=9) throw std::out_of_range("Engine table generation index outside9 samples");
            return m.scalar(engineconfig+0x10+static_cast<std::uint32_t>(i)*4);
        });
    };
    const auto limit=signed_bits(m.word(config+0x18)+1);
    std::int32_t current=2;
    // Memory bounds reject corrupt indexes; no fabricated default or changed cap.
    while(current<limit) {
        const auto off=static_cast<std::uint32_t>(current)*4;
        const float f24=m.scalar(config+0x20+off), f23=m.scalar(config+0x24+off);
        float f0;
        if(m.word(0x32792c)==0) {
            float f1=m.scalar(engineconfig+4)-lower;
            f0=m.scalar(config)*f1;f0=f0+lower;
        } else {
            const float f22=upper-200.0f;
            float f21=(lower+upper)*0.5f;
            bool pending=true;
            while(f21<f22 && pending) {
                const float f20=evaluate(f21);
                float value=f21*f23;value=value/f24;
                f0=evaluate(value);f0=f0*f23;f0=f0/f24;
                if(f20<f0) { m.scalar(out50+off,f21);pending=false; }
                const float next=f21+50.0f;
                if(next==f21) hp2::not_implemented("0x00188DE8","non-progressing exceptional table scan");
                f21=next;
            }
            if(pending) m.scalar(out50+off,upper-200.0f);
            f0=m.scalar(out50+off)*f26;
        }
        m.scalar(out50+off,f0);
        f0=m.scalar(0x327928)*m.scalar(out50+off);f0=f0*f23;f0=f0/f24;
        m.scalar(out74+off+4,f0);
        ++current;
    }
    m.scalar(out50+static_cast<std::uint32_t>(current)*4,upper);
}
void Engine::ComputeAuxiliaries() {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    const auto config=m.word(s+0x10);const float scale=m.scalar(0x325420);
    float value=m.scalar(s+0xc);float bound=m.scalar(config)*scale;
    if(value<bound) { m.scalar(s+0xc,bound);value=bound; }
    bound=m.scalar(config+8)*scale;
    if(bound<value) m.scalar(s+0xc,bound);
    m.word(s+0x48,0);
}
float DriveTrain::GetDifferentialAngularVelocity() {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    const auto config=m.word(s+0x10);const float f4=m.scalar(config+0x10);
    float f3=0;
    if(m.scalar(0x325434)<=f4) {
        const auto a=m.word(s+0x20),b=m.word(s+0x24);
        float f0=m.scalar(a+0x20)+m.scalar(b+0x20);f0=f0*0.5f;f3=f0*m.scalar(config+0xc);
    }
    if(f4<=m.scalar(0x325438)) {
        const auto a=m.word(s+0x18),b=m.word(s+0x1c);
        float f0=m.scalar(a+0x20)+m.scalar(b+0x20);f0=f0*0.5f;f0=f0*m.scalar(config+8);f3=f3+f0;
        if(m.scalar(0x325434)<=f4) f3=f3*0.5f;
    }
    return f3*ratio(m,s);
}
float DriveTrain::GetDriveWheelSlippage() {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    const float f4=m.scalar(m.word(s+0x10)+0x10);float f3=0;
    if(m.scalar(0x32543c)<=f4) {
        f3=m.scalar(m.word(s+0x20)+0x164)+m.scalar(m.word(s+0x24)+0x164);f3=f3*0.5f;
    }
    if(f4<=m.scalar(0x325440)) {
        float f0=m.scalar(m.word(s+0x18)+0x164)+m.scalar(m.word(s+0x1c)+0x164);
        f0=f0*0.5f;f0=f3+f0;f3=f0*0.5f;
    }
    return f3;
}
void DriveTrain::SetDifferentialAngularVelocity(float value) {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    const float f23=value-GetDifferentialAngularVelocity();
    const float f22=m.scalar(m.word(s+0x28)+0x384)*m.scalar(0x325444);
    auto config=m.word(s+0x10);
    const auto axle=[&](std::uint32_t off,float config_lane) {
        const auto a=m.word(s+off),b=m.word(s+off+4);
        bool average=f22<40.0f;
        if(m.signed_byte(a+0x155)==0 && m.signed_byte(b+0x155)==0) average=true;
        float a20=m.scalar(a+0x20),b20=m.scalar(b+0x20);
        const float denominator=ratio(m,s)*config_lane;
        const float f21=f23/denominator;
        if(average) {b20=(a20+b20)*0.5f;a20=b20;}
        m.scalar(a+0x20,a20+f21);m.scalar(b+0x20,b20+f21);
    };
    if(m.scalar(0x325448)<=m.scalar(config+0x10)) {
        axle(0x20,m.scalar(config+0xc));config=m.word(s+0x10);
    }
    if(m.scalar(config+0x10)<=m.scalar(0x32544c)) axle(0x18,m.scalar(config+8));
}
std::uint32_t DriveTrain::Idling() {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    const auto engine=m.word(s+0x14);
    if(!(m.scalar(engine+0x44)<m.scalar(0x325470))) return 0;
    float f0=m.scalar(m.word(engine+0x10))*m.scalar(0x325474);f0=f0+m.scalar(0x325478);
    return m.scalar(s+0x40)<=f0 ? 1:0;
}
std::uint32_t DriveTrain::ClutchMatched() {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    float f0=m.scalar(m.word(s+0x14)+0xc)-m.scalar(s+0x40);f0=std::fabs(f0);
    return f0<=m.scalar(0x327938) ? 1:0;
}
void DriveTrain::ComputeAuxiliaries() {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    m.scalar(s+0x40,GetDifferentialAngularVelocity());
    const auto state=m.signed_word(s+0x38);const auto engine=m.word(s+0x14);
    if(state==1) {
        const auto pending=m.word(s+0x3c);m.word(s+0x30,m.word(s+0x2c));m.word(engine+0x4c,0);
        if(pending!=0 || m.scalar(0x325464)<m.scalar(engine+0x44)) {
            m.word(s+0x3c,0);m.word(s+0x38,2);
        }
    } else if(state==0) {
        m.word(engine+0x4c,0);const float f1=m.scalar(engine+0x4c);
        if(m.scalar(s+0xc)<=f1) {m.word(s+0x38,1);m.scalar(s+0xc,f1);}
    } else if(state==2) {
        m.word(engine+0x4c,m.word(0x325460));
        if(1.0f<=m.scalar(s+0xc)) {
            m.scalar(s+0xc,1);
            if(ClutchMatched()!=0 || m.word(s+0x30)==1) m.word(s+0x38,3);
        }
        if(Idling()!=0) m.word(s+0x38,0);
    } else if(state==3) {
        m.scalar(engine+0x4c,1);auto current=m.word(s+0x30);
        if(current!=1) {
            const float f20=m.scalar(s+0x40)-m.scalar(engine+0xc);
            const float f2=GetDriveWheelSlippage();
            if(m.scalar(0x325458)<m.scalar(engine+0x44) && 0.0f<f2) {
                const float f0=m.scalar(engine+0xc);m.scalar(s+0x40,f0);SetDifferentialAngularVelocity(f0);
            } else {
                float f0=f20*m.scalar(0x32545c);f0=m.scalar(engine+0xc)+f0;m.scalar(engine+0xc,f0);
            }
            if(Idling()!=0) m.word(s+0x38,0);
            current=m.word(s+0x30);
        }
        if(current!=m.word(s+0x2c)) {m.word(s+0x38,0);m.word(s+0x3c,1);}
    }
}
std::uint32_t DriveTrain::GetDerivative(std::uint32_t out) {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    const auto at=0x3077c8u+m.word(s+0x38)*4;
    const float value=m.scalar(m.word(s+0x10)+0x1c)*m.scalar(at);
    m.scalar(out,value);return 0;
}
void Engine::Sub_00188998(std::uint32_t config,std::uint32_t car) {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    m.word(s+8,0x2ac3f8);
    Table(m,s+0x18).Sub_0018D810(config+0x10,9,m.scalar(config),m.scalar(config+8));
    Table(m,s+0x2c).Sub_0018D810(config+0x34,3,m.scalar(config),m.scalar(config+8));
    m.word(s+0x10,config);m.word(s+0x14,car);m.scalar(s+0x58,1);
    m.word(s+0x40,0);m.word(s+0x50,0);m.word(s+0x54,0);m.scalar(s+0x4c,1);
}
void DriveTrain::Sub_00189020(std::uint32_t config,std::uint32_t engine,
                  std::uint32_t wheels,std::uint32_t car) {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    m.word(s+0x14,engine);const auto engineconfig=m.word(engine+0x10);
    m.word(s+8,0x2ac3b0);m.word(s+0x10,config);
    for(std::uint32_t i=0;i<4;++i) m.word(s+0x18+i*4,m.word(wheels+i*4));
    m.word(s+0x28,car);
    CalculateShiftPoints(m,config,engineconfig,s+0x4c,s+0x70,m.scalar(0x327924));
    m.word(s+0x2c,2);m.word(s+0xc,0);m.word(s+0x38,2);m.word(s+0x30,2);
    m.word(s+0x3c,0);
}
float Engine::GetTorque() {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    const float f20=m.scalar(s+0xc)*m.scalar(0x325424);
    float result=Table(m,s+0x18).GetValue(f20)*m.scalar(s+0x58);
    if(m.word(s+0x40)!=0) result=result*m.scalar(0x32791c);
    const float f0=m.scalar(m.word(s+0x10)+8)-100.0f;
    if(!(f20<f0)) result=(-m.scalar(0x327918))*result;
    return result;
}
float Engine::GetTotalEngineTorque() {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    float f20=GetTorque();
    float f1=m.scalar(s+0x44)*m.scalar(s+0x4c);f20=f20*f1;
    if(m.word(s+0x50)!=0) f20=f20*3.0f;
    float f0=m.scalar(s+0xc)*m.scalar(0x325428);
    f0= -Table(m,s+0x2c).GetValue(f0);
    float f2=m.scalar(s+0x44)*m.scalar(s+0x4c);
    f1=1.0f-f2;f0=f0*f1;f1=f20+f0;
    if(m.word(m.word(s+0x14)+0x39c)==1 && f1<0.0f) f1=f1*m.scalar(0x32542c);
    return f1;
}
std::uint32_t Engine::GetDerivative(std::uint32_t out) {
    auto& m=m_;const auto s=address_;
    using namespace detail_group_0018f808;
    float f0=GetTotalEngineTorque();f0=f0+m.scalar(s+0x48);
    f0=f0/m.scalar(m.word(s+0x10)+0xc);m.scalar(out,f0);return 0;
}

void Engine::DecrementNitro() {
 EngineTimerProjection s{m_.signed_word(address_+0x50),m_.scalar(address_+0x54)};
 DecrementNitro(s,m_.signed_word(0x327cf8),m_.scalar(0x325430));m_.word(address_+0x50,static_cast<std::uint32_t>(s.Field_0x50));
}
float Engine::GetEngineTorque(float rpm) {
 float torque=Table(m_,address_+0x18).GetValue(rpm)*m_.scalar(address_+0x58);
 if(m_.word(address_+0x40)!=0) torque=torque*m_.scalar(0x32791c);
 if(m_.scalar(m_.word(address_+0x10)+8)-100.0f<=rpm) torque=torque*(-m_.scalar(0x327918));
 return torque;
}
void DriveTrain::FinalizeClutchTorque() {}
void DriveTrain::PrintCurrentState() {}
std::int32_t DriveTrain::SetSportGear(std::int32_t value){using namespace port::driving::native::detail_group_0018bce0_memory;auto s=load(m_,address_);const auto result=SetSportGear(s,value);store(m_,address_,s);return result;}
std::int32_t DriveTrain::SetDesiredGear(std::int32_t value){using namespace port::driving::native::detail_group_0018bce0_memory;auto s=load(m_,address_);const auto result=SetDesiredGear(s,value,globals(m_));store(m_,address_,s);return result;}
float DriveTrain::GetDifferentialAngularAcceleration(){using namespace port::driving::native::detail_group_0018bce0_memory;auto s=load(m_,address_);const auto result=GetDifferentialAngularAcceleration(s,globals(m_));store(m_,address_,s);return result;}
void DriveTrain::ApplyClutchTorque(float value){using namespace port::driving::native::detail_group_0018bce0_memory;auto s=load(m_,address_);ApplyClutchTorque(value,s,globals(m_));store(m_,address_,s);}
void DriveTrain::GuessClutchTorque(){using namespace port::driving::native::detail_group_0018bce0_memory;auto s=load(m_,address_);GuessClutchTorque(s,globals(m_),[&]{return Engine(m_,m_.word(address_+0x14)).GetTotalEngineTorque();});store(m_,address_,s);}
void DriveTrain::ModifyClutchTorque(){using namespace port::driving::native::detail_group_0018bce0_memory;auto s=load(m_,address_);ModifyClutchTorque(s,globals(m_),[&]{return Engine(m_,m_.word(address_+0x14)).GetTotalEngineTorque();});store(m_,address_,s);}
}
namespace port::driving {
std::int32_t SetSportGear(DrivetrainProjection& s,std::int32_t value) {return hp2::recovered::DriveTrain::SetSportGear(s,value);}
std::int32_t SetDesiredGear(DrivetrainProjection& s,std::int32_t request,const DrivetrainGlobals& g) {return hp2::recovered::DriveTrain::SetDesiredGear(s,request,g);}
float GetDifferentialAngularAcceleration(const DrivetrainProjection& s,const DrivetrainGlobals& g) {return hp2::recovered::DriveTrain::GetDifferentialAngularAcceleration(s,g);}
void ApplyClutchTorque(float value,DrivetrainProjection& s,const DrivetrainGlobals& g) {return hp2::recovered::DriveTrain::ApplyClutchTorque(value,s,g);}
void GuessClutchTorque(DrivetrainProjection& s,const DrivetrainGlobals& g,const EngineEvaluation& engine) {return hp2::recovered::DriveTrain::GuessClutchTorque(s,g,engine);}
void ModifyClutchTorque(DrivetrainProjection& s,const DrivetrainGlobals& g,const EngineEvaluation& engine) {return hp2::recovered::DriveTrain::ModifyClutchTorque(s,g,engine);}
void FinalizeClutchTorque() {}
void DecrementNitro(EngineTimerProjection&s,std::int32_t ticks,float scale){hp2::recovered::Engine::DecrementNitro(s,ticks,scale);}
void Sub_0018A7E8(Memory&m,std::uint32_t a,std::uint32_t c,std::uint32_t car){return hp2::recovered::Engine(m,a).Sub_00188998(c,car);}
void Sub_0018AE70(Memory&m,std::uint32_t a,std::uint32_t c,std::uint32_t e,std::uint32_t w,std::uint32_t car){return hp2::recovered::DriveTrain(m,a).Sub_00189020(c,e,w,car);}
float GetTorque(Memory&m,std::uint32_t a){return hp2::recovered::Engine(m,a).GetTorque();}
float GetTotalEngineTorque(Memory&m,std::uint32_t a){return hp2::recovered::Engine(m,a).GetTotalEngineTorque();}
std::uint32_t Engine::GetDerivative(Memory&m,std::uint32_t a,std::uint32_t out){return hp2::recovered::Engine(m,a).GetDerivative(out);}
}
namespace port::driving::native {
std::int32_t SetSportGear(Memory&m,std::uint32_t a,std::int32_t value){return hp2::recovered::DriveTrain(m,a).SetSportGear(value);}
std::int32_t SetDesiredGear(Memory&m,std::uint32_t a,std::int32_t value){return hp2::recovered::DriveTrain(m,a).SetDesiredGear(value);}
void GuessClutchTorque(Memory&m,std::uint32_t a){return hp2::recovered::DriveTrain(m,a).GuessClutchTorque();}
void ModifyClutchTorque(Memory&m,std::uint32_t a){return hp2::recovered::DriveTrain(m,a).ModifyClutchTorque();}
void DecrementNitro(Memory&m,std::uint32_t a){return hp2::recovered::Engine(m,a).DecrementNitro();}
}
namespace port::driving {



}

namespace hp2::recovered {
// Original inline member bodies, Debug0018A348..0018A4A0; E-0044-E01.
void Engine::SetThrottleOverride(float value){m_.scalar(address_+0x4c,value);}
void Engine::ApplyTorque(float value){m_.scalar(address_+0x48,m_.scalar(address_+0x48)+value);}
float Engine::GetNitroBoostTimer(){return m_.scalar(address_+0x54);}
void Engine::SetNitroBoostTimer(float value){m_.scalar(address_+0x54,value);}
void Engine::SetNitroFlag(std::int32_t value){m_.word(address_+0x50,static_cast<std::uint32_t>(value));}
std::int32_t Engine::GetNitroFlag(){return m_.signed_word(address_+0x50);}
std::uint32_t Engine::GetEI(){return m_.word(address_+0x10);}
float Engine::GetMaxRPM(){return m_.scalar(GetEI()+8);}
float Engine::GetRedLineRPM(){return m_.scalar(GetEI()+4);}
float Engine::GetIdleRPM(){return m_.scalar(GetEI());}
float Engine::GetEffectiveThrottle(){return GetThrottle()*m_.scalar(address_+0x4c);}
float Engine::GetThrottle(){return m_.scalar(address_+0x44);}
float Engine::GetTorqueResponse(){return 1.0f/m_.scalar(GetEI()+0xc);}
float Engine::GetAngularAcceleration(){return (GetTotalEngineTorque()+m_.scalar(address_+0x48))/m_.scalar(GetEI()+0xc);}
float Engine::GetAngularVelocity(){return m_.scalar(address_+0xc);}
void Engine::SetIsHP(std::int32_t value){m_.word(address_+0x40,static_cast<std::uint32_t>(value));}
void Engine::SetState(float value){m_.scalar(address_+0xc,value);}
void Engine::SetState(std::uint32_t input){m_.word(address_+0xc,m_.word(input));}
void Engine::GetState(std::uint32_t out){m_.word(out,m_.word(address_+0xc));}
float Engine::GetState(){return m_.scalar(address_+0xc);}
std::int32_t Engine::GetSize(){return 1;}
void Engine::SetCatchupTorqueMultiplier(float value){m_.scalar(address_+0x58,value);}
void Engine::SetThrottle(float value){m_.scalar(address_+0x44,value);}
// Original inline drivetrain members, Debug0018A1B0..0018A2F0; E-0044-E01.
std::int32_t DriveTrain::RearWheelDrive(){return m_.scalar(0x32548c)<=m_.scalar(GetDriveTrainInfo()+0x10);}
std::int32_t DriveTrain::FrontWheelDrive(){return m_.scalar(GetDriveTrainInfo()+0x10)<=m_.scalar(0x325490);}
std::int32_t DriveTrain::GetTopGear(){return port::driving::detail_group_0018bce0::add_one(m_.signed_word(GetDriveTrainInfo()+0x18));}
std::int32_t DriveTrain::GetDesiredGear(){return m_.signed_word(address_+0x2c);}
std::int32_t DriveTrain::GetPreviousGear(){return m_.signed_word(address_+0x34);}
std::int32_t DriveTrain::GetSportGear(){return m_.signed_word(address_+0x48);}
std::int32_t DriveTrain::GetCurrentGear(){return m_.signed_word(address_+0x30);}
std::int32_t DriveTrain::IsClutchEngaged(){return GetCurrentClutchState()==3;}
std::uint32_t DriveTrain::GetEngine(){return m_.word(address_+0x14);}
std::uint32_t DriveTrain::GetDriveTrainInfo(){return m_.word(address_+0x10);}
float DriveTrain::GetShiftDownRPM(std::int32_t gear){return m_.scalar(address_+0x70+static_cast<std::uint32_t>(gear)*4);}
float DriveTrain::GetShiftUpRPM(std::int32_t gear){return m_.scalar(address_+0x4c+static_cast<std::uint32_t>(gear)*4);}
std::int32_t DriveTrain::GetCurrentClutchState(){return m_.signed_word(address_+0x38);}
void DriveTrain::SetState(std::int32_t gear){m_.word(address_+0x2c,static_cast<std::uint32_t>(gear));m_.scalar(address_+0xc,1);m_.word(address_+0x38,3);m_.word(address_+0x30,static_cast<std::uint32_t>(gear));}
void DriveTrain::SetState(std::uint32_t input){m_.word(address_+0xc,m_.word(input));}
void DriveTrain::GetState(std::uint32_t out){m_.word(out,m_.word(address_+0xc));}
float DriveTrain::GetState(){return m_.scalar(address_+0xc);}
std::int32_t DriveTrain::GetSize(){return 1;}
}

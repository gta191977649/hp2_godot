// TASK-0041 E-0041-R02; retail 0x0018BCE0: DriveTrain::SetDesiredGear(Gear).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#include "Sub_0018BCE0.hpp"
#include "port/not_implemented.hpp"
#include "port/driving/ee_scalar.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace port::driving {
namespace {
std::int32_t add_one(std::int32_t value) {
    const std::uint32_t bits=static_cast<std::uint32_t>(value)+std::uint32_t{1};
    std::int32_t result; std::memcpy(&result,&bits,sizeof(result)); return result;
}
// Port bounds checks guard projections; these are not original game branches.
std::size_t index(std::int32_t value) {
    if(value<0 || value>=9) throw std::out_of_range("Drivetrain projection index outside captured table");
    return static_cast<std::size_t>(value);
}
float evaluate(const EngineEvaluation& engine) {
    if(!engine) hp2::not_implemented("0x0018AAB0","engine curve evaluation dependency");
    return engine();
}
}
// ELF 0018ABF0..0018AC33; E-0038-D01, strict expiry, branch-likely store.
void Sub_0018ABF0(EngineTimerProjection& s,std::int32_t ticks,float scale) {
    if(s.Field_0x50!=0 && s.Field_0x54+4.0f < static_cast<float>(ticks)*scale) s.Field_0x50=0;
}
// ELF 0018BCA0..0018BCDB; E-0038-D02.
std::int32_t Sub_0018BCA0(DrivetrainProjection& s,std::int32_t value) {
    if(value>1 && value<=add_one(s.config_Field_0x18)) {
        if(s.Field_0x48==1) s.Field_0x44=0;
        s.Field_0x48=value;
    }
    return s.Field_0x48;
}
// ELF 0018BCE0..0018C02B; E-0038-D03. Preserve all stores before early returns.
std::int32_t SetDesiredGear(DrivetrainProjection& s,std::int32_t request,const DrivetrainGlobals& g) {
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
// ELF 0018B360; E-0038-D04. The two axle branches have different addition order.
float Sub_0018B360(const DrivetrainProjection& s,const DrivetrainGlobals& g) {
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
// ELF 0018B6C8; E-0038-D05. Each wheel's +10c gate remains independent.
void Sub_0018B6C8(float value,DrivetrainProjection& s,const DrivetrainGlobals& g) {
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
        // Original DIV.S0018B794/0018B8A0 can see zero wheel values at rest.
        // Project only that EE exceptional operation; no epsilon/balanced substitute.
        float ratio=sum==0.0f ? ee_div_s_zero(numerator,sum,rear_axle?"0x0018B8A0":"0x0018B794") : numerator/sum;
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
// ELF 0018B948; E-0038-D06. Unresolved engine branch has an explicit dependency seam.
void Sub_0018B948(DrivetrainProjection& s,const DrivetrainGlobals& g,const EngineEvaluation& engine) {
    if(s.Field_0x38==3) { Sub_0018B6C8(evaluate(engine),s,g); return; }
    if(s.Field_0x38!=0 && s.Field_0x38!=2) return;
    float f4=s.engine_Field_0xc-s.Field_0x40;
    if(g.Field_003308bc<f4) f4=g.Field_003308bc;
    else if(f4< -g.Field_003308bc) f4= -g.Field_003308bc;
    const float f1=(s.Field_0x40-s.engine_Field_0xc)*g.Field_0032e370;
    float f2=s.config_Field_0x14;
    if(s.Field_0x4c!=0 && f1*s.Field_0x4c<0 && 2000.0f<std::abs(f1) && 2000.0f<std::abs(s.Field_0x4c)) f2=f2*0.5f;
    const float f0=f4*f2;
    s.Field_0x4c=f1;
    Sub_0018B6C8(f0*s.Field_0xc,s,g);
}
// ELF 0018BAA0; E-0038-D07. Keep instruction multiply grouping, unlike decompile.
void Sub_0018BAA0(DrivetrainProjection& s,const DrivetrainGlobals& g,const EngineEvaluation& engine) {
    if(s.Field_0x38!=3) return;
    float f20=evaluate(engine)+s.engine_Field_0x48;
    f20=f20/s.engine_config_Field_0xc;
    const float f12=f20-Sub_0018B360(s,g);
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
    Sub_0018B6C8(f12/f5,s,g);
}
// ELF 0018BC10: jr ra / nop; E-0038-D07. Confirmed empty entry, not a fallback.
void Sub_0018BC10() {}
}


// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x0018B948; E-0042-U111; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void SolverDependencies::GuessClutchTorque() {
    using namespace detail_group_00138f38; hp2::not_implemented("0x0018B948", "solver dependency"); }
}

// Retail 0x0018BAA0; E-0042-C071; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void SolverDependencies::ModifyClutchTorque() {
    using namespace detail_group_00138f38; hp2::not_implemented("0x0018BAA0", "solver iteration dependency"); }
}

// Retail 0x0018BC10; E-0042-U112; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void SolverDependencies::FinalizeClutchTorque() {
    using namespace detail_group_00138f38; port::driving::FinalizeClutchTorque(); }
}

// Retail 0x0018ABF0; E-0042-C063; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void DecrementNitro(EngineTimerProjection& s,std::int32_t ticks,float scale) {
    using namespace detail_group_0018bce0;
    if(s.Field_0x50!=0 && s.Field_0x54+4.0f < static_cast<float>(ticks)*scale) s.Field_0x50=0;
}
}

// Retail 0x0018BCA0; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
std::int32_t SetSportGear(DrivetrainProjection& s,std::int32_t value) {
    using namespace detail_group_0018bce0;
    if(value>1 && value<=add_one(s.config_Field_0x18)) {
        if(s.Field_0x48==1) s.Field_0x44=0;
        s.Field_0x48=value;
    }
    return s.Field_0x48;
}
}

// Retail 0x0018BCE0; E-0041-R02; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
std::int32_t SetDesiredGear(DrivetrainProjection& s,std::int32_t request,const DrivetrainGlobals& g) {
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
}

// Retail 0x0018B360; E-0042-C068; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
float GetDifferentialAngularAcceleration(const DrivetrainProjection& s,const DrivetrainGlobals& g) {
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
}

// Retail 0x0018B6C8; E-0042-C070; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void ApplyClutchTorque(float value,DrivetrainProjection& s,const DrivetrainGlobals& g) {
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
}

// Retail 0x0018B948; E-0042-U111; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void GuessClutchTorque(DrivetrainProjection& s,const DrivetrainGlobals& g,const EngineEvaluation& engine) {
    using namespace detail_group_0018bce0;
    if(s.Field_0x38==3) { ApplyClutchTorque(evaluate(engine),s,g); return; }
    if(s.Field_0x38!=0 && s.Field_0x38!=2) return;
    float f4=s.engine_Field_0xc-s.Field_0x40;
    if(g.Field_003308bc<f4) f4=g.Field_003308bc;
    else if(f4< -g.Field_003308bc) f4= -g.Field_003308bc;
    const float f1=(s.Field_0x40-s.engine_Field_0xc)*g.Field_0032e370;
    float f2=s.config_Field_0x14;
    if(s.Field_0x4c!=0 && f1*s.Field_0x4c<0 && 2000.0f<std::abs(f1) && 2000.0f<std::abs(s.Field_0x4c)) f2=f2*0.5f;
    const float f0=f4*f2;
    s.Field_0x4c=f1;
    ApplyClutchTorque(f0*s.Field_0xc,s,g);
}
}

// Retail 0x0018BAA0; E-0042-C071; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void ModifyClutchTorque(DrivetrainProjection& s,const DrivetrainGlobals& g,const EngineEvaluation& engine) {
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
}

// Retail 0x0018BC10; E-0042-U112; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void FinalizeClutchTorque() {
    using namespace detail_group_0018bce0;}
}

// Retail 0x0018BCA0; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving::native {
std::int32_t SetSportGear(Memory& m,std::uint32_t a,std::int32_t request){
    using namespace detail_group_0018bce0_memory;auto s=load(m,a);const auto r=port::driving::SetSportGear(s,request);store(m,a,s);return r;}
}

// Retail 0x0018BCE0; E-0041-R02; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving::native {
std::int32_t SetDesiredGear(Memory& m,std::uint32_t a,std::int32_t request){
    using namespace detail_group_0018bce0_memory;auto s=load(m,a);const auto r=port::driving::SetDesiredGear(s,request,globals(m));store(m,a,s);return r;}
}

// Retail 0x0018ABF0; E-0042-C063; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving::native {
void DecrementNitro(Memory& m,std::uint32_t a){
    using namespace detail_group_0018bce0_memory;EngineTimerProjection s{m.signed_word(a+0x50),m.scalar(a+0x54)};port::driving::DecrementNitro(s,m.signed_word(0x00330c78),m.scalar(0x0032e338));m.word(a+0x50,static_cast<std::uint32_t>(s.Field_0x50));}
}

// Retail 0x0018B948; E-0042-U111; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving::native {
void GuessClutchTorque(Memory& m,std::uint32_t a){
    using namespace detail_group_0018bce0_memory;auto s=load(m,a);port::driving::GuessClutchTorque(s,globals(m),[&]{return port::driving::GetTotalEngineTorque(m,m.word(a+0x14));});store(m,a,s);}
}

// Retail 0x0018BAA0; E-0042-C071; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving::native {
void ModifyClutchTorque(Memory& m,std::uint32_t a){
    using namespace detail_group_0018bce0_memory;auto s=load(m,a);port::driving::ModifyClutchTorque(s,globals(m),[&]{return port::driving::GetTotalEngineTorque(m,m.word(a+0x14));});store(m,a,s);}
}

// Retail 0x0018A7E8; E-0042-C059; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void Sub_0018A7E8(Memory& m,std::uint32_t s,std::uint32_t config,std::uint32_t car) {
    using namespace detail_group_0018f808;
    m.word(s+8,0x2b4618);
    Sub_0018F808(m,s+0x18,config+0x10,9,m.scalar(config),m.scalar(config+8));
    Sub_0018F808(m,s+0x2c,config+0x34,3,m.scalar(config),m.scalar(config+8));
    m.word(s+0x10,config);m.word(s+0x14,car);m.scalar(s+0x58,1);
    m.word(s+0x40,0);m.word(s+0x50,0);m.word(s+0x54,0);m.scalar(s+0x4c,1);
}
}

// Retail 0x0018AE70; E-0042-U109; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
void Sub_0018AE70(Memory& m,std::uint32_t s,std::uint32_t config,std::uint32_t engine,
                  std::uint32_t wheels,std::uint32_t car) {
    using namespace detail_group_0018f808;
    m.word(s+0x14,engine);const auto engineconfig=m.word(engine+0x10);
    m.word(s+8,0x2b45d0);m.word(s+0x10,config);
    for(std::uint32_t i=0;i<4;++i) m.word(s+0x18+i*4,m.word(wheels+i*4));
    m.word(s+0x28,car);
    Sub_0018AC38(m,config,engineconfig,s+0x50,s+0x74,m.scalar(0x3308ac));
    m.word(s+0x2c,2);m.word(s+0xc,0);m.word(s+0x38,2);m.word(s+0x30,2);
    m.word(s+0x3c,0);m.word(s+0x4c,0);
}
}

// Retail 0x0018AA28; E-0042-C061; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
float GetTorque(Memory& m,std::uint32_t s) {
    using namespace detail_group_0018f808;
    const float f20=m.scalar(s+0xc)*m.scalar(0x32e32c);
    float result=Table::GetValue(m,s+0x18,f20)*m.scalar(s+0x58);
    if(m.word(s+0x40)!=0) result=result*m.scalar(0x3308a4);
    const float f0=m.scalar(m.word(s+0x10)+8)-100.0f;
    if(!(f20<f0)) result=(-m.scalar(0x3308a0))*result;
    return result;
}
}

// Retail 0x0018AAB0; E-0042-C062; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
float GetTotalEngineTorque(Memory& m,std::uint32_t s) {
    using namespace detail_group_0018f808;
    float f20=GetTorque(m,s);
    float f1=m.scalar(s+0x44)*m.scalar(s+0x4c);f20=f20*f1;
    if(m.word(s+0x50)!=0) f20=f20*3.0f;
    float f0=m.scalar(s+0xc)*m.scalar(0x32e330);
    f0= -Table::GetValue(m,s+0x2c,f0);
    float f2=m.scalar(s+0x44)*m.scalar(s+0x4c);
    f1=1.0f-f2;f0=f0*f1;f1=f20+f0;
    if(m.word(m.word(s+0x14)+0x39c)==1 && f1<0.0f) f1=f1*m.scalar(0x32e334);
    return f1;
}
}

// Retail 0x0018A9D8; E-0042-C060; E-0042-S001; exact Debug TU ..\indep\src\engine.cpp
namespace port::driving {
std::uint32_t Engine::GetDerivative(Memory& m,std::uint32_t s,std::uint32_t out) {
    using namespace detail_group_0018f808;
    float f0=GetTotalEngineTorque(m,s);f0=f0+m.scalar(s+0x48);
    f0=f0/m.scalar(m.word(s+0x10)+0xc);m.scalar(out,f0);return 0;
}
}

#include "game_core/indep/src/engine.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0018AE70; E-0042-U109; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018ae70.md.
void DriveTrain::Sub_0018AE70() {
    hp2::not_implemented("0x0018AE70", "DriveTrain::Sub_0018AE70 (signature and full body unresolved)");
}

// ELF 0x0018B360; E-0042-C068; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018b360.md.
void DriveTrain::GetDifferentialAngularAcceleration() {
    hp2::not_implemented("0x0018B360", "DriveTrain::GetDifferentialAngularAcceleration (signature and full body unresolved)");
}

// ELF 0x0018B6C8; E-0042-C070; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018b6c8.md.
void DriveTrain::ApplyClutchTorque() {
    hp2::not_implemented("0x0018B6C8", "DriveTrain::ApplyClutchTorque (signature and full body unresolved)");
}

// ELF 0x0018B948; E-0042-U111; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018b948.md.
void DriveTrain::GuessClutchTorque() {
    hp2::not_implemented("0x0018B948", "DriveTrain::GuessClutchTorque (signature and full body unresolved)");
}

// ELF 0x0018BAA0; E-0042-C071; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018baa0.md.
void DriveTrain::ModifyClutchTorque() {
    hp2::not_implemented("0x0018BAA0", "DriveTrain::ModifyClutchTorque (signature and full body unresolved)");
}

// ELF 0x0018BC10; E-0042-U112; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018bc10.md.
void DriveTrain::FinalizeClutchTorque() {
    hp2::not_implemented("0x0018BC10", "DriveTrain::FinalizeClutchTorque (signature and full body unresolved)");
}

// ELF 0x0018BCA0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018bca0.md.
void DriveTrain::SetSportGear() {
    hp2::not_implemented("0x0018BCA0", "DriveTrain::SetSportGear (signature and full body unresolved)");
}

// ELF 0x0018BCE0; E-0041-R02; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018bce0.md.
void DriveTrain::SetDesiredGear() {
    hp2::not_implemented("0x0018BCE0", "DriveTrain::SetDesiredGear (signature and full body unresolved)");
}

// ELF 0x0018C030; E-0042-U113; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0018c030.md.
void DriveTrain::PrintCurrentState() {
    hp2::not_implemented("0x0018C030", "DriveTrain::PrintCurrentState (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0018A7E8; E-0042-C059; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018a7e8.md.
void Engine::Sub_0018A7E8() {
    hp2::not_implemented("0x0018A7E8", "Engine::Sub_0018A7E8 (signature and full body unresolved)");
}

// ELF 0x0018A9D8; E-0042-C060; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018a9d8.md.
void Engine::GetDerivative() {
    hp2::not_implemented("0x0018A9D8", "Engine::GetDerivative (signature and full body unresolved)");
}

// ELF 0x0018AA28; E-0042-C061; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018aa28.md.
void Engine::GetTorque() {
    hp2::not_implemented("0x0018AA28", "Engine::GetTorque (signature and full body unresolved)");
}

// ELF 0x0018AAB0; E-0042-C062; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018aab0.md.
void Engine::GetTotalEngineTorque() {
    hp2::not_implemented("0x0018AAB0", "Engine::GetTotalEngineTorque (signature and full body unresolved)");
}

// ELF 0x0018ABF0; E-0042-C063; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018abf0.md.
void Engine::DecrementNitro() {
    hp2::not_implemented("0x0018ABF0", "Engine::DecrementNitro (signature and full body unresolved)");
}
} // namespace hp2::recovered

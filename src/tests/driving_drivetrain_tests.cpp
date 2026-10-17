#include "game_core/port/driving/projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include <limits>
#include <cstring>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace port::driving;
namespace {
void check(bool ok,const char* text) { if(!ok) throw std::runtime_error(text); }
DrivetrainGlobals globals() {
    DrivetrainGlobals g;
    // Synthetic, exactly representable values; not asserted original defaults.
    g.Field_0032e358=0.25f; g.Field_0032e35c=0.75f;
    g.Field_0032e370=2; g.Field_0032e374=0.75f; g.Field_0032e378=0.25f;
    g.Field_0032e388=g.Field_0032e390=1;
    g.Field_0032e38c=g.Field_0032e394=0.25f;
    g.Field_003308b8=0.25f; g.Field_003308bc=3000; g.Field_003308c4=0.5f;
    g.Field_00330c88=0.5f; return g;
}
DrivetrainProjection fixture() {
    DrivetrainProjection s;
    s.Field_0x30=3; s.Field_0x38=3; s.Field_0x48=1;
    s.config_Field_0x18=5; s.config_Field_0x20.fill(2);
    s.Field_0x50.fill(200); s.Field_0x74.fill(100);
    s.engine_Field_0xc=300; s.engine_config_Field_0x0=50;
    s.engine_config_Field_0x8=1000; s.engine_config_Field_0xc=2;
    s.config_Field_0x8=s.config_Field_0xc=2; s.config_Field_0x10=0.5f;
    for(auto& w:s.wheels) { w.Field_0x155=1; w.Field_0x24=2; w.config_Field_0xc=2; w.Field_0x20=4; }
    return s;
}
}
int main() {
    const auto raw=[](float value){std::uint32_t bits;std::memcpy(&bits,&value,4);return bits;};
    check(raw(ee_div_s_zero(0.0f,0.0f,"0x0018B794"))==0x7f7fffffu,"EE positive zero over positive zero saturates positive");
    check(raw(ee_div_s_zero(-0.0f,0.0f,"0x0018B794"))==0xff7fffffu,"EE negative zero numerator sign retained");
    check(raw(ee_div_s_zero(0.0f,-0.0f,"0x0018B8A0"))==0xff7fffffu,"EE negative zero denominator sign retained");
    check(raw(ee_div_s_zero(-0.0f,-0.0f,"0x0018B8A0"))==0x7f7fffffu,"EE two negative zeros saturate positive");
    check(raw(ee_div_s_zero(-2.0f,-0.0f,"0x0018B8A0"))==0x7f7fffffu,"Finite numerator uses XOR sign");
    bool bad_operand=false;try{(void)ee_div_s_zero(std::numeric_limits<float>::infinity(),0,"0x0018B794");}catch(const hp2::NotImplemented&){bad_operand=true;}
    check(bad_operand,"Zero divide projection rejects unsupported nonfinite numerator");
    const auto g=globals();
    EngineTimerProjection timer{7,2};
    DecrementNitro(timer,12,0.5f); check(timer.Field_0x50==7,"Expiry equality is not strict less");
    DecrementNitro(timer,13,0.5f); check(timer.Field_0x50==0,"Expiry crossing clears flag");
    timer={0,100}; DecrementNitro(timer,0,1); check(timer.Field_0x54==100,"Inactive timer preserves stamp");
    auto s=fixture(); s.Field_0x44=9;
    check(SetSportGear(s,1)==1 && s.Field_0x44==9,"Below lower bound preserves fields");
    check(SetSportGear(s,6)==6 && s.Field_0x44==0,"Inclusive upper bound and old1 timer clear");
    s.Field_0x44=7; check(SetSportGear(s,2)==2 && s.Field_0x44==7,"Other previous value preserves timer");
    check(SetSportGear(s,7)==2,"Above inclusive upper bound ignored");
    s=fixture(); s.config_Field_0x18=std::numeric_limits<std::int32_t>::max();
    check(SetSportGear(s,2)==1,"Upper-bound addiu wraps32 before signed comparison");
    s.Field_0x2c=9; SetDesiredGear(s,-1,g);
    check(s.Field_0x2c==9,"Automatic upper bound also preserves addiu wrap");
    s=fixture(); check(SetDesiredGear(s,8,g)==8 && s.Field_0x2c==8,"Direct request passes through");
    s=fixture(); s.Field_0x30=1; s.Field_0x38=0;
    check(SetDesiredGear(s,-1,g)==-1 && s.Field_0x2c==2 && s.Field_0x48==1,"Low current sets2 before state gate");
    s=fixture(); s.Field_0x38=0; s.Field_0x2c=7;
    SetDesiredGear(s,-1,g); check(s.Field_0x48==1 && s.Field_0x2c==7,"Automatic request writes48 even when gated");
    s=fixture(); SetDesiredGear(s,-1,g); check(s.Field_0x2c==4,"All four wheels allow increase");
    for(int i=0;i<4;++i) {
        s=fixture(); s.wheels[i].Field_0x155=0; s.Field_0x2c=9;
        SetDesiredGear(s,-1,g); check(s.Field_0x2c==9,"Each contact byte inhibits increase");
        s=fixture(); s.wheels[i].Field_0x160=4; s.Field_0x2c=9;
        SetDesiredGear(s,-1,g); check(s.Field_0x2c==9,"Each wheel strict4 boundary inhibits increase");
    }
    s=fixture(); s.engine_Field_0xc=100; s.Field_0x2c=9;
    SetDesiredGear(s,-1,g); check(s.Field_0x2c==9,"Down threshold equality does not decrement");
    s.engine_Field_0xc=99; SetDesiredGear(s,-1,g); check(s.Field_0x2c==2,"Strict down threshold decrements");
    s=fixture(); s.Field_0x30=6; s.Field_0x2c=9;
    SetDesiredGear(s,-1,g); check(s.Field_0x2c==9,"Maximum selected index inhibits increase");
    s=fixture(); s.Field_0x30=2; s.engine_Field_0xc=149; s.Field_0x2c=9;
    SetDesiredGear(s,-1,g); check(s.Field_0x2c==1,"Current2 low-control fallback uses config0 plus100");
    s=fixture(); s.Field_0x30=2; s.engine_Field_0xc=149; s.engine_Field_0x44=0.25f; s.Field_0x2c=9;
    SetDesiredGear(s,-1,g); check(s.Field_0x2c==9,"Low-control equality skips down fallback");
    s=fixture(); s.Field_0x38=0; s.Field_0x44=2;
    check(SetDesiredGear(s,-2,g)==-2 && s.Field_0x44==2.5f,"Minus2 advances timer before gate");
    s=fixture(); s.Field_0x2c=s.Field_0x48=3; s.Field_0x44=9.5f; s.Field_0x34=3;
    check(SetDesiredGear(s,-2,g)==-2 && s.Field_0x44==10,"Timer equality preserves minus2");
    check(SetDesiredGear(s,-2,g)==-1,"Timer above10 returns minus1");
    s=fixture(); s.Field_0x2c=4; s.Field_0x48=2; s.engine_Field_0xc=600;
    check(SetDesiredGear(s,-2,g)==-2 && s.Field_0x2c==3 && s.Field_0x48==3 && s.Field_0x44==0 && s.Field_0x34==3,"Cancels request above projected threshold");
    s=fixture(); s.Field_0x2c=4; s.Field_0x48=2;
    SetDesiredGear(s,-2,g); check(s.Field_0x2c==2 && s.Field_0x44==0 && s.Field_0x34==3,"Descending state records previous and resets timer");
    s=fixture(); s.Field_0x2c=2; s.Field_0x48=4;
    SetDesiredGear(s,-2,g); check(s.Field_0x2c==4 && s.Field_0x44==0,"Ascending state increments current");
    s=fixture(); s.Field_0x2c=s.Field_0x48=3; s.Field_0x34=3; s.engine_Field_0xc=149;
    check(SetDesiredGear(s,-2,g)==-1 && s.Field_0x2c==2,"Equal request low value decrements");
    // E-0038-D04 independent register arithmetic: wheel lane (10-1*2)/2=4;
    // each axle (4+4)*.5*2=8; both -> (8+8)*.5*2=16.
    s=fixture(); for(auto& w:s.wheels) { w.Field_0x108=10; w.Field_0x114=1; }
    check(GetDifferentialAngularAcceleration(s,g)==16,"Both axle arithmetic fixture");
    s.config_Field_0x10=0; check(GetDifferentialAngularAcceleration(s,g)==16,"Front branch only");
    s.config_Field_0x10=1; check(GetDifferentialAngularAcceleration(s,g)==16,"Rear branch only");
    // E-0038-D05: value8 * ratio2=16; each axle16*.5*2=16;
    // equal+20 fields give split .5, producing8 per wheel; engine20-8=12.
    s=fixture(); s.engine_Field_0x48=20;
    ApplyClutchTorque(8,s,g); check(s.engine_Field_0x48==12,"Engine subtraction occurs once");
    for(const auto& w:s.wheels) check(w.Field_0x108==8,"Balanced four-wheel split");
    s=fixture();for(auto& w:s.wheels){w.Field_0x20=0;w.Field_0x155=0;}
    ApplyClutchTorque(8,s,g);
    check(s.wheels[0].Field_0x108==14 && s.wheels[1].Field_0x108==2 && s.wheels[2].Field_0x108==2 && s.wheels[3].Field_0x108==14,"Zero wheel ratio follows EE saturation then original bounds, not balanced split");
    for(const auto& w:s.wheels)check(std::isfinite(w.Field_0x108),"Rest launch ratio remains finite");
    s=fixture(); s.Field_0x30=1; s.engine_Field_0x48=20;
    ApplyClutchTorque(8,s,g); check(s.engine_Field_0x48==20 && s.wheels[0].Field_0x108==0,"Current1 is confirmed early return");
    s=fixture(); s.car_Field_0x3a4=1; s.wheels[0].Field_0x10c=1;
    ApplyClutchTorque(8,s,g);
    check(s.wheels[0].Field_0x108==0 && s.wheels[1].Field_0x108==12 && s.wheels[2].Field_0x108==12 && s.wheels[3].Field_0x108==4,"Asymmetric split and independent blocked wheel");
    s=fixture(); s.car_Field_0x3a4=2; ApplyClutchTorque(8,s,g);
    check(s.wheels[0].Field_0x108==12 && s.wheels[1].Field_0x108==4 && s.wheels[2].Field_0x108==4 && s.wheels[3].Field_0x108==12,"Mode2 bound retains front rear asymmetry");
    s=fixture(); s.car_Field_0x3a4=2; s.wheels[1].Field_0x155=0; s.wheels[2].Field_0x155=0;
    ApplyClutchTorque(8,s,g); for(const auto& w:s.wheels) check(w.Field_0x108==8,"Either missing contact selects half bound");
    s=fixture(); s.wheels[0].Field_0x20=1; s.wheels[1].Field_0x20=7;
    s.wheels[2].Field_0x20=1; s.wheels[3].Field_0x20=7;
    ApplyClutchTorque(8,s,g);
    check(s.wheels[0].Field_0x108==14 && s.wheels[1].Field_0x108==2 && s.wheels[2].Field_0x108==14 && s.wheels[3].Field_0x108==2,"Front/rear ratio numerator asymmetry");
    s=fixture(); s.Field_0x38=0; s.Field_0xc=1; s.config_Field_0x14=1;
    s.engine_Field_0xc=4000; s.Field_0x40=0; s.Field_0x4c=4000;
    GuessClutchTorque(s,g); check(s.Field_0x4c==-8000 && s.engine_Field_0x48==-1500,"Clamped direction reversal halves coupling");
    s=fixture(); s.Field_0x38=2; s.Field_0xc=1; s.config_Field_0x14=1;
    s.engine_Field_0xc=1000; s.Field_0x40=0; s.Field_0x4c=4000;
    GuessClutchTorque(s,g); check(s.Field_0x4c==-2000 && s.engine_Field_0x48==-1000,"Exact2000 does not halve coupling");
    s=fixture(); s.Field_0x38=1; GuessClutchTorque(s,g); ModifyClutchTorque(s,g);
    check(s.engine_Field_0x48==0,"Nonactive branch does not require engine evaluation");
    s=fixture(); bool threw=false;
    try { GuessClutchTorque(s,g); } catch(const hp2::NotImplemented&) { threw=true; }
    check(threw,"Missing engine algorithm throws");
    // E-0038-D07: engine(8+0)/2=4; derivative0; inverse inertia
    // .5 + .25*(4*.5)*4 + .25*(4*.5)*4 =4.5; transfer4/4.5.
    s=fixture(); ModifyClutchTorque(s,g,[]{return 8.0f;});
    check(s.engine_Field_0x48== -(4.0f/4.5f),"Coupling instruction multiply grouping fixture");
    FinalizeClutchTorque();
    std::cout<<"DRIVING_DRIVETRAIN_OK: instruction fixtures, no original equivalence claim\n";
}

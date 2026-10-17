#include "game_core/indep/src/engine.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/table.hpp"
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
    GuessClutchTorque(s,g); check(s.Field_0x4c==4000 && s.engine_Field_0x48==-3000,"Debug clamps torque without Retail history or halving");
    s=fixture(); s.Field_0x38=2; s.Field_0xc=1; s.config_Field_0x14=1;
    s.engine_Field_0xc=1000; s.Field_0x40=0; s.Field_0x4c=4000;
    GuessClutchTorque(s,g); check(s.Field_0x4c==4000 && s.engine_Field_0x48==-1000,"Debug does not write legacy history projection field");
    s=fixture(); s.Field_0x38=1; GuessClutchTorque(s,g); ModifyClutchTorque(s,g);
    check(s.engine_Field_0x48==0,"Nonactive branch does not require engine evaluation");
    s=fixture(); bool threw=false;
    try { GuessClutchTorque(s,g); } catch(const hp2::NotImplemented&) { threw=true; }
    check(threw,"Missing engine algorithm throws");
    // E-0038-D07: engine(8+0)/2=4; derivative0; inverse inertia
    // .5 + .25*(4*.5)*4 + .25*(4*.5)*4 =4.5; transfer4/4.5.
    s=fixture(); ModifyClutchTorque(s,g,[]{return 8.0f;});
    check(s.engine_Field_0x48== -(4.0f/4.5f),"Coupling instruction multiply grouping fixture");
    // E-0044-E01: actual Debug layout. Independent listing fixtures use deliberately
    // different neighboring shift entries, making a +4 displacement observable.
    Memory memory;
    std::vector<std::uint8_t> literals(0x1000),globalsBytes(0x2000);
    memory.map(0x325000,literals);memory.map(0x327000,globalsBytes);
    std::vector<std::uint8_t> clutchStates(16);memory.map(0x3077c8,clutchStates);
    const auto config=memory.allocate(0x50),engineConfig=memory.allocate(0x40);
    const auto engineAddress=memory.allocate(0x5c),drive=memory.allocate(0x94);
    const auto car=memory.allocate(0x400),wheels=memory.allocate(16);
    for(unsigned i=0;i<4;++i) {
      const auto wheel=memory.allocate(0x180),wi=memory.allocate(0x20);
      memory.word(wheels+i*4,wheel);memory.word(wheel+0xc,wi);
      memory.scalar(wheel+0x24,2);memory.byte(wheel+0x155,1);
    }
    memory.scalar(engineConfig,50);memory.scalar(engineConfig+4,900);memory.scalar(engineConfig+8,1000);memory.scalar(engineConfig+0xc,2);
    for(unsigned i=0;i<9;++i)memory.scalar(engineConfig+0x10+i*4,200);
    for(unsigned i=0;i<3;++i)memory.scalar(engineConfig+0x34+i*4,20);
    memory.word(config+0x18,3);memory.scalar(config,0.5f);
    for(unsigned i=0;i<8;++i)memory.scalar(config+0x20+i*4,2);
    memory.scalar(0x327924,0.75f);memory.scalar(0x327928,0.5f);
    hp2::recovered::Engine actualEngine(memory,engineAddress);
    actualEngine.Sub_00188998(engineConfig,car);
    hp2::recovered::DriveTrain actualDrive(memory,drive);
    actualDrive.Sub_00189020(config,engineAddress,wheels,car);
    check(memory.word(engineAddress+8)==0x2ac3f8 && memory.word(drive+8)==0x2ac3b0,"Debug constructor vtables");
    check(memory.scalar(drive+0x4c+2*4)==475 && memory.scalar(drive+0x70+3*4)==237.5f,"Debug +4C/+70 shift arrays from constructor");
    check(memory.scalar(drive+0x4c+4*4)==1000,"Terminal ShiftUpRPM entry uses maximum RPM");
    memory.scalar(config+0x1c,4);memory.scalar(0x3077c8+2*4,0.5f);
    const auto clutchDerivative=memory.allocate(4);actualDrive.GetDerivative(clutchDerivative);
    check(memory.scalar(clutchDerivative)==2,"Debug drivetrain derivative indexes actual3077C8 clutch-state table");
    memory.word(drive+0x30,3);memory.word(drive+0x38,3);memory.word(drive+0x2c,8);
    memory.scalar(engineAddress+0xc,300);memory.scalar(0x32547c,1);memory.scalar(0x325480,0.25f);
    memory.scalar(drive+0x4c+3*4,250);memory.scalar(drive+0x70+3*4,100);
    actualDrive.SetDesiredGear(-1);check(memory.word(drive+0x2c)==4,"Instance reads actual Debug upshift array offset");
    memory.word(drive+0x38,0);memory.scalar(drive+0xc,1);memory.scalar(drive+0x40,0);
    memory.scalar(config+0x14,2);memory.scalar(config+0x10,0.5f);memory.scalar(0x327934,100);
    memory.scalar(engineAddress+0x48,0);memory.scalar(drive+0x4c,777);
    actualDrive.GuessClutchTorque();
    check(memory.scalar(engineAddress+0x48)==-200 && memory.scalar(drive+0x4c)==777,"Debug clutch never aliases ShiftUpRPM[0] as torque history");
    memory.scalar(0x325424,1);memory.scalar(0x325428,1);memory.scalar(engineAddress+0xc,300);
    memory.scalar(engineAddress+0x44,0.5f);memory.scalar(engineAddress+0x4c,0.5f);
    check(actualEngine.GetTotalEngineTorque()==35,"Canonical engine torque plus off-throttle braking");
    const auto tableAddress=memory.allocate(20),samples=memory.allocate(12);
    memory.scalar(samples,10);memory.scalar(samples+4,20);memory.scalar(samples+8,30);
    hp2::recovered::Table actualTable(memory,tableAddress);actualTable.Sub_0018D810(samples,3,100,300);
    check(actualTable.GetValue(150)==15,"Canonical bounded Table interpolation");
    memory.scalar(0x3254d8,0.01f);memory.scalar(0x3254dc,0.01f);
    check(actualTable.InverseLookup(10)==10 && actualTable.InverseLookup(25)==250,"Debug inverse unusual exact-first return retained");
    memory.scalar(0x325420,2);actualEngine.SetState(10.0f);actualEngine.ApplyTorque(7);
    actualEngine.ComputeAuxiliaries();
    check(actualEngine.GetState()==100 && memory.scalar(engineAddress+0x48)==0,"Debug engine clamps idle and clears road torque");
    actualEngine.SetState(3000.0f);actualEngine.ComputeAuxiliaries();check(actualEngine.GetState()==2000,"Debug engine maximum clamp");
    actualEngine.SetState(300.0f);actualEngine.SetThrottle(0.5f);actualEngine.SetThrottleOverride(0.5f);
    const auto derivative=memory.allocate(4);actualEngine.GetDerivative(derivative);
    check(memory.scalar(derivative)==17.5f,"Canonical engine derivative divides total plus road torque by inertia");
    const auto graphAddress=memory.allocate(8),points=memory.allocate(24);
    memory.scalar(points,10);memory.scalar(points+4,100);memory.scalar(points+8,20);memory.scalar(points+12,200);memory.scalar(points+16,30);memory.scalar(points+20,300);
    memory.scalar(0x3254e0,0.01f);memory.scalar(0x3254e4,0.01f);
    hp2::recovered::Graph graph(memory,graphAddress);graph.Sub_0018D9E8(points,3);
    check(graph.GetValue(15)==150 && graph.GetValue(-1)==100 && graph.GetValue(31)==300,"Canonical Graph forward interpolation and clamps");
    check(graph.GetInverse(150)==15 && graph.GetInverse(0)==10 && graph.GetInverse(400)==30,"Canonical Graph inverse interpolation and clamps");
    graph.Sub_0018D9E8(points,1);check(graph.GetInverse(150)==100,"Debug one-point inverse fallback returns first y");
    FinalizeClutchTorque();
    std::cout<<"DRIVING_DRIVETRAIN_OK: instruction fixtures, no original equivalence claim\n";
}

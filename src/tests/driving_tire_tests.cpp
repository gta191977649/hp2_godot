// TASK-0044: fixtures now target Debug addresses; historical API adapters forward canonical classes.
#include "game_core/indep/src/wheel.hpp"
#include "game_core/indep/inc/PhysicsCar.hpp"
#include "game_core/port/driving/surface.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <vector>
using namespace port::driving;
namespace {
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
bool close(float a,float b){return std::abs(a-b)<0.0001f;}
void globals(Memory&m){
    // Original scalar coefficient bits read in Ghidra, E-0038-T01/T10.
    const std::uint32_t p[]={0x38c90fdb,0x4096cbe4,0x40c90fdb,0x3fc90fdb,0x40490fdb,0x3e2aaaab,0x3c088889,0x39500d01,0x3638ef1d,0x4622f983};
    for(unsigned i=0;i<10;++i)m.word(0x32625c+i*4,p[i]);
    const std::uint32_t q[]={0x40c90fdb,0x40c90fdb,0x40490fdb,0x3fc90fdb,0xbe2aaaa4,0x3c08873e,0xb94fb21f,0x362e9c14,0x38c90fdb,0x3fc90fdc};
    for(unsigned i=0;i<10;++i)m.word(0x324b1c+i*4,q[i]);
    m.word(0x324b44,0x38c90fdb);m.word(0x324b48,0x3fc90fdc);m.word(0x324b4c,0x38c90fdb);m.word(0x324b50,0x3fc90fdc);
    m.word(0x326280,0x4622f983); // gp-0x709c original phase scale.
    m.scalar(0x32f210,0.0001f);m.scalar(0x32f214,0.0001f);
    for(auto base:{0x325734u,0x32575cu}){
        m.word(base,0x38c90fdb);m.word(base+4,0x40490fd0);m.word(base+8,0x40c90fd0);m.word(base+12,0x40c90fd0);
    }
    m.word(0x32576c,0x3fc90fdc);m.scalar(0x325754,0.001f);m.scalar(0x325758,0.001f);
    m.scalar(0x325744,0.15f);m.scalar(0x325748,2.237f);m.scalar(0x32574c,0.1f);
    m.scalar(0x325734-8,13.410818f);m.scalar(0x325734-4,0.11185f);
    m.scalar(0x327d4c,0);m.scalar(0x327d48,1);m.scalar(0x327d50,1);m.scalar(0x327d5c,1);m.scalar(0x327d64,1);
    m.scalar(0x327d70,0.2f);m.word(0x327d6c,1);m.word(0x327d60,0);
    m.vector(0x317b20,{{0,0,0,1}});m.scalar(0x32571c,0.001f);
}
SolverMatrix identity(){SolverMatrix mat{};mat[0]=mat[5]=mat[10]=mat[15]=1;return mat;}

// TASK-0045 E-0045-T01/T02/T04: independent finite fixtures around the canonical
// algorithms. Only the five approved material coefficients below come from ELF data;
// table curves, loads, velocities and noise samples are deliberately synthetic.
void surface_regressions(){
    Memory m;m.map(0x2f0000,0x50000);globals(m);
    struct MaterialBits {unsigned id,stat,dynamic,rolling,noise;};
    const MaterialBits surfaces[]={
        {1,0x3f800000,0x3f800000,0x3f800000,0x00000000},
        {2,0x3f7851ec,0x3f75374c,0x3f800000,0x3d4ccccd},
        {6,0x3f75c28f,0x3f72b021,0x3f866666,0x3d4ccccd},
        {7,0x3f75c28f,0x3f733333,0x40000000,0x3d4ccccd},
        {8,0x3f733333,0x3f7028f6,0x3f800000,0x3c23d70a}
    };
    for(const auto& material:surfaces){const auto row=0x3102e8+material.id*0x90;
        m.word(row+8,material.id);m.scalar(row+0x18,1);m.word(row+0x1c,material.noise);
        m.word(row+0x20,material.stat);m.word(row+0x24,material.dynamic);m.word(row+0x28,material.rolling);
    }
    const auto wheel=m.allocate(0x190),info=m.allocate(0x20),tire=m.allocate(0x20),control=m.allocate(0x30),car=m.allocate(0x410);
    m.word(wheel+0xc,info);m.word(wheel+0x10,tire);m.word(wheel+0x14,control);m.word(wheel+0x18,car);
    m.scalar(info+0xc,2);m.word(info+0x14,2);m.scalar(tire,1);m.scalar(tire+4,1);m.scalar(tire+8,1);
    m.scalar(tire+0xc,2);m.scalar(tire+0x10,2);m.scalar(wheel+0x24,4);
    m.scalar(wheel+0x178,1);m.scalar(wheel+0x17c,1);m.matrix(wheel+0x70,identity());
    m.scalar(car+0x380,10);m.word(car+0x39c,0);m.scalar(0x327d3c,0.5f);
    hp2::recovered::Wheel canonical(m,wheel);
    const auto deriv=m.allocate(8),point=m.allocate(16);
    m.vector(point,{{0.5f,0.5f,0,0}});
    for(unsigned i=0;i<16;++i)m.scalar(0x2f0d40+4*i,float(i));
    float road_rolling=0,sand_rolling=0;
    for(const auto& material:surfaces){
        const auto row=0x3102e8+material.id*0x90;m.word(wheel+0x150,material.id);
        // A pure longitudinal saturated candidate isolates each exact static multiplier.
        m.scalar(wheel+0x108,0);m.scalar(wheel+0x114,100);m.scalar(wheel+0x118,0);
        m.scalar(wheel+0x158,10);m.byte(wheel+0x154,0);m.byte(wheel+0x155,1);m.vector(wheel+0xf0,{{0,0,0,0}});
        canonical.FinalizeGroundForce(0);
        check(close(m.scalar(wheel+0x114),20*m.scalar(row+0x20)),"Each approved surface caps force with its original static multiplier");
        check(m.scalar(wheel+0x168)==1,"Saturation publishes original usage ratio");
        // Pure slip=-4, nonzero road velocity: capacity20*dynamic and no low-speed cancellation.
        m.scalar(wheel+0x20,0);m.scalar(wheel+0x108,0);m.scalar(wheel+0x158,10);
        m.word(wheel+0x10c,0);m.vector(wheel+0xf0,{{4,0,0,0}});canonical.GuessGroundForce(0,0);
        check(m.byte(wheel+0x154)==1&&close(m.scalar(wheel+0x114),-20*m.scalar(row+0x24)),"Dynamic branch reads the queried wheel material row");
        check(m.scalar(wheel+0x160)==-4&&m.scalar(wheel+0x164)==-4,"CurrentSlip and OldSlip retain original difference stores");
        // With drag isolated, sand's original rolling multiplier2 doubles road drag.
        m.scalar(wheel+0x20,2);m.scalar(wheel+0x108,0);m.scalar(wheel+0x114,0);m.scalar(wheel+0x16c,0);
        m.scalar(0x325720,0.25f);m.byte(wheel+0x154,1);canonical.GetDerivative(deriv);
        check(close(m.scalar(deriv+4),-0.125f*m.scalar(row+0x28)),"Rolling resistance uses its separate original multiplier");
        if(material.id==1)road_rolling=m.scalar(deriv+4);if(material.id==7)sand_rolling=m.scalar(deriv+4);
        check(close(hp2::recovered::GetRoadNoise(m,point,int(material.id)),12*m.scalar(row+0x1c)),"Material road-noise amplitude multiplies bilinear grid value");
    }
    check(sand_rolling==2*road_rolling,"Sand and road differ in rolling resistance independently of tire-force limits");
    m.word(wheel+0x150,1);m.vector(wheel+0xf0,{{0,0,0,0}});m.scalar(wheel+0x108,0);m.word(wheel+0x10c,0);
    for(float load:{2.0f,4.0f}){m.scalar(wheel+0x158,load);m.scalar(wheel+0x114,100);m.scalar(wheel+0x118,0);canonical.FinalizeGroundForce(0);
        check(close(m.scalar(wheel+0x114),2*load),"Normal load scales the original force cap");}
    m.scalar(wheel+0x158,0);m.scalar(wheel+0x114,100);m.byte(wheel+0x155,1);canonical.FinalizeGroundForce(0);
    check(m.scalar(wheel+0x114)==0&&m.scalar(wheel+0x168)==1,"Zero load retains the original diagnostic ratio convention");
    m.byte(wheel+0x155,0);canonical.FinalizeGroundForce(0);check(m.scalar(wheel+0x168)==0,"No-contact overrides usage independently of material");
    // TractionBoost and CatchupFrictionMultiplier remain independent original factors.
    m.byte(wheel+0x155,1);m.scalar(wheel+0x158,10);m.scalar(wheel+0x20,0);m.scalar(wheel+0x16c,0);m.vector(wheel+0xf0,{{4,0,0,0}});
    m.scalar(wheel+0x178,2);m.scalar(wheel+0x17c,0.5f);m.scalar(wheel+0x108,0);canonical.GuessGroundForce(0,0);
    check(close(m.scalar(wheel+0x114),-20),"Independent boost and catchup factors multiply dynamic capacity");
    m.scalar(wheel+0x17c,1);m.scalar(wheel+0x108,0);canonical.GuessGroundForce(0,0);check(close(m.scalar(wheel+0x114),-40),"Catchup is not folded into the stored tire coefficient");
    // The old curve uses stored ZForce rather than its load argument; the new curve uses normalized load.
    const auto values=m.allocate(8);m.scalar(values,2);m.scalar(values+4,2);Sub_0018F808(m,0x308658,values,2,0,4);
    m.word(0x327d60,0);m.scalar(wheel+0x158,10);
    check(close(canonical.GetLateralForce(1,0.25f),20)&&close(canonical.GetLateralForce(999,0.25f),20),"Original old lateral curve uses the wheel load store");
    m.scalar(0x325750,1);m.scalar(0x327d68,1);m.word(0x327d60,1);
    for(unsigned i=0;i<7;++i){const auto samples=m.allocate(8),table=m.allocate(0x14);m.scalar(samples,float(i*2));m.scalar(samples+4,float(i*2+10));Sub_0018F808(m,table,samples,2,0,10);m.word(0x3087c0+i*4,table);}
    check(close(canonical.GetLateralForce(2,0.25f),5)&&close(canonical.GetLateralForce(4,0.25f),7),"New lateral family interpolates both angle curves and load axis");
    check(close(canonical.GetLateralForce(2,1),14),"Large slip angle selects the seventh original family curve");
    // Strict MaxStaticSlip comparison: equality retains the non-sliding route.
    m.scalar(info+0xc,1);m.scalar(tire+0x10,1);m.scalar(0x327d3c,1);m.scalar(wheel+0x178,1);m.scalar(wheel+0x17c,1);
    m.vector(wheel+0xf0,{{1,0,0,0}});m.scalar(wheel+0x20,2);m.scalar(wheel+0x108,0);m.word(wheel+0x10c,0);canonical.GuessGroundForce(0,0);
    check(m.byte(wheel+0x154)==0,"Slip equal to MaxStaticSlip does not select sliding");
    m.scalar(wheel+0x20,std::nextafter(2.0f,std::numeric_limits<float>::infinity()));m.scalar(wheel+0x108,0);canonical.GuessGroundForce(0,0);
    check(m.byte(wheel+0x154)==1,"Next representable larger slip selects sliding");
    // W01 publication copies original stores without modifying state or combining coefficients.
    m.word(wheel+0x150,8);m.scalar(wheel+0x158,3.5f);m.scalar(wheel+0x15c,0.125f);m.scalar(wheel+0x160,-2);
    m.scalar(wheel+0x168,0.75f);m.scalar(wheel+0x178,1.25f);m.scalar(wheel+0x17c,0.8f);
    m.scalar(wheel+0x100,-2);m.scalar(wheel+0x104,4);m.scalar(wheel+0x114,10);m.scalar(wheel+0x118,-20);
    m.scalar(tire+0xc,2);m.scalar(tire+0x10,3);m.word(wheel+0x184,0x7fc12345u);
    const auto image=m.view(wheel,0x190);const std::vector<std::uint8_t> before(image.data,image.data+image.size);
    const auto observed=observe_grip(m,wheel),again=observe_grip(m,wheel);
    check(observed.terrain==8&&observed.slipping&&observed.z_force==3.5f&&observed.current_slip==-2&&observed.road_noise==0.125f,"Observer returns original material/load/slip/noise stores");
    check(observed.slip==std::array<float,2>{{-2,4}}&&observed.ground_force==std::array<float,2>{{10,-20}},"Observer retains original two-vector stores");
    check(observed.traction_usage==0.75f&&observed.traction_boost==1.25f&&observed.catchup_friction==0.8f&&observed.static_coefficient==2&&observed.dynamic_coefficient==3,"Observer does not recompute an effective grip coefficient");
    check(again.ground_force==observed.ground_force&&std::equal(before.begin(),before.end(),image.data),"Repeated diagnostic observation leaves every wheel byte unchanged");
    m.word(wheel+0x150,32);check(observe_grip(m,wheel).terrain==32,"Observer accepts original CAR row while the synthetic surface selector excludes it");
    // T03: yaw boost reads mutable original globals and preserves grade/driver-mode branches.
    m.scalar(0x324dd0,0.25f);m.scalar(0x327060,1);m.scalar(0x327064,3);m.scalar(0x327068,0);m.scalar(0x32706c,10);m.scalar(0x327070,100);
    float grade=-0.5f;const float yaw=hp2::recovered::YawFrictionBoost(m,0.5f,0,5,1,2,0,grade,0.25f);
    check(grade==0.5f&&yaw==4.5f,"Player yaw branch uses both controls and speed-interpolated boost");
    grade=-0.5f;check(hp2::recovered::YawFrictionBoost(m,0.5f,0,5,1,2,1,grade,0.25f)==3,"Other driver mode applies post-collision factor to NFS control");
    grade=-0.5f;check(hp2::recovered::YawFrictionBoost(m,0.125f,1,5,1,2,0,grade,0.25f)==1.5f,"Ebrake and small yaw return original grade base");
}
}
int main(){
    Memory m;m.map(0x2f0000,0x50000);globals(m);
    m.scalar(0x32577c,0.25f);Sub_001A57E0(m);
    check(m.scalar(0x327d38)==0.25f && m.word(0x308658)==0x308638 && m.word(0x30865c)==7 && m.scalar(0x308668)==24,"Static tire curve header initialization");
    check(m.word(0x3087a8)==0x308700 && m.word(0x3087ac)==6 && m.scalar(0x3087b8)==0.5f,"Seventh tire-family header initialization");
    Sub_0025AFA8(m);check(m.vector(0x317b20)==SolverVector{{0,0,0,1}},"Original math-global initialization");
    // Synthetic piecewise table only entry0; axis-angle paths ask ratio0 exactly.
    m.half(0x316e80,0);m.scalar(0x316e84,0);
    check(bASin(m,1)==0x4000 && bASin(m,-1)==0xc000,"Asin table saturation endpoints");
    m.half(0x316ec0,100);m.scalar(0x316ec4,0.5f);
    check(bASin(m,0.25390625f)==228 && bASin(m,-0.25390625f)==static_cast<std::uint16_t>(-228),"Asin table fractional interpolation and sign");
    m.half(0x316f80,300);m.scalar(0x316f84,0.5f);
    check(bASin(m,0.75f)==300,"Asin upper interval search");
    check(bATan(m,1,0)==0 && bATan(m,0,1)==0x4000,"Axis quadrant outputs");
    check(bATan(m,-1,0)==-32768 && bATan(m,0,-1)==-16384,"Signed negative quadrants");
    check(bFixATan(m,65536,0)==0 && bFixATan(m,0,65536)==0x4000,"Fixed heading positive axes");
    check(bFixATan(m,-65536,0)==-32768 && bFixATan(m,0,-65536)==-16384,"Fixed heading signed quadrants");
    check(bDiv(0,0)==0 && bDiv(1,0)==2147483647 && bDiv(-1,0)==(-2147483647-1),"Fixed division zero literals");
    check(bDiv(3,2)==98302,"Fixed division signed64 product extraction");
    m.half(0x317910,100);m.half(0x317912,200);
    check(bFixATan(m,1024)==150 && bFixATan(m,-1024)==static_cast<std::uint16_t>(-150),"Fixed angle table interpolation and sign");
    check(bFixATan(m,0x1000000)==0x3fff && bFixATan(m,0x200000)==0x3edd,"Fixed angle high ranges");
    m.half(0x317910,0);m.half(0x317912,0);
    check(close(bSin(m,std::uint16_t{0}),0) && close(bSin(m,std::uint16_t{0x4000}),1),"Original polynomial zero and quarter turn");
    check(close(bSin(m,0.0f),0),"Float angle conversion dispatch");
    check(close(Sub_0011B6E0(m,0),0) && close(Sub_0011B6E0(m,1.57079637f),1),"Separate original render polynomial");
    SolverMatrix ma=identity(),mb=identity();ma[12]=2;mb[13]=3;
    eMulMatrix(ma,ma,mb);check(ma[12]==2 && ma[13]==3,"Matrix multiplication aliases safely");
    const auto wheel=m.allocate(0x180),config=m.allocate(0x40),tire=m.allocate(0x40),brake=m.allocate(0x10),car=m.allocate(0x600),carconfig=m.allocate(0x560);
    hp2::recovered::Wheel original_wheel(m,wheel);
    m.word(wheel+0xc,config);m.word(wheel+0x10,tire);m.word(wheel+0x14,brake);m.word(wheel+0x18,car);
    m.word(car,carconfig);m.scalar(config+0xc,2);m.word(config+0x14,2);
    m.scalar(tire,1);m.scalar(tire+4,1);m.scalar(tire+8,1);m.scalar(tire+0xc,1);m.scalar(tire+0x10,1);
    m.scalar(wheel+0x17c,1);m.scalar(wheel+0x178,1);m.scalar(wheel+0x158,3);m.word(wheel+0x150,0);
    m.matrix(wheel+0x70,identity());m.vector(wheel+0xf0,{{0,0,0,0}});
    m.scalar(0x327d54,0.5f);m.scalar(car+0x380,18);m.scalar(0x32572c,13);m.scalar(0x325730,0.1f);
    check(close(original_wheel.GetPilotFactor(),0.75f),"Finite double mix midpoint");
    m.word(wheel+0x10c,1);check(original_wheel.GetPilotFactor()==1,"Threshold flag bypasses double mix");m.word(wheel+0x10c,0);
    const auto table=m.allocate(0x14),values=m.allocate(8);m.scalar(values,2);m.scalar(values+4,2);
    Sub_0018F808(m,0x308658,values,2,0,4);m.word(0x327d60,0);
    // Non-sliding path with zero local XY: +108/config radius; no invented default lateral force.
    m.scalar(wheel+0x108,10);original_wheel.GuessGroundForce(0,0);
    check(m.byte(wheel+0x154)==0 && m.scalar(wheel+0x114)==5 && close(m.scalar(wheel+0x118),6),"Non-sliding force follows original tire table");
    // Sliding path: slip=-4, capacity6, magnitude4 => forceX=-6.
    m.scalar(0x31030c,1);m.scalar(0x327d3c,0.5f);m.scalar(wheel+0x158,6);
    m.vector(wheel+0xf0,{{4,0,0,0}});m.scalar(wheel+0x108,-2);
    original_wheel.GuessGroundForce(0,0);
    check(m.byte(wheel+0x154)==1 && close(m.scalar(wheel+0x114),-6),"Sliding route retains operation grouping");
    check(close(m.scalar(wheel+0x108),-0.3f) && m.scalar(wheel+0x160)==-4,"Negative request adjustment and slip stores");
    // Finalizer norm5/cap4 ->(2.4,3.2), then force vector transforms all lanes.
    m.vector(wheel+0xf0,{{0,0,0,0}});m.scalar(wheel+0x108,0);m.scalar(wheel+0x114,3);m.scalar(wheel+0x118,4);
    m.scalar(wheel+0x158,2);m.scalar(0x310308,2);m.byte(wheel+0x154,0);m.byte(wheel+0x155,1);
    original_wheel.FinalizeGroundForce(0);
    check(close(m.scalar(wheel+0x114),2.4f) && close(m.scalar(wheel+0x118),3.2f),"Finalizer clamps vector magnitude");
    check(close(m.scalar(wheel+0x120),2.4f) && close(m.scalar(wheel+0x124),3.2f) && m.scalar(wheel+0x128)==2,"Finalizer force publication");
    check(m.scalar(wheel+0x168)==1,"Finalizer ratio upper bound");
    m.byte(wheel+0x155,0);original_wheel.FinalizeGroundForce(0);check(m.scalar(wheel+0x168)==0,"Contact byte overrides final ratio");
    // Independent oblique normal exercises all force basis modes (normal0,.6,.8).
    m.vector(wheel+0x140,{{0,0.6f,0.8f,0}});m.scalar(0x310308,100);m.byte(wheel+0x155,1);
    const std::uint32_t modes[]={1,2,3,4,99};const float ys[]={3,4.32f,4.8f,5.4f,3},zs[]={4,1.76f,1.4f,3.2f,4};
    for(unsigned i=0;i<5;++i){m.word(0x327d6c,modes[i]);m.scalar(wheel+0x114,2);m.scalar(wheel+0x118,3);m.scalar(wheel+0x158,4);
        original_wheel.FinalizeGroundForce(0);check(close(m.scalar(wheel+0x120),2) && close(m.scalar(wheel+0x124),ys[i]) && close(m.scalar(wheel+0x128),zs[i]),"Finalizer original basis mode publication");}
    m.word(0x327d6c,3);m.vector(wheel+0x140,{{1,0,0,0}});bool exceptional=false;
    try{original_wheel.FinalizeGroundForce(0);}catch(const hp2::NotImplemented&){exceptional=true;}
    check(exceptional,"Degenerate VU normalization remains explicit addressed gap");m.word(0x327d6c,1);
    // Table family interpolation: index1.5 combines curve1 value2 and curve2 value4 =>3.
    m.word(0x327d60,1);m.scalar(0x327d64,1);m.scalar(0x327d68,1);m.scalar(0x325750,1);
    for(unsigned i=0;i<7;++i){const auto v=m.allocate(8);m.scalar(v,float(i*2));m.scalar(v+4,float(i*2));const auto t=m.allocate(0x14);Sub_0018F808(m,t,v,2,0,10);m.word(0x3087c0+i*4,t);}
    check(close(original_wheel.GetLateralForce(1,0.25f),3),"Adjacent family curve blend");
    (void)table;
    // Complete wheel/contact wrapper with selected synthetic plane provider.
    PlaneContact provider;provider.height=0.5f;m.word(config+0x14,0);m.matrix(car+0x50,identity());
    m.vector(car+0x20,{{1,2,3,1}});m.vector(car+0x30,{{4,5,6,0}});m.vector(car+0x90,{{0,0,0,0}});
    m.vector(carconfig+0x120,{{0,0,0,1}});m.scalar(carconfig+0x138,0);
    original_wheel.SetState(car,0,0,0,0,0.1f,provider);
    check(close(m.scalar(wheel+0xa0),1) && close(m.scalar(wheel+0xa4),2) && close(m.scalar(wheel+0xa8),3.1f),"Wheel world point derived by recovered matrices");
    check(m.word(wheel+0x150)==1 && m.byte(wheel+0x155)==1 && m.word(car+0x37c)==1,"Plane query publishes original material/contact stores");
    check(m.scalar(wheel+0x138)==0.5f && m.vector(wheel+0xf0)==SolverVector{{4,5,6,0}},"Contact height and velocity lanes");
    const auto body=m.allocate(0x80);m.vector(body+0x20,{{0,0,1,0}});
    const float distance=Sub_001A4580(m,wheel,wheel+0xa0,body,provider);
    check(close(distance,-2.6f) && m.scalar(wheel+0x15c)==0,"Contact projection and original zero-amplitude roughness");
    m.vector(body+0x20,{{1,0,0,0}});
    check(Sub_001A4580(m,wheel,wheel+0xa0,body,provider)==-100 && m.word(wheel+0x150)==0,"Near-tangent branch returns literal sentinel and clears material");
    // Derivative: separate material/rate drag groups and post-lock correction.
    const auto derivative=m.allocate(8);m.scalar(wheel+0x20,2);m.scalar(wheel+0x24,4);
    m.scalar(wheel+0x114,3);m.scalar(wheel+0x108,20);m.scalar(wheel+0x16c,0.25f);
    m.word(wheel+0x150,0);m.scalar(0x310310,0.5f);m.scalar(0x325720,0.2f);m.scalar(0x325724,0.4f);
    m.byte(wheel+0x154,1);m.word(wheel+0x10c,0);
    check(original_wheel.GetDerivative(derivative)==0 && m.scalar(derivative)==2 && close(m.scalar(derivative+4),3.4f),"Wheel derivative operation grouping");
    m.byte(wheel+0x154,0);m.word(wheel+0x10c,1);m.scalar(wheel+0x160,4);
    m.scalar(0x325728,0.001f);m.scalar(0x327d78,0.01f);
    original_wheel.GetDerivative(derivative);check(close(m.scalar(derivative+4),-200),"Correction follows lock zero store");
    check(Sub_001A63C0(m,wheel,derivative)==derivative && m.word(derivative)==0 && m.word(derivative+4)==0,"Controller derivative empty two-float state");
    surface_regressions();
    std::cout<<"driving_tire_tests passed (instruction fixtures, no original equivalence)\n";
}


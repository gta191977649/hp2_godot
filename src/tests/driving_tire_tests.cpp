#include "game_core/Sub_001A4930.hpp"
#include "game_core/Sub_0018F808.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace port::driving;
namespace {
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
bool close(float a,float b){return std::abs(a-b)<0.0001f;}
void globals(Memory&m){
    // Original scalar coefficient bits read in Ghidra, E-0038-T01/T10.
    const std::uint32_t p[]={0x38c90fdb,0x4096cbe4,0x40c90fdb,0x3fc90fdb,0x40490fdb,0x3e2aaaab,0x3c088889,0x39500d01,0x3638ef1d,0x4622f983};
    for(unsigned i=0;i<10;++i)m.word(0x32f1b4+i*4,p[i]);
    const std::uint32_t q[]={0x40c90fdb,0x40c90fdb,0x40490fdb,0x3fc90fdb,0xbe2aaaa4,0x3c08873e,0xb94fb21f,0x362e9c14,0x38c90fdb,0x3fc90fdc};
    for(unsigned i=0;i<10;++i)m.word(0x32da2c+i*4,q[i]);
    m.word(0x32da5c,0x38c90fdb);m.word(0x32da60,0x3fc90fdc);
    m.word(0x32e854,0x4622f983); // gp-0x709c original phase scale.
    m.scalar(0x32f210,0.0001f);m.scalar(0x32f214,0.0001f);
    for(auto base:{0x32e660u,0x32e688u}){
        m.word(base,0x38c90fdb);m.word(base+4,0x40490fd0);m.word(base+8,0x40c90fd0);m.word(base+12,0x40c90fd0);
    }
    m.word(0x32e698,0x3fc90fdc);m.scalar(0x32e680,0.001f);m.scalar(0x32e684,0.001f);
    m.scalar(0x32e670,0.15f);m.scalar(0x32e674,2.237f);m.scalar(0x32e678,0.1f);
    m.scalar(0x32e660-8,13.410818f);m.scalar(0x32e660-4,0.11185f);
    m.scalar(0x330ccc,0);m.scalar(0x330cc8,1);m.scalar(0x330cd0,1);m.scalar(0x330cdc,1);m.scalar(0x330ce4,1);
    m.scalar(0x330cf0,0.2f);m.word(0x330cec,1);m.word(0x330ce0,0);
    m.vector(0x320a60,{{0,0,0,1}});m.scalar(0x32e648,0.001f);
}
SolverMatrix identity(){SolverMatrix mat{};mat[0]=mat[5]=mat[10]=mat[15]=1;return mat;}
}
int main(){
    Memory m;m.map(0x2f0000,0x50000);globals(m);
    m.scalar(0x32e6a8,0.25f);Sub_001A57E0(m);
    check(m.scalar(0x330cb8)==0.25f && m.word(0x310f48)==0x310f28 && m.word(0x310f4c)==7 && m.scalar(0x310f58)==24,"Static tire curve header initialization");
    check(m.word(0x311098)==0x310ff0 && m.word(0x31109c)==6 && m.scalar(0x3110a8)==0.5f,"Seventh tire-family header initialization");
    Sub_0025AFA8(m);check(m.vector(0x320a60)==SolverVector{{0,0,0,1}},"Original math-global initialization");
    // Synthetic piecewise table only entry0; axis-angle paths ask ratio0 exactly.
    m.half(0x31fdc0,0);m.scalar(0x31fdc4,0);
    check(Sub_0025A748(m,1)==0x4000 && Sub_0025A748(m,-1)==0xc000,"Asin table saturation endpoints");
    m.half(0x31fe00,100);m.scalar(0x31fe04,0.5f);
    check(Sub_0025A748(m,0.25390625f)==228 && Sub_0025A748(m,-0.25390625f)==static_cast<std::uint16_t>(-228),"Asin table fractional interpolation and sign");
    m.half(0x31fec0,300);m.scalar(0x31fec4,0.5f);
    check(Sub_0025A748(m,0.75f)==300,"Asin upper interval search");
    check(Sub_0025A890(m,1,0)==0 && Sub_0025A890(m,0,1)==0x4000,"Axis quadrant outputs");
    check(Sub_0025A890(m,-1,0)==-32768 && Sub_0025A890(m,0,-1)==-16384,"Signed negative quadrants");
    check(Sub_0025AB50(m,65536,0)==0 && Sub_0025AB50(m,0,65536)==0x4000,"Fixed heading positive axes");
    check(Sub_0025AB50(m,-65536,0)==-32768 && Sub_0025AB50(m,0,-65536)==-16384,"Fixed heading signed quadrants");
    check(Sub_0025A300(0,0)==0 && Sub_0025A300(1,0)==2147483647 && Sub_0025A300(-1,0)==(-2147483647-1),"Fixed division zero literals");
    check(Sub_0025A300(3,2)==98302,"Fixed division signed64 product extraction");
    m.half(0x320850,100);m.half(0x320852,200);
    check(Sub_0025AA78(m,1024)==150 && Sub_0025AA78(m,-1024)==static_cast<std::uint16_t>(-150),"Fixed angle table interpolation and sign");
    check(Sub_0025AA78(m,0x1000000)==0x3fff && Sub_0025AA78(m,0x200000)==0x3edd,"Fixed angle high ranges");
    m.half(0x320850,0);m.half(0x320852,0);
    check(close(Sub_0025A540(m,0),0) && close(Sub_0025A540(m,0x4000),1),"Original polynomial zero and quarter turn");
    check(close(Sub_0025A5E0(m,0),0),"Float angle conversion dispatch");
    check(close(Sub_0011B6E0(m,0),0) && close(Sub_0011B6E0(m,1.57079637f),1),"Separate original render polynomial");
    SolverMatrix ma=identity(),mb=identity();ma[12]=2;mb[13]=3;
    Sub_0011B248(ma,ma,mb);check(ma[12]==2 && ma[13]==3,"Matrix multiplication aliases safely");
    const auto wheel=m.allocate(0x180),config=m.allocate(0x40),tire=m.allocate(0x40),brake=m.allocate(0x10),car=m.allocate(0x600),carconfig=m.allocate(0x560);
    m.word(wheel+0xc,config);m.word(wheel+0x10,tire);m.word(wheel+0x14,brake);m.word(wheel+0x18,car);
    m.word(car,carconfig);m.scalar(config+0xc,2);m.word(config+0x14,2);
    m.scalar(tire,1);m.scalar(tire+4,1);m.scalar(tire+8,1);m.scalar(tire+0xc,1);m.scalar(tire+0x10,1);
    m.scalar(wheel+0x17c,1);m.scalar(wheel+0x178,1);m.scalar(wheel+0x158,3);m.word(wheel+0x150,0);
    m.matrix(wheel+0x70,identity());m.vector(wheel+0xf0,{{0,0,0,0}});
    m.scalar(0x330cd4,0.5f);m.scalar(car+0x380,18);m.scalar(0x32e658,13);m.scalar(0x32e65c,0.1f);
    check(close(Sub_001A4818(m,wheel),0.75f),"Finite double mix midpoint");
    m.word(wheel+0x10c,1);check(Sub_001A4818(m,wheel)==1,"Threshold flag bypasses double mix");m.word(wheel+0x10c,0);
    const auto table=m.allocate(0x14),values=m.allocate(8);m.scalar(values,2);m.scalar(values+4,2);
    Sub_0018F808(m,0x310f48,values,2,0,4);m.word(0x330ce0,0);
    // Non-sliding path with zero local XY: +108/config radius; no invented default lateral force.
    m.scalar(wheel+0x108,10);Sub_001A4930(m,wheel,0,0);
    check(m.byte(wheel+0x154)==0 && m.scalar(wheel+0x114)==5 && close(m.scalar(wheel+0x118),6),"Non-sliding force follows original tire table");
    // Sliding path: slip=-4, capacity6, magnitude4 => forceX=-6.
    m.scalar(0x318c24,1);m.scalar(0x330cbc,0.5f);m.scalar(wheel+0x158,6);
    m.vector(wheel+0xf0,{{4,0,0,0}});m.scalar(wheel+0x108,-2);
    Sub_001A4930(m,wheel,0,0);
    check(m.byte(wheel+0x154)==1 && close(m.scalar(wheel+0x114),-6),"Sliding route retains operation grouping");
    check(close(m.scalar(wheel+0x108),-0.3f) && m.scalar(wheel+0x160)==-4,"Negative request adjustment and slip stores");
    // Finalizer norm5/cap4 ->(2.4,3.2), then force vector transforms all lanes.
    m.vector(wheel+0xf0,{{0,0,0,0}});m.scalar(wheel+0x108,0);m.scalar(wheel+0x114,3);m.scalar(wheel+0x118,4);
    m.scalar(wheel+0x158,2);m.scalar(0x318c20,2);m.byte(wheel+0x154,0);m.byte(wheel+0x155,1);
    Sub_001A4EE0(m,wheel,0);
    check(close(m.scalar(wheel+0x114),2.4f) && close(m.scalar(wheel+0x118),3.2f),"Finalizer clamps vector magnitude");
    check(close(m.scalar(wheel+0x120),2.4f) && close(m.scalar(wheel+0x124),3.2f) && m.scalar(wheel+0x128)==2,"Finalizer force publication");
    check(m.scalar(wheel+0x168)==1,"Finalizer ratio upper bound");
    m.byte(wheel+0x155,0);Sub_001A4EE0(m,wheel,0);check(m.scalar(wheel+0x168)==0,"Contact byte overrides final ratio");
    // Independent oblique normal exercises all force basis modes (normal0,.6,.8).
    m.vector(wheel+0x140,{{0,0.6f,0.8f,0}});m.scalar(0x318c20,100);m.byte(wheel+0x155,1);
    const std::uint32_t modes[]={1,2,3,4,99};const float ys[]={3,4.32f,4.8f,5.4f,3},zs[]={4,1.76f,1.4f,3.2f,4};
    for(unsigned i=0;i<5;++i){m.word(0x330cec,modes[i]);m.scalar(wheel+0x114,2);m.scalar(wheel+0x118,3);m.scalar(wheel+0x158,4);
        Sub_001A4EE0(m,wheel,0);check(close(m.scalar(wheel+0x120),2) && close(m.scalar(wheel+0x124),ys[i]) && close(m.scalar(wheel+0x128),zs[i]),"Finalizer original basis mode publication");}
    m.word(0x330cec,3);m.vector(wheel+0x140,{{1,0,0,0}});bool exceptional=false;
    try{Sub_001A4EE0(m,wheel,0);}catch(const hp2::NotImplemented&){exceptional=true;}
    check(exceptional,"Degenerate VU normalization remains explicit addressed gap");m.word(0x330cec,1);
    // Table family interpolation: index1.5 combines curve1 value2 and curve2 value4 =>3.
    m.word(0x330ce0,1);m.scalar(0x330ce4,1);m.scalar(0x330ce8,1);m.scalar(0x32e67c,1);
    for(unsigned i=0;i<7;++i){const auto v=m.allocate(8);m.scalar(v,float(i*2));m.scalar(v+4,float(i*2));const auto t=m.allocate(0x14);Sub_0018F808(m,t,v,2,0,10);m.word(0x3110b0+i*4,t);}
    check(close(Sub_001A4D28(m,1,0.25f,wheel),3),"Adjacent family curve blend");
    (void)table;
    // Complete wheel/contact wrapper with selected synthetic plane provider.
    PlaneContact provider;provider.height=0.5f;m.word(config+0x14,0);m.matrix(car+0x50,identity());
    m.vector(car+0x20,{{1,2,3,1}});m.vector(car+0x30,{{4,5,6,0}});m.vector(car+0x90,{{0,0,0,0}});
    m.vector(carconfig+0x120,{{0,0,0,1}});m.scalar(carconfig+0x138,0);
    Sub_001A4170(m,0,0,0.1f,wheel,car,0,0,provider);
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
    m.word(wheel+0x150,0);m.scalar(0x318c28,0.5f);m.scalar(0x32e64c,0.2f);m.scalar(0x32e650,0.4f);
    m.byte(wheel+0x154,1);m.word(wheel+0x10c,0);
    check(Sub_001A46B0(m,wheel,derivative)==0 && m.scalar(derivative)==2 && close(m.scalar(derivative+4),3.4f),"Wheel derivative operation grouping");
    m.byte(wheel+0x154,0);m.word(wheel+0x10c,1);m.scalar(wheel+0x160,4);
    m.scalar(0x32e654,0.001f);m.scalar(0x330cf8,0.01f);
    Sub_001A46B0(m,wheel,derivative);check(close(m.scalar(derivative+4),-200),"Correction follows lock zero store");
    check(Sub_001A63C0(m,wheel,derivative)==derivative && m.word(derivative)==0 && m.word(derivative+4)==0,"Controller derivative empty two-float state");
    std::cout<<"driving_tire_tests passed (instruction fixtures, no original equivalence)\n";
}


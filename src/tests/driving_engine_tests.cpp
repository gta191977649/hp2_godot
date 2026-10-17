#include "game_core/Sub_0018F808.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace port::driving;
namespace {
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
struct Fixture {
    Memory m;
    std::uint32_t ec,tc,engine,transmission,car,wheels,out;
    Fixture() {
        m.map(0x310000,0x30000);
        ec=m.allocate(0x60);tc=m.allocate(0x50);engine=m.allocate(0x60);
        transmission=m.allocate(0xa0);car=m.allocate(0x400);wheels=m.allocate(16);out=m.allocate(4);
        m.scalar(ec,100);m.scalar(ec+4,500);m.scalar(ec+8,900);m.scalar(ec+0xc,2);
        for(unsigned i=0;i<9;++i)m.scalar(ec+0x10+i*4,float(10+i*10));
        m.scalar(ec+0x34,2);m.scalar(ec+0x38,4);m.scalar(ec+0x3c,6);
        m.scalar(tc,0.5f);m.scalar(tc+8,2);m.scalar(tc+0xc,2);m.scalar(tc+0x10,0.5f);
        m.word(tc+0x18,5);m.scalar(tc+0x1c,0.25f);
        for(unsigned i=0;i<9;++i)m.scalar(tc+0x20+i*4,2);
        for(unsigned i=0;i<4;++i) {
            auto wheel=m.allocate(0x190);m.word(wheels+4*i,wheel);
            m.scalar(wheel+0x20,4);m.scalar(wheel+0x164,8);m.byte(wheel+0x155,1);
        }
        m.scalar(0x3308ac,0.75f);m.scalar(0x3308b0,0.5f);m.word(0x3308b4,0);
        m.scalar(0x32e328,1);m.scalar(0x32e32c,1);m.scalar(0x32e330,1);m.scalar(0x32e334,0.5f);
        m.scalar(0x3308a0,0.5f);m.scalar(0x3308a4,2);
        for(auto a:{0x32e33cu,0x32e344u,0x32e350u})m.scalar(a,0.25f);
        for(auto a:{0x32e340u,0x32e348u,0x32e354u})m.scalar(a,0.75f);
        m.scalar(0x32e34c,1);m.scalar(0x32e360,0.25f);m.scalar(0x32e364,0.5f);
        m.scalar(0x32e368,0.25f);m.scalar(0x32e36c,0.25f);
        m.scalar(0x32e37c,0.25f);m.scalar(0x32e380,1);m.scalar(0x32e384,0);m.scalar(0x3308c0,1);
        Sub_0018A7E8(m,engine,ec,car);Sub_0018AE70(m,transmission,tc,engine,wheels,car);
    }
};
}
int main() {
    check(cvt_w_s(1.75f)==1 && cvt_w_s(-1.75f)==-1,"CVT.W.S truncates both signs");
    for(float v:{std::numeric_limits<float>::infinity(),2147483648.0f}) {
        bool caught=false;try{(void)cvt_w_s(v);}catch(const hp2::NotImplemented&){caught=true;}
        check(caught,"Exceptional conversion must not silently produce an index");
    }
    Fixture f;auto& m=f.m;
    check(m.word(f.engine+8)==0x2b4618 && m.word(f.engine+0x18)==f.ec+0x10,"Engine constructor pointers");
    check(m.scalar(f.engine+0x58)==1 && m.scalar(f.engine+0x4c)==1,"Constructor trailing and multiplier stores");
    check(m.scalar(f.engine+0x28)==0.01f,"Nine sample scale");
    check(Sub_0018F838(m,f.engine+0x18,-1)==10,"Uniform lower clamp");
    check(Sub_0018F838(m,f.engine+0x18,200)==20,"Uniform exact interior sample");
    check(Sub_0018F838(m,f.engine+0x18,250)==25,"Uniform fractional interpolation");
    check(Sub_0018F838(m,f.engine+0x18,999)==90,"Uniform last index avoids following load");
    const auto table=m.allocate(20),samples=m.allocate(24);
    m.word(table,samples);m.word(table+4,3);
    m.scalar(samples,0);m.scalar(samples+4,10);m.scalar(samples+8,2);m.scalar(samples+12,20);
    m.scalar(samples+16,6);m.scalar(samples+20,0);m.scalar(0x32e3f0,0.01f);
    check(Sub_0018F9F0(m,table,-10)==10 && Sub_0018F9F0(m,table,9)==0,"Irregular endpoint clamps");
    check(Sub_0018F9F0(m,table,4)==10,"Irregular second interval");
    m.scalar(0x32e3f0,5);
    check(Sub_0018F9F0(m,table,2.5f)==10,"Irregular tiny width midpoint branch");
    m.word(table+4,1);check(Sub_0018F9F0(m,table,50)==10,"Irregular single sample");
    check(m.scalar(f.transmission+0x50+2*4)==300 && m.scalar(f.transmission+0x50+6*4)==900,"Static table generator and terminal slot");
    check(m.scalar(f.transmission+0x74+3*4)==150,"Down table writes index current plus one");
    check(m.word(f.transmission+0x18)==m.word(f.wheels) && m.word(f.transmission+0x24)==m.word(f.wheels+12),"Constructor four original pointer stores");
    // Dynamic scan constant torque: next ratio double produces a crossover at midpoint500.
    for(unsigned i=0;i<9;++i)m.scalar(f.ec+0x10+i*4,10);
    m.word(f.tc+0x18,2);m.scalar(f.tc+0x2c,4);m.word(0x3308b4,1);
    Sub_0018AC38(m,f.tc,f.ec,f.transmission+0x50,f.transmission+0x74,0.75f);
    check(m.scalar(f.transmission+0x58)==375 && m.scalar(f.transmission+0x80)==375,"Dynamic scan first crossover and ratios");
    m.scalar(f.tc+0x2c,2);
    Sub_0018AC38(m,f.tc,f.ec,f.transmission+0x50,f.transmission+0x74,0.75f);
    check(m.scalar(f.transmission+0x58)==525,"Dynamic scan no crossover falls back upper minus200");
    Fixture e;auto& em=e.m;em.scalar(e.engine+0xc,200);em.scalar(e.engine+0x44,0.5f);
    check(Sub_0018AA28(em,e.engine)==20,"Engine curve evaluation");
    check(Sub_0018AAB0(em,e.engine)==8.75f,"Engine power and drag grouping");
    em.word(e.engine+0x50,1);check(Sub_0018AAB0(em,e.engine)==28.75f,"Engine triple multiplier branch");
    em.word(e.engine+0x50,0);em.scalar(e.engine+0x44,0);em.word(e.car+0x39c,1);
    check(Sub_0018AAB0(em,e.engine)==-1.25f,"Negative result car flag scales");
    em.scalar(e.engine+0xc,800);check(Sub_0018AA28(em,e.engine)==-40,"Rev threshold equality triggers negative scale");
    em.scalar(e.engine+0xc,50);em.scalar(e.engine+0x48,99);Sub_0018A988(em,e.engine);
    check(em.scalar(e.engine+0xc)==100 && em.word(e.engine+0x48)==0,"Engine lower clamp and accumulator clear");
    em.scalar(e.engine+0xc,1000);Sub_0018A988(em,e.engine);check(em.scalar(e.engine+0xc)==900,"Engine upper clamp");
    em.scalar(e.engine+0xc,200);em.scalar(e.engine+0x44,0.5f);em.word(e.car+0x39c,0);em.scalar(e.engine+0x48,3.25f);
    check(Sub_0018A9D8(em,e.engine,e.out)==0 && em.scalar(e.out)==6,"Engine derivative sum then division");
    check(Sub_0018B008(em,e.transmission)==16 && Sub_0018B0D8(em,e.transmission)==8,"Drivetrain axle scalar paths");
    em.scalar(e.tc+0x10,0);check(Sub_0018B0D8(em,e.transmission)==4,"Front-only branch unconditionally halves");
    em.scalar(e.tc+0x10,0.5f);
    em.scalar(em.word(e.wheels)+0x20,1);em.scalar(em.word(e.wheels+4)+0x20,7);
    em.scalar(em.word(e.wheels+8)+0x20,1);em.scalar(em.word(e.wheels+12)+0x20,7);
    Sub_0018B178(em,e.transmission,24);
    for(unsigned i=0;i<4;++i)check(em.scalar(em.word(e.wheels+i*4)+0x20)==6,"Low speed averages then adjusts wheel values");
    // State transition sequence3 ->0 ->1 ->2 uses real memory fields and pending flag.
    em.scalar(e.engine+0x44,0.5f);em.scalar(e.engine+0xc,24);em.word(e.transmission+0x38,3);em.word(e.transmission+0x2c,3);
    Sub_0018B4A0(em,e.transmission);check(em.word(e.transmission+0x38)==0 && em.word(e.transmission+0x3c)==1,"Requested value mismatch enters state0");
    Sub_0018B4A0(em,e.transmission);check(em.word(e.transmission+0x38)==1 && em.scalar(e.engine+0x4c)==0,"State0 waits for coupling zero");
    Sub_0018B4A0(em,e.transmission);check(em.word(e.transmission+0x38)==2 && em.word(e.transmission+0x30)==3 && em.word(e.transmission+0x3c)==0,"State1 copies desired and consumes pending");
    em.scalar(0x310050+2*4,4);check(Sub_0018B698(em,e.transmission,e.out)==0 && em.scalar(e.out)==1,"State table derivative grouping");
    check(Sub_0018C368()==1 && Sub_0018C1C8()==1,"Exact scalar count leaves");
    std::cout<<"DRIVING_ENGINE_OK: bounded instruction fixtures, original validation deferred\n";
}


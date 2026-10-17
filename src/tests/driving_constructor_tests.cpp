#include "game_core/Sub_00137100_runtime.hpp"
#include <iostream>
#include <stdexcept>
using port::driving::Memory;
using namespace port::driving::native;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class Function>void rejects(Function run){bool rejected=false;try{run();}catch(const std::exception&){rejected=true;}check(rejected,"Expected a bounded memory rejection");}
}
int run(){
    Memory memory;const auto first=memory.allocate(32);memory.word(first,0x89abcdef);
    check(first==0x01000000&&memory.byte(first)==0xef&&memory.half(first+2)==0x89ab,"Arena uses bounded little-endian logical addresses");
    check(memory.signed_byte(first+3)==-119&&memory.signed_word(first)<0,"Signed loads preserve original bit patterns");
    auto stable=memory.view(first,32);const auto later=memory.allocate(64);stable.word(4,0x12345678);
    check(memory.word(first+4)==0x12345678&&later>first,"Existing byte views survive new region allocation");
    rejects([&]{memory.word(first+32,0xdeadbeef);});
    check(later>=first+48,"Logical component allocations retain an unmapped16-byte guard");
    rejects([&]{memory.map(first+4,4);});rejects([&]{memory.word(0);});rejects([&]{memory.word(first+31);});
    rejects([&]{memory.map(0xfffffff0,32);});rejects([&]{memory.allocate(4,3);});
    std::uint32_t scratch_address;
    {Memory::Scratch outer(memory);scratch_address=memory.allocate(16);memory.word(scratch_address,42);
        {Memory::Scratch inner(memory);const auto nested=memory.allocate(16);memory.word(nested,3);}
        check(memory.word(scratch_address)==42,"Nested scratch preserves enclosing allocations");
        memory.map(0x02000000,16);
    }
    rejects([&]{memory.word(scratch_address);});check(memory.word(0x02000000)==0,"Scratch does not erase explicit mapped regions");
    check(memory.allocate(16)==scratch_address,"Scratch restores allocation cursor");
    // E-0038-I14 independent intrusive list/count fixtures, including signed adjustor.
    memory.map(0x00300000,0x40000);memory.map(0x002b0000,0x10000);
    const auto head=memory.allocate(16),body=memory.allocate(0x180),wheel=memory.allocate(0x184);
    port::driving::init::Sub_0019C170(memory.view(head,16),head);
    memory.word(body+8,0x002b5430);memory.word(0x002b5444,0x0019dcc8);memory.half(0x002b5440,0xfff8);
    memory.word(wheel+8,0x002b5c40);memory.word(0x002b5c54,0x001a5c28);
    Sub_0019C238(memory,head,body);Sub_0019C238(memory,head,wheel);
    check(memory.word(head)==body&&memory.word(head+4)==wheel&&memory.word(body)==wheel&&memory.word(body+4)==head&&memory.word(wheel)==head&&memory.word(wheel+4)==body,"Intrusive append exact forward/back links");
    check(memory.word(head+8)==34,"Body count32 plus wheel count2 from original return-delay bytes");
    const auto helper=memory.allocate(0x18),helper_config=memory.allocate(16);
    memory.word(helper,helper_config);memory.scalar(helper_config+4,2);memory.scalar(helper_config+8,3);
    check(Sub_001ADFB8(memory,helper,0.25f)==1.25f&&Sub_001ADFB8(memory,helper,-2)==0,"001ADFB8 max.S has no integer truncation");
    // E-0038-I17 instruction-derived unequal axle split and all-four-lane projection.
    const auto mover=memory.allocate(0x80),config=memory.allocate(0x560),car=memory.allocate(0x900),controls=memory.allocate(0x240),flags=memory.allocate(0x20);
    memory.word(mover+0x18,body);memory.word(mover+0x4c,config);memory.word(mover+0x54,car);memory.word(mover+0x48,helper);
    memory.word(body+0xc,config+0x1e0);memory.scalar(config+0x1e0,4);
    memory.matrix(config+0x1f0,port::driving::init::Sub_0025AC68());
    memory.vector(config+0x110,{{0,1,2,3}});memory.vector(config+0x120,{{3,5,7,9}});memory.scalar(config+0x180,-1);
    memory.word(helper,config+0x2f0);memory.scalar(config+0x2f4,0.25f);memory.scalar(config+0x2f8,0.75f);memory.scalar(config+0x2fc,3);memory.scalar(car+0x3c0,2);
    memory.word(0x0032fff4,1); // gp-0x58FC, enable config+0x110 subtraction.
    memory.scalar(0x0032dcc0,10); // gp-0x7C30.
    memory.word(0x00330ca0,0x7fc12345);
    std::array<std::uint32_t,4> controller{};
    for(unsigned i=0;i<4;++i){controller[i]=memory.allocate(0xa0);const auto w=memory.allocate(0x184);const auto cfg=memory.allocate(16);memory.vector(cfg,{{float(i+3),5,7,9}});
        memory.word(mover+0x20+4*i,controller[i]);memory.word(mover+0x30+4*i,w);memory.word(controller[i]+0x90,w);memory.word(controller[i]+0x94,body);memory.word(w+0xc,cfg);}
    Sub_001375F0(memory,mover);
    check(memory.scalar(controller[0]+0x78)==5.75f&&memory.scalar(controller[1]+0x78)==5.75f&&memory.scalar(controller[2]+0x78)==17.25f&&memory.scalar(controller[3]+0x78)==17.25f,"Recovered unequal first/second axle split");
    check(memory.vector(controller[0]+0x80)==std::array<float,4>{{3,4,5,6}},"Offset-vector difference includes fourth lane");
    for(unsigned i=0;i<4;++i)check(memory.word(memory.word(mover+0x30+4*i)+0x24)==0x7fc12345,"Four raw global publications preserve payload bits");
    // E-0038-I23/I24: time sentinel, exact equality and throttle early gate.
    memory.word(mover,controls);memory.word(car+0x34,flags);memory.word(car+0x3bc,1);
    memory.scalar(0x0032dcc4,0.5f);memory.scalar(0x0032dcc8,2);
    memory.scalar(controls+0x214,0.5f);memory.vector(body+0x90,{{0.25f,0,0,77}});memory.vector(body+0xa0,{{0,0,0,88}});
    check(Sub_001377D8(memory,mover)==2,"Equality at throttle and0.25 vector length admits state2");
    memory.vector(body+0x90,{{0.5f,0,0,77}});check(Sub_001377D8(memory,mover)==1,"Bounded nonzero motion admits state1");
    memory.scalar(controls+0x214,0.5001f);check(Sub_001377D8(memory,mover)==0,"Strict throttle threshold gate");
    memory.word(car+0x860,0x7fffffff);memory.scalar(car+0x864,100);check(!Sub_00133130(memory,car),"Time sentinel suppresses predicate");
    memory.word(car+0x860,0xfffffff0);memory.word(0x00330c78,0x10);memory.scalar(0x0032dc34,0.5f);memory.scalar(car+0x864,17);
    check(Sub_00133130(memory,car),"32bit clock subtraction wraps before signed float conversion");
    check(Sub_001A63D0({{1,2,3,4}},{{0,0,2,5}},{{3,0,0,7}})==std::array<float,4>{{1,8,3,9}},"Velocity helper cross retains angular W then adds all linear lanes");
    // Full call/store-graph fixture. The synthetic table/config values are only
    // an ownership/order test; they are not a replacement vehicle setup.
    memory.map(0x002ae000,0x1000);
    memory.map(0x002f94f0,0x80); // Synthetic zero roughness samples for graph fixture.
    struct Table {std::uint32_t address,count,reset;};
    for(const auto& entry:std::array<Table,5>{{{0x002b5430,0x0019dcc8,0x0019d848},{0x002b5c40,0x001a5c28,0x001a4358},
            {0x002b5cd0,0x001a6660,0x001a5fc0},{0x002b4618,0x0018c368,0x0018a988},{0x002b45d0,0x0018c1c8,0x0018b4a0}}}){
        memory.half(entry.address+0x10,0);memory.word(entry.address+0x14,entry.count);memory.half(entry.address+0x40,0);memory.word(entry.address+0x44,entry.reset);
    }
    memory.half(0x002aeac0,0xfff8);memory.word(0x002aeac4,0x00138e08);
    memory.scalar(config+0x2b0,1);memory.scalar(config+0x2b4,5);memory.scalar(config+0x2b8,9);memory.scalar(config+0x2bc,1);
    for(unsigned i=0;i<12;++i)memory.scalar(config+0x2c0+4*i,12);
    memory.word(config+0x288,2);for(unsigned i=0;i<4;++i)memory.scalar(config+0x290+4*i,1);
    memory.scalar(config+0x244,0.5f);memory.scalar(config+0x248,-0.5f);memory.scalar(config+0x264,0.5f);memory.scalar(config+0x268,-0.5f);
    memory.matrix(car+0x90,port::driving::init::Sub_0025AC68());memory.word(car+0x3c8,2);memory.scalar(car+0x3d0,3);
    memory.scalar(0x00331124,100);memory.scalar(0x0032dd3c,1);memory.scalar(0x0032e328,1);
    memory.vector(0x00320a60,{{0,0,0,0}});memory.scalar(controls+0x214,0);
    port::driving::PlaneContact plane;InitServices services;services.contact=&plane;services.Field_00137938_stack_w=0;
    unsigned clock_calls=0,delta_calls=0,final_calls=0;
    services.Sub_00250F38=[&](){return std::uint64_t(++clock_calls);};
    services.Sub_00250F48=[&](std::uint64_t a,std::uint64_t b){++delta_calls;check(b==a+1,"Performance counter pair ordering");return 77u;};
    services.ResolveForces=[&](Memory&,std::uint32_t a){check(a==mover,"Final virtual adjustor resolves mover address");++final_calls;};
    check(Sub_00137100(memory,mover,config,controls,car,services)==mover,"Constructor returns the original this address");
    check(memory.word(mover+0x10)==50,"Complete component graph has32+8*2+1+1 derivative words");
    check(final_calls==1&&clock_calls==4&&delta_calls==2,"Constructor resets initial graph then post-state graph before final sync");
    check(memory.word(mover+0x60)==1&&memory.word(mover+0x64)==1&&memory.word(mover+0x68)==2&&memory.scalar(mover+0x74)==1,"Final constructor stores occur after final synchronization");
    const auto body2=memory.word(mover+0x18),engine=memory.word(mover+0x40),drive=memory.word(mover+0x44);
    check(memory.word(body2+0xc)==config+0x1e0&&memory.word(engine+0x10)==config+0x2b0&&memory.word(drive+0x10)==config+0x270,"Full graph preserves exact configuration slices");
    for(unsigned i=0;i<4;++i){const auto a=memory.word(mover+0x20+4*i);check(memory.word(a+0x10)==memory.word(mover+0x20+4*(i^1)),"Four controller sibling pointers pair0/1 and2/3");}
    for(unsigned i=0;i<4;++i){
        const auto a=memory.word(mover+0x20+4*i),next=memory.word(a),old=memory.word(next);
        memory.scalar(a+0xa0,3.5f);
        check(memory.word(next)==old,"Controller+0xA0 force-response field cannot overwrite following list node");
    }
    std::cout<<"Driving constructor and logical-memory fixtures passed; original equivalence remains unverified.\n";
    return 0;
}
int main(){try{return run();}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

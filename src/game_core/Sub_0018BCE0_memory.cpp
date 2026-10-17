// TASK-0041 E-0041-R02; retail 0x0018BCE0: DriveTrain::SetDesiredGear(Gear).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#include "Sub_0018BCE0_memory.hpp"
#include "Sub_0018BCE0.hpp"
#include "Sub_0018F808.hpp"
namespace port::driving::native {
namespace {
DrivetrainGlobals globals(Memory& m) {
    DrivetrainGlobals g;
    g.Field_0032e358=m.scalar(0x0032e358);g.Field_0032e35c=m.scalar(0x0032e35c);g.Field_0032e370=m.scalar(0x0032e370);
    g.Field_0032e374=m.scalar(0x0032e374);g.Field_0032e378=m.scalar(0x0032e378);g.Field_0032e388=m.scalar(0x0032e388);
    g.Field_0032e38c=m.scalar(0x0032e38c);g.Field_0032e390=m.scalar(0x0032e390);g.Field_0032e394=m.scalar(0x0032e394);
    g.Field_003308b8=m.scalar(0x003308b8);g.Field_003308bc=m.scalar(0x003308bc);g.Field_003308c4=m.scalar(0x003308c4);
    g.Field_00330c88=m.scalar(0x00330c88);return g;
}
DrivetrainProjection load(Memory& m,std::uint32_t a) {
    DrivetrainProjection s;
    s.Field_0xc=m.scalar(a+0xc);s.Field_0x2c=m.signed_word(a+0x2c);s.Field_0x30=m.signed_word(a+0x30);
    s.Field_0x34=m.signed_word(a+0x34);s.Field_0x38=m.signed_word(a+0x38);s.Field_0x40=m.scalar(a+0x40);
    s.Field_0x44=m.scalar(a+0x44);s.Field_0x48=m.signed_word(a+0x48);s.Field_0x4c=m.scalar(a+0x4c);
    const auto config=m.word(a+0x10),engine=m.word(a+0x14),ec=m.word(engine+0x10),car=m.word(a+0x28);
    s.config_Field_0x8=m.scalar(config+8);s.config_Field_0xc=m.scalar(config+0xc);s.config_Field_0x10=m.scalar(config+0x10);s.config_Field_0x14=m.scalar(config+0x14);s.config_Field_0x18=m.signed_word(config+0x18);
    for(unsigned i=0;i<9;++i){s.config_Field_0x20[i]=m.scalar(config+0x20+i*4);s.Field_0x50[i]=m.scalar(a+0x50+i*4);s.Field_0x74[i]=m.scalar(a+0x74+i*4);}
    s.engine_Field_0xc=m.scalar(engine+0xc);s.engine_Field_0x44=m.scalar(engine+0x44);s.engine_Field_0x48=m.scalar(engine+0x48);
    s.engine_config_Field_0x0=m.scalar(ec);s.engine_config_Field_0x8=m.scalar(ec+8);s.engine_config_Field_0xc=m.scalar(ec+0xc);s.car_Field_0x3a4=m.signed_word(car+0x3a4);
    for(unsigned i=0;i<4;++i){const auto w=m.word(a+0x18+i*4);auto& p=s.wheels[i];p.Field_0x20=m.scalar(w+0x20);p.Field_0x24=m.scalar(w+0x24);p.Field_0x108=m.scalar(w+0x108);p.Field_0x114=m.scalar(w+0x114);p.Field_0x10c=m.signed_word(w+0x10c);p.Field_0x155=m.signed_byte(w+0x155);p.Field_0x160=m.scalar(w+0x160);p.config_Field_0xc=m.scalar(m.word(w+0xc)+0xc);}
    return s;
}
void store(Memory& m,std::uint32_t a,const DrivetrainProjection& s) {
    m.word(a+0x2c,static_cast<std::uint32_t>(s.Field_0x2c));m.word(a+0x34,static_cast<std::uint32_t>(s.Field_0x34));
    m.scalar(a+0x44,s.Field_0x44);m.word(a+0x48,static_cast<std::uint32_t>(s.Field_0x48));m.scalar(a+0x4c,s.Field_0x4c);
    m.scalar(m.word(a+0x14)+0x48,s.engine_Field_0x48);
    for(unsigned i=0;i<4;++i)m.scalar(m.word(a+0x18+i*4)+0x108,s.wheels[i].Field_0x108);
}
}
std::int32_t Sub_0018BCA0(Memory& m,std::uint32_t a,std::int32_t request){auto s=load(m,a);const auto r=port::driving::Sub_0018BCA0(s,request);store(m,a,s);return r;}
std::int32_t SetDesiredGear(Memory& m,std::uint32_t a,std::int32_t request){auto s=load(m,a);const auto r=port::driving::SetDesiredGear(s,request,globals(m));store(m,a,s);return r;}
void Sub_0018ABF0(Memory& m,std::uint32_t a){EngineTimerProjection s{m.signed_word(a+0x50),m.scalar(a+0x54)};port::driving::Sub_0018ABF0(s,m.signed_word(0x00330c78),m.scalar(0x0032e338));m.word(a+0x50,static_cast<std::uint32_t>(s.Field_0x50));}
void Sub_0018B948(Memory& m,std::uint32_t a){auto s=load(m,a);port::driving::Sub_0018B948(s,globals(m),[&]{return port::driving::Sub_0018AAB0(m,m.word(a+0x14));});store(m,a,s);}
void Sub_0018BAA0(Memory& m,std::uint32_t a){auto s=load(m,a);port::driving::Sub_0018BAA0(s,globals(m),[&]{return port::driving::Sub_0018AAB0(m,m.word(a+0x14));});store(m,a,s);}
}

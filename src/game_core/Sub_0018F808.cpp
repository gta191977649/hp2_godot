#include "Sub_0018F808.hpp"
#include "port/not_implemented.hpp"
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace port::driving {
namespace {
std::int32_t signed_bits(std::uint32_t v) { std::int32_t s; std::memcpy(&s,&v,4); return s; }
// Shared port-side implementation carrier for the instruction sequence0018F838.
template<class Read> float interpolate(float value,float lower,float upper,float step,
                                      std::uint32_t count,Read read) {
    if(value<lower) value=lower;
    else if(upper<value) value=upper;
    float f12=value-lower; f12=f12*step;
    const auto i=cvt_w_s(f12);
    if(i==signed_bits(count-1)) return read(i);
    const float f2=read(i);
    float f0=read(i+1)-f2;
    const float f1=f12-static_cast<float>(i);
    f0=f0*f1; return f2+f0;
}
float ratio(Memory& m,std::uint32_t s) {
    return m.scalar(m.word(s+0x10)+0x20+m.word(s+0x30)*4);
}
}
std::int32_t cvt_w_s(float f) {
    if(!std::isfinite(f) || static_cast<double>(f)<-2147483648.0 || static_cast<double>(f)>=2147483648.0)
        hp2::not_implemented("0x0018F878","CVT.W.S exceptional or out-of-range operand");
    return static_cast<std::int32_t>(f);
}
// ELF00189168; E-0038-E01. Integer decrement wraps, followed by signed CVT.S.W.
void Sub_00189168(Memory& m,std::uint32_t s) {
    const float numerator=static_cast<float>(signed_bits(m.word(s+4)-1));
    const float denominator=m.scalar(s+0xc)-m.scalar(s+8);
    m.scalar(s+0x10,numerator/denominator);
}
// ELF00189120; E-0038-E01.
void Sub_00189120(Memory& m,std::uint32_t s,float f12,float f13) {
    m.scalar(s+8,f12);m.scalar(s+0xc,f13);Sub_00189168(m,s);
}
// ELF0018F808; E-0038-E01.
void Sub_0018F808(Memory& m,std::uint32_t s,std::uint32_t values,std::uint32_t count,float f12,float f13) {
    m.word(s,values);m.word(s+4,count);Sub_00189120(m,s,f12,f13);
}
// ELF0018F838..0018F8CC; E-0038-E01.
float Sub_0018F838(Memory& m,std::uint32_t s,float value) {
    const auto data=m.word(s), count=m.word(s+4);
    return interpolate(value,m.scalar(s+8),m.scalar(s+0xc),m.scalar(s+0x10),count,
        [&](std::int32_t i){
            if(i<0 || static_cast<std::uint32_t>(i)>=count)
                throw std::out_of_range("Interpolation index outside mapped table");
            return m.scalar(data+static_cast<std::uint32_t>(i)*4);
        });
}
// ELF0018F9F0; E-0038-E01. Interval tests and tiny-width midpoint preserved.
float Sub_0018F9F0(Memory& m,std::uint32_t s,float value) {
    const auto count=m.signed_word(s+4);const auto data=m.word(s);
    if(count<2) return m.scalar(data+4);
    if(value<=m.scalar(data)) return m.scalar(data+4);
    const auto last=data+static_cast<std::uint32_t>(count)*8;
    if(m.scalar(last-8)<=value) return m.scalar(last-4);
    const float epsilon=m.scalar(0x32e3f0);
    for(std::int32_t i=0;i<count-1;++i) {
        const auto at=data+static_cast<std::uint32_t>(i)*8;
        const float f4=m.scalar(at),next=m.scalar(at+8);
        if(f4<=value && value<next) {
            const float width=next-f4,base=m.scalar(at+4),delta=m.scalar(at+12)-base;
            if(!(epsilon<std::fabs(width))) return base+delta*0.5f;
            float f0=value-f4;f0=f0/width;f0=f0*delta;return base+f0;
        }
    }
    return m.scalar(data+4);
}
// ELF0018AC38; E-0038-E02. Original stack table remains a local value carrier.
void Sub_0018AC38(Memory& m,std::uint32_t config,std::uint32_t engineconfig,
                  std::uint32_t out50,std::uint32_t out74,float f26) {
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
        if(m.word(0x3308b4)==0) {
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
                if(next==f21) hp2::not_implemented("0x0018AD74","non-progressing exceptional table scan");
                f21=next;
            }
            if(pending) m.scalar(out50+off,upper-200.0f);
            f0=m.scalar(out50+off)*f26;
        }
        m.scalar(out50+off,f0);
        f0=m.scalar(0x3308b0)*m.scalar(out50+off);f0=f0*f23;f0=f0/f24;
        m.scalar(out74+off+4,f0);
        ++current;
    }
    m.scalar(out50+static_cast<std::uint32_t>(current)*4,upper);
}
// ELF0018A7E8; E-0038-E03. Writes+58 despite allocator label size elsewhere.
void Sub_0018A7E8(Memory& m,std::uint32_t s,std::uint32_t config,std::uint32_t car) {
    m.word(s+8,0x2b4618);
    Sub_0018F808(m,s+0x18,config+0x10,9,m.scalar(config),m.scalar(config+8));
    Sub_0018F808(m,s+0x2c,config+0x34,3,m.scalar(config),m.scalar(config+8));
    m.word(s+0x10,config);m.word(s+0x14,car);m.scalar(s+0x58,1);
    m.word(s+0x40,0);m.word(s+0x50,0);m.word(s+0x54,0);m.scalar(s+0x4c,1);
}
// ELF0018AE70; E-0038-E03.
void Sub_0018AE70(Memory& m,std::uint32_t s,std::uint32_t config,std::uint32_t engine,
                  std::uint32_t wheels,std::uint32_t car) {
    m.word(s+0x14,engine);const auto engineconfig=m.word(engine+0x10);
    m.word(s+8,0x2b45d0);m.word(s+0x10,config);
    for(std::uint32_t i=0;i<4;++i) m.word(s+0x18+i*4,m.word(wheels+i*4));
    m.word(s+0x28,car);
    Sub_0018AC38(m,config,engineconfig,s+0x50,s+0x74,m.scalar(0x3308ac));
    m.word(s+0x2c,2);m.word(s+0xc,0);m.word(s+0x38,2);m.word(s+0x30,2);
    m.word(s+0x3c,0);m.word(s+0x4c,0);
}
// ELF0018AA28; E-0038-E04. Original multiply grouping differs from decompile.
float Sub_0018AA28(Memory& m,std::uint32_t s) {
    const float f20=m.scalar(s+0xc)*m.scalar(0x32e32c);
    float result=Sub_0018F838(m,s+0x18,f20)*m.scalar(s+0x58);
    if(m.word(s+0x40)!=0) result=result*m.scalar(0x3308a4);
    const float f0=m.scalar(m.word(s+0x10)+8)-100.0f;
    if(!(f20<f0)) result=(-m.scalar(0x3308a0))*result;
    return result;
}
// ELF0018AAB0; E-0038-E04.
float Sub_0018AAB0(Memory& m,std::uint32_t s) {
    float f20=Sub_0018AA28(m,s);
    float f1=m.scalar(s+0x44)*m.scalar(s+0x4c);f20=f20*f1;
    if(m.word(s+0x50)!=0) f20=f20*3.0f;
    float f0=m.scalar(s+0xc)*m.scalar(0x32e330);
    f0= -Sub_0018F838(m,s+0x2c,f0);
    float f2=m.scalar(s+0x44)*m.scalar(s+0x4c);
    f1=1.0f-f2;f0=f0*f1;f1=f20+f0;
    if(m.word(m.word(s+0x14)+0x39c)==1 && f1<0.0f) f1=f1*m.scalar(0x32e334);
    return f1;
}
// ELF0018A988; E-0038-E04. Bytes at this entry are not analyzed in Ghidra;
// raw listing words establish the sequential lower/upper clamping and+48 clear.
void Sub_0018A988(Memory& m,std::uint32_t s) {
    const auto config=m.word(s+0x10);const float scale=m.scalar(0x32e328);
    float value=m.scalar(s+0xc);float bound=m.scalar(config)*scale;
    if(value<bound) { m.scalar(s+0xc,bound);value=bound; }
    bound=m.scalar(config+8)*scale;
    if(bound<value) m.scalar(s+0xc,bound);
    m.word(s+0x48,0);
}
// ELF0018A9D8; E-0038-E04.
std::uint32_t Sub_0018A9D8(Memory& m,std::uint32_t s,std::uint32_t out) {
    float f0=Sub_0018AAB0(m,s);f0=f0+m.scalar(s+0x48);
    f0=f0/m.scalar(m.word(s+0x10)+0xc);m.scalar(out,f0);return 0;
}
// ELF0018B008; E-0038-E05.
float Sub_0018B008(Memory& m,std::uint32_t s) {
    const auto config=m.word(s+0x10);const float f4=m.scalar(config+0x10);
    float f3=0;
    if(m.scalar(0x32e33c)<=f4) {
        const auto a=m.word(s+0x20),b=m.word(s+0x24);
        float f0=m.scalar(a+0x20)+m.scalar(b+0x20);f0=f0*0.5f;f3=f0*m.scalar(config+0xc);
    }
    if(f4<=m.scalar(0x32e340)) {
        const auto a=m.word(s+0x18),b=m.word(s+0x1c);
        float f0=m.scalar(a+0x20)+m.scalar(b+0x20);f0=f0*0.5f;f0=f0*m.scalar(config+8);f3=f3+f0;
        if(m.scalar(0x32e33c)<=f4) f3=f3*0.5f;
    }
    return f3*ratio(m,s);
}
// ELF0018B0D8; E-0038-E05. Front path halves even when rear path was skipped.
float Sub_0018B0D8(Memory& m,std::uint32_t s) {
    const float f4=m.scalar(m.word(s+0x10)+0x10);float f3=0;
    if(m.scalar(0x32e344)<=f4) {
        f3=m.scalar(m.word(s+0x20)+0x164)+m.scalar(m.word(s+0x24)+0x164);f3=f3*0.5f;
    }
    if(f4<=m.scalar(0x32e348)) {
        float f0=m.scalar(m.word(s+0x18)+0x164)+m.scalar(m.word(s+0x1c)+0x164);
        f0=f0*0.5f;f0=f3+f0;f3=f0*0.5f;
    }
    return f3;
}
// ELF0018B178 and001A4168 single +20 store; E-0038-E05.
void Sub_0018B178(Memory& m,std::uint32_t s,float value) {
    const float f23=value-Sub_0018B008(m,s);
    const float f22=m.scalar(m.word(s+0x28)+0x384)*m.scalar(0x32e34c);
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
    if(m.scalar(0x32e350)<=m.scalar(config+0x10)) {
        axle(0x20,m.scalar(config+0xc));config=m.word(s+0x10);
    }
    if(m.scalar(config+0x10)<=m.scalar(0x32e354)) axle(0x18,m.scalar(config+8));
}
// ELF0018BC18; E-0038-E05.
std::uint32_t Sub_0018BC18(Memory& m,std::uint32_t s) {
    const auto engine=m.word(s+0x14);
    if(!(m.scalar(engine+0x44)<m.scalar(0x32e37c))) return 0;
    float f0=m.scalar(m.word(engine+0x10))*m.scalar(0x32e380);f0=f0+m.scalar(0x32e384);
    return m.scalar(s+0x40)<=f0 ? 1:0;
}
// ELF0018BC68; E-0038-E05.
std::uint32_t Sub_0018BC68(Memory& m,std::uint32_t s) {
    float f0=m.scalar(m.word(s+0x14)+0xc)-m.scalar(s+0x40);f0=std::fabs(f0);
    return f0<=m.scalar(0x3308c0) ? 1:0;
}
// ELF0018B4A0; E-0038-E05.
void Sub_0018B4A0(Memory& m,std::uint32_t s) {
    m.scalar(s+0x40,Sub_0018B008(m,s));
    const auto state=m.signed_word(s+0x38);const auto engine=m.word(s+0x14);
    if(state==1) {
        const auto pending=m.word(s+0x3c);m.word(s+0x30,m.word(s+0x2c));m.word(engine+0x4c,0);
        if(pending!=0 || m.scalar(0x32e36c)<m.scalar(engine+0x44)) {
            m.word(s+0x3c,0);m.word(s+0x38,2);
        }
    } else if(state==0) {
        m.word(engine+0x4c,0);const float f1=m.scalar(engine+0x4c);
        if(m.scalar(s+0xc)<=f1) {m.word(s+0x38,1);m.scalar(s+0xc,f1);}
    } else if(state==2) {
        m.word(engine+0x4c,m.word(0x32e368));
        if(1.0f<=m.scalar(s+0xc)) {
            m.scalar(s+0xc,1);
            if(Sub_0018BC68(m,s)!=0 || m.word(s+0x30)==1) m.word(s+0x38,3);
        }
        if(Sub_0018BC18(m,s)!=0) m.word(s+0x38,0);
    } else if(state==3) {
        m.scalar(engine+0x4c,1);auto current=m.word(s+0x30);
        if(current!=1) {
            const float f20=m.scalar(s+0x40)-m.scalar(engine+0xc);
            const float f2=Sub_0018B0D8(m,s);
            if(m.scalar(0x32e360)<m.scalar(engine+0x44) && 0.0f<f2) {
                const float f0=m.scalar(engine+0xc);m.scalar(s+0x40,f0);Sub_0018B178(m,s,f0);
            } else {
                float f0=f20*m.scalar(0x32e364);f0=m.scalar(engine+0xc)+f0;m.scalar(engine+0xc,f0);
            }
            if(Sub_0018BC18(m,s)!=0) m.word(s+0x38,0);
            current=m.word(s+0x30);
        }
        if(current!=m.word(s+0x2c)) {m.word(s+0x38,0);m.word(s+0x3c,1);}
    }
}
// ELF0018B698; E-0038-E05, raw listing words at an unanalyzed entry.
std::uint32_t Sub_0018B698(Memory& m,std::uint32_t s,std::uint32_t out) {
    const auto at=0x310050u+m.word(s+0x38)*4;
    const float value=m.scalar(m.word(s+0x10)+0x1c)*m.scalar(at);
    m.scalar(out,value);return 0;
}
// ELF0018C368/0018C1C8; E-0038-E06, jr ra / li v0,1.
std::uint32_t Sub_0018C368(){return 1;}
std::uint32_t Sub_0018C1C8(){return 1;}
}


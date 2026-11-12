// TASK-0044 E-0044-E01: Debug tables; full query manifest in local/task0044/engine.
#include "game_core/port/driving/projection_support.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/table.hpp"
namespace port::driving {
void Sub_0018F808(Memory& m,std::uint32_t s,std::uint32_t data,std::uint32_t count,float lo,float hi) {hp2::recovered::Table(m,s).Sub_0018D810(data,count,lo,hi);}
float Table::GetValue(Memory& m,std::uint32_t s,float x) {return hp2::recovered::Table(m,s).GetValue(x);}
float Graph::GetValue(Memory& m,std::uint32_t s,float x) {return hp2::recovered::Graph(m,s).GetValue(x);}
}
namespace hp2::recovered {
void Table::Sub_0018D810(std::uint32_t values,std::uint32_t count,float lower,float upper) {
    m_.word(address_,values);m_.word(address_+4,count);SetMinMax(lower,upper);
}
float Table::GetValue(float value) {
    using namespace port::driving::detail_group_0018f808;
    const auto data=m_.word(address_),count=m_.word(address_+4);
    return interpolate(value,m_.scalar(address_+8),m_.scalar(address_+0xc),m_.scalar(address_+0x10),count,
      [&](std::int32_t i){if(i<0 || static_cast<std::uint32_t>(i)>=count) throw std::out_of_range("Interpolation index outside mapped table");return m_.scalar(data+static_cast<std::uint32_t>(i)*4);});
}
void Graph::Sub_0018D9E8(std::uint32_t points,std::uint32_t count) {m_.word(address_,points);m_.word(address_+4,count);}
float Graph::GetValue(float value) {
    const auto count=m_.signed_word(address_+4);const auto data=m_.word(address_);
    if(count<2 || value<=m_.scalar(data)) return m_.scalar(data+4);
    const auto last=data+static_cast<std::uint32_t>(count)*8;
    if(m_.scalar(last-8)<=value) return m_.scalar(last-4);
    const float epsilon=m_.scalar(0x3254e0);
    for(std::int32_t i=0;i<count-1;++i) {
        const auto at=data+static_cast<std::uint32_t>(i)*8;
        const float start=m_.scalar(at),next=m_.scalar(at+8);
        if(start<=value && value<next) {
            const float width=next-start,base=m_.scalar(at+4),delta=m_.scalar(at+12)-base;
            if(!(epsilon<std::fabs(width))) return base+delta*0.5f;
            float fraction=value-start;fraction=fraction/width;fraction=fraction*delta;return base+fraction;
        }
    }
    return m_.scalar(data+4);
}
}
#include "game_core/indep/src/table.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0018FCB8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0018fcb8.md.
void Average::Record() {
    hp2::not_implemented("0x0018FCB8", "Average::Record (signature and full body unresolved)");
}

// ELF 0x0018FD50; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0018fd50.md.
void Average::Reset() {
    hp2::not_implemented("0x0018FD50", "Average::Reset (signature and full body unresolved)");
}

// ELF 0x0018FDA0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0018fda0.md.
void Average::Flush() {
    hp2::not_implemented("0x0018FDA0", "Average::Flush (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0018F9F0; E-0042-C077; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018f9f0.md.

} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00190050; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00190050.md.
void Linear::Init() {
    hp2::not_implemented("0x00190050", "Linear::Init (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/Table.hpp"


namespace hp2::recovered {
float Table::InverseLookup(float value) {
 const auto data=m_.word(address_);const auto count=m_.signed_word(address_+4);
 float previous=m_.scalar(data);
 if(std::fabs(value-previous)<m_.scalar(0x3254d8)) return previous;
 bool side=previous<value;
 for(std::int32_t i=1;i<count;++i) {
  const float current=m_.scalar(data+static_cast<std::uint32_t>(i)*4);
  float index;
  if(std::fabs(value-current)<m_.scalar(0x3254dc)) index=static_cast<float>(i);
  else if((current<value)!=side) index=static_cast<float>(i-1)+(value-previous)/(current-previous);
  else {previous=current;side=current<value;continue;}
  return m_.scalar(address_+8)+index/m_.scalar(address_+0x10);
 }
 return m_.scalar(address_+8)+static_cast<float>(count-1)/m_.scalar(address_+0x10);
}
float Graph::GetInverse(float value) {
 const auto data=m_.word(address_);const auto count=m_.signed_word(address_+4);
 if(count<2) return m_.scalar(data+4);
 if(value<=m_.scalar(data+4)) return m_.scalar(data);
 const auto last=data+static_cast<std::uint32_t>(count-1)*8;
 if(m_.scalar(last+4)<=value) return m_.scalar(last);
 for(std::int32_t i=0;i<count-1;++i) {
  const auto at=data+static_cast<std::uint32_t>(i)*8;
  const float start=m_.scalar(at+4),next=m_.scalar(at+12);
  if(start<=value && value<next) {
   const float width=next-start,base=m_.scalar(at),delta=m_.scalar(at+8)-base;
   if(std::fabs(width)<=m_.scalar(0x3254e4)) return base+delta*0.5f;
   return base+((value-start)/width)*delta;
  }
 }
 return m_.scalar(data+4);
}
}


#include "game_core/port/race/control_construction.hpp"
#include <stdexcept>
namespace hp2::debug {
// Debug0018DBF8; E-0099-W24; original table.cpp. Padding+3 is untouched.
void Average::Sub_0018DBF8(int n){
 if(n<=0||static_cast<unsigned>(n)>0x3fffffffu)throw std::invalid_argument("Average requires a positive representable sample allocation");
 auto& m=m_;const auto a=address_;
 m.byte(a+1,0);m.byte(a+2,0);m.word(a+4,0);m.word(a+8,0);m.byte(a,static_cast<std::uint8_t>(n));
 const auto data=m.allocate(static_cast<std::uint32_t>(n)*4);m.word(a+12,data);
 for(int i=0;i<n;++i)m.word(data+static_cast<std::uint32_t>(i)*4,0);
}
// Debug0018DD58; E-0099-W24. Does not reset CurrentSlot or fAverage.
void Average::Reset(float value){
 auto& m=m_;const auto a=address_;const auto count=m.byte(a);
 for(unsigned i=0;i<count;++i)m.scalar(m.word(a+12)+i*4,value);
 m.byte(a+2,0);m.scalar(a+4,value*static_cast<float>(count));
}
// Debug0018E010; E-0099-W24.
void PidError::Reset(float calibration){
 m_.word(address_+0x20,0);m_.scalar(address_+0x24,calibration);
 Average(m_,address_).Reset(0);Average(m_,address_+0x10).Reset(0);
}
}


namespace hp2::debug {
// Debug0018DC70; E-0099-W24. Does not null pData or reset counters.
void Average::Sub_0018DC70(int flags){if(const auto data=m_.word(address_+12))m_.release(data);if(flags&1)m_.release(address_);}
}

namespace hp2::debug {
// Debug0018DCC0; E-0099-W47. Original byte ring indices, ordered total update.
void Average::Record(float value){auto& m=m_;const auto a=address_;if(m.byte(a+2)<m.byte(a))m.byte(a+2,m.byte(a+2)+1);const float total=m.scalar(a+4)+value;m.scalar(a+4,total);const auto entry=m.word(a+12)+4*m.byte(a+1);m.scalar(a+4,total-m.scalar(entry));m.scalar(entry,value);const auto next=static_cast<std::uint8_t>(m.byte(a+1)+1);m.byte(a+1,next);m.scalar(a+8,m.scalar(a+4)/static_cast<float>(m.byte(a+2)));if(m.byte(a)<=next)m.byte(a+1,0);}
// Debug0018DF38; E-0099-W47. Integral trapezoid and derivative original ordering.
void PidError::Record(float error,float seconds){const float previous=m_.scalar(address_+0x20);m_.scalar(address_+0x20,error);m_.scalar(address_+0x24,previous);Average(m_,address_).Record(seconds*(previous+(error-previous)*.5f));Average(m_,address_+16).Record((error-previous)/seconds);}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug0018DE40; E-0099-W74; original table.cpp; replay-frame/0018de40.json/type-Average.json.
void Average::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginSection(0x3279c0);
 for(unsigned i=0;i<3;++i)s.MoveData(address_+i,1);
 s.MoveData(address_+4,4);for(unsigned i=0;i<m_.byte(address_);++i)s.MoveData(m_.word(address_+12)+4*i,4);
 if(m_.byte(snapshot+0x3c)||m_.byte(snapshot+0x3d)){
  m_.word(address_+8,0);if(m_.byte(address_+2))m_.scalar(address_+8,m_.scalar(address_+4)/static_cast<float>(m_.byte(address_+2)));
 }
 s.EndSection();
}
// Debug0018DFA0; E-0099-W74; replay-frame/0018dfa0.json/type-PidError.json.
void PidError::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2ac800);Average(m_,address_).DoSnapshot(snapshot);
 Average(m_,address_+0x10).DoSnapshot(snapshot);s.MoveData(address_+0x20,4);s.MoveData(address_+0x24,4);s.EndSection();
}
}

namespace hp2::debug {
// Debug0018E058, E-0099-W92. Source operation order is retained.
void Linear::Init(float x0,float y0,float x1,float y1){const float slope=(y1-y0)/(x1-x0);m_.scalar(address_,slope);m_.scalar(address_+4,y0-slope*x0);}
// Debug0018E078, E-0099-W92.
float Linear::GetValue(float x)const{return m_.scalar(address_)*x+m_.scalar(address_+4);}
}

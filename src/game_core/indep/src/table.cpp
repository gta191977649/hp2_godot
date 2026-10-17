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

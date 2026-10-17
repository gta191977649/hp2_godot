
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x0018F808; E-0041-R04; E-0042-S001; exact Debug TU ..\indep\src\table.cpp
namespace port::driving {
void Sub_0018F808(Memory& m,std::uint32_t s,std::uint32_t values,std::uint32_t count,float f12,float f13) {
    using namespace detail_group_0018f808;
    m.word(s,values);m.word(s+4,count);SetMinMax(m,s,f12,f13);
}
}

// Retail 0x0018F838; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\table.cpp
namespace port::driving {
float Table::GetValue(Memory& m,std::uint32_t s,float value) {
    using namespace detail_group_0018f808;
    const auto data=m.word(s), count=m.word(s+4);
    return interpolate(value,m.scalar(s+8),m.scalar(s+0xc),m.scalar(s+0x10),count,
        [&](std::int32_t i){
            if(i<0 || static_cast<std::uint32_t>(i)>=count)
                throw std::out_of_range("Interpolation index outside mapped table");
            return m.scalar(data+static_cast<std::uint32_t>(i)*4);
        });
}
}

// Retail 0x0018F9F0; E-0042-C077; E-0042-S001; exact Debug TU ..\indep\src\table.cpp
namespace port::driving {
float Graph::GetValue(Memory& m,std::uint32_t s,float value) {
    using namespace detail_group_0018f808;
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
void Graph::GetValue() {
    hp2::not_implemented("0x0018F9F0", "Graph::GetValue (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00190050; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00190050.md.
void Linear::Init() {
    hp2::not_implemented("0x00190050", "Linear::Init (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/Table.hpp"

namespace hp2::recovered {
// ELF 0x0018F808; E-0041-R04; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018f808.md.
void Table::Sub_0018F808() {
    hp2::not_implemented("0x0018F808", "Table::Sub_0018F808 (signature and full body unresolved)");
}

// ELF 0x0018F838; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0018f838.md.
void Table::GetValue() {
    hp2::not_implemented("0x0018F838", "Table::GetValue (signature and full body unresolved)");
}
} // namespace hp2::recovered

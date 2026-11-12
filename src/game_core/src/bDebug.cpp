// TASK-0099 E-0099-W75. Exact Debug mdebug definition/TU src/bDebug.cpp.
// SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "bDebug.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
namespace hp2::debug {
// Debug0024A350. Original unsigned tick wrap, float division, then milliseconds.
float bGetTickerDifference(::port::driving::Memory& m,std::uint32_t start,std::uint32_t end){const float frequency=m.scalar(0x326254);if(!std::isfinite(frequency)||frequency==0)hp2::not_implemented("0x0024A350","exceptional original ticker frequency division");return (float(end-start)/frequency)*1000.f;}
}

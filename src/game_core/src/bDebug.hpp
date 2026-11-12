#pragma once
// Port declaration of original src/bDebug.cpp primitive arithmetic, not an ABI.
#include "game_core/port/driving/memory.hpp"
namespace hp2::debug {
float bGetTickerDifference(::port::driving::Memory&,std::uint32_t start,std::uint32_t end); //0024A350, E-0099-W75.
}

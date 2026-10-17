
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail None; explicit host partition; original TU unconfirmed
namespace port::driving::init {
void ByteView::require(std::size_t offset,std::size_t length) const {
    if(!data || offset>size || length>size-offset) throw std::out_of_range("PS2 initialization image is truncated");
}
}

// Retail None; explicit host partition; original TU unconfirmed
namespace port::driving::init {
std::uint32_t ByteView::word(std::size_t offset) const {
    require(offset,4);
    return std::uint32_t(data[offset]) | (std::uint32_t(data[offset+1])<<8) |
           (std::uint32_t(data[offset+2])<<16) | (std::uint32_t(data[offset+3])<<24);
}
}

// Retail None; explicit host partition; original TU unconfirmed
namespace port::driving::init {
void ByteView::word(std::size_t offset,std::uint32_t value) const {
    require(offset,4);
    for(unsigned i=0;i<4;++i)data[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
}

// Retail None; explicit host partition; original TU unconfirmed
namespace port::driving::init {
float ByteView::scalar(std::size_t offset) const {
    auto bits=word(offset);float value;static_assert(sizeof(value)==sizeof(bits));
    std::memcpy(&value,&bits,4);return value;
}
}

// Retail None; explicit host partition; original TU unconfirmed
namespace port::driving::init {
void ByteView::scalar(std::size_t offset,float value) const {
    std::uint32_t bits;std::memcpy(&bits,&value,4);word(offset,bits);
}
}

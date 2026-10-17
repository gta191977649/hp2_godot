#pragma once
// TASK-0044: host binding of a recovered object to explicit PS2 memory.
// This storage is port-side, not an assertion about the original C++ ABI.
#include "memory.hpp"

namespace port::driving {
class ObjectView {
public:
    ObjectView(Memory& memory, std::uint32_t address) : m_(memory), address_(address) {}
    std::uint32_t address() const { return address_; }
protected:
    Memory& m_;
    std::uint32_t address_;
};
}

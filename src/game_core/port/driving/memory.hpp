#pragma once
// Port-side bounded logical address space. No original allocator or native ABI is claimed.
#include "game_core/port/driving/byte_view.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <vector>

namespace port::driving {
class Memory {
public:
    using Address=std::uint32_t;
    // Port-side stack lifetime for temporary logical allocations. Persistent
    // constructor allocations must occur outside this scope.
    class Scratch {
    public:
        explicit Scratch(Memory& memory):memory_(memory),count_(memory.allocations_.size()),next_(memory.next_){}
        ~Scratch() noexcept;
        Scratch(const Scratch&)=delete;
        Scratch& operator=(const Scratch&)=delete;
    private:
        Memory& memory_;
        std::size_t count_;
        Address next_;
    };
    Address allocate(std::uint32_t size,std::uint32_t alignment=16);
    void map(Address,const std::vector<std::uint8_t>&);
    void map(Address,std::uint32_t size);
    init::ByteView view(Address,std::size_t size);
    std::uint8_t byte(Address) const;
    std::int8_t signed_byte(Address) const;
    std::uint16_t half(Address) const;
    std::int16_t signed_half(Address) const;
    std::uint32_t word(Address) const;
    std::int32_t signed_word(Address) const;
    float scalar(Address) const;
    void byte(Address,std::uint8_t);
    void half(Address,std::uint16_t);
    void word(Address,std::uint32_t);
    void scalar(Address,float);
    std::array<float,4> vector(Address) const;
    std::array<float,16> matrix(Address) const;
    void vector(Address,const std::array<float,4>&);
    void matrix(Address,const std::array<float,16>&);
    void copy(Address destination,Address source,std::size_t count);
private:
    const std::uint8_t* resolve(Address,std::size_t) const;
    std::uint8_t* resolve(Address,std::size_t);
    std::map<Address,std::vector<std::uint8_t>> regions_;
    std::vector<Address> allocations_;
    Address next_=0x01000000;
};
}

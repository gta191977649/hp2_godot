#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace port::driving::init {
struct ByteView {
    std::uint8_t* data;
    std::size_t size;
    template<std::size_t N> ByteView(std::array<std::uint8_t,N>& image):data(image.data()),size(N){}
    ByteView(std::uint8_t* p,std::size_t n):data(p),size(n){}
    void require(std::size_t offset,std::size_t length) const;
    std::uint32_t word(std::size_t offset) const;
    float scalar(std::size_t offset) const;
    void word(std::size_t offset,std::uint32_t value) const;
    void scalar(std::size_t offset,float value) const;
};
}

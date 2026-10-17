#pragma once
// Port-side byte views of initialization dependencies; original source path unknown.
// Not an implementation of the whole 00137100 entry, which still throws.
#include <array>
#include <cstddef>
#include <cstdint>

namespace port::driving::init {
// Explicit PS2 little-endian images retain untouched bytes and encoded addresses.
// Bounds checks belong to the port adapter, not the original algorithm.
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
// All entries below: SLUS_203.62, revision unknown, SHA-256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// E-0038-I01: ELF 0025AC68, four quadword stores.
std::array<float,16> Sub_0025AC68();
// E-0038-I02: ELF 0025AC98, scalar 4x4 affine inverse; ordinary finite scope.
std::array<float,16> Sub_0025AC98(const std::array<float,16>&);
// E-0038-I03: ELF 0019CC58, config+0 scalar / config+0x10 matrix.
void Sub_0019CC58(ByteView,ByteView);
// E-0038-I04: ELF 0019C9B0, config pointer supplied as encoded PS2 word.
void Sub_0019C9B0(ByteView,std::uint32_t,ByteView);
// E-0038-I05: ELF 001A3F10, arguments a1/a2/a3/t0 are encoded words.
void Sub_001A3F10(ByteView,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
// E-0038-I06: ELF 001A5EA0, arguments a1/a2/a3/t0/t1 are encoded words.
void Sub_001A5EA0(ByteView,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
// E-0038-I07: ELF 0019C170; argument is encoded address of list head.
void Sub_0019C170(ByteView,std::uint32_t);
// E-0038-I08: ELF 0019CC40; null second argument leaves +0x160 unchanged.
void Sub_0019CC40(ByteView,const std::array<std::uint32_t,4>*);
// E-0038-I09: ELF 001A5F88; explicit resolved vectors replace pointer walks.
void Sub_001A5F88(ByteView,float,const std::array<float,4>&,const std::array<float,4>&);
// E-0038-I10: ELF 001A4348; explicit word replaces global DAT_00330CA0.
void Sub_001A4348(ByteView,std::uint32_t);
// E-0038-I11: ELF 001A4158; f12/f13 are stored to +0x1c/+0x20.
void Sub_001A4158(ByteView,float,float);
// E-0038-I12: ELF 001A5FB0, two zero stores.
void Sub_001A5FB0(ByteView);
}

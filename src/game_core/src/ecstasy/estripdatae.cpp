#include "estripdatae.hpp"
#include <cstring>
#include <stdexcept>

namespace port::vehicle_packet_detail {
float read_float(const std::vector<std::uint8_t>& data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 4) throw std::runtime_error("Truncated vehicle packet component");
    const std::uint32_t bits = std::uint32_t(data[offset]) | std::uint32_t(data[offset+1]) << 8 |
        std::uint32_t(data[offset+2]) << 16 | std::uint32_t(data[offset+3]) << 24;
    float value; std::memcpy(&value, &bits, 4); return value;
}
}
namespace hp2::recovered {
using port::vehicle_packet_detail::read_float;
// ELF 0011C248; E-0041-R16; docs/DEBUG_REFERENCE_SYMBOLS.md. Host projection; original ABI unconfirmed.
std::uint8_t eStripData::GetNumVerts(const std::vector<std::uint8_t>& data, std::size_t packet) {
    if (packet > data.size() || data.size() - packet < 5) throw std::runtime_error("Truncated vehicle packet header");
    return data[packet+4];
}
// ELF 0011C2B8; E-0041-R18; docs/DEBUG_REFERENCE_SYMBOLS.md. Host projection; original ABI unconfirmed.
std::array<float,3> eStripData::GetXYZ(const std::vector<std::uint8_t>& data, std::size_t packet, std::size_t index) {
    const auto count = GetNumVerts(data,packet);
    if (index >= count) throw std::runtime_error("Vehicle packet position index out of range");
    const std::size_t groups=count/4, remainder=count%4, lane=index%4;
    std::size_t offset=packet+0x20+(index/4)*0x30;
    if (index < groups*4) return {read_float(data,offset+lane*4),read_float(data,offset+lane*4+16),read_float(data,offset+lane*4+32)};
    if (groups) offset+=4;
    return {read_float(data,offset+lane*4),read_float(data,offset+(remainder+lane)*4),read_float(data,offset+(remainder*2+lane)*4)};
}
// ELF 0011C268; E-0041-R17; docs/DEBUG_REFERENCE_SYMBOLS.md. Host projection; original ABI unconfirmed.
std::array<float,2> eStripData::GetST0(const std::vector<std::uint8_t>& data, std::size_t packet, std::size_t index) {
    const auto count=eStripData::GetNumVerts(data,packet);
    if(index>=count) throw std::runtime_error("Vehicle packet UV index out of range");
    std::size_t offset=packet+count*12+0x20;
    if(count%4 && count/4) offset+=4;
    if(index<(count/2)*2) {
        offset+=(index/2)*16+4;
        if(index%2) return {read_float(data,offset+12),read_float(data,offset+8)};
    } else {
        if(count/2) offset+=(index/2)*16+4;
        offset+=4;
    }
    return {read_float(data,offset),read_float(data,offset+4)};
}
// ELF 0011C180; E-0041-R15; docs/DEBUG_REFERENCE_SYMBOLS.md. Host projection; original ABI unconfirmed.
std::uint16_t eStripDataE::GetElementRGBA(const std::vector<std::uint8_t>& data, std::size_t packet, std::size_t index) {
    const auto count=eStripData::GetNumVerts(data,packet);
    if(index>=count)throw std::runtime_error("Vehicle packet color index out of range");
    std::size_t offset=packet+count*12+count*8+0x28+index*2;
    if(count%4 && count/4)offset+=4;
    if(count%2 && count/2)offset+=4;
    if(offset>data.size() || data.size()-offset<2)throw std::runtime_error("Truncated vehicle packet color");
    return std::uint16_t(data[offset]) | std::uint16_t(data[offset+1])<<8;
}
}

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0011BF98; E-0041-R13; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/0011bf98.md.
void eStripDataE::GetElementXYZ() {
    hp2::not_implemented("0x0011BF98", "eStripDataE::GetElementXYZ (signature and full body unresolved)");
}

// ELF 0x0011C090; E-0041-R14; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/0011c090.md.
void eStripDataE::GetElementST0() {
    hp2::not_implemented("0x0011C090", "eStripDataE::GetElementST0 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0011C248; E-0041-R16; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/0011c248.md.
void eStripData::GetNumVerts() {
    hp2::not_implemented("0x0011C248", "eStripData::GetNumVerts (signature and full body unresolved)");
}

// ELF 0x0011C268; E-0041-R17; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/0011c268.md.
void eStripData::GetST0() {
    hp2::not_implemented("0x0011C268", "eStripData::GetST0 (signature and full body unresolved)");
}

// ELF 0x0011C2B8; E-0041-R18; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/0011c2b8.md.
void eStripData::GetXYZ() {
    hp2::not_implemented("0x0011C2B8", "eStripData::GetXYZ (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0011C180; E-0041-R15; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/0011c180.md.
void eStripDataE::GetElementRGBA() {
    hp2::not_implemented("0x0011C180", "eStripDataE::GetElementRGBA (signature and full body unresolved)");
}
} // namespace hp2::recovered

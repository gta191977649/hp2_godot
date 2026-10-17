#pragma once
// TASK-0038 E-0038-C01/C02. Port-side environment query adapter and recovered roughness.
#include "port/driving/memory.hpp"
namespace port::driving {
struct ContactProvider {
    virtual ~ContactProvider()=default;
    virtual float Sub_00152070(Memory&,std::uint32_t owner,std::uint32_t point,
        std::uint32_t material,std::uint32_t plane,std::uint32_t found)=0;
};
// Explicit synthetic environment geometry, not a recovered original road query.
// Material1 is the ELF table row with SKID_ROAD at00318CB0 (E-0038-C01).
struct PlaneContact final:ContactProvider {
    float height=0;
    float Sub_00152070(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)override;
};
// ELF00153458/00153498; E-0038-C02; native offsets/data, not host random noise.
float Sub_00153458(Memory&,std::int32_t,std::int32_t);
float Sub_00153498(Memory&,std::uint32_t,std::int32_t);
}

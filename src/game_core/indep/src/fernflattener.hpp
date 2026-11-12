#pragma once
// Port companion declaration; no original header pathname or host ABI claim.
#include "game_core/port/race/world_construction.hpp"
#include "game_core/port/driving/object_view.hpp"
#include <optional>
namespace hp2::debug {
class FernShaker:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001D67A0(std::uint32_t); //E-0099-W65 original constructor.
 void Sub_001D69C8(int); //original destructor.
 void Shake(std::uint32_t,std::optional<std::uint32_t> =std::nullopt); //001D6A40.
};
void ShakeFern(::port::driving::Memory&,std::uint32_t,std::uint32_t,std::optional<std::uint32_t> =std::nullopt); //001D6EA8.
void FlattenFern(::port::driving::Memory&,std::uint32_t); //001D6790 empty.
void ServiceFernFlatteners(::port::driving::Memory&); //001D6798 empty.
void FlushFernShakers(::port::driving::Memory&); //001D6DE8.
class FernShakerInfo {
    ::port::driving::Memory& memory_;std::uint32_t address_;
public:
    FernShakerInfo(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    // Debug001D7170, E-0099-W06; ../indep/src/fernflattener.cpp.
    void Init(std::uint32_t,std::uint32_t,const ::port::race::EmitterGroupLookup&);
};
}


// TASK-0112 E-0112-ESC01. Include once at END original copmgr.cpp.
namespace hp2::debug {
// Debug0021F748/0021F778. Actual RP/world/busy predicates; no invented absence.
bool tCopManager::IsChopperAvailable()const{const auto world=memory_.word(0x3272f0);return memory_.word(world+0x18)&&memory_.word(0x311f78+0x26c)&&!memory_.word(address_+0x138);}
std::uint32_t tCopManager::RequestChopper(){auto& m=memory_;const auto chopper=m.word(m.word(0x3272f0)+0x18);std::uint32_t result=0;if(chopper&&m.word(0x311f78+0x26c)&&!m.word(address_+0x138)){const int attacks=m.signed_word(address_+0x28fc);if(!m.word(0x3121ec)||attacks){m.word(address_+0x138,1);result=chopper;if(attacks)m.word(address_+0x28fc,std::uint32_t(attacks-1));}}return result;}
// Debug00220B08. The actual first HP event has bRoadBlocks=0. Preserve the
// original loaded-data/RP return before requiring positive deployment children.
std::uint32_t tCopManager::RequestRoadBlock(std::uint32_t,float,::port::race::PhysicsConstructionServices*){auto& m=memory_;if(!m.word(0x328c00)||!m.word(0x311f78+0x264))return 0;hp2::not_implemented("0x00221080","active authored roadblock deployment requires original cop selection, graph, layout, teleport, barricade and spike children");}
}

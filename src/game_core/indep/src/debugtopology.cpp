// Exact original ../indep/src/debugtopology.cpp, live MCP source corroborated.
// Debug NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/driving/player_services.hpp"
namespace hp2::debug {
// Debug001FEE80, E-0099-W63. Instruction-proven original empty body.
void RealDriverAddNearbyDebugTopology(::port::driving::Memory&,std::uint32_t,std::uint32_t){}
}
namespace hp2::recovered {
// Legacy port Car-argument bridge; the exact original body is the target above.
void RealDriverAddNearbyDebugTopology(::port::driving::Memory& m,std::uint32_t car){hp2::debug::RealDriverAddNearbyDebugTopology(m,car+0x520,car+0x60);}
}

// Original ../indep/src/terraintype.cpp ownership: E-0099-W12 read-only ECOFF
// FDR/SYMR/marker and path-string captures, separately from function recovery.
#include "game_core/port/race/smokeables.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <stdexcept>

namespace hp2::debug {
// Debug001D1248, E-0099-W12. Exact ordered33-row hash lookup, absent returnsNONE0.
int MatchTerrainType(const ::port::driving::Memory& memory,std::uint32_t hash){
    for(int i=0;i<33;++i)if(memory.word(0x3102e8+i*0x90+4)==hash)return i;
    return 0;
}
// Debug001D1280, E-0099-W12. Original process-static hashes are not in ELF data.
void Sub_001D1280(::port::driving::Memory& memory){
    for(std::uint32_t row=0x3102e8;row<0x311578;row+=0x90){
        const auto address=memory.word(row);std::string name;
        for(std::uint32_t n=0;n<256;++n){const auto ch=memory.byte(address+n);if(!ch)break;name.push_back(char(ch));}
        if(name.size()==256)throw std::out_of_range("Original terrain type name exceeds host bound");
        memory.word(row+4,hp2::frontend::bStringHash(name.c_str()));
    }
}
}

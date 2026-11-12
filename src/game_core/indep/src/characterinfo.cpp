// TASK-0099 E-0099-W28. Original ..\indep\src\characterinfo.cpp source
// ownership corroborated by read-only MCP FDR/SYMR/source-marker/string captures.
#include "game_core/port/race/character_resources.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <cstring>
#include <stdexcept>

namespace hp2::debug {
// Debug001D6030, including bStrICmp0024CAB0/bToUpper0024CB98, E-0099-W28.
std::uint32_t GetBodyTypeInfo(const ::port::driving::Memory& m,std::uint32_t name){
    const auto upper=[](std::uint8_t c){return std::uint32_t(c-'a')<26?std::uint8_t(c&0x5f):c;};
    for(int i=0;i<m.signed_word(0x328438);++i){const auto info=m.word(0x328434)+std::uint32_t(i)*0xb4;
        for(std::uint32_t j=0;;++j){const auto a=upper(m.byte(name+j)),b=upper(m.byte(info+j));if(a!=b)break;if(!a)return info;}}
    return 0;
}
// Debug001D61C8, E-0099-W28. Original index bounds, not CharacterNumber search.
std::uint32_t GetCharacterInfo(const ::port::driving::Memory& m,int number){return number>=0 && number<m.signed_word(0x328440)?m.word(0x32843c)+std::uint32_t(number)*0x44:0;}
std::uint32_t GetCharacterInfoFromNameHash(const ::port::driving::Memory& m,std::uint32_t hash){
    for(int i=0;i<m.signed_word(0x328440);++i){const auto info=m.word(0x32843c)+std::uint32_t(i)*0x44;std::string name;for(unsigned j=0;;++j){const auto c=m.byte(info+4+j);if(!c)break;name+=char(c);}if(hp2::frontend::bStringHash(name.c_str())==hash)return info;}return 0;
}
}
namespace port::race {
CharacterResources load_character_resources(::port::driving::Memory& m,const std::vector<std::vector<std::uint8_t>>& bundles){
    if(m.word(0x328434) || m.word(0x32843c))throw std::logic_error("Character source owner requires unloaded original tables");
    CharacterResources resources;
    for(const auto& bytes:bundles){std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(bytes,&consumed);
        if(consumed!=bytes.size())throw std::out_of_range("Incomplete original character source bundle");
        const auto visit=[&](const auto& self,const auto& list)->void{for(const auto& chunk:list){
            if(chunk.tag==0x34300 || chunk.tag==0x34301){const auto stride=chunk.tag==0x34300?0x44u:0xb4u;
                if(chunk.size%stride)throw std::out_of_range("Invalid original character record span");
                const auto allocation=m.allocate(chunk.size?chunk.size:8);resources.owned_allocations.push_back(allocation);
                std::memcpy(m.view(allocation,chunk.size?chunk.size:8).data,bytes.data()+(chunk.size?chunk.payload_offset():chunk.header_offset),chunk.size?chunk.size:8);
                const auto payload=allocation+(chunk.size?0:8);
                // Original LoaderCharacterInfo001D5F68: exact raw source arrays.
                if(chunk.tag==0x34300){m.word(0x32843c,payload);m.word(0x328440,chunk.size/stride);resources.character_chunk=true;}
                else{m.word(0x328434,payload);m.word(0x328438,chunk.size/stride);resources.body_chunk=true;}}
            self(self,chunk.children);
        }};visit(visit,chunks);
    }
    resources.source_data_loaded=true;return resources;
}
void unload_character_resources(::port::driving::Memory& m,CharacterResources& resources){
    if(!resources.source_data_loaded)throw std::logic_error("Character source owner is not loaded");
    // Original UnloaderCharacterInfo001D5FE8 resets matching table globals.
    // Borrowing DriverInfo/AnimatedDriver objects must be destroyed by caller.
    if(resources.body_chunk){m.word(0x328434,0);m.word(0x328438,0);}
    if(resources.character_chunk){m.word(0x32843c,0);m.word(0x328440,0);}
    for(auto it=resources.owned_allocations.rbegin();it!=resources.owned_allocations.rend();++it)m.release(*it);
    resources=CharacterResources{};
}
}

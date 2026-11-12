#pragma once
// Port-only owner of complete original CharacterInfo/BodyTypeInfo source records.
#include "game_core/port/driving/memory.hpp"
namespace port::race {
struct CharacterResources {std::vector<std::uint32_t> owned_allocations;bool character_chunk=false,body_chunk=false,source_data_loaded=false;};
CharacterResources load_character_resources(::port::driving::Memory&,const std::vector<std::vector<std::uint8_t>>& complete_bundles);
void unload_character_resources(::port::driving::Memory&,CharacterResources&);
}
namespace hp2::debug {
// Debug001D6030/001D61C8; E-0099-W28. Original raw table consumers.
std::uint32_t GetBodyTypeInfo(const ::port::driving::Memory&,std::uint32_t name);
std::uint32_t GetCharacterInfo(const ::port::driving::Memory&,int number);
std::uint32_t GetCharacterInfoFromNameHash(const ::port::driving::Memory&,std::uint32_t); //001D6140.
}

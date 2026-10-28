#include "race_launch_catalog.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <stdexcept>
namespace port::race {
LaunchCatalog::LaunchCatalog(const std::vector<std::uint8_t>& bytes,::port::driving::Memory& memory)
    :cars_(::port::selection::decode_car_info(bytes)){
    const auto chunks=hp2::assets::parse_chunks_top(bytes);
    const auto* characters=hp2::assets::find_first(chunks,0x34300);
    const auto* tracks=hp2::assets::find_first(chunks,0x34201);
    const auto* skins=hp2::assets::find_first(chunks,0x34601);
    if(!characters || !tracks || !skins || characters->size%0x44 || tracks->size%0xa4 || skins->size%0x40)
        throw std::runtime_error("Original launch catalogs missing or incompatible with reviewed Retail schema");
    const auto word=[&](std::size_t at){return std::uint32_t(bytes.at(at))|(std::uint32_t(bytes.at(at+1))<<8)
        |(std::uint32_t(bytes.at(at+2))<<16)|(std::uint32_t(bytes.at(at+3))<<24);};
    const auto text=[&](std::size_t at,std::size_t maximum){std::size_t length=0;
        while(length<maximum && bytes.at(at+length))++length;
        if(length==maximum)throw std::runtime_error("Unterminated original launch catalog string");
        return std::string(reinterpret_cast<const char*>(bytes.data()+at),length);};
    characters_.reserve(characters->size/0x44);
    for(unsigned i=0;i<characters->size/0x44;++i){const auto row=characters->payload_offset()+i*0x44;
        characters_.push_back({static_cast<int>(word(row)),text(row+4,32),text(row+0x24,24),i*0x44});}
    tracks_.reserve(tracks->size/0xa4);
    for(unsigned i=0;i<tracks->size/0xa4;++i){const auto row=tracks->payload_offset()+i*0xa4;
        tracks_.push_back({text(row,32),static_cast<std::int8_t>(bytes.at(row+0x41)),
            static_cast<std::int8_t>(bytes.at(row+0x43)),word(row+0x54)});
        if(i==53)tracks_.back().name.clear(); // Retail LoaderTrackGeometry00153608 clears track54.
    }
    skins_.reserve(skins->size/0x40);
    for(unsigned i=0;i<skins->size/0x40;++i){const auto row=skins->payload_offset()+i*0x40;
        skins_.push_back({static_cast<int>(word(row)),static_cast<int>(word(row+4)),i*0x40});}
    // Parse/validate/reserve first: malformed assets do not grow the live logical arena.
    const auto total=std::uint64_t(characters->size)+skins->size;
    if(total==0 || total>0xffffffffu)throw std::runtime_error("Original launch catalog extent exceeds logical address space");
    const auto character_base=memory.allocate(static_cast<std::uint32_t>(total));
    const auto skin_base=character_base+characters->size;
    for(unsigned i=0;i<characters->size;++i)memory.byte(character_base+i,bytes.at(characters->payload_offset()+i));
    for(unsigned i=0;i<skins->size;++i)memory.byte(skin_base+i,bytes.at(skins->payload_offset()+i));
    for(auto& character:characters_)character.address+=character_base;
    for(auto& skin:skins_)skin.address+=skin_base;
}
const LaunchTrack* LaunchCatalog::track(int number) const{
    if(number<1 || static_cast<std::size_t>(number)>tracks_.size())return nullptr;
    const auto& value=tracks_[number-1];return value.name.empty()?nullptr:&value;
}
std::uint32_t LaunchCatalog::character_address(int index) const{
    return index<0 || static_cast<std::size_t>(index)>=characters_.size()?0:characters_[index].address;
}
bool LaunchCatalog::character_is_male(int index) const{
    if(!character_address(index))hp2::not_implemented("0x00182610","original GetCharacterInfo missing gender record");
    const auto& body=characters_[index].body_type;
    // bStrICmp0024CAB0 -> bToUpper0024CB98 only folds ASCII a..z.
    const char wanted[]="MALE";
    if(body.size()!=4)return false;
    for(unsigned i=0;i<4;++i){const unsigned c=static_cast<unsigned char>(body[i]);
        if((c-'a'<26?(c&0x5f):c)!=unsigned(wanted[i]))return false;}
    return true;
}
std::optional<int> LaunchCatalog::character_number_from_hash(std::uint32_t hash) const{
    for(const auto& character:characters_)if(hp2::frontend::bStringHash(character.name.c_str())==hash)return character.number;
    return std::nullopt;
}
std::uint32_t LaunchCatalog::skin_info(int car,int number,int fallback) const{
    for(const auto& skin:skins_)if(skin.car==car && skin.number==number)return skin.address;
    // Debug GetSkinInfo00186330 falls back to FIRST table row, not the car's default skin.
    return fallback && !skins_.empty()?skins_.front().address:0;
}
ParticipantServices LaunchCatalog::services(int language) const{
    ParticipantServices result{cars_,language,{},{},{},{},{}};
    result.character_address=[this](int index){return character_address(index);};
    result.character_is_male=[this](int index){return character_is_male(index);};
    result.character_number_from_hash=[this](std::uint32_t hash){return character_number_from_hash(hash);};
    result.track_cop_hash=[this](int number)->std::optional<std::uint32_t>{
        const auto* info=track(number);if(!info)return std::nullopt;return info->cop_name_hash;};
    result.skin_info=[this](int car,int skin,int fallback){(void)skin_info(car,skin,fallback);};
    return result;
}
}

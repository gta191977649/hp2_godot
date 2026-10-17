#include "game_core/port/track/texture_decode.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void word(std::vector<std::uint8_t>& data,std::size_t offset,std::uint32_t value){
    for(unsigned byte=0;byte<4;++byte)data.at(offset+byte)=std::uint8_t(value>>(byte*8));
}
std::array<std::uint8_t,4> colour(unsigned index){
    const std::array<std::uint8_t,4> alpha{{128,255,0,64}};
    return {{std::uint8_t(index*13),std::uint8_t(240-index*7),std::uint8_t(17+index*3),alpha[index%4]}};
}
// Synthetic, asymmetric source rows: base0..15 and mip12..15. No player-owned
// pixels or package data are included. PSMT8 physical CLUT slots8..15 reside16..23.
std::vector<std::uint8_t> fixture(unsigned depth){
    const auto base_bytes=16*depth/8,mip_bytes=4*depth/8;
    const auto image_bytes=base_bytes+mip_bytes,palette_offset=(image_bytes+15)&~15u;
    const auto palette_bytes=depth==32?0u:(depth==8?1024u:64u);
    std::vector<std::uint8_t> data(256+palette_offset+palette_bytes);
    word(data,0,hp2::assets::kMainChunkId);word(data,4,std::uint32_t(data.size()-8));
    word(data,8,hp2::assets::kEntryTableChunkId);word(data,12,164);
    const auto header=16u;data[header+8]='T';word(data,header+32,0x12345678);
    data[header+0x24]=4;data[header+0x26]=4;data[header+0x28]=std::uint8_t(depth);
    word(data,header+0x34,palette_offset);word(data,header+0x38,image_bytes);
    word(data,header+0x3c,palette_bytes);word(data,header+0x44,base_bytes);
    data[header+0x48]=data[header+0x49]=2;
    data[header+0x4a]=depth==32?0:std::uint8_t(depth==8?19:20);data[header+0x6e]=1;
    data[header+0x75]=2;
    word(data,header+0x40,0x01234567);
    word(data,180,hp2::assets::kDataPoolChunkId);word(data,184,std::uint32_t(data.size()-188));
    const auto pixels=[&](std::size_t offset,unsigned count,unsigned first){
        for(unsigned pixel=0;pixel<count;++pixel){const auto index=first+pixel;
            if(depth==32){const auto rgba=colour(index);for(unsigned c=0;c<4;++c)data[offset+pixel*4+c]=rgba[c];}
            else if(depth==8)data[offset+pixel]=std::uint8_t(index);
            else data[offset+pixel/2]|=std::uint8_t(index<<((pixel%2)*4));
        }
    };
    pixels(256,16,0);pixels(256+base_bytes,4,12);
    if(depth!=32)for(unsigned index=0;index<16;++index){
        const auto physical=depth==8&&index>=8?index+8:index;
        const auto rgba=colour(index);for(unsigned c=0;c<4;++c)data[256+palette_offset+physical*4+c]=rgba[c];
    }
    return data;
}
void expected(const hp2::assets::DecodedTexture& image,unsigned first){
    for(std::size_t pixel=0;pixel<std::size_t(image.width)*image.height;++pixel){
        const auto rgba=colour(first+unsigned(pixel));
        for(std::size_t channel=0;channel<4;++channel)
            check(image.rgba.at(pixel*4+channel)==rgba[channel],"Track source row/column or raw alpha changed");
    }
}
}
int main(){
    for(const unsigned depth:{4u,8u,32u}){
        auto archive=hp2::assets::Hp2TpkArchive::load(fixture(depth));check(bool(archive),"Synthetic track TPK invalid");
        const auto header=archive->headers().front();auto image=port::track::decode_texture_level(*archive,header);
        check(header.BiasLevel==2,"Original texture BiasLevel offset changed");
        check(header.ImageParentHash==0x01234567,"Original image parent hash offset changed");
        check(bool(image)&&image->width==4&&image->height==4,"Track base decode failed");expected(*image,0);
        auto mip=header;mip.width=mip.height=2;mip.shift_width=mip.shift_height=1;
        mip.image_placement+=header.BaseImageSize;mip.image_size=header.BaseImageSize>>2;
        mip.is_swizzled=header.mip_is_swizzled[1];
        const auto level=port::track::decode_texture_level(*archive,mip);
        check(bool(level)&&level->width==2&&level->height==2,"Track original mip decode failed");expected(*level,12);
        if(depth!=32){
            const auto legacy=archive->decode(header,false,true);check(bool(legacy),"Legacy decoder failed");
            check(legacy->rgba.front()==colour(12)[0],"Shared indexed decoder row convention changed");
        }
    }
    std::cout<<"Track source-row texture tests passed (4-bit, 8-bit, linear32, base and original mip)\n";
}

// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\texture.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/Texture.hpp"

#include "game_core/indep/src/texture.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/track/texture_bindings.hpp"
#include "game_core/port/race/debug_transport.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <cstring>
#include <stdexcept>

namespace hp2::recovered {
// Retail001ABF48 / Debug001A95C8; E0056T01, reverse/task0056/textures.md.
std::optional<std::uint32_t> GetTextureInfo(std::uint32_t name,bool return_default,
    const std::set<std::uint32_t>& loaded,const std::set<std::uint32_t>& targets) {
    if(loaded.count(name)||targets.count(name))return name;
    if(!return_default)return std::nullopt;
    auto fallback=GetTextureInfo(0x19006835,false,loaded,targets);
    if(!fallback)hp2::not_implemented("0x001ABF48","GetTextureInfo: original DEFAULTTEXTURE absent, bBreak path");
    return fallback;
}
// ELF 0x001AC3F0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/001ac3f0.md.
void TexturePack::GetLoadedTexture() {
    hp2::not_implemented("0x001AC3F0", "TexturePack::GetLoadedTexture (signature and full body unresolved)");
}

// ELF 0x001AC468; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ac468.md.
void TexturePack::GetTexture() {
    hp2::not_implemented("0x001AC468", "TexturePack::GetTexture (signature and full body unresolved)");
}

// ELF 0x001AC4B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ac4b0.md.
void TexturePack::GetNthTexture() {
    hp2::not_implemented("0x001AC4B0", "TexturePack::GetNthTexture (signature and full body unresolved)");
}

// ELF 0x001AC780; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ac780.md.
void TexturePack::FindLoadedTextureData() {
    hp2::not_implemented("0x001AC780", "TexturePack::FindLoadedTextureData (signature and full body unresolved)");
}

// ELF 0x001AC7D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ac7d0.md.
void TexturePack::FindNotYetLoadedNeighbourTextureData() {
    hp2::not_implemented("0x001AC7D0", "TexturePack::FindNotYetLoadedNeighbourTextureData (signature and full body unresolved)");
}

// ELF 0x001AD008; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ad008.md.
void TexturePack::GetLoadedSize() {
    hp2::not_implemented("0x001AD008", "TexturePack::GetLoadedSize (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug001A9A70; E-0099-W13. A palette is required only when PaletteSize!=0.
std::uint32_t TexturePack::GetLoadedTexture(std::uint32_t hash)const{
 const auto& m=memory_;const int count=m.signed_word(address_+0x74);
 for(int i=0;i<count;++i){const auto t=m.word(address_+0x70)+std::uint32_t(i)*0xa4;
  if(m.word(t+0x20)==hash && m.word(t+0x90) && (!m.word(t+0x3c)||m.word(t+0x94)))return t;
 }return 0;
}
// Debug001A9AE8; E-0099-W13. First matching table entry even when unloaded.
std::uint32_t TexturePack::GetTexture(std::uint32_t hash)const{
 const auto& m=memory_;const int count=m.signed_word(address_+0x74);
 for(int i=0;i<count;++i){const auto t=m.word(address_+0x70)+std::uint32_t(i)*0xa4;if(m.word(t+0x20)==hash)return t;}
 return 0;
}
// Debug001A98A0; E-0099-W13. No unreviewed header byte is reinterpreted.
void TexturePack::AttachTextureTable(std::uint32_t table,int count){
 auto& m=memory_;m.word(address_+0x70,table);m.word(address_+0x74,count);
 for(int i=0;i<count;++i){const auto t=table+std::uint32_t(i)*0xa4;const auto parent=m.word(t+0x40);
  m.word(t+0x8c,parent?GetTexture(parent):t);m.word(t+0x88,address_);
  m.word(t+0x90,0);m.word(t+0x94,0);m.word(t+0x98,0);m.half(t+0xa0,0);m.half(t+0xa2,0);
 }m.word(0x3267f8,1);
}
// Debug001A9930; E-0099-W13. Placement is tested independently of size.
void TexturePack::AssignTextureData(std::uint32_t bytes,int begin,int size){
 auto& m=memory_;const int count=m.signed_word(address_+0x74);
 for(int i=0;i<count;++i){const auto t=m.word(address_+0x70)+std::uint32_t(i)*0xa4;
  const int image=m.signed_word(t+0x30),palette=m.signed_word(t+0x34);
  if(m.word(t+0x38)&&begin<=image&&std::int64_t(image)<std::int64_t(begin)+size)m.word(t+0x90,bytes+std::uint32_t(image-begin));
  if(m.word(t+0x3c)&&begin<=palette&&std::int64_t(palette)<std::int64_t(begin)+size)m.word(t+0x94,bytes+std::uint32_t(palette-begin));
 }m.word(0x3267f8,1);
}
// Debug001A95C8 ->001055E0/00105650/001058B8; E-0099-W13.
std::uint32_t GetTextureInfo(port::driving::Memory& m,std::uint32_t hash,int return_default,int include_unloaded){
 constexpr std::uint32_t head=0x327ea0;
 for(auto pack=m.word(head);pack!=head;pack=m.word(pack)){
  const auto texture=include_unloaded?TexturePack(m,pack).GetTexture(hash):TexturePack(m,pack).GetLoadedTexture(hash);
  if(texture){const auto next=m.word(pack),previous=m.word(pack+4);m.word(previous,next);m.word(next+4,previous);
   const auto first=m.word(head);m.word(head,pack);m.word(first+4,pack);m.word(pack,first);m.word(pack+4,head);
   m.byte(texture+0x78,1);return texture;}
 }
 // Read real Debug target IDs and TextureInfo records. Target initialization
 // belongs to original screen-buffer setup, not this resident-pack owner.
 for(unsigned i=0;i<22;++i){const int id=m.signed_word(0x2c7410+i*0xb0);
  if(id<0||id>=22)throw std::out_of_range("Original render-target ID outside TextureInfo table");
  const auto texture=0x2bfd10+std::uint32_t(id)*0xa4;if(m.word(texture+0x20)==hash)return texture;
 }
 if(!return_default)return 0;
 const auto fallback=GetTextureInfo(m,0x19006835,0,0);
 if(!fallback)hp2::not_implemented("0x001A95C8","original DEFAULTTEXTURE absent at bBreak boundary");
 return fallback;
}
}
namespace port::track {
namespace {
std::string texture_string(const std::vector<std::uint8_t>& b,std::size_t at,std::size_t end){
 std::string s;for(;at<end;++at){if(!b.at(at))return s;s.push_back(char(b[at]));}
 throw std::runtime_error("Unterminated original texture pack metadata");
}
}
void TextureBindings::init(){
 if(initialized_)throw std::logic_error("Texture bindings already initialized");
 const auto first=memory_.word(0x327ea0);
 if(first&&first!=0x327ea0)throw std::logic_error("TexturePackList already has another owner");
 memory_.word(0x327ea0,0x327ea0);memory_.word(0x327ea4,0x327ea0);initialized_=true;
}
// Port resource owner of Debug00100830's resident load/attachment path,
// E-0099-W13. Inventory registration is not claimed to be implemented.
std::size_t TextureBindings::load_bundle(const std::vector<std::uint8_t>& bytes){
 if(!initialized_)throw std::logic_error("Texture bindings are not initialized");
 const auto chunks=hp2::assets::parse_chunks_top(bytes);const auto before=packs_.size();
 const auto walk=[&](const auto& self,const std::vector<hp2::assets::Chunk>& list)->void{
  for(const auto& main:list){if(main.tag!=0xb0300001){self(self,main.children);continue;}
   if(main.children.size()<3||main.children[0].tag!=0x30300002||main.children[1].tag!=0x30300003||main.children[2].tag!=0x30300004)
    throw std::runtime_error("Original resident texture pack chunk order mismatch");
   const auto& info=main.children[0];const auto& table=main.children[1];const auto& data=main.children[2];
   if(info.size<0x50||table.size%0xa4)throw std::runtime_error("Original resident texture pack schema mismatch");
   const auto data_start=(data.payload_offset()+0x7f)&~std::size_t(0x7f),data_end=data.payload_offset()+data.size;
   if(data_start>data_end)throw std::runtime_error("Original texture pool alignment exceeds chunk");
   const auto name=texture_string(bytes,info.payload_offset(),info.payload_offset()+0x10);
   const auto filename=texture_string(bytes,info.payload_offset()+0x10,info.payload_offset()+0x50);
   if(name.size()>=32||filename.size()>=64)throw std::runtime_error("Original texture pack metadata exceeds object storage");
   Pack p;p.record=memory_.allocate(0x94);
   if(table.size){p.table=memory_.allocate(table.size);std::memcpy(memory_.view(p.table,table.size).data,bytes.data()+table.payload_offset(),table.size);}
   const auto size=std::uint32_t(data_end-data_start);
   if(size){p.pixels=memory_.allocate(size,0x80);std::memcpy(memory_.view(p.pixels,size).data,bytes.data()+data_start,size);}
   packs_.push_back(p);
   // Port-owned pack metadata stores only constructor-confirmed state required
   // by resident texture access. Optional active monitor-device transport is explicit.
   for(std::size_t i=0;i<name.size();++i)memory_.byte(p.record+8+std::uint32_t(i),std::uint8_t(name[i]));
   for(std::size_t i=0;i<filename.size();++i)memory_.byte(p.record+0x28+std::uint32_t(i),std::uint8_t(filename[i]));
   memory_.word(p.record+0x68,hp2::frontend::bStringHash(name.c_str()));memory_.word(p.record+0x90,0x40);
   memory_.word(p.record+0x80,p.record+0x80);memory_.word(p.record+0x84,p.record+0x80);
   constexpr std::uint32_t head=0x327ea0;const auto previous=memory_.word(head+4);
   memory_.word(previous,p.record);memory_.word(head+4,p.record);memory_.word(p.record,head);memory_.word(p.record+4,previous);
   // Actual00256740/00256768 packet producer and platform guard; E-0099-W30.
   const auto inventory=std::string("TexturePack,")+name;
   {port::driving::Memory::Temporary text(memory_,static_cast<std::uint32_t>(inventory.size()+1));
    for(unsigned i=0;i<inventory.size();++i)memory_.byte(text.address()+i,static_cast<std::uint8_t>(inventory[i]));memory_.byte(text.address()+static_cast<unsigned>(inventory.size()),0);
    memory_.word(p.record+0x6c,hp2::debug::bAddCodeineInventory(memory_,text.address(),0x2ade48));}

   hp2::debug::TexturePack pack(memory_,p.record);pack.AttachTextureTable(p.table,int(table.size/0xa4));pack.AssignTextureData(p.pixels,0,int(size));
  }
 };walk(walk,chunks);return packs_.size()-before;
}
std::uint32_t TextureBindings::lookup(std::uint32_t hash,int fallback,int unloaded){
 if(!initialized_)throw std::logic_error("Texture bindings are not initialized");
 return hp2::debug::GetTextureInfo(memory_,hash,fallback,unloaded);
}
void TextureBindings::unload_all(){
 for(const auto& p:packs_){const auto next=memory_.word(p.record),previous=memory_.word(p.record+4);
  memory_.word(previous,next);memory_.word(next+4,previous);
  hp2::debug::bRemoveCodeineInventory(memory_,memory_.signed_word(p.record+0x6c));
  if(p.pixels)memory_.release(p.pixels);
  if(p.table)memory_.release(p.table);
  memory_.release(p.record);
 }packs_.clear();
}
}

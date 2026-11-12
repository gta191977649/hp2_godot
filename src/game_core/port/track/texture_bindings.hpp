#pragma once
// Port-owned resident TexturePack resources in shared logical Memory.
// TASK-0099 E-0099-W13; Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/driving/memory.hpp"
#include <vector>
namespace hp2::debug {
// Debug001A95C8; E-0099-W13. Includes actual pack LRU/UsedFlag mutations,
// resident/unloaded distinction, actual Memory render targets and default lookup.
std::uint32_t GetTextureInfo(port::driving::Memory&,std::uint32_t,int return_default,int include_unloaded);
class TexturePack {
    port::driving::Memory& memory_;std::uint32_t address_;
public:
    TexturePack(port::driving::Memory& m,std::uint32_t address):memory_(m),address_(address){}
    // Debug001A9A70/001A9AE8/001A98A0/001A9930; E-0099-W13.
    std::uint32_t GetLoadedTexture(std::uint32_t)const;
    std::uint32_t GetTexture(std::uint32_t)const;
    void AttachTextureTable(std::uint32_t,int);
    void AssignTextureData(std::uint32_t,int,int);
};
}
namespace port::track {
class TextureBindings {
    port::driving::Memory& memory_;
    struct Pack {std::uint32_t record=0,table=0,pixels=0;};
    std::vector<Pack> packs_;
    bool initialized_=false;
public:
    explicit TextureBindings(port::driving::Memory& memory):memory_(memory){}
    TextureBindings(const TextureBindings&)=delete;
    TextureBindings& operator=(const TextureBindings&)=delete;
    // Actual00256768 packet packing and002563F0 inactive-platform branch are bound
    // through E-0099-W30. Active PS2 development-monitor transport needs a device.
    static constexpr bool inventory_transport_bound=false;
    static constexpr const char* inventory_transport_gap=
        "Debug00256350 active PS2 development-monitor transport requires a device binding";
    void init();
    // Debug00100830 resident-loader schema. Retail00100820 listing and
    //001AC220/001AC2B0/001AC3F0/001AC468 establish correspondence of the
    // consumed0xA4 header fields only; remaining bytes are retained opaque.
    // Complete original headers and128-aligned pixel/palette bytes are owned.
    // Actual inventory packet/hash and inactive platform behavior are preserved.
    // Active monitor device/debug UI and streaming I/O remain outside this owner.
    std::size_t load_bundle(const std::vector<std::uint8_t>& original_bytes);
    std::uint32_t lookup(std::uint32_t name,int return_default,int include_unloaded);
    // Port resource lifetime: callers must first release every borrowed pointer.
    void unload_all();
    std::size_t pack_count()const{return packs_.size();}
};
}

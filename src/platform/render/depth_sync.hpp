#pragma once
// Port-side dependency tracking; no scene objects, resources or recovered ABI.
#include <cstdint>
#include <map>
namespace port::render {
struct DepthDirtyState {
    enum Change : unsigned { Geometry=1,Transform=2,Visibility=4,Sorting=8,Material=16,Bounds=32,All=63 };
    std::map<std::uint64_t,unsigned> pending;
    std::uint64_t structure_epoch=0,discovered_epoch=0;
    void mark(std::uint64_t id,unsigned fields=All){pending[id]|=fields;}
    void forget(std::uint64_t id){pending.erase(id);}
    unsigned changes(std::uint64_t id,bool tracked)const{const auto found=pending.find(id);return tracked&&found!=pending.end()?found->second:All;}
    void observe_structure(bool previous_source,bool owned,bool geometry,bool same_viewport){if(previous_source||(!owned&&geometry&&same_viewport))++structure_epoch;}
    bool structure_changed()const{return structure_epoch!=discovered_epoch;}
    void discovered(){discovered_epoch=structure_epoch;}
    void consumed(){pending.clear();}
};
}

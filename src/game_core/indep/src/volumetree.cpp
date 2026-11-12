// E-0048-A06: exact ECOFF source marker ..\indep\src\volumetree.cpp.
#include "volumetree.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <stdexcept>
namespace hp2::recovered {
// ELF 0x001B9CD0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b9cd0.md.
void vAABB::Contains() {
    hp2::not_implemented("0x001B9CD0", "vAABB::Contains (signature and full body unresolved)");
}

// ELF 0x001B9D40; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b9d40.md.
void vAABB::Encloses() {
    hp2::not_implemented("0x001B9D40", "vAABB::Encloses (signature and full body unresolved)");
}
// Debug001B70E8; E-0048-A06; strict boundaries retained.
bool vAABB::Contains(float x,float y,float z)const {
    return std::abs(x-PositionX)<ExtentX&&std::abs(y-PositionY)<ExtentY&&std::abs(z-PositionZ)<ExtentZ;
}
// Debug001B72F0; E-0048-A06. Asset parser validates graph acyclicity first.
const vAABB* vAABBTree::QueryLeafHelper(const vAABB* aabb,float x,float y,float z)const {
    if(aabb->NumChildren>10)throw std::runtime_error("Volume tree child count exceeds ten");
    for(int child=0;child<aabb->NumChildren;++child){
        const auto index=aabb->ChildrenIndicies[child];
        if(index<0||std::size_t(index)>=NodeArray.size())throw std::runtime_error("Volume tree child index exceeds table");
        const auto* candidate=&NodeArray[index];
        if(candidate->Contains(x,y,z)){
            if(candidate->NumChildren<1)return candidate;
            if(const auto* leaf=QueryLeafHelper(candidate,x,y,z))return leaf;
        }
    }
    return nullptr;
}
// Debug001B73F0; E-0048-A06.
const vAABB* vAABBTree::QueryLeaf(float x,float y,float z)const {
    if(NodeArray.empty()||!NodeArray.front().Contains(x,y,z))return nullptr;
    if(NodeArray.front().NumChildren>0)return QueryLeafHelper(&NodeArray.front(),x,y,z);
    return &NodeArray.front();
}
}

namespace hp2::debug {
// Debug001B72F0; E-0099-W29; Contains001B70E8 is inlined below. Loader validates the graph.
std::uint32_t vAABBTree::QueryLeafHelper(std::uint32_t node,float x,float y,float z)const{
 const auto base=m_.word(address_);const auto count=m_.signed_half(node+14);
 for(int i=0;i<count;++i){const auto child=base+static_cast<std::int32_t>(m_.signed_half(node+28+static_cast<unsigned>(i)*2))*48;
  if(std::abs(x-m_.scalar(child))<m_.scalar(child+16)&&std::abs(y-m_.scalar(child+4))<m_.scalar(child+20)&&std::abs(z-m_.scalar(child+8))<m_.scalar(child+24)){
   if(m_.signed_half(child+14)<1)return child;if(const auto leaf=QueryLeafHelper(child,x,y,z))return leaf;
  }
 }return 0;
}
// Debug001B73F0; E-0099-W29. Strict boundary tests; first matching recursive leaf wins.
std::uint32_t vAABBTree::QueryLeaf(float x,float y,float z)const{
 const auto root=m_.word(address_);if(!root)return 0;
 if(!(std::abs(x-m_.scalar(root))<m_.scalar(root+16)&&std::abs(y-m_.scalar(root+4))<m_.scalar(root+20)&&std::abs(z-m_.scalar(root+8))<m_.scalar(root+24)))return 0;
 return m_.signed_half(root+14)>0?QueryLeafHelper(root,x,y,z):root;
}
}

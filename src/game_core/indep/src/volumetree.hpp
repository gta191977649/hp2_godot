#pragma once
#include <array>
#include <cstdint>
#include <vector>
// E-0048-A06. Port companion header; original source marker is
// ..\indep\src\volumetree.cpp. Explicit host containers, not PS2 object ABI.
namespace hp2::recovered {
class vAABB {
public:
    float PositionX{},PositionY{},PositionZ{};
    std::int16_t ParentIndex{},NumChildren{};
    float ExtentX{},ExtentY{},ExtentZ{};
    std::array<std::int16_t,10> ChildrenIndicies{};
    // Debug001B70E8; E-0048-A06.
    bool Contains(float x,float y,float z)const;
    // ELF 0x001B9CD0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b9cd0.md.
    // Original Debug reference: vAABB::Contains(float, float, float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Contains();
    // ELF 0x001B9D40; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b9d40.md.
    // Original Debug reference: vAABB::Encloses(vAABB *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Encloses();
};
class vAABBTree {
public:
    std::vector<vAABB> NodeArray; // Host-owned table replaces original EE pointer.
    // Debug001B72F0/001B73F0; E-0048-A06.
    const vAABB* QueryLeafHelper(const vAABB*,float x,float y,float z)const;
    const vAABB* QueryLeaf(float x,float y,float z)const;
};
}

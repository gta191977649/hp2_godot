#pragma once
// TASK-0046 E-0046-S01. Debug NFSHP2.ELF SHA256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Original TU indep/src/skids.cpp; original header marker ../indep/inc/Skids.hpp.
// ObjectView/array/services signatures are explicit host bindings, not PS2 ABI.
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/driving/skid_services.hpp"
namespace hp2::recovered {
class SkidSegment:public port::driving::ObjectView {
public:
    using ObjectView::ObjectView;
    // Debug001A7E98/001A7F08/001A7F80; E-0046-S01.
    void SetPoints(const port::driving::SkidVector&,const port::driving::SkidVector&);
    void GetPoints(port::driving::SkidVector&,port::driving::SkidVector* =nullptr)const;
    void GetEndPoints(port::driving::SkidVector&,port::driving::SkidVector&)const;
};
class SkidSet:public port::driving::ObjectView {
public:
    using ObjectView::ObjectView;
    // Debug001A8008 constructor /001A80D8 destructor; E-0046-S01.
    void Sub_001A8008(std::uint32_t maker,std::uint32_t car,const port::driving::SkidVector&,const port::driving::SkidVector&,int terrain,float intensity,port::driving::SkidServices&);
    void Sub_001A80D8(port::driving::SkidServices&);
    // Debug001A8150/001A8398/001A83B8/001A8B68; E-0046-S01.
    int AddSegment(const port::driving::SkidVector&,const port::driving::SkidVector&,float intensity,port::driving::SkidServices&);
    void FinishedAddingSkids();
    void GetLastPoints(port::driving::SkidVector&,port::driving::SkidVector&)const;
    void Render(std::uint8_t intensityReduction,port::driving::SkidServices&);
};
class SkidMaker:public port::driving::ObjectView {
public:
    using ObjectView::ObjectView;
    // Debug001A8648/001A8778; E-0046-S01.
    void MakeSkid(std::uint32_t car,const port::driving::SkidVector&,const port::driving::SkidVector&,int terrain,float intensity,port::driving::SkidServices&);
    void MakeNoSkid();
};
// Debug001A8578/001A8798/001A8850/001A88A8/001A88C8; E-0046-S01.
std::uint32_t CreateNewSkidSet(port::driving::Memory&,std::uint32_t maker,std::uint32_t car,const port::driving::SkidVector&,const port::driving::SkidVector&,int terrain,float intensity,port::driving::SkidServices&);
void InitSkids(port::driving::Memory&,port::driving::SkidServices&,const port::driving::SkidTextureLookup&);
void CloseSkids(port::driving::Memory&,port::driving::SkidServices&);
void DeleteThisSkid(port::driving::Memory&,std::uint32_t set,port::driving::SkidServices&);
void RenderSkids(port::driving::Memory&,const port::driving::SkidView&,port::driving::SkidServices&);
}

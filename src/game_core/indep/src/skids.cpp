// TASK-0046 E-0046-S01. Debug NFSHP2.ELF SHA256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Original TU indep/src/skids.cpp (E-0042-S001); host service boundaries explicit.
#include "game_core/indep/src/skids.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/port/race/world_construction.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace hp2::debug {
// Debug001A8798, E-0099-W06. Full original initializer over shared logical
// globals/pool. This overload does not use the separate synthetic SkidArena.
void InitSkids(::port::driving::Memory& memory,const ::port::race::TextureInfoLookup& lookup){
    if(memory.word(0x00327e68)==0){
        const auto pool=bNewSlotPool(memory,0x110,0x80,0x002add90,0x10);
        memory.word(0x00327e68,pool);
        memory.word(pool+0x14,memory.word(pool+0x14)&~std::uint32_t(SLOTPOOL_FLAG_OVERFLOW_IF_FULL));
    }
    for(unsigned i=0;i<33;++i){
        const auto destination=0x00308c30+i*4;
        memory.word(destination,0);
        const auto name=memory.word(0x003102e8+i*0x90+0x10);
        if(!name)continue;
        std::string text;
        for(unsigned offset=0;;++offset){
            if(offset==4096)hp2::not_implemented("0x0024C8B0","unterminated TerrainTypeInfo.SkidTextureName");
            const auto c=memory.byte(name+offset);if(!c)break;text.push_back(static_cast<char>(c));
        }
        const auto hash=hp2::frontend::bStringHash(text.c_str());
        if(!lookup)hp2::not_implemented("0x001A95C8","original GetTextureInfo service is not bound");
        memory.word(destination,lookup(hash,1,0));
    }
    memory.word(0x00327e6c,0);memory.word(0x00327e70,0);
}
}
namespace port::driving::skid_detail {
// Port memory/list adapters only. No alternate geometry/eligibility implementation.
SkidVector read(const Memory&m,std::uint32_t a){return {{m.scalar(a),m.scalar(a+4),m.scalar(a+8)}};}
void write(Memory&m,std::uint32_t a,const SkidVector&v){for(unsigned i=0;i<3;++i)m.scalar(a+4*i,v[i]);}
void remove(Memory&m,std::uint32_t p){const auto n=m.word(p),b=m.word(p+4);m.word(b,n);m.word(n+4,b);}
void front(Memory&m,std::uint32_t head,std::uint32_t p){const auto first=m.word(head);m.word(p,first);m.word(p+4,head);m.word(first+4,p);m.word(head,p);}
void back(Memory&m,std::uint32_t head,std::uint32_t p){const auto last=m.word(head+4);m.word(last,p);m.word(head+4,p);m.word(p,head);m.word(p+4,last);}
std::string text(const Memory&m,std::uint32_t p){std::string s;if(!p)return s;for(unsigned i=0;i<128;++i){auto c=m.byte(p+i);if(!c)return s;s.push_back(static_cast<char>(c));}throw std::runtime_error("Unterminated original skid texture name");}
// Host equivalent of finite EE truncation+low-byte store. Reject unsupported conversions.
std::uint8_t byte_cast(float x,const char*address){if(!std::isfinite(x)||x>=2147483648.0f||x< -2147483648.0f)hp2::not_implemented(address,"nonfinite/out-of-range EE skid conversion");return static_cast<std::uint8_t>(static_cast<std::int32_t>(x));}
}
namespace hp2::recovered {
using namespace port::driving;
using namespace port::driving::skid_detail;
// Debug001A7E98; E-0046-S01. Original compressed delta: trunc(v*64), low byte.
void SkidSegment::SetPoints(const SkidVector&p,const SkidVector&d){
    write(m_,address_,p);for(unsigned i=0;i<3;++i)m_.byte(address_+0xc+i,byte_cast(d[i]*64.0f,"0x001A7E98"));
}
// Debug001A7F08; E-0046-S01.
void SkidSegment::GetPoints(SkidVector&p,SkidVector*d)const{
    p=read(m_,address_);if(d)for(unsigned i=0;i<3;++i)(*d)[i]=static_cast<float>(m_.signed_byte(address_+0xc+i))*0.015625f;
}
// Debug001A7F80; E-0046-S01.
void SkidSegment::GetEndPoints(SkidVector&l,SkidVector&r)const{
    SkidVector p,d;GetPoints(p,&d);for(unsigned i=0;i<3;++i){l[i]=p[i]+d[i];r[i]=p[i]-d[i];}
}
// Debug001A8008; E-0046-S01. Original GetClan/Malloc node supplied by synthetic clan service.
void SkidSet::Sub_001A8008(std::uint32_t maker,std::uint32_t car,const SkidVector&p,const SkidVector&d,int terrain,float intensity,SkidServices&s){
    validate_skid_services(s);auto&a=*s.arena;const auto pnode=a.nodes+((address_-a.pool)/0x110)*16;
    m_.word(address_+0x30,maker);m_.word(address_+0x24,car);m_.word(address_+0x34,static_cast<std::uint32_t>(terrain));m_.word(address_+0xf0,0);
    write(m_,address_+0x40,p);write(m_,address_+0x60,p);write(m_,address_+0x50,p);write(m_,address_+0x100,p);
    m_.word(address_+0x28,a.clan_list);m_.word(address_+0x2c,pnode);m_.word(pnode+8,address_);back(m_,a.clan_list,pnode);
    AddSegment(p,d,intensity,s);
}
// Debug001A80D8; E-0046-S01. Explicit host destruction; pool lifetime remains a service.
void SkidSet::Sub_001A80D8(SkidServices&s){
    const auto maker=m_.word(address_+0x30);if(maker)SkidMaker(m_,maker).MakeNoSkid();
    remove(m_,m_.word(address_+0x2c));free_skid_slot(m_,*s.arena,address_);
}
// Debug001A8150; E-0046-S01. Original VU xyz grouping, merge/end/capacity decisions.
int SkidSet::AddSegment(const SkidVector&p,const SkidVector&d,float intensity,SkidServices&s){
    validate_skid_services(s);const auto n=m_.signed_word(address_+0xf0);float extra=0;SkidVector normal{};
    if(n>0){const auto last=read(m_,address_+0x70+(n-1)*16);for(unsigned i=0;i<3;++i)normal[i]=p[i]-last[i];
        float sum=normal[0]*normal[0];sum=sum+normal[1]*normal[1];sum=sum+normal[2]*normal[2];extra=std::sqrt(sum);
        if(extra!=0){const float inv=1.0f/extra;for(auto&v:normal)v=v*inv;}
        else if(std::all_of(normal.begin(),normal.end(),[](float v){return v==0.0f;})){
            // TASK0053 E0053D01: 001A81C8 VRSQRT Q,vf0.w,vf1.x clamps
            // 1/sqrt(0) to positive VU MAX, then001A81DC multiplies delta by Q.
            // VU exponent255 is finite; double carries its exact maximal value.
            // This handles only exact signed-zero components, not underflow.
            const double reciprocal=std::ldexp(2.0-std::ldexp(1.0,-23),128);
            for(auto&v:normal)v=static_cast<float>(static_cast<double>(v)*reciprocal);
        }else hp2::not_implemented("0x001A8150","nonzero underflowed VU reciprocal-square-root segment");
    }
    bool merge=false;
    if(n>1){const auto prev=read(m_,address_+0x10);float dot=normal[0]*prev[0];dot=dot+normal[1]*prev[1];dot=dot+normal[2]*prev[2];
        const float angle=1.0f-dot,total=m_.scalar(address_+0x20)+extra;
        if(m_.scalar(0x003257c8)<angle){FinishedAddingSkids();return 0;}
        merge=angle<m_.scalar(0x003257cc)||total<0.25f;if(3.0f<total)merge=false;
    }
    std::uint32_t seg;
    if(merge){seg=address_+0x60+n*16;m_.scalar(address_+0x20,m_.scalar(address_+0x20)+extra);}
    else{if(n==8)return 1;seg=address_+0x70+n*16;m_.word(address_+0xf0,n+1);if(n+1>1)write(m_,address_+0x10,normal);m_.scalar(address_+0x20,extra);}
    SkidSegment(m_,seg).SetPoints(p,d);m_.byte(seg+0xf,byte_cast(intensity*255.0f,"0x001A8150"));
    // Debug00252A58 bExpandBoundingBox; then synthetic Clan expansion boundary.
    auto lo=read(m_,address_+0x60),hi=read(m_,address_+0x50);
    for(unsigned i=0;i<3;++i){lo[i]=std::min(lo[i],p[i]-extra);hi[i]=std::max(hi[i],p[i]+extra);}
    write(m_,address_+0x60,lo);write(m_,address_+0x50,hi);
    auto&a=*s.arena;for(unsigned i=0;i<3;++i){m_.scalar(a.clan_bbox+4*i,std::min(m_.scalar(a.clan_bbox+4*i),lo[i]));m_.scalar(a.clan_bbox+16+4*i,std::max(m_.scalar(a.clan_bbox+16+4*i),hi[i]));}
    SkidVector center;for(unsigned i=0;i<3;++i)center[i]=(lo[i]+hi[i])*0.5f;write(m_,address_+0x100,center);return 0;
}
// Debug001A8398; E-0046-S01.
void SkidSet::FinishedAddingSkids(){const auto maker=m_.word(address_+0x30);if(maker){m_.word(maker,0);m_.word(address_+0x30,0);}}
// Debug001A8B68; E-0046-S01, inline ../indep/inc/Skids.hpp.
void SkidSet::GetLastPoints(SkidVector&p,SkidVector&d)const{SkidSegment(m_,address_+0x70+(m_.signed_word(address_+0xf0)-1)*16).GetPoints(p,&d);}
// Debug001A83B8; E-0046-S01/S02. Original ePoly constructor and Render3D submission.
void SkidSet::Render(std::uint8_t reduction,SkidServices&s){
    const auto terrain=m_.signed_word(address_+0x34);if(terrain<0||terrain>=33)throw std::runtime_error("Invalid original skid terrain");
    const auto&t=s.arena->textures[terrain];if(!t.present)return;
    ePoly poly;poly.Sub_0010FD48();const float z=m_.scalar(0x003257d0);const int n=m_.signed_word(address_+0xf0);
    for(int i=0;i<n-1;++i){const auto p=address_+0x70+i*16;
        SkidSegment(m_,p).GetEndPoints(poly.Vertices[0],poly.Vertices[3]);SkidSegment(m_,p+16).GetEndPoints(poly.Vertices[1],poly.Vertices[2]);
        for(auto&v:poly.Vertices)v[2]=v[2]+z;
        const auto a=m_.byte(p+15),b=m_.byte(p+31);
        poly.Colours[0][3]=a<reduction?0:static_cast<std::uint8_t>((a-reduction)>>1);
        poly.Colours[1][3]=b<reduction?0:static_cast<std::uint8_t>((b-reduction)>>1);
        poly.Colours[2][3]=poly.Colours[1][3];poly.Colours[3][3]=poly.Colours[0][3];poly.Render3D(t,terrain,s);
    }
}
// Debug001A8578; E-0046-S01.
std::uint32_t CreateNewSkidSet(Memory&m,std::uint32_t maker,std::uint32_t car,const SkidVector&p,const SkidVector&d,int terrain,float intensity,SkidServices&s){
    validate_skid_services(s);auto&a=*s.arena;auto slot=allocate_skid_slot(m,a);
    if(!slot){DeleteThisSkid(m,m.word(a.list+4),s);slot=allocate_skid_slot(m,a);}
    if(!slot)throw std::runtime_error("Original skid pool failed to release an LRU slot");
    SkidSet(m,slot).Sub_001A8008(maker,car,p,d,terrain,intensity,s);front(m,a.list,slot);return slot;
}
// Debug001A8648; E-0046-S01.
void SkidMaker::MakeSkid(std::uint32_t car,const SkidVector&p,const SkidVector&d,int terrain,float intensity,SkidServices&s){
    auto set=m_.word(address_);if(!set){m_.word(address_,CreateNewSkidSet(m_,address_,car,p,d,terrain,intensity,s));return;}
    if(car==m_.word(set+0x24)&&terrain==m_.signed_word(set+0x34))if(!SkidSet(m_,set).AddSegment(p,d,intensity,s))return;
    SkidVector last,delta;SkidSet(m_,set).GetLastPoints(last,delta);
    const auto n=m_.signed_word(set+0xf0);const float old_intensity=static_cast<float>(m_.byte(set+0x70+(n-1)*16+15))*m_.scalar(0x003257d4);
    SkidSet(m_,set).FinishedAddingSkids();set=CreateNewSkidSet(m_,address_,car,last,delta,terrain,old_intensity,s);
    SkidSet(m_,set).AddSegment(p,d,intensity,s);m_.word(address_,set);
}
// Debug001A8778; E-0046-S01.
void SkidMaker::MakeNoSkid(){const auto set=m_.word(address_);if(set)SkidSet(m_,set).FinishedAddingSkids();}
// Debug001A8798; E-0046-S01. Original 33 name/hash/GetTextureInfo requests.
void InitSkids(Memory&m,SkidServices&s,const SkidTextureLookup&lookup){
    if(!s.arena)hp2::not_implemented("0x001A8798","Skid pool allocation service");
    if(!s.arena->list)allocate_skid_arena(m,*s.arena);
    if(!lookup)hp2::not_implemented("0x001A8798","Original skid texture lookup service");
    for(unsigned i=0;i<33;++i){auto&t=s.arena->textures[i];t={};const auto name=text(m,m.word(0x003102e8+i*0x90+0x10));
        if(!name.empty()){
            // Debug0024C8B0 checked against the existing canonical leaf; retain one hash body.
            const auto hash=hp2::frontend::bStringHash(name.c_str());
            t=lookup(name,hash);t.name=name;t.hash=hash;
        }
    }
}
// Debug001A8850; E-0046-S01. Host arena deallocation is logical; no dangling publication.
void CloseSkids(Memory&,SkidServices&s){
    // Original bDeleteSlotPool does not walk SkidSet destructors. Host Memory
    // owns arena bytes until scene reset; dropping its binding models pool release.
    if(s.arena)*s.arena={};
}
// Debug001A88A8; E-0046-S01.
void DeleteThisSkid(Memory&m,std::uint32_t set,SkidServices&s){remove(m,set);SkidSet(m,set).Sub_001A80D8(s);}
// Debug001A88C8; E-0046-S01/S02. Single synthetic clan traversal; original per-view
// bbox/car mask, integer pixel thresholds, distance fade and render-driven global LRU.
void RenderSkids(Memory&m,const SkidView&view,SkidServices&s){
    validate_skid_services(s);if(view.nis_playing)return;auto&a=*s.arena;
    auto node=m.word(a.clan_list);while(node!=a.clan_list){const auto set=m.word(node+8);const auto next=m.word(node);
        bool visible=IsBoundingBoxVisible(read(m,set+0x60),read(m,set+0x50),view.clip_matrix);
        const auto car=m.word(set+0x24);if(car)visible=visible&&((m.word(car+0x514)>>(view.view_id&31))&1);
        if(visible){const auto center=read(m,set+0x100);float squared=(center[0]-view.position[0])*(center[0]-view.position[0]);
            squared=squared+(center[1]-view.position[1])*(center[1]-view.position[1]);squared=squared+(center[2]-view.position[2])*(center[2]-view.position[2]);
            const float pixels=static_cast<float>(eView::GetPixelSize(1,std::sqrt(squared),view.H));
            if(4<pixels){std::uint8_t reduction=0;if(pixels<=10)reduction=byte_cast(256.0f-(pixels-4.0f)*m.scalar(0x003257d8),"0x001A88C8");
                SkidSet(m,set).Render(reduction,s);remove(m,set);front(m,a.list,set);
            }
        }
        node=next;
    }
}
}

namespace hp2::debug {
// Debug001A8850, E-0099-W18. Exact pool/null and reverse texture-table clears.
void CloseSkids(::port::driving::Memory& m){
 if(m.word(0x327e68)){bDeleteSlotPool(m,m.word(0x327e68));m.word(0x327e68,0);}
 for(int i=32;i>=0;--i)m.word(0x308c30+4*i,0);
}
}

// TASK-0099 E-0099-W71. Actual shared original ownership, separate from retained
// synthetic fixture overloads above. Debug SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/indep/inc/Skids.hpp"
#include "game_core/port/driving/ee_vu_skid_lane.hpp"
#include "game_core/indep/inc/Clans.hpp"
#include "game_core/src/bList.hpp"
#include <cstring>
namespace hp2::debug {
namespace {
// Port-only logical-memory/list/policy adapters, no original helper ABI claim.
using M=::port::driving::Memory;
void skid_unlink(M&m,std::uint32_t p){const auto n=m.word(p),b=m.word(p+4);m.word(b,n);m.word(n+4,b);}
void skid_front(M&m,std::uint32_t p){const auto h=0x327e60u,n=m.word(h);m.word(h,p);m.word(n+4,p);m.word(p+4,h);m.word(p,n);}
float skid_policy(std::optional<std::uint32_t> p,const char*site){if(!p)hp2::not_implemented(site,"Explicit port policy for original uninitialized skid stack W is required");float value;std::memcpy(&value,&*p,4);if(!std::isfinite(value))hp2::not_implemented(site,"Exceptional original skid stack W policy outside finite projection");return value;}
}
void SkidSet::Sub_001A8008(std::uint32_t maker,std::uint32_t car,std::uint32_t p,std::uint32_t d,int terrain,float intensity,std::optional<std::uint32_t> radius){m_.word(address_+0x30,maker);m_.word(address_+0x24,car);m_.word(address_+0x34,std::uint32_t(terrain));m_.word(address_+0xf0,0);m_.copy(address_+0x40,p,16);m_.copy(address_+0x60,p,16);m_.copy(address_+0x50,p,16);m_.copy(address_+0x100,p,16);const auto clan=GetClan(m_,p);m_.word(address_+0x28,clan);const auto node=bPNode::Malloc(m_),head=clan+8,tail=m_.word(head+4);m_.word(address_+0x2c,node);m_.word(tail,node);m_.word(head+4,node);m_.word(node,head);m_.word(node+8,address_);m_.word(node+4,tail);AddSegment(p,d,intensity,radius);}
void SkidSet::Sub_001A80D8(int flags){if(const auto maker=m_.word(address_+0x30))SkidMaker(m_,maker).MakeNoSkid();const auto node=m_.word(address_+0x2c);skid_unlink(m_,node);bPNode::Free(m_,node);if(flags&1)SlotPool(m_,m_.word(0x327e68)).Free(address_);}
int SkidSet::AddSegment(std::uint32_t p,std::uint32_t d,float intensity,std::optional<std::uint32_t> radius){auto&m=m_;const auto a=address_;const auto count=m.signed_word(a+0xf0);float extra=0;std::array<float,4>normal{};std::uint32_t normal_w_bits=0;
 if(count>0){const auto last=a+0x60+std::uint32_t(count)*16;for(unsigned i=0;i<4;++i){const float left=m.scalar(p+4*i),right=m.scalar(last+4*i);if(!std::isfinite(left)||!std::isfinite(right)||std::fpclassify(left)==FP_SUBNORMAL||std::fpclassify(right)==FP_SUBNORMAL)hp2::not_implemented("0x001A81A0","Original extended/denormal VU skid subtraction lane requires its instruction carrier");normal[i]=left-right;}const float xx=normal[0]*normal[0],yy=normal[1]*normal[1],zz=normal[2]*normal[2];const float square=(xx+yy)+zz;if(!std::isfinite(square))hp2::not_implemented("0x001A81C4","Exceptional original skid distance SQRT");extra=std::sqrt(square);
  if(extra){const float q=1.f/extra;for(unsigned lane=0;lane<3;++lane){normal[lane]*=q;if(!std::isfinite(normal[lane]))hp2::not_implemented("0x001A81DC","Exceptional original XYZ skid normalization remains unresolved");}const float input_w=normal[3];normal[3]*=q;if(!std::isfinite(normal[3]))normal_w_bits=::port::driving::vu_skid_extreme_product_001a81dc(input_w,double(q)).bits;else std::memcpy(&normal_w_bits,&normal[3],4);}
  else if(normal[0]==0&&normal[1]==0&&normal[2]==0){const double q=std::ldexp(2.-std::ldexp(1.,-23),128);for(unsigned lane=0;lane<3;++lane)normal[lane]=float(double(normal[lane])*q);const float input_w=normal[3];const double product=double(input_w)*q;if(!std::isfinite(product)||std::fabs(product)>double(std::numeric_limits<float>::max()))normal_w_bits=::port::driving::vu_skid_extreme_product_001a81dc(input_w,q).bits;else{normal[3]=float(product);std::memcpy(&normal_w_bits,&normal[3],4);}}

  else hp2::not_implemented("0x001A81C8","Nonzero underflowed original VU skid squared norm");
 }
 bool merge=false;if(count>1){float dot=normal[0]*m.scalar(a+0x10);dot=dot+normal[1]*m.scalar(a+0x14);dot=dot+normal[2]*m.scalar(a+0x18);const float angle=1-dot,total=m.scalar(a+0x20)+extra;if(m.scalar(0x3257c8)<angle){FinishedAddingSkids();return 0;}merge=total<.25f||angle<m.scalar(0x3257cc);if(3<total)merge=false;}
 std::uint32_t segment;if(merge){segment=a+0x60+std::uint32_t(count)*16;m.scalar(a+0x20,m.scalar(a+0x20)+extra);}else{if(count==8)return 1;segment=a+0x70+std::uint32_t(count)*16;m.word(a+0xf0,std::uint32_t(count+1));if(count+1>1){for(unsigned lane=0;lane<3;++lane)m.scalar(a+0x10+4*lane,normal[lane]);m.word(a+0x1c,normal_w_bits);}m.scalar(a+0x20,extra);}
 ::port::driving::SkidVector pos{{m.scalar(p),m.scalar(p+4),m.scalar(p+8)}},delta{{m.scalar(d),m.scalar(d+4),m.scalar(d+8)}};hp2::recovered::SkidSegment(m,segment).SetPoints(pos,delta);m.byte(segment+15,::port::driving::skid_detail::byte_cast(intensity*255,"0x001A8314"));
 const float radius_w=skid_policy(radius,"0x00252A74");for(unsigned i=0;i<4;++i){const float padding=i==3?radius_w:extra,point=m.scalar(p+i*4),lo=m.scalar(a+0x60+i*4),hi=m.scalar(a+0x50+i*4);if(!std::isfinite(point)||!std::isfinite(lo)||!std::isfinite(hi))hp2::not_implemented("0x00252A84","Exceptional original full-lane skid bounding box");m.scalar(a+0x60+i*4,std::min(lo,point-padding));m.scalar(a+0x50+i*4,std::max(hi,point+padding));}
 Clan(m,m.word(a+0x28)).ExpandBoundingBox(a+0x60,a+0x50);for(unsigned i=0;i<4;++i)m.scalar(a+0x100+i*4,(m.scalar(a+0x60+i*4)+m.scalar(a+0x50+i*4))*.5f);return 0;
}
void SkidSet::FinishedAddingSkids(){if(const auto maker=m_.word(address_+0x30)){m_.word(maker,0);m_.word(address_+0x30,0);}}
void SkidSet::GetLastPoints(std::uint32_t p,std::uint32_t d){const auto segment=address_+0x70+std::uint32_t(m_.signed_word(address_+0xf0)-1)*16;::port::driving::SkidVector position,delta;hp2::recovered::SkidSegment(m_,segment).GetPoints(position,&delta);for(unsigned i=0;i<3;++i){m_.scalar(p+4*i,position[i]);if(d)m_.scalar(d+4*i,delta[i]);}}
std::uint32_t CreateNewSkidSet(M&m,std::uint32_t maker,std::uint32_t car,std::uint32_t p,std::uint32_t d,int terrain,float intensity,std::optional<std::uint32_t> radius){const auto pool=m.word(0x327e68),head=0x327e60u;if(!m.word(head))hp2::not_implemented("0x001A8B30","Original SkidSetList process static initialization is required");if(!pool)hp2::not_implemented("0x001A8798","Original shared SkidSet pool startup is required");if(m.word(pool+0x18)==m.word(pool+0x2c)){const auto old=m.word(head+4);if(old==head)hp2::not_implemented("0x001A8590","Original full skid pool has no published LRU owner");skid_unlink(m,old);SkidSet(m,old).Sub_001A80D8(3);}const auto set=SlotPool(m,pool).Malloc();SkidSet(m,set).Sub_001A8008(maker,car,p,d,terrain,intensity,radius);skid_front(m,set);return set;}
void SkidMaker::MakeSkid(std::uint32_t car,std::uint32_t p,std::uint32_t d,int terrain,float intensity,std::optional<std::uint32_t> radius,std::optional<std::uint32_t> restart_w){auto set=m_.word(address_);if(!set){m_.word(address_,CreateNewSkidSet(m_,address_,car,p,d,terrain,intensity,radius));return;}if(car==m_.word(set+0x24)&&terrain==m_.signed_word(set+0x34))if(!SkidSet(m_,set).AddSegment(p,d,intensity,radius))return;
 ::port::driving::Memory::Temporary stack(m_,32);const auto last=stack.address(),delta=last+16;SkidSet(m_,set).GetLastPoints(last,delta);const auto n=m_.signed_word(set+0xf0);const float old=float(m_.byte(set+0x70+std::uint32_t(n-1)*16+15))*m_.scalar(0x3257d4);SkidSet(m_,set).FinishedAddingSkids();(void)skid_policy(restart_w,"0x001A8724");m_.word(last+12,*restart_w);set=CreateNewSkidSet(m_,address_,car,last,delta,terrain,old,radius);SkidSet(m_,set).AddSegment(p,d,intensity,radius);m_.word(address_,set);
}
void SkidMaker::MakeNoSkid(){if(const auto set=m_.word(address_))SkidSet(m_,set).FinishedAddingSkids();}
void DeleteThisSkid(M&m,std::uint32_t set){skid_unlink(m,set);SkidSet(m,set).Sub_001A80D8(3);}
}

namespace hp2::debug {
// Debug001A8B30: required original process static, never lazy-init insideSkids.
void Sub_001A8B30(::port::driving::Memory&m){m.word(0x327e64,0x327e60);m.word(0x327e60,0x327e60);}
}

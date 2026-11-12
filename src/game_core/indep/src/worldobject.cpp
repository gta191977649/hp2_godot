// TASK-0099 E-0099-W57. Exact original indep/src/worldobject.cpp owner from
// fresh raw .mdebug FDR/SYMR/marker/path comparisons. Debug NFSHP2.ELF SHA256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/indep/inc/WorldObject.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug001CACF0, E-0099-W57. Explicit original CRT member, never auto-init.
void Sub_001CACF0(::port::driving::Memory& m){m.word(0x328294,0x328290);m.word(0x328290,0x328290);}
// Debug001CA8E8, E-0099-W57. Original always creates/publishes this pool.
void InitWorldObjects(::port::driving::Memory& m){m.word(0x328298,bNewSlotPool(m,0x60,128,0x2afb58,0x10));}
// Debug001CA730, E-0099-W57. Actual pooled eModel construction, original
// full matrix copy/nullable Enabled branch and32-bit AddLighting store.
void WorldObject::Sub_001CA730(std::uint32_t name,std::uint32_t matrix,bool light){
 const auto pool=m_.word(0x326700);if(!pool)hp2::not_implemented("0x00109E18","Original eModelSlotPool startup is required by WorldObject");const auto model=bOMalloc(m_,pool);if(!model)hp2::not_implemented("0x001CA730","Original eModel allocation returned null before constructor stores");m_.word(model+8,0);m_.word(model+12,0);::port::race::eModel(m_,model).Init(name);m_.word(address_+8,model);
 if(matrix){m_.copy(address_+0x20,matrix,64);m_.word(address_+12,1);}else m_.word(address_+12,0);m_.word(address_+16,light?1:0);
 const auto tail=m_.word(0x328294);if(!tail)hp2::not_implemented("0x001CACF0","Original WorldObjectList process initialization is required");m_.word(tail,address_);m_.word(address_+4,tail);m_.word(0x328294,address_);m_.word(address_,0x328290);
}
// Debug001CA7F8/001CAD08, E-0099-W57. Model destructor/pooled release then
// original object unlink, followed by object pool deletion only whenflag1.
void WorldObject::Sub_001CA7F8(int flags){if(const auto model=m_.word(address_+8))::port::race::eModel(m_,model).Sub_00115100(3);const auto next=m_.word(address_),prev=m_.word(address_+4);m_.word(prev,next);m_.word(next+4,prev);if(flags&1)SlotPool(m_,m_.word(0x328298)).Free(address_);}
// Debug001CA918, E-0099-W57. Actual source-owned objects precede pool delete.
void CloseWorldObjects(::port::driving::Memory& m){while(m.word(0x328290)!=0x328290){const auto object=m.word(0x328290);if(!object)hp2::not_implemented("0x001CACF0","Original WorldObjectList process initialization is required");WorldObject(m,object).Sub_001CA7F8(3);}bDeleteSlotPool(m,m.word(0x328298));m.word(0x328298,0);}
}

namespace hp2::debug {
// Debug001CA870, E-0099-W76. Actual pooled model's source hash, not a cached host label.
std::uint32_t WorldObject::GetNameHash()const{return m_.word(m_.word(address_+8)+8);}
}

namespace hp2::debug {
// Debug001CA8A0, W79. Original stack eModel joins/reconnects/unlinks the actual
// live model list; not an eFindSolid shortcut that drops constructor side effects.
int WorldObject::DoesArtExist(::port::driving::Memory& m,std::uint32_t hash){::port::driving::Memory::Temporary local(m,24);const auto a=local.address();m.word(a+8,0);m.word(a+12,0);::port::race::eModel model(m,a);model.Init(hash);const bool exists=m.word(a+12)!=0;model.Sub_00115100(2);return exists;}
}

#include "game_core/port/race/nis_override.hpp"
namespace hp2::debug {
// Debug001CAB50. Preserve the model-less object point-box and actual source model bounds.
bool AnyWorldObjectsInBox(::port::driving::Memory& m,const std::array<float,4>& low,const std::array<float,4>& high){const auto head=0x328290u;for(auto object=m.word(head);object!=head;object=m.word(object)){if(!object)hp2::not_implemented("0x001CAB50","actual WorldObjectList must be initialized");if(!m.word(object+0xc))continue;auto minimum=m.vector(object+0x50),maximum=minimum;if(const auto model=m.word(object+8))::port::race::eModel(m,model).GetBoundingBox(minimum,maximum);bool overlaps=true;for(unsigned k=0;k<3;++k)if(!(minimum[k]<high[k]&&low[k]<maximum[k]))overlaps=false;if(overlaps)return true;}return false;}
}

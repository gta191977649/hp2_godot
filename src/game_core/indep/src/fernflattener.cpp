// Debug source marker ../indep/src/fernflattener.cpp; E-0099-W06.
#include "game_core/port/race/world_construction.hpp"
#include "game_core/indep/src/fernflattener.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/not_implemented.hpp"
#include <string>
namespace hp2::debug {
// Debug001D6698, E-0099-W06. The original function is an empty return, confirmed
// by read-only decompilation. This does not imply other fern functions are empty.
void InitFernFlatteners(){}
// Debug001D7170, E-0099-W06. Original Init stores hashes; it does not load
// or construct a rigid animation object. Emitters use the loaded resource owner.
void FernShakerInfo::Init(std::uint32_t animation,std::uint32_t group,const ::port::race::EmitterGroupLookup& lookup){
    std::uint32_t hashes[2]{};
    const std::uint32_t names[2]{animation,group};
    for(unsigned i=0;i<2;++i){
        if(!names[i])hp2::not_implemented("0x0024C8B0","FernShakerInfo name is not bound");
        std::string text;
        for(unsigned offset=0;;++offset){
            if(offset==4096)hp2::not_implemented("0x0024C8B0","unterminated FernShakerInfo name");
            const auto c=memory_.byte(names[i]+offset);if(!c)break;text.push_back(static_cast<char>(c));
        }
        hashes[i]=hp2::frontend::bStringHash(text.c_str());
        memory_.word(address_+4*i,hashes[i]);
    }
    if(!lookup)hp2::not_implemented("0x00127438","original afxGetEmitters service is not bound");
    lookup(memory_,address_+8,hashes[1]);
}
// Debug001D6D30, E-0099-W06. The original array allocation includes a16-byte
// cookie area, two16-byte infos, and an exact two-element emitter-list setup.
void InitFernShakers(::port::driving::Memory& memory,const ::port::race::EmitterGroupLookup& lookup){
    memory.word(0x00328460,bNewSlotPool(memory,0x1c,0x20,0x002b0948,0x10));
    memory.word(0x0032845c,2);
    const auto allocation=memory.allocate(0x30);const auto table=allocation+16;
    memory.word(0x00328458,table);memory.word(allocation,2);
    for(unsigned i=0;i<2;++i){const auto list=table+i*16+8;memory.word(list,list);memory.word(list+4,list);}
    FernShakerInfo first(memory,table);first.Init(0x002b0978,0x002b0978,lookup);
    FernShakerInfo second(memory,table+16);second.Init(0x002b0988,0x002b0988,lookup);
}
}

#include "game_core/indep/inc/Table.hpp"
#include "game_core/port/track/emitter_bindings.hpp"
namespace hp2::debug {
// Debug001D70D0, E-0099-W18. Full recovered initializer, not list-only repair.
void Sub_001D70D0(::port::driving::Memory& m){
 hp2::recovered::Table(m,0x311690).Sub_0018D810(0x311660,5,0,1);
 hp2::recovered::Table(m,0x3116a8).Sub_0018D810(0x311678,5,0,1);
 for(auto head:{0x328450u,0x328468u}){m.word(head,head);m.word(head+4,head);}
}
// Debug001D6E38/001D71C0/001D6DE8, E-0099-W18.
void CloseFernShakers(::port::driving::Memory& m){
 const auto table=m.word(0x328458);
 if(table){const auto count=m.word(table-16);for(auto i=count;i>0;--i)afxDeleteEmitters(m,table+(i-1)*16+8);m.release(table-16);}
 m.word(0x328458,0);m.word(0x32845c,0);
 FlushFernShakers(m);
 bDeleteSlotPool(m,m.word(0x328460));m.word(0x328460,0);
}
// Debug001D66A0/001D6340, E-0099-W18. InitFernFlatteners is proven empty;
// nonempty simulated lists require their actual scenery/matrix destructor.
void CloseFernFlatteners(::port::driving::Memory& m){
 if(m.word(0x328450)!=0x328450)hp2::not_implemented("0x001D6340","active FernFlattener destructor is not reconstructed");
 if(m.word(0x328448)){bDeleteSlotPool(m,m.word(0x328448));m.word(0x328448,0);}
}
}

#include "game_core/indep/inc/WorldObject.hpp"
#include "game_core/indep/inc/AcidFX.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include <cmath>
namespace hp2::debug {
namespace {
// E-0099-W65 original inlined SceneryInstance::GetMatrix; all16 lanes assigned.
std::array<float,16> fern_matrix(::port::driving::Memory& m,std::uint32_t instance){std::array<float,16>a{};for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)a[row*4+col]=float(m.signed_half(instance+0x1c+2*(row*3+col)))*0x1p-14f;for(unsigned i=0;i<3;++i)a[12+i]=m.scalar(instance+0x10+4*i);a[15]=1;return a;}
}
// Debug001D6790/001D6798: fresh rawJRRA/NOP at both entries. Original names.
void FlattenFern(::port::driving::Memory&,std::uint32_t){}
void ServiceFernFlatteners(::port::driving::Memory&){}
// Debug001D6DE8. Source caller detaches before derived destruction.
void FlushFernShakers(::port::driving::Memory& m){while(m.word(0x328468)!=0x328468){const auto body=m.word(0x328468),next=m.word(body),prev=m.word(body+4);m.word(prev,next);m.word(next+4,prev);FernShaker(m,body).Sub_001D69C8(3);}}
// Debug001D67A0. Original source ../indep/src/fernflattener.cpp; E-0099-W65.
void FernShaker::Sub_001D67A0(std::uint32_t code){
 const auto section=m_.word(0x2f0860+4*(code>>16));m_.word(address_+12,m_.word(0x327d04));m_.word(address_+8,code);m_.word(address_+16,0);m_.word(address_+20,0);m_.word(address_+24,0);if(!section)return;
 const auto instance=m_.word(section+0x1c)+0x30*(code&0xffff);if(!instance)return;
 const auto model=m_.word(m_.word(section+0x14)+std::uint32_t(std::int32_t(m_.signed_half(instance+12))*0x28)+0x14);if(!model)return;
 if(!::port::race::eModel(m_,model).GetRigidAnimation())return;
 m_.word(address_+16,instance);::port::driving::Memory::Temporary matrix(m_,64);m_.matrix(matrix.address(),fern_matrix(m_,instance));const auto object=bOMalloc(m_,m_.word(0x328298));WorldObject(m_,object).Sub_001CA730(m_.word(model+8),matrix.address(),false);m_.word(address_+20,object);m_.half(instance+14,std::uint16_t(m_.half(instance+14)|0x10));
 for(int i=0;i<m_.signed_word(0x32845c);++i){const auto info=m_.word(0x328458)+16*std::uint32_t(i);const auto animation=::port::race::eModel(m_,model).GetRigidAnimation();if(m_.word(info)==m_.word(animation+4)){m_.word(address_+24,info);break;}}
}
// Debug001D69C8. Caller removes FernShaker node; bNode dtor(flags0) is empty.
void FernShaker::Sub_001D69C8(int flags){if(const auto instance=m_.word(address_+16)){m_.half(instance+14,std::uint16_t(m_.half(instance+14)&0xffef));if(const auto object=m_.word(address_+20))WorldObject(m_,object).Sub_001CA7F8(3);}if(flags&1)SlotPool(m_,m_.word(0x328460)).Free(address_);}
// Debug001D6A40. VU squared norm consumesXYZ only; all inheritedW is explicit0.
// Optional policy is forwarded solely to original raw AcidFX's unwritten discW.
void FernShaker::Shake(std::uint32_t velocity,std::optional<std::uint32_t> policy){const auto instance=m_.word(address_+16);if(!instance)return;const auto model=m_.word(m_.word(address_+20)+8),animation=::port::race::eModel(m_,model).GetRigidAnimation();if(!animation)return;
 const float x=m_.scalar(velocity),y=m_.scalar(velocity+4),z=m_.scalar(velocity+8);const float xx=x*x,yy=y*y,zz=z*z;const float norm=(xx+yy)+zz;if(!std::isfinite(norm))hp2::not_implemented("0x001D6AAC","Original nonfinite EE Fern velocity norm is outside the recovered native projection");const float speed=std::sqrt(norm)*m_.scalar(0x325b30);if(!std::isfinite(speed))hp2::not_implemented("0x001D6ABC","Original nonfinite EE Fern speed is outside the recovered native projection");if(!(5<speed))return;
 if(::port::race::eModel(m_,model).GetRigidAnimationTime()<0){const int frame=30<speed?::port::driving::cvt_w_s(float(m_.signed_half(animation))*.5f):0;m_.word(address_+12,m_.word(0x327d04));::port::race::eModel(m_,model).SetRigidAnimationFrameNumber(frame);}
 float intensity=(speed-5)*m_.scalar(0x325b34);intensity=std::fmax(intensity,0.f);intensity=std::fmin(1.f,intensity);if(!(0<intensity))return;const auto info=m_.word(address_+24);if(!info)return;
 const auto head=info+8;for(auto emitter=m_.word(head);emitter!=head;emitter=m_.word(emitter)){m_.matrix(emitter+0x30,fern_matrix(m_,instance));for(unsigned axis=0;axis<3;++axis)m_.word(emitter+0x70+4*axis,m_.word(velocity+4*axis));m_.word(emitter+0x7c,0);AcidEffectSystem(m_,0x2e6f40).TriggerEmitter(emitter,-intensity,0,policy);}
}
// Debug001D6EA8. Original signed elapsed expiry>240, saved-next detach/dtor,
// first matching scenery code, full pool skip, constructor and tail publication.
void ShakeFern(::port::driving::Memory& m,std::uint32_t code,std::uint32_t velocity,std::optional<std::uint32_t> policy){const auto head=0x328468u;if(!m.word(head))hp2::not_implemented("0x001D70D0","Original FernShakerList static initialization is required");for(auto node=m.word(head);node!=head;){const auto next=m.word(node);if(std::int32_t(m.word(0x327d04)-m.word(node+12))>240){const auto prev=m.word(node+4);m.word(prev,next);m.word(next+4,prev);FernShaker(m,node).Sub_001D69C8(3);}node=next;}
 auto body=m.word(head);for(;body!=head;body=m.word(body))if(m.word(body+8)==code)break;
 if(body==head){const auto pool=m.word(0x328460);if(!pool)hp2::not_implemented("0x001D6D30","Original FernShaker pool startup is required");if(m.word(pool+0x18)==m.word(pool+0x2c))return;body=SlotPool(m,pool).Malloc();FernShaker(m,body).Sub_001D67A0(code);const auto tail=m.word(head+4);m.word(tail,body);m.word(head+4,body);m.word(body+4,tail);m.word(body,head);}FernShaker(m,body).Shake(velocity,policy);
}
}

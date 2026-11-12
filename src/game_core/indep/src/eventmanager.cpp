// TASK-0099 E-0099-W29. Original ../indep/src/eventmanager.cpp independently
// confirmed through Debug ECOFF FDR/SYMR/definition-marker bytes via read-only MCP.
// Debug NFSHP2.ELF SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/race/events.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cstdio>

namespace port::race {
namespace {
using ::port::driving::Memory;
// Port-owned primitives implementing the original inlined bList stores.
void unlink(Memory& m,std::uint32_t node){const auto next=m.word(node),previous=m.word(node+4);m.word(previous,next);m.word(next+4,previous);}
void append(Memory& m,std::uint32_t head,std::uint32_t node){const auto previous=m.word(head+4);m.word(previous,node);m.word(head+4,node);m.word(node+4,previous);m.word(node,head);}
void merge(Memory& m,std::uint32_t from,std::uint32_t to){while(m.word(from)!=from){const auto node=m.word(from);unlink(m,node);append(m,to,node);}}
// Port-only exception lifetime protection. Original execution has no exceptions;
// missing native callees must not strand persistent events on a released stack head.
struct EventProcessScope {
 Memory& memory;Memory::Temporary storage;std::uint32_t temporary,locked;bool finished=false;
 explicit EventProcessScope(Memory& m):memory(m),storage(m,16),temporary(storage.address()),locked(temporary+8){
  memory.word(temporary,temporary);memory.word(temporary+4,temporary);memory.word(locked,locked);memory.word(locked+4,locked);
 }
 ~EventProcessScope(){if(!finished){merge(memory,locked,0x003280c8);merge(memory,temporary,0x003280c8);memory.word(0x003280d0,0x003280c8);memory.word(0x003280d4,0);}}
};
}
}
namespace hp2::debug {
// Debug001B7DE0; E-0099-W29. Does not reset stats, pools or initialized CurrentEventQueue.
void Sub_001B7DE0(::port::driving::Memory& m){
 for(auto head:{0x003280b0u,0x003280b8u,0x003280c0u,0x003280c8u})m.word(head+4,head);
 for(auto head:{0x003280b0u,0x003280b8u,0x003280c0u,0x003280c8u})m.word(head,head);
}
// Debug001B7490; E-0099-W29.
void emEventManagerInit(::port::driving::Memory& m){
 m.word(0x003280a8,bNewSlotPool(m,0x24,0x3c,0x002ae720,0x10));
 m.word(0x003280ac,bNewSlotPool(m,0x18,0x14,0x002ae730,0x10));
}
// Debug001B74E0; E-0099-W29. Original deliberately leaves stale global pool values.
void emEventManagerClose(::port::driving::Memory& m){if(const auto pool=m.word(0x003280a8))bDeleteSlotPool(m,pool);if(const auto pool=m.word(0x003280ac))bDeleteSlotPool(m,pool);}
// Debug001B76E8; E-0099-W29. Existing function identity increments references and ignores a new mask.
int emAddHandler(::port::driving::Memory& m,std::uint32_t function,std::uint32_t mask){
 if(!function||!mask)return 0;constexpr std::uint32_t head=0x003280c0;
 for(auto node=m.word(head);node!=head;node=m.word(node))if(m.word(node+8)==function){m.word(node+16,m.word(node+16)+1);return 1;}
 const auto pool=m.word(0x003280ac);if(!pool)hp2::not_implemented("0x001B76E8","EventHandlerSlotPool is not initialized");
 const auto node=bOMalloc(m,pool);if(!node)return 0;
 m.word(node+12,mask);m.word(node+16,1);const auto previous=m.word(head+4);m.word(previous,node);
 const auto high=m.signed_word(0x0030b6d0);m.word(0x0030b6c4,m.word(0x0030b6c4)+1);m.word(head+4,node);m.word(node,head);m.word(node+8,function);m.word(node+4,previous);
 if(high<m.signed_word(0x0030b6c4))m.word(0x0030b6d0,m.word(0x0030b6c4));return 1;
}
// Debug001B77C0 ->handler destructor001B7E88; E-0099-W29.
void emRemoveHandler(::port::driving::Memory& m,std::uint32_t function){
 constexpr std::uint32_t head=0x003280c0;for(auto node=m.word(head);node!=head;node=m.word(node))if(m.word(node+8)==function){
  const auto references=m.word(node+16)-1;m.word(node+16,references);if(!references){::port::race::unlink(m,node);SlotPool(m,m.word(0x003280ac)).Free(node);m.word(0x0030b6c4,m.word(0x0030b6c4)-1);}return;
 }
}
// Debug001B7848; E-0099-W29. Clears the full original36-byte payload before publication.
std::uint32_t emAddEvent(::port::driving::Memory& m,std::uint32_t id){
 const auto pool=m.word(0x003280a8);if(!pool)hp2::not_implemented("0x001B7848","EventSlotPool is not initialized");
 const auto event=bOMalloc(m,pool);if(!event)return 0;for(unsigned i=0;i<0x24;i+=4)m.word(event+i,0);
 const auto head=m.word(0x003280d0);m.word(event+12,id);m.word(event+8,0);const auto previous=m.word(head+4);
 m.word(previous,event);m.word(head+4,event);m.word(0x0030b6c0,m.word(0x0030b6c0)+1);m.word(event+4,previous);m.word(event,head);return event;
}
// Debug001B78D0; E-0099-W29. Removing the currently handled event is a confirmed no-op.
void emRemoveEvent(::port::driving::Memory& m,std::uint32_t event){if(event!=m.word(0x003280d4)){::port::race::unlink(m,event);SlotPool(m,m.word(0x003280a8)).Free(event);m.word(0x0030b6c0,m.word(0x0030b6c0)-1);}}
// Debug001B7918/001B7928; E-0099-W29. Original wrapping signed-count storage, no clamp.
void emLockEvent(::port::driving::Memory& m,std::uint32_t event){m.word(event+8,m.word(event+8)+1);}
void emUnlockEvent(::port::driving::Memory& m,std::uint32_t event){m.word(event+8,m.word(event+8)-1);}
// Debug001B7938; E-0099-W29. Deferred newly queued events, retained locked events,
// direct original handler identities and real tick-difference accounting.
void emProcessAllEvents(::port::driving::Memory& m,const ::port::race::EventDispatch& services){
 constexpr std::uint32_t master=0x003280c8,handlers=0x003280c0;
 ::port::race::EventProcessScope scope(m);m.word(0x003280d0,scope.temporary);
 for(auto event=m.word(master);event!=master;){const auto next=m.word(event);const auto id=m.word(event+12);auto after=next;m.word(0x003280d4,event);
  if(m.word(handlers)!=handlers){for(auto handler=m.word(handlers);handler!=handlers;handler=m.word(handler))if(id&m.word(handler+12)){
    const auto function=m.word(handler+8);const auto binding=services.handlers.find(function);
    if(binding==services.handlers.end()||!binding->second){char text[128];std::snprintf(text,sizeof(text),"native whole event-handler implementation for original address0x%08X is not bound",function);hp2::not_implemented("0x001B7938",text);}
    if(!services.ticker)hp2::not_implemented("0x0024A340","event dispatch requires raw Count-unit ticker service");
    const auto start=services.ticker();binding->second(m,event);const auto end=services.ticker();
    const float elapsed=(static_cast<float>(end-start)/m.scalar(0x00326254))*1000.f;m.scalar(handler+20,m.scalar(handler+20)+elapsed);
   }after=m.word(event);}
  const auto previous=m.word(event+4),references=m.word(event+8);m.word(previous,after);m.word(after+4,previous);m.word(0x003280d4,0);
  if(!references)SlotPool(m,m.word(0x003280a8)).Free(event);else ::port::race::append(m,scope.locked,event);
  event=next;
 }
 ::port::race::merge(m,scope.locked,master);::port::race::merge(m,scope.temporary,master);m.word(0x003280d0,master);scope.finished=true;
}
// Debug001B7DA0 and typed-list destructors001B7E18/001B7CB0; E-0099-W29.
void Sub_001B7DA0(::port::driving::Memory& m){
 for(auto head:{0x003280c8u,0x003280c0u})while(m.word(head)!=head){const auto node=m.word(head);::port::race::unlink(m,node);SlotPool(m,m.word(head==0x003280c8?0x003280a8:0x003280ac)).Free(node);}
 // Original001B7D28 deletes each header on flags3. Loaded port records are
 // interior pointers in source-owned chunks; unload them before static teardown.
 for(auto head:{0x003280b8u,0x003280b0u})if(m.word(head)!=head)hp2::not_implemented("0x001B7DA0","loaded raw EventTriggerPack header deletion requires resource unload before static teardown");
}
}

#include "game_core/indep/src/volumetree.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace hp2::debug {
// Debug001B7518; E-0099-W29. Loader mutates original header/tree pointers only.
int LoaderEventManager(::port::driving::Memory& m,std::uint32_t chunk){
 if(m.word(chunk)!=0x80036000)return 0;const std::uint64_t end=std::uint64_t(chunk)+8+m.word(chunk+4);m.view(chunk,static_cast<std::size_t>(end-chunk));
 std::uint32_t pack=0;for(auto child=chunk+8;child<end;){const auto tag=m.word(child),size=m.word(child+4);const auto aligned=(child+0x17)&~15u;
  if(std::uint64_t(child)+8+size>end)throw std::out_of_range("EventTriggerPack child extends beyond source container");
  if(tag==0x36001){pack=aligned;if(m.word(pack+8)!=1)return 1;}
  else if(tag==0x36002&&pack){m.word(pack+16,aligned);m.word(aligned,aligned+16);}
  else if(tag==0x36003&&pack)m.word(pack+20,aligned);
  child+=8+size;
 }
 if(pack)::port::race::append(m,(!m.word(pack+12)||!m.word(pack+16)||!m.word(pack+20))?0x003280b0:0x003280b8,pack);return 1;
}
// Debug001B7660; E-0099-W29. The original non-header first-child path does not advance.
int UnloaderEventManager(::port::driving::Memory& m,std::uint32_t chunk){
 if(m.word(chunk)!=0x80036000)return 0;if(!m.word(chunk+4))return 1;
 if(m.word(chunk+8)!=0x36001)hp2::not_implemented("0x001B76D4","original EventManager unloader non-advancing loop requires first child36001");
 const auto pack=(chunk+8+0x17)&~15u;if(m.word(pack+8)==1)::port::race::unlink(m,pack);return 1;
}
// Debug001B7B10; E-0099-W29. Raw tree/source ordering, strict 3D sphere,40-event bound.
std::uint32_t emTriggerEvents(::port::driving::Memory& m,const std::array<float,3>& p){
 constexpr std::uint32_t head=0x003280b8,results=0x0030b6d8;unsigned count=0;
 for(auto pack=m.word(head);pack!=head&&count<40;pack=m.word(pack)){
  const auto leaf=vAABBTree(m,m.word(pack+16)).QueryLeaf(p[0],p[1],p[2]);if(!leaf)continue;
  const auto num=-static_cast<int>(m.signed_half(leaf+14));
  for(int i=0;i<num&&count<40;++i){const auto trigger=m.word(pack+20)+static_cast<std::int32_t>(m.signed_half(leaf+28+static_cast<unsigned>(i)*2))*32;
   const auto x=std::abs(p[0]-m.scalar(trigger+16)),y=std::abs(p[1]-m.scalar(trigger+20)),z=std::abs(p[2]-m.scalar(trigger+24)),radius=m.scalar(trigger+28);
   if((x*x+y*y)+z*z<radius*radius){const auto event=emAddEvent(m,m.word(trigger+4));if(!event)hp2::not_implemented("0x001B7B10","original trigger event allocation failed before source-pointer publication");
    m.word(results+count*4,event);m.word(event+16,trigger);++count;
   }
  }
 }
 if(!count)return 0;m.word(results+count*4,0);return results;
}
}
namespace port::race {
namespace {
// Port validation of the exact source records; no replacement host geometry/tree.
void validate_event_pack(Memory& m,std::uint32_t chunk){
 const auto end=chunk+8+m.word(chunk+4);std::uint32_t pack=0,tree=0,array=0,tree_end=0,array_end=0;
 for(auto child=chunk+8;child<end;){const auto aligned=(child+0x17)&~15u,tag=m.word(child),size=m.word(child+4);if(std::uint64_t(child)+8+size>end||aligned>child+8+size)throw std::runtime_error("Malformed event source child");
  if(tag==0x36001){if(pack||child+8+size-aligned<24)throw std::runtime_error("Duplicate/truncated event pack header");pack=aligned;}
  if(tag==0x36002){tree=aligned;tree_end=child+8+size;}
  if(tag==0x36003){array=aligned;array_end=child+8+size;}child+=8+size;
 }
 if(!pack||m.word(pack+8)!=1)return;const auto triggers=m.signed_word(pack+12);if(triggers<0)throw std::runtime_error("Negative source event-trigger count");
 if(!triggers||!tree||!array)return;const auto nodes=m.signed_half(tree+8);
 if(nodes<=0||tree_end-tree<16+static_cast<unsigned>(nodes)*48||array_end-array<static_cast<std::uint64_t>(triggers)*32)throw std::runtime_error("Truncated source event tree/trigger records");
 std::vector<unsigned char> visiting(static_cast<unsigned>(nodes));
 const auto check=[&](const auto& self,int index)->void{
  if(index<0||index>=nodes)throw std::out_of_range("Event tree source child index outside node table");if(visiting[index]==1)throw std::runtime_error("Cyclic source event volume tree");if(visiting[index]==2)return;visiting[index]=1;
  const auto node=tree+16+static_cast<unsigned>(index)*48;const int count=m.signed_half(node+14);if(count>10||count< -10)throw std::runtime_error("Source event volume child count exceeds ten");
  for(unsigned axis=0;axis<3;++axis)if(!std::isfinite(m.scalar(node+axis*4))||!std::isfinite(m.scalar(node+16+axis*4)))throw std::runtime_error("Nonfinite source event volume");
  for(int i=0;i<std::abs(count);++i){const int child=m.signed_half(node+28+static_cast<unsigned>(i)*2);if(count>0)self(self,child);else if(child<0||child>=triggers)throw std::out_of_range("Event leaf source trigger index outside record table");}
  visiting[index]=2;
 };check(check,0);
 for(int i=0;i<triggers;++i)for(unsigned field=16;field<32;field+=4)if(!std::isfinite(m.scalar(array+static_cast<unsigned>(i)*32+field)))throw std::runtime_error("Nonfinite source event sphere");
}
}
// Port-owned explicit Retail adapter. Retail001BA100/001BA6F8 versus Debug001B7518/001B7B10.
EventResources load_retail_event_resources(Memory& m,const std::vector<std::uint8_t>& bytes){
 for(auto head:{0x003280b0u,0x003280b8u})if(m.word(head)!=head||m.word(head+4)!=head)throw std::logic_error("Event pack lists must be explicitly initialized and empty before resource load");
 std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(bytes,&consumed);if(consumed!=bytes.size())throw std::runtime_error("Incomplete event resource chunk stream");
 EventResources out;out.owner=&m;
 const auto visit=[&](const auto& self,const std::vector<hp2::assets::Chunk>& items)->void{for(const auto& c:items){if(c.tag==0x80036000){
   // Retain original header modulo16: child pointer alignment is absolute.
   const auto prefix=static_cast<std::uint32_t>(c.header_offset&15),size=c.size+8;const auto allocation=m.allocate(size+prefix);out.allocations.push_back(allocation);const auto chunk=allocation+prefix;
   std::memcpy(m.view(chunk,size).data,bytes.data()+c.header_offset,size);validate_event_pack(m,chunk);hp2::debug::LoaderEventManager(m,chunk);out.chunks.push_back(chunk);
   for(auto child=chunk+8;child<chunk+size;child+=m.word(child+4)+8)if(m.word(child)==0x36001){out.packs.push_back((child+0x17)&~15u);break;}
  }else self(self,c.children);}};
 visit(visit,chunks);out.loaded=true;return out;
}
void unload_event_resources(Memory& m,EventResources& resources){
 if(resources.owner!=&m||!resources.loaded)throw std::invalid_argument("Event resource owner/lifetime mismatch");
 for(auto event=m.word(0x003280c8);event!=0x003280c8;event=m.word(event))if(const auto trigger=m.word(event+16)){
  for(unsigned i=0;i<resources.chunks.size();++i){const auto chunk=resources.chunks[i];if(trigger>=chunk&&std::uint64_t(trigger)<std::uint64_t(chunk)+8+m.word(chunk+4))throw std::logic_error("Queued events still reference the event resources being unloaded");}
 }
 for(auto chunk:resources.chunks)hp2::debug::UnloaderEventManager(m,chunk);for(auto allocation:resources.allocations)m.release(allocation);resources={};
}
}

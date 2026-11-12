#include "race_menu_host.hpp"
#include "game_core/indep/src/joystick.hpp"
#include "game_core/port/input/joyevents.hpp"
#include <algorithm>

namespace port::race {
using namespace hp2::frontend;
namespace {void set_visibility(driving::Memory& memory,std::uint32_t hash,int state){const auto screen=hp2::debug::FindFlailerScreen(memory,hash);if(screen)hp2::debug::FlailerRenderManager(memory,0x2e82b0).SetScreenVisibility(screen,state);}}
hp2::frontend::FeScreen RaceMenuHost::read(std::uint32_t hash){
 const auto owner=hp2::debug::FindFlailerScreen(memory_,hash);auto documents=snapshot_visible_flailer(memory_,hash,false,false,owner);
 if(documents.size()!=1)throw std::runtime_error("Source race menu document is absent or ambiguous: hash="+std::to_string(hash)+" matches="+std::to_string(documents.size()));
 auto result=std::move(documents.front());result.index=int(hash);return result;
}
void RaceMenuHost::write(const FeScreen& document){
 // Only value fields changed by the recovered semantic menu adapter are
 // replayed. Intrusive links, allocations and cached source pointers stay native.
 for(const auto& w:document.widgets){
  const auto at=w.port_source_address;
  if(!at||memory_.word(at+8)!=w.name_hash||memory_.byte(at+0x28)!=w.type)throw std::runtime_error("Source race menu widget identity changed");
  memory_.byte(at+0x29,w.visible);
  memory_.byte(at+0x2a,w.layer);memory_.word(at+0x4c,w.clip_hash_0x4c);memory_.word(at+0x50,w.texture_hash);
  for(const auto& value:std::array<std::pair<unsigned,float>,4>{{{0x30,w.x},{0x34,w.y},{0x38,w.scale_x},{0x3c,w.scale_y}}})memory_.scalar(at+value.first,value.second);
  const auto colour=at+(w.type==1?0x5c:0x58);
  memory_.byte(colour,w.r);memory_.byte(colour+1,w.g);memory_.byte(colour+2,w.b);memory_.byte(colour+3,w.a);
  if(w.type==1){
   const auto capacity=memory_.word(at+0x54);
   memory_.word(at+0x58,w.text_align);memory_.word(at+0x60,w.string_id_0x60);
   if(!w.string_id_0x60&&capacity){const auto count=std::min<std::size_t>(w.text.size(),capacity-1);for(std::size_t i=0;i<count;++i)memory_.byte(at+0x64+std::uint32_t(i),std::uint8_t(w.text[i]));memory_.byte(at+0x64+std::uint32_t(count),0);}
  }else{
   memory_.word(at+0x40,std::uint32_t(w.field_0x40));memory_.word(at+0x44,w.field_0x44);
   for(unsigned i=0;i<3;++i)memory_.word(at+0x5c+4*i,w.field_0x5c[i]);
   for(unsigned i=0;i<8;++i)memory_.scalar(at+0x70+4*i,w.field_0x70[i]);
  }
  if(w.dirty_0x2b)memory_.byte(at+0x2b,w.dirty_0x2b);
 }
}
void RaceMenuHost::reload(Page& page){
 auto document=read(page.document.name_hash_0x08);
 if(document.widgets.size()!=page.document.widgets.size())throw std::logic_error("Source race menu changed widget ownership");
 // Keep element addresses: MenuList retains the highlight-bar pointer.
 for(std::size_t i=0;i<document.widgets.size();++i)page.document.widgets[i]=std::move(document.widgets[i]);
}
void RaceMenuHost::bind(decltype(callback_) callback,std::function<void(int)> sound){callback_=std::move(callback);sound_=std::move(sound);}
std::string RaceMenuHost::source_name(std::uint32_t address)const{
 std::string name;for(unsigned i=0;i<256;++i){const auto c=memory_.byte(address+i);if(!c)return name;name.push_back(char(c));}throw std::runtime_error("Source race screen name is unterminated");
}
void RaceMenuHost::register_page(std::uint32_t name,std::uint32_t hash){registration_[source_name(name)]=hash;}
void RaceMenuHost::dispatch(Page& page,int message,MenuEvent_0016A458& event){
 write(page.document);
 hp2::debug::ScreenCallbackData data;data.ItemHash=event.field_0x00;data.Port=event.pad_index_0x04;data.Param=event.result_0x08;data.Noise=event.sound_id_0x0C;
 if(!callback_)throw std::logic_error("Source race screen callback is unbound");
 callback_(page.document.name_hash_0x08,hp2::debug::ScreenMessage(message),data);
 event.result_0x08=data.Param;event.sound_id_0x0C=data.Noise;reload(page);
}
void RaceMenuHost::change_screen(std::uint32_t name,int push){
 if(push&&current_name_)history_.push_back(current_name_);
 if(current_){auto& page=*pages_.at(current_);MenuEvent_0016A458 event;event.field_0x00=current_;dispatch(page,hp2::debug::SCREEN_UNACTIVATE,event);set_visibility(memory_,current_,1);}
 current_=current_name_=0;
 if(!name)return;
 const auto text=source_name(name);const auto entry=registration_.find(text);if(entry==registration_.end())throw std::runtime_error("Original race screen registration is missing: "+text);const auto hash=entry->second;
 auto& page=pages_[hash];if(!page){page=std::make_unique<Page>();page->document=read(hash);page->group.screen_0x14=&page->document;page->group.callback_0x18=[this,p=page.get()](int msg,auto& event){dispatch(*p,msg,event);};}
 else reload(*page);
 current_=hash;current_name_=name;if(hash==0x605d2bf1)history_.clear();set_visibility(memory_,hash,0);
 const auto filter=filter_ports_.find(hash);if(filter!=filter_ports_.end())page->group.pad_index_0x10=filter->second;
 page->group.EnsureMenu_0016A858();write(page->document);
}
void RaceMenuHost::filter_port(std::uint32_t source_name,int port){
 const auto name=this->source_name(source_name);const auto entry=registration_.find(name);if(entry==registration_.end())throw std::runtime_error("Original filtered race screen is unregistered: "+name);const auto hash=entry->second;filter_ports_[hash]=port;const auto found=pages_.find(hash);if(found!=pages_.end())found->second->group.pad_index_0x10=port;
}
void RaceMenuHost::gray(std::uint32_t item,int mode,std::uint32_t screen){
 const auto found=pages_.find(screen);if(found==pages_.end())return;
 auto& page=*found->second;reload(page);Sub_0016D2A8(page.group.child_0x08,page.document,item,mode);write(page.document);
}
void RaceMenuHost::input(int port,int event,bool pressed){
 if(popup_.active){popup_.Sub_0016AF28(port,event,pressed?1:0,popup_services());if(popup_view_)write(popup_view_->screen);return;}
 if(!current_)return;
 auto& page=*pages_.at(current_);reload(page);
 if(event==hp2::input::JOY_EVENT_MENU_BACK&&pressed){MenuEvent_0016A458 data;data.field_0x00=current_;data.pad_index_0x04=port;data.result_0x08=1;data.sound_id_0x0C=6;dispatch(page,hp2::debug::SCREEN_BACK_BUTTON,data);if(sound_)sound_(data.sound_id_0x0C);if(data.result_0x08==1&&!history_.empty()){const auto previous=history_.back();history_.pop_back();if(!source_name(previous).empty())change_screen(previous,0);}}
 else page.group.HandleJoyEvent_0016AA00(port,event,pressed);
 write(page.document);
}
WidgetMetrics RaceMenuHost::measure(const FeWidget& widget){
 const auto at=widget.port_source_address?widget.port_source_address:hp2::debug::FindFlailerObject(memory_,popup_view_?popup_view_->screen.name_hash_0x08:current_,widget.name_hash,widget.type);
 if(!at)return {};
 if(widget.type==1){hp2::debug::FlailerTextChunk text(memory_,at);return {text.GetScaledWidth(),text.GetScaledHeight()};}
 const auto texture=hp2::debug::FlailerBitmapChunk(memory_,at).GetTextureInfo();if(!texture)return {};
 return {memory_.signed_half(texture+0x24)*widget.scale_x,memory_.signed_half(texture+0x26)*widget.scale_y};
}
frontend::PopupServices RaceMenuHost::popup_services(){
 auto services=popup_view_->services([this](auto msg,auto& data){if(!callback_||!current_)throw std::logic_error("Pause popup owner is absent");callback_(current_,msg,data);},sound_);
 services.show_view=[this]{set_visibility(memory_,popup_.config.screen_hash,0);};
 const auto dismiss=services.dismiss_view;services.dismiss_view=[this,dismiss]{dismiss();set_visibility(memory_,popup_.config.screen_hash,1);};
 return services;
}
void RaceMenuHost::summon(const selection::Popup& config,const std::string& message){
 if(popup_.active)throw std::logic_error("Nested race popup exceeds the active owner");
 popup_view_=std::make_unique<frontend::PopupView>(read(config.screen_hash),memory_,[this](const auto& w){return measure(w);},[this,message](int id){if(!id)return message;std::string text;const auto at=hp2::debug::GetTranslatedString(memory_,id);for(unsigned i=0;i<4096;++i){const auto c=memory_.byte(at+i);if(!c)return text;text.push_back(char(c));}throw std::runtime_error("Race popup translated string is unterminated");});
 popup_.Sub_0016AAD8(config,popup_services());write(popup_view_->screen);
}
void RaceMenuHost::dismiss_popup(int parameter){
 if(!popup_.active)return;
 popup_.Sub_0016AE08(popup_.config.port,parameter,popup_services());
 if(popup_view_)write(popup_view_->screen);
}
void RaceMenuHost::tick(float elapsed){
 if(current_){auto& page=*pages_.at(current_);reload(page);MenuEvent_0016A458 event;event.field_0x00=current_;dispatch(page,hp2::debug::SCREEN_TICK,event);event.field_0x00=current_;dispatch(page,hp2::debug::SCREEN_RENDER,event);}
 if(popup_.active){popup_view_->tick(memory_.word(0x327cd8),elapsed);write(popup_view_->screen);}
}
std::uint32_t RaceMenuHost::selected_item()const{
 if(popup_.active)return 0xe0105663u+std::uint32_t(popup_.index);
 const auto found=pages_.find(current_);if(found==pages_.end())return 0;
 const auto* list=found->second->group.child_0x08;
 return list&&list->current_0x08&&list->current_0x08!=list->sentinel()?list->current_0x08->name_hash_0x0C:0;
}
}

#pragma once
// TASK-0112. Port projection of source Flailer records into the existing
// recovered semantic menu/popup adapters. No original MenuSystem ABI asserted.
#include "race_presentation.hpp"
#include "game_core/port/race/flailer_resources.hpp"
#include "game_core/port/frontend/popup.hpp"
#include <map>
#include <memory>

namespace port::race {
class RaceMenuHost {
 driving::Memory& memory_;
 struct Page {
  hp2::frontend::FeScreen document;
  hp2::frontend::MenuGroup_0016AA00 group;
 };
 std::map<std::uint32_t,std::unique_ptr<Page>> pages_;
 std::map<std::uint32_t,int> filter_ports_;
 std::map<std::string,std::uint32_t> registration_;
 std::string source_name(std::uint32_t)const;
 std::uint32_t current_=0;
 std::uint32_t current_name_=0;
 std::vector<std::uint32_t> history_;
 std::unique_ptr<frontend::PopupView> popup_view_;
 frontend::PopupMenu popup_;
 std::function<void(std::uint32_t,hp2::debug::ScreenMessage,hp2::debug::ScreenCallbackData&)> callback_;
 std::function<void(int)> sound_;
 hp2::frontend::FeScreen read(std::uint32_t);
 void write(const hp2::frontend::FeScreen&);
 void reload(Page&);
 void dispatch(Page&,int,hp2::frontend::MenuEvent_0016A458&);
 frontend::PopupServices popup_services();
public:
 explicit RaceMenuHost(driving::Memory& memory):memory_(memory){}
 void bind(decltype(callback_) callback,std::function<void(int)> sound);
 void change_screen(std::uint32_t source_name,int root);
 void gray(std::uint32_t item,int mode,std::uint32_t screen);
 void filter_port(std::uint32_t source_name,int port);
 void register_page(std::uint32_t source_name,std::uint32_t resource_hash);
 void input(int port,int event,bool pressed);
 void tick(float elapsed);
 void summon(const selection::Popup&,const std::string& message);
 void dismiss_popup(int source_parameter);
 std::uint32_t current_screen()const{return current_;}
 bool popup_active()const{return popup_.active;}
 std::uint32_t selected_item()const;
 hp2::frontend::WidgetMetrics measure(const hp2::frontend::FeWidget&);
};
}

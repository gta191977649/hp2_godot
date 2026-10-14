#include "game_core/extensions/debug_console.hpp"
#include "game_core/extensions/viewer_session.hpp"
#include "game_core/extensions/viewer_ui.hpp"
#include <csys/system.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <stdexcept>
#include <iostream>
using namespace port::extensions;
static void check(bool value) {if(!value)throw std::runtime_error("Console assertion failed");}
int main() {
    DebugConsole console;
    check(!console.visible());
    Input toggle{Input::Kind::Toggle};toggle.pressed=true;
    check(console.input(toggle)&&console.visible());
    toggle.repeat=true;console.input(toggle);check(console.visible());
    toggle.repeat=false;console.input(toggle);check(console.visible());
    toggle.pressed=false;console.input(toggle);toggle.pressed=true;console.input(toggle);check(!console.visible());
    for(int i=0;i<3000;++i)console.log("line "+std::to_string(i));
    check(console.log_count()==2048);
    console.execute("clear");check(console.log_count()==0);
    console.execute("info on");check(console.info());
    console.execute("info invalid");check(console.info());
    console.execute("info off");check(!console.info());
    console.snapshot({"synthetic",60,16.7,12,9,3,2,1});console.execute("status");
    bool found=false;for(auto& item:console.commands().Items())if(item.m_Data.find("screen=synthetic")!=std::string::npos)found=true;
    check(found);console.execute("help");console.execute("filter synthetic");
    check(console.commands().Commands().count("help")&&console.commands().Commands().count("filter"));
    std::vector<Vertex> vertices(5);vertices[2].position={12,22};vertices[3].position={20,22};vertices[4].position={12,30};
    auto b=convert_batch(vertices,{99,99,0,1,2},2,3,2,{8,18,20,20},{10,20},{2,2},{30,30});
    check(b.indices.size()==3&&b.vertices[0].position.x==4&&b.vertices[0].position.y==4);
    check(b.clip.x==0&&b.clip.y==0&&b.clip.width==30&&b.clip.height==30);
    check(convert_batch(vertices,{0,1,2},0,3,0,{-20,-20,1,1},{0,0},{1,1},{100,100}).indices.empty());
    bool rejected=false;try{convert_batch(vertices,{7,1,2},0,3,0,{0,0,20,20},{0,0},{1,1},{100,100});}catch(const std::out_of_range&){rejected=true;}check(rejected);
    toggle.pressed=false;console.input(toggle);toggle.pressed=true;console.input(toggle);
    console.frame(800,600,1.f/60);auto frame=console.frame(800,600,1.f/60);
    check(!frame.empty()&&!console.font().rgba.empty());
    console.execute("clear");
    auto tick=[&] {console.frame(800,600,1.f/60);};
    auto press=[&](Key code) {
        Input e{Input::Kind::Key};e.code=static_cast<int>(code);e.pressed=true;
        console.input(e);tick();e.pressed=false;console.input(e);tick();
    };
    // Real input widget, not RunCommand: completion plus Enter must execute status.
    for(char c:std::string("stat")) {
        Input text{Input::Kind::Text};text.unicode=c;console.input(text);tick();
    }
    press(Key::Tab);press(Key::Enter);
    found=false;for(auto& item:console.commands().Items())if(item.m_Data.find("screen=synthetic")!=std::string::npos)found=true;
    check(found);
    const auto before=console.log_count();press(Key::Up);press(Key::Enter);
    check(console.log_count()>before);
    ViewerSession viewers(true);
    register_viewer_commands(console.commands(),viewers);
    console.execute("carviewer");check(viewers.next()->action==ViewerAction::Car);
    viewers.activate(ViewerKind::Car);
    console.execute("viewer.load \"CARS/synthetic test.bin\"");
    const auto request=viewers.next();check(request&&request->source=="CARS/synthetic test.bin");
    viewers.apply(*request);check(viewers.state().status=="Loading");
    viewers.loaded(false,"Synthetic test resource is unavailable","");
    check(viewers.state().status=="Failed"&&!viewers.state().fixture);
    console.execute("viewer.status");console.execute("viewer.clear");
    check(viewers.next()->action==ViewerAction::Clear);
    console.execute("viewer.load ../invalid");check(!viewers.next());
    console.execute("viewer.unknown");check(!viewers.next());
    console.execute("filter \"\"");
    console.log("Loader not implemented; no resource was read or loaded",true);
    check(!console.frame(1280,800,1.f/60).empty());
    console.frame(32,32,1.f/60);console.frame(32,32,1.f/60);
    // Viewer UI remains interactive with the F8 console compiled/selected off.
    DebugConsole viewer_console;viewer_console.enable_console(false);viewer_console.enable_viewer_input(true);
    ViewerUi ui;ViewerUiState state;state.kind="car";
    state.cars.push_back({39,"911TURBO","CARS/911TURBO/GEOMETRY.BIN","",true,{{1,"Silver"},{2,"Red"}}});
    state.car_id=39;state.skin_id=1;ui.update(state);
    auto draw=[&]{ViewerUi::fps(800,60,16.7);ui.draw(800,600);};
    viewer_console.frame(800,600,1.f/60,draw);viewer_console.frame(800,600,1.f/60,draw);
    Input move{Input::Kind::Mouse};move.x=40;move.y=68;
    check(viewer_console.input(move)); // First click is captured before the next NewFrame.
    Input button{Input::Kind::Button};button.code=0;button.pressed=true;
    check(viewer_console.input(button));viewer_console.frame(800,600,1.f/60,draw);
    check(!ui.next()); // Clicking queues on release, without mutating the scene during drawing.
    button.pressed=false;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    auto selected=ui.next();check(selected&&selected->action=="preview"&&selected->enabled);
    move.x=75;move.y=115;viewer_console.input(move);button.pressed=true;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    button.pressed=false;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    const auto color=ui.next();check(color&&color->action=="select_skin"&&color->car_id==39&&color->skin_id==2);
    move.x=30;move.y=162;viewer_console.input(move);button.pressed=true;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    button.pressed=false;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    const auto lods=ui.next();check(lods&&lods->action=="show_lods"&&lods->enabled);
    move.x=30;move.y=185;viewer_console.input(move);button.pressed=true;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    button.pressed=false;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    const auto blur=ui.next();check(blur&&blur->action=="wheel_blur"&&blur->enabled);
    move.x=80;move.y=208;viewer_console.input(move);button.pressed=true;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    move.x=120;viewer_console.input(move);viewer_console.frame(800,600,1.f/60,draw);
    button.pressed=false;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    bool speed_changed=false;while(auto speed=ui.next())if(speed->action=="wheel_speed")speed_changed=speed->value>0&&speed->value<=150;
    check(speed_changed);
    toggle.pressed=true;check(!viewer_console.input(toggle)&&!viewer_console.visible());
    move.x=700;move.y=500;check(!viewer_console.input(move));
    state.kind="track";ui.update(state);check(!viewer_console.frame(320,240,1.f/60,draw).empty());
    // Repaired persisted/dragged positions and resized viewports, including tiny screens.
    auto constrained=[&](float width,float height){
        viewer_console.frame(width,height,1.f/60,[&]{ViewerUi::fps(width,60,16.7);ui.draw(width,height);});
        for(auto* window:ImGui::GetCurrentContext()->Windows)if(window->Active&&!(window->Flags&ImGuiWindowFlags_ChildWindow)){
            check(window->Pos.x>=0&&window->Pos.y>=0);
            check(window->Pos.x+window->Size.x<=width+1&&window->Pos.y+window->Size.y<=height+1);
        }
    };
    constrained(800,600);
    for(const auto pos:{ImVec2{-900,-900},ImVec2{2000,-500},ImVec2{-500,2000},ImVec2{2000,2000}}){
        for(auto* window:ImGui::GetCurrentContext()->Windows)if(window->Active&&!(window->Flags&ImGuiWindowFlags_ChildWindow))ImGui::SetWindowPos(window,pos,ImGuiCond_Always);
        constrained(800,600);
    }
    constrained(160,100);constrained(32,32);constrained(800,600);
    std::cout << "DEBUG_CONSOLE_OK\n";
}

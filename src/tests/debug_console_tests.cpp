#include "game_core/extensions/debug_console.hpp"
#include "game_core/extensions/viewer_session.hpp"
#include <csys/system.h>
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
    viewers.apply(*request);check(viewers.state().status=="Not implemented");
    console.execute("viewer.status");console.execute("viewer.clear");
    check(viewers.next()->action==ViewerAction::Clear);
    console.execute("viewer.load ../invalid");check(!viewers.next());
    console.execute("viewer.unknown");check(!viewers.next());
    console.execute("filter \"\"");
    console.log("Loader not implemented; no resource was read or loaded",true);
    check(!console.frame(1280,800,1.f/60).empty());
    console.frame(32,32,1.f/60);console.frame(32,32,1.f/60);
    std::cout << "DEBUG_CONSOLE_OK\n";
}

#include "game_core/extensions/debug_console.hpp"
#include "game_core/extensions/viewer_session.hpp"
#include "game_core/extensions/viewer_ui.hpp"
#include <csys/system.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <stdexcept>
#include <iostream>
using namespace port::extensions;
static void check_impl(bool value,int line) {if(!value)throw std::runtime_error("Console assertion failed at line "+std::to_string(line));}
#define check(...) check_impl((__VA_ARGS__),__LINE__)
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
    console.execute("drivinglab");check(viewers.next()->action==ViewerAction::Driving);
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
    // Both modes belong to the same CarViewer window. Actual ImGui pointer
    // input queues their transitions on release, preserving the selected car.
    auto* car_window=ImGui::FindWindowByName("CarViewer###CarViewerCompact");check(car_window);
    move.x=55;move.y=231;viewer_console.input(move);button.pressed=true;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    check(!ui.next());
    button.pressed=false;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    const auto drive=ui.next();check(drive&&drive->action=="test_drive"&&!ui.next());
    check(state.kind=="car"&&state.car_id==39&&state.skin_id==1);
    state.kind="driving";state.driving_telemetry="Synthetic test / unknown body";ui.update(state);
    viewer_console.frame(800,600,1.f/60,draw);viewer_console.frame(800,600,1.f/60,draw);
    check(ImGui::FindWindowByName("CarViewer###CarViewerCompact")==car_window);
    // The driving surface has only the return button: former simulation
    // toolbar locations cannot queue manual start/pause/step/record actions.
    for(const auto point:{ImVec2{35,130},ImVec2{190,130},ImVec2{40,155}}){
        move.x=point.x;move.y=point.y;viewer_console.input(move);
        button.pressed=true;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
        button.pressed=false;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
        check(!ui.next());
    }
    move.x=85;move.y=91;viewer_console.input(move);button.pressed=true;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    check(!ui.next());
    button.pressed=false;viewer_console.input(button);viewer_console.frame(800,600,1.f/60,draw);
    const auto preview=ui.next();check(preview&&preview->action=="drive_exit"&&!ui.next());
    check(state.kind=="driving"&&state.car_id==39&&state.skin_id==1);
    state.kind="car";ui.update(state);viewer_console.frame(800,600,1.f/60,draw);
    check(ImGui::FindWindowByName("CarViewer###CarViewerCompact")==car_window);
    toggle.pressed=true;check(!viewer_console.input(toggle)&&!viewer_console.visible());
    move.x=700;move.y=500;check(!viewer_console.input(move));
    state.kind="driving";state.driving_telemetry="Synthetic test / unknown body";ui.update(state);check(!viewer_console.frame(800,600,1.f/60,draw).empty());
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
    // Track debug controls remain visible outside Advanced and queue independent actions.
    state.kind="track";state.track_id=11;state.debug_ready=false;ui.update(state);
    auto tick_track=[&]{viewer_console.frame(800,600,1.f/60,draw);};
    tick_track();tick_track();
    auto click_target=[&](const char* name){
        const auto found=ui.targets().find(name);check(found!=ui.targets().end());
        move.x=found->second[0];move.y=found->second[1];viewer_console.input(move);
        button.pressed=true;viewer_console.input(button);tick_track();
        button.pressed=false;viewer_console.input(button);tick_track();
    };
    check(!state.render_collision&&!state.render_material);
    click_target("show_lod");check(!ui.next());
    click_target("render_collision");check(!ui.next());
    click_target("render_material");check(!ui.next());
    state.debug_ready=true;ui.update(state);tick_track();
    click_target("show_lod");auto lod_action=ui.next();
    check(lod_action&&lod_action->action=="show_lod"&&lod_action->enabled&&!ui.next());
    state.show_lod=true;ui.update(state);tick_track();
    click_target("show_lod");lod_action=ui.next();
    check(lod_action&&lod_action->action=="show_lod"&&!lod_action->enabled&&!ui.next());
    click_target("render_collision");auto collision_action=ui.next();
    check(collision_action&&collision_action->action=="render_collision"&&collision_action->enabled&&!ui.next());
    state.render_collision=true;ui.update(state);tick_track();
    click_target("render_material");auto material_action=ui.next();
    check(material_action&&material_action->action=="render_material"&&material_action->enabled&&!ui.next());
    state.render_material=true;state.material_legend.push_back({4,"Grass",{{0,1,0,1}},12});ui.update(state);tick_track();
    click_target("render_collision");collision_action=ui.next();
    check(collision_action&&collision_action->action=="render_collision"&&!collision_action->enabled);
    state.kind="driving";ui.update(state);tick_track();
    check(!ui.targets().count("render_collision")&&!ui.targets().count("render_material"));
    // Physical framebuffer and input scale together; all widget dimensions stay
    // logical. Repeated frames do not rebuild the font atlas or compound scale.
    state.kind="track";state.camera_known=true;state.camera_host={{12,34,56}};ui.update(state);
    for(float scale:{1.f,1.5f,2.f,1.f}){
        viewer_console.set_ui_scale(scale);const auto revision=viewer_console.font().revision;
        for(int repeat=0;repeat<3;++repeat){
            const auto batches=viewer_console.frame(1280,800,1.f/60,[&]{ViewerUi::fps(1280/scale,60,16.7);ui.draw(1280/scale,800/scale);});
            check(viewer_console.font().revision==revision&&ImGui::GetIO().DisplayFramebufferScale.x==scale);
            check(std::abs(ImGui::GetIO().DisplaySize.x-1280/scale)<.01f);
            for(const auto& batch:batches)check(batch.clip.x>=0&&batch.clip.y>=0&&batch.clip.x+batch.clip.width<=1280.01f&&batch.clip.y+batch.clip.height<=800.01f);
        }
        auto* coordinates=ImGui::FindWindowByName("Track coordinates");check(coordinates&&coordinates->Active);
        const auto point=ui.targets().at("render_collision");move.x=point[0]*scale;move.y=point[1]*scale;
        viewer_console.input(move);button.pressed=true;viewer_console.input(button);
        viewer_console.frame(1280,800,1.f/60,[&]{ui.draw(1280/scale,800/scale);});
        button.pressed=false;viewer_console.input(button);
        viewer_console.frame(1280,800,1.f/60,[&]{ui.draw(1280/scale,800/scale);});
        auto action=ui.next();if(!action)std::cerr<<"DPI click scale="<<scale<<" target="<<point[0]<<","<<point[1]<<" mouse="<<ImGui::GetIO().MousePos.x<<","<<ImGui::GetIO().MousePos.y<<"\n";check(action&&action->action=="render_collision");
    }
    std::cout << "DEBUG_CONSOLE_OK\n";
}

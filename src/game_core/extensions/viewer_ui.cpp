#include "viewer_ui.hpp"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
namespace port::extensions {
void ViewerUi::update(ViewerUiState state){
    if(state.kind=="driving"&&state_.kind!="driving")driving_entered_=true;
    if(state.kind=="car"&&state_.kind=="driving")driving_left_=true;
    if(state.source!=state_.source||state.kind!=state_.kind){
        std::strncpy(source_,state.source.c_str(),sizeof(source_)-1);source_[sizeof(source_)-1]=0;
    }
    state_=std::move(state);
}
void ViewerUi::enqueue(UiAction action){
    if(state_.kind=="frontend")throw std::logic_error("Open a viewer first");
    pending_.push_back(std::move(action));
}
std::optional<UiAction> ViewerUi::next(){
    if(pending_.empty())return {};auto action=std::move(pending_.front());pending_.pop_front();return action;
}
void ViewerUi::draw(float width,float height){
    if(state_.kind=="frontend")return;
    const ImVec2 available{std::max(1.f,width),std::max(1.f,height)};
    ImGui::SetNextWindowPos({16,36},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({std::min(340.f,available.x),std::min(300.f,available.y)},ImGuiCond_FirstUseEver);
    if(state_.preview&&!was_preview_){
        ImGui::SetNextWindowPos({16,36},ImGuiCond_Always);
        ImGui::SetNextWindowSize({std::min(340.f,available.x),std::min(280.f,available.y)},ImGuiCond_Always);
    }
    if(driving_entered_){
        ImGui::SetNextWindowPos({16,36},ImGuiCond_Always);
        ImGui::SetNextWindowSize({std::min(320.f,available.x),std::min(170.f,available.y-36)},ImGuiCond_Always);
        driving_entered_=false;
    }
    if(driving_left_){
        ImGui::SetNextWindowSize({std::min(340.f,available.x),std::min(300.f,available.y)},ImGuiCond_Always);
        driving_left_=false;
    }
    was_preview_=state_.preview;
    ImGui::SetNextWindowSizeConstraints({std::min(180.f,available.x),std::min(100.f,available.y)},{std::min(420.f,available.x),(advanced_open_||state_.kind=="driving")?available.y:std::min(320.f,available.y)});
    const bool visible=ImGui::Begin((state_.kind=="driving"||state_.kind=="car")?"CarViewer###CarViewerCompact":"TrackViewer###TrackViewerCompact");
    if(visible){
        const auto tooltip=[&](const std::string& text){
            ImGui::SetNextWindowSizeConstraints({0,0},available);ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+std::max(1.f,std::min(320.f,width-24)));
            ImGui::TextUnformatted(text.c_str());ImGui::PopTextWrapPos();ImGui::EndTooltip();
        };
        const auto button=[&](const char* label,const char* action){if(ImGui::Button(label))enqueue({action});};
        const auto beside=[&](const char* label){
            const float right=ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+
                ImGui::CalcTextSize(label).x+2*ImGui::GetStyle().FramePadding.x;
            if(right<=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x)ImGui::SameLine();
        };
        if(state_.kind=="driving"){
            ImGui::TextWrapped("Test drive / synthetic plane");
            button("Return to CarViewer","drive_exit");
            ImGui::TextWrapped("W/S: accelerate/brake | A/D: steer");
            ImGui::TextWrapped("Mouse: look | Esc: release cursor | Click world: capture");
            if(!state_.status.empty())ImGui::TextWrapped("%s",state_.status.c_str());
            if(!state_.diagnostic.empty())ImGui::TextWrapped("%s",state_.diagnostic.c_str());
            ImGui::TextWrapped("Original equivalence not verified.");
        }
        if(state_.kind!="car"&&state_.kind!="driving") {
            button("CarViewer","car");beside("Back to Frontend");button("Back to Frontend","close");
            ImGui::Separator();
        }
        if(state_.kind=="car"){
            bool preview=state_.preview;
            if(ImGui::Checkbox("Preview Mode",&preview)){UiAction a{"preview"};a.enabled=preview;enqueue(std::move(a));}
            const auto car=std::find_if(state_.cars.begin(),state_.cars.end(),[&](const auto& c){return c.id==state_.car_id;});
            const auto title=car==state_.cars.end()?std::string("Select car"):car->code+"  #"+std::to_string(car->id);
            ImGui::SetNextItemWidth(-1);
            ImGui::SetNextWindowSizeConstraints({0,0},available);
            if(ImGui::BeginCombo("##Car",title.c_str())){
                ImGui::InputText("Search",filter_,sizeof(filter_));
                std::string needle=filter_;std::transform(needle.begin(),needle.end(),needle.begin(),[](unsigned char c){return char(std::tolower(c));});
                const bool child=ImGui::BeginChild("Cars",{0,std::min(180.f,std::max(1.f,height-100))},true);
                if(child)for(const auto& choice:state_.cars){
                    auto key=choice.code+" "+std::to_string(choice.id);std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return char(std::tolower(c));});
                    if(!needle.empty()&&key.find(needle)==std::string::npos)continue;
                    ImGui::PushID(choice.id);
                    const auto label=choice.code+"  #"+std::to_string(choice.id)+(choice.available?"":" [missing]");
                    if(ImGui::Selectable(label.c_str(),choice.id==state_.car_id)){
                        UiAction action{"select_car"};action.car_id=choice.id;action.skin_id=choice.skins.empty()?-1:choice.skins.front().id;enqueue(std::move(action));ImGui::CloseCurrentPopup();
                    }
                    if(!choice.available&&ImGui::IsItemHovered())tooltip(choice.error);
                    ImGui::PopID();
                }
                ImGui::EndChild();ImGui::EndCombo();
            }
            if(car!=state_.cars.end()){
                const float edge=30.f,spacing=ImGui::GetStyle().ItemSpacing.x;
                int column=0;const int columns=std::max(1,int((ImGui::GetContentRegionAvail().x+spacing)/(edge+spacing)));
                for(const auto& skin:car->skins){
                    if(column++%columns)ImGui::SameLine();
                    ImGui::PushID(skin.id);
                    const bool selected=skin.id==state_.skin_id;
                    if(selected)ImGui::PushStyleColor(ImGuiCol_Border,{1,1,1,1});
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,selected?3.f:1.f);
                    const auto& c=skin.preview;
                    if(ImGui::ColorButton("##Skin",{c[0],c[1],c[2],1},ImGuiColorEditFlags_NoTooltip|ImGuiColorEditFlags_NoDragDrop,{edge,edge})){
                        UiAction action{"select_skin"};action.car_id=car->id;action.skin_id=skin.id;enqueue(std::move(action));
                    }
                    ImGui::PopStyleVar();if(selected)ImGui::PopStyleColor();
                    if(ImGui::IsItemHovered())tooltip(skin.name+" (#"+std::to_string(skin.id)+")\n"+(skin.preview_ready?"Material preview (viewer)":skin.preview_error.empty()?"Preview unavailable / loading":skin.preview_error));
                    ImGui::PopID();
                }
                for(const auto& skin:car->skins)if(skin.id==state_.skin_id)ImGui::TextWrapped("%s (#%d)",skin.name.c_str(),skin.id);
            }
            bool show=state_.show_lods;if(ImGui::Checkbox("Show LODs",&show)){UiAction a{"show_lods"};a.enabled=show;enqueue(std::move(a));}
            beside("Reset Camera");button("Reset Camera","reset");
            bool blur=state_.wheel_blur;
            if(ImGui::Checkbox("Wheel Blur (high speed)",&blur)){UiAction a{"wheel_blur"};a.enabled=blur;enqueue(std::move(a));}
            if(ImGui::IsItemHovered())tooltip("Animate original tire matrices and WHEEL_BLUR overlay using the preview speed below.");
            float speed=state_.wheel_speed;
            ImGui::SetNextItemWidth(150);
            if(ImGui::SliderFloat("Wheel speed (rad/s)",&speed,-150.f,150.f,"%.1f")){UiAction a{"wheel_speed"};a.value=speed;enqueue(std::move(a));}
            if(ImGui::IsItemHovered())tooltip("Port-side preview angular velocity. Negative reverses rotation; zero stops it. Default 80 rad/s is a viewer choice, not an original physics constant.");
            button("Test drive","test_drive");
        }
        if(state_.kind!="driving")ImGui::TextWrapped("%s",state_.status.c_str());
        if(state_.kind=="car"&&!state_.diagnostic.empty())ImGui::TextWrapped("%s",state_.diagnostic.c_str());
        advanced_open_=state_.kind!="driving"&&ImGui::CollapsingHeader("Advanced");
        if(advanced_open_){
        ImGui::InputText("Resource",source_,sizeof(source_));
        if(ImGui::Button("Load")){UiAction action{"load"};action.source=source_;enqueue(std::move(action));}
        beside("Clear");button("Clear","clear");beside("Fixture");button("Fixture","fixture");
        beside("Reset Camera");button("Reset Camera","reset");
        ImGui::Separator();
        ImGui::TextWrapped("State: %s",state_.status.c_str());
        if(!state_.source.empty())ImGui::TextWrapped("Source: %s",state_.source.c_str());
        if(!state_.rendering.empty())ImGui::TextWrapped("%s",state_.rendering.c_str());
        if(!state_.diagnostic.empty())ImGui::TextWrapped("%s",state_.diagnostic.c_str());
        if(!state_.camera.empty())ImGui::TextWrapped("Camera: %s",state_.camera.c_str());
        if(state_.kind=="car"){
            const auto checkbox=[&](const char* label,const char* action,bool value){if(ImGui::Checkbox(label,&value)){UiAction request{action};request.enabled=value;enqueue(std::move(request));}};
            checkbox("Wireframe","wireframe",state_.wireframe);beside("Bounds");checkbox("Bounds","bounds",state_.bounds);
            checkbox("Unlit inspection (debug)","inspection",state_.inspection);
            if(ImGui::CollapsingHeader("Parts"))for(const auto& part:state_.parts){
                bool enabled=part.enabled;
                if(ImGui::Checkbox(part.name.c_str(),&enabled)){UiAction action{"part"};action.name=part.name;action.enabled=enabled;enqueue(std::move(action));}
            }
        }
        ImGui::Separator();
        ImGui::TextWrapped("%s",state_.kind=="car"?"LMB orbit | RMB pan | Wheel zoom | F frame | R reset":"Hold RMB: look + WASD/QE | Shift faster | Wheel speed | F frame | R reset");
        ImGui::TextUnformatted("F8: console (when enabled)");
        }
    }
    ImGui::End();
}
void ViewerUi::fps(float width,double fps,double ms){
    ImGui::SetNextWindowPos({std::max(0.f,width-8),4},ImGuiCond_Always,{1,0});
    ImGui::SetNextWindowSizeConstraints({0,0},{std::max(1.f,width),ImGui::GetIO().DisplaySize.y});
    ImGui::SetNextWindowBgAlpha(0);
    ImGui::Begin("Debug FPS",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored({.6f,.9f,1,1},"%.0f FPS / %.2f ms",fps,ms);ImGui::End();
}
}

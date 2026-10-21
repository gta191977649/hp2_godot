#include "viewer_ui.hpp"
#include <imgui.h>
#include <imgui_internal.h>
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
void ViewerUi::draw_render_controls(){
    if(state_.effects.empty())return;
    const auto target=[&](const std::string& name){const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();targets_[name]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f}};};
    const bool effects_open=ImGui::CollapsingHeader("Effects");target("track_effects_group");
    if(effects_open){
        std::string category;
        for(const auto& effect:state_.effects){
            if(category!=effect.category){category=effect.category;ImGui::Separator();ImGui::TextUnformatted(category.c_str());}
            const bool disabled=!state_.render_ready||!effect.available;
            if(disabled){ImGui::PushItemFlag(ImGuiItemFlags_Disabled,true);ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ImGui::GetStyle().Alpha*.5f);}
            bool enabled=effect.enabled;ImGui::PushID(effect.id.c_str());
            if(ImGui::Checkbox(effect.name.c_str(),&enabled)){UiAction a{"track_effect"};a.name=effect.id;a.enabled=enabled;enqueue(std::move(a));}target("effect_"+effect.id);
            if(disabled){ImGui::PopStyleVar();ImGui::PopItemFlag();}
            if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)&&!effect.reason.empty()){ImGui::BeginTooltip();ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+280);ImGui::TextUnformatted(effect.reason.c_str());ImGui::PopTextWrapPos();ImGui::EndTooltip();}
            ImGui::SameLine();ImGui::TextDisabled("%s",!effect.available?"unavailable":effect.active?"active":"inactive");ImGui::PopID();
        }
        if(ImGui::Button("Restore Original Defaults"))enqueue({"track_render_defaults"});target("track_render_defaults");
    }
    const bool visibility_open=ImGui::CollapsingHeader("Visibility");target("track_visibility_group");
    if(visibility_open){
        ImGui::Text("Near %.3f | Far %.1f",double(state_.effective_near),double(state_.effective_far));
        ImGui::Text("Instances %d visible / %d considered / %d loaded",state_.visible_instances,state_.considered_instances,state_.loaded_instances);
        ImGui::Text("Sections %d / %d",state_.selected_sections,state_.loaded_sections);
        if(!state_.render_ready){ImGui::PushItemFlag(ImGuiItemFlags_Disabled,true);ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ImGui::GetStyle().Alpha*.5f);}
        bool props=state_.render_props;
        if(ImGui::Checkbox("Render props",&props)){UiAction a{"track_props"};a.enabled=props;enqueue(std::move(a));}target("track_props");
        bool enabled=state_.distance_override;float distance=state_.draw_distance;
        if(ImGui::Checkbox("Override Draw Distance",&enabled)){UiAction a{"track_distance"};a.enabled=enabled;a.value=distance;enqueue(std::move(a));}target("track_distance_override");
        if(enabled){ImGui::SetNextItemWidth(-1);if(ImGui::SliderFloat("##DrawDistance",&distance,std::max(1.f,state_.effective_near*2),state_.original_far*4,"%.1f")){UiAction a{"track_distance"};a.enabled=true;a.value=distance;enqueue(std::move(a));}target("track_distance");}
        if(ImGui::Button("Restore Original Visibility")){UiAction a{"track_distance"};a.enabled=false;a.value=state_.original_far;enqueue(std::move(a));}target("track_distance_reset");
        if(!state_.render_ready){ImGui::PopStyleVar();ImGui::PopItemFlag();}
        ImGui::Text("Native %.2f ms | Submit %.2f ms",state_.native_frame_ms,state_.host_submit_ms);
    }
}

void ViewerUi::draw(float width,float height){
    targets_.clear();
    if(state_.kind=="frontend")return;
    const ImVec2 available{std::max(1.f,width),std::max(1.f,height)};
    float panel_top=36;
    if(state_.camera_known&&(state_.kind=="track"||state_.real_track)){
        ImGui::SetNextWindowPos({8,4},ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints({0,0},{std::max(1.f,width-160),available.y});
        ImGui::SetNextWindowBgAlpha(.35f);
        ImGui::Begin("Track coordinates",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings);
        const auto& p=state_.camera_host;
        ImGui::Text("Camera HP2:   X %.2f  Y %.2f  Z %.2f",double(p[2]),double(p[0]),double(p[1]));
        ImGui::Text("Camera Godot: X %.2f  Y %.2f  Z %.2f",double(p[0]),double(p[1]),double(p[2]));
        if(state_.draw_calls>=0)ImGui::Text("Draw calls (total): %lld",static_cast<long long>(state_.draw_calls));
        else ImGui::TextUnformatted("Draw calls (total): N/A");
        ImGui::SameLine();
        if(state_.rendered_primitives>=0)ImGui::Text(" | Triangles/primitives: %lld",static_cast<long long>(state_.rendered_primitives));
        else ImGui::TextUnformatted(" | Triangles/primitives: N/A");
        panel_top=ImGui::GetWindowPos().y+ImGui::GetWindowSize().y+4;ImGui::End();
    }
    ImGui::SetNextWindowPos({16,panel_top},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({std::min(340.f,available.x),std::min(state_.kind=="track"?440.f:300.f,available.y)},ImGuiCond_FirstUseEver);
    if(state_.kind=="track"&&state_.track_id<0){
        ImGui::SetNextWindowPos({16,panel_top},ImGuiCond_Always);
        ImGui::SetNextWindowSize({std::min(340.f,available.x),std::min(440.f,available.y)},ImGuiCond_Always);
    }
    if(state_.preview&&!was_preview_){
        ImGui::SetNextWindowPos({16,panel_top},ImGuiCond_Always);
        ImGui::SetNextWindowSize({std::min(340.f,available.x),std::min(280.f,available.y)},ImGuiCond_Always);
    }
    if(driving_entered_){
        ImGui::SetNextWindowPos({16,panel_top},ImGuiCond_Always);
        ImGui::SetNextWindowSize({std::min(320.f,available.x),std::min(310.f,available.y-36)},ImGuiCond_Always);
        driving_entered_=false;
    }
    if(driving_left_){
        ImGui::SetNextWindowSize({std::min(340.f,available.x),std::min(300.f,available.y)},ImGuiCond_Always);
        driving_left_=false;
    }
    was_preview_=state_.preview;
    const float panel_height=std::max(1.f,available.y-(state_.camera_known&&(state_.kind=="track"||state_.real_track)?panel_top:0));
    ImGui::SetNextWindowSizeConstraints({std::min(180.f,available.x),std::min(100.f,panel_height)},{std::min(420.f,available.x),(advanced_open_||state_.kind=="driving"||state_.kind=="track"||state_.kind=="track_runtime")?panel_height:std::min(320.f,panel_height)});
    const auto window_flags=state_.kind=="driving"?ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse:ImGuiWindowFlags_None;
    const bool track_window=state_.kind!="car"&&(state_.kind!="driving"||state_.real_track);
    if(track_window)if(auto* window=ImGui::FindWindowByName("TrackViewer###TrackViewerCompact"))
        if(window->Pos.y<panel_top)ImGui::SetNextWindowPos({window->Pos.x,panel_top},ImGuiCond_Always);
    const bool visible=ImGui::Begin((state_.kind=="car"||(state_.kind=="driving"&&!state_.real_track))?"CarViewer###CarViewerCompact":"TrackViewer###TrackViewerCompact",nullptr,window_flags);
    if(visible){
        if(state_.kind=="track"||state_.real_track){
            const bool open=ImGui::CollapsingHeader("Render statistics");
            if(open!=state_.render_statistics_enabled){UiAction action{"render_statistics"};action.enabled=open;enqueue(std::move(action));}
            if(open){
                for(const auto& line:state_.render_statistics)ImGui::TextWrapped("%s",line.c_str());
                if(ImGui::Button("Export draw diagnostic"))enqueue({"export_render_statistics"});
            }
        }
        const auto tooltip=[&](const std::string& text){
            ImGui::SetNextWindowSizeConstraints({0,0},available);ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+std::max(1.f,std::min(320.f,width-24)));
            ImGui::TextUnformatted(text.c_str());ImGui::PopTextWrapPos();ImGui::EndTooltip();
        };
        const auto target=[&](const std::string& name){const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();targets_[name]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f}};};
        const auto button=[&](const char* label,const char* action){if(ImGui::Button(label))enqueue({action});target(action);};
        const auto beside=[&](const char* label){
            const float right=ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+
                ImGui::CalcTextSize(label).x+2*ImGui::GetStyle().FramePadding.x;
            if(right<=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x)ImGui::SameLine();
        };
        if(state_.kind=="driving"){
            // Keep the exit header outside the independently scrolling diagnostics.
            // A focused grip child must never scroll Return out of reach.
            ImGui::SetScrollY(0);
            ImGui::TextWrapped("%s",state_.real_track?("Test drive / "+state_.track_name).c_str():"Test drive / synthetic plane");
            button(state_.real_track?"Return to TrackViewer":"Return to CarViewer","drive_exit");
            const bool body=ImGui::BeginChild("DrivingBody",{0,0},false);
            if(body){
            if(state_.real_track)draw_render_controls();
            if(state_.driving_powertrain_known){
                const auto gear=state_.driving_gear==0?std::string("R"):state_.driving_gear==1?std::string("N"):std::to_string(state_.driving_gear-1);
                ImGui::Text("Speed %.2f (native units) | Gear %s",double(state_.driving_speed),gear.c_str());
                ImGui::Text("RPM %.0f | Handbrake %s",double(state_.driving_rpm),state_.driving_handbrake?"ON":"off");
                const char* transmissions[]{"Automatic","Manual"};int transmission=state_.driving_transmission==1?1:0;
                ImGui::SetNextItemWidth(120);
                if(ImGui::BeginCombo("Mode",transmissions[transmission])){
                    for(int i=0;i<2;++i){if(ImGui::Selectable(transmissions[i],i==transmission)){UiAction a{"drive_transmission"};a.lod=i;enqueue(std::move(a));}target("drive_transmission_"+std::to_string(i));}
                    ImGui::EndCombo();
                }target("drive_transmission");
                ImGui::SameLine();
                if(state_.driving_transmission==1){button("N (neutral)","drive_neutral");}else ImGui::TextDisabled("N: manual only");
            }
            // TASK-0045: these choices queue a setting; only committed snapshots feed this model.
            if(state_.driving_surface_known||state_.real_track){
                if(!state_.real_track){
                const auto request_surface=[&](int mode,int terrain){UiAction a{"drive_surface"};a.lod=mode;a.car_id=terrain;enqueue(std::move(a));};
                const char* modes[]{"Uniform","Zones"};int mode=state_.driving_surface_mode;
                ImGui::SetNextItemWidth(155);
                if(ImGui::BeginCombo("Surface mode",modes[mode])){
                    for(int i=0;i<2;++i){if(ImGui::Selectable(modes[i],i==mode))request_surface(i,state_.driving_surface_terrain);target("drive_surface_mode_"+std::to_string(i));}
                    ImGui::EndCombo();
                }target("drive_surface_mode");
                const auto selected=std::find_if(state_.driving_materials.begin(),state_.driving_materials.end(),[&](const auto& m){return m.id==state_.driving_surface_terrain;});
                const auto name=selected==state_.driving_materials.end()?std::string("Unknown"):selected->name+" (#"+std::to_string(selected->id)+")";
                const float material_row_y=ImGui::GetCursorPosY();ImGui::SetNextItemWidth(155);
                if(mode==0 && ImGui::BeginCombo("Material",name.c_str())){
                    for(const auto& material:state_.driving_materials){const auto label=material.name+" (#"+std::to_string(material.id)+")";
                        if(ImGui::Selectable(label.c_str(),material.id==state_.driving_surface_terrain))request_surface(0,material.id);target("drive_material_"+std::to_string(material.id));
                        if(material.id==state_.driving_surface_terrain)ImGui::SetItemDefaultFocus();
                    }ImGui::EndCombo();
                }
                if(mode==0)target("drive_material");
                if(mode!=0){ImGui::TextDisabled("Material: per-wheel zones");ImGui::SetCursorPosY(material_row_y+ImGui::GetFrameHeightWithSpacing());}
                if(mode==1)ImGui::TextWrapped("World zones: ROAD / DIRT / GRASS / SAND / SNOW");
                }
                const bool show_grip=ImGui::CollapsingHeader("Wheel grip (stored values)");target("drive_grip");
                if(show_grip){
                    ImGui::TextWrapped("Read-only original fields; units not established. Tool colors identify surfaces.");
                    // Pinned ImGui1.77 uses legacy Columns, not the later Tables API.
                    constexpr int columns=23;ImGui::SetNextWindowContentSize({columns*90.f,0});
                    const bool child=ImGui::BeginChild("DrivingGrip",{0,120},true,ImGuiWindowFlags_HorizontalScrollbar);
                    if(child){
                        ImGui::Columns(columns,"GripColumns",true);
                        for(int j=0;j<columns;++j)ImGui::SetColumnWidth(j,90);
                        for(const auto* label:{"Wheel","Contact","Terrain","Terrain S","Terrain D","Terrain roll","Slipping","Z force","Current slip","Usage","Boost","Catchup","Road noise","Slip X","Slip Y","Force X","Force Y","Tire static","Tire dynamic","Height","Normal X","Normal Y","Normal Z"}){ImGui::TextUnformatted(label);ImGui::NextColumn();}
                        ImGui::Separator();
                        for(unsigned i=0;i<state_.driving_grip.size();++i){const auto& g=state_.driving_grip[i];
                            ImGui::Text("%u",i);ImGui::NextColumn();ImGui::TextUnformatted(g.contact_known?(g.contact?"yes":"no"):"Unknown");ImGui::NextColumn();
                            const auto material=std::find_if(state_.driving_materials.begin(),state_.driving_materials.end(),[&](const auto& m){return m.id==g.terrain;});
                            if(g.known){if(material!=state_.driving_materials.end())ImGui::Text("%s #%d",material->name.c_str(),g.terrain);else ImGui::Text("Unknown #%d",g.terrain);}else ImGui::TextUnformatted("Unknown");ImGui::NextColumn();
                            for(int j=0;j<3;++j){if(g.known&&material!=state_.driving_materials.end()){const float values[]{material->static_friction,material->dynamic_friction,material->rolling_friction};ImGui::Text("%.4g",double(values[j]));}else ImGui::TextUnformatted("Unknown");ImGui::NextColumn();}
                            ImGui::TextUnformatted(g.known?(g.slipping?"yes":"no"):"Unknown");ImGui::NextColumn();
                            const float values[]{g.z_force,g.current_slip,g.traction_usage,g.traction_boost,g.catchup_friction,g.road_noise,g.slip[0],g.slip[1],g.ground_force[0],g.ground_force[1],g.static_coefficient,g.dynamic_coefficient};
                            for(float value:values){if(g.known)ImGui::Text("%.4g",double(value));else ImGui::TextUnformatted("Unknown");ImGui::NextColumn();}
                            const float contact_values[]{g.ground_point[2],g.ground_plane[0],g.ground_plane[1],g.ground_plane[2]};
                            for(float value:contact_values){if(g.known&&g.contact)ImGui::Text("%.4g",double(value));else ImGui::TextUnformatted("Unknown");ImGui::NextColumn();}
                        }
                        ImGui::Columns(1);
                    }
                    ImGui::EndChild();
                }
            }
            for(const auto& hint:state_.driving_hints)ImGui::TextWrapped("%s",hint.c_str());
            if(!state_.status.empty())ImGui::TextWrapped("%s",state_.status.c_str());
            if(!state_.diagnostic.empty())ImGui::TextWrapped("%s",state_.diagnostic.c_str());
            ImGui::TextWrapped("Original equivalence not verified.");
            }
            ImGui::EndChild();
        }
        if(state_.kind=="track_runtime")draw_render_controls();
        if(state_.kind!="car"&&state_.kind!="driving"&&state_.kind!="track_runtime") {
            button("CarViewer","car");beside("Back to Frontend");button("Back to Frontend","close");
            ImGui::Separator();
        }
        if(state_.kind=="track"){
            const auto selected=std::find_if(state_.tracks.begin(),state_.tracks.end(),[&](const auto& t){return t.id==state_.track_id;});
            const auto title=selected==state_.tracks.end()?std::string("Select track"):selected->name+" #"+std::to_string(selected->id);
            ImGui::SetNextItemWidth(-1);
            if(ImGui::BeginCombo("##Track",title.c_str())){
                for(const auto& track:state_.tracks){
                    const auto label=track.name+" #"+std::to_string(track.id)+(track.available?"":" [missing]");
                    if(ImGui::Selectable(label.c_str(),track.id==state_.track_id)){UiAction a{"select_track"};a.car_id=track.id;a.source=track.path;enqueue(std::move(a));}
                    if(!track.error.empty()&&ImGui::IsItemHovered())tooltip(track.error);
                }ImGui::EndCombo();
            }
            const auto car=std::find_if(state_.cars.begin(),state_.cars.end(),[&](const auto& c){return c.id==state_.car_id;});
            const auto car_title=car==state_.cars.end()?std::string("Select vehicle"):car->code;
            ImGui::SetNextItemWidth(-1);
            if(ImGui::BeginCombo("##TrackCar",car_title.c_str())){
                for(const auto& c:state_.cars)if(c.available){if(ImGui::Selectable(c.code.c_str(),c.id==state_.car_id)){UiAction a{"select_car"};a.car_id=c.id;a.skin_id=c.skins.empty()?-1:c.skins.front().id;enqueue(std::move(a));}}
                ImGui::EndCombo();
            }
            button("Drive on track","track_drive");beside("Reset Camera");button("Reset Camera","reset");
            draw_render_controls();
            // Pinned ImGui1.77 exposes disabled item flags through its internal API.
            // These independent tool flags affect browsing presentation only.
            if(!state_.debug_ready){ImGui::PushItemFlag(ImGuiItemFlags_Disabled,true);ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ImGui::GetStyle().Alpha*0.5f);}
            bool collision=state_.render_collision,material=state_.render_material,lod=state_.show_lod;
            if(ImGui::Checkbox("Show LOD",&lod)){UiAction a{"show_lod"};a.enabled=lod;enqueue(std::move(a));}target("show_lod");
            if(ImGui::Checkbox("Render Collision",&collision)){UiAction a{"render_collision"};a.enabled=collision;enqueue(std::move(a));}target("render_collision");
            if(ImGui::Checkbox("Render Material",&material)){UiAction a{"render_material"};a.enabled=material;enqueue(std::move(a));}target("render_material");
            if(!state_.debug_ready){ImGui::PopStyleVar();ImGui::PopItemFlag();}
            if(!state_.debug_diagnostic.empty())ImGui::TextWrapped("%s",state_.debug_diagnostic.c_str());
            if(state_.render_material&&state_.debug_ready){
                ImGui::TextUnformatted("Terrain materials (polygons)");
                ImGui::BeginChild("TrackMaterialLegend",{0,std::min(120.f,std::max(40.f,height*0.2f))},true);
                for(const auto& entry:state_.material_legend){ImGui::PushID(entry.id);
                    ImGui::ColorButton("##TerrainColor",{entry.color[0],entry.color[1],entry.color[2],entry.color[3]},ImGuiColorEditFlags_NoTooltip,{14,14});ImGui::SameLine();
                    ImGui::Text("#%d %s (%d)",entry.id,entry.name.c_str(),entry.polygon_count);ImGui::PopID();}
                ImGui::EndChild();
            }
            if(!state_.diagnostic.empty()&&ImGui::CollapsingHeader("Recovery details"))ImGui::TextWrapped("%s",state_.diagnostic.c_str());
            if(!state_.rendering.empty())ImGui::TextWrapped("%s",state_.rendering.c_str());
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

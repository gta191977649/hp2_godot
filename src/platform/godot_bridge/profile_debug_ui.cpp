#include "profile_debug_ui.hpp"
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#if HP2_DEBUG_BUILD
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
namespace port::godot_bridge {
namespace {
std::array<float,11> compact_graph(const godot::Dictionary& packet){
    std::array<float,11> graph;graph.fill(std::numeric_limits<float>::quiet_NaN());
    const godot::Dictionary metrics=packet.get("metrics",godot::Dictionary{});
    const char* keys[]{"frame_wall_ms","process_cpu_ms","host_ms","sun_readback_ms","draw_calls","primitives"};
    const int slots[]{0,1,2,3,9,10};
    for(int i=0;i<6;++i){const auto value=metrics.get(keys[i],godot::Variant{});if(value.get_type()!=godot::Variant::NIL)graph[slots[i]]=float(value);}
    const char* roles[]{"colour","native_depth","history_0","history_1","root"};
    std::array<bool,5> found{};
    const godot::Array rows=packet.get("viewports",godot::Array{});
    for(int i=0;i<rows.size();++i){
        const godot::Dictionary row=rows[i];
        const auto role=godot::String(row.get("role","")).utf8();
        for(int j=0;j<5;++j)if(!found[j]&&std::string(role.get_data())==roles[j]){
            found[j]=true;const auto value=row.get("gpu_ms",godot::Variant{});
            if(value.get_type()!=godot::Variant::NIL)graph[4+j]=float(value);
            break;
        }
    }
    return graph;
}
// Port-side UI snapshot only. Every nested container retained by history is owned;
// packet metadata such as output paths is read live, not duplicated 240 times.
godot::Dictionary compact_sample(const godot::Dictionary& packet){
    godot::Dictionary sample;
    const godot::Dictionary metrics=packet.get("metrics",godot::Dictionary{});
    godot::Dictionary owned_metrics;
    for(const char* key:{"frame_wall_ms","process_cpu_ms","host_ms","sun_readback_ms","draw_calls","primitives","collector_ms"})
        if(metrics.has(key))owned_metrics[key]=metrics[key];
    sample["metrics"]=owned_metrics;
    godot::Array viewports;
    const godot::Array input_viewports=packet.get("viewports",godot::Array{});
    for(int i=0;i<input_viewports.size();++i){
        const godot::Dictionary input=input_viewports[i];godot::Dictionary row;
        for(const char* key:{"role","gpu_ms","source_frame","timing_status"})if(input.has(key))row[key]=input[key];
        viewports.push_back(row);
    }
    sample["viewports"]=viewports;
    godot::Array functions;
    const godot::Array input_functions=packet.get("functions",godot::Array{});
    for(int i=0;i<input_functions.size();++i){
        const godot::Dictionary input=input_functions[i];godot::Dictionary row;
        for(const char* key:{"name","ms","calls","max_ms","source"})if(input.has(key))row[key]=input[key];
        functions.push_back(row);
    }
    sample["functions"]=functions;
    return sample;
}
}
godot::Dictionary ProfileDebugUi::draw(const godot::Dictionary& packet,float width,float height){
    godot::Dictionary result;
    if(!bool(packet.get("enabled",false)))return result;
    const bool compact=bool(packet.get("compact_history",false));
    if(compact!=compact_history_active_){
        graph_history_.clear();
        if(compact)for(const auto& sample:history_)graph_history_.push_back(compact_graph(sample));
        compact_history_active_=compact;
    }
    const auto frame=std::int64_t(packet.get("engine_frame",-1));
    if(!paused_&&frame!=frame_){
        history_.push_back(compact?compact_sample(packet):packet.duplicate(true));
        if(compact)graph_history_.push_back(compact_graph(packet));
        if(history_.size()>240){history_.pop_front();if(compact)graph_history_.pop_front();}
        selected_=int(history_.size())-1;
    }
    frame_=frame;
    ImGui::SetNextWindowPos({std::max(8.f,width-440.f),42.f},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({std::min(430.f,std::max(280.f,width-16)),std::min(600.f,std::max(250.f,height-50))},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({240,180},{std::max(240.f,width-16),std::max(180.f,height-16)});
    result["visible"]=true;
    if(ImGui::Begin("Profile Debug")){
        result["expanded"]=true;
        const auto position=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
        ImGui::SetWindowPos({std::clamp(position.x,8.f,std::max(8.f,width-size.x-8)),std::clamp(position.y,8.f,std::max(8.f,height-size.y-8))});
        godot::Dictionary targets;
        const auto target=[&](const char* key){const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();targets[key]=godot::Vector2((a.x+b.x)*.5f,(a.y+b.y)*.5f);};
        if(ImGui::Button(paused_?"Resume":"Pause"))paused_=!paused_;
        target("pause");
        ImGui::SameLine();if(ImGui::Button("Clear")){history_.clear();graph_history_.clear();selected_=0;}
        target("clear");
        ImGui::SameLine();if(ImGui::Button("Export 120 frames"))result["action"]="capture";
        target("capture");result["targets"]=targets;
        const char* names[]{"CPU / frame wall time","GPU viewports (delayed)","Draw calls / primitives","Functions (inclusive wall)"};
        ImGui::SetNextItemWidth(-1);ImGui::Combo("##profile_category",&category_,names,4);
        if(ImGui::Button("Functions"))category_=3;target("functions");ImGui::SameLine();
        ImGui::Text("%s  |  %d / 240 frames",paused_?"Paused":"Live",int(history_.size()));
        if(paused_&&!history_.empty()){ImGui::SliderInt("Inspect frame",&selected_,0,int(history_.size())-1);target("inspect");}
        if(!history_.empty()){
            selected_=std::clamp(selected_,0,int(history_.size())-1);
            const godot::Dictionary sample=history_[selected_];
            const godot::Dictionary metrics=sample.get("metrics",godot::Dictionary{});
            const char* cpu[]{"frame_wall_ms","process_cpu_ms","host_ms","sun_readback_ms"};
            const char* gpu[]{"colour","native_depth","history_0","history_1","root"};
            const char* counters[]{"draw_calls","primitives"};
            const auto reading=[&](const godot::Dictionary& entry,const char* key)->float{
                if(category_==1){const godot::Array rows=entry.get("viewports",godot::Array{});for(int j=0;j<rows.size();++j){const godot::Dictionary row=rows[j];if(godot::String(row.get("role",""))==key){const auto value=row.get("gpu_ms",godot::Variant{});return value.get_type()==godot::Variant::NIL?std::numeric_limits<float>::quiet_NaN():float(value);}}return std::numeric_limits<float>::quiet_NaN();}
                const godot::Dictionary data=entry.get("metrics",godot::Dictionary{});return data.has(key)?float(data[key]):std::numeric_limits<float>::quiet_NaN();
            };
            if(category_!=3){
                const int count=category_==1?5:category_==2?2:4;
                for(int k=0;k<count;++k){
                    const char* key=category_==1?gpu[k]:category_==2?counters[k]:cpu[k];
                    const int slot=category_==1?4+k:category_==2?9+k:k;
                    std::array<float,240> points{};float maximum=category_==2?1.f:16.67f;bool available=false;
                    for(std::size_t j=0;j<history_.size();++j){const float value=compact?graph_history_[j][slot]:reading(history_[j],key);points[j]=value;if(std::isfinite(value)){maximum=std::max(maximum,value);available=true;}}
                    const float current=compact?graph_history_[selected_][slot]:reading(sample,key);const auto label=std::string(key)+(std::isfinite(current)?"":" (N/A)");
                    ImGui::TextUnformatted(label.c_str());if(std::isfinite(current)){ImGui::SameLine();ImGui::Text(category_==2?"%.0f":"%.2f ms",current);}
                    if(available){
                        const ImVec2 start=ImGui::GetCursorScreenPos(),extent{std::max(20.f,ImGui::GetContentRegionAvail().x),42};
                        ImGui::InvisibleButton(("##"+std::string(key)).c_str(),extent);
                        auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(start,{start.x+extent.x,start.y+extent.y},IM_COL32(18,30,43,255));
                        for(std::size_t j=1;j<history_.size();++j)if(std::isfinite(points[j-1])&&std::isfinite(points[j])){
                            const ImVec2 a{start.x+extent.x*float(j-1)/239.f,start.y+extent.y*(1-points[j-1]/(maximum*1.1f))};
                            const ImVec2 b{start.x+extent.x*float(j)/239.f,start.y+extent.y*(1-points[j]/(maximum*1.1f))};
                            draw->AddLine(a,b,IM_COL32(104,216,255,255),1.2f);
                        }
                    }
                    else ImGui::TextDisabled("No engine timing available");
                }
            }else{
                const godot::Array functions=sample.get("functions",godot::Array{});
                result["function_rows"]=functions.size();
                ImGui::TextDisabled("Inclusive wall time; nested entries overlap.");
                for(int j=0;j<std::min(16,int(functions.size()));++j){const godot::Dictionary row=functions[j];
                    const auto label=godot::String(row.get("name","")).utf8();
                    ImGui::Text("%7.2f ms x%d  %s",double(row.get("ms",0)),int(row.get("calls",0)),label.get_data());
                    if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::TextUnformatted(godot::String(row.get("source","")).utf8().get_data());ImGui::Text("Max call %.3f ms",double(row.get("max_ms",0)));ImGui::EndTooltip();}
                }
                if(functions.is_empty())ImGui::TextDisabled("No instrumented functions in this frame.");
            }
            ImGui::Separator();
            ImGui::Text("Draws %.0f | primitives %.0f",double(metrics.get("draw_calls",0)),double(metrics.get("primitives",0)));
            ImGui::Text("Collector %.3f ms",double(metrics.get("collector_ms",0)));
        }
        ImGui::TextWrapped("CPU entries overlap; readback may wait for GPU. GPU values are delayed, never summed. Counts include profiler UI. Unknown GPU samples are N/A and graph gaps. Functions cover instrumented scopes, not a complete native call stack.");
        if(bool(packet.get("recording",false)))ImGui::Text("Recording %d / %d",int(packet.get("recorded",0)),int(packet.get("limit",0)));
        else {const auto output=godot::String(packet.get("output","")).utf8();if(output.length())ImGui::TextWrapped("Saved: %s",output.get_data());}
    }
    ImGui::End();result["history"]=int(history_.size());result["paused"]=paused_;result["category"]=category_;
    // Explicit fixture-only observability; no extra Variant arrays in live draws.
    if(bool(packet.get("verify_history",false))){
        result["graph_cache_size"]=int(graph_history_.size());result["selected"]=selected_;
        if(compact&&!graph_history_.empty()){
            const auto values=[](const std::array<float,11>& graph){godot::Array row;for(float value:graph)row.push_back(std::isfinite(value)?godot::Variant(value):godot::Variant());return row;};
            result["selected_graph"]=values(graph_history_[std::clamp(selected_,0,int(graph_history_.size())-1)]);
            result["first_graph"]=values(graph_history_.front());
        }
    }
    return result;
}
}
#endif

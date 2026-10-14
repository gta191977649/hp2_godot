#pragma once
#include <array>
#include <deque>
#include <optional>
#include <string>
#include <vector>
namespace port::extensions {
// Port-side ImGui model. UI consumes snapshots and emits deferred actions only.
struct UiSkin {int id=-1;std::string name;std::array<float,3> preview{{.35f,.35f,.35f}};bool preview_ready=false;std::string preview_error;};
struct UiCar {int id=-1;std::string code,path,error;bool available=false;std::vector<UiSkin> skins;};
struct UiPart {std::string name;bool enabled=true;};
struct ViewerUiState {
    std::string kind="frontend",source,status,diagnostic,rendering,camera;
    int car_id=-1,skin_id=-1,lod=-1;
    float wheel_speed=80;
    bool wireframe=false,bounds=false,inspection=false,show_lods=false,preview=false,wheel_blur=false;
    std::vector<UiCar> cars;
    std::vector<UiPart> parts;
};
struct UiAction {std::string action,source,name;int car_id=-1,skin_id=-1,lod=-1;bool enabled=false;double value=0;};
class ViewerUi {
    ViewerUiState state_;
    bool advanced_open_=false;
    bool was_preview_=false;
    std::deque<UiAction> pending_;
    char source_[512]{},filter_[128]{};
public:
    void update(ViewerUiState state);
    void draw(float width,float height);
    void enqueue(UiAction action);
    std::optional<UiAction> next();
    const ViewerUiState& state()const{return state_;}
    static void fps(float width,double fps,double ms);
};
}

#pragma once
// TASK-0098 E-0098-L02. Port-owned typed projection of Debug CarLockedDisplayer
// and its grouped AnimationGroup. No original file ownership or PS2 ABI claim.
#include "game_core/indep/inc/point.hpp"
#include "game_core/indep/inc/EventTree.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <array>
#include <functional>
#include <optional>
#include <string_view>

namespace hp2::debug { struct VisibleCar; }
namespace port::driving { class Memory; }
namespace port::selection {
// Port projections of only the read fields in NFSTaskParameters/NFSPointsRewards.
struct TaskRewardRow {int UnlockedInGameType=0,Reward=0,CarSelectUnlockBlurb=0;};
struct PointsRewardRow {int RequiredPoints=0,Reward=0;};
struct UnlockTables {std::array<TaskRewardRow,10> tasks;std::array<PointsRewardRow,19> points;};
UnlockTables decode_unlock_tables(const port::driving::Memory&);
struct UnlockBlurbServices {
    const UnlockTables& tables;
    // Ordered like original lEventTrees, never sorted by tree type/name.
    const std::vector<hp2::debug::EventTree>& trees;
    // Existing localization/printf dialect adapter; optional arg is signed original
    // event nNumber or RequiredPoints, not a host event ordinal or formatted label.
    std::function<std::string(int,std::optional<int>)> translated_format;
};
void FormatUnlockBlurbForCar(std::array<char,256>&,int car_type,int game_type,
    int player_number,const UnlockBlurbServices&);
struct LockedDisplayServices {
    // Original FormatUnlockBlurbForCar00244A68 writes at most256 bytes.
    std::function<void(std::array<char,256>&,int,int,int)> format_unlock_blurb;
    // Original RenderTextIntoRectangle; caller owns frame allocation/render ordering.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::string_view,float)> render_text;
};
class LockedDisplay {
public:
    explicit LockedDisplay(hp2::frontend::FeScreen&);
    ~LockedDisplay();
    LockedDisplay(const LockedDisplay&)=delete;
    LockedDisplay& operator=(const LockedDisplay&)=delete;
    void SetCurrentCar(hp2::debug::VisibleCar*,int unlock_all,int player_number,
        int game_type,std::uint32_t real_timer,const LockedDisplayServices&);
    void Update(std::uint32_t real_timer,float real_time_elapsed);
    void Render(int unlock_all,const LockedDisplayServices&);
    hp2::debug::VisibleCar* CurrentCar=nullptr;
    float EndPosition=0;
    int MaxAlpha=0;
    std::uint32_t ChangedTime=0;
    std::array<char,256> UnlockBlurb{};
    hp2::debug::tCubic1D Spline;
private:
    hp2::frontend::FeScreen& screen_;
    int saved_x_=0,saved_y_=0;
    void setup_slide(float,float);
    void publish_slide();
    void publish_position(int,int);
};
}

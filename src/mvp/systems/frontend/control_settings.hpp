#pragma once

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>

#include <array>

namespace hp2 {

class ControlSettings final {
public:
    enum class Context { Race, Menu, Shared };
    enum class Group { Frontend, Race, Cop };
    struct Binding { const char* action; const char* label; Context context; Group group; };
    static constexpr std::array bindings{
        Binding{"hp2_accelerate", "Accelerate", Context::Race, Group::Race}, Binding{"hp2_brake", "Brake / Reverse", Context::Race, Group::Race},
        Binding{"hp2_steer_left", "Steer Left", Context::Race, Group::Race}, Binding{"hp2_steer_right", "Steer Right", Context::Race, Group::Race},
        Binding{"hp2_handbrake", "Handbrake", Context::Shared, Group::Race}, Binding{"hp2_r2", "Shift Up", Context::Race, Group::Race},
        Binding{"hp2_l2", "Shift Down", Context::Race, Group::Race}, Binding{"hp2_l1", "Look Behind", Context::Race, Group::Race},
        Binding{"hp2_camera", "Change Camera", Context::Shared, Group::Race}, Binding{"hp2_horn", "Horn / Siren", Context::Race, Group::Race},
        Binding{"hp2_select", "Reset Car", Context::Shared, Group::Race}, Binding{"hp2_nitro", "Nitro", Context::Race, Group::Cop},
        Binding{"hp2_roadblock", "Call Roadblock", Context::Race, Group::Cop},
        Binding{"hp2_helicopter", "Call Helicopter", Context::Race, Group::Cop},
        Binding{"hp2_zone_zoom", "Look Ahead (Zone Zoom)", Context::Race, Group::Race},
        Binding{"hp2_zone_freeze", "360° Camera (Zone Freeze)", Context::Race, Group::Race},
        Binding{"hp2_cycle_hud", "Cycle HUD", Context::Race, Group::Race}, Binding{"hp2_pause", "Pause / Resume", Context::Shared, Group::Race},
        Binding{"hp2_menu_confirm", "Menu Confirm", Context::Menu, Group::Frontend},
        Binding{"hp2_menu_back", "Menu Back", Context::Menu, Group::Frontend},
        Binding{"hp2_menu_up", "Menu Up", Context::Menu, Group::Frontend},
        Binding{"hp2_menu_down", "Menu Down", Context::Menu, Group::Frontend},
        Binding{"hp2_menu_left", "Menu Left", Context::Menu, Group::Frontend},
        Binding{"hp2_menu_right", "Menu Right", Context::Menu, Group::Frontend},
    };

    static constexpr int defaultDeadZone = 8;
    static constexpr int defaultSensitivity = 100;

    explicit ControlSettings(godot::Callable storage);
    ~ControlSettings();
    godot::String deviceName{"Keyboard"};
    godot::String key_name(std::size_t row) const;
    static godot::String key_name(godot::Key key);
    bool can_bind(std::size_t row, godot::Key key) const;
    int conflict(std::size_t row, godot::Key key) const;
    bool bind(std::size_t row, godot::Key key);
    bool restore_keyboard();
    bool tune_stick(int deadZone, int sensitivity);
    int dead_zone() const { return deadZone_; }
    int sensitivity() const { return sensitivity_; }
    float steering(float value) const;

private:
    using Keys = std::array<godot::Key, bindings.size()>;
    bool available_key(godot::Key key) const;
    static bool overlaps(std::size_t first, std::size_t second);
    static int conflict(const Keys& keys, std::size_t row, godot::Key key);
    Keys swapped_keys(std::size_t row, godot::Key key) const;
    bool save(const Keys& keys, int deadZone, int sensitivity) const;
    void apply(const Keys& keys);

    godot::Callable storage_;
    Keys defaults_{}, keys_{};
    int deadZone_{defaultDeadZone};
    int sensitivity_{defaultSensitivity};
};

}

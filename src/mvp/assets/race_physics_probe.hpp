#pragma once

#include <fstream>
#include <string>

struct WorldSimulation;
namespace godot { class Node3D; }

namespace hp2 {

struct ControllerState;
struct MainMenuScreenServices;

// Opt-in fixture and telemetry. All driving inputs still go through the native scanner.
class RacePhysicsProbe {
public:
    void configure(bool isolatedProfile, int track);
    bool enabled() const noexcept { return ticks_.is_open(); }
    bool complete() const noexcept { return complete_; }
    unsigned ticks_written() const noexcept { return ticksWritten_; }
    static void start_race(MainMenuScreenServices& menus, const void* context);
    void before_step(const WorldSimulation& world);
    void after_step(const WorldSimulation& world);
    bool read_input(ControllerState& pad) const;
    void render_sample(unsigned step, const godot::Node3D& car, const godot::Node3D& camera);

private:
    enum class Maneuver { Left, Right, KeyLeft, Graze };
    int track_{};
    std::string model_;
    float targetSpeed_{};
    float triggerDistance_{};
    float timeScale_{};
    Maneuver maneuver_{Maneuver::Left};
    int triggerStep_{-1};
    unsigned ticksWritten_{};
    int steer_{128};
    bool recording_{};
    bool complete_{};
    std::ofstream ticks_;
    std::ofstream frames_;
};

}

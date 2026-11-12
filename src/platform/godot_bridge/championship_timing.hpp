#pragma once

#include <godot_cpp/classes/os.hpp>
#include <chrono>

namespace port::godot_bridge {

inline bool championship_timing_enabled() {
    static const bool enabled = [] {
        const auto arguments = godot::OS::get_singleton()->get_cmdline_user_args();
        bool benchmark = false;
        for (int index = 0; index < arguments.size(); ++index) {
            if (arguments[index] == "--championship-wall-only") return false;
            if (arguments[index] == "--championship-performance") benchmark = true;
        }
        return benchmark;
    }();
    return enabled;
}

class ChampionshipTimingScope {
public:
    explicit ChampionshipTimingScope(double& milliseconds)
        : milliseconds_(milliseconds), enabled_(championship_timing_enabled()) {
        if (enabled_) start_ = Clock::now();
    }

    ~ChampionshipTimingScope() {
        if (enabled_)
            milliseconds_ += std::chrono::duration<double, std::milli>(Clock::now() - start_).count();
    }

    ChampionshipTimingScope(const ChampionshipTimingScope&) = delete;
    ChampionshipTimingScope& operator=(const ChampionshipTimingScope&) = delete;

private:
    using Clock = std::chrono::steady_clock;
    double& milliseconds_;
    bool enabled_;
    Clock::time_point start_;
};

} // namespace port::godot_bridge

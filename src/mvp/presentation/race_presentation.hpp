#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <memory>

namespace hp2 {

class RacePresentation : public godot::Node3D {
    GDCLASS(RacePresentation, godot::Node3D)

public:
    RacePresentation();
    ~RacePresentation() override;
    bool start_frontend(const godot::String& assets, const godot::String& profile, const godot::String& ffmpeg);
    bool start_race(const godot::String& assets, const godot::String& profile);
    void _process(double delta) override;
    godot::Dictionary telemetry() const;

protected:
    static void _bind_methods();

private:
    struct Session;
    std::unique_ptr<Session> session_;
    bool start(const godot::String& assets, const godot::String& profile, const godot::String& ffmpeg, bool frontend);
    void fail(const char* stage, const char* reason);
};

}

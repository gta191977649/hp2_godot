#pragma once
// TASK-0038. Port-side laboratory contracts; no PS2 ABI or physics is implied.
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <memory>
#include <string>
#include <vector>
namespace port::driving {
struct Input { float throttle=0, brake=0, steering=0; };
using Matrix = std::array<float,16>;
struct WheelSnapshot {
    Matrix tire{}, brake{}, blur{};
    float phase=0, angular_velocity=0, blur_alpha=0;
    std::optional<bool> contact;
};
struct Snapshot {
    std::uint64_t step=0;
    std::optional<float> dt;
    std::optional<Matrix> body;
    std::optional<std::array<WheelSnapshot,4>> wheels;
    std::optional<std::array<float,3>> velocity, angular_velocity;
};
// Port-side publication validation, not recovered game rules.
void validate_matrix(const Matrix&);
void validate_wheels(const std::array<WheelSnapshot,4>&);
struct Config {
    std::string code;
    std::uint32_t index=0;
    std::array<std::uint8_t,0x560> raw{}; // E-0038-002, untyped original row.
};
Config read_config(const std::vector<std::uint8_t>& bundle,const std::string& code);
// Transactional adapter seam. A backend returns a complete next snapshot or throws.
// Callback fixtures exercise tool contracts only. Production uses the owned Simulation
// interface below, with original-data configuration and recovered algorithm dependencies.
using Step = std::function<Snapshot(const Config&,const Input&,const Snapshot&)>;
// Port-side owned production simulation. Time is elapsed clock input to recovered scheduling.
struct Simulation {
    virtual ~Simulation()=default;
    virtual void reset()=0;
    virtual Snapshot advance(const Config&,const Input&,float elapsed)=0;
    virtual float minimum_frame_elapsed()const=0;
};
class Session {
public:
    explicit Session(Step backend={});
    explicit Session(std::shared_ptr<Simulation>);
    void configure(Config config);
    void fail(std::string reason);
    void input(Input value);
    void pause();
    void run();
    bool single_step();
    bool tick(float elapsed=0);
    void reset();
    void recording(bool enabled);
    void replay();
    const Snapshot& snapshot()const{return snapshot_;}
    const Input& controls()const{return input_;}
    const std::string& diagnostic()const{return diagnostic_;}
    const std::string& status()const{return status_;}
    const std::vector<Input>& tape()const{return tape_;}
    bool paused()const{return paused_;}
    bool configured()const{return config_.has_value();}
    bool is_recording()const{return recording_;}
    bool is_replaying()const{return replay_;}
    std::size_t replay_position()const{return cursor_;}
private:
    bool advance(float elapsed=0);
    Step backend_;
    std::shared_ptr<Simulation> simulation_;
    std::optional<Config> config_;
    Snapshot snapshot_;
    Input input_;
    std::vector<Input> tape_;
    std::vector<float> elapsed_tape_;
    std::size_t cursor_=0;
    double pending_elapsed_=0;
    bool paused_=true,recording_=false,replay_=false;
    std::string status_="No vehicle",diagnostic_;
};
}

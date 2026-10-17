#pragma once
// TASK-0038. Port-side laboratory contracts; no PS2 ABI or physics is implied.
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <memory>
#include <string>
#include <vector>
#include "surface.hpp"
#include "skid_services.hpp"
namespace port::driving {
struct TrackEnvironment;
// Port semantic inputs. Discrete requests are consumed after one successful step.
struct Input {
    float throttle=0, brake=0, steering=0, handbrake=0;
    int transmission=-1; // unchanged / automatic0 / manual1
    int shift=0; // down-1 / none0 / up1
    bool neutral=false;
};
void validate_input(const Input&);
struct PowertrainSnapshot {
    float rpm=0,speed=0,forward_speed=0;
    int gear=1,manual_transmission=0;
    bool handbrake=false;
};
using Matrix = std::array<float,16>;
struct WheelSnapshot {
    Matrix tire{}, brake{}, blur{};
    float phase=0, angular_velocity=0, blur_alpha=0;
    std::optional<bool> contact;
    std::optional<GripSnapshot> grip;
};
struct Snapshot {
    std::uint64_t step=0;
    std::optional<float> dt;
    std::optional<Matrix> body;
    std::optional<std::array<WheelSnapshot,4>> wheels;
    std::optional<std::array<float,3>> velocity, angular_velocity;
    std::optional<SurfaceSettings> surface;
    std::optional<std::array<SurfaceMaterial,5>> surface_catalog;
    std::optional<PowertrainSnapshot> powertrain;
    std::string track_label;
    std::vector<SurfaceMaterial> track_material_catalog;
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
// E-0048-D05: original CarTypeInfo::BaseModelName char[32] at0x20, independent of code.
std::string base_model_name(const Config&);
// Transactional adapter seam. A backend returns a complete next snapshot or throws.
// Callback fixtures exercise tool contracts only. Production uses the owned Simulation
// interface below, with original-data configuration and recovered algorithm dependencies.
using Step = std::function<Snapshot(const Config&,const Input&,const Snapshot&)>;
// Port-side owned production simulation. Time is elapsed clock input to recovered scheduling.
struct Simulation {
    virtual ~Simulation()=default;
    virtual void reset()=0;
    virtual Snapshot advance(const Config&,const Input&,float elapsed,const SurfaceSettings& surface={})=0;
    virtual float minimum_frame_elapsed()const=0;
    virtual void track(std::shared_ptr<const TrackEnvironment>);
    virtual void skid_textures(SkidTextureLookup){}
    virtual std::vector<SkidQuad> render_skids(const SkidView&){return {};}
};
class Session {
public:
    explicit Session(Step backend={});
    explicit Session(std::shared_ptr<Simulation>);
    void configure(Config config);
    void fail(std::string reason);
    void input(Input value);
    void surface(SurfaceSettings value);
    void track(std::shared_ptr<const TrackEnvironment>);
    const std::string& track_label()const{return track_label_;}
    void pause();
    void run();
    bool single_step();
    bool tick(float elapsed=0);
    void reset();
    void recording(bool enabled);
    void replay();
    void skid_textures(SkidTextureLookup lookup){if(simulation_)simulation_->skid_textures(std::move(lookup));}
    std::vector<SkidQuad> render_skids(const SkidView& view);
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
    std::vector<SurfaceSettings> surface_tape_;
    SurfaceSettings surface_;
    std::size_t cursor_=0;
    double pending_elapsed_=0;
    bool paused_=true,recording_=false,replay_=false;
    std::string status_="No vehicle",diagnostic_;
    std::string track_label_;
};
}

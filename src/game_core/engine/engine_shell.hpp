#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hp2::engine {
inline constexpr const char* binary_sha256 = "7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2";
enum class Mode { Observation, Strict };
enum class Status { Ready, Completed, WaitingForFrameBoundary, Unimplemented, Error, Stopped };
enum class Module { Lifecycle, State, Time, Input, World, Presentation, UnresolvedCalls };
enum class Phase { Startup, FrameStart, StateDispatch, PreDispatch, TimeStep, WorldBranch, PostModules, Commit, Boundary, Tail };
enum class EventKind { Executed, SyntheticPlaceholder, Skipped, Branch, Waiting, Boundary, Unimplemented, Error, HostShutdown };
struct CallSite {
    std::uint32_t caller, pc, target;
    Module module;
    Phase phase;
    const char* guard;
    const char* evidence;
    const char* confidence;
};
const std::vector<CallSite>& call_sites();
const char* name(Status value);
const char* name(Module value);
const char* name(Phase value);
const char* name(EventKind value);

// Port-side copies of observed words, not native pointers or recovered class layouts.
struct RawState {
    std::uint32_t gp_a54c = 0; // startup branch
    std::uint32_t gp_a938 = 1; // object presence; class unresolved
    std::uint32_t gp_a998 = 1; // world candidate token
    std::uint32_t gp_a858 = 1; // record/replay candidate token
    std::uint32_t gp_ac50 = 0; // skip 0x0012F190 when nonzero
    std::uint32_t a858_field18 = 0;
    std::uint32_t a938_field2c = 1;
    std::uint32_t gp_b360 = 0, gp_a544 = 0; // boundary counter and frame snapshot
};
struct SyntheticFixtures {
    bool predicate_1680b8 = false;
    bool predicate_14d6e0 = false;
    float elapsed_result = 10.0f;
    float elapsed_scale = 0.001f;
    float base_step = 0.02f;
    float effective_step = 0.01f;
};
struct FrameTiming {
    std::optional<std::uint64_t> previous_stamp, current_stamp;
    std::optional<float> elapsed, elapsed_scale, scaled_elapsed, base_step, effective_step;
    std::array<std::optional<std::uint64_t>, 10> stamps; // DAT_002F0058..007C
};
struct TraceEvent {
    std::uint64_t frame;
    std::uint32_t pc, target;
    Module module;
    Phase phase;
    EventKind kind;
    std::string note;
    const char* evidence;
};
using FrameTrace = std::vector<TraceEvent>;
struct FrameContext {
    RawState raw;
    FrameTiming timing;
    std::uint64_t completed_frames = 0;
    double host_delta_seconds = 0, host_elapsed_seconds = 0;
    std::uint32_t current_pc = 0;
    Phase phase = Phase::Startup;
    FrameTrace trace;
    std::vector<std::uint32_t> unresolved_sites;
    std::string error;
};
struct CallArguments {
    std::array<std::optional<std::uint64_t>, 4> a;
    std::optional<float> f12;
};
struct CallResult {
    bool success = true;
    std::optional<std::uint64_t> integer;
    std::optional<float> scalar;
    std::string error;
};
using Handler = std::function<CallResult(FrameContext&, const CallSite&, const CallArguments&)>;
class ModuleRegistry {
public:
    void bind_site(std::uint32_t pc, Handler handler);
    void bind_target(std::uint32_t target, Handler handler);
    const Handler* find(const CallSite& site) const;
private:
    std::map<std::uint32_t, Handler> sites_, targets_;
};
struct HostFrameEvent { double delta_seconds = 0; bool boundary = false; };
class EngineShell {
public:
    ModuleRegistry& modules() { return modules_; }
    Status initialize(Mode mode = Mode::Observation, RawState raw = {}, SyntheticFixtures fixtures = {},
                      std::uint64_t boot_a0 = 0, std::uint64_t boot_a1 = 0);
    Status step_frame(HostFrameEvent event = {});
    void shutdown(); // Host lifecycle operation; no original shutdown symbol inferred.
    Status status() const { return status_; }
    const FrameContext& context() const { return context_; }
    const std::vector<TraceEvent>& startup_trace() const { return startup_trace_; }
private:
    bool invoke(std::uint32_t pc, CallResult& result);
    bool apply_result(const CallSite& site, const CallResult& result, const CallArguments& passed);
    CallArguments arguments(const CallSite& site) const;
    std::uint32_t next(std::uint32_t pc, const CallResult& result);
    void branch(std::uint32_t pc, bool taken, const char* condition);
    void skip(std::uint32_t first, std::uint32_t last, const char* reason);
    void fail(const CallSite& site, const std::string& reason);
    ModuleRegistry modules_;
    FrameContext context_;
    std::vector<TraceEvent> startup_trace_;
    Mode mode_ = Mode::Observation;
    Status status_ = Status::Stopped;
    SyntheticFixtures fixtures_;
    std::uint64_t boot_a0_ = 0, boot_a1_ = 0, synthetic_tick_ = 0;
    std::optional<std::uint64_t> saved_frame_stamp_; // original callee-saved s2
    std::optional<float> saved_world_step_; // original callee-saved f20
    std::uint32_t pc_ = 0;
};
// Synthetic debug presets only. No original mode enum inferred.
RawState scenario_state(unsigned scenario);
SyntheticFixtures scenario_fixtures(unsigned scenario);
} // namespace hp2::engine

#pragma once
// Port-side debug scaffolds. No original formats, ABI or coordinate system are asserted.
#include <deque>
#include <optional>
#include <string>
namespace csys { class System; }
namespace port::extensions {
enum class ViewerKind { Frontend, Car, Track };
enum class ViewerAction { Car, Track, Close, Load, Clear, Fixture, Reset };
struct ViewerRequest { ViewerAction action; std::string source; };
struct ViewerState {
    ViewerKind kind = ViewerKind::Frontend;
    std::string source, status = "Empty", diagnostic, camera, rendering;
    bool fixture = false;
};
// Host-thread only. Commands enqueue work; the host consumes it outside ImGui callbacks.
class ViewerSession {
public:
    explicit ViewerSession(bool enabled) : enabled_(enabled) {}
    void request(ViewerAction action, std::string source = {});
    std::optional<ViewerRequest> next();
    void activate(ViewerKind kind);
    void apply(const ViewerRequest& request);
    // Port-side completion from the car loader; never reports a fixture as loaded content.
    void loaded(bool success, std::string diagnostic, std::string rendering);
    void rendering(std::string description) { state_.rendering = std::move(description); }
    void camera(std::string description) { state_.camera = std::move(description); }
    const ViewerState& state() const { return state_; }
    bool enabled() const { return enabled_; }
    std::string describe() const;
private:
    bool enabled_;
    ViewerState state_;
    std::deque<ViewerRequest> pending_;
};
const char* viewer_name(ViewerKind kind);
const char* action_name(ViewerAction action);
ViewerAction parse_viewer_action(const std::string& action);
ViewerKind parse_viewer_kind(const std::string& kind);
// Linked with the optional console, not required for console-off viewers.
void register_viewer_commands(csys::System& commands, ViewerSession& session);
}

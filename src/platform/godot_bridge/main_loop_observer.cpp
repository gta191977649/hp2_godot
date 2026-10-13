#include "main_loop_observer.hpp"
#include "game_core/port/engine/world_connections.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <cstdio>
using namespace hp2::engine;
namespace port::godot_bridge {
namespace {
godot::String hex(std::uint32_t address) {char text[12];std::snprintf(text,sizeof(text),"0x%08X",address);return godot::String(text);}
godot::Variant scalar(const std::optional<float>& value) {return value ? godot::Variant(double(*value)) : godot::Variant();}
}
void MainLoopObserver::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("initialize","scenario","strict","alternate_startup"),&MainLoopObserver::initialize,DEFVAL(0),DEFVAL(false),DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("step_frame","host_delta_seconds","boundary"),&MainLoopObserver::step_frame,DEFVAL(0.0),DEFVAL(true));
    godot::ClassDB::bind_method(godot::D_METHOD("shutdown"),&MainLoopObserver::shutdown);
    godot::ClassDB::bind_method(godot::D_METHOD("snapshot"),&MainLoopObserver::snapshot);
    godot::ClassDB::bind_method(godot::D_METHOD("get_trace","startup"),&MainLoopObserver::get_trace,DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("get_world_connections"),&MainLoopObserver::get_world_connections);
}
godot::Dictionary MainLoopObserver::initialize(std::int64_t scenario,bool strict,bool alternate) {
    if (scenario<0 || scenario>5) {
        godot::Dictionary error;error["status"]="Error";error["error"]="Synthetic scenario must be 0..5";error["accepted"]=false;return error;
    }
    strict_=strict;scenario_=scenario;
    auto raw=scenario_state(static_cast<unsigned>(scenario));if(alternate) raw.gp_a54c=1;
    shell_.initialize(strict ? Mode::Strict : Mode::Observation,raw,scenario_fixtures(static_cast<unsigned>(scenario)));
    return snapshot();
}
godot::Dictionary MainLoopObserver::step_frame(double delta,bool boundary) {shell_.step_frame({delta,boundary});return snapshot();}
godot::Dictionary MainLoopObserver::shutdown() {shell_.shutdown();return snapshot();}
godot::Dictionary MainLoopObserver::snapshot() const {
    const auto& c=shell_.context();godot::Dictionary row;
    row["status"]=name(shell_.status());row["completed_frames"]=static_cast<std::int64_t>(c.completed_frames);
    row["phase"]=name(c.phase);row["pc"]=hex(c.current_pc);row["error"]=godot::String::utf8(c.error.c_str());
    row["scenario"]=scenario_;row["synthetic"]=!strict_;row["strict"]=strict_;
    row["host_delta_seconds"]=c.host_delta_seconds;row["host_elapsed_seconds"]=c.host_elapsed_seconds;
    row["base_step"]=scalar(c.timing.base_step);row["effective_step"]=scalar(c.timing.effective_step);
    row["elapsed_result"]=scalar(c.timing.elapsed);row["scaled_elapsed"]=scalar(c.timing.scaled_elapsed);
    row["unresolved_count"]=static_cast<std::int64_t>(c.unresolved_sites.size());
    row["boundary_counter"]=static_cast<std::int64_t>(c.raw.gp_b360);row["boundary_snapshot"]=static_cast<std::int64_t>(c.raw.gp_a544);
    row["call_site_count"]=61;row["target_count"]=39;row["world_connection_count"]=static_cast<std::int64_t>(world_connections().size());
    row["elf_sha256"]=binary_sha256;row["executable_revision"]="unknown; boot VER 1.00";
    row["verification"]="Structure tested; original behavior unverified";
    return row;
}
godot::Array MainLoopObserver::get_trace(bool startup) const {
    godot::Array array;
    const auto& trace=startup ? shell_.startup_trace() : shell_.context().trace;
    for(const auto& e:trace) {
        godot::Dictionary row;row["frame"]=static_cast<std::int64_t>(e.frame);row["pc"]=hex(e.pc);row["target"]=hex(e.target);
        row["module"]=name(e.module);row["phase"]=name(e.phase);row["event"]=name(e.kind);
        row["note"]=godot::String::utf8(e.note.c_str());row["evidence"]=e.evidence;array.append(row);
    }
    return array;
}
godot::Array MainLoopObserver::get_world_connections() const {
    godot::Array array;
    for(const auto& e:world_connections()) {
        godot::Dictionary row;row["caller"]=hex(e.caller);row["pc"]=hex(e.pc);row["target"]=e.direct_target ? hex(e.direct_target) : godot::String("virtual / runtime target");
        row["slot"]=e.slot;row["contract"]=e.contract;row["confidence"]=e.confidence;row["evidence"]=e.evidence;array.append(row);
    }
    return array;
}
}

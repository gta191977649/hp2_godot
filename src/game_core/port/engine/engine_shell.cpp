// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#include "game_core/port/engine/engine_shell.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace hp2::engine {
const std::vector<CallSite>& call_sites() {
    static const std::vector<CallSite> sites = {
#include "call_sites.inc"
    };
    return sites;
}
namespace {
const CallSite& site_at(std::uint32_t pc) {
    const auto& sites = call_sites();
    const auto it = std::find_if(sites.begin(), sites.end(), [pc](const CallSite& s) { return s.pc == pc; });
    if (it == sites.end()) throw std::logic_error("Unknown schedule call PC");
    return *it;
}
TraceEvent event_for(const CallSite& site, const FrameContext& ctx, EventKind kind, std::string note) {
    return {site.phase==Phase::Startup ? 0 : ctx.completed_frames + 1, site.pc, site.target, site.module, site.phase, kind, std::move(note), site.evidence};
}
}
const char* name(Status v) {
    switch(v) {
    case Status::Ready:return "Ready";case Status::Completed:return "Completed";
    case Status::WaitingForFrameBoundary:return "WaitingForFrameBoundary";
    case Status::Unimplemented:return "Unimplemented";case Status::Error:return "Error";case Status::Stopped:return "Stopped";
    } return "Unknown";
}
const char* name(Module v) {
    switch(v) {
    case Module::Lifecycle:return "Lifecycle";case Module::State:return "State";case Module::Time:return "Time";
    case Module::Input:return "Input candidate";case Module::World:return "World candidate";
    case Module::Presentation:return "Presentation candidate";case Module::UnresolvedCalls:return "UnresolvedCalls";
    } return "Unknown";
}
const char* name(Phase v) {
    switch(v) {
    case Phase::Startup:return "Startup";case Phase::FrameStart:return "FrameStart";case Phase::StateDispatch:return "StateDispatch";
    case Phase::PreDispatch:return "PreDispatch";case Phase::TimeStep:return "TimeStep";case Phase::WorldBranch:return "WorldBranch";
    case Phase::PostModules:return "PostModules";case Phase::Commit:return "Commit";case Phase::Boundary:return "Boundary";case Phase::Tail:return "Tail";
    } return "Unknown";
}
const char* name(EventKind v) {
    switch(v) {
    case EventKind::Executed:return "Executed";case EventKind::SyntheticPlaceholder:return "SyntheticPlaceholder";
    case EventKind::Skipped:return "Skipped";case EventKind::Branch:return "Branch";case EventKind::Waiting:return "Waiting";
    case EventKind::Boundary:return "Boundary";case EventKind::Unimplemented:return "Unimplemented";
    case EventKind::Error:return "Error";case EventKind::HostShutdown:return "HostShutdown";
    } return "Unknown";
}
void ModuleRegistry::bind_site(std::uint32_t pc, Handler handler) {
    site_at(pc);
    if (!handler) throw std::invalid_argument("Empty site handler");
    sites_[pc] = std::move(handler);
}
void ModuleRegistry::bind_target(std::uint32_t target, Handler handler) {
    if (!handler) throw std::invalid_argument("Empty target handler");
    targets_[target] = std::move(handler);
}
const Handler* ModuleRegistry::find(const CallSite& site) const {
    auto found = sites_.find(site.pc);
    if (found != sites_.end()) return &found->second;
    found = targets_.find(site.target);
    return found == targets_.end() ? nullptr : &found->second;
}
void EngineShell::fail(const CallSite& site, const std::string& reason) {
    context_.error = reason;
    context_.trace.push_back(event_for(site, context_, EventKind::Error, reason));
    status_ = Status::Error;
}
CallArguments EngineShell::arguments(const CallSite& s) const {
    CallArguments args;
    const auto& raw = context_.raw;
    switch(s.pc) {
    case 0x12f014:case 0x12f020:args.a[0]=boot_a0_;args.a[1]=boot_a1_;break;
    case 0x12f034:args.a[0]=0x2fc050;break;
    case 0x12f048:args.a[0]=1;args.a[1]=0;break;
    case 0x12f064:args.a={0x12f378,0x70004000,0,0};break;
    case 0x12f3d4:case 0x12f3dc:args.a[0]=raw.gp_a938;break;
    case 0x12f3ec:args.a[0]=raw.gp_a938;args.a[1]=3;args.a[2]=0;break;
    case 0x12f434:case 0x12f4d0:case 0x12f540:args.a[0]=raw.gp_a858;break;
    case 0x12f448:args.a[0]=saved_frame_stamp_;args.a[1]=context_.timing.current_stamp;break;
    case 0x12f454:args.f12=context_.timing.scaled_elapsed;break;
    case 0x12f488:args.a[0]=raw.gp_a998;args.f12=context_.timing.base_step;break;
    case 0x12f490:case 0x12f4c0:case 0x12f50c:args.a[0]=raw.gp_a998;break;
    case 0x12f4a4:case 0x12f4ec:args.f12=0.0f;break;
    case 0x12f518:args.f12=saved_world_step_;break;
    case 0x12f538:args.a[0]=raw.gp_a998;args.f12=saved_world_step_;break;
    case 0x12f550:args.f12=context_.timing.effective_step;break;
    case 0x12f5a4:args.a[0]=raw.a938_field2c;args.f12=context_.timing.base_step;break;
    case 0x12f5b0:args.f12=context_.timing.base_step;break;
    case 0x12f5f4:args.a[0]=raw.gp_a998;break;
    }
    // Unspecified registers remain unknown, not invented zero-valued arguments.
    return args;
}
bool EngineShell::invoke(std::uint32_t pc, CallResult& result) {
    const auto& site = site_at(pc);
    context_.current_pc = pc;context_.phase = site.phase;
    const auto args = arguments(site);
    const auto* handler = modules_.find(site);
    if (!handler && mode_ == Mode::Strict) {
        status_ = Status::Unimplemented;
        context_.error = "No handler for call site";
        context_.unresolved_sites.push_back(pc);
        context_.trace.push_back(event_for(site,context_,EventKind::Unimplemented,context_.error));
        return false;
    }
    if ((site.target==0x250f48 && (!args.a[0] || !args.a[1])) ||
        ((site.target==0x1a33f0 || site.target==0x1a35e0 || site.target==0x14dcd8 ||
          site.pc==0x12f488 || site.pc==0x12f5a4 || site.pc==0x12f5b0) && !args.f12)) {
        fail(site,"Required timing operand unresolved");return false;
    }
    if (args.f12 && !std::isfinite(*args.f12)) {fail(site,"Known timing operand is nonfinite");return false;}
    try {
        if (handler) result = (*handler)(context_,site,args);
        else {
            if (site.target==0x250f38) result.integer=(synthetic_tick_+=1000);
            if (site.target==0x250f48) result.scalar=fixtures_.elapsed_result;
            if (site.target==0x1680b8) result.integer=fixtures_.predicate_1680b8 ? 1 : 0;
            if (site.target==0x14d6e0) result.integer=fixtures_.predicate_14d6e0 ? 1 : 0;
            if (site.target==0x14d718) result.scalar=fixtures_.effective_step;
            if (site.target==0x1a33f0) context_.timing.base_step=fixtures_.base_step;
            context_.unresolved_sites.push_back(pc);
        }
    } catch (const std::exception& error) {fail(site,error.what());return false;}
      catch (...) {fail(site,"Handler threw an unknown exception");return false;}
    if (!result.success) {fail(site,result.error.empty() ? "Module handler failed" : result.error);return false;}
    context_.trace.push_back(event_for(site,context_,handler ? EventKind::Executed : EventKind::SyntheticPlaceholder,
        handler ? "Handler invocation returned; contract validation follows; original behavior unverified" : "Synthetic fixture/placeholder; no original module body executed"));
    return apply_result(site,result,args);
}
bool EngineShell::apply_result(const CallSite& s, const CallResult& result, const CallArguments& passed) {
    auto& timing=context_.timing;
    if (s.target==0x250f38 || s.target==0x1680b8 || s.target==0x14d6e0) {
        if (!result.integer) {fail(s,"Required integer result unresolved");return false;}
    }
    if (s.target==0x250f48 || s.target==0x14d718) {
        if (!result.scalar || !std::isfinite(*result.scalar)) {fail(s,"Required finite scalar result unresolved");return false;}
    }
    if (s.target==0x250f38) {
        timing.current_stamp=result.integer;
        if (s.pc==0x12f3a0 || s.pc==0x12f45c) {timing.previous_stamp=result.integer;saved_frame_stamp_=result.integer;}
        const std::map<std::uint32_t,unsigned> stamp_index={{0x12f3b0,0},{0x12f3fc,1},{0x12f4b4,2},
            {0x12f4fc,2},{0x12f528,2},{0x12f560,2},{0x12f56c,3},{0x12f57c,4},
            {0x12f5c0,5},{0x12f5d0,6},{0x12f604,7},{0x12f624,8},{0x12f664,9}};
        const auto it=stamp_index.find(s.pc);
        if (it!=stamp_index.end()) timing.stamps[it->second]=result.integer;
        if (s.pc==0x12f3b0) context_.raw.gp_a544=context_.raw.gp_b360;
    }
    if (s.target==0x250f48) {
        if (!timing.elapsed_scale) {fail(s,"Original elapsed scale fGpffff82d0 unresolved");return false;}
        timing.elapsed=result.scalar;timing.scaled_elapsed=*result.scalar * *timing.elapsed_scale;
        if (!std::isfinite(*timing.scaled_elapsed)) {fail(s,"Scaled timing is nonfinite");return false;}
    }
    if (s.target==0x14d718) {timing.effective_step=result.scalar;saved_world_step_=result.scalar;}
    if (s.target==0x1a35e0) timing.effective_step=passed.f12;
    return true;
}
void EngineShell::branch(std::uint32_t pc, bool taken, const char* condition) {
    context_.trace.push_back({context_.phase==Phase::Startup ? 0 : context_.completed_frames+1,pc,0,Module::UnresolvedCalls,context_.phase,
        EventKind::Branch,std::string(condition)+(taken ? " => true" : " => false"),
        context_.phase==Phase::Startup ? "E-0010-02" : "E-0010-01"});
}
void EngineShell::skip(std::uint32_t first, std::uint32_t last, const char* reason) {
    for (const auto& s:call_sites()) if (s.caller==0x12f378 && s.pc>=first && s.pc<=last)
        context_.trace.push_back(event_for(s,context_,EventKind::Skipped,reason));
}
std::uint32_t EngineShell::next(std::uint32_t pc, const CallResult& result) {
    auto& raw=context_.raw;
    switch(pc) {
    case 0x12f3c0:
        branch(0x12f3cc,raw.gp_a938!=0,"gp_a938 != 0");
        if (!raw.gp_a938) {skip(0x12f3d4,0x12f3ec,"Object absent");return 0x12f3f4;}break;
    case 0x12f404:
        branch(0x12f410,raw.gp_ac50==0,"gp_ac50 == 0");
        if (raw.gp_ac50) {skip(0x12f418,0x12f418,"Record/compare dispatch suppressed");return 0x12f420;}break;
    case 0x12f420:
        branch(0x12f42c,raw.gp_a858!=0,"gp_a858 != 0");
        if (!raw.gp_a858) {skip(0x12f434,0x12f434,"Record object absent");return 0x12f43c;}break;
    case 0x12f464:
        branch(0x12f46c,*result.integer!=0,"predicate_1680b8 != 0");
        if (*result.integer || !raw.gp_a938) {
            if (!*result.integer) branch(0x12f480,false,"gp_a938 != 0");
            skip(0x12f488,0x12f540,"World branch not selected");
            context_.timing.effective_step=*result.integer ? std::optional<float>(0.0f) : context_.timing.base_step;
            return 0x12f550;
        }
        branch(0x12f480,true,"gp_a938 != 0");
        if (!raw.gp_a998) {fail(site_at(pc),"World branch requires nonzero gp_a998 token; original precondition unresolved");return pc;}
        return 0x12f488;
    case 0x12f490:
        branch(0x12f498,*result.integer!=0,"predicate_14d6e0 != 0");
        if (*result.integer) {skip(0x12f4d0,0x12f540,"World gate active");return 0x12f4a4;}
        skip(0x12f4a4,0x12f4c0,"World gate inactive");return 0x12f4d0;
    case 0x12f4c0:skip(0x12f550,0x12f560,"Alternate world path handled");return 0x12f56c;
    case 0x12f4d0:
        if (!raw.gp_a858) {fail(site_at(pc),"0x0012F4D8 requires gp_a858 object token");return pc;}
        branch(0x12f4e0,raw.a858_field18!=0,"a858_field18 != 0");
        if (raw.a858_field18) {skip(0x12f50c,0x12f538,"Record/replay candidate branch");return 0x12f4ec;}
        skip(0x12f4ec,0x12f4fc,"World update path");return 0x12f50c;
    case 0x12f4fc:return 0x12f540;
    case 0x12f540:skip(0x12f550,0x12f560,"World path handled");return 0x12f56c;
    case 0x12f584:
        branch(0x12f590,raw.gp_a938!=0,"gp_a938 != 0");
        if (raw.gp_a938) branch(0x12f59c,raw.a938_field2c!=0,"a938_field2c != 0");
        if (!raw.gp_a938 || !raw.a938_field2c) {skip(0x12f5a4,0x12f5a4,"Optional callback absent");return 0x12f5b0;}break;
    case 0x12f5e0:
        branch(0x12f5ec,raw.gp_a998!=0,"gp_a998 != 0");
        if (!raw.gp_a998) {skip(0x12f5f4,0x12f5f4,"World token absent");return 0x12f5fc;}break;
    case 0x12f63c:return 0x12f65c;
    case 0x12f674:return 0x12f3b0;
    }
    const auto& sites=call_sites();
    const auto it=std::find_if(sites.begin(),sites.end(),[pc](const CallSite& s) {return s.pc==pc;});
    return (it+1)->pc;
}
Status EngineShell::initialize(Mode mode, RawState raw, SyntheticFixtures fixtures, std::uint64_t a0, std::uint64_t a1) {
    context_=FrameContext{};context_.raw=raw;startup_trace_.clear();synthetic_tick_=0;
    saved_frame_stamp_.reset();saved_world_step_.reset();
    mode_=mode;fixtures_=fixtures;boot_a0_=a0;boot_a1_=a1;status_=Status::Ready;pc_=0x12f014;
    if (mode==Mode::Observation) context_.timing.elapsed_scale=fixtures.elapsed_scale;
    for (auto pc:{0x12f014u,0x12f020u}) {
        CallResult result;
        if (!invoke(pc,result)) {startup_trace_=context_.trace;return status_;}
    }
    const auto alternate=context_.raw.gp_a54c!=0;
    branch(0x12f02c,alternate,"gp_a54c != 0 after bootstrap handlers");
    context_.trace.push_back(event_for(site_at(alternate ? 0x12f048 : 0x12f034),context_,EventKind::Skipped,"Other startup branch"));
    for (auto pc:{alternate ? 0x12f034u : 0x12f048u,0x12f064u,0x12f3a0u}) {
        CallResult result;
        if (!invoke(pc,result)) {startup_trace_=context_.trace;return status_;}
    }
    startup_trace_=context_.trace;context_.trace.clear();context_.unresolved_sites.clear();
    pc_=0x12f3b0;context_.current_pc=pc_;context_.phase=Phase::FrameStart;return status_;
}
Status EngineShell::step_frame(HostFrameEvent event) {
    if (status_==Status::Stopped || status_==Status::Error || status_==Status::Unimplemented) return status_;
    if (!std::isfinite(event.delta_seconds) || event.delta_seconds<0) {
        context_.error="Invalid host delta";status_=Status::Error;
        context_.trace.push_back({context_.completed_frames,context_.current_pc,0,Module::Lifecycle,context_.phase,
            EventKind::Error,context_.error,"host-only"});return status_;
    }
    if (status_==Status::Ready || status_==Status::Completed) {
        context_.trace.clear();context_.unresolved_sites.clear();context_.error.clear();context_.timing.stamps={};
        saved_world_step_.reset();
    }
    context_.host_delta_seconds=event.delta_seconds;context_.host_elapsed_seconds+=event.delta_seconds;
    bool boundary_available=event.boundary;
    for (unsigned budget=0;budget<100;++budget) {
        if (pc_==0x12f65c) {
            context_.current_pc=pc_;context_.phase=Phase::Boundary;
            if (boundary_available) {++context_.raw.gp_b360;boundary_available=false;}
            const auto difference=context_.raw.gp_b360-context_.raw.gp_a544;
            if (!difference || (difference & 0x80000000u)) {
                if (status_!=Status::WaitingForFrameBoundary)
                    context_.trace.push_back({context_.completed_frames+1,pc_,0,Module::Time,Phase::Boundary,
                        EventKind::Waiting,"Waiting for positive signed counter difference; no PS2 busy spin","E-0010-01"});
                status_=Status::WaitingForFrameBoundary;return status_;
            }
            context_.trace.push_back({context_.completed_frames+1,pc_,0,Module::Time,Phase::Boundary,
                EventKind::Boundary,"Boundary counter advanced; resume original tail","E-0010-01"});
            pc_=0x12f664;
        }
        const auto invoked_pc=pc_;
        CallResult result;
        if (!invoke(pc_,result)) return status_;
        pc_=next(pc_,result);
        if (status_==Status::Error) return status_;
        if (invoked_pc==0x12f674) {++context_.completed_frames;status_=Status::Completed;return status_;}
    }
    fail(site_at(pc_),"Port schedule budget exhausted");return status_;
}
void EngineShell::shutdown() {
    context_.trace.push_back({context_.completed_frames,context_.current_pc,0,Module::Lifecycle,context_.phase,
        EventKind::HostShutdown,"Host operation: pending frame cancelled; no original shutdown inferred","host-only"});
    status_=Status::Stopped;
}
RawState scenario_state(unsigned scenario) {
    RawState raw;
    if (scenario==1) {raw.gp_a938=0;raw.gp_a998=0;raw.gp_a858=0;}
    if (scenario==3) raw.a858_field18=1;
    if (scenario==5) {raw.gp_ac50=1;raw.a938_field2c=0;}
    return raw;
}
SyntheticFixtures scenario_fixtures(unsigned scenario) {
    SyntheticFixtures fixtures;
    if (scenario==2) fixtures.predicate_14d6e0=true;
    if (scenario==4) fixtures.predicate_1680b8=true;
    return fixtures;
}
} // namespace hp2::engine

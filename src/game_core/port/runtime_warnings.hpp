#pragma once
// TASK-0111. Port-side worker-safe warning retention; no original PS2 behavior,
// Godot calls, recovered defaults, or exception swallowing policy is supplied.
#include <cstdint>
#include <cstdio>
#include <limits>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace port::diagnostics {
struct RuntimeWarning {
    std::string address,reason,action;
    std::uint64_t count=0;
};
namespace detail {
struct WarningBus {
    std::mutex mutex;
    std::vector<RuntimeWarning> warnings;
    std::map<std::tuple<std::string,std::string,std::string>,std::size_t> index;
    std::vector<std::string> pending;
};
inline WarningBus& warning_bus(){static WarningBus bus;return bus;}
inline std::string warning_message(const RuntimeWarning& warning){
    return "WARNING [ELF "+warning.address+"] "+warning.reason+" | action: "+warning.action+" | count: "+std::to_string(warning.count);
}
}
// First occurrence and powers-of-two repeats are queued. Exact counters remain
// available regardless of UI visibility or message draining; repeated source
// gaps cannot flood the console once per simulation frame.
inline void warn(const std::string& address,const std::string& reason,const std::string& action) noexcept {
    try {
        auto& bus=detail::warning_bus();std::lock_guard<std::mutex> lock(bus.mutex);
        const auto key=std::make_tuple(address,reason,action);
        const auto found=bus.index.find(key);
        std::size_t index;
        if(found==bus.index.end()){
            index=bus.warnings.size();
            bus.warnings.push_back({address,reason,action,0});
            // Roll back a failed index insertion so a later warning can retry.
            try{bus.index.emplace(key,index);}catch(...){bus.warnings.pop_back();throw;}
        }else index=found->second;
        auto& warning=bus.warnings[index];
        if(warning.count==std::numeric_limits<std::uint64_t>::max())return;
        ++warning.count;
        if((warning.count&(warning.count-1))==0){
            const auto message=detail::warning_message(warning);
            // Native stderr is independent of the hidden/main-thread console.
            // The console executable used by build --run inherits this stream.
            std::fprintf(stderr,"%s\n",message.c_str());
            std::fflush(stderr);
            bus.pending.push_back(message);
        }
    }catch(...){
        // Even allocation exhaustion must not turn optional presentation into
        // a different simulation exception or replace a pending strict throw.
        std::fprintf(stderr,"WARNING [ELF %s] %s | action: %s | warning retention unavailable\n",address.c_str(),reason.c_str(),action.c_str());
    }
}
inline std::vector<RuntimeWarning> warnings_snapshot(){
    auto& bus=detail::warning_bus();std::lock_guard<std::mutex> lock(bus.mutex);return bus.warnings;
}
// Shutdown summary closes the gap between power-of-two repeat notices and the
// exact final occurrence count. Call after source workers have joined.
inline void print_warning_summary() noexcept {
    try {
        auto& bus=detail::warning_bus();std::lock_guard<std::mutex> lock(bus.mutex);
        for(const auto& warning:bus.warnings)std::fprintf(stderr,"FINAL WARNING [ELF %s] %s | action: %s | count: %llu\n",warning.address.c_str(),warning.reason.c_str(),warning.action.c_str(),static_cast<unsigned long long>(warning.count));
        std::fflush(stderr);
    } catch (...) {
        std::fprintf(stderr,"WARNING final diagnostic counters unavailable\n");
        std::fflush(stderr);
    }
}
// Called by the main-thread console owner only after it exists. An absent or
// hidden console does not discard messages; workers only append to this bus.
inline std::vector<std::string> take_warning_messages(){
    auto& bus=detail::warning_bus();std::lock_guard<std::mutex> lock(bus.mutex);
    std::vector<std::string> out;out.swap(bus.pending);return out;
}
}

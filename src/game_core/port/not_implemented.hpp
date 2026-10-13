// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#pragma once
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

// AGENT.md R3: a recovered code path that has not been implemented must fail loudly, naming the
// ELF address and what is missing. A silent default or a plausible approximation is
// indistinguishable from recovered behaviour at runtime, which is the exact failure mode the
// project's evidence rules exist to prevent.
//
// This header is the only place that decides what "fail loudly" does, and the only place that
// catches it. A call site states one fact and nothing else:
//
//     hp2::not_implemented("0x0016BAC0", "tPopupMenu joypad handling");
//
// and a boundary that reports the gap instead of propagating it uses one line:
//
//     result.not_implemented = hp2::guard([&] { ...recovered calls... });
//
// The call itself cannot be factored out of the site: the ELF address is the evidence of which
// original function is absent, nothing else in the program knows it, and no mechanism can infer
// that a path was never reversed. gta-reversed reaches the same conclusion -- its
// NOTSA_UNREACHABLE appears at 465 sites.
//
// Two things are borrowed from that project's notsa::unreachable (gta-reversed source/Base.h):
// a pluggable debug reaction that breaks into the debugger, and the idea that the reaction
// belongs in one place rather than at the sites. One thing is deliberately NOT borrowed: it
// compiles down to __builtin_unreachable() in release, because for it "unreachable" means a
// genuine invariant. Ours means "not reversed yet", which will be reached, so it must fail in
// every configuration -- turning it into UB would be exactly the silent-wrong-answer failure
// R3 exists to prevent.
namespace hp2 {

class NotImplemented : public std::logic_error {
public:
    explicit NotImplemented(std::string message) : std::logic_error(std::move(message)) {}
};

// Called before the throw when one is installed. A host uses this to stop in a debugger or show
// a dialog without game_core depending on any OS header. Returning is allowed and means "carry
// on reporting": the throw happens regardless, so execution never continues into untranslated
// code.
using NotImplementedHandler = void (*)(const char* elf_address, const char* what,
                                       const char* message);

namespace detail {
inline NotImplementedHandler& not_implemented_handler_slot() {
    static NotImplementedHandler handler = nullptr;
    return handler;
}
// Zero-configuration fallback: HP2_ASSERT_NOT_IMPLEMENTED=1 routes through the standard
// assert() so a debug build stops at the site with no host code at all.
inline bool assert_requested() {
    static const bool requested = [] {
        const char* value = std::getenv("HP2_ASSERT_NOT_IMPLEMENTED");
        return value != nullptr && *value != '\0' && *value != '0';
    }();
    return requested;
}
} // namespace detail

// Install (or, with nullptr, remove) the debug reaction. One call, in one place, at startup.
inline void set_not_implemented_handler(NotImplementedHandler handler) {
    detail::not_implemented_handler_slot() = handler;
}

// The one report point for a reached-but-untranslated recovered path.
[[noreturn]] inline void not_implemented(const char* elf_address, const char* what) {
    std::string message = "hp2::NotImplemented: ";
    message += what;
    message += " (ELF ";
    message += elf_address;
    message += " not implemented; see tasks/active and reverse/functions records)";

    if (const auto handler = detail::not_implemented_handler_slot()) {
        handler(elf_address, what, message.c_str());
    } else if (detail::assert_requested()) {
        std::fprintf(stderr, "%s\n", message.c_str());
        assert(false && "hp2::not_implemented -- see stderr for the ELF address");
    }
    throw NotImplemented(std::move(message));
}

// Runs `body` and returns the message of the NotImplemented it raised, or an empty string when
// it completed. This is the only place the project catches NotImplemented, so a boundary that
// reports gaps does not grow its own try/catch.
template <class Body>
std::string guard(Body&& body) {
    try {
        std::forward<Body>(body)();
    } catch (const NotImplemented& error) {
        return error.what();
    }
    return {};
}

} // namespace hp2

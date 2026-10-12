#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// TASK-0011: frontend/menu screen-switch structural projection.
//
// Evidence (SLUS_203.62, sha256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2):
//   RequestScreen     <- FUN_0014cc78 (thin wrapper) -> FUN_0016c4c0
//   FindScreenByName  <- FUN_0016c8c8 (circular list scan, strcmp == FUN_002535f8)
//   PushHistory       <- FUN_0016b418 (manager+0x180 count; slot = manager+count*0x20)
//   PopHistory        <- FUN_0016b438 (same slot math, count--)
//   Reset             <- FUN_0016b460 (HP2_FillBytes(manager,0,0x184))
// A string literal "New Main Menu" at ELF 0x002b2f00 is the only confirmed screen name;
// "TitleScreen" (0x002b2f68), "MumboJumboScreen" (0x002b2f78), "Title Screen" (0x002b2f90),
// "Legal MumboJumbo" (0x002b2fa0), "BootAutoload" (0x002b2fb8) and
// "Language Options Pre FE" (0x002b2fc8) are confirmed sibling names pushed as history by
// FUN_0016f950, which also requests "New Main Menu" (push_history=true) when a two-field
// gated state is reached. See docs/FRONTEND_MENU.md (E-0011-*) for full evidence.
//
// This class is a *structural projection*: it reproduces the observed manager-level state
// machine (registry / current screen / bounded history stack) with Godot-independent C++.
// It does not reproduce screen-internal rendering, widget trees, or the per-widget message
// handler (FUN_0016eef8) that triggers a request from player input; that wiring is tracked
// as an open item in docs/FRONTEND_MENU.md. Never treat this as a verified original
// reconstruction: original behavior comparison has not been performed.
namespace hp2::frontend {

inline constexpr const char* binary_sha256 = "7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2";

// manager+0x180 is a byte count in the original; slots are 0x20 bytes each, so the inline
// array fits exactly 12 entries before it would run into the following +0x184 flag byte.
// The original overflow behavior at the 13th push is unresolved; this port refuses instead
// of inventing wraparound/corruption semantics.
inline constexpr std::size_t kHistoryCapacity = 12;

enum class RequestOutcome {
    Switched,       // FUN_0016c8c8 found the name; current screen updated.
    NotFound,       // Lookup returned 0; original leaves current screen cleared (no error).
    ClearedToNone,  // name was empty; mirrors the param_2==0 branch in FUN_0016c4c0.
    HistoryFull     // Port-only guard; original capacity/overflow behavior is unresolved.
};

struct HistoryEntry {
    std::string name; // Only the name field (first bytes of the 0x20-byte slot) is evidenced.
};

class ScreenManager {
public:
    // Registers a screen name as reachable, modeling an entry in the circular descriptor
    // list scanned by FUN_0016c8c8. Order/placement semantics of the original list are not
    // reproduced; this is a simple set-like registry keyed by name.
    void register_screen(std::string name);
    bool is_registered(const std::string& name) const;
    const std::vector<std::string>& registered_screens() const { return registry_; }

    // Models FUN_0014cc78(name, push_history) -> FUN_0016c4c0(manager, name, push_history).
    // When a current screen exists and push_history is true, its name is pushed to history
    // before switching (FUN_0016c4c0's `if (param_3 != 0) FUN_0016b418(...)` branch).
    RequestOutcome request_screen(const std::string& name, bool push_history);

    bool has_current() const { return current_.has_value(); }
    const std::string& current_screen() const;

    std::size_t history_depth() const { return history_.size(); }
    const std::vector<HistoryEntry>& history() const { return history_; }

    // Composes PopHistory (FUN_0016b438) with RequestScreen(name, push_history=false), the
    // same push_history=false pattern FUN_0016f950 uses for its own resolved-target switch.
    // This composition is a structural hypothesis for "Back" navigation, not a function
    // confirmed at a single original call site; see docs/FRONTEND_MENU.md open items.
    std::optional<RequestOutcome> go_back();

    // Models FUN_0016b460: clears history + count, leaves the registry untouched.
    void reset();

private:
    std::vector<std::string> registry_;
    std::vector<HistoryEntry> history_;
    std::optional<std::string> current_;
};

} // namespace hp2::frontend

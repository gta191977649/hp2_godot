#include "game_core/port/frontend/screen_manager.hpp"
#include <stdexcept>
using namespace hp2::frontend;
namespace {
void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }

void empty_manager_has_no_current() {
    ScreenManager manager;
    require(!manager.has_current(), "fresh manager must have no current screen");
    require(manager.history_depth() == 0, "fresh manager must have empty history");
}

void request_unregistered_name_is_not_found() {
    ScreenManager manager;
    require(manager.request_screen("New Main Menu", true) == RequestOutcome::NotFound, "unregistered name must report NotFound");
    require(!manager.has_current(), "NotFound must not set a current screen");
}

void request_registered_name_switches() {
    ScreenManager manager;
    manager.register_screen("New Main Menu");
    require(manager.request_screen("New Main Menu", true) == RequestOutcome::Switched, "registered name must switch");
    require(manager.has_current() && manager.current_screen() == "New Main Menu", "current screen must match request");
    require(manager.history_depth() == 0, "switching from no current screen must not push history");
}

// Mirrors the evidenced chain: TitleScreen -> MumboJumboScreen -> New Main Menu (FUN_0016f950).
void evidenced_attract_chain_pushes_history() {
    ScreenManager manager;
    for (const char* name : {"TitleScreen", "MumboJumboScreen", "New Main Menu"}) manager.register_screen(name);
    require(manager.request_screen("TitleScreen", true) == RequestOutcome::Switched, "enter TitleScreen");
    require(manager.request_screen("MumboJumboScreen", true) == RequestOutcome::Switched, "enter MumboJumboScreen");
    require(manager.history_depth() == 1, "leaving TitleScreen must push it to history");
    require(manager.history().back().name == "TitleScreen", "history top must be the screen left behind");
    require(manager.request_screen("New Main Menu", true) == RequestOutcome::Switched, "enter New Main Menu");
    require(manager.history_depth() == 2, "leaving MumboJumboScreen must push it to history");
    require(manager.current_screen() == "New Main Menu", "current screen must be New Main Menu");
}

void go_back_restores_previous_screen_without_repushing() {
    ScreenManager manager;
    for (const char* name : {"TitleScreen", "MumboJumboScreen"}) manager.register_screen(name);
    manager.request_screen("TitleScreen", true);
    manager.request_screen("MumboJumboScreen", true);
    const auto outcome = manager.go_back();
    require(outcome.has_value() && *outcome == RequestOutcome::Switched, "go_back must switch to the popped screen");
    require(manager.current_screen() == "TitleScreen", "go_back must restore TitleScreen");
    require(manager.history_depth() == 0, "go_back must not re-push the restored screen");
}

void go_back_with_empty_history_reports_nullopt() {
    ScreenManager manager;
    manager.register_screen("New Main Menu");
    manager.request_screen("New Main Menu", true);
    require(!manager.go_back().has_value(), "go_back with empty history must report no-op, not a false switch");
}

void request_empty_name_clears_current() {
    ScreenManager manager;
    manager.register_screen("New Main Menu");
    manager.request_screen("New Main Menu", false);
    require(manager.request_screen("", false) == RequestOutcome::ClearedToNone, "empty name must clear current screen");
    require(!manager.has_current(), "cleared manager must report no current screen");
}

void history_capacity_is_bounded() {
    ScreenManager manager;
    for (std::size_t i = 0; i <= kHistoryCapacity + 1; ++i) manager.register_screen("S" + std::to_string(i));
    manager.request_screen("S0", false); // no previous screen to push yet
    for (std::size_t i = 1; i <= kHistoryCapacity; ++i) {
        require(manager.request_screen("S" + std::to_string(i), true) == RequestOutcome::Switched, "each distinct switch must push exactly one history entry");
    }
    require(manager.history_depth() == kHistoryCapacity, "history must saturate at the evidenced 12-slot capacity");
    require(manager.request_screen("S" + std::to_string(kHistoryCapacity + 1), true) == RequestOutcome::HistoryFull, "13th push must be refused, not invented");
}

void reset_clears_history_but_keeps_registry() {
    ScreenManager manager;
    manager.register_screen("New Main Menu");
    manager.register_screen("TitleScreen");
    manager.request_screen("TitleScreen", false);
    manager.request_screen("New Main Menu", true);
    require(manager.history_depth() == 1, "precondition: history has one entry");
    manager.reset();
    require(manager.history_depth() == 0, "reset must clear history");
    require(manager.is_registered("New Main Menu") && manager.is_registered("TitleScreen"), "reset must not clear the registry");
}
} // namespace

int main() {
    empty_manager_has_no_current();
    request_unregistered_name_is_not_found();
    request_registered_name_switches();
    evidenced_attract_chain_pushes_history();
    go_back_restores_previous_screen_without_repushing();
    go_back_with_empty_history_reports_nullopt();
    request_empty_name_clears_current();
    history_capacity_is_bounded();
    reset_clears_history_but_keeps_registry();
    return 0;
}

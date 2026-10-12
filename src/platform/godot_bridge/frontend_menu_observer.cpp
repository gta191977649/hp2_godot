#include "frontend_menu_observer.hpp"
#include <godot_cpp/core/class_db.hpp>

using namespace hp2::frontend;

namespace port::godot_bridge {
namespace {
godot::String outcome_name(RequestOutcome value) {
    switch (value) {
        case RequestOutcome::Switched: return "Switched";
        case RequestOutcome::NotFound: return "NotFound";
        case RequestOutcome::ClearedToNone: return "ClearedToNone";
        case RequestOutcome::HistoryFull: return "HistoryFull";
    }
    return "Unknown";
}
} // namespace

void FrontEndMenuObserver::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("initialize"), &FrontEndMenuObserver::initialize);
    godot::ClassDB::bind_method(godot::D_METHOD("request_screen", "name", "push_history"), &FrontEndMenuObserver::request_screen);
    godot::ClassDB::bind_method(godot::D_METHOD("go_back"), &FrontEndMenuObserver::go_back);
    godot::ClassDB::bind_method(godot::D_METHOD("reset"), &FrontEndMenuObserver::reset);
    godot::ClassDB::bind_method(godot::D_METHOD("snapshot"), &FrontEndMenuObserver::snapshot);
    godot::ClassDB::bind_method(godot::D_METHOD("get_registered_screens"), &FrontEndMenuObserver::get_registered_screens);
    godot::ClassDB::bind_method(godot::D_METHOD("get_history"), &FrontEndMenuObserver::get_history);
}

godot::Dictionary FrontEndMenuObserver::initialize() {
    manager_ = ScreenManager();
    // Confirmed registered screen classes (TASK-0011 evidence; see docs/FRONTEND_MENU.md):
    //   "TitleScreen"       - FUN_0016f440 class dispatcher (ctor via FUN_002577c0, name 0x2b2f68)
    //   "MumboJumboScreen"  - FUN_0016f810 class dispatcher (ctor via FUN_002577c0, name 0x2b2f78)
    //   "New Main Menu"     - string literal 0x2b2f00; requested by TitleScreen's instance
    //                         handler (FUN_0016eef8, message 1) and by FUN_0016f950/FUN_0016fa80.
    //                         Its own class dispatcher/constructor was not located this session.
    manager_.register_screen("TitleScreen");
    manager_.register_screen("MumboJumboScreen");
    manager_.register_screen("New Main Menu");
    manager_.request_screen("TitleScreen", false);
    return snapshot();
}

godot::Dictionary FrontEndMenuObserver::request_screen(const godot::String& name, bool push_history) {
    manager_.request_screen(name.utf8().get_data(), push_history);
    return snapshot();
}

godot::Dictionary FrontEndMenuObserver::go_back() {
    manager_.go_back();
    return snapshot();
}

godot::Dictionary FrontEndMenuObserver::reset() {
    manager_.reset();
    return snapshot();
}

godot::Dictionary FrontEndMenuObserver::snapshot() const {
    godot::Dictionary row;
    row["has_current"] = manager_.has_current();
    row["current_screen"] = godot::String::utf8(manager_.current_screen().c_str());
    row["history_depth"] = static_cast<std::int64_t>(manager_.history_depth());
    row["history_capacity"] = static_cast<std::int64_t>(kHistoryCapacity);
    row["elf_sha256"] = binary_sha256;
    row["verification"] = "Structural projection of the screen-switch manager; original behavior unverified";
    return row;
}

godot::Array FrontEndMenuObserver::get_registered_screens() const {
    godot::Array array;
    for (const auto& name : manager_.registered_screens()) array.append(godot::String::utf8(name.c_str()));
    return array;
}

godot::Array FrontEndMenuObserver::get_history() const {
    godot::Array array;
    for (const auto& entry : manager_.history()) array.append(godot::String::utf8(entry.name.c_str()));
    return array;
}

} // namespace port::godot_bridge

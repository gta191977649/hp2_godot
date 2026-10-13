#include "game_core/indep/src/frontendscreens/options.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/frontend/frontendscreens/registry_0016adb0.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    try {
        std::string requested, animated;
        int requests = 0, preparations = 0;
        bool history = false;
        port::frontend::ScreenActionServices services{
            [&](const char* name, bool push) { requested = name; history = push; ++requests; },
            [&](const char* name) { animated = name; },
            [&] { ++preparations; }};
        auto check = [](bool value) { if (!value) throw std::runtime_error("action mismatch"); };
        // 37 FUN_0016ADB0 registrations plus the one TASK-0021 added: "View Event Tree"
        // registers through the sibling FUN_0016AD88 at 0x001FB860, which is why it was missed.
        check(port::frontend::registrations().size() == 38);
        const auto* tree = port::frontend::find_registration("View Event Tree");
        check(tree != nullptr && tree->document_hash == 0x147F8A59u &&
              tree->handler_address == 0x001FB218u);
        const auto* language = port::frontend::find_registration("Language Options Pre FE");
        check(language && language->document_hash == 0x632B068E);
        const auto* name_entry = port::frontend::find_registration("NameEntry");
        check(name_entry && name_entry->document_hash == 0x617A4E0A && name_entry->handler_address == 0x0024DBD8);
        check(port::frontend::find_registration("missing") == nullptr);
        std::int32_t mode = -1;
        hp2::frontend::Sub_00216638(1, 0x52E039F3, mode, services);
        check(mode == 1 && requested == "Cops On Mode Select" && history);
        hp2::frontend::Sub_00216638(1, 0x52E039F4, mode, services);
        check(mode == 0 && requested == "Cops Off Mode Select");
        hp2::frontend::Sub_00216638(1, 0x52E039F5, mode, services);
        check(requested == "Options");
        hp2::frontend::Sub_00216638(1, 0xFFFFFFFF, mode, services);
        hp2::frontend::Sub_00216638(4, 0x52E039F3, mode, services);
        check(requests == 3 && mode == 0);
        const char* targets[] = {"Audio Options", "Joystick PreConfig", "Display Options",
            "Phils Load Save", "NameEntry", "Credits", "Language Options"};
        for (unsigned i = 0; i < 7; ++i) {
            requested.clear(); animated.clear();
            hp2::frontend::Sub_001EECA8(1, 0x52E039F3 + i, services);
            const bool direct = i == 3 || i == 5 || i == 6;
            check((direct ? requested : animated) == targets[i]);
            check((direct ? animated : requested).empty());
        }
        check(preparations == 1);
        bool threw = false;
        try { hp2::frontend::Sub_001EECA8(1, 0, services); }
        catch (const hp2::NotImplemented&) { threw = true; }
        check(threw);
        std::cout << "FRONTEND_ACTIONS_OK\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

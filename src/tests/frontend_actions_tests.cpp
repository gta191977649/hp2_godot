#include "game_core/indep/src/frontendscreens/options.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/frontend/frontendscreens/registry_0016adb0.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

        // ---------------------------------------------- Language Options, TASK-0025 / E-0025-L*
        check(language->handler_address == 0x001F1DD8u);
        check(port::frontend::find_registration("Language Options")->handler_address == 0x001F1DD8u);
        std::vector<std::pair<std::uint32_t, std::uint32_t>> calls;
        std::int32_t* loader_target = nullptr; // what 0021E0D0 writes DAT_0031C1C8 through
        port::frontend::ScreenActionServices language_services{
            nullptr, nullptr, nullptr,
            [&](std::uint32_t address, std::uint32_t argument) {
                if (address == 0x00183830)
                    hp2::not_implemented("0x00183830", "full profile reset not translated");
                calls.emplace_back(address, argument);
                // 0021E0D0 owns the global; the handler re-reads what the loader stored.
                if (address == 0x0021E0D0 && loader_target)
                    *loader_target = static_cast<std::int32_t>(argument);
            }};
        // "Language Options": +0x08 == 1, so no 00183830 reset. MENUITEM_1..6 -> 0..5, and each
        // one applies the locale before the manager pop.
        for (std::uint32_t i = 0; i < 6; ++i) {
            hp2::frontend::LanguageOptionsScreenBase instance; instance.field_0x08 = 1;
            std::int32_t language_global = 0, dat_00331A60 = 0;
            calls.clear(); loader_target = &language_global;
            hp2::frontend::Sub_001F1B08(1, 0x52E039F3 + i, instance, language_global,
                                        dat_00331A60, language_services);
            check(instance.field_0x00 == static_cast<std::int32_t>(i));
            check(language_global == static_cast<std::int32_t>(i));
            check(dat_00331A60 == 0);
            check(calls == std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                              {0x0021E0D0, i}, {0x00170320, 1}});
        }
        {   // "Language Options Pre FE": +0x08 == 0 reaches the untranslated 00183830 reset.
            hp2::frontend::LanguageOptionsScreenBase instance; instance.field_0x08 = 0;
            std::int32_t language_global = 0, dat_00331A60 = 0;
            calls.clear(); threw = false; loader_target = &language_global;
            try {
                hp2::frontend::Sub_001F1B08(1, 0x52E039F4, instance, language_global,
                                            dat_00331A60, language_services);
            } catch (const hp2::NotImplemented&) { threw = true; }
            check(threw && calls == std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                                        {0x0021E0D0, 1}});
        }
        {   // MENUITEM_7: writes DAT_00331A60 and pops, with no locale change.
            hp2::frontend::LanguageOptionsScreenBase instance; instance.field_0x08 = 1;
            std::int32_t language_global = 3, dat_00331A60 = 0;
            calls.clear(); loader_target = nullptr;
            hp2::frontend::Sub_001F1B08(1, 0x52E039F9, instance, language_global,
                                        dat_00331A60, language_services);
            check(dat_00331A60 == 1 && language_global == 3 && instance.field_0x00 == 0);
            check(calls == std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0x00170320, 1}});
        }
        {   // Out-of-range hashes and non-activation messages leave everything alone, and an
            // unset language (-1) blocks the pop after the loader ran.
            hp2::frontend::LanguageOptionsScreenBase instance; instance.field_0x08 = 1;
            std::int32_t language_global = 0, dat_00331A60 = 0;
            calls.clear(); loader_target = nullptr;
            hp2::frontend::Sub_001F1B08(1, 0x52E039F2, instance, language_global,
                                        dat_00331A60, language_services);
            hp2::frontend::Sub_001F1B08(1, 0x52E039FA, instance, language_global,
                                        dat_00331A60, language_services);
            hp2::frontend::Sub_001F1B08(4, 0x52E039F3, instance, language_global,
                                        dat_00331A60, language_services);
            check(calls.empty() && instance.field_0x00 == 0 && dat_00331A60 == 0);
            port::frontend::ScreenActionServices unset{nullptr, nullptr, nullptr,
                [&](std::uint32_t address, std::uint32_t argument) {
                    calls.emplace_back(address, argument);
                    if (address == 0x0021E0D0) language_global = -1;
                }};
            hp2::frontend::Sub_001F1B08(1, 0x52E039F5, instance, language_global,
                                        dat_00331A60, unset);
            check(calls == std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0x0021E0D0, 2}});
        }
        {   // A missing service is a loud failure, never a dropped locale change.
            hp2::frontend::LanguageOptionsScreenBase instance;
            std::int32_t language_global = 0, dat_00331A60 = 0;
            threw = false;
            try {
                hp2::frontend::Sub_001F1B08(1, 0x52E039F3, instance, language_global,
                                            dat_00331A60, services);
            } catch (const hp2::NotImplemented&) { threw = true; }
            check(threw);
        }
        // 001F1A78 construction: +0x00 seeded from DAT_0031C1C8, -1 clamped to 0.
        {
            hp2::frontend::FeScreen screen;
            hp2::frontend::LanguageOptionsScreenBase instance;
            hp2::frontend::Construct_001F1A78(instance, screen, nullptr, 4, 1);
            check(instance.field_0x00 == 4 && instance.field_0x08 == 1);
            hp2::frontend::Construct_001F1A78(instance, screen, nullptr, -1, 0);
            check(instance.field_0x00 == 0 && instance.field_0x08 == 0);
        }
        std::cout << "FRONTEND_ACTIONS_OK\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

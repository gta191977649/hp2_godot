// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#include "game_core/port/frontend/frontendscreens/registry_0016adb0.hpp"

namespace port::frontend {
namespace {
// E-0015-REG-01: literal registration operands; original TU unknown.
//
// TASK-0021 follow-up (E-0021-22): one row was missing, and its absence is why selecting
// Championship or Ultimate Racer did nothing. Both mode-select handlers request the class
// "View Event Tree", `find_registration` returned nullptr for it, and
// `FrontEndRuntime::apply_pending_screen` raised NotImplemented at 0x0016C8C8 instead of
// opening screen 39.
//
// The row was missed because this class does **not** register through FUN_0016ADB0 like the
// other 37: it tail-calls the sibling FUN_0016AD88 at 0x001FB860. Its operands are built at
// 0x001FB7F0..0x001FB830 -- `lui a1,0x2c` + `addiu a1,a1,-0x58e8` gives the class-name string
// 0x002BA718 ("View Event Tree"), `lui a2,0x20` + `addiu a2,a2,-0x4de8` gives the handler
// 0x001FB218, and `lui a3,0x147f` + `ori a3,a3,0x8a59` gives the document hash 0x147F8A59.
// That hash is exactly FRONTB.BUN screen 39's stored name hash, which is the confirmation.
//
// Worth recording because it is easy to get wrong: a row's document hash is the **screen's**
// stored name hash, not StringHash_002534A8 of the class name in the same row. The two differ
// -- StringHash_002534A8("View Event Tree") is 0xFB14F36C, and screen 39 stores 0x147F8A59 --
// because the class name and the authored document name are different strings.
constexpr std::array<Registration, 63> table{{
    {"Popup", 0x037F3133u, 0x00000000u}, // call 0x0016E8E8
    {"Select Car Dialog", 0x41C2FDC3u, 0x00000000u}, // call 0x0016E90C
    {"Select Car Dialog P1", 0x6344DA01u, 0x00000000u}, // call 0x0016E930
    {"Select Car Dialog P2", 0x6344DA02u, 0x00000000u}, // call 0x0016E954
    {"Challenge", 0x2D62CFE2u, 0x001B6BD0u}, // call 0x001B991C
    {"ChallengeBeACop", 0xE55F1297u, 0x001B6F18u}, // call 0x001B9944
    {"Single Race Setup", 0xD1EADBCAu, 0x001B8AE8u}, // call 0x001B9970
    {"Free Run Setup", 0xD1EADBCAu, 0x001B8C98u}, // call 0x001B999C
    {"Lap Knockout Setup", 0xD1EADBCAu, 0x001B8F30u}, // call 0x001B99CC
    {"Knockout Setup", 0xD1EADBCAu, 0x001B9210u}, // call 0x001B99F4
    {"Be The Cop Setup", 0xD1EADBCAu, 0x001B93D8u}, // call 0x001B9A2C
    {"Options", 0x95AD096Bu, 0x001EEF00u}, // call 0x001F56AC
    {"Audio Options", 0xE116761Cu, 0x001F0278u}, // call 0x001F56D8
    {"JukeBox Music", 0x95B2B627u, 0x001F1220u}, // call 0x001F5704
    {"Username Options", 0x617A4E0Au, 0x001F19E8u}, // call 0x001F5730
    {"Language Options", 0x632B068Eu, 0x001F1DD8u}, // call 0x001F5760
    {"Language Options Pre FE", 0x632B068Eu, 0x001F1DD8u}, // call 0x001F5784
    {"Credits", 0xFD8A3B6Du, 0x001F21B0u}, // call 0x001F57B0
    {"Credits FE Movie", 0xB5761D00u, 0x001F2470u}, // call 0x001F57D8
    {"Credits Reward movie", 0xB5761D00u, 0x001F2470u}, // call 0x001F57F8
    {"BBG Creation movie", 0xB5761D00u, 0x001F2470u}, // call 0x001F5818
    {"BBG Credits movie", 0xB5761D00u, 0x001F2470u}, // call 0x001F5838
    {"EACredits", 0x39C4C62Cu, 0x001F3320u}, // call 0x001F5868
    {"BBGCredits", 0x39C4C62Cu, 0x001F3320u}, // call 0x001F5888
    {"Display Options", 0x8BC68340u, 0x001F3EC0u}, // call 0x001F58B4
    {"Customize P1 HUD", 0x28891A22u, 0x001F4960u}, // call 0x001F58E4
    {"Customize P2 HUD", 0x28891A22u, 0x001F49C8u}, // call 0x001F590C
    {"Calibrate Display", 0xDDF09124u, 0x001F4A30u}, // call 0x001F593C
    {"Calibrate display offset", 0x7AB428C1u, 0x001F4B20u}, // call 0x001F5974
    {"New Main Menu", 0xAE0F8AA1u, 0x00216A10u}, // call 0x00218064
    {"Cops On Mode Select", 0x5B35F533u, 0x002174A0u}, // call 0x00218090
    {"Cops Off Mode Select", 0x033E7771u, 0x00217AF8u}, // call 0x002180C4
    {"Joystick PreConfig", 0x8BD5060Bu, 0x00240060u}, // call 0x00241214
    {"Phils Load Save", 0x9829BFABu, 0x00247830u}, // call 0x00247E14
    {"BootAutoload", 0xFA2AC5C6u, 0x00247AB0u}, // call 0x00247E3C
    {"Autosave", 0xFA2AC5C6u, 0x00247D00u}, // call 0x00247E6C
    {"NameEntry", 0x617A4E0Au, 0x0024DBD8u}, // call 0x0024DCA4
    {"View Event Tree", 0x147F8A59u, 0x001FB218u}, // tail call 0x001FB860 -> FUN_0016AD88
    // TASK-0098 entry prerequisites: E-0091-G01 registration operands.
    // Registration is not implementation/readiness of a screen handler.
    {"Select Car", 0xB5074AB4u, 0x00198928u}, // E-0091-G01@00198BC8
    {"Select Car2P", 0x18DC8E52u, 0x00198928u}, // E-0091-G01@00198BC8
    {"Select Car2P", 0x18DC8E53u, 0x00198928u}, // E-0091-G01@00198BC8
    {"Select Track", 0x15417B53u, 0x001A0920u}, // E-0091-G01@001A09E8
    {"Pause Menu", 0x605D2BF1u, 0x00171060u}, // E-0091-G01@00174B20
    {"Pause audio", 0x6B321E4Eu, 0x00171898u}, // E-0091-G01@00174B20
    {"Pause display", 0xCC36FAB2u, 0x00173D88u}, // E-0091-G01@00174B20
    {"Pause Options", 0x2B5A03A8u, 0x001729F0u}, // E-0091-G01@00174B20
    {"Race Objective", 0x887B4D37u, 0x00171BB8u}, // E-0091-G01@00174B20
    {"Race Objective 2PBAC", 0xB5ECD7A0u, 0x00171BB8u}, // E-0091-G01@00174B20
    {"Pause BAC controls", 0x887B4D37u, 0x00171FD0u}, // E-0091-G01@00174B20
    {"Debug Menu", 0xD4742CD7u, 0x00174590u}, // E-0091-G01@00174B20
    {"Points Rewards", 0x7E420558u, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Task Rewards", 0x7E420558u, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Events Unlocked", 0x7E420558u, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Reward menu", 0x7E420558u, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Race Results", 0xAB049B2Bu, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Tourney Standings", 0x0532B6FFu, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Best Lap Times", 0xACB2A4CAu, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Best Total Times", 0xF5A3CF51u, 0x001DD278u}, // E-0091-G01@001E1F88
    {"End Game Menu", 0x1A64E903u, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Be The Cop Results 1P", 0x67EB8D12u, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Be The Cop Results 2P", 0x67EB8D33u, 0x001DD278u}, // E-0091-G01@001E1F88
    {"Lap Times 1P", 0xC4881CBDu, 0x001E10F0u}, // E-0091-G01@001E1EEC
    {"Lap Times 2P", 0xC4881CDEu, 0x001E10F0u}, // E-0091-G01@001E1EEC
}};
} // namespace

const std::array<Registration, 63>& registrations() noexcept { return table; }

const Registration* find_registration(std::string_view name) noexcept {
    for (const auto& entry : table) {
        if (entry.class_name == name) return &entry;
    }
    return nullptr;
}

const Registration* find_registration(std::string_view name, std::uint32_t document_hash) noexcept {
    for (const auto& entry : table) {
        if (entry.class_name == name && entry.document_hash == document_hash) return &entry;
    }
    return nullptr;
}

const Registration* find_registration_for_document_hash(std::uint32_t hash) noexcept {
    for (const auto& entry : table) {
        if (entry.document_hash == hash) return &entry;
    }
    return nullptr;
}

} // namespace port::frontend

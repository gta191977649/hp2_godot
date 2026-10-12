#include "game_core/frontend/frontendscreens/registry_0016adb0.hpp"

namespace port::frontend {
namespace {
// E-0015-REG-01: literal registration operands; original TU unknown.
constexpr std::array<Registration, 37> table{{
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
}};
} // namespace

const std::array<Registration, 37>& registrations() noexcept { return table; }

const Registration* find_registration(std::string_view name) noexcept {
    for (const auto& entry : table) {
        if (entry.class_name == name) return &entry;
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

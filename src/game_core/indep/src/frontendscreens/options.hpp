// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\frontendscreens\options.cpp`
// (string at 0x002B9E00, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, and the `.hpp` extension is port-side -- only
// the `.cpp` path was recovered.
// `.cpp` path was recovered.
#pragma once
#include <cstdint>
#include <functional>
#include <array>
#include <optional>
#include "game_core/indep/src/fmenus.hpp"

namespace port::frontend {
// Port-side dependency seam, not an original PS2 ABI/layout.
struct ScreenActionServices {
    std::function<void(const char*, bool)> request;
    std::function<void(const char*)> exit_animation;
    std::function<void()> display_prepare;
    // Original function address and immediate parameter, as in BackServices::invoke. The
    // activation slices that call out of the front end (the locale loader, the profile reset,
    // the manager pop) name the address they are standing in for, so the host seam cannot
    // silently substitute a different effect.
    std::function<void(std::uint32_t, std::uint32_t)> invoke;
};
// Host projection of manager 0016C670's message-0x11 payload; not the PS2 ABI.
struct BackEvent { std::int32_t permission=1, sound=6; };
// Explicit branch inputs for partial screen recovery. Missing inputs fail loudly;
// callers must capture state, never assume an unknown settings comparison is false.
struct BackContext {
    bool exit_active=false;
    std::uint32_t autosave=0;
    std::optional<bool> settings_changed;
    std::optional<bool> keyboard_active;
    std::optional<std::int32_t> language;
    std::uint32_t selected_hash=0;
    std::uint32_t language_instance_0x08=0;
    std::int32_t loadsave_state_0x54=0;
    bool jukebox_selected_enabled=false, jukebox_selected_loaded=false;
    std::array<std::uint8_t,8> display_field_0x64{}, dat_002FC19C{};
};
struct BackServices {
    // Original function address and immediate parameter; effects are host services.
    std::function<void(std::uint32_t,std::uint32_t)> invoke;
    std::function<void(const char*)> push_history;
};
// Port-side routing only; handler slices below retain their actual ELF addresses.
BackEvent query_back(std::uint32_t handler_address,BackContext& context,const BackServices& services);
}
namespace hp2::frontend {
// ELF 0x00248568/0x001DAEC8; E-0015-21. Only the zero-velocity exit path is modelled.
struct Type_00248568 {
    std::uint32_t field_0x0c = 0, field_0x10 = 0;
    float field_0x18 = 0, field_0x1c = 0, field_0x24 = 650;
    float field_0x28 = 0, field_0x40 = 0;
    float field_0x2c = 0, field_0x30 = 0, field_0x34 = 0, field_0x38 = 0;
    std::uint16_t field_0x48 = 2;
};
// ELF 0x002490D8; E-0015-21. Host stores the request target separately from PS2 pointers.
class Type_002490D8 {
public:
    void Construct_002490D8(FeScreen& screen);
    bool Update_00249128(float delta_seconds);
private:
    FeScreen* field_0x00 = nullptr;
    std::vector<Type_00248568> field_0x04;
    // E-0019-N01: original name unknown. Shared by entrance/exit lists initialized
    // at 00248BE8; membership in this port projection is not original class evidence.
    void Sub_00248C60(float x, float duration, std::uint32_t group, std::uint32_t mask);
    void Sub_002486E8(Type_00248568& path);
};
// ELF 0x00216638; E-0015-11, reverse/functions/<binary-id>/frontend/00216638.md.
// Only message 1 is translated here; visuals are separately driven by the port adapter.
void Sub_00216638(int message, std::uint32_t item_hash, std::int32_t& dat_002FC070,
                  const ::port::frontend::ScreenActionServices& services);
// ELF 0x001EECA8; E-0015-12, corresponding function record. Message-1 action slice.
void Sub_001EECA8(int message, std::uint32_t item_hash,
                  const ::port::frontend::ScreenActionServices& services);

// ------------------------------------------------------- Language Options (TASK-0025, E-0025-L*)
// MENUITEM_1..MENUITEM_6 are the six languages the screen offers; MENUITEM_7 is the extra item
// 001F1A78 always disables and hides. Hashes under StringHash_002534A8, which is why they are
// contiguous. The same six values appear on other screens with other meanings -- they are
// positional item names, not language names.
inline constexpr std::uint32_t kWidgetMenuItem1_52E039F3 = 0x52E039F3u;
inline constexpr std::uint32_t kWidgetMenuItem6_52E039F8 = 0x52E039F8u;
inline constexpr std::uint32_t kWidgetMenuItem7_52E039F9 = 0x52E039F9u;
// Pointer table at ELF 0x0031C168, read verbatim. Index 6 (LANGUAGES\FE_LONGEST.BIN) and 7
// (LANGUAGES\FE_ID.BIN) exist in the table but no menu item selects them. (E-0025-L03)
inline constexpr int kLanguageCount_0031C168 = 6;

// ELF 0x001F1A78 instance. "Language Options Pre FE" allocates 0x10 bytes and constructs with
// 001F1A78 directly; "Language Options" allocates 0x58 and goes through 001F1D70, which
// forwards to 001F1A78 and then builds a 001EE918 sub-object at +0x10 that is NOT translated.
// Only the three words 001F1B08 reads are modelled. (E-0025-L01)
struct Type_001F1A78 {
    // +0x00: the language the screen will apply. Seeded from DAT_0031C1C8, or 0 when that is -1.
    std::int32_t field_0x00 = 0;
    // +0x08: the constructor's third argument. 0 for "Language Options Pre FE", 1 for
    // "Language Options". Only 0 reaches the 00183830 full profile reset.
    std::int32_t field_0x08 = 0;
};
// ELF 0x001F1A78 / 0x001F1D70, message 0x0B of 001F1DD8. `list` and `screen` are what the
// original resolves from the current class name inside 0016D2A8.
void Construct_001F1A78(Type_001F1A78& instance, FeScreen& screen, MenuList_00169F78* list,
                        std::int32_t dat_0031C1C8, std::int32_t field_0x08);
// ELF 0x001F1B08 message 0x01, reached through the class router 0x001F1DD8. `dat_0031C1C8` is
// the current-language global the locale loader 0021E0D0 owns; `dat_00331A60` is the global the
// MENUITEM_7 arm writes, whose consumer is unresolved (see TASK-0025). The 0x11 (Back) slice is
// the separate overload below.
void Sub_001F1B08(int message, std::uint32_t item_hash, Type_001F1A78& instance,
                  std::int32_t& dat_0031C1C8, std::int32_t& dat_00331A60,
                  const ::port::frontend::ScreenActionServices& services);
// ELF001EFB80 messages4/8/9, E-0017-S04. Shared viewport utility is fmenus;
// screen-specific selector target and nonwrapping end guards are the Audio handler.
void Sub_001EFB80(int message,MenuEvent_0016A458& event,Type_00249DE0& viewport,
                  Type_0024A690& selector,FeScreen& screen,const WidgetMeasure& measure);
// ELF001F3A28 messages4/8/9; E-0017-S05, Display viewport/selector slice.
void Sub_001F3A28(int message,MenuEvent_0016A458& event,Type_00249DE0& viewport,
                  Type_0024A690& selector,FeScreen& screen,const WidgetMeasure& measure);
// Message0x11 slices, SLUS_203.62 SHA7335b8ed...acc2; E-0017-B01..B08.
void Sub_001EFB80(::port::frontend::BackContext&,::port::frontend::BackEvent&,const ::port::frontend::BackServices&);
void Sub_001F3A28(::port::frontend::BackContext&,::port::frontend::BackEvent&,const ::port::frontend::BackServices&);
void Sub_002402C0(::port::frontend::BackContext&,::port::frontend::BackEvent&,const ::port::frontend::BackServices&);
void Sub_0024D870(::port::frontend::BackContext&,::port::frontend::BackEvent&,const ::port::frontend::BackServices&);
void Sub_001F1B08(::port::frontend::BackContext&,::port::frontend::BackEvent&,const ::port::frontend::BackServices&);
void Sub_001F0D10(::port::frontend::BackContext&,::port::frontend::BackEvent&,const ::port::frontend::BackServices&);
void Sub_001F1568(::port::frontend::BackContext&,::port::frontend::BackEvent&,const ::port::frontend::BackServices&);
}

namespace port::frontend {
// Port-side reuse seam for the recovered 001DA780/001DAEC8 zero-velocity path.
bool advance_zero_velocity_path(hp2::frontend::Type_00248568& path, float delta_seconds);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: preserved_existing_module; class-to-original-file binding not asserted.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::frontend {
class AudioOptionsScreen {
public:
    // ELF 0x001EF3C8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001ef3c8.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001EF3C8();
    // ELF 0x001EFA00; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001efa00.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001EFA00();
    // ELF 0x001EFB78; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001efb78.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001EFB78();
    // ELF 0x001EFB80; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001efb80.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001EFB80();
};
} // namespace hp2::frontend

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: preserved_existing_module; class-to-original-file binding not asserted.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::frontend {
class JukeBoxMusicScreen {
public:
    // ELF 0x001F02D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f02d8.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F02D8();
    // ELF 0x001F0460; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001f0460.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F0460();
    // ELF 0x001F0D10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f0d10.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F0D10();
    // ELF 0x001F4E68; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f4e68.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F4E68();
};
} // namespace hp2::frontend

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: preserved_existing_module; class-to-original-file binding not asserted.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::frontend {
class tDisplayOptionsScreen {
public:
    // ELF 0x001F33B8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f33b8.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F33B8();
    // ELF 0x001F3588; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001f3588.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F3588();
    // ELF 0x001F35E0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001f35e0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F35E0();
    // ELF 0x001F3A28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f3a28.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F3A28();
};
} // namespace hp2::frontend

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: preserved_existing_module; class-to-original-file binding not asserted.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::frontend {
class tOptionsScreen {
public:
    // ELF 0x001EEBB8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001eebb8.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001EEBB8();
    // ELF 0x001EEC50; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001eec50.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001EEC50();
    // ELF 0x001EECA8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001eeca8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001EECA8();
    // ELF 0x001F4E68; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f4e68.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F4E68();
};
} // namespace hp2::frontend

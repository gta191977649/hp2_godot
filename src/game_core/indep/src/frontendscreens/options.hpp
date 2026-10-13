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

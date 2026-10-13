// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\frontendscreens\joystickconfiguration.cpp`
// (string at 0x002BE300, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, not the original translation unit.
#include "options.hpp"
#include "game_core/port/not_implemented.hpp"

namespace hp2::frontend {
// ELF002402C0 message0x11, jump table002BE510 ->00240308; E-0017-B03.
// Compares per-player cached enabled bytes to00240B50's DAT0030CDCD/0030D49D.
// Settings comparison is a host input until profile controller state is represented.
void Sub_002402C0(::port::frontend::BackContext& context,::port::frontend::BackEvent&,
                     const ::port::frontend::BackServices& services) {
    if(!context.settings_changed)hp2::not_implemented("0x002402C0","controller profile comparison missing");
    if(*context.settings_changed&&context.autosave) {
        if(!services.push_history)hp2::not_implemented("0x001F5C28","controller Autosave history service missing");
        services.push_history("Autosave");
    }
}
}

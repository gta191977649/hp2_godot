#include "options.hpp"
#include "game_core/not_implemented.hpp"

namespace hp2::frontend {
// ELF002402C0 message0x11, jump table002BE510 ->00240308; E-0017-B03.
// Compares per-player cached enabled bytes to00240B50's DAT0030CDCD/0030D49D.
// Settings comparison is a host input until profile controller state is represented.
void Sub_002402C0(::port::frontend::BackContext& context,::port::frontend::BackEvent&,
                     const ::port::frontend::BackServices& services) {
    if(!context.settings_changed)HP2_NOT_IMPLEMENTED("0x002402C0","controller profile comparison missing");
    if(*context.settings_changed&&context.autosave) {
        if(!services.push_history)HP2_NOT_IMPLEMENTED("0x001F5C28","controller Autosave history service missing");
        services.push_history("Autosave");
    }
}
}

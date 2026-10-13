#pragma once

// TASK-0020 follow-up: the debugger reaction for a reached-but-untranslated recovered path.
//
// Modelled on gta-reversed's `notsa::unreachable` (that project's `source/Base.h`): log the
// details, then show an Abort / Retry / Ignore box where Retry breaks into the debugger. It
// lives here rather than in game_core so the recovered core keeps depending on nothing but the
// standard library -- the hook it installs into is `hp2::set_not_implemented_handler`.
//
// Installing it is opt-in, because a modal box during normal play is worse than a reported gap:
// `install_not_implemented_dialog()` does nothing unless `HP2_NOT_IMPLEMENTED_DIALOG=1` is set.
// Whatever the viewer chooses, `hp2::not_implemented` still throws afterwards, so execution
// never continues into untranslated code.
//
// On a platform without a message box this falls back to writing the details to stderr; the
// `HP2_ASSERT_NOT_IMPLEMENTED=1` path in not_implemented.hpp remains available everywhere.
namespace hp2::platform {

void install_not_implemented_dialog();

} // namespace hp2::platform

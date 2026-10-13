#include "platform/not_implemented_dialog.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace hp2::platform {
namespace {

bool dialog_requested() {
    const char* value = std::getenv("HP2_NOT_IMPLEMENTED_DIALOG");
    return value != nullptr && *value != '\0' && *value != '0';
}

void report(const char* elf_address, const char* what, const char* message) {
    // stderr first and unbuffered-flushed, so the details survive even if the viewer kills the
    // process from the dialog. gta-reversed flushes its spdlog sinks here for the same reason.
    std::fprintf(stderr, "%s\n", message);
    std::fflush(stderr);

#ifdef _WIN32
    char body[1024];
    std::snprintf(body, sizeof(body),
                  "A recovered code path that has not been translated was reached.\n\n"
                  "Original function: ELF %s\n"
                  "Missing: %s\n\n"
                  "Retry breaks into the debugger. Abort and Ignore both continue, and the port\n"
                  "then reports the gap through hp2::NotImplemented -- execution never carries on\n"
                  "into untranslated code.",
                  elf_address, what);
    const int choice = MessageBoxA(nullptr, body, "hp2: not implemented",
                                   MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);
    if (choice == IDRETRY) {
#if defined(_MSC_VER)
        __debugbreak();
#elif defined(__GNUC__)
        __builtin_trap();
#endif
    }
#else
    (void)elf_address;
    (void)what;
#endif
}

} // namespace

void install_not_implemented_dialog() {
    if (!dialog_requested()) return;
    hp2::set_not_implemented_handler(&report);
}

} // namespace hp2::platform

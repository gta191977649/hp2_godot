#pragma once
#include <stdexcept>
#include <string>

// AGENT.md R3: a recovered code path that has not been implemented must fail loudly, naming the
// ELF address and what is missing. A silent default or a plausible approximation is
// indistinguishable from recovered behaviour at runtime, which is the exact failure mode the
// project's evidence rules exist to prevent.
namespace hp2 {

class NotImplemented : public std::logic_error {
public:
    explicit NotImplemented(std::string message) : std::logic_error(std::move(message)) {}
};

[[noreturn]] inline void throw_not_implemented(const char* elf_address, const char* what) {
    std::string message = "hp2::NotImplemented: ";
    message += what;
    message += " (ELF ";
    message += elf_address;
    message += " not implemented; see tasks/active and reverse/functions records)";
    throw NotImplemented(std::move(message));
}

} // namespace hp2

// HP2_NOT_IMPLEMENTED("0x0016BAC0", "tPopupMenu joypad handling");
#define HP2_NOT_IMPLEMENTED(elf_address, what) ::hp2::throw_not_implemented((elf_address), (what))

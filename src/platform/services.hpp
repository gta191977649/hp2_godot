#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

namespace port::platform {
// Scaffold contracts, not inferred original engine interfaces. Narrow or revise
// them only when a recovered call site provides evidence for a real contract.
struct Rendering { virtual ~Rendering() = default; virtual void present() = 0; };
struct Audio { virtual ~Audio() = default; virtual void stop_all() = 0; };
struct Input { virtual ~Input() = default; virtual bool action_pressed(std::string_view action) const = 0; };
struct FileSystem {
    virtual ~FileSystem() = default;
    virtual bool read(std::string_view path, std::vector<std::uint8_t>& bytes) const = 0;
};
struct Timing { virtual ~Timing() = default; virtual std::uint64_t monotonic_microseconds() const = 0; };
struct Diagnostics { virtual ~Diagnostics() = default; virtual void log(std::string_view message) = 0; };
// Non-owning; implementations and their lifetime belong to the host.
struct Services { Rendering& rendering; Audio& audio; Input& input; FileSystem& files; Timing& timing; Diagnostics& diagnostics; };
}

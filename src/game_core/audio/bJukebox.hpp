#pragma once
#include "sound.hpp"
#include <optional>

// Original __FILE__ is src/bJukebox.cpp. This is port-side cartridge I/O; scheduling
// projection retains ELF addresses. E-0016-A04, A05; hardware equivalence unverified.
namespace hp2::port::audio {
struct MusicTune {
    std::string name;
    std::string category;
    std::uint32_t offset = 0, size = 0, sync_size = 0;
    int type = 0; // Exact name field 4, parsed by original 001AEFD0 via001AED90.
};
class JukeboxCartridge {
public:
    static JukeboxCartridge load_from_zzdata(const std::string& root);
    const std::vector<MusicTune>& tunes() const { return tunes_; }
    PcmSound decode(std::size_t index) const;
private:
    std::string path_;
    std::vector<MusicTune> tunes_;
};

// Original selection matrix002B6300, FUN001AF050(mode5). Genre/user disable flags
// are explicit caller input; never assumed enabled by this predicate.
bool Sub_001AF050(int type, bool genre_enabled);
// Single category projection of FUN0027ECA8. Original increments before searching,
// wraps in authored order and leaves the cursor unchanged if no enabled tune exists.
std::optional<std::size_t> Sub_0027ECA8(const std::vector<bool>& enabled, std::size_t& cursor);
// FUN0025A3D0 counter-seeded integer transform, source frame counter supplied by host.
int Sub_0025A3D0(int count, std::uint32_t& seed);

// Frontend-only projection of003115D0/001AEAB0. Fresh original AudioOptions permits
// all genres (001EF1C8 +0xaf=0xff). User saves and race scheduling are not represented.
class FrontendMusic {
public:
    explicit FrontendMusic(const std::vector<MusicTune>& tunes);
    std::optional<std::size_t> enter(std::uint32_t frame_counter);
    std::optional<std::size_t> finished(std::uint32_t frame_counter);
private:
    std::vector<bool> enabled_;
    std::size_t cursor_=0;
    bool first_frontend_=true; // gp b610 initial1, consumed by001AEA40 category5.
    bool first_random_=true; // gp b61c initial1, consumed by001AEAB0 fallback.
    bool playing_=false;
};
} // namespace hp2::port::audio

// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so its
// name and location are this project's, not the original project's.
#include "ps2_profile_store.hpp"

#include "ps2_asset_root.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

namespace hp2::platform {
namespace {
namespace fs = std::filesystem;

std::optional<std::string> env_value(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return std::nullopt;
    return std::string(value);
}

// The per-user fallback. Nothing here is recovered: the original writes to a memory card
// through the PS2 driver, which has no host equivalent, so a host convention is chosen and
// labelled as such.
fs::path default_save_root() {
    if (auto local_app_data = env_value("LOCALAPPDATA")) {
        return fs::path(*local_app_data) / "hp2_godot" / "saves";
    }
    if (auto xdg = env_value("XDG_DATA_HOME")) {
        return fs::path(*xdg) / "hp2_godot" / "saves";
    }
    if (auto home = env_value("HOME")) {
        return fs::path(*home) / ".local" / "share" / "hp2_godot" / "saves";
    }
    return fs::path("local") / "saves";
}

fs::path save_root(const std::string& search_start_dir) {
    if (auto configured = env_value(kSaveRootEnvVar)) return fs::path(*configured);
    if (auto configured = resolve_local_config_value(search_start_dir, "port_save_root")) {
        return fs::path(*configured);
    }
    return default_save_root();
}

std::optional<std::vector<std::uint8_t>> read_whole_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::nullopt;
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
}

} // namespace

std::string profile_store_path(const std::string& search_start_dir, int region) {
    const fs::path path = save_root(search_start_dir) /
                          hp2::memorycard::Sub_0012DEE0(region) /
                          hp2::memorycard::kProfileFileName_002BEBE0;
    return path.generic_string();
}

std::optional<std::string> resolve_profile_path(const std::string& search_start_dir, int region) {
    if (auto configured = env_value(kProfilePathEnvVar)) return configured;
    if (auto configured = resolve_local_config_value(search_start_dir, "profile_path")) {
        return configured;
    }
    const std::string own = profile_store_path(search_start_dir, region);
    std::error_code ec;
    if (fs::is_regular_file(fs::path(own), ec)) return own;
    return std::nullopt;
}

ProfileLoad load_profile(const std::string& search_start_dir, int region,
                         hp2::memorycard::Profile_002FC050& profile) {
    ProfileLoad result;
    auto path = resolve_profile_path(search_start_dir, region);
    if (!path) {
        result.error = "not configured";
        return result;
    }
    result.path = *path;

    auto payload = read_whole_file(fs::path(*path));
    if (!payload) {
        result.error = "cannot open " + *path;
        return result;
    }

    // The recovered half decides what a valid payload is; this file only supplies the bytes.
    switch (hp2::memorycard::Load_00245D30(*payload, profile)) {
    case hp2::memorycard::LoadResult_00245780::kOk:
        result.loaded = true;
        return result;
    case hp2::memorycard::LoadResult_00245780::kWrongLength:
        result.error = "wrong length";
        return result;
    case hp2::memorycard::LoadResult_00245780::kBadChecksum:
        result.error = "bad checksum";
        return result;
    }
    result.error = "unknown load result";
    return result;
}

std::string store_profile(const std::string& search_start_dir, int region,
                          hp2::memorycard::Profile_002FC050& profile) {
    const fs::path path(profile_store_path(search_start_dir, region));

    // A configured HP2_PS2_PROFILE_PATH is read-only input. profile_store_path never points at
    // it unless the user deliberately set port_save_root to the same place, and this check
    // catches that case rather than overwriting what they pointed us at.
    if (auto configured = env_value(kProfilePathEnvVar)) {
        std::error_code ec;
        if (fs::equivalent(fs::path(*configured), path, ec)) {
            return "refusing to overwrite the configured read-only profile at " + *configured;
        }
    }

    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    if (ec) return "cannot create " + path.parent_path().generic_string() + ": " + ec.message();

    const std::vector<std::uint8_t> payload = hp2::memorycard::Store_00245738(profile);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return "cannot open " + path.generic_string() + " for writing";
    file.write(reinterpret_cast<const char*>(payload.data()),
               static_cast<std::streamsize>(payload.size()));
    if (!file) return "write failed for " + path.generic_string();
    return std::string();
}

} // namespace hp2::platform

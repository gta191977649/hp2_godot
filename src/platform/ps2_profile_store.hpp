// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so its
// name and location are this project's, not the original project's. Do not read this path as
// evidence of where the original kept this code.
#pragma once
#include "game_core/memorycard/include/memorycard.h"

#include <optional>
#include <string>

// TASK-0021: the host half of the save system. `src/game_core/memorycard` is the recovered
// half and touches no file; this file is the only place that does, so the recovered code stays
// free of `<filesystem>` and of any host path convention (AGENT.md R4, AGENTS.md rule 7).
//
// Reading is opt-in and off by default. With nothing configured the runtime gets a zeroed
// `Profile_002FC050`, which is exactly what the original's own FUN_00245700 hands the driver
// before a load, so the port's behaviour is unchanged from before this task.
//
// Writing never goes near a real memory card and never overwrites a configured read path: a
// configured `profile_path` is treated as read-only input, and stores land under the port's own
// save root instead. That is a port-side safety decision, recorded in docs/DECISIONS.md D009.
namespace hp2::platform {

// Resolution order for the optional read path, mirroring resolve_zzdata_root's shape:
//  1. the HP2_PS2_PROFILE_PATH environment variable,
//  2. config/ps2_assets.local.json's "profile_path" key.
inline constexpr const char* kProfilePathEnvVar = "HP2_PS2_PROFILE_PATH";

// Resolution order for the port's own save root:
//  1. the HP2_PORT_SAVE_ROOT environment variable,
//  2. config/ps2_assets.local.json's "port_save_root" key,
//  3. a per-user host location (%LOCALAPPDATA%/hp2_godot/saves on Windows, otherwise
//     $XDG_DATA_HOME/hp2_godot/saves or $HOME/.local/share/hp2_godot/saves).
inline constexpr const char* kSaveRootEnvVar = "HP2_PORT_SAVE_ROOT";

struct ProfileLoad {
    // True only when a payload was found, was 0x114E0 bytes and passed FUN_001878B0's checksum.
    bool loaded = false;
    // The path that was read, or empty when nothing was configured or present.
    std::string path;
    // Empty when `loaded`, otherwise why not -- "not configured", "wrong length",
    // "bad checksum" or a filesystem reason. This is a runtime condition, not an untranslated
    // path, so it is reported rather than thrown.
    std::string error;
};

// Where a store would go: <save root>/<BASLUS-20362HP2>/settings.db. Pure path arithmetic; it
// creates nothing and reads nothing.
std::string profile_store_path(const std::string& search_start_dir, int region);

// The configured read path, if any. Prefers HP2_PS2_PROFILE_PATH, then the local config key,
// then the port's own store path when that file already exists -- so a profile this port wrote
// is picked up on the next run without any configuration at all.
std::optional<std::string> resolve_profile_path(const std::string& search_start_dir, int region);

// Resolve, read and verify. On any failure `profile` is left exactly as it was handed in.
ProfileLoad load_profile(const std::string& search_start_dir, int region,
                         hp2::memorycard::Profile_002FC050& profile);

// Stamp the size word and checksum through the recovered Store_00245738, then write 0x114E0
// bytes to profile_store_path(), creating the save directory if needed. Returns an empty string
// on success, otherwise the reason. Never writes to a configured HP2_PS2_PROFILE_PATH.
std::string store_profile(const std::string& search_start_dir, int region,
                          hp2::memorycard::Profile_002FC050& profile);

} // namespace hp2::platform

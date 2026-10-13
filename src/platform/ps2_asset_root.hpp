#pragma once
#include <optional>
#include <string>

// TASK-0012: the single Godot-independent entry point for resolving an authorized local PS2
// ZZDATA copy at runtime. Mirrors tools/ps2_assets.py's resolution order and config schema
// exactly (same env var name, same config/ps2_assets.local.json "root" key) so the Python
// offline-analysis tools and this native runtime loader agree on where assets live without
// duplicating the configured path. Never hardcode a ZZDATA path anywhere else; go through this.
namespace hp2::platform {

inline constexpr const char* kAssetRootEnvVar = "HP2_PS2_ASSET_ROOT";

// Resolution order (first match wins), matching tools/ps2_assets.py:
//  1. The HP2_PS2_ASSET_ROOT environment variable (works regardless of where this runs from).
//  2. config/ps2_assets.local.json's "root" key, found by walking upward from `search_start_dir`
//     (e.g. the running executable's directory) looking for a "config" subdirectory -- this
//     avoids hardcoding the offset between a build/export output location and the repo root.
// Returns the ZZDATA directory itself (containing FRONTEND/, GLOBAL/, CARS/, ...), accepting a
// configured root that points either directly at ZZDATA or at its GAME/ZZDATA-containing parent.
// Returns std::nullopt if nothing is configured or the configured path doesn't look like ZZDATA.
std::optional<std::string> resolve_zzdata_root(const std::string& search_start_dir);

// The same config lookup as step 2 above, for any flat string key in
// config/ps2_assets.local.json. Added by TASK-0021 so the optional save-profile keys resolve
// through this one entry point instead of a second copy of the file walk and the extractor.
std::optional<std::string> resolve_local_config_value(const std::string& search_start_dir,
                                                     const std::string& key);

} // namespace hp2::platform

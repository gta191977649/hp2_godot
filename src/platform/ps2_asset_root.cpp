#include "ps2_asset_root.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace hp2::platform {
namespace {
namespace fs = std::filesystem;

// Minimal, targeted extraction of the "root" string value from config/ps2_assets.local.json.
// This is NOT a general JSON parser: it only handles this repo's own config writer output
// (config/ps2_assets.example.json / ps2_assets.local.json), which is always a flat
// {"root": "...", ...} object with no escaped quotes inside the path. Do not reuse this for
// arbitrary JSON.
std::optional<std::string> extract_root_value(const std::string& json_text) {
    std::size_t key = json_text.find("\"root\"");
    if (key == std::string::npos) return std::nullopt;
    std::size_t colon = json_text.find(':', key);
    if (colon == std::string::npos) return std::nullopt;
    std::size_t open_quote = json_text.find('"', colon);
    if (open_quote == std::string::npos) return std::nullopt;
    std::size_t close_quote = json_text.find('"', open_quote + 1);
    if (close_quote == std::string::npos) return std::nullopt;
    return json_text.substr(open_quote + 1, close_quote - open_quote - 1);
}

bool looks_like_zzdata(const fs::path& root) {
    std::error_code ec;
    return fs::is_directory(root / "FRONTEND", ec);
}

// Walks upward from `start_dir` (at most a handful of levels, matching this repo's known
// directory depth) looking for config/ps2_assets.local.json, so the resolver works whether this
// runs from the repo root, a build output directory, or an exported executable's folder.
std::optional<fs::path> find_local_config(const fs::path& start_dir) {
    fs::path dir = start_dir;
    std::error_code ec;
    for (int level = 0; level < 8; ++level) {
        fs::path candidate = dir / "config" / "ps2_assets.local.json";
        if (fs::is_regular_file(candidate, ec)) return candidate;
        fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) break;
        dir = parent;
    }
    return std::nullopt;
}

} // namespace

std::optional<std::string> resolve_zzdata_root(const std::string& search_start_dir) {
    std::optional<std::string> configured_root;

    if (const char* env = std::getenv(kAssetRootEnvVar); env && *env) {
        configured_root = std::string(env);
    } else if (auto config_path = find_local_config(fs::path(search_start_dir))) {
        std::ifstream file(*config_path);
        if (file) {
            std::ostringstream buffer;
            buffer << file.rdbuf();
            configured_root = extract_root_value(buffer.str());
        }
    }

    if (!configured_root || configured_root->empty()) return std::nullopt;

    fs::path root(*configured_root);
    if (looks_like_zzdata(root)) return root.generic_string();
    if (looks_like_zzdata(root / "GAME" / "ZZDATA")) return (root / "GAME" / "ZZDATA").generic_string();
    return std::nullopt;
}

} // namespace hp2::platform

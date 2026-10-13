#pragma once
#include "game_core/port/assets/hp2_tpk_texture.hpp"
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <map>
#include <optional>

// TASK-0012: Godot-facing wrapper around hp2::assets::Hp2TpkArchive. Loads real HP2 texture
// packages directly from the user's configured local ZZDATA copy at runtime (see
// src/platform/ps2_asset_root.hpp for the resolution order) -- no pre-extracted PNGs are shipped
// or required; this is the only supported path for getting real original textures into Godot.
namespace port::godot_bridge {

class Ps2TextureLoader : public godot::RefCounted {
    GDCLASS(Ps2TextureLoader, godot::RefCounted)
protected:
    static void _bind_methods();

public:
    // Dictionary: {"ok": bool, "root": String, "error": String}.
    godot::Dictionary resolve_root() const;

    // relative_path uses forward slashes under the resolved ZZDATA root, e.g. "FRONTEND/FRONTB.BUN".
    // Returns an ImageTexture on success, or a null Ref on failure (check get_last_error()).
    godot::Ref<godot::ImageTexture> load_texture(const godot::String& relative_path, const godot::String& texture_name);

    // Lists texture names available in the given archive (for diagnostics/tools, not required
    // for normal menu use).
    godot::Array list_textures(const godot::String& relative_path);

    // Resolves a TexturePack entry hash (as stored in frontend widget records at +0x50) against
    // every bundle in the search list, decoding and caching on first use. Returns a null Ref when
    // the hash is not present in any searched bundle.
    godot::Ref<godot::ImageTexture> load_texture_by_hash(std::int64_t texture_hash);

    // Same lookup keyed on the texture's name, hashed with the recovered front-end string hash
    // (hp2::frontend::StringHash_002534A8, ELF 0x002534A8) so callers never have to reimplement
    // it. Used for the names the recovered code carries as strings, e.g. the event tree's
    // SPEED_EVENT_PANEL_* / PURSUIT_EVENT_PANEL_* panels.
    godot::Ref<godot::ImageTexture> load_texture_by_name(const godot::String& texture_name);

    // Bundles searched by load_texture_by_hash, as ZZDATA-relative forward-slash paths. Defaults
    // to the frontend/global bundles that actually carry frontend widget textures.
    void set_search_bundles(const godot::PackedStringArray& bundles);
    godot::PackedStringArray get_search_bundles() const;

    godot::String get_last_error() const { return last_error_; }

private:
    bool load_archive(const godot::String& relative_path, hp2::assets::Hp2TpkArchive& out_archive);
    const hp2::assets::Hp2TpkArchive* cached_archive(const godot::String& relative_path);

    godot::String last_error_;
    std::map<std::string, hp2::assets::Hp2TpkArchive> archive_cache_;
    std::map<std::uint32_t, godot::Ref<godot::ImageTexture>> texture_cache_;
    godot::PackedStringArray search_bundles_;
};

} // namespace port::godot_bridge

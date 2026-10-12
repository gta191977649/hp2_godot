#pragma once
#include "game_core/assets/hp2_tpk_texture.hpp"
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
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

    godot::String get_last_error() const { return last_error_; }

private:
    bool load_archive(const godot::String& relative_path, hp2::assets::Hp2TpkArchive& out_archive);
    godot::String last_error_;
};

} // namespace port::godot_bridge

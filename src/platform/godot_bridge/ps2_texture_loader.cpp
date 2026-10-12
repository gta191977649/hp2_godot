#include "ps2_texture_loader.hpp"
#include "platform/ps2_asset_root.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

using namespace hp2::assets;

namespace port::godot_bridge {
namespace {

std::string search_start_dir() {
    // Prefer the Godot project directory (works when running via the editor/portable runtime
    // from the source tree); fall back to the running executable's own directory (works for an
    // exported standalone build placed anywhere under the repo).
    godot::String project_path = godot::ProjectSettings::get_singleton()->globalize_path("res://");
    if (!project_path.is_empty()) return std::string(project_path.utf8().get_data());
    godot::String exe_path = godot::OS::get_singleton()->get_executable_path().get_base_dir();
    return std::string(exe_path.utf8().get_data());
}

} // namespace

void Ps2TextureLoader::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("resolve_root"), &Ps2TextureLoader::resolve_root);
    godot::ClassDB::bind_method(godot::D_METHOD("load_texture", "relative_path", "texture_name"), &Ps2TextureLoader::load_texture);
    godot::ClassDB::bind_method(godot::D_METHOD("list_textures", "relative_path"), &Ps2TextureLoader::list_textures);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_error"), &Ps2TextureLoader::get_last_error);
}

godot::Dictionary Ps2TextureLoader::resolve_root() const {
    godot::Dictionary result;
    auto root = hp2::platform::resolve_zzdata_root(search_start_dir());
    if (root) {
        result["ok"] = true;
        result["root"] = godot::String::utf8(root->c_str());
        result["error"] = "";
    } else {
        result["ok"] = false;
        result["root"] = "";
        result["error"] = godot::String("No PS2 asset root configured. Set the ") +
            godot::String(hp2::platform::kAssetRootEnvVar) +
            godot::String(" environment variable, or create config/ps2_assets.local.json "
                           "(see config/ps2_assets.example.json).");
    }
    return result;
}

bool Ps2TextureLoader::load_archive(const godot::String& relative_path, Hp2TpkArchive& out_archive) {
    auto root = hp2::platform::resolve_zzdata_root(search_start_dir());
    if (!root) {
        last_error_ = "PS2 asset root not configured (see resolve_root()).";
        return false;
    }
    godot::String full_path = godot::String::utf8(root->c_str()).path_join(relative_path);
    godot::Ref<godot::FileAccess> file = godot::FileAccess::open(full_path, godot::FileAccess::READ);
    if (file.is_null()) {
        last_error_ = godot::String("Could not open ") + full_path + ": " +
            godot::String::num_int64(static_cast<std::int64_t>(godot::FileAccess::get_open_error()));
        return false;
    }
    std::int64_t length = file->get_length();
    godot::PackedByteArray bytes = file->get_buffer(length);
    file->close();

    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(bytes.size()));
    if (!buffer.empty()) std::memcpy(buffer.data(), bytes.ptr(), buffer.size());

    auto archive = Hp2TpkArchive::load(std::move(buffer));
    if (!archive) {
        last_error_ = full_path + " does not contain an HP2 TPK texture section.";
        return false;
    }
    out_archive = std::move(*archive);
    last_error_ = "";
    return true;
}

godot::Ref<godot::ImageTexture> Ps2TextureLoader::load_texture(const godot::String& relative_path, const godot::String& texture_name) {
    Hp2TpkArchive archive;
    if (!load_archive(relative_path, archive)) return {};

    std::string name(texture_name.utf8().get_data());
    auto decoded = archive.decode_by_name(name, /*flip_vertical=*/true);
    if (!decoded) {
        last_error_ = godot::String("Texture ") + texture_name + " not found or failed to decode in " + relative_path;
        return {};
    }

    godot::PackedByteArray image_bytes;
    image_bytes.resize(static_cast<std::int64_t>(decoded->rgba.size()));
    std::memcpy(image_bytes.ptrw(), decoded->rgba.data(), decoded->rgba.size());

    godot::Ref<godot::Image> image = godot::Image::create_from_data(decoded->width, decoded->height, false,
                                                                      godot::Image::FORMAT_RGBA8, image_bytes);
    if (image.is_null()) {
        last_error_ = "Godot Image::create_from_data failed";
        return {};
    }
    return godot::ImageTexture::create_from_image(image);
}

godot::Array Ps2TextureLoader::list_textures(const godot::String& relative_path) {
    godot::Array names;
    Hp2TpkArchive archive;
    if (!load_archive(relative_path, archive)) return names;
    for (const auto& header : archive.headers()) {
        if (!header.name.empty()) names.append(godot::String::utf8(header.name.c_str()));
    }
    return names;
}

} // namespace port::godot_bridge

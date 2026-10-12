#include "ps2_frontend_layout.hpp"
#include "platform/ps2_asset_root.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <cstring>

namespace port::godot_bridge {
namespace {

std::string search_start_dir() {
    godot::String project_path = godot::ProjectSettings::get_singleton()->globalize_path("res://");
    if (!project_path.is_empty()) return std::string(project_path.utf8().get_data());
    return std::string(godot::OS::get_singleton()->get_executable_path().get_base_dir().utf8().get_data());
}

} // namespace

void Ps2FrontEndLayout::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("load", "relative_path"), &Ps2FrontEndLayout::load);
    godot::ClassDB::bind_method(godot::D_METHOD("get_screen_count"), &Ps2FrontEndLayout::get_screen_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_screen_info", "index"), &Ps2FrontEndLayout::get_screen_info);
    godot::ClassDB::bind_method(godot::D_METHOD("get_widgets", "index"), &Ps2FrontEndLayout::get_widgets);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_error"), &Ps2FrontEndLayout::get_last_error);
    godot::ClassDB::bind_method(godot::D_METHOD("load_fonts", "bundles"), &Ps2FrontEndLayout::load_fonts);
    godot::ClassDB::bind_method(godot::D_METHOD("get_font", "texture_hash"), &Ps2FrontEndLayout::get_font);
}

godot::Dictionary Ps2FrontEndLayout::load(const godot::String& relative_path) {
    godot::Dictionary result;
    screens_.clear();

    auto root = hp2::platform::resolve_zzdata_root(search_start_dir());
    if (!root) {
        last_error_ = "PS2 asset root not configured (set HP2_PS2_ASSET_ROOT or create config/ps2_assets.local.json).";
        result["ok"] = false; result["screen_count"] = 0; result["error"] = last_error_;
        return result;
    }
    godot::String full_path = godot::String::utf8(root->c_str()).path_join(relative_path);
    godot::Ref<godot::FileAccess> file = godot::FileAccess::open(full_path, godot::FileAccess::READ);
    if (file.is_null()) {
        last_error_ = godot::String("Could not open ") + full_path;
        result["ok"] = false; result["screen_count"] = 0; result["error"] = last_error_;
        return result;
    }
    godot::PackedByteArray bytes = file->get_buffer(file->get_length());
    file->close();

    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(bytes.size()));
    if (!buffer.empty()) std::memcpy(buffer.data(), bytes.ptr(), buffer.size());

    screens_ = hp2::frontend::parse_screens(buffer);
    if (screens_.empty()) {
        last_error_ = full_path + " contains no frontend screen list chunk.";
        result["ok"] = false; result["screen_count"] = 0; result["error"] = last_error_;
        return result;
    }
    last_error_ = "";
    result["ok"] = true;
    result["screen_count"] = get_screen_count();
    result["error"] = "";
    return result;
}

godot::Dictionary Ps2FrontEndLayout::get_screen_info(int index) const {
    godot::Dictionary info;
    if (index < 0 || index >= get_screen_count()) return info;
    const auto& screen = screens_[static_cast<std::size_t>(index)];
    info["index"] = screen.index;
    info["width"] = screen.width;
    info["height"] = screen.height;
    info["widget_count"] = static_cast<int>(screen.widgets.size());
    godot::PackedStringArray names;
    for (const auto& w : screen.widgets) {
        if (!w.name.empty()) names.push_back(godot::String::utf8(w.name.c_str()));
    }
    info["names"] = names;
    return info;
}

godot::Array Ps2FrontEndLayout::get_widgets(int index) const {
    godot::Array widgets;
    if (index < 0 || index >= get_screen_count()) return widgets;
    for (const auto& w : screens_[static_cast<std::size_t>(index)].widgets) {
        godot::Dictionary d;
        d["chunk_id"] = static_cast<std::int64_t>(w.chunk_id);
        d["is_text"] = w.chunk_id == hp2::frontend::kTextWidgetId;
        d["name"] = godot::String::utf8(w.name.c_str());
        d["name_hash"] = static_cast<std::int64_t>(w.name_hash);
        d["class_hash"] = static_cast<std::int64_t>(w.class_hash);
        d["type"] = w.type;
        d["visible"] = w.visible;
        d["layer"] = w.layer;
        d["x"] = w.x; d["y"] = w.y;
        d["scale_x"] = w.scale_x; d["scale_y"] = w.scale_y;
        d["texture_hash"] = static_cast<std::int64_t>(w.texture_hash);
        // PS2 colour bytes: 0x80 is the neutral/opaque value, so scale by 128 and clamp.
        d["color_r"] = static_cast<double>(w.r) / 128.0;
        d["color_g"] = static_cast<double>(w.g) / 128.0;
        d["color_b"] = static_cast<double>(w.b) / 128.0;
        d["color_a"] = static_cast<double>(w.a) / 128.0;
        d["text"] = godot::String::utf8(w.text.c_str());
        d["text_align"] = static_cast<std::int64_t>(w.text_align);
        d["text_capacity"] = static_cast<std::int64_t>(w.text_capacity);
        widgets.append(d);
    }
    return widgets;
}

int Ps2FrontEndLayout::load_fonts(const godot::PackedStringArray& bundles) {
    fonts_.clear();
    auto root = hp2::platform::resolve_zzdata_root(search_start_dir());
    if (!root) {
        last_error_ = "PS2 asset root not configured.";
        return 0;
    }
    for (int i = 0; i < bundles.size(); ++i) {
        godot::String full_path = godot::String::utf8(root->c_str()).path_join(bundles[i]);
        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(full_path, godot::FileAccess::READ);
        if (file.is_null()) continue;
        godot::PackedByteArray bytes = file->get_buffer(file->get_length());
        file->close();
        std::vector<std::uint8_t> buffer(static_cast<std::size_t>(bytes.size()));
        if (!buffer.empty()) std::memcpy(buffer.data(), bytes.ptr(), buffer.size());
        for (auto& font : hp2::frontend::parse_fonts(buffer)) {
            bool known = false;
            for (const auto& existing : fonts_) {
                if (existing.texture_hash == font.texture_hash) { known = true; break; }
            }
            if (!known) fonts_.push_back(std::move(font));
        }
    }
    return static_cast<int>(fonts_.size());
}

godot::Dictionary Ps2FrontEndLayout::get_font(std::int64_t texture_hash) const {
    godot::Dictionary result;
    const std::uint32_t hash = static_cast<std::uint32_t>(texture_hash);
    for (const auto& font : fonts_) {
        if (font.texture_hash != hash) continue;
        godot::Dictionary glyphs;
        for (const auto& g : font.glyphs) {
            godot::PackedFloat32Array metrics;
            metrics.push_back(g.u);
            metrics.push_back(g.v);
            metrics.push_back(g.w);
            metrics.push_back(g.h);
            metrics.push_back(g.advance);
            glyphs[static_cast<std::int64_t>(g.code)] = metrics;
        }
        result["line_height"] = static_cast<std::int64_t>(font.line_height);
        result["glyphs"] = glyphs;
        return result;
    }
    return result;
}

} // namespace port::godot_bridge

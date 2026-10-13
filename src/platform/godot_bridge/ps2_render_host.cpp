#include "ps2_render_host.hpp"
#include "boundary.hpp"
#include "game_core/port/not_implemented.hpp"
#include "port/runtime/frame_builder.hpp"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <cstring>

namespace port::godot_bridge {
namespace {

godot::Rect2 to_rect(const port::render::Rect& rect) {
    return godot::Rect2(rect.x, rect.y, rect.width, rect.height);
}

godot::Color to_color(const port::render::Color& color) {
    return godot::Color(color.r, color.g, color.b, color.a);
}

godot::Ref<godot::Image> to_image(const port::render::Bitmap& bitmap) {
    godot::PackedByteArray pixels;
    pixels.resize(static_cast<int64_t>(bitmap.rgba.size()));
    if (!bitmap.rgba.empty())
        std::memcpy(pixels.ptrw(), bitmap.rgba.data(), bitmap.rgba.size());
    return godot::Image::create_from_data(bitmap.width, bitmap.height, false,
                                          godot::Image::FORMAT_RGBA8, pixels);
}

} // namespace

std::shared_ptr<const port::render::Bitmap> LiveResources::texture(port::render::TextureId id) {
    if (id == port::runtime::kIntroMovieTexture) return intro_;
    if (id == port::runtime::kBackgroundMovieTexture) return background_;
    return catalog_ == nullptr ? nullptr : catalog_->texture(id);
}

void Ps2RenderHost::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("attach", "parent_canvas_item"),
                                &Ps2RenderHost::attach);
    godot::ClassDB::bind_method(godot::D_METHOD("set_output_scale", "scale"),
                                &Ps2RenderHost::set_output_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_output_scale"),
                                &Ps2RenderHost::get_output_scale);
    godot::ClassDB::bind_method(
        godot::D_METHOD("draw", "runtime", "intro_movie", "background_movie"),
        &Ps2RenderHost::draw);
    godot::ClassDB::bind_method(godot::D_METHOD("release"), &Ps2RenderHost::release);
    godot::ClassDB::bind_method(godot::D_METHOD("get_texture_count"),
                                &Ps2RenderHost::get_texture_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_error"), &Ps2RenderHost::get_last_error);
}

Ps2RenderHost::~Ps2RenderHost() { release(); }

void Ps2RenderHost::attach(const godot::RID& parent_canvas_item) {
    auto* server = godot::RenderingServer::get_singleton();
    if (server == nullptr) {
        last_error_ = "RenderingServer unavailable";
        return;
    }
    if (!item_.is_valid()) item_ = server->canvas_item_create();
    server->canvas_item_set_parent(item_, parent_canvas_item);
    last_error_ = "";
}

void Ps2RenderHost::set_output_scale(double scale) {
    output_scale_ = scale > 0 ? scale : 1.0;
}

void Ps2RenderHost::release() {
    auto* server = godot::RenderingServer::get_singleton();
    if (server == nullptr) { textures_.clear(); item_ = godot::RID(); return; }
    for (auto& entry : textures_)
        if (entry.second.rid.is_valid()) server->free_rid(entry.second.rid);
    textures_.clear();
    if (item_.is_valid()) server->free_rid(item_);
    item_ = godot::RID();
}

godot::RID Ps2RenderHost::texture_for(port::render::TextureId id,
                                      port::render::Resources& resources) {
    const auto bitmap = resources.texture(id);
    if (!bitmap || bitmap->width <= 0 || bitmap->height <= 0) return godot::RID();
    auto* server = godot::RenderingServer::get_singleton();
    auto& entry = textures_[id];
    if (entry.rid.is_valid() && entry.width == bitmap->width && entry.height == bitmap->height) {
        // Original entries never change; the live movie surfaces bump their revision per frame.
        if (entry.revision != bitmap->revision) {
            server->texture_2d_update(entry.rid, to_image(*bitmap), 0);
            entry.revision = bitmap->revision;
        }
        return entry.rid;
    }
    if (entry.rid.is_valid()) server->free_rid(entry.rid);
    entry.rid = server->texture_2d_create(to_image(*bitmap));
    entry.width = bitmap->width;
    entry.height = bitmap->height;
    entry.revision = bitmap->revision;
    return entry.rid;
}

int Ps2RenderHost::submit(const port::render::Frame& frame, port::render::Resources& resources) {
    int movie_quads = 0;
    auto* server = godot::RenderingServer::get_singleton();
    server->canvas_item_clear(item_);
    server->canvas_item_set_transform(
        item_, godot::Transform2D().scaled(godot::Vector2(static_cast<real_t>(output_scale_),
                                                          static_cast<real_t>(output_scale_))));
    if (frame.clear.a > 0) {
        server->canvas_item_add_rect(
            item_, godot::Rect2(0, 0, frame.logical_size.x, frame.logical_size.y),
            to_color(frame.clear));
    }
    for (const auto& quad : frame.quads) {
        if (quad.texture == 0) { // solid-colour rect; see render::Quad
            server->canvas_item_add_rect(item_, to_rect(quad.destination), to_color(quad.color));
            continue;
        }
        if (quad.texture == port::runtime::kIntroMovieTexture ||
            quad.texture == port::runtime::kBackgroundMovieTexture) ++movie_quads;
        const auto rid = texture_for(quad.texture, resources);
        if (!rid.is_valid()) continue;
        // clip_uv true: sampling stays inside the frame's source rectangle, so a glyph never
        // bleeds in its neighbour from the shared font atlas and a cropped quad keeps the exact
        // partial region port::render::crop produced for the original clip.
        server->canvas_item_add_texture_rect_region(item_, to_rect(quad.destination), rid,
                                                    to_rect(quad.source), to_color(quad.color),
                                                    false, true);
    }
    return movie_quads;
}

godot::Dictionary Ps2RenderHost::draw(godot::Object* runtime, godot::Object* intro_movie,
                                      godot::Object* background_movie) {
    godot::Dictionary result;
    result["ok"] = false;
    auto* layout = godot::Object::cast_to<Ps2FrontEndLayout>(runtime);
    if (layout == nullptr) {
        last_error_ = "draw() requires a Ps2FrontEndLayout";
        result["error"] = last_error_;
        return result;
    }
    if (!item_.is_valid()) {
        last_error_ = "attach() has not been called";
        result["error"] = last_error_;
        return result;
    }
    auto* intro = godot::Object::cast_to<Ps2MoviePlayer>(intro_movie);
    auto* background = godot::Object::cast_to<Ps2MoviePlayer>(background_movie);
    LiveResources resources(layout->runtime().catalog(),
                            intro == nullptr ? nullptr : intro->frame_bitmap(),
                            background == nullptr ? nullptr : background->frame_bitmap());

    port::render::Frame frame;
    port::runtime::FrameStats stats;
    // Two different outcomes: a recovered path we have not translated (expected, reported as
    // not_implemented) and a port bug (unexpected, reported as an error). Neither may cross into
    // Godot, which is compiled -fno-exceptions.
    std::string gap;
    const auto failure =
        catch_all([&] { gap = hp2::guard([&] { stats = layout->runtime().build_frame(resources, frame); }); });
    if (!failure.empty()) {
        last_error_ = godot::String::utf8(("port error building the frame: " + failure).c_str());
        result["error"] = last_error_;
        return result;
    }
    if (!gap.empty()) {
        last_error_ = godot::String::utf8(gap.c_str());
        result["error"] = last_error_;
        result["not_implemented"] = last_error_;
        return result;
    }
    const int movie_quads = submit(frame, resources);

    last_error_ = "";
    result["ok"] = true;
    result["quads"] = stats.quads;
    result["glyphs"] = stats.glyphs;
    result["texts"] = stats.texts;
    result["missing"] = stats.missing;
    result["clipped_out"] = stats.clipped_out;
    result["sequence"] = static_cast<std::int64_t>(frame.sequence);
    result["textures"] = get_texture_count();
    result["movie_quads"] = movie_quads;
    result["error"] = "";
    return result;
}

} // namespace port::godot_bridge

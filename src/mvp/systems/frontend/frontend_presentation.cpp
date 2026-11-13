#include "frontend_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "frontend_video.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/Texture.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/interval_tweener.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/video_stream_player.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

namespace hp2 {

using namespace godot;

namespace {
constexpr std::uint32_t panelMovie = 0xc3f0caefu;
constexpr std::uint32_t titleMovie = 0xf9117c29u;

Color color(std::uint32_t rgba) {
    const float divisor = 128.0f;
    const auto lane = [&](int shift) { return std::min(1.0f, ((rgba >> shift) & 255u) / divisor); };
    return {lane(0), lane(8), lane(16), lane(24)};
}

Ref<Image> rgba_image(const std::uint8_t* source, int width, int height) {
    if (!source || width < 1 || height < 1 || width > 8192 || height > 8192)
        throw std::runtime_error("Invalid frontend image extent");
    PackedByteArray pixels;
    pixels.resize(std::int64_t(width) * height * 4);
    std::memcpy(pixels.ptrw(), source, pixels.size());
    auto* bytes = pixels.ptrw();
    for (std::int64_t i = 3; i < pixels.size(); i += 4)
        bytes[i] = std::min(255, bytes[i] * 2);
    auto image = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, pixels);
    if (image.is_null()) throw std::runtime_error("Godot rejected frontend image");
    return image;
}
}

struct FrontendPresentation::Storage {
    Storage(Node2D& node, const GameResources& files)
        : canvas(node), background(*memnew(Node2D)), overlay(*memnew(Node2D)),
          player(*memnew(VideoStreamPlayer)), video(player, files) {
        canvas.add_child(&background);
        canvas.add_child(&overlay);
        overlay.set_z_index(20);
        canvas.add_child(&player);
        player.set_expand(true);
        player.set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        player.hide();
    }
    Node2D& canvas;
    Node2D& background;
    Node2D& overlay;
    struct Texture {
        std::weak_ptr<const TextureMetadataStorage> metadata;
        std::weak_ptr<TexturePixelStorage> pixels;
        std::weak_ptr<TexturePixelStorage> palette;
        const std::uint8_t* pixelAddress{};
        const std::uint8_t* paletteAddress{};
        bool linearPalette{};
        std::uint64_t frame{};
        Ref<ImageTexture> image;
    };
    std::unordered_map<const EditorMarkerTexture*, Texture> textures;
    std::uint64_t frame{};
    VideoStreamPlayer& player;
    FrontendVideo video;
    Ref<Tween> transition;
    std::string screen;
    struct Highlight {
        Node2D* node{};
        Ref<Tween> tween;
        std::uint32_t target{}, revision{};
        std::uint64_t frame{};
    };
    std::map<std::pair<std::uint32_t, std::uint32_t>, Highlight> highlights;
    struct Preview {
        SubViewportContainer* container{};
        SubViewport* viewport{};
        Camera3D* camera{};
        std::vector<MeshInstance3D*> nodes;
        std::vector<std::tuple<std::uint32_t, std::uintptr_t, std::uintptr_t, std::uint64_t>> models;
    };
    std::map<int, Preview> previews;
    void update_previews(const FrontendStateView& state) {
        if (!state.previews || !state.cameras) throw std::runtime_error("Frontend preview owners are unavailable");
        std::map<int, std::vector<const FrontendModelRequest*>> views;
        for (const auto& request : state.previews->models) {
            if (request.viewNumber < 1 || request.viewNumber > 2)
                throw std::runtime_error("Unsupported frontend preview view " + std::to_string(request.viewNumber));
            if (request.model && request.model->solid) views[request.viewNumber].push_back(&request);
        }
        for (auto& [view, preview] : previews) preview.container->set_visible(views.count(view) != 0);
        for (const auto& [view, models] : views) {
            auto& preview = previews[view];
            if (!preview.viewport) {
                preview.container = memnew(SubViewportContainer);
                canvas.add_child(preview.container);
                preview.container->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
                preview.container->set_z_index(10);
                preview.container->set_size({640,480});
                preview.viewport = memnew(SubViewport);
                preview.container->add_child(preview.viewport);
                preview.viewport->set_size({640,480});
                preview.viewport->set_use_own_world_3d(true);
                preview.viewport->set_transparent_background(true);
                preview.camera = memnew(Camera3D);
                preview.viewport->add_child(preview.camera);
                preview.camera->make_current();
            }
            const auto* camera = state.cameras->screens[view].view;
            if (!camera) throw std::runtime_error("Frontend preview has no source camera");
            const auto& pose = camera->current;
            const auto eye = race_vector(pose.eye);
            const auto forward = race_vector(pose.forward);
            const float up[] = {-pose.matrix[1],-pose.matrix[5],-pose.matrix[9]};
            if (forward.cross(race_vector(up)).length_squared() < 0.01f)
                throw std::runtime_error("Frontend preview camera basis is invalid");
            preview.camera->set_perspective(pose.fieldOfView * (360.0 / 65536.0), pose.nearClip, pose.farClip);
            preview.camera->look_at_from_position(eye, eye + forward, race_vector(up));

            MeshConverter converter;
            for (std::size_t i = 0; i < models.size(); ++i) {
                const auto& request = *models[i];
                const auto& model = *request.model;
                const auto identity = std::make_tuple(model.key, reinterpret_cast<std::uintptr_t>(model.solid),
                    reinterpret_cast<std::uintptr_t>(model.replacement), model.resources ? model.resources->bindingRevision : 0);
                if (i == preview.nodes.size()) {
                    auto* node = memnew(MeshInstance3D);
                    preview.viewport->add_child(node);
                    preview.nodes.push_back(node);
                    preview.models.emplace_back();
                }
                auto* node = preview.nodes[i];
                if (preview.models[i] != identity) {
                    node->set_mesh(converter.convert(model));
                    preview.models[i] = identity;
                }
                node->set_transform(race_transform(request.transform.data()));
                node->show();
            }
            for (std::size_t i = models.size(); i < preview.nodes.size(); ++i) preview.nodes[i]->hide();
        }
    }

    Ref<ImageTexture> texture_for(const TextureInfo& source) {
        auto& cached = textures[source.descriptor];
        if (cached.image.is_valid() && cached.frame == frame) return cached.image;
        const auto owner = textureMetadataOwner(source.descriptor);
        const auto* pixels = source.pixels.Data();
        const auto* palette = source.palette.Data();
        if (cached.image.is_valid() && owner && owner == cached.metadata.lock() &&
            cached.pixels.lock() == source.pixels.owner && cached.palette.lock() == source.palette.owner &&
            cached.pixelAddress == pixels && cached.paletteAddress == palette &&
            cached.linearPalette == source.palette.nativeLinearPalette &&
            source.pixels.owner && source.pixels.owner->PublishedReadOnly() &&
            (!palette || (source.palette.owner && source.palette.owner->PublishedReadOnly()))) {
            cached.frame = frame;
            return cached.image;
        }
        std::uint32_t width{}, height{};
        const auto bytes = DecodeTextureMipRgba(source, 0, 0, source.pixels.size, width, height);
        cached = {owner, source.pixels.owner, source.palette.owner, pixels, palette,
            source.palette.nativeLinearPalette, frame,
            ImageTexture::create_from_image(rgba_image(bytes.data(), width, height))};
        if (cached.image.is_null()) throw std::runtime_error("Frontend texture upload failed");
        return cached.image;
    }

    void draw(Node2D& target, const FlailerStrip& strip, Vector2 origin = {}) {
        // PAL145950 traverses secondary atlases before the base texture.
        if (strip.nextTexture) draw(target, *strip.nextTexture, origin);
        if (strip.polygons.empty()) return;
        if (!strip.texture) throw std::runtime_error("Frontend layout is missing its texture descriptor");
        if (strip.texture->key == titleMovie || strip.texture->key == panelMovie) {
            if (!video.playing()) return;
            const auto& quad = strip.polygons.front().vertices;
            const Vector2 position(quad[0].position[0], quad[0].position[1]);
            player.set_position(position);
            player.set_size(Vector2(quad[2].position[0], quad[2].position[1]) - position);
            player.set_z_index(1);
            player.show();
            return;
        }
        const auto visible = [](const FlailerPoly& polygon) {
            return std::any_of(polygon.vertices.begin(), polygon.vertices.end(),
                [](const auto& vertex) { return (vertex.color >> 24) != 0; });
        };
        if (std::none_of(strip.polygons.begin(), strip.polygons.end(), visible)) return;
        const auto* source = textureInfoForDescriptor(strip.texture);
        if (!source || !source->pixels.Data() || (source->paletteBytes && !source->palette.Data())) {
            if (source && source->pack && source->pack->HasPendingReads()) return;
            throw std::runtime_error("Visible frontend texture has no decoded source, key=" +
                std::to_string(strip.texture->key));
        }
        const auto texture = texture_for(*source);
        PackedVector2Array points, uv;
        PackedColorArray colors;
        PackedInt32Array indices;
        for (const auto& polygon : strip.polygons) {
            if (!visible(polygon)) continue;
            const auto base = static_cast<int32_t>(points.size());
            const auto& bounds = polygon.glyphSamplingBounds;
            for (const auto& vertex : polygon.vertices) {
                points.push_back(Vector2(vertex.position[0], vertex.position[1]) - origin);
                const auto& uvq = vertex.textureCoordinate;
                if (uvq[2] == 0.0f) throw std::runtime_error("Frontend polygon has a zero texture projection");
                float u = uvq[0] / uvq[2], v = uvq[1] / uvq[2];
                if (bounds != std::array<float,4>{}) {
                    u = std::clamp(u, bounds[0], std::max(bounds[0], bounds[2]));
                    v = std::clamp(v, bounds[1], std::max(bounds[1], bounds[3]));
                }
                uv.push_back({u, v});
                colors.push_back(color(vertex.color));
            }
            for (int32_t offset : {0, 1, 2, 0, 2, 3}) indices.push_back(base + offset);
        }
        RenderingServer::get_singleton()->canvas_item_add_triangle_array(target.get_canvas_item(),
            indices, points, colors, uv, {}, {}, texture->get_rid());
    }

    void draw_layers(const FlailerRenderManager& layout, Node2D& target, int first, int last) {
        for (int layer = first; layer >= last; --layer)
            for (const auto* strip : layout.layers[layer]) draw(target, *strip);
    }

    void update_highlights(const FlailerRenderManager& layout) {
        auto* rendering = RenderingServer::get_singleton();
        for (const auto& source : layout.highlights) {
            const auto* first = source.layout;
            while (first && first->polygons.empty()) first = first->nextTexture.get();
            if (!first) continue;
            const auto& point = first->polygons.front().vertices[0].position;
            Vector2 minimum(point[0], point[1]);
            Vector2 maximum = minimum;
            for (auto* batch = first; batch; batch = batch->nextTexture.get())
                for (const auto& polygon : batch->polygons)
                    for (const auto& vertex : polygon.vertices) {
                        const Vector2 position(vertex.position[0], vertex.position[1]);
                        minimum = minimum.min(position);
                        maximum = maximum.max(position);
                    }
            const auto center = (minimum + maximum) * 0.5f;
            auto& highlight = highlights[{source.screen, source.echo}];
            if (!highlight.node) {
                highlight.node = memnew(Node2D);
                overlay.add_child(highlight.node);
            }
            highlight.frame = frame;
            auto& node = *highlight.node;
            node.set_position(center);
            rendering->canvas_item_clear(node.get_canvas_item());
            draw(node, *source.layout, center);
            if (highlight.tween.is_valid() && highlight.target == source.target &&
                highlight.revision == source.revision) continue;
            if (highlight.tween.is_valid()) highlight.tween->kill();
            highlight.target = source.target;
            highlight.revision = source.revision;
            node.set_scale({1, 1});
            node.set_self_modulate(Color(1, 1, 1, 1));
            auto tween = node.create_tween();
            tween->set_loops();
            tween->set_trans(Tween::TRANS_SINE);
            tween->set_ease(Tween::EASE_OUT);
            tween->tween_property(&node, "scale", Vector2(source.scaleRatio, source.scaleRatio),
                source.duration)->from(Vector2(1, 1));
            tween->parallel()->tween_property(&node, "self_modulate:a", source.endAlpha, source.duration)->from(1.0);
            if (source.period > source.duration)
                tween->tween_interval(source.period - source.duration);
            highlight.tween = tween;
        }
        for (auto it = highlights.begin(); it != highlights.end();) {
            if (it->second.frame == frame) {
                ++it;
                continue;
            }
            if (it->second.tween.is_valid()) it->second.tween->kill();
            overlay.remove_child(it->second.node);
            it->second.node->queue_free();
            it = highlights.erase(it);
        }
    }
};

FrontendPresentation::FrontendPresentation(Node2D& canvas, const GameResources& files)
    : storage_(std::make_unique<Storage>(canvas, files)) {}
FrontendPresentation::~FrontendPresentation() = default;
bool FrontendPresentation::video_playing() const noexcept {return storage_->video.playing();}
std::uint64_t FrontendPresentation::video_request() const noexcept {return storage_->video.request_id();}

void FrontendPresentation::update(const FrontendStateView& state, Vector2 viewport) {
    auto& s = *storage_;
    if (!state.layout || !state.movie)
        throw std::runtime_error("Frontend state is missing an owner");
    s.video.update(*state.movie);
    s.update_previews(state);
    s.player.hide();
    const float scale = std::min(viewport.x / 640.0f, viewport.y / 480.0f);
    s.canvas.set_scale({scale, scale});
    s.canvas.set_position((viewport - Vector2(640, 480) * scale) * 0.5f);
    const std::string screen = state.screen ? state.screen : "";
    if (s.screen != screen) {
        s.screen = screen;
        if (s.transition.is_valid()) s.transition->kill();
        s.canvas.set_modulate(Color(1, 1, 1, 1));
        if (!screen.empty()) {
            s.canvas.set_modulate(Color(1, 1, 1, 0));
            s.transition = s.canvas.create_tween();
            s.transition->tween_property(&s.canvas, "modulate:a", 1.0, 0.16);
            std::fprintf(stderr, "HP2_MVP_TRACE stage=frontend_screen screen=\"%s\"\n", state.screen);
        }
    }

    auto* rendering = RenderingServer::get_singleton();
    rendering->canvas_item_clear(s.background.get_canvas_item());
    rendering->canvas_item_clear(s.overlay.get_canvas_item());
    ++s.frame;
    s.draw_layers(*state.layout, s.background, 24, 20);
    s.draw_layers(*state.layout, s.overlay, 15, 0);
    s.draw_layers(*state.layout, s.overlay, 19, 16);
    s.draw_layers(*state.layout, s.overlay, 28, 25);
    s.update_highlights(*state.layout);
    if (screen == "Title Screen" && s.video.playing()) {
        s.player.set_position({});
        s.player.set_size({640, 480});
        s.player.set_z_index(31);
        s.player.show();
    }

    for (auto it = s.textures.begin(); it != s.textures.end();) {
        if (it->second.frame != s.frame)
            it = s.textures.erase(it);
        else ++it;
    }
}

}

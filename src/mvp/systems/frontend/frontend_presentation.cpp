#include "frontend_presentation.hpp"

#include "indep/inc/Flags.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/Language.hpp"
#include "indep/inc/Texture.hpp"
#include "frontend_video.hpp"
#include "assets/mesh_converter.hpp"
#include "indep/inc/Fred.hpp"
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/video_stream_player.hpp>

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/polygon2d.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

namespace hp2 {

using namespace godot;

namespace {
constexpr std::uint32_t titleMovie = 0xc3f0caefu;
constexpr std::uint32_t panelMovie = 0xf9117c29u;

Color color(std::uint32_t argb) {
    const float divisor = 128.0f;
    const auto lane = [&](int shift) { return std::min(1.0f, ((argb >> shift) & 255u) / divisor); };
    return {lane(16), lane(8), lane(0), lane(24)};
}

Ref<Image> rgba_image(const std::uint8_t* source, int width, int height, bool opaque) {
    if (!source || width < 1 || height < 1 || width > 8192 || height > 8192)
        throw std::runtime_error("Invalid frontend image extent");
    PackedByteArray pixels;
    pixels.resize(std::int64_t(width) * height * 4);
    std::memcpy(pixels.ptrw(), source, pixels.size());
    auto* bytes = pixels.ptrw();
    for (std::int64_t i = 3; i < pixels.size(); i += 4)
        bytes[i] = opaque ? 255 : std::min(255, bytes[i] * 2);
    auto image = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, pixels);
    if (image.is_null()) throw std::runtime_error("Godot rejected frontend image");
    return image;
}
}

struct FrontendPresentation::Storage {
    Storage(Node2D& node, const GameResources& files, const String& ffmpeg)
        : canvas(node), player(*memnew(VideoStreamPlayer)), video(player, files, ffmpeg) {
        canvas.add_child(&player);
        player.set_expand(true);
        player.set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        player.hide();
    }
    Node2D& canvas;
    struct Object {
        CanvasItem* node{};
        int kind{};
    };
    struct Texture {
        std::weak_ptr<const TextureMetadataStorage> metadata;
        Ref<ImageTexture> image;
    };
    using Key = std::tuple<std::uint32_t, int, std::size_t>;
    std::map<Key, Object> objects;
    std::unordered_map<std::uint32_t, Texture> textures;
    VideoStreamPlayer& player;
    FrontendVideo video;
    Ref<Tween> transition;
    std::string screen;
    std::uint32_t selected{};
    Ref<Tween> selection;
    struct Preview {
        SubViewportContainer* container{};
        SubViewport* viewport{};
        Camera3D* camera{};
        std::vector<MeshInstance3D*> nodes;
        std::vector<std::tuple<std::uint32_t, std::uintptr_t, std::uintptr_t, std::uint64_t>> models;
    };
    std::map<int, Preview> previews;
    PanelContainer* eventPanel{};
    Label* eventList{};
    int shownEvent{};
    bool showingDetails{};

    void update_events(const FrontendFrame& frame) {
        if (frame.events.empty()) {
            if (eventPanel) eventPanel->hide();
            shownEvent=0;
            return;
        }
        if (!eventPanel) {
            eventPanel=memnew(PanelContainer);
            canvas.add_child(eventPanel);
            eventPanel->set_position({116,82});
            eventPanel->set_size({465,305});
            eventPanel->set_z_index(29);
            eventPanel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
            eventList=memnew(Label);
            eventPanel->add_child(eventList);
            eventList->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
            eventList->add_theme_font_size_override("font_size",16);
            eventList->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        }
        eventPanel->show();
        if (shownEvent==frame.selectedEvent && showingDetails==frame.eventDetails) return;
        shownEvent=frame.selectedEvent;
        showingDetails=frame.eventDetails;
        String text;
        if (frame.eventDetails) {
            text = String(frame.eventDescription.c_str());
        } else {
            const auto selected=std::find_if(frame.events.begin(),frame.events.end(),
                [&](const FrontendEventEntry& event) {return event.number==frame.selectedEvent;});
            const auto index=std::distance(frame.events.begin(),selected);
            const auto first=std::max<std::ptrdiff_t>(0,index-3);
            const auto last=std::min<std::ptrdiff_t>(frame.events.size(),first+8);
            for(auto i=first;i<last;++i) {
                const auto& event=frame.events[i];
                text += String(event.number==shownEvent ? "> " : "   ") + String::num_int64(event.number) +
                    "  " + event.title.c_str() + (event.status>0 ? "\n" : "  [Locked]\n");
            }
        }
        eventList->set_text(text);
        eventPanel->set_modulate(Color(1,1,1,0.6));
        eventPanel->create_tween()->tween_property(eventPanel,"modulate:a",1.0,0.18);
    }

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

    Ref<ImageTexture> get_bitmap(const EditorMarkerTexture& descriptor) {
        const auto owner = textureMetadataOwner(&descriptor);
        const auto* info = textureInfoForDescriptor(&descriptor);
        if (!info || !info->pixels.Data())
            throw std::runtime_error("Visible frontend texture has no pixels, key=" + std::to_string(descriptor.key));
        auto& cached = textures[descriptor.key];
        if (cached.image.is_valid() && owner && owner == cached.metadata.lock()) return cached.image;
        std::uint32_t width{}, height{};
        const auto bytes = DecodeTextureMipRgba(*info, 0, 0, info->pixels.size, width, height);
        cached = {owner, ImageTexture::create_from_image(rgba_image(bytes.data(), width, height, false))};
        return cached.image;
    }

    void text(Label& label, const FlailerTextChunk& source, const FrontendStateView& state) {
        const auto* value = source.text;
        if (state.language->textEnabled && source.languageId)
            value = state.language->GetString(static_cast<std::int32_t>(source.languageId));
        label.set_text(value ? String(value) : String());
        label.set_position({source.topLeftX, source.topLeftY});
        const auto height = state.resources->ScaledTextHeight(source);
        label.add_theme_font_size_override("font_size", std::max(1, int(height)));
        auto foreground = color(source.color);
        if (foreground.a > 0.0f && foreground.get_luminance() < 0.28f)
            foreground = foreground.lerp(Color(0.90f,0.93f,1.0f,foreground.a),0.78f);
        label.add_theme_color_override("font_color", foreground);
        label.add_theme_color_override("font_outline_color", Color(0,0,0,foreground.a));
        label.add_theme_constant_override("outline_size", 2);
        label.set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        const auto width = label.get_minimum_size().x;
        const auto shift = source.formatOptions & 4u ? width * 0.5f : source.formatOptions & 1u ? width : 0.0f;
        label.set_position({source.topLeftX - shift, source.topLeftY});
    }

    void bitmap(Polygon2D& node, const FlailerBitmapChunk& source, const FrontendStateView& state) {
        const auto* descriptor = source.texture ? source.texture : state.resources->FindTexture(source.textureKey);
        if (!descriptor && !source.textureKey) {
            node.hide();
            return;
        }
        if (!descriptor && state.resources->Models().resourceRegistry)
            descriptor = state.resources->Models().resourceRegistry->ResolveTexture(source.textureKey,true);
        if (!descriptor)
            throw std::runtime_error("Visible frontend bitmap has no texture, object=" + std::to_string(source.key) +
                ", texture=" + std::to_string(source.textureKey));
        const bool isMovie = source.nativeFullscreenMovie || descriptor->key == titleMovie || descriptor->key == panelMovie;
        if (isMovie) {
            const auto* record = descriptor->authoredRecord;
            if (!record || descriptor->authoredRecordSize < 0x28)
                throw std::runtime_error("Invalid movie panel descriptor");
            const Vector2 extent(record[0x24] | record[0x25] << 8, record[0x26] | record[0x27] << 8);
            const bool fullscreen = source.nativeFullscreenMovie;
            player.set_position(fullscreen ? Vector2() : Vector2(source.topLeftX, source.topLeftY));
            player.set_size(fullscreen ? Vector2(640,480) : extent * Vector2(source.scaleX,source.scaleY));
            player.set_z_index(node.get_z_index());
            player.set_visible(video.playing());
            node.hide();
            return;
        }
        const auto texture = get_bitmap(*descriptor);
        if (texture.is_null()) throw std::runtime_error("Frontend bitmap texture upload failed");

        const auto* record = descriptor->authoredRecord;
        if (!record || descriptor->authoredRecordSize < 0x28)
            throw std::runtime_error("Frontend bitmap has an invalid descriptor");
        const Vector2 extent(record[0x24] | record[0x25] << 8, record[0x26] | record[0x27] << 8);
        const Vector2 size = extent * Vector2(source.scaleX, source.scaleY);
        node.set_position({source.topLeftX,source.topLeftY});
        node.set_rotation(0);

        const Vector2 corners[] = {{0,0}, {size.x,0}, size, {0,size.y}};

        PackedVector2Array points, uv;
        PackedColorArray colors;
        for (int i = 0; i < 4; ++i) {
            auto point = corners[i];
            if (source.authoredMetrics[0] == 2 && source.rotation)
                point = (point - size * 0.5f).rotated(source.rotation * 0.01745329252f) + size * 0.5f;
            points.push_back(point);
            uv.push_back(Vector2(source.textureCoordinates[i][0], source.textureCoordinates[i][1]) * extent);
            colors.push_back(color(source.colors[i]));
        }
        node.set_polygon(points);
        node.set_uv(uv);
        node.set_vertex_colors(colors);
        node.set_texture(texture);
        node.show();
    }
};

FrontendPresentation::FrontendPresentation(Node2D& canvas, const GameResources& files, const String& ffmpeg)
    : storage_(std::make_unique<Storage>(canvas, files, ffmpeg)) {}
FrontendPresentation::~FrontendPresentation() = default;
bool FrontendPresentation::video_playing() const noexcept {return storage_->video.playing();}
std::uint64_t FrontendPresentation::video_request() const noexcept {return storage_->video.request_id();}

void FrontendPresentation::update(const FrontendStateView& state, Vector2 viewport) {
    auto& s = *storage_;
    if (!state.resources || !state.visibility || !state.language || !state.movie)
        throw std::runtime_error("Frontend state is missing an owner");
    s.video.update(*state.movie);
    s.update_previews(state);
    s.update_events(*state.previews);
    s.player.hide();
    const float scale = std::min(viewport.x / 640.0f, viewport.y / 480.0f);
    s.canvas.set_scale({scale, scale});
    s.canvas.set_position((viewport - Vector2(640, 480) * scale) * 0.5f);
    if (state.screen && s.screen != state.screen) {
        s.screen = state.screen;
        if (s.transition.is_valid()) s.transition->kill();
        s.canvas.set_modulate(Color(1,1,1,0));
        s.transition = s.canvas.create_tween();
        s.transition->tween_property(&s.canvas, "modulate:a", 1.0, 0.16);
        std::fprintf(stderr, "HP2_MVP_TRACE stage=frontend_screen screen=\"%s\"\n", state.screen);
    }

    CanvasItem* selectedNode = nullptr;
    std::set<Storage::Key> active;
    const auto render = [&](const FlailerScreenResource& screen, int slot) {
        const auto& root = screen.objects;
        std::size_t ordinal = 0;
        for (auto* object = root.previous; object != &root; object = object->previous, ++ordinal) {
            if (object->visible != 1 || (object->kind != 1 && object->kind != 2)) continue;
            const Storage::Key key{screen.key, slot, ordinal};
            active.insert(key);
            auto& entry = s.objects[key];
            if (entry.node && entry.kind != object->kind) {
                s.canvas.remove_child(entry.node);
                entry.node->queue_free();
                entry.node = nullptr;
            }
            if (!entry.node) {
                entry.kind = object->kind;
                if (entry.kind == 1) entry.node = memnew(Label);
                else entry.node = memnew(Polygon2D);
                s.canvas.add_child(entry.node);
            }
            entry.node->set_z_index(28 - static_cast<std::int8_t>(object->authoredFlags));
            if (entry.kind == 1 && object->key == state.selectedItem) selectedNode = entry.node;
            if (entry.kind == 1) s.text(*static_cast<Label*>(entry.node), static_cast<const FlailerTextChunk&>(*object), state);
            else s.bitmap(*static_cast<Polygon2D*>(entry.node), static_cast<const FlailerBitmapChunk&>(*object), state);
        }
    };
    for (std::size_t i = 0; i < state.visibility->visibleScreens.size(); ++i) {
        if (const auto* screen = state.visibility->visibleScreens[i]) {
            if (auto* shared = state.resources->FindScreen(screen->sharedScreenKey)) render(*shared, i * 2);
            render(*screen, i * 2 + 1);
        }
    }
    if (state.screen && std::strcmp(state.screen, "Title Screen") == 0 && s.video.playing()) {
        s.player.set_position({});
        s.player.set_size({640,480});
        s.player.set_z_index(31);
        s.player.show();
    }
    if (s.selected != state.selectedItem) {
        s.selected = state.selectedItem;
        if (s.selection.is_valid()) s.selection->kill();
        for (auto& [key, entry] : s.objects) entry.node->set_modulate(Color(1,1,1,1));
        if (selectedNode) {
            selectedNode->set_modulate(Color(1,1,1,0.65));
            s.selection = selectedNode->create_tween();
            s.selection->tween_property(selectedNode, "modulate:a", 1.0, 0.20);
        }
    }
    for (auto it = s.objects.begin(); it != s.objects.end();) {
        if (active.count(it->first)) {
            ++it;
        } else {
            s.canvas.remove_child(it->second.node);
            it->second.node->queue_free();
            it = s.objects.erase(it);
        }
    }
    for (auto it = s.textures.begin(); it != s.textures.end();) {
        if (it->second.metadata.expired()) it = s.textures.erase(it);
        else ++it;
    }
}

}

#include "frontend_presentation.hpp"

#include "assets/shader_resource.hpp"

#include "assets/mesh_converter.hpp"
#include "bridge/entity_state_bridge.hpp"
#include "indep/inc/CameraMover.hpp"
#include "presentation/race_presentation.hpp"
#include "systems/effects/race_effects_presentation.hpp"
#include "frontend_video.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/Texture.hpp"
#include "bware/inc/bWare.hpp"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/interval_tweener.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/video_stream_player.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>
#include <cmath>
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
constexpr std::uint32_t titleScreen = 0xb5761d00u;
constexpr std::uint32_t gridObject = 0x00163ce5u;
constexpr std::uint32_t loadingScreen = 0xd0131dd1u;
bool is_player_letterbox(std::uint32_t object) {
    switch (object) {
    case 0x464BA8EAu: case 0xD911BEACu: case 0xF7F1CFB3u: case 0xF88DD0B1u:
        return true; // Player letterbox bitmaps
    default:
        return false;
    }
}
const Vector2 designSize(640, 480);

constexpr float videoAspect = 4.0f / 3.0f;

struct CanvasLayout {
    Vector2 visible{designSize};
    float scale{1};

    void resize(Vector2 viewport) {
        if (viewport.x <= 0 || viewport.y <= 0) return;
        scale = std::min(viewport.x / designSize.x, viewport.y / designSize.y);
        visible = viewport / scale;
    }

    Rect2 visible_rect() const {
        return {(designSize - visible) * 0.5f, visible};
    }

    static Rect2 fit_movie(Rect2 bounds) {
        const float width = std::min(bounds.size.x, bounds.size.y * videoAspect);
        const Vector2 size(width, width / videoAspect);
        return {bounds.position + (bounds.size - size) * 0.5f, size};
    }
};

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
    Storage(Node2D& node, const GameResources& files, ApplicationRuntime& runtime, RacePresentation& host)
        : canvas(node), background(*memnew(Node2D)), videoMatte(*memnew(Node2D)), overlay(*memnew(Node2D)),
          mapBackground(*memnew(Node2D)), mapTexture(*memnew(Node2D)),
          mapForeground(*memnew(Node2D)), player(*memnew(VideoStreamPlayer)), video(player, files),
          pointerRuntime(runtime), pointerSurface(*memnew(Control)) {
        canvas.add_child(&background);
        canvas.add_child(&videoMatte);
        videoMatte.set_z_index(1);
        canvas.add_child(&overlay);
        overlay.set_z_index(20);
        overlay.add_child(&mapBackground);
        overlay.add_child(&mapTexture);
        overlay.add_child(&mapForeground);
        Ref<Shader> shader;
        shader = load_shader("res://shader/minimap.gdshader");
        Ref<ShaderMaterial> material;
        material.instantiate();
        material->set_shader(shader);
        mapTexture.set_material(material);
        canvas.add_child(&player);
        player.set_expand(true);
        player.set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        player.hide();
        canvas.add_child(&pointerSurface);
        pointerSurface.set_name("FrontendPointer");
        pointerSurface.set_z_index(40);
        pointerSurface.set_mouse_filter(Control::MOUSE_FILTER_STOP);
        pointerSurface.set_default_cursor_shape(Control::CURSOR_ARROW);
        pointerSurface.connect("gui_input", callable_mp(&host, &RacePresentation::frontend_pointer_input));
        pointerSurface.hide();
    }
    Node2D& canvas;
    Node2D& background;
    Node2D& videoMatte;
    Node2D& overlay;
    Node2D& mapBackground;
    Node2D& mapTexture;
    Node2D& mapForeground;
    CanvasLayout layout;
    TextureRect* rearView{};
    Ref<ShaderMaterial> rearMaterial;
    std::array<Ref<ImageTexture>, 2> rearOverlays;
    struct MapMesh {
        Ref<ArrayMesh> mesh;
        Array geometry;
    };
    std::vector<MapMesh> mapMeshes;
    std::size_t usedMapMeshes{};
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
    bool loading{};
    Control* creditsClip{};
    std::vector<Label*> creditsFallback;

    void update_credits_fallback(const std::vector<FrontendCreditsText>& lines, bool active) {
        if (!active) {
            if (creditsClip) {
                canvas.remove_child(creditsClip);
                creditsClip->queue_free();
                creditsClip = nullptr;
                creditsFallback.clear();
            }
            return;
        }
        if (lines.empty()) {
            if (creditsClip) creditsClip->hide();
            return;
        }
        if (!creditsClip) {
            creditsClip = memnew(Control);
            creditsClip->set_name("PortCreditsFallback");
            creditsClip->set_clip_contents(true);
            creditsClip->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
            creditsClip->set_z_index(21);
            canvas.add_child(creditsClip);
        }
        const auto& clip = lines.front();
        creditsClip->set_position({clip.clipLeft, clip.clipTop});
        creditsClip->set_size({clip.clipWidth, clip.clipHeight});
        creditsClip->show();
        for (std::size_t i = 0; i < lines.size(); ++i) {
            const auto& line = lines[i];
            if (i == creditsFallback.size()) {
                auto* label = memnew(Label);
                label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
                label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
                label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
                label->add_theme_constant_override("outline_size", 3);
                creditsClip->add_child(label);
                creditsFallback.push_back(label);
            }
            auto& label = *creditsFallback[i];
            label.add_theme_font_size_override("font_size", std::max(1, int(std::lround(line.height))));
            label.add_theme_color_override("font_color", color(line.color));
            label.set_text(String::utf8(line.text));
            const float width = std::max(1.0f, 2.0f * std::min(line.centerX - clip.clipLeft,
                clip.clipLeft + clip.clipWidth - line.centerX));
            label.set_size({width, line.height});
            label.set_position({line.centerX - clip.clipLeft - width * 0.5f, line.top - clip.clipTop});
            label.show();
        }
        for (std::size_t i = lines.size(); i < creditsFallback.size(); ++i)
            creditsFallback[i]->hide();
    }
    ApplicationRuntime& pointerRuntime;
    Control& pointerSurface;
    using Action = FrontendPointerAction;
    struct Hit { Rect2 rect; FrontendPointerTarget target; };
    std::vector<Hit> hits;
    std::optional<FrontendPointerTarget> pressedTarget, hoverTarget;
    std::uint64_t pointerEpoch{};
    FrontendPointerTarget pointerPage{};
    bool pointerModal{}, pointerReplay{}, pointerHelp{}, pointerTitle{}, pointerHorizontal{};

    static bool same_target(const FrontendPointerTarget& a, const FrontendPointerTarget& b) {
        return a.screen == b.screen && a.object == b.object && a.event == b.event && a.action == b.action;
    }

    void send_pointer(FrontendPointerTarget target, Action action) {
        target.action = action;
        pointerRuntime.QueueFrontendPointer(pointerEpoch, target);
    }

    void pointer_input(const Ref<InputEvent>& event) {
        const Ref<InputEventMouse> mouse = event;
        if (mouse.is_null()) return;
        const auto point = mouse->get_position() + pointerSurface.get_position();
        auto found = std::find_if(hits.rbegin(), hits.rend(),
            [&](const Hit& hit) { return hit.rect.has_point(point); });
        std::optional<FrontendPointerTarget> target;
        if (found != hits.rend()) target = found->target;
        if (!target && pointerTitle) target = pointerPage;
        if (const Ref<InputEventMouseMotion> motion = event; motion.is_valid()) {
            if (target && (target->object || target->event) && target->action == Action::Confirm &&
                !target->event && (!hoverTarget || !same_target(*hoverTarget, *target))) {
                send_pointer(*target, Action::Focus);
            }
            hoverTarget = target;
            return;
        }
        const Ref<InputEventMouseButton> button = event;
        if (button.is_null()) return;
        switch (button->get_button_index()) {
        case MOUSE_BUTTON_LEFT:
            if (button->is_pressed()) pressedTarget = target;
            else {
                if (target && pressedTarget && same_target(*target, *pressedTarget))
                    send_pointer(*target, target->action);
                pressedTarget.reset();
            }
            break;
        case MOUSE_BUTTON_RIGHT:
            if (button->is_pressed()) send_pointer(pointerPage, Action::Back);
            break;
        case MOUSE_BUTTON_WHEEL_UP: case MOUSE_BUTTON_WHEEL_DOWN:
            if (button->is_pressed() && !pointerModal) {
                const bool up = button->get_button_index() == MOUSE_BUTTON_WHEEL_UP;
                if (pointerReplay) send_pointer(pointerPage, up ? Action::ReplayPrevious : Action::ReplayNext);
                else if (pointerHorizontal || (target && target->action == Action::Right)) {
                    const auto entry = target && target->action == Action::Right ? *target : pointerPage;
                    send_pointer(entry, up ? Action::Left : Action::Right);
                } else send_pointer(pointerPage, up ? Action::Previous : Action::Next);
            }
            break;
        default: break;
        }
    }

    void update_pointer(const FrontendStateView& state) {
        hits.clear();
        if (pointerEpoch != state.pointerEpoch || pointerReplay != state.pointerReplay ||
            pointerHelp != state.pointerHelp) {
            pressedTarget.reset();
            hoverTarget.reset();
            pointerEpoch = state.pointerEpoch;
        }
        pointerModal = state.pointerModal;
        pointerReplay = state.pointerReplay;
        pointerHelp = state.pointerHelp;
        pointerTitle = !pointerModal && screen == "Title Screen";
        pointerHorizontal = screen == "Select Car" || screen == "Select Track";
        const auto page = std::find_if(state.pointerTargets.begin(), state.pointerTargets.end(),
            [](const auto& target) { return target.object == 0; });
        pointerPage = page == state.pointerTargets.end() ? FrontendPointerTarget{} : *page;
        if ((page == state.pointerTargets.end() && !pointerReplay) || loading) {
            pressedTarget.reset();
            hoverTarget.reset();
            pointerSurface.hide();
            return;
        }
        pointerSurface.set_position(layout.visible_rect().position);
        pointerSurface.set_size(layout.visible_rect().size);
        pointerSurface.show();
        if (pointerHelp) return;
        const auto add = [&](Rect2 rect, FrontendPointerTarget target) {
            rect = rect.intersection(layout.visible_rect());
            if (!rect.has_area()) return;
            auto previous = std::find_if(hits.begin(), hits.end(),
                [&](const Hit& hit) { return same_target(hit.target, target); });
            if (previous == hits.end()) hits.push_back({rect, target});
            else previous->rect = previous->rect.merge(rect);
        };
        std::vector<std::uint32_t> characters;
        if (!pointerModal && screen == "NameEntry") {
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 14; ++col) {
                    char name[16];
                    std::snprintf(name, sizeof(name), "CHAR_%X%X", row, col);
                    characters.push_back(hashString(name));
                }
        }
        for (int layer = 28; layer >= 0; --layer) {
            for (const auto* strip : state.layout->layers[layer]) {
                auto item = std::find_if(state.pointerTargets.begin(), state.pointerTargets.end(),
                    [&](const auto& entry) {
                        return entry.object && entry.object == strip->sourceObject &&
                            (!strip->sourceScreen || entry.screen == strip->sourceScreen);
                    });
                FrontendPointerTarget target;
                if (item != state.pointerTargets.end()) target = *item;
                else if (!pointerModal && (strip->sourceObject == 0xd1cda594u ||
                    strip->sourceObject == 0x9345e4a7u || strip->sourceObject == 0xedf2e4eeu ||
                    strip->sourceObject == 0x20d73d01u)) {
                    if (strip->sourceScreen && strip->sourceScreen != pointerPage.screen) continue;
                    target = pointerPage;
                    switch (strip->sourceObject) {
                    case 0xd1cda594u: target.action = Action::Left; break;
                    case 0x9345e4a7u: target.action = Action::Right; break;
                    case 0xedf2e4eeu: target.action = Action::Previous; break;
                    default: target.action = Action::Next; break;
                    }
                } else if (!pointerModal && std::any_of(state.layout->pointerArrowRows.begin(),
                    state.layout->pointerArrowRows.end(), [&](const auto& row) {
                        return row.screen == pointerPage.screen && row.value == strip->sourceObject;
                    })) {
                    target = pointerPage;
                    target.object = state.selectedItem;
                    target.action = Action::Right;
                } else if (std::find(characters.begin(), characters.end(), strip->sourceObject) != characters.end()) {
                    target = pointerPage;
                    target.object = strip->sourceObject;
                } else continue;
                for (auto* part = strip; part; part = part->nextTexture.get()) {
                    for (const auto& polygon : part->polygons) {
                        if (std::none_of(polygon.vertices.begin(), polygon.vertices.end(),
                            [](const auto& v) { return (v.color >> 24) != 0; })) continue;
                        Rect2 bounds({polygon.vertices[0].position[0], polygon.vertices[0].position[1]}, {});
                        for (const auto& v : polygon.vertices) bounds = bounds.expand({v.position[0], v.position[1]});
                        add(bounds, target);
                    }
                }
            }
        }
        if (!pointerModal && screen == "View Event Tree") {
            for (const auto& panel : state.layout->pointerEventPanels) {
                const auto& b = panel.bounds;
                auto target = pointerPage;
                target.event = panel.number;
                add(Rect2({b[0], b[1]}, {b[2] - b[0], b[3] - b[1]}).abs(), target);
            }
        }
    }
    struct Highlight {
        Node2D* node{};
        Ref<Tween> tween;
        std::uint32_t target{}, revision{};
        std::uint64_t frame{};
    };
    std::map<std::pair<std::uint32_t, std::uint32_t>, Highlight> highlights;
    struct Preview {
        TextureRect* display{};
        SubViewport* viewport{};
        Camera3D* camera{};
        std::unique_ptr<RaceEffectsPresentation> effects;
        std::vector<MeshInstance3D*> nodes;
        std::vector<std::tuple<std::uint32_t, std::uintptr_t, std::uintptr_t, std::uint64_t, bool, bool>> models;
        std::vector<std::vector<const EditorMarkerTexture*>> textures;
    };
    std::map<int, Preview> previews;
    CarPaintAssets previewPaint;

    void update_previews(const FrontendStateView& state,const RaceEffectsFrame& effects) {
        static const auto platformModel = hashString("PLATFORM");
        if (!state.previews || !state.cameras) throw std::runtime_error("Frontend preview owners are unavailable");
        std::map<int, std::vector<const FrontendModelRequest*>> views;
        for (const auto& request : state.previews->models) {
            if (request.viewNumber < 1 || request.viewNumber > 2)
                throw std::runtime_error("Unsupported frontend preview view " + std::to_string(request.viewNumber));
            if (request.model && request.model->solid) views[request.viewNumber].push_back(&request);
        }
        if (!views.empty() && previewPaint.reflection.is_null()) {
            auto& resources = pointerRuntime.PresentationResources();
            const auto* specular = resources.SpecialTexture(6);
            const auto* reflection = resources.SpecialTexture(8);
            previewPaint.specular = upload_paint_texture(specular, true);
            previewPaint.reflection = upload_paint_texture(reflection, true);
            previewPaint.frontend = true;
            if (previewPaint.reflection.is_null())
                throw std::runtime_error("Frontend paint is missing CarSelectEnvMap");
            if (previewPaint.specular.is_valid())
                previewPaint.specularSampling = paint_texture_sampling(*specular);
            previewPaint.reflectionSampling = paint_texture_sampling(*reflection);
        }
        for (auto& [view, preview] : previews) {
            const bool active = views.count(view) != 0;
            preview.display->set_visible(active);
            if (!active) {
                preview.effects->update_preview_particles(effects);
                preview.viewport->set_update_mode(SubViewport::UPDATE_DISABLED);
            }
        }
        // Fit the authored camera to the physical pixels occupied by its
        // 4:3 canvas. Keep the UI rectangle in design coordinates.
        const Vector2i resolution(std::max(2, int(std::ceil(designSize.x * layout.scale))),
                                  std::max(2, int(std::ceil(designSize.y * layout.scale))));
        for (const auto& [view, models] : views) {
            auto& preview = previews[view];
            if (!preview.viewport) {
                preview.display = memnew(TextureRect);
                preview.display->set_name(String("PreviewDisplay") + String::num_int64(view));
                canvas.add_child(preview.display);
                preview.display->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
                preview.display->set_z_index(10);
                preview.display->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
                preview.display->set_size(designSize);
                Ref<ShaderMaterial> composite;
                composite.instantiate();
                composite->set_shader(load_shader("res://shader/frontend_preview.gdshader"));
                preview.display->set_material(composite);
                preview.viewport = memnew(SubViewport);
                preview.viewport->set_name("PreviewViewport");
                preview.display->add_child(preview.viewport);
                preview.viewport->set_use_own_world_3d(true);
                preview.viewport->set_transparent_background(true);
                preview.viewport->set_msaa_3d(Viewport::MSAA_4X);
                preview.viewport->set_scaling_3d_scale(1.0f);
                preview.display->set_texture(preview.viewport->get_texture());
                preview.camera = memnew(Camera3D);
                preview.viewport->add_child(preview.camera);
                preview.camera->make_current();
                preview.camera->set_cull_mask_value(3,false);
                auto* effectsRoot=memnew(Node3D);
                preview.viewport->add_child(effectsRoot);
                preview.effects=std::make_unique<RaceEffectsPresentation>(*effectsRoot);
            }
            if (preview.viewport->get_size() != resolution)
                preview.viewport->set_size(resolution);
            if (preview.viewport->get_update_mode() != SubViewport::UPDATE_WHEN_VISIBLE)
                preview.viewport->set_update_mode(SubViewport::UPDATE_WHEN_VISIBLE);
            const auto* camera = state.cameras->screens[view].view;
            if (!camera) throw std::runtime_error("Frontend preview has no source camera");
            const auto& pose = camera->current;
            const auto eye = race_vector(pose.eye);
            const auto forward = race_vector(pose.forward);
            const float up[] = {-pose.matrix[1],-pose.matrix[5],-pose.matrix[9]};
            if (forward.cross(race_vector(up)).length_squared() < 0.01f)
                throw std::runtime_error("Frontend preview camera basis is invalid");
            const std::string_view page = state.screen ? state.screen : "";
            const bool carSelection = page == "Select Car" || page == "Select Car2P";
            const bool ntsc = state.cameras->projection.nativeSourceVideoMode == HP2_VIDEO_NTSC;
            const auto fov = carSelection ? source_main_camera(pose, ntsc).verticalFov :
                pose.fieldOfView * (360.0f / 65536.0f);
            preview.camera->set_perspective(fov, pose.nearClip, pose.farClip);
            preview.camera->look_at_from_position(eye, eye + forward, race_vector(up));
            if (state.effects) preview.effects->update_preview_effects(*state.effects,*preview.camera,view);
            const float sourceFocal = 256.0f / std::tan(pose.fieldOfView * (3.141592654f / 65536.0f));
            preview.effects->update_preview_particles(effects, sourceFocal);

            MeshConverter converter(&previewPaint);
            MeshConverter unlitConverter;
            auto reflectedPaint = previewPaint;
            reflectedPaint.reflection.unref();
            MeshConverter reflectedConverter(&reflectedPaint);
            for (std::size_t i = 0; i < models.size(); ++i) {
                const auto& request = *models[i];
                const auto& model = *request.model;
                const auto identity = std::make_tuple(model.key, reinterpret_cast<std::uintptr_t>(model.solid),
                    reinterpret_cast<std::uintptr_t>(model.replacement), model.resources ? model.resources->bindingRevision : 0,
                    request.reflectedCar, request.lighting.has_value());
                std::vector<const EditorMarkerTexture*> replacements;
                for (int slot = 0; slot < model.replacementCount; ++slot)
                    replacements.push_back(model.replacement[slot].texture);
                if (i == preview.nodes.size()) {
                    auto* node = memnew(MeshInstance3D);
                    preview.viewport->add_child(node);
                    preview.nodes.push_back(node);
                    preview.models.emplace_back();
                    preview.textures.emplace_back();
                }
                auto* node = preview.nodes[i];
                const bool platform = model.key == platformModel && !request.lighting;
                if (preview.models[i] != identity || preview.textures[i] != replacements) {
                    auto& meshConverter = !request.lighting ? unlitConverter :
                        request.reflectedCar ? reflectedConverter : converter;
                    node->set_mesh(platform ? meshConverter.convert_frontend_platform(model) :
                        meshConverter.convert(model, true, 0, false, request.reflectedCar ?
                            MeshConverter::ScenerySurface::OpaqueOnly : MeshConverter::ScenerySurface::Full));
                    node->set_meta("source_reflected_car", request.reflectedCar);
                    preview.models[i] = identity;
                    preview.textures[i] = std::move(replacements);
                }
                node->set_transform(race_transform(request.transform.data()));
                const bool hasPaint = request.lighting && node->get_mesh()->has_meta("hp2_car_paint");
                if (hasPaint) apply_paint_lighting(*node, *request.lighting);
                if (hasPaint || platform)
                    node->set_instance_shader_parameter("hp2_paint_focal", Vector3(sourceFocal, 0, 0));
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

    void draw_grid(Node2D& target, const FlailerPoly& polygon, const Ref<ImageTexture>& texture) {
        const auto& topLeft = polygon.vertices[0];
        const auto& bottomRight = polygon.vertices[2];
        const Vector2 origin(topLeft.position[0], topLeft.position[1]);
        const Vector2 tileSize = Vector2(bottomRight.position[0], bottomRight.position[1]) - origin;
        if (tileSize.x <= 0 || tileSize.y <= 0) return;
        const auto visible = layout.visible_rect();
        const auto end = visible.get_end();
        const Vector2 start(origin.x + std::floor((visible.position.x - origin.x) / tileSize.x) * tileSize.x,
                            origin.y + std::floor((visible.position.y - origin.y) / tileSize.y) * tileSize.y);
        const auto dimensions = texture->get_size();
        const Vector2 uvStart(topLeft.textureCoordinate[0], topLeft.textureCoordinate[1]);
        const Vector2 uvEnd(bottomRight.textureCoordinate[0], bottomRight.textureCoordinate[1]);
        auto* rendering = RenderingServer::get_singleton();
        for (float y = start.y; y < end.y; y += tileSize.y) {
            for (float x = start.x; x < end.x; x += tileSize.x) {
                const Vector2 tile(x, y);
                const auto clipped = visible.intersection({tile, tileSize});
                if (!clipped.has_area()) continue;
                const auto uv = uvStart + (clipped.position - tile) / tileSize * (uvEnd - uvStart);
                const auto uvSize = clipped.size / tileSize * (uvEnd - uvStart);
                rendering->canvas_item_add_texture_rect_region(target.get_canvas_item(), clipped,
                    texture->get_rid(), {uv * dimensions, uvSize * dimensions}, color(topLeft.color));
            }
        }
    }

    static bool spans_canvas(const FlailerPoly& polygon) {
        const auto& first = polygon.vertices[0].position;
        const auto& last = polygon.vertices[2].position;
        return first[0] <= 2 && first[1] <= 3 && last[0] >= 638 && last[1] >= 477;
    }

    void draw(Node2D& target, const FlailerStrip& strip, Vector2 origin = {}, bool projective = false) {
        // PAL145950 traverses secondary atlases before the base texture.
        if (strip.nextTexture) draw(target, *strip.nextTexture, origin, projective);
        if (strip.polygons.empty()) return;
        if (!strip.texture) throw std::runtime_error("Frontend layout is missing its texture descriptor");
        if (strip.texture->key == titleMovie || strip.texture->key == panelMovie) {
            if (!video.playing()) return;
            const auto& quad = strip.polygons.front().vertices;
            const Vector2 position(quad[0].position[0], quad[0].position[1]);
            const Rect2 panel(position, Vector2(quad[2].position[0], quad[2].position[1]) - position);
            if (!panel.has_area()) return;
            const bool title = strip.sourceScreen == titleScreen;
            const auto rect = title ? panel : layout.fit_movie(panel);
            RenderingServer::get_singleton()->canvas_item_add_rect(videoMatte.get_canvas_item(),
                title ? layout.visible_rect() : panel,
                Color(0, 0, 0));
            player.set_position(rect.position);
            player.set_size(rect.size);
            player.set_z_index(title ? 31 : 1);
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
        // Select Car moves/clips its grid to reveal only the authored
        // backdrop fragment. Repeating that thin fragment creates stripes
        // and speckles across the car. Tile only a full-canvas grid.
        if (strip.sourceObject == gridObject && std::all_of(strip.polygons.begin(), strip.polygons.end(), spans_canvas)) {
            for (const auto& polygon : strip.polygons)
                if (visible(polygon)) draw_grid(target, polygon, texture);
            return;
        }
        PackedVector2Array points, uv;
        PackedColorArray colors;
        PackedInt32Array indices;
        PackedFloat32Array textureQ;
        for (const auto& polygon : strip.polygons) {
            if (!visible(polygon)) continue;
            const auto base = static_cast<int32_t>(points.size());
            const auto& bounds = polygon.glyphSamplingBounds;
            const auto& topLeft = polygon.vertices[0].position;
            const auto& bottomRight = polygon.vertices[2].position;
            const bool raceLetterbox = is_player_letterbox(strip.sourceObject);
            const bool letterboxTop = strip.sourceObject == 0xD911BEACu || strip.sourceObject == 0xF7F1CFB3u;
            const bool fullCanvas = !raceLetterbox && origin == Vector2{} && !projective &&
                topLeft[0] <= 1 && topLeft[1] <= 2 && bottomRight[0] >= 639 && bottomRight[1] >= 479 &&
                bounds == std::array<float,4>{};
            // The popup's anonymous top and bottom backdrops span the
            // authored canvas. Keep their inner edge and fill the surround.
            const bool edgeBackdrop = !fullCanvas && origin == Vector2{} && !projective &&
                bounds == std::array<float,4>{} && topLeft[0] <= 4 && bottomRight[0] >= 636 &&
                bottomRight[1] - topLeft[1] >= 48 &&
                ((topLeft[1] <= 2 && bottomRight[1] < 120) ||
                 (topLeft[1] > 360 && bottomRight[1] >= 478));
            const bool curtain = edgeBackdrop && !raceLetterbox;
            // Shared header backdrops are full-width bitmap strips. Text,
            // preview panels and menu rows remain on the authored canvas.
            const float height = bottomRight[1] - topLeft[1];
            const bool decorativeBand = !raceLetterbox && origin == Vector2{} && !projective &&
                bounds == std::array<float,4>{} && topLeft[0] <= 4 && bottomRight[0] >= 636 &&
                topLeft[1] >= 0 && height >= 0 && height <= 48 &&
                (screen == "Title Screen" || strip.sourceScreen == loadingScreen ||
                 topLeft[1] <= 75 || height <= 4);
            const float cover = std::max(layout.visible.x / designSize.x, layout.visible.y / designSize.y);
            const auto viewport = layout.visible_rect();
            if (raceLetterbox) {
                const auto fill = [&](Rect2 rect) {
                    rect = rect.intersection(viewport);
                    if (rect.has_area())
                        RenderingServer::get_singleton()->canvas_item_add_rect(
                            target.get_canvas_item(), rect, Color(0, 0, 0, color(polygon.vertices[0].color).a));
                };
                const float top = topLeft[1], bottom = bottomRight[1];
                const auto end = viewport.get_end();
                fill({viewport.position.x, top, topLeft[0] - viewport.position.x, bottom - top});
                fill({bottomRight[0], top, end.x - bottomRight[0], bottom - top});
                if (letterboxTop && viewport.position.y < 0)
                    fill({viewport.position.x, viewport.position.y, viewport.size.x, -viewport.position.y});
                else if (!letterboxTop && end.y > designSize.y)
                    fill({viewport.position.x, designSize.y, viewport.size.x, end.y - designSize.y});
            }
            for (const auto& vertex : polygon.vertices) {
                Vector2 point(vertex.position[0], vertex.position[1]);
                if (fullCanvas) point = (point - designSize * 0.5f) * cover + designSize * 0.5f;
                if ((decorativeBand || curtain) && bottomRight[0] > topLeft[0])
                    point.x = viewport.position.x + viewport.size.x *
                        ((point.x - topLeft[0]) / (bottomRight[0] - topLeft[0]));
                if (curtain) {
                    // Preserve the animated inner edge. Extend only the edge
                    // facing away from the center of the authored canvas.
                    const float middle = (topLeft[1] + bottomRight[1]) * 0.5f;
                    if (middle < designSize.y * 0.5f && point.y == topLeft[1])
                        point.y = std::min(point.y, viewport.position.y);
                    else if (middle > designSize.y * 0.5f && point.y == bottomRight[1])
                        point.y = std::max(point.y, viewport.get_end().y);
                }
                points.push_back(point - origin);
                const auto& uvq = vertex.textureCoordinate;
                if (uvq[2] == 0.0f) throw std::runtime_error("Frontend polygon has a zero texture projection");
                // TrackMap::ClipPolygon has already interpolated homogeneous
                // UVQ. Keep its numerators and q until the fragment shader.
                float u = uvq[0], v = uvq[1];
                if (projective) textureQ.push_back(uvq[2]);
                else {
                    u /= uvq[2];
                    v /= uvq[2];
                }
                if (bounds != std::array<float,4>{}) {
                    u = std::clamp(u, bounds[0], std::max(bounds[0], bounds[2]));
                    v = std::clamp(v, bounds[1], std::max(bounds[1], bounds[3]));
                }
                uv.push_back({u, v});
                colors.push_back(color(vertex.color));
            }
            for (int32_t offset : {0, 1, 2, 0, 2, 3}) indices.push_back(base + offset);
        }
        auto* rendering = RenderingServer::get_singleton();
        if (projective) {
            Array arrays;
            arrays.resize(Mesh::ARRAY_MAX);
            arrays[Mesh::ARRAY_VERTEX] = points;
            arrays[Mesh::ARRAY_COLOR] = colors;
            arrays[Mesh::ARRAY_TEX_UV] = uv;
            arrays[Mesh::ARRAY_CUSTOM0] = textureQ;
            arrays[Mesh::ARRAY_INDEX] = indices;
            if (usedMapMeshes == mapMeshes.size()) mapMeshes.emplace_back();
            auto& cached = mapMeshes[usedMapMeshes++];
            if (cached.mesh.is_null() || cached.geometry != arrays) {
                Ref<ArrayMesh> mesh;
                mesh.instantiate();
                mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays, {}, {},
                    Mesh::ARRAY_FLAG_USE_2D_VERTICES | (Mesh::ARRAY_CUSTOM_R_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT));
                cached = {mesh, arrays};
            }
            rendering->canvas_item_add_mesh(target.get_canvas_item(), cached.mesh->get_rid(), {}, Color(1, 1, 1, 1),
                texture->get_rid());
        } else {
            rendering->canvas_item_add_triangle_array(target.get_canvas_item(),
                indices, points, colors, uv, {}, {}, texture->get_rid());
        }
    }

    void draw_layers(const FlailerRenderManager& layout, Node2D& target, int first, int last) {
        for (int layer = first; layer >= last; --layer)
            for (const auto* strip : layout.layers[layer]) draw(target, *strip, {}, layer == 14);
    }

    void update_highlights(const FlailerRenderManager& layout) {
        auto* rendering = RenderingServer::get_singleton();
        for (const auto& source : layout.highlights) {
            if (!layout.IsScreenVisible(source.screen)) continue;
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
                highlight.node->set_meta("flailer_highlight_screen", static_cast<int64_t>(source.screen));
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

FrontendPresentation::FrontendPresentation(Node2D& canvas, const GameResources& files,
    ApplicationRuntime& runtime, RacePresentation& host)
    : storage_(std::make_unique<Storage>(canvas, files, runtime, host)) {}
FrontendPresentation::~FrontendPresentation() = default;
void FrontendPresentation::pointer_input(const Ref<InputEvent>& event) { storage_->pointer_input(event); }
bool FrontendPresentation::video_playing() const noexcept {return storage_->video.playing();}
std::uint64_t FrontendPresentation::video_request() const noexcept {return storage_->video.request_id();}
bool FrontendPresentation::showing_loading() const noexcept {return storage_->loading;}

void FrontendPresentation::update_rear_view(const Ref<Texture2D>& view,
    const std::array<const EditorMarkerTexture*, 2>& textures, bool ntsc) {
    auto& s = *storage_;
    if (view.is_null()) {
        if (s.rearView) {
            s.rearView->hide();
            s.rearView->set_texture({});
        }
        return;
    }
    std::array<Ref<ImageTexture>, 2> overlays;
    for (std::size_t index = 0; index < overlays.size(); ++index) {
        if (!textures[index]) throw std::runtime_error("Missing original RVM or RVM_MASK descriptor");
        const auto* texture = textureInfoForDescriptor(textures[index]);
        if (!texture || !texture->pixels.Data() || (texture->paletteBytes && !texture->palette.Data())) {
            if (s.rearView) s.rearView->hide();
            return;
        }
        overlays[index] = s.texture_for(*texture);
    }
    if (!s.rearView) {
        s.rearView = memnew(TextureRect);
        s.rearView->set_name("RearViewMirror");
        s.rearView->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        s.rearView->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        s.rearView->set_z_index(19);
        s.canvas.add_child(s.rearView);
        Ref<Shader> shader;
        // PAL10EC70 draws both overlays in target coordinates. PAL10EDC8
        // flips the completed image. ALPHA44 blends the frame and ALPHA48
        // adds the mask RGB and replaces the resulting target alpha.
        shader = load_shader("res://shader/rear_view_mask.gdshader");
        s.rearMaterial.instantiate();
        s.rearMaterial->set_shader(shader);
        s.rearView->set_material(s.rearMaterial);
    }
    const float yScale = 480.0f / (ntsc ? 448.0f : 512.0f);
    s.rearView->set_position({200, 20 * yScale});
    s.rearView->set_size({240, 64 * yScale});
    if (s.rearView->get_texture() != view) s.rearView->set_texture(view);
    if (s.rearOverlays != overlays) {
        s.rearMaterial->set_shader_parameter("frame_texture", overlays[0]);
        s.rearMaterial->set_shader_parameter("mask_texture", overlays[1]);
        s.rearOverlays = overlays;
    }
    s.rearView->show();
}

void FrontendPresentation::update(const FrontendStateView& state, Vector2 viewport,const RaceEffectsFrame& effects) {
    auto& s = *storage_;
    if (!state.layout || !state.movie)
        throw std::runtime_error("Frontend state is missing an owner");
    s.loading = state.layout->IsScreenVisible(loadingScreen);
    s.layout.resize(viewport);
    s.video.update(*state.movie);
    s.update_previews(state,effects);
    s.player.hide();
    s.canvas.set_scale(Vector2(s.layout.scale, s.layout.scale));
    s.canvas.set_position((s.layout.visible - designSize) * (s.layout.scale * 0.5f));
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
    rendering->canvas_item_clear(s.videoMatte.get_canvas_item());
    rendering->canvas_item_clear(s.mapBackground.get_canvas_item());
    rendering->canvas_item_clear(s.mapTexture.get_canvas_item());
    rendering->canvas_item_clear(s.mapForeground.get_canvas_item());
    s.usedMapMeshes = 0;
    ++s.frame;
    s.draw_layers(*state.layout, s.background, 24, 20);
    s.draw_layers(*state.layout, s.mapBackground, 15, 15);
    // Donor TrackMap::DrawMap submits the road texture on layer 14,
    // between its backdrop and vehicle markers.
    s.draw_layers(*state.layout, s.mapTexture, 14, 14);
    s.draw_layers(*state.layout, s.mapForeground, 13, 0);
    s.draw_layers(*state.layout, s.mapForeground, 19, 16);
    s.draw_layers(*state.layout, s.mapForeground, 28, 25);
    s.mapMeshes.resize(s.usedMapMeshes);
    s.update_highlights(*state.layout);
    s.update_pointer(state);
    s.update_credits_fallback(state.creditsText, screen == "Port Credits");
    for (auto it = s.textures.begin(); it != s.textures.end();) {
        if (it->second.frame != s.frame)
            it = s.textures.erase(it);
        else ++it;
    }
}

}

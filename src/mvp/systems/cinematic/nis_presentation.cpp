#include "nis_presentation.hpp"

#include "assets/shader_resource.hpp"

#include "assets/mesh_converter.hpp"
#include "indep/inc/NIS.hpp"

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>

#include <unordered_map>

namespace hp2 {

using namespace godot;

struct NisPresentation::Storage {
    struct Part {
        MeshInstance3D* mesh{};
        Skeleton3D* skeleton{};
        const EditorMarkerSolid* solid{};
        bool seen{};
    };
    std::unordered_map<std::size_t, Part> parts;
    ColorRect* filter{};
    std::uint32_t step{};
    std::uint64_t replayEpoch{};
    bool sampled{};
    float sampledFraction{};

    void sample(Node3D& scene, NisManagerRegistry& registry, float worldFrameFraction);

    void submit(Node3D& scene, MeshConverter& converter, std::size_t slot, const EditorMarkerModel& model,
        const float* transform, const std::vector<std::array<float,16>>* pose) {
        auto& part = parts[slot];
        if (!part.mesh || part.solid != model.solid ||
            (pose ? !part.skeleton || static_cast<std::size_t>(part.skeleton->get_bone_count()) != pose->size()
                  : part.skeleton != nullptr)) {
            if (part.mesh) {
                scene.remove_child(part.mesh);
                part.mesh->queue_free();
            }
            const auto mesh = converter.convert(model, true, pose ? pose->size() : 0);
            part = {memnew(MeshInstance3D), nullptr, model.solid, false};
            part.mesh->set_mesh(mesh);
            if (pose) part.skeleton = attach_pose_palette(*part.mesh, "NisPose", pose->size());
            scene.add_child(part.mesh);
        }
        part.seen = true;
        part.mesh->set_transform(race_transform(transform));
        if (pose) {
            for (std::size_t bone = 0; bone < pose->size(); ++bone) {
                const auto transform = race_transform((*pose)[bone].data());
                if (part.skeleton->get_bone_pose(bone) != transform)
                    part.skeleton->set_bone_pose(bone, transform);
            }
        }
    }
};

NisPresentation::NisPresentation(Node& host) : storage_(std::make_unique<Storage>()) {
    auto* layer = memnew(CanvasLayer);
    layer->set_layer(-1);
    host.add_child(layer);
    auto* filter = memnew(ColorRect);
    layer->add_child(filter);
    filter->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    filter->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    Ref<Shader> shader;
    shader = load_shader("res://shader/nis_video.gdshader");
    Ref<ShaderMaterial> material;
    material.instantiate();
    material->set_shader(shader);
    filter->set_material(material);
    filter->hide();
    storage_->filter = filter;
}

NisPresentation::~NisPresentation() = default;

std::size_t NisPresentation::part_count() const { return storage_->parts.size(); }

void NisPresentation::Storage::sample(Node3D& scene, NisManagerRegistry& registry, float worldFrameFraction) {
    for (auto& part : parts) part.second.seen = false;
    MeshConverter converter;
    struct Submission { Storage& storage; Node3D& scene; MeshConverter& converter; };
    Submission submission{*this, scene, converter};
    VisitNisPoses(registry, 1, [](void* user, std::size_t slot, const EditorMarkerModel& model,
        const float* transform, const std::vector<std::array<float,16>>* pose) {
        auto& context = *static_cast<Submission*>(user);
        context.storage.submit(context.scene, context.converter, slot, model, transform, pose);
    }, &submission, worldFrameFraction);
    for (auto it = parts.begin(); it != parts.end();) {
        if (it->second.seen) { ++it; continue; }
        scene.remove_child(it->second.mesh);
        it->second.mesh->queue_free();
        it = parts.erase(it);
    }
}

void NisPresentation::update(Node3D& scene, NisManagerRegistry& registry, bool monochrome,
    std::uint32_t step, std::uint64_t replayEpoch, float worldFrameFraction) {
    auto& storage = *storage_;
    storage.filter->set_visible(monochrome);
    if (storage.sampled && storage.step == step && storage.replayEpoch == replayEpoch &&
        storage.sampledFraction == worldFrameFraction) return;
    storage.step = step;
    storage.replayEpoch = replayEpoch;
    storage.sampled = true;
    storage.sampledFraction = worldFrameFraction;
    storage.sample(scene, registry, worldFrameFraction);
}

void NisPresentation::clear() {
    storage_->parts.clear(); // The scene owns these nodes and is retiring.
    storage_->sampled = false;
    storage_->filter->hide();
}

}

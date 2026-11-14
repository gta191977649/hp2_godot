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
#include <godot_cpp/classes/skin.hpp>

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
            if (pose) {
                // The donor provides final inverse-bind-adjusted matrices.
                // Flat bones with identity binds pass those matrices to Godot's GPU skinning.
                part.skeleton = memnew(Skeleton3D);
                part.skeleton->set_name("NisPose");
                part.mesh->add_child(part.skeleton);
                Ref<Skin> skin;
                skin.instantiate();
                for (std::size_t bone = 0; bone < pose->size(); ++bone) {
                    part.skeleton->add_bone(String("NisBone") + String::num_uint64(bone));
                    skin->add_bind(static_cast<int>(bone), Transform3D());
                }
                part.mesh->set_skin(skin);
                part.mesh->set_skeleton_path(NodePath("NisPose"));
            }
            scene.add_child(part.mesh);
        }
        part.seen = true;
        part.mesh->set_transform(race_transform(transform));
        if (pose) {
            for (std::size_t bone = 0; bone < pose->size(); ++bone)
                part.skeleton->set_bone_pose(static_cast<int>(bone), race_transform((*pose)[bone].data()));
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

void NisPresentation::update(Node3D& scene, NisManagerRegistry& registry, bool monochrome) {
    auto& storage = *storage_;
    storage.filter->set_visible(monochrome);
    for (auto& part : storage.parts) part.second.seen = false;
    MeshConverter converter;
    struct Submission { Storage& storage; Node3D& scene; MeshConverter& converter; };
    Submission submission{storage, scene, converter};
    VisitNisPoses(registry, 1, [](void* user, std::size_t slot, const EditorMarkerModel& model,
        const float* transform, const std::vector<std::array<float,16>>* pose) {
        auto& context = *static_cast<Submission*>(user);
        context.storage.submit(context.scene, context.converter, slot, model, transform, pose);
    }, &submission);
    for (auto it = storage.parts.begin(); it != storage.parts.end();) {
        if (it->second.seen) { ++it; continue; }
        scene.remove_child(it->second.mesh);
        it->second.mesh->queue_free();
        it = storage.parts.erase(it);
    }
}

void NisPresentation::clear() {
    storage_->parts.clear(); // The scene owns these nodes and is retiring.
    storage_->filter->hide();
}

}

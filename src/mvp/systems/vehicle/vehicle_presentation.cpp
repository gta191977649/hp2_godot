#include "vehicle_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "bridge/presentation_snapshot.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/eLight.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/RaceCarModels.hpp"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace hp2 {

using namespace godot;

void VehiclePresentation::attach(Node3D& parent, Car& car, const VehicleEntityState& state, int worldFrames,
    const CarPaintAssets& paint) {
    const auto source = RaceCarModels(car, worldFrames);
    if (!source[0]) throw std::runtime_error("Missing body model for car " + std::to_string(state.model));
    model = state.model;
    skin = state.skin;
    meshes.clear();
    wheelBlur.fill(nullptr);
    rearWheelBlur.fill(nullptr);
    layers = 0;
    root = memnew(Node3D);
    root->set_meta("hp2_player_vehicle", state.player);
    parent.add_child(root);
    MeshConverter converter(&paint);
    for (std::size_t index = 0; index < 5; ++index) {
        const auto* part = source[index];
        if (!part || !part->solid) continue;
        const auto mesh = converter.convert(*part);
        if (!mesh->get_surface_count()) {
            if (index == 0) throw std::runtime_error("Body has no drawable surfaces");
            continue;
        }
        auto* node = memnew(MeshInstance3D);
        root->add_child(node);
        node->set_mesh(mesh);
        meshes.push_back(node);
    }
    MeshConverter wheelConverter(&paint, true);
    for (std::size_t axle = 0; axle < 2; ++axle) {
        const auto* part = source[5 + axle];
        if (!part || !part->solid) throw std::runtime_error("Missing authored wheel for car " + std::to_string(model));
        const auto mesh = wheelConverter.convert(*part);
        if (!mesh->get_surface_count()) throw std::runtime_error("Wheel has no drawable surfaces");
        for (std::size_t index = axle * 2; index < axle * 2 + 2; ++index) {
            auto* node = memnew(MeshInstance3D);
            node->set_name(String("Wheel") + String::num_int64(index));
            root->add_child(node);
            node->set_mesh(mesh);
            meshes.push_back(node);
            wheels[index] = node;
        }
    }
    std::array<Ref<ArrayMesh>, 2> blurMeshes;
    for (std::size_t axle = 0; axle < blurMeshes.size(); ++axle) {
        const auto* part = carRenderModel(&car, 10 + axle);
        if (part && part->solid) blurMeshes[axle] = converter.convert_wheel_blur(*part);
    }
    for (const auto index : {0, 3, 1, 2}) {
        const auto& mesh = blurMeshes[index / 2];
        if (mesh.is_null() || !mesh->get_surface_count()) continue;
        const auto addBlur = [&](const char* name, std::uint32_t layer) {
            auto* node = memnew(MeshInstance3D);
            node->set_name(String(name) + String::num_int64(index));
            node->set_mesh(mesh);
            node->set_layer_mask(layer);
            node->hide();
            root->add_child(node);
            return node;
        };
        wheelBlur[index] = addBlur("WheelBlur", primaryLayer);
        rearWheelBlur[index] = addBlur("RearWheelBlur", rearLayer);
    }
}

void VehiclePresentation::update(const VehicleEntityState& state, const LightSceneRegistry& lights,
    const float* environment, const RaceStateView& race) {
    root->set_transform(race_transform(state.transform.data()));
    const auto nextLayers = (state.visible ? primaryLayer : 0u) |
        (state.rearVisible ? rearLayer : 0u);
    root->set_visible(nextLayers != 0);
    if (nextLayers != layers) {
        for (auto* mesh : meshes) mesh->set_layer_mask(nextLayers);
        layers = nextLayers;
    }
    const auto updateBlur = [](MeshInstance3D* node, const WheelBlurState& blur) {
        if (!node) return;
        node->set_visible(blur.alpha > 0.0f);
        if (blur.alpha <= 0.0f) return;
        node->set_transform(race_transform(blur.transform.data()));
        node->set_instance_shader_parameter("hp2_wheel_blur_alpha", blur.alpha);
    };
    for (std::size_t index = 0; index < wheels.size(); ++index) {
        wheels[index]->set_transform(race_transform(state.wheels[index].data()));
        updateBlur(wheelBlur[index], state.wheelBlur[index]);
        updateBlur(rearWheelBlur[index], state.rearWheelBlur[index]);
    }
    if (!nextLayers) return;
    LightContextBatch batch(lights);
    const auto body = batch.SetupRace(state.transform.data(), environment, race.camera->eye);
    const auto focal = [](const CarCinematicViewFrame& camera) {
        return 256.0f / std::tan(camera.fieldOfView * (3.141592654f / 65536.0f));
    };
    const float front = focal(*race.camera);
    const float rear = race.rearCamera ? focal(*race.rearCamera) : front;
    const float frontProjection = front / 256.0f / (race.ntsc ? 448.0f / (512.0f * 1.0714286566f) : 1.0f / 1.285f);
    const float rearProjection = rear / 256.0f / (64.0f / (512.0f * 1.08f));
    for (auto* mesh : meshes) {
        if (!mesh->get_mesh()->has_meta("hp2_car_paint")) continue;
        mesh->set_instance_shader_parameter("hp2_paint_focal", Vector3(front, rear, (frontProjection + rearProjection) * 0.5f));
        const auto wheel = std::find(wheels.begin(), wheels.end(), mesh);
        if (wheel == wheels.end()) {
            apply_paint_lighting(*mesh, body);
        } else {
            float transform[16];
            eMulMatrix(transform, state.wheels[wheel - wheels.begin()].data(), state.transform.data());
            apply_paint_lighting(*mesh, batch.SetupRace(transform, race.camera->matrix, race.camera->eye));
        }
    }
}

}

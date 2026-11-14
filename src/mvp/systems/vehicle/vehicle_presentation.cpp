#include "vehicle_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "bridge/presentation_snapshot.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/eLight.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/CameraMover.hpp"
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
    layers = 0;
    root = memnew(Node3D);
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
            root->add_child(node);
            node->set_mesh(mesh);
            meshes.push_back(node);
            wheels[index] = node;
        }
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
    for (std::size_t index = 0; index < wheels.size(); ++index)
        wheels[index]->set_transform(race_transform(state.wheels[index].data()));
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

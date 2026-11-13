#include "vehicle_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "bridge/presentation_snapshot.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/RaceCarModels.hpp"

#include <godot_cpp/classes/mesh_instance3d.hpp>

#include <stdexcept>

namespace hp2 {

using namespace godot;

void VehiclePresentation::attach(Node3D& parent, Car& car, const VehicleEntityState& state, int worldFrames) {
    const auto source = RaceCarModels(car, worldFrames);
    if (!source[0]) throw std::runtime_error("Missing body model for car " + std::to_string(state.model));
    model = state.model;
    skin = state.skin;
    meshes.clear();
    layers = 0;
    root = memnew(Node3D);
    parent.add_child(root);
    MeshConverter converter;
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
    for (std::size_t axle = 0; axle < 2; ++axle) {
        const auto* part = source[5 + axle];
        if (!part || !part->solid) throw std::runtime_error("Missing authored wheel for car " + std::to_string(model));
        const auto mesh = converter.convert(*part);
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

void VehiclePresentation::update(const VehicleEntityState& state) {
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
}

}

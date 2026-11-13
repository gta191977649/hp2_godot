#include "track_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "indep/inc/Scenery.hpp"

#include <godot_cpp/classes/mesh_instance3d.hpp>

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace hp2 {

using namespace godot;

std::size_t attach_track(Node3D& root, const Scenery& scenery, const SkyResources* sky) {
    MeshConverter converter;
    std::unordered_map<const EditorMarkerModel*, Ref<ArrayMesh>> meshes;
    std::size_t placements{};
    for (std::size_t index = 0; index < scenery.loadedSectionCount; ++index) {
        const auto* section = scenery.loadedSections[index];
        if (!section) throw std::runtime_error("Loaded section table contains an empty entry");
        for (std::size_t item = 0; item < section->instanceCount; ++item) {
            const auto& instance = section->instances[item];
            // Single-player primary view, forward route, as in StuffScenery11321C.
            if (instance.flags & 0x19u) continue;
            if (instance.modelIndex < 0 || std::size_t(instance.modelIndex) >= section->modelCount)
                throw std::runtime_error("Invalid scenery model index in section " + std::to_string(section->number));
            const auto& model = section->models[instance.modelIndex];
            const EditorMarkerModel* level = nullptr;
            for (const auto& candidate : model.levels) {
                if (candidate.solid) {
                    level = &candidate;
                    break;
                }
            }
            if (!level) continue; // Original selection rejects placements without an authored LOD.
            auto& mesh = meshes[level];
            if (mesh.is_null()) mesh = converter.convert(*level);
            if (!mesh->get_surface_count()) continue; // All strips can be non-drawing.
            auto* node = memnew(MeshInstance3D);
            root.add_child(node);
            node->set_mesh(mesh);
            if (!(instance.flags & 0x200u)) {
                float frame[16]{};
                for (int lane = 0; lane < 9; ++lane)
                    frame[(lane / 3) * 4 + lane % 3] = instance.rotation[lane] * (1.0f / 16384.0f);
                std::copy_n(instance.position, 3, frame + 12);
                frame[15] = 1;
                node->set_transform(race_transform(frame));
            }
            ++placements;
        }
    }
    if (sky && sky->Model(0).solid) {
        const auto mesh = converter.convert(sky->Model(0), false);
        if (!mesh->get_surface_count()) throw std::runtime_error("Authored sky has no drawable surfaces");
        auto* node = memnew(MeshInstance3D);
        root.add_child(node);
        node->set_mesh(mesh);
        node->set_transform(race_transform(sky->Transform(0)));
    }
    if (!placements) throw std::runtime_error("No drawable track placements");
    return placements;
}

}

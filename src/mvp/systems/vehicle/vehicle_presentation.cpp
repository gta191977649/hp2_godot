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
#include <godot_cpp/classes/skeleton3d.hpp>

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
    driverMeshes.fill(nullptr);
    driverSkeletons.fill(nullptr);
    driverPoses = {};
    driverModels.fill(0);
    driverTextures.fill(0);
    driverPoseValid = false;
    const auto* seat = RaceCarDriverSeat(car);
    driverSeat = seat ? race_vector(seat) : Vector3();
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
        if (index == 0) bodyBounds = mesh->get_aabb();
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
        auto* node = memnew(MeshInstance3D);
        node->set_name(String("WheelBlur") + String::num_int64(index));
        node->set_mesh(mesh);
        node->hide();
        root->add_child(node);
        wheelBlur[index] = node;
    }
}

void VehiclePresentation::update(const VehicleEntityState& state, const LightSceneRegistry& lights,
    const float* environment, const RaceStateView& race, bool showExterior) {
    root->set_transform(race_transform(state.transform.data()));
    const auto nextLayers = (showExterior ? primaryLayer : 0u) |
        (state.rearVisible ? rearLayer : 0u);
    root->set_visible(nextLayers != 0);
    if (nextLayers != layers) {
        for (auto* mesh : meshes) mesh->set_layer_mask(nextLayers);
        for (auto* mesh : wheelBlur) if (mesh) mesh->set_layer_mask(nextLayers);
        layers = nextLayers;
    }
    for (std::size_t index = 0; index < wheels.size(); ++index) {
        wheels[index]->set_transform(race_transform(state.wheels[index].data()));
        if (auto* node = wheelBlur[index]) {
            const auto& blur = state.wheelBlur[index];
            node->set_visible(nextLayers && blur.alpha > 0.1f);
            if (!node->is_visible()) continue;
            node->set_transform(race_transform(blur.transform.data()));
            node->set_instance_shader_parameter("hp2_wheel_blur_alpha", blur.alpha);
        }
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

void VehiclePresentation::update_driver(Car& car, const VehicleEntityState& state,
    const CarCinematicViewFrame& camera, std::uint32_t step, std::uint64_t replayEpoch, float fraction) {
    auto* visual = car.visual;
    int detail = -1;
    if ((layers & primaryLayer) && car.driverVisible && visual && visual->bindings.vehicle &&
        !visual->replaySuppressed && RaceCarDriverSeat(car)) {
        CarCinematicView view{};
        view.current = camera;
        CarCinematicScreen screen{};
        screen.view = &view;
        screen.matricesCalculated = 1;
        screen.sourcePixelScale = 256.0f / std::tan(camera.fieldOfView * (3.141592654f / 65536.0f));
        const auto pixels = ecstasyViewSourcePositionPixelSize(&screen, state.transform.data() + 12, 3.0f);
        const auto minimum = visual->bindings.vehicle->driverAnimationCode ? 50 : 75;
        if (carRenderDriverVisibilityOverride || pixels > minimum) {
            const auto transform = race_transform(state.transform.data());
            const auto facing = -transform.basis.get_column(0).normalized().dot(
                (race_vector(camera.eye) - transform.origin).normalized());
            const bool gesture = carDriverVisualMaximumRemaining(visual) != 0;
            const bool close = facing > 0.9f ? pixels > 999 || (gesture && pixels > 300) :
                pixels > 300 || (gesture && pixels > 150);
            detail = close || state.player ? 1 : 2;
            if (carDriverVisualForcedDetail >= 0 && carDriverVisualForcedDetail < 3) detail = carDriverVisualForcedDetail;
            if (carDriverVisualForceAnimated && detail >= 2) detail = 1;
        }
    }
    const auto model_at = [&](int slot) -> const EditorMarkerModel* {
        const auto* animation = visual->animations[slot];
        const auto* asset = animation ? animation->model : visual->staticModels[slot];
        return asset && asset->solid ? asset : nullptr;
    };
    if (detail >= 0 && !model_at(detail)) {
        detail = -1;
        for (const int slot : {1, 0, 2}) if (model_at(slot)) { detail = slot; break; }
    }
    if (detail >= 0) {
        const std::array<std::uint32_t, 2> textures{visual->replacements[0].replacementKey,
            visual->replacements[1].replacementKey};
        if (textures != driverTextures) {
            for (auto* mesh : driverMeshes) if (mesh) { mesh->hide(); mesh->queue_free(); }
            driverMeshes.fill(nullptr);
            driverSkeletons.fill(nullptr);
            driverPoses = {};
            driverTextures = textures;
        }
    }
    for (std::size_t slot = 0; slot < driverMeshes.size(); ++slot)
        if (driverMeshes[slot]) driverMeshes[slot]->set_visible(static_cast<int>(slot) == detail);
    if (detail < 0) return;

    const auto* asset = model_at(detail);
    auto& mesh = driverMeshes[detail];
    auto& skeleton = driverSkeletons[detail];
    auto& pose = driverPoses[detail];
    const auto* tree = visual->animations[detail] ? editorMarkerModelAnimationTree(asset) : nullptr;
    const bool rebuild = !mesh || driverModels[detail] != asset->key;
    if (rebuild) {
        pose.valid = false;
        if (mesh) { mesh->hide(); mesh->queue_free(); }
        const auto geometry = MeshConverter().convert(*asset, true, tree ? tree->nodeCount : 0);
        mesh = memnew(MeshInstance3D);
        mesh->set_name(String("RaceDriver") + String::num_int64(detail));
        mesh->set_mesh(geometry);
        mesh->set_layer_mask(primaryLayer);
        root->add_child(mesh);
        skeleton = tree ? attach_pose_palette(*mesh, "DriverPose", tree->nodeCount) : nullptr;
        driverModels[detail] = asset->key;
    }
    if (!tree) {
        mesh->set_position(driverSeat);
        return;
    }
    if (driverReplayEpoch != replayEpoch)
        for (auto& history : driverPoses) history.valid = false;
    if (!driverPoseValid || driverStep != step || driverReplayEpoch != replayEpoch) {
        // The source sampler advances its steering curve. Sample once per
        // committed world step regardless of the presentation frame rate.
        SampleDriverPresentationPose(*visual, *visual->animations[detail], driverRotations, driverTranslation);
        driverStep = step;
        driverReplayEpoch = replayEpoch;
        driverPoseValid = true;
    }
    if (!pose.valid || pose.step != step) {
        const auto& palette = visual->poseCache.Prepare(*tree, driverRotations, CAR_DRIVER_VISUAL_BONE_COUNT);
        pose.capture(step, Transform3D(Basis(), driverSeat + race_vector(driverTranslation)), &palette);
    }
    pose.apply(*mesh, skeleton, fraction);
}

}

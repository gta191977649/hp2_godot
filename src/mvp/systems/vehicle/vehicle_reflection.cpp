#include "vehicle_reflection.hpp"

#include "assets/mesh_converter.hpp"
#include "assets/shader_resource.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/CarState.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/eEnvMap.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/World.hpp"
#include "systems/track/track_presentation.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh_instance2d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world3d.hpp>

#include <cmath>

namespace hp2 {
using namespace godot;
namespace {
constexpr int faceSize = 64;
constexpr int atlasSize = 256;
constexpr float reflectionFar = 32.0f;

void add_patch(SubViewport& atlas, SubViewport& source,
    const EnvironmentMap& environment, std::size_t index, bool back) {
    PackedVector2Array positions, uv;
    PackedInt32Array triangles;
    for (std::size_t row = 0; row < 6; ++row) {
        for (std::size_t column = 0; column < 6; ++column) {
            const auto& sample = back ? environment.BackSample(index, row, column) :
                environment.Sample(index, row, column);
            positions.push_back(Vector2(sample.mappedST[0] * atlasSize, sample.mappedST[1] * atlasSize).floor());
            uv.push_back({sample.faceST[0], sample.faceST[1]});
        }
    }
    for (int row = 0; row < 5; ++row) {
        for (int column = 0; column < 5; ++column) {
            const int a = row * 6 + column;
            for (const int vertex : {a, a + 6, a + 1, a + 1, a + 6, a + 7}) triangles.push_back(vertex);
        }
    }
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = positions;
    arrays[Mesh::ARRAY_TEX_UV] = uv;
    arrays[Mesh::ARRAY_INDEX] = triangles;
    Ref<ArrayMesh> mesh;
    mesh.instantiate();
    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    Ref<ShaderMaterial> material;
    material.instantiate();
    material->set_shader(load_shader("res://shader/car_environment_remap.gdshader"));
    auto* node = memnew(MeshInstance2D);
    atlas.add_child(node);
    node->set_mesh(mesh);
    node->set_texture(source.get_texture());
    node->set_material(material);
}
}

struct VehicleReflection::Storage {
    // These source records belong to the presentation adapter. EnvironmentMap
    // supplies the authored schedule and projection, and performs no drawing.
    GameTime clock{};
    CarCinematicServices views{};
    std::array<EcstasyRenderTarget, 15> targets{};
    std::unique_ptr<EnvironmentMap> environment;
    std::array<SubViewport*, 6> faces{};
    std::array<Camera3D*, 6> cameras{};
    SubViewport* atlas{};
    CarPaintAssets paint;
    std::unique_ptr<TrackSceneryLod> track;

    Storage(Node3D& scene, const RaceStateView& race, bool sharedWorld) {
        views.clock = &clock;
        views.presentationLayout = race.rearCamera ? 2 : 1;
        gameTimeSetVideoMode(&clock, race.ntsc ? HP2_VIDEO_NTSC : HP2_VIDEO_PAL);
        for (std::size_t i = 8; i < targets.size(); ++i) {
            auto& target = targets[i];
            target.id = static_cast<int>(i);
            target.width = target.height = i == 14 ? atlasSize : faceSize;
            target.format = 0x32;
            target.opaqueOnly = i != 14;
            target.enabled = 1;
        }
        environment = std::make_unique<EnvironmentMap>(views, targets.data(), targets.size());
        atlas = memnew(SubViewport);
        atlas->set_name("CarReflectionAtlas");
        atlas->set_size({atlasSize, atlasSize});
        atlas->set_disable_3d(true);
        atlas->set_use_hdr_2d(true);
        atlas->set_transparent_background(true);
        scene.add_child(atlas);
        Ref<World3D> world = scene.get_world_3d();
        if (!sharedWorld) world.instantiate();
        for (std::size_t i = 0; i < faces.size(); ++i) {
            faces[i] = memnew(SubViewport);
            faces[i]->set_name(String("CarReflectionFace") + String::num_uint64(i));
            faces[i]->set_size({faceSize, faceSize});
            faces[i]->set_use_hdr_2d(true);
            faces[i]->set_world_3d(world);
            faces[i]->set_update_mode(SubViewport::UPDATE_DISABLED);
            atlas->add_child(faces[i]);
            auto* camera = memnew(Camera3D);
            faces[i]->add_child(camera);
            camera->set_cull_mask((sharedWorld ? trackReflectionLayer : 1u) | trackSkyLayer);
            camera->make_current();
            cameras[i] = camera;
            if (i != 2) add_patch(*atlas, *faces[i], *environment, i, false);
        }
        if (!sharedWorld) {
            auto* root = memnew(Node3D);
            root->set_name("ReflectionScenery");
            faces[0]->add_child(root);
            track = std::make_unique<TrackSceneryLod>(*root);
        }
        for (std::size_t patch = 0; patch < 4; ++patch)
            add_patch(*atlas, *faces[2], *environment, patch, true);
        paint.specular = upload_paint_texture(race.renderResources->SpecialTexture(6));
        paint.reflection = atlas->get_texture();
    }
};

VehicleReflection::VehicleReflection(Node3D& scene, const RaceStateView& race, bool sharedWorld)
    : storage_(std::make_unique<Storage>(scene, race, sharedWorld)) {}
VehicleReflection::~VehicleReflection() = default;
const CarPaintAssets& VehicleReflection::assets() const { return storage_->paint; }
const float* VehicleReflection::matrix() const { return storage_->environment->BodyMatrix().data(); }
TrackSceneryLod* VehicleReflection::track() const { return storage_->track.get(); }

void VehicleReflection::update(const RaceStateView& race, double elapsed, bool reset) {
    auto& s = *storage_;
    auto& world = *race.simulation->world;
    const auto* players = race.simulation->players;
    const auto* player = players ? players->playersByNumber[0] : nullptr;
    auto* car = player && player->car ? player->car->bindings.owner : world.carTable[0];
    if (!car || !race.camera) return;
    s.views.presentationLayout = race.rearCamera ? 2 : 1;
    const auto* origin = race.reflectionOrigin ? race.reflectionOrigin : car->Movement()->bindings.state->centerPosition;
    if (reset) {
        // Reinitialize all face poses after a discontinuous World restore.
        // Ordinary refreshes continue with the source cadence below.
        for (std::size_t i = 0; i < 6; ++i) {
            s.environment->UpdateCameras(race.camera->eye, origin, 0, 1.0f / (race.ntsc ? 60.0f : 50.0f));
        }
    }
    s.environment->UpdateCameras(race.camera->eye, origin,
        0, static_cast<float>(elapsed));
    float focal{};
    for (std::size_t i = 0; i < s.faces.size(); ++i) {
        const auto& view = s.views.screens[i + 4];
        if (!view.enabled && !reset) continue;
        const auto& frame = view.view->current;
        const auto eye = race_vector(frame.eye);
        const float up[]{frame.matrix[1], frame.matrix[5], frame.matrix[9]};
        s.cameras[i]->look_at_from_position(eye, eye + race_vector(frame.forward), -race_vector(up));
        // PAL113030 passes the short cull far plane. Ecstasy's projection uses
        // a 512-wide focal basis and the face's actual 64-wide render target.
        const auto halfAngle = static_cast<std::uint16_t>(frame.fieldOfView >> 1);
        const float tangent = rotationSineAngle(halfAngle) / vectorCosineAngle(halfAngle);
        focal = 256.0f / tangent;
        const float fov = s.track ? 2.0f * std::atan((faceSize * 0.5f) / focal) * (180.0f / 3.141592654f) :
            frame.fieldOfView * (360.0f / 65536.0f);
        s.cameras[i]->set_perspective(fov, frame.nearClip, reflectionFar);
        s.faces[i]->set_update_mode(SubViewport::UPDATE_ONCE);
    }
    if (focal == 0.0f) return;
    if (s.track) {
        const float slope = (faceSize * 0.5f) / focal;
        const float frustumReach = reflectionFar * std::sqrt(1.0f + 2.0f * slope * slope);
        s.track->update(origin, focal, frustumReach, reset);
    }
    s.atlas->set_update_mode(SubViewport::UPDATE_ONCE);
}
}

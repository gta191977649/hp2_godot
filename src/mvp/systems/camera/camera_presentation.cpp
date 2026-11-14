#include "camera_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "bridge/presentation_snapshot.hpp"
#include "indep/inc/Track.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/aabb.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace hp2 {

using namespace godot;

namespace {
constexpr double idleDelay = 1.5;
constexpr double returnDuration = 0.6;
constexpr double entryDuration = 0.24;
constexpr float mouseSensitivity = 0.0025f;
constexpr float mouseReferenceFov = 80.0f;
constexpr float minPitch = -45.0f * Math_PI / 180.0f;
constexpr float maxPitch = 89.0f * Math_PI / 180.0f;
constexpr float carPitchGain = 0.8f;
constexpr float defaultPitch = 15.0f * Math_PI / 180.0f;
constexpr float defaultDistance = 5.5f;
constexpr float minimumDistance = 0.8f;
constexpr float groundClearance = 0.14f;
constexpr float minimumNear = 0.05f;
constexpr float bodyClearance = 0.18f;
const Vector3 up(0, 1, 0);

void native_point(const Vector3& p, float (&out)[4]) {
    out[0] = p.x;
    out[1] = -p.z;
    out[2] = p.y;
    out[3] = 0;
}

struct TerrainQuery {
    TerrainSurface surface;
    explicit TerrainQuery(TerrainTopology& topology) { terrainSurfaceInitialize(&surface, &topology); }
    ~TerrainQuery() { terrainSurfaceDispose(&surface); }
};
}

void apply_race_camera(godot::Camera3D& camera, const RaceCameraState& state) {
    const auto eye = race_vector(state.eye.data());
    const auto forward = race_vector(state.forward.data());
    // Source view raster is Y-down. Godot's Camera3D up is Y-up.
    const auto up = -race_vector(state.up.data());
    if (forward.cross(up).length_squared() < 0.01f)
        throw std::runtime_error("Primary camera has a degenerate basis");
    camera.set_keep_aspect_mode(godot::Camera3D::KEEP_HEIGHT);
    camera.set_perspective(state.verticalFov, state.nearClip, state.farClip);
    camera.look_at_from_position(eye, eye + forward, up);
}

void RaceOrbitCamera::capture(bool enabled) {
    if (enabled == captured_) return;
    captured_ = enabled;
    auto* input = Input::get_singleton();
    if (enabled) input->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
    else {
        if (input->get_mouse_mode() == Input::MOUSE_MODE_CAPTURED)
            input->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
        reset();
    }
}

void RaceOrbitCamera::reset() {
    motion_ = {};
    active_ = returning_ = hasSafeOffset_ = playerExterior_ = false;
    lastMotion_ = {};
    entryStarted_ = {};
    terrainQueries_ = 0;
    lastOffset_ = {};
}

bool RaceOrbitCamera::mouse_motion(const Vector2& relative) {
    if (!captured_ || Input::get_singleton()->get_mouse_mode() != Input::MOUSE_MODE_CAPTURED ||
        !relative.is_finite() || relative.is_zero_approx()) return false;
    motion_ += relative;
    return true;
}

bool RaceOrbitCamera::apply(Camera3D& camera, const VehicleEntityState& player,
        const AABB& bodyBounds, TerrainTopology* topology, double delta) {
    terrainQueries_ = 0;
    playerExterior_ = false;
    const auto motion = motion_;
    motion_ = {};
    if (!topology || !captured_) {
        active_ = false;
        return false;
    }
    if (!active_ && motion.is_zero_approx()) return false;

    const auto base = camera.get_global_transform();
    const float stockNear = camera.get_near();
    const auto now = std::chrono::steady_clock::now();
    const auto bodyTransform = race_transform(player.transform.data());
    const Vector3 center = bodyTransform.xform(bodyBounds.get_center());
    const auto localBasis = bodyTransform.basis.inverse();
    const auto halfSize = bodyBounds.size * 0.5f;
    const auto bodyExit = [&](const Vector3& axis) {
        const auto local = localBasis.xform(axis).abs();
        return std::min({halfSize.x / std::max(local.x, 0.0001f),
            halfSize.y / std::max(local.y, 0.0001f), halfSize.z / std::max(local.z, 0.0001f)});
    };
    const Vector3 forward = race_vector(player.transform.data());
    const float heading = std::atan2(forward.x, forward.z);
    const auto originalOffset = base.origin - center;
    if (!motion.is_zero_approx()) {
        if (!active_ || returning_) {
            const bool resume = active_ && hasSafeOffset_;
            entryStarted_ = now;
            entryFromStock_ = !resume;
            entryPose_ = resume ? lastPose_ : base;
            entryNear_ = resume ? lastNear_ : stockNear;
            entryPose_.origin -= center;
            const auto seed = resume ? lastOffset_ : originalOffset;
            const bool exterior = seed.length() >= (resume ? minimumDistance : 2.0f) && (resume || player.visible);
            yaw_ = exterior ? std::atan2(seed.x, seed.z) : heading + Math_PI;
            pitch_ = exterior ? std::clamp(static_cast<float>(std::atan2(seed.y,
                std::hypot(seed.x, seed.z))), minPitch, maxPitch) : defaultPitch;
            if (!resume)
                distance_ = exterior ? std::clamp(static_cast<float>(seed.length()), minimumDistance, 9.0f) : defaultDistance;
            clearDistance_ = exterior ? seed.length() : 0.0f;
        }
        active_ = true;
        returning_ = false;
        lastMotion_ = now;
        // SACarCam applies raw relative input directly to the world angles
        // and scales the response by the current field of view.
        const float gain = 2.0f * mouseSensitivity * camera.get_fov() / mouseReferenceFov;
        yaw_ = std::remainder(yaw_ - motion.x * gain, 2.0f * Math_PI);
        // GTA's Alpha raises the forward vector. Our pitch raises the eye
        // on the opposite side of the target, so the vertical sign flips.
        pitch_ = std::clamp(pitch_ - motion.y * gain * carPitchGain, minPitch, maxPitch);
    }

    const auto idle = std::chrono::duration<double>(now - lastMotion_).count();
    const float weight = std::clamp(static_cast<float>((idle - idleDelay) / returnDuration), 0.0f, 1.0f);
    if (weight >= 1.0f) {
        reset();
        return false;
    }
    const bool wasReturning = returning_;
    returning_ = weight > 0;
    const Vector3 direction(std::sin(yaw_) * std::cos(pitch_), std::sin(pitch_),
        std::cos(yaw_) * std::cos(pitch_));
    Vector3 offset = direction * distance_;
    if (returning_ && originalOffset.length_squared() > 0.0001f) {
        // Interpolate azimuth explicitly so opposite views still follow an arc.
        const float targetYaw = std::atan2(originalOffset.x, originalOffset.z);
        const float targetPitch = std::atan2(originalOffset.y, std::hypot(originalOffset.x, originalOffset.z));
        // Unwrap against the previous target to avoid flipping sides when
        // a moving HP2 camera crosses the opposite bearing during return.
        if (!wasReturning) returnYaw_ = yaw_;
        returnYaw_ += std::remainder(targetYaw - returnYaw_, 2.0f * Math_PI);
        const float yaw = Math::lerp(yaw_, returnYaw_, weight);
        const float pitch = Math::lerp(pitch_, targetPitch, weight);
        const float radius = Math::lerp(distance_, static_cast<float>(originalOffset.length()), weight);
        offset = Vector3(std::sin(yaw) * std::cos(pitch), std::sin(pitch),
            std::cos(yaw) * std::cos(pitch)) * radius;
    }

    // The World owns its topology. Release the independent query before the
    // next original tick can replace or unload the track.
    TerrainQuery query(*topology);
    float anchor[4], eye[4];
    native_point(center, anchor);
    const auto viewportSize = camera.get_viewport()->get_visible_rect().size;
    const float aspect = viewportSize.x / std::max(1.0f, static_cast<float>(viewportSize.y));
    const float projectionRadius = std::tan(camera.get_fov() * Math_PI / 360.0f) * std::hypot(aspect, 1.0f);
    const float clearance = std::max(groundClearance, stockNear * projectionRadius);
    const float floorMargin = groundClearance * (1.0f - weight);
    const auto groundAt = [&](const Vector3& point, bool& found) {
        native_point(point, eye);
        query.surface.lastGround = nullptr;
        ++terrainQueries_;
        return terrainSurfaceElevation(&query.surface, eye, nullptr, nullptr, &found);
    };
    Vector3 candidate;
    float candidateGround = 0.0f;
    const auto constrain = [&](const Vector3& arm, float minimumArm = 0.0f, bool entering = false) {
        const float requested = arm.length();
        if (requested < 0.001f) return false;
        const auto axis = arm / requested;
        // GTA tests the volume around the camera arm. Use the original HP2
        // segment query along the center and the four corner rays of that volume.
        const auto cross = axis.cross(up);
        const auto side = cross.length_squared() > 0.0001f ? cross.normalized() :
            Vector3(-std::cos(yaw_), 0.0f, std::sin(yaw_));
        const auto vertical = side.cross(axis);
        const float probeRadius = minimumArm > 0.0f ? minimumNear * projectionRadius : clearance;
        const auto horizontal = side * probeRadius;
        const auto height = vertical * probeRadius;
        float midpoint[4];
        native_point(center + arm * 0.5f, midpoint);
        const float radius = 0.5f * std::max(std::abs(arm.x), std::abs(arm.z)) + 2.0f * clearance;
        terrainSurfaceUpdate(&query.surface, midpoint, radius, nullptr);
        if (query.surface.polygonCount == TERRAIN_SURFACE_POLYGON_LIMIT) return false;
        float available = requested;
        for (const auto& margin : {Vector3(), horizontal + height, horizontal - height,
                                   -horizontal + height, -horizontal - height}) {
            native_point(center + arm + margin, eye);
            ++terrainQueries_;
            if (terrainSurfaceConstrainSegment(&query.surface, 1, true, anchor, eye, groundClearance * 0.5f, 0.0f))
                available = std::min(available, std::max(0.0f, static_cast<float>((race_vector(eye) - center).dot(axis))));
        }
        const float shortest = returning_ || entering ? std::min(minimumDistance, requested) :
            std::max({minimumArm, minimumDistance, bodyExit(axis) + bodyClearance});
        if (available < shortest) return false;
        const float recovering = !entering && clearDistance_ > 0.0f ?
            clearDistance_ + static_cast<float>(delta) * 8.0f : available;
        float distance = std::clamp(recovering, shortest, available);
        const auto aboveGround = [&](float radius) {
            candidate = center + axis * radius;
            bool found = false;
            candidateGround = groundAt(candidate, found);
            return found && candidate.y >= candidateGround + floorMargin - 0.001f;
        };
        if (!aboveGround(distance)) {
            // Keep the requested pitch and solve for the arm length at the floor.
            if (!aboveGround(shortest)) return false;
            float blocked = distance;
            distance = shortest;
            for (int i = 0; i < 6; ++i) {
                const float middle = (distance + blocked) * 0.5f;
                if (aboveGround(middle)) distance = middle;
                else blocked = middle;
            }
            aboveGround(distance);
        }
        if (!entering) clearDistance_ = distance;
        return true;
    };
    if (!constrain(offset)) {
        bool cleared = false;
        if (!returning_ && pitch_ < 0.0f) {
            // The ground closed the arm before it cleared the car. Keep the
            // bearing and place the camera just outside the body at road height.
            const Vector3 bearing(std::sin(yaw_), 0.0f, std::cos(yaw_));
            auto close = center + bearing * (bodyExit(bearing) + bodyClearance);
            bool found = false;
            const float floor = groundAt(close, found);
            close.y = std::max(close.y + std::tan(pitch_) * close.distance_to(center), floor + floorMargin);
            const auto arm = close - center;
            if (found) cleared = constrain(arm, arm.length() - 0.001f);
        }
        if (!cleared && (!hasSafeOffset_ || !constrain(lastOffset_))) return false;
    }

    const auto orbitOffset = candidate - center;
    const float bodyGap = orbitOffset.length() - bodyExit(orbitOffset.normalized());
    const Vector3 viewUp = std::abs(orbitOffset.normalized().dot(up)) < 0.9999f ? up :
        Vector3(std::sin(yaw_), 0.0f, std::cos(yaw_));
    const auto orbit = Transform3D(Basis(), candidate).looking_at(center, viewUp);
    const auto basis = orbit.basis.get_rotation_quaternion().slerp(base.basis.get_rotation_quaternion(), weight);
    // Preserve the nearby body and keep the near plane clear of the road.
    const float closeNear = std::max(minimumNear, std::min(bodyGap * 0.75f,
        (candidate.y - candidateGround) / std::max(1.0f, projectionRadius)));
    auto pose = Transform3D(Basis(basis), candidate);
    float near = Math::lerp(std::min(stockNear, closeNear), stockNear, weight);
    const float entryTime = std::chrono::duration<float>(now - entryStarted_).count();
    if (entryTime < entryDuration) {
        const float t = entryTime / entryDuration;
        const float amount = t * t * (3.0f - 2.0f * t);
        auto start = entryFromStock_ ? base : entryPose_;
        if (!entryFromStock_) start.origin += center;
        pose = start.interpolate_with(pose, amount);
        const auto startArm = start.origin - center;
        const auto endArm = orbitOffset;
        if (startArm.length_squared() > 0.01f) {
            const auto azimuth = [](const Vector3& arm) { return std::atan2(arm.x, arm.z); };
            const auto elevation = [](const Vector3& arm) { return std::atan2(arm.y, std::hypot(arm.x, arm.z)); };
            const auto startYaw = azimuth(startArm);
            const auto yaw = startYaw + std::remainder(azimuth(endArm) - startYaw, 2.0f * Math_PI) * amount;
            const auto pitch = Math::lerp(elevation(startArm), elevation(endArm), amount);
            const auto radius = Math::lerp(startArm.length(), endArm.length(), amount);
            pose.origin = center + Vector3(std::sin(yaw) * std::cos(pitch), std::sin(pitch),
                std::cos(yaw) * std::cos(pitch)) * radius;
        }
        if (amount > 0.0f && constrain(pose.origin - center, 0.0f, true)) pose.origin = candidate;
        else pose = start;
        near = Math::lerp(entryNear_, near, amount);
    }
    lastOffset_ = pose.origin - center;
    lastPose_ = pose;
    lastNear_ = near;
    hasSafeOffset_ = true;
    const auto arm = lastOffset_.normalized();
    playerExterior_ = lastOffset_.length() - bodyExit(arm) >= bodyClearance * 0.95f;
    camera.set_global_transform(pose);
    camera.set_near(near);
    return true;
}

}

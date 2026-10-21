#pragma once
// Host storage for original six-view capture and the single 2D reflection atlas.
#include "game_core/port/track/environment.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <functional>
#include <optional>
#include <string>
#include <memory>
#include <godot_cpp/variant/array.hpp>
namespace port::godot_bridge {
class TrackReflectionRenderer {
public:
    struct CaptureView {
        unsigned face_index=0,view_id=4;
        std::uint32_t host_layer=2;
        godot::SubViewport* viewport=nullptr;
        godot::Node3D* scene_root=nullptr;
        godot::Camera3D* camera=nullptr;
        port::track::EnvironmentFace native;
        std::uint32_t exclude_flags=0x11a;
    };
    using SubmitView=std::function<bool(const CaptureView&,std::string&)>;
    TrackReflectionRenderer(godot::Node* owner,SubmitView);
    ~TrackReflectionRenderer();
    TrackReflectionRenderer(const TrackReflectionRenderer&)=delete;
    TrackReflectionRenderer& operator=(const TrackReflectionRenderer&)=delete;
    bool update(const port::track::EnvironmentVector& viewer,
        const std::optional<port::track::EnvironmentVector>& original_environment_origin,
        std::uint32_t frame_counter,float original_capture_focal_h,int original_view_mode=1,bool in_game=true,bool backwards=false);
    void clear();
    bool ready()const;
 godot::Array scheduled_viewports()const;
    const std::string& diagnostic()const;
    const port::track::EnvironmentMatrix& hack_matrix()const;
    godot::Ref<godot::Texture2D> texture()const;
    // Front and back are primitive groups in the SAME original target14 atlas.
    godot::Ref<godot::Texture2D> front_texture()const{return texture();}
    godot::Ref<godot::Texture2D> back_texture()const{return texture();}
    godot::Ref<godot::Texture2D> face_texture(unsigned)const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}

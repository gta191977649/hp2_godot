#include "driving_particle_display.hpp"
#include "ps2_driving_runtime.hpp"
#include "platform/ps2_asset_root.hpp"
#include "game_core/port/track/settings.hpp"
#include "game_core/port/track/texture_state.hpp"
#include "port/assets/catalog.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <cstring>
#include <numeric>
namespace port::godot_bridge {
namespace {
godot::Ref<godot::Texture2D> upload(const hp2::assets::DecodedTexture& bitmap){
    godot::PackedByteArray pixels;pixels.resize(bitmap.rgba.size());
    std::memcpy(pixels.ptrw(),bitmap.rgba.data(),bitmap.rgba.size());
    return godot::ImageTexture::create_from_image(godot::Image::create_from_data(bitmap.width,bitmap.height,false,godot::Image::FORMAT_RGBA8,pixels));
}
}
godot::Dictionary DrivingParticleDisplay::present(Ps2DrivingRuntime& source,godot::Node3D& owner,godot::Camera3D& camera){
    if(owner_&&owner_!=&owner)throw std::logic_error("Particle display owner changed without clear");
    if(!renderer_){
        const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");
        const auto root=hp2::platform::resolve_zzdata_root(start.utf8().get_data());
        if(!root)throw std::runtime_error("Original wheel particle assets are not configured");
        port::assets::DirectorySource assets(*root);auto scene=std::make_shared<port::track::Scene>();
        port::track::append_particle_library(*scene,assets);
        auto math=port::driving::read_debug_image(port::driving::debug_image_path(start.utf8().get_data()));
        for(const auto& texture:scene->textures){auto& levels=textures_[texture.header.tex_hash];levels.push_back(upload(texture.bitmap));
            if(const auto found=scene->mipmaps.find(texture.header.tex_hash);found!=scene->mipmaps.end())for(const auto& bitmap:found->second)levels.push_back(upload(bitmap));}
        auto renderer=std::make_unique<TrackParticleRenderer>(owner,[this](auto hash){return textures_.at(hash).at(0);},[this](auto hash,unsigned level){return textures_.at(hash).at(level);});
        renderer->set_mesh_reuse(true);renderer->set_driving_source(&source);
        scene_=std::move(scene);math_=std::move(math);renderer_=std::move(renderer);owner_=&owner;
    }
    port::track::FrameView view;const auto position=camera.get_global_position(),direction=-camera.get_global_basis().get_column(2);
    view.position={{float(position.z),float(position.x),float(position.y)}};
    view.direction={{float(direction.z),float(direction.x),float(direction.y)}};
    const auto projection=camera.get_camera_projection();
    const auto clip=projection*godot::Projection(camera.get_global_transform().affine_inverse());
    constexpr unsigned columns[]{2,0,1,3};
    for(unsigned b=0;b<4;++b)for(unsigned r=0;r<4;++r)view.clip_matrix[b*4+r]=float(clip.columns[columns[b]][r]);
    view.focal_h=float(projection.columns[0].x)*256.f;view.near_z=float(camera.get_near());view.far_z=float(camera.get_far());
    // Synthetic floor has no GS depth target. Use the ordinary host depth test
    // with the same recovered particle packet/depth projection, without inventing
    // native terrain depth. Real tracks always require their original depth source.
    renderer_->abort_frame(); // Discard any previous failed host submission.
    const auto frame=renderer_->update(scene_,math_,view,0,{},port::track::shared_render_settings().enabled(port::track::EffectId::Particles),1,&camera,false);
    const auto order=port::track::alpha_order_001055B0(frame.alpha_keys);std::vector<std::size_t> ranks(order.size());
    for(std::size_t i=0;i<order.size();++i)ranks[order[i]]=i;
    renderer_->set_alpha_ranks(ranks,ranks.size(),float((owner.get_global_position()-position).dot(direction)));
    renderer_->commit_frame();
    godot::Dictionary result;result["ok"]=true;result["active"]=int64_t(frame.active);result["visible"]=int64_t(frame.visible);result["unsupported_quads"]=int64_t(frame.unsupported_quads);
    godot::Array gaps;for(const auto& gap:frame.gaps)gaps.push_back(godot::String::utf8(gap.c_str()));result["gaps"]=gaps;result["stats"]=renderer_->stats();return result;
}
}

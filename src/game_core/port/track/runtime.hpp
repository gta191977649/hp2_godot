#pragma once
#include "assets.hpp"
#include "effects.hpp"
#include "game_core/port/driving/memory.hpp"
#include <memory>
#include <string>
namespace port::track {
// Port-owned immutable resources and frame values; native XYZ, column-major matrices.
struct FrameView {
    std::array<float,3> position{},direction{{1,0,0}};
    std::array<float,16> clip_matrix{}; // native world -> homogeneous clip (-w..w)
    float focal_h=1; // horizontal pixel focal length, original eView::H
    std::uint32_t exclude_flags=0x18; // ordinary single-view, forward direction
};
struct DrawCommand {
    std::size_t instance=0,solid=0,surface=0;
    unsigned lod=0,visibility=0;
    std::array<float,16> matrix{};
    std::array<float,28> light_context{};
    bool identity=false;
    bool has_light_context=true;
    std::size_t original_order=0;
    float depth=0; // native camera-forward distance; canonical strip sorting refines this
    EffectState effect;
};
struct Frame {
    std::vector<DrawCommand> commands;
    std::vector<int> sections;
    int zone=-1;
    std::size_t considered_instances=0,visible_instances=0;
    std::size_t submitted_surfaces=0; // original raw submit order before strip eligibility
};
class Runtime {
public:
    void load(std::shared_ptr<const Scene>);
    void load(assets::Source&,const Entry&);
    bool poll_load()const{return ready();} // synchronous native completion; host owns async scheduling
    void unload();
    bool ready()const{return bool(scene_);}
    const std::shared_ptr<const Scene>& resources()const{return scene_;}
    Frame prepare_frame(const FrameView&,port::driving::Memory& original_math);
    std::string diagnostics()const;
private:
    std::shared_ptr<const Scene> scene_;
    std::vector<std::size_t> zone_order_;
    std::vector<std::uint32_t> texture_chain_order_;
};
}

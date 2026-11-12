#pragma once
// Port-owned production lifetime for the recovered single-player selection page.
#include "port/render/frontend_packet.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <memory>
#include <map>
#include <string>
namespace port::runtime {
class FrontEndRuntime;
struct SelectionObservation {
    bool active=false,loaded=false;
    int car=-1,skin=-1,visible_cars=0,overlay=130;
    std::uint64_t generation=0;
    double load_ms=0;
    std::uint64_t cache_hits=0;
};
class SelectionRuntime {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    explicit SelectionRuntime(FrontEndRuntime&);
    ~SelectionRuntime();
    void activate();
    void tick(float elapsed);
    bool input(int port,int event,bool pressed);
    void close();
    SelectionObservation observation()const;
    port::render::FrontendPacket packet(const port::render::Frame&,port::render::Resources&);
    std::map<std::uint32_t,std::string> ui_targets(const hp2::frontend::FeScreen* document=nullptr)const;
    bool ui_choose(const std::string& target);
    float layer_depth(int layer)const;
    void after_display();
    bool popup_active()const;
    std::vector<hp2::frontend::FeScreen*> popup_documents()const;
    const hp2::frontend::FeWidget* ghost_target(const hp2::frontend::FeWidget&)const;
};
}

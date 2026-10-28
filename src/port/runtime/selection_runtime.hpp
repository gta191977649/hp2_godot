#pragma once
// Port-owned production lifetime for the recovered single-player selection page.
#include "port/render/frontend_packet.hpp"
#include <memory>
namespace port::runtime {
class FrontEndRuntime;
struct SelectionObservation {
    bool active=false,loaded=false;
    int car=-1,skin=-1,visible_cars=0,overlay=130;
    std::uint64_t generation=0;
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
    void after_display();
};
}

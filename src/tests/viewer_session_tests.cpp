#include "game_core/extensions/viewer_session.hpp"
#include <stdexcept>
#include <iostream>
using namespace port::extensions;
void check(bool value) {if(!value)throw std::runtime_error("Viewer session assertion failed");}
template<class Body>void rejects(Body body) {bool failed=false;try{body();}catch(const std::exception&){failed=true;}check(failed);}
int main() {
    ViewerSession session(true);
    session.request(ViewerAction::Car);session.request(ViewerAction::Load,"CARS/synthetic test.bin");
    check(session.state().kind==ViewerKind::Frontend); // Enqueue must not switch scenes in a callback.
    check(session.next()->action==ViewerAction::Car);
    session.activate(ViewerKind::Car);session.apply(*session.next());
    check(session.state().status=="Loading"&&!session.state().fixture);
    session.loaded(true,"", "parts=3 triangles=100");
    check(session.state().status=="Loaded" && session.describe().find("triangles=100")!=std::string::npos);
    rejects([&]{session.loaded(true,"","");});
    session.apply({ViewerAction::Load,"CARS/missing/GEOMETRY.BIN"});
    session.loaded(false,"Could not open original asset", "ignored");
    check(session.state().status=="Failed"&&!session.state().fixture&&session.state().rendering.empty());
    check(session.state().source=="CARS/missing/GEOMETRY.BIN");check(!session.next());
    session.camera("synthetic orbit");check(session.describe().find("synthetic orbit")!=std::string::npos);
    session.apply({ViewerAction::Clear,{}});check(!session.state().fixture&&session.state().source.empty());
    session.apply({ViewerAction::Fixture,{}});check(session.state().fixture);
    session.activate(ViewerKind::Track);check(session.state().source.empty());
    session.apply({ViewerAction::Load,"TRACKS/missing.bin"});
    check(session.state().status=="Not implemented"&&session.state().fixture);
    session.activate(ViewerKind::Frontend);rejects([&]{session.apply({ViewerAction::Reset,{}});});
    rejects([&]{session.request(ViewerAction::Load,"../invalid");});
    rejects([&]{session.request(ViewerAction::Load,"D:/invalid");});
    rejects([&]{parse_viewer_action("unknown");});
    ViewerSession release(false);rejects([&]{release.request(ViewerAction::Car);});
    rejects([&]{release.activate(ViewerKind::Track);});
    std::cout<<"VIEWER_SESSION_OK\n";
}

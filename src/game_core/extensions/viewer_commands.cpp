#include "viewer_session.hpp"
#include <csys/system.h>
#include <exception>
namespace port::extensions {
void register_viewer_commands(csys::System& system,ViewerSession& session) {
    auto enqueue=[&system,&session](ViewerAction action,std::string source={}) {
        try {session.request(action,std::move(source));}
        catch(const std::exception& error) {system.Log(csys::ERROR)<<error.what()<<csys::endl;}
    };
    system.RegisterCommand("drivinglab","Open the single-car driving recovery laboratory",[enqueue] {enqueue(ViewerAction::Driving);});
    system.RegisterCommand("carviewer","Open CarViewer for original vehicle BIN resources",[enqueue] {enqueue(ViewerAction::Car);});
    system.RegisterCommand("trackviewer","Open the synthetic TrackViewer scaffold",[enqueue] {enqueue(ViewerAction::Track);});
    system.RegisterCommand("viewer.close","Return to a newly initialized frontend",[enqueue] {enqueue(ViewerAction::Close);});
    system.RegisterCommand("viewer.load","Load a vehicle GEOMETRY.BIN; TrackViewer loading remains unavailable",
        [enqueue](const csys::String& source) {enqueue(ViewerAction::Load,source.m_String);},csys::Arg<csys::String>("relative-path"));
    system.RegisterCommand("viewer.clear","Clear the active viewer",[enqueue] {enqueue(ViewerAction::Clear);});
    system.RegisterCommand("viewer.fixture","Restore synthetic test geometry",[enqueue] {enqueue(ViewerAction::Fixture);});
    system.RegisterCommand("viewer.reset","Reset the active viewer camera",[enqueue] {enqueue(ViewerAction::Reset);});
    system.RegisterCommand("viewer.status","Show the active viewer, request and camera state",[&system,&session] {
        system.Log()<<session.describe()<<csys::endl;
    });
}
}

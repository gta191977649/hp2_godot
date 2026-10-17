#include "viewer_session.hpp"
#include <algorithm>
#include <stdexcept>
#include <sstream>
namespace port::extensions {
const char* viewer_name(ViewerKind kind) {
    switch(kind) {
    case ViewerKind::Frontend: return "frontend";
    case ViewerKind::Car: return "car";
    case ViewerKind::Track: return "track";
    case ViewerKind::Driving: return "driving";
    }
    throw std::invalid_argument("Unknown viewer kind");
}
const char* action_name(ViewerAction action) {
    switch(action) {
    case ViewerAction::Car:return "car"; case ViewerAction::Track:return "track";
    case ViewerAction::Driving:return "driving";
    case ViewerAction::Close:return "close";case ViewerAction::Load:return "load";
    case ViewerAction::Clear:return "clear";case ViewerAction::Fixture:return "fixture";
    case ViewerAction::Reset:return "reset";
    }
    throw std::invalid_argument("Unknown viewer action");
}
ViewerAction parse_viewer_action(const std::string& action) {
    for(auto value:{ViewerAction::Car,ViewerAction::Track,ViewerAction::Driving,ViewerAction::Close,ViewerAction::Load,
                   ViewerAction::Clear,ViewerAction::Fixture,ViewerAction::Reset})
        if(action==action_name(value))return value;
    throw std::invalid_argument("Unknown viewer action: "+action);
}
ViewerKind parse_viewer_kind(const std::string& kind) {
    for(auto value:{ViewerKind::Frontend,ViewerKind::Car,ViewerKind::Track,ViewerKind::Driving})
        if(kind==viewer_name(value))return value;
    throw std::invalid_argument("Unknown viewer kind: "+kind);
}
void ViewerSession::request(ViewerAction action,std::string source) {
    if(!enabled_)throw std::logic_error("Debug viewers are disabled in Release builds");
    action_name(action); // Reject invalid enum values as well as unknown parsed names.
    if(action==ViewerAction::Load) {
        std::replace(source.begin(),source.end(),'\\','/');
        if(source.empty()||source.front()=='/'||source.find(':')!=std::string::npos)
            throw std::invalid_argument("Expected a non-empty ZZDATA-relative path");
        std::istringstream parts(source);std::string part;
        while(std::getline(parts,part,'/'))if(part=="..")
            throw std::invalid_argument("Resource path must stay relative to ZZDATA");
    }
    pending_.push_back({action,std::move(source)});
}
std::optional<ViewerRequest> ViewerSession::next() {
    if(pending_.empty())return {};
    auto result=std::move(pending_.front());pending_.pop_front();return result;
}
void ViewerSession::activate(ViewerKind kind) {
    if(!enabled_&&kind!=ViewerKind::Frontend)
        throw std::logic_error("Debug viewers are disabled in Release builds");
    viewer_name(kind);
    state_={};state_.kind=kind;
    if(kind!=ViewerKind::Frontend) {state_.fixture=true;state_.status="Synthetic fixture";}
}
void ViewerSession::apply(const ViewerRequest& request) {
    if(!enabled_)throw std::logic_error("Debug viewers are disabled in Release builds");
    if(state_.kind==ViewerKind::Frontend)
        throw std::logic_error("Open CarViewer or TrackViewer first");
    switch(request.action) {
    case ViewerAction::Load:
        state_.rendering.clear();
        if(state_.kind==ViewerKind::Car) {
            state_.source=request.source;state_.status="Loading";
            state_.diagnostic.clear();state_.fixture=false;break;
        }
        state_.source=request.source;state_.status="Not implemented";
        state_.diagnostic="Loader not implemented; no resource was read or loaded";
        break;
    case ViewerAction::Clear:
        state_.rendering.clear();
        state_.source.clear();state_.status="Empty";state_.diagnostic.clear();state_.fixture=false;break;
    case ViewerAction::Fixture:
        state_.rendering.clear();
        state_.source.clear();state_.status="Synthetic fixture";state_.diagnostic.clear();state_.fixture=true;break;
    case ViewerAction::Reset:break; // Camera reset belongs to the host, not a recovered model.
    default:throw std::invalid_argument("Scene transitions must be handled by the host");
    }
}
void ViewerSession::loaded(bool success,std::string diagnostic,std::string rendering) {
    if(!enabled_||state_.kind!=ViewerKind::Car||state_.status!="Loading")
        throw std::logic_error("No car load is pending");
    state_.status=success?"Loaded":"Failed";state_.fixture=false;
    state_.diagnostic=std::move(diagnostic);
    state_.rendering=success?std::move(rendering):std::string{};
}
std::string ViewerSession::describe() const {
    return std::string("viewer=")+viewer_name(state_.kind)+" source="+(state_.source.empty()?"(none)":state_.source)
        +" status="+state_.status+" fixture="+(state_.fixture?"synthetic":"none")
        +" camera="+(state_.camera.empty()?"(none)":state_.camera)
        +(state_.rendering.empty()?"":" rendering="+state_.rendering)
        +(state_.diagnostic.empty()?"":" diagnostic="+state_.diagnostic);
}
}

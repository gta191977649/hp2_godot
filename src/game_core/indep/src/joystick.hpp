// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class Joystick {
public:
    // ELF 0x00153C20; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00153c20.md.
    // Original Debug reference: Joystick::FindEventNode(JoyEvent); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FindEventNode();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class QueuedJoystickEvents {
public:
    // ELF 0x00153A00; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00153a00.md.
    // Original Debug reference: QueuedJoystickEvents::AddEvent(int, unsigned short, EventNode *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void AddEvent();
    // ELF 0x00153A50; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00153a50.md.
    // Original Debug reference: QueuedJoystickEvents::RemoveEvent(int, JoyEvent); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void RemoveEvent();
};
} // namespace hp2::recovered

// TASK-0099 E-0099-W62. Original input-list/static component, not a physical
// device poll/scanner binding. Debug authority SHA256
//815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
void Sub_00155228(::port::driving::Memory&);
class Joystick:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 std::uint32_t FindEventNode(int event)const; //00153870.
 void ConnectEventNodeConfig(std::uint32_t event_node); //001538A0,E-0099-W77.
};
class QueuedJoystickEvents:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void RemoveEvent(int port,int event); //001536A0.
};
int IsJoyHandlerAdded(::port::driving::Memory&,int port,int event); //00153EC8.
void RemoveJoyHandler(::port::driving::Memory&,int port,int event); //00153E38, active node destruction explicit.
// E-0099-W77. Raw original callback addresses are stored, not host callbacks.
void AddJoyHandler(::port::driving::Memory&,int port,int event,std::uint32_t handler,std::uint32_t handler_object); //00153D90;00153E30 thunk.
int DetermineAvailableJoystickConfigurations(::port::driving::Memory&,std::uint32_t configuration_numbers,int max_configuration_numbers,int joystick_type); //00154BF8.
int ChooseJoystickConfigurationNumber(::port::driving::Memory&,int joystick_type,int user_configuration_number); //00154C88.
std::uint32_t FindScannerConfig(::port::driving::Memory&,int event,int joystick_type,int user_configuration_number); //00154CF0.
}

namespace hp2::debug {void JoystickDoSnapshot(::port::driving::Memory&,std::uint32_t snapshot); /*001540B0,E-0099-W74*/}

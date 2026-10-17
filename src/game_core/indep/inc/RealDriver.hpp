// TASK-0046: typed members below target Debug815b60c7...332f1b, E-0046-C01.
// Historical Retail template provenance/gaps remain explicitly address-labelled;
// they do not describe the typed Debug implementation or its PS2 ABI.
// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/RealDriver.hpp.
// TASK-0046 typed player entries use Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Unrelated historical Retail gaps are separately documented below.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/driving/player_services.hpp"
#include "CarDriver.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class RealDriver : public CarDriver {
public:
    using CarDriver::CarDriver;
    void Sub_0013AB98(std::uint32_t car); // Debug constructor; E-0046-C01
    // Debug0013B198,0013B200,0013B438,0013B948; E-0046-C01.
    void SetManualTransmission(int type);
    void Drive(float t,port::driving::PlayerServices&);
    void DoSteering(float t,int port,port::driving::PlayerServices&);
    void DoThrottleBrakeReverse(float t,port::driving::PlayerServices&);
    // Debug0013BC90/0013BD70; original static handler bound to this PS2 address.
    void JoyHandler_ShiftUp(int port,int event,std::uint8_t param);
    void JoyHandler_ShiftDown(int port,int event,std::uint8_t param);
    void DisableStartLineState(); // Debug0013B168
    // ELF 0x00130958; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00130958.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00130958();
    // ELF 0x00136C58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c58.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136C58();
    // ELF 0x00136C60; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c60.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136C60();
    // ELF 0x00136CE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136ce8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136CE8();
    // ELF 0x00136CF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136cf0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136CF0();
    // ELF 0x0013AD90; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013ad90.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0013AD90();
    // ELF 0x0013AE58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013ae58.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0013AE58();
    // ELF 0x0013AEB8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013aeb8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: RealDriver::DoSnapshot(ReplaySnapshot *); E-0041-V01; retail ABI unconfirmed.
    void DoSnapshot();
    // ELF 0x0013B080; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013b080.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: RealDriver::AddJoyHandlers(void); E-0041-V01; retail ABI unconfirmed.
    void AddJoyHandlers();
    // ELF 0x0013B250; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013b250.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: RealDriver::RemoveJoyHandlers(void); E-0041-V01; retail ABI unconfirmed.
    void RemoveJoyHandlers();
    // Debug0013B1D8; E-0046-C01.
    void SetLowGear();
    // ELF 0x0013C420; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013c420.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: RealDriver::StartHonkHorn(void); E-0041-V01; retail ABI unconfirmed.
    void StartHonkHorn();
    // ELF 0x0013C460; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013c460.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: RealDriver::StopHonkHorn(void); E-0041-V01; retail ABI unconfirmed.
    void StopHonkHorn();
    // ELF 0x0013CAE0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013cae0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: RealDriver::GetDriverNumber(void); E-0041-V01; retail ABI unconfirmed.
    void GetDriverNumber();
    // ELF 0x0013CAF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013caf0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: RealDriver::SetDesiredSpeed(float, float, bool); E-0041-V01; retail ABI unconfirmed.
    void SetDesiredSpeed();
    // ELF 0x002A0550; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/002a0550.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_002A0550();

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // Debug0013B368; E-0046-C01.
    void ResetJoyValues();
};
} // namespace hp2::recovered

namespace hp2::recovered {
void Sub_0013C750(port::driving::Memory&); // Debug global table constructor; E-0046-C01
void MaybeAddTrackRoutePoint(port::driving::Memory&,std::uint32_t car); // Debug001C2ED0
void RealDriverAddNearbyDebugTopology(port::driving::Memory&,std::uint32_t car); // Debug001FEE80
}

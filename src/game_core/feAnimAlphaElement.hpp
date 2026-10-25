// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/indep/src/fmenus.hpp"

namespace hp2::debug {
// TASK-0098 E-0098-U03; decoded host ownership, not original intrusive-list ABI.
// The bound screen/widget storage must outlive these animations.
struct feAnimAlphaElement {
    hp2::frontend::FeWidget* Object=nullptr;
    std::array<float,4> SavedAlphas{};
    explicit feAnimAlphaElement(hp2::frontend::FeWidget*);
    ~feAnimAlphaElement();
    feAnimAlphaElement(const feAnimAlphaElement&)=delete;
    feAnimAlphaElement& operator=(const feAnimAlphaElement&)=delete;
    void SetAlphaScale(float);
};
struct feAlphaAnimation {
    hp2::frontend::FeScreen& ScreenHash; // semantic screen binding, not a PS2 integer overlay
    std::vector<std::unique_ptr<feAnimAlphaElement>> Fading;
    explicit feAlphaAnimation(hp2::frontend::FeScreen&);
    feAlphaAnimation(hp2::frontend::FeScreen&,std::uint32_t group_hash);
    ~feAlphaAnimation();
    void AddObject(std::uint32_t);
    void SetAlphaScale(float);
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class feAnimAlphaElement {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

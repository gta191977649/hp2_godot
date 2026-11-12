#pragma once
// TASK-0110 E-0110-A01. Port numerical carrier, not an original helper or a
// complete EE/VU emulator. Sony's EE manual specifies fixed truncation; the
// captured six-point Debug002171D8 call matches all24 output words under this
// scoped host mode, while RNE differs in12. General EE low-bit differences,
// exceptional encodings and other callers are not established by that sample.
#include "game_core/port/not_implemented.hpp"
#include <cfenv>
#include <exception>

namespace port::driving {
class EeTruncationScope {
    int prior_;
public:
    explicit EeTruncationScope(const char* instruction_address)
        : prior_(std::fegetround()) {
        if(prior_==-1||std::fesetround(FE_TOWARDZERO)!=0)
            hp2::not_implemented(instruction_address,"Host cannot select the scoped truncating EE numerical carrier");
    }
    ~EeTruncationScope() noexcept {
        // A valid previous mode must remain restorable even during a source
        // guard unwind. Continuing with another caller's rounding is unsafe.
        if(std::fesetround(prior_)!=0)std::terminate();
    }
    EeTruncationScope(const EeTruncationScope&)=delete;
    EeTruncationScope& operator=(const EeTruncationScope&)=delete;
};
}

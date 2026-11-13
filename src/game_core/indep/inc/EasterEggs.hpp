#ifndef NFSHP2_INDEP_INC_EASTEREGGS_HPP
#define NFSHP2_INDEP_INC_EASTEREGGS_HPP

#include "indep/inc/CarDriver.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace hp2 {

// Typed final-PAL authored data, not a serialized class representation.
struct EasterEggCode {
    std::array<std::uint32_t,8> buttons;
    std::uint32_t kind;
    std::uint32_t selector;
    std::uint32_t exclusiveGroup;
    std::uint32_t languageMessageId;
    std::uint32_t unlocked;
    std::uint32_t persistent;
};

class EasterEggs final {
public:
    explicit EasterEggs(GameInputRouter* input = nullptr) noexcept;
    EasterEggs(const EasterEggs&) = delete;
    EasterEggs& operator=(const EasterEggs&) = delete;
    EasterEggs(EasterEggs&&) = delete;
    EasterEggs& operator=(EasterEggs&&) = delete;

    GameInputResult Activate();
    GameInputResult UnActivate();
    void JoyHandler(std::int32_t device,std::int32_t event,std::uint8_t value);
    // PC-only six-character payload for Alt+HP2C01..Alt+HP2C78. The native
    // input owner handles Alt/text capture; original DS2 event words, gates
    // and unlock groups remain authoritative. The all-zero record06 is rejected.
    bool TryUnlockTextCode(const char* text,std::size_t length) noexcept;
    void ClearButtons() noexcept;
    // Native descriptive name for the final-only24F700 tail. No beta
    // ClearSpecial/DisableRenderEggs identity or additional acceptance.
    void ResetNonPersistentUnlocks() noexcept;
    std::uint32_t IsTrackUnlocked(std::uint32_t track,bool reverse) const noexcept;
    std::uint32_t IsCarUnlocked(std::uint32_t model,bool pursuit) const noexcept;
    std::uint32_t IsEasterEggUnlocked(std::uint32_t kind,std::uint32_t selector) const noexcept;

    // The source's global record pointer aliases one mutable authored table.
    // EventInfo and other consumers borrow these same records, never a second
    // unlock bank. Router registrations must be removed before owner teardown.
    GameInputRouter* input;
    EasterEggCode* records;
    std::size_t recordCount;
    std::uint32_t active{};
    // The PAL menu temporarily sets active=0 while Help is visible without
    // unregistering the sixteen input events. Keep registration ownership
    // independent so rebuilding menus cannot append duplicate scanner nodes.
    bool registrationsAttached{};
    std::uint32_t pendingMessage{};
    std::uint32_t lastMessage{};
    std::array<std::uint32_t,8> buttons{};
    std::uint32_t buttonCount{};
    std::uint32_t ringStart{};
private:
    bool UnlockRecord(std::size_t index) noexcept;
};

extern EasterEggs easterEggs;

} // namespace hp2
#endif

#pragma once
#include "FMenus.hpp"
#include "bware/inc/bWare.hpp"
#include <list>
#include <exception>
#include <memory>

// Native ownership and containers use explicit PC representations.
namespace hp2 {
class EndGameScreens;
extern EndGameScreens* pEndGameScreens;

class EndGameScreens final {
public:
    EndGameScreens(RaceScreenServices& services,std::int32_t continueType);
    ~EndGameScreens() noexcept;
    std::exception_ptr Close() noexcept;
    EndGameScreens(const EndGameScreens&)=delete;
    EndGameScreens& operator=(const EndGameScreens&)=delete;
    void BuildScreenList();
    const char* GetNextScreen() const;
    const char* GetCurrentScreen() const;
    void RemoveScreen(const char* name);
    void AdvanceScreen();
    bool AtRoot() const;
    void MoveBackScreen();
    void ResumeScreens();
    void SuspendScreens();
    void HandleBaseEvent(ScreenMessage message,ScreenCallbackData& data);
    void AutosaveCompletedCallback();
    bool RequestCoordinatorActionAfterAutosave(std::int32_t kind,std::int32_t argument);
    static void AutosaveCompletedBridge();
    RaceScreenServices& Services() noexcept {return services_;}
    std::int32_t ContinueType() const noexcept {return continueType_;}
    const std::list<const char*>& Screens() const noexcept {return screens_;}
    std::int32_t& ScreenCounter(std::size_t index) {return screenCounters_.at(index);}
private:
    bool RequestAutosave();
    RaceScreenServices& services_;
    std::int32_t continueType_{};
    std::list<const char*> screens_;
    std::list<const char*>::iterator current_;
    bool active_{},suspendPending_{},closed_{};
    std::exception_ptr closeFailure_;
    // Completion may outlive this results owner; the authorised file write
    // is independent of the retired UI/coordinator continuation.
    std::shared_ptr<EndGameScreens*> autosaveOwner_;
    std::array<std::int32_t,11> screenCounters_{};
};
// All thirteen authored EndGameScreens configurations, including both lap
// time displays. Autosave is a MemcardScreens owner. Register every entry in
// the application's existing FlailerScreenStorage collection.
std::array<FlailerScreenConfig,13>& EndGameScreenConfigurations();
void EndGameMenusCallback(ScreenMessage message,ScreenCallbackData* data);
void LapTimesDisplayCallback(ScreenMessage message,ScreenCallbackData* data);

// PAL 1E25E8: the header bridge loads the one canonical manager and
// tail-calls AutosaveCompletedCallback. Preserve the native null diagnostic.
inline void EndGameScreens::AutosaveCompletedBridge()
{
    Require(pEndGameScreens,"Autosave completion has no live EndGameScreens owner");
    pEndGameScreens->AutosaveCompletedCallback();
}
}

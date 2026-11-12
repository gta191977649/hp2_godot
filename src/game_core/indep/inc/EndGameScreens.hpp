// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/EndGameScreens.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/race/end_game_screens.hpp"
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
namespace hp2::debug {
int GetTheRightTrackLanguageID(const ::port::driving::Memory&,std::uint32_t,int); //001D9FF0, W72.
int GetTheRightTrackLanguageID(const ::port::driving::Memory&,int,int); //001DA018.
// E-0112-R02. Original EndGameScreens0x48-byte logical view, Debug authority.
class EndGameScreens:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001D97C8(int,const ::port::race::EndGameScreenServices&);
 void Sub_001D9848(int,const ::port::race::EndGameScreenServices& ={});
 void BuildScreenList(const ::port::race::EndGameScreenServices&); //001D9550.
 void ResetAllScreenCounters(); //001D97C0, actual empty function.
 void Constructed(); //001D99D0, actual empty function.
 std::uint32_t GetNextScreen()const; //001D98B8.
 std::uint32_t GetCurrentScreen()const; //001D99C0.
 void RemoveScreen(std::uint32_t); //001D98E0.
 void AdvanceScreen(); //001D9968.
 bool AtRoot()const; //001D9988.
 void MoveBackScreen(); //001D99A0.
 void ResumeScreens(const ::port::race::EndGameScreenServices&); //001D99D8.
 void SuspendScreens(const ::port::race::EndGameScreenServices&); //001D9A88.
 void AutosaveCompletedCallback(const ::port::race::EndGameScreenServices&); //001D9AD8.
 void SetAutosaveCallback(const ::port::race::EndGameScreenServices&); //001D9B10.
};
bool ShouldSeeBeACopResults1Player(const ::port::driving::Memory&); //001D9198.
bool ShouldSeeBeACopResults2Player(const ::port::driving::Memory&); //001D91D0.
bool ShouldSeeEventRewardScreen(const ::port::driving::Memory&); //001D9208.
bool ShouldSeePointsRewardScreen(const ::port::driving::Memory&,const ::port::race::EndGameScreenServices&); //001D92C0.
bool ShouldSeeEventsUnlockedScreen(const ::port::driving::Memory&); //001D9308.
bool ShouldSeeTaskRewardScreen(const ::port::driving::Memory&); //001D9380.
bool ShouldSeeRaceResultsScreen(const ::port::driving::Memory&); //001D9398.
bool ShouldSeeTourney(const ::port::driving::Memory&); //001D9400.
bool ShouldSeeBestLap(const ::port::driving::Memory&); //001D9460.
bool ShouldSeeBestTotalTimes(const ::port::driving::Memory&); //001D94B0.
bool ShouldAutoSave(const ::port::driving::Memory&); //001D9518.
bool ShouldSeeEndGame(); //001D9528.
class tSlideshowStats:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001DBCB0(int);void Sub_001DC1B8(int);
 void SetAlphaScale(float);void SetupDataset(std::uint32_t);void Tick();void ChangeStats(int);
};
class tRaceResultsPointsAnimation:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001DC650(int);void Sub_001DEB08(int);
 void StartAnimation();void Redraw();void Tick();
};
class RaceResultsScreen:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001DC838(int);void Sub_001DEAA0(int);
 void BuildPointsResults();void Scroll(ScreenMessage,ScreenCallbackData&);void Tick(ScreenMessage,ScreenCallbackData&);
};
class EndGameMenusBase:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void HandleEvent(ScreenMessage,ScreenCallbackData&,const ::port::race::EndGameScreenServices&);
};
class EndGameMenu:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001DDA78(int,ScreenCallbackData&,const ::port::race::EndGameScreenServices&);
 void Sub_001DE958(int);
 void FigureOutPhrases(std::uint32_t);
 void Popup(int,int,int,const ::port::race::EndGameScreenServices&);
 void HandleEvent(ScreenMessage,ScreenCallbackData&,const ::port::race::EndGameScreenServices&);
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class EndGameScreens {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001DCE18; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dce18.md.
    // Original Debug reference: EndGameScreens::GetNextScreen(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetNextScreen();
    // ELF 0x001DCF00; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dcf00.md.
    // Original Debug reference: EndGameScreens::MoveBackScreen(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void MoveBackScreen();
};
} // namespace hp2::recovered

namespace hp2::debug {std::uint32_t FigureOutRaceObjective(::port::driving::Memory&,int); /*001DF9B0,E-0112-P03*/}

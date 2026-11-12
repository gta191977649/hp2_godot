#pragma once
// TASK-0098 E-0098-S01. Debug NFSHP2.ELF SHA815b60c7...332f1b.
// Recovered ../indep/inc/Fred.hpp names; explicit host storage, NOT PS2 ABI.
#include "CarInfo.hpp"
#include "EventTree.hpp"
namespace port::driving {class Memory;}
namespace port::race {struct DefaultSettings;struct ParticipantServices;struct StartServices;}
namespace hp2::debug {
// E-0107-F01, Debug GameTypes enum read verbatim through read-only MCP.
enum GameTypes { FRED_NO_GAME_TYPE=0, FRED_DEMO_GAME=1, FRED_EXPRESS_GAME=2,
 FRED_CHALLENGE_GAME_SINGLE_RACE=3, FRED_CHALLENGE_GAME_KNOCKOUT=4,
 FRED_CHALLENGE_GAME_LAP_KNOCKOUT=5, FRED_CHALLENGE_GAME_FREE_RUN=6,
 FRED_CHALLENGE_GAME_BE_A_COP=7, FRED_ULTIMATE_GAME=8, FRED_HOTPURSUIT_GAME=9,
 FRED_REPLAY_GAME=10, FRED_NUM_GAME_TYPES=11 };
enum eCarStatus { eCAR_UNLOCKED_FULLY_VISIBLE=0,eCAR_LOCKED_FULLY_VISIBLE=1,
    eCAR_LOCKED_PARTLY_VISIBLE=2,eCAR_LOCKED_INVISIBLE=3,eCAR_LOCKED_CLOAKED=4 };
struct RideInfo {
    static void DoSnapshot(::port::driving::Memory&,std::uint32_t,std::uint32_t snapshot); //001866C8,W74.
    int Type=0;
    std::uint8_t InstanceIndex=0,SkinNumber=0,HasDash=0,CanBeVertexDamaged=0;
    void Init(int,int,int,int);
};
struct CarStatus {
    static void AddAllSkins(::port::driving::Memory&,std::uint32_t,std::uint32_t); //00185CD8 raw source operands.
    eCarStatus nStatus=eCAR_LOCKED_INVISIBLE;
    std::array<std::uint8_t,10> aSkins{};
    int nSkins=0;
    bool bDashAvailable=false;
    void AddAllSkins(const CarTypeInfo&,int language);
};
// Partial decoded settings. Other profile fields remain in existing Retail storage.
struct cPlayerSettings {
    static void DoSnapshot(::port::driving::Memory&,std::uint32_t,std::uint32_t snapshot); //001EA3D0,W74.
    std::uint8_t DrivingStyle=0,TransmissionType=0;
    std::int8_t PreferredGender=0;
    // Debug001EA330, E-0098-RP01; explicit logical-storage overload, not host-layout stores.
    static void ResetToDefaults(::port::driving::Memory&,std::uint32_t,const ::port::race::DefaultSettings&);
};
struct FrontendPlayerInfo {
    static void FlushAvailableCars(::port::driving::Memory&,std::uint32_t); //00185AF8.
    static std::uint32_t GetCarStatus(const ::port::driving::Memory&,std::uint32_t,int); //00185B28.
    static void SetCarAvailability(::port::driving::Memory&,std::uint32_t,eCarStatus,int,int,bool); //00185AB0.
    static void BuildRideInfo(::port::driving::Memory&,std::uint32_t,std::uint32_t,int,std::uint32_t,int); //00185B88.
    static void AssignCarOfType(::port::driving::Memory&,std::uint32_t,int,int,std::uint32_t,int); //00185C38.
    static std::uint32_t GetSelectedRideInfo(const ::port::driving::Memory&,std::uint32_t,int); //00185CC8.
    std::array<CarStatus,64> aCarStatus{};
    std::array<RideInfo,11> RideInfoByGameMode{};
    cPlayerSettings mPlayerSettings;
    void FlushAvailableCars();
    CarStatus& GetCarStatus(int type);
    void SetCarAvailability(eCarStatus,int type,int skin_number,bool dash_available,
                            const std::array<CarTypeInfo,64>&,int language);
    void BuildRideInfo(RideInfo&,int type,std::uint32_t skin_number,int use_dash_if_available,
                       const std::array<CarTypeInfo,64>&) const;
    void AssignCarOfType(int game_type,int type,std::uint32_t skin_number,int use_dash_if_available,
                         const std::array<CarTypeInfo,64>&);
    RideInfo& GetSelectedRideInfo(int game_type);
};
struct FrontendDatabase {
    // Debug001828E8, E-0101-PL01. Optional service binds the original StartRace
    // child at the port host handoff; absent service invokes the raw child.
    static void ContinueMultiTrackEvent(::port::driving::Memory&,std::uint32_t,const std::function<void()>& start_race={});
    // TASK-0110 E-0110-F02; raw Debug source-memory finish dependencies.
    static void CheckForAbortion(::port::driving::Memory&,std::uint32_t); //00185830.
    static bool RaceWasAborted(const ::port::driving::Memory&,std::uint32_t); //00185878.
    static void GetOptionsSetFromRace(::port::driving::Memory&,std::uint32_t); //00185400.
    static int FigureOutBonusPoints(::port::driving::Memory&,std::uint32_t,std::uint32_t,std::uint32_t); //00185690.
    static void AwardPointsToPlayer(::port::driving::Memory&,std::uint32_t,std::uint32_t,std::uint32_t); //00185750.
    static void TheRaceIsOver(::port::driving::Memory&,std::uint32_t,std::uint32_t); //001858B8,E-0110-F02.
    static bool IsThereAnotherRace(::port::driving::Memory&,std::uint32_t); //00182780,E-0110-F04.
    static bool IsMultiTrack(const ::port::driving::Memory&,std::uint32_t); //001827B8,E-0110-F04.
    static bool SetupNextRace(::port::driving::Memory&,std::uint32_t); //001827E0,E-0101-RA02.
    static void TheRaceWasRestarted(::port::driving::Memory&,std::uint32_t); //00185A50,E-0110-F04.
    static void TheRaceWasQuit(::port::driving::Memory&,std::uint32_t); //001859F0,E-0110-F04; actual retire path, no finish award.
    static int InsertPlayerName(::port::driving::Memory&,std::uint32_t,std::uint32_t); //00184DC0.
    static void SetupRaceParametersAfterRaceSetup(::port::driving::Memory&,std::uint32_t); //00184640, E-0107-F01; FreeRun branch.
    static void StartRace(::port::driving::Memory&,std::uint32_t); //00183378, E-0099-W86 raw source request.
    static void MaybeSetSelectedCarForMultiTrackEvent(::port::driving::Memory&,std::uint32_t); //00182818.
    static bool IsChallengeGame(const ::port::driving::Memory&,std::uint32_t); //001839B8.
    static bool IsEventGame(const ::port::driving::Memory&,std::uint32_t); //001839E0.
    // E-0099-W84, raw source high-score pointers/record publication; not full profile defaults.
    static std::uint32_t GetBestTotalTimesTable(::port::driving::Memory&,std::uint32_t,int,int,int,int); //00185108.
    static std::uint32_t GetBestLapTimesTable(::port::driving::Memory&,std::uint32_t,int,int,int); //00185140.
    static void SetRecordTimesToBeat(::port::driving::Memory&,std::uint32_t,int); //00182990.
    static void Sub_00181BE8(::port::driving::Memory&,std::uint32_t); //Original complete sparse constructor; not InitWithDefaults.
    static void CalculateStartingPositions(::port::driving::Memory&); //00184390 raw original global RaceParameters; this unconsumed.
    static std::uint32_t GetEventTree(const ::port::driving::Memory&,std::uint32_t); //00183930.
    static void MakeEventCarsAvailable(::port::driving::Memory&,std::uint32_t,std::uint32_t,std::uint32_t); //001822F0.
    static void SetupCarsForRace(::port::driving::Memory&,std::uint32_t); //00183A58 raw explicit-opponent events; nonexplicit Retail schema frontier.
    // W56 raw original-memory bindings, separate from this decoded host storage.
    static bool UseTheRaceEvent(const ::port::driving::Memory&,std::uint32_t address); //00182718
    static bool UseTreeEvent(const ::port::driving::Memory&,std::uint32_t); //00182758, W78.
    static void TheRaceIsOn(::port::driving::Memory&,std::uint32_t address,std::uint32_t race); //00185888
    static void TheRaceIsLeaving(::port::driving::Memory&,std::uint32_t address); //001859C0, W62.
    std::array<FrontendPlayerInfo,2> FrontendPlayers{};
    int GameType=0,NumPlayerCars=1,nCurrentEvent=0;
    int bRaceWasAborted=0;
    void MakeEventCarsAvailable(const EventTree&,const EventInfo&,
                                const std::array<CarTypeInfo,64>&,int language);
    void SetupCarsForRace(::port::driving::Memory&,std::uint32_t,const EventInfo&,
        const ::port::race::ParticipantServices&);
    void CalculateStartingPositions(::port::driving::Memory&,std::uint32_t);
    void StartRace(::port::driving::Memory&,std::uint32_t,RaceEvent&,const ::port::race::StartServices&);
};
// E-0099-W84. Logical-memory API for the recovered class; not a host PS2 ABI.
struct HighScoreTable {
    static int MaybeRecordHighScore(::port::driving::Memory&,std::uint32_t,std::uint32_t,std::uint32_t); //00184E60,E-0110-F02.
    static void InitStatsTables(::port::driving::Memory&,std::uint32_t); //00184A78.
    static int GetUniqueTrackIndex(::port::driving::Memory&,int); //00184C10, this unconsumed.
    static int GetUniqueLapIndex(const ::port::driving::Memory&,int); //00184C48, this unconsumed.
    static int GetTotalTimesIndex(::port::driving::Memory&,int,int,int,int); //00184CA8.
    static int GetLapTimesIndex(::port::driving::Memory&,int,int,int); //00184D40.
};
struct TransientStatsElement {
    static void Construct(::port::driving::Memory&,std::uint32_t,std::uint32_t); //00185178,E-0110-F02.
};
struct TransientStats {
    static void Construct(::port::driving::Memory&,std::uint32_t,std::uint32_t); //00185320,E-0110-F02.
};
int PickCharacterNumberThatIsntRacing(std::array<int,8>&,int preferred_gender,
    const std::function<bool(int)>& character_is_male);
int PickCharacterNumberThatIsntRacing(::port::driving::Memory&,std::uint32_t,int,int); //00182610.
int PickCopCharacterNumber(::port::driving::Memory&,int); //00183A08 original argument unconsumed.
int AreThereAnyCarsOfType(::port::driving::Memory&,int); //00182440.
int AreThereAnyCarsOfColour(::port::driving::Memory&,int,int); //001824D8.
int PickRandomSkinOfUniqueColour(::port::driving::Memory&,int); //00182588.
int PickCopCharacterNumber(::port::driving::Memory&,std::uint32_t,int,
    const ::port::race::ParticipantServices&);
}

#include "indep/src/speech/SEDWedge.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/src/speech/mgr.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/GameFlow.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/CarInfo.hpp"

// PAL2075D8..207648. The game session gate precedes the actual SDK speech
// message boundary2919D0(category206,NULL), including a genuine null session.
WorldAudioMessagesResult worldAudioPlayPullover(WorldAudioMessages* messages,
    const GameSession* session)
{
    if(session) {
        const int32_t state=session->state;
        if(state==4 || state==7 || state==5 || state==9)return WORLD_AUDIO_MESSAGES_OK;
    }
    return worldAudioMessagesEnqueue(messages,0xCE,nullptr,0);
}

// PAL2077B8..2077DC. This caller has no session suppression gate; the
// actual SDK request is categoryD6 with one authored word equal to1.
WorldAudioMessagesResult worldAudioPlayBeACopIntro(WorldAudioMessages* messages)
{
    const uint32_t value=1;
    return worldAudioMessagesEnqueue(messages,0xD6,&value,1);
}

WorldAudioMessagesResult worldAudioPlayPursuitFinish(WorldAudioMessages* messages,
    const GameSession* session, int32_t classOneCount, int32_t variant,
    int32_t finishKind, int32_t completedCount)
{
    if (session) {
        const int32_t state=session->state;
        if (state==4 || state==7 || state==5 || state==9) return WORLD_AUDIO_MESSAGES_OK;
    }
    uint32_t mask;
    if (classOneCount==2) {
        if (variant) mask=0x200;
        else mask=finishKind==2 ? 0x8000 : 0x20000;
    } else if (!variant) {
        mask=0x400;
    } else {
        switch (completedCount) {
        case 0:mask=0x800;break;
        case 1:mask=0x1000;break;
        case 2:mask=0x2000;break;
        default:mask=0x200;break;
        }
    }
    return worldAudioMessagesEnqueue(messages,0xD5,&mask,1);
}

namespace {
// SingleP1OrP2, final2077E0/207810. Source compares the Car borrowed by
// pPlayersByNumber[0]; creation order and driver number are not substitutes.
template<class T> T SpeechSingleOrPlayer(const CarFramePlayerState& player,
    const RaceParameters& parameters,T single,T first,T second)
{
    if (parameters.mode!=2)return single;
    const auto* firstPlayer=player.registry->playersByNumber[0];
    return firstPlayer && firstPlayer->car==player.car ? first:second;
}

WorldAudioMessages* SpeechMessages(const CarFramePlayerState& player)
{
    const auto* stream=player.registry->services->streamedAudio;
    return stream?stream->messages:nullptr;
}
}

// RaceFinishedSpeech, final206AB8..206BC4 and its complete local callees
// 206920/206A00/206A30/2077E0/207810. The finish-reason argument of the
// released function is unused. The native boundary borrows its actual Player.
WorldAudioMessagesResult worldAudioRaceFinishedSpeech(CarFramePlayerState* player)
{
    if (!player)return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    if (player->raceFinishedSpeechSent)return WORLD_AUDIO_MESSAGES_OK;
    if (!player->registry || !player->registry->services || !player->car ||
        !player->car->bindings.owner)return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    auto& services=*player->registry->services;
    const auto* race=services.race;
    if (!race || !race->services || !race->services->settings)
        return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    const auto& parameters=*race->services->settings;
    // After its early suppressions, final2069BC dereferences the actual
    // session. A missing native owner is an error, never a speech success.
    if (!parameters.specialPursuit && player->car->bindings.player &&
        parameters.mode && race->entryCount>=2 && parameters.raceKind!=2 && !services.session)
        return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    const RaceCommentary commentary{race,&parameters,services.session,nullptr,nullptr,nullptr};
    if (parameters.specialPursuit || raceCommentarySuppressed(&commentary,player->car->bindings.owner)) {
        player->raceFinishedSpeechSent=1;
        return WORLD_AUDIO_MESSAGES_OK;
    }
    const auto* car=player->car->bindings.owner;
    bool busted=false;
    if (services.pursuit) {
        if (car->tableIndex>=8)return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
        busted=services.pursuit->participantReset[car->tableIndex].finishOnReset!=0;
    }
    WorldAudioMessagesResult result=WORLD_AUDIO_MESSAGES_OK;
    if (busted) {
        const uint32_t value=0x200;
        result=worldAudioMessagesEnqueue(SpeechMessages(*player),0xA8,&value,1);
    } else if (parameters.raceKind!=3 && parameters.raceKind!=4) {
        const auto* entry=car->raceEntry;
        if (!entry || entry->position<1 || entry->position>8)
            return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
        uint32_t words[2]{};
        bool lastPlace=false;
        if (static_cast<size_t>(entry->position)==race->entryCount) {
            if (race->entryCount==2) {
                words[1]=SpeechSingleOrPlayer(*player,parameters,8u,16u,32u);
                lastPlace=true;
            } else if (vectorDefaultRandomBounded(2)!=0) {
                // FlipCoinNoReplay206900 uses bMath's one existing RNG.
                words[1]=SpeechSingleOrPlayer(*player,parameters,1u,2u,4u);
                lastPlace=true;
            }
        }
        if (!lastPlace) {
            // Final31BA50/31BA10/31BA30, eight distinct finish positions.
            static constexpr uint32_t single[]={1,8,0x40,0x200,0x1000,0x8000,0x40000,0x200000};
            static constexpr uint32_t first[]={2,0x10,0x80,0x400,0x2000,0x10000,0x80000,0x400000};
            static constexpr uint32_t second[]={4,0x20,0x100,0x800,0x4000,0x20000,0x100000,0x800000};
            const auto* table=SpeechSingleOrPlayer<const uint32_t*>(*player,parameters,single,first,second);
            words[0]=table[entry->position-1];
        }
        result=worldAudioMessagesEnqueue(SpeechMessages(*player),0xA5,words,2);
    }
    player->raceFinishedSpeechSent=1;
    return result;
}

// MaybeDoKnockoutRaceOverSpeech, final207018..2070E8. It owns the same
// one-shot Player+A84 flag as RaceFinishedSpeech, including its null gate.
WorldAudioMessagesResult worldAudioKnockoutRaceOverSpeech(CarFramePlayerState* player,
    int32_t won)
{
    if (!player || player->raceFinishedSpeechSent)return WORLD_AUDIO_MESSAGES_OK;
    if (!player->registry || !player->registry->services ||
        !player->registry->services->race)return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    const auto& race=*player->registry->services->race;
    if (!race.services || !race.services->settings)return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    uint32_t words[2];
    if (race.services->settings->raceKind==3 || race.entryCount<3) {
        words[0]=won?0u:1u;words[1]=0;
    } else {words[0]=1;words[1]=won?1u:0u;}
    const auto result=worldAudioMessagesEnqueue(SpeechMessages(*player),0xA9,words,2);
    player->raceFinishedSpeechSent=1;
    return result;
}

namespace hp2 {
namespace {
uint32_t SpeechResultPlayer(const RaceParameters& parameters,int32_t player,
    uint32_t single,uint32_t first,uint32_t second)
{return parameters.mode==2?(player==0?first:second):single;}
bool SuppressRecordSpeech(const RaceParameters& parameters,const RaceEvent* event,
    const TransientStatsElement& result)
{return result.completionReason==2 || (parameters.eventGame && (!event || event->GetStatus()!=4));}
}
WorldAudioMessagesResult worldAudioPostGameLapRecord(WorldAudioMessages* messages,
    const RaceParameters& parameters,const RaceEvent* event,int32_t player,int32_t record,
    const TransientStatsElement& result)
{
    if(SuppressRecordSpeech(parameters,event,result))return WORLD_AUDIO_MESSAGES_OK;
    const uint32_t value=record?SpeechResultPlayer(parameters,player,0x40,0x80,0x100):
        SpeechResultPlayer(parameters,player,0x200,0x400,0x800);
    return worldAudioMessagesEnqueue(messages,0xa6,&value,1);
}
WorldAudioMessagesResult worldAudioPostGameTimeRecord(WorldAudioMessages* messages,
    const RaceParameters& parameters,const RaceEvent* event,int32_t player,int32_t record,
    const TransientStatsElement& result)
{
    if(SuppressRecordSpeech(parameters,event,result))return WORLD_AUDIO_MESSAGES_OK;
    const uint32_t value=record?SpeechResultPlayer(parameters,player,0x40,0x80,0x100):
        vectorDefaultRandomBounded(2)?SpeechResultPlayer(parameters,player,1,2,4):
        SpeechResultPlayer(parameters,player,8,0x10,0x20);
    return worldAudioMessagesEnqueue(messages,0xa6,&value,1);
}
WorldAudioMessagesResult worldAudioTournamentRaceSpeech(WorldAudioMessages* messages,bool finished,int32_t place)
{
    if(finished) {
        static constexpr uint32_t positions[]{1,8,0x40,0x200,0x1000,0x8000,0x40000,0x200000};
        if(place<0 || place>=8)return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
        const uint32_t values[]{positions[place],0};return worldAudioMessagesEnqueue(messages,0xa5,values,2);
    }
    if(place)return WORLD_AUDIO_MESSAGES_OK;
    const uint32_t value=0x4000;return worldAudioMessagesEnqueue(messages,0xa7,&value,1);
}
WorldAudioMessagesResult worldAudioUnlockCarSpeech(WorldAudioMessages* messages,const CarInfoResource& cars,int32_t model)
{
    if(model<0 || model>=64 || !cars.CarsLoaded())return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    const auto& car=cars.Car(static_cast<std::size_t>(model));
    uint32_t value=static_cast<uint32_t>(worldSpeechConvertCarTypeToUnlockSpeech(model));
    if(!value)return WORLD_AUDIO_MESSAGES_OK; // Final conversion deliberately has silent models.
    uint32_t category;
    if(car.bodyCategory==1)category=0x96;
    else if(car.pursuitConfiguration) {
        if(value==0x80){value=1;category=0xcc;}else category=0x95;
    } else category=0x93;
    return worldAudioMessagesEnqueue(messages,category,&value,1);
}
WorldAudioMessagesResult worldAudioUnlockTrackSpeech(WorldAudioMessages* messages,int32_t track,int32_t direction)
{
    const uint32_t values[]{static_cast<uint32_t>(worldSpeechConvertTrackNumberToUnlockSpeech(track)),direction==1?2u:1u};
    return values[0]?worldAudioMessagesEnqueue(messages,0x97,values,2):WORLD_AUDIO_MESSAGES_OK;
}
WorldAudioMessagesResult worldAudioRewardScreenSpeech(WorldAudioMessages* messages,
    const CarInfoResource& cars,bool rewardEarned,const EventReward* reward)
{
    if(!rewardEarned || !reward)return WORLD_AUDIO_MESSAGES_OK;
    const auto object=static_cast<int32_t>((reward->selection>>8)&255u);
    return (reward->selection&255u)==0?worldAudioUnlockCarSpeech(messages,cars,object):
        worldAudioUnlockTrackSpeech(messages,object,static_cast<int32_t>((reward->selection>>16)&255u));
}
WorldAudioMessagesResult worldAudioRaceEventSpeech(WorldAudioMessages* messages,CarFramePlayerState* player,
    const TransientStatsElement* result,const RaceEvent& event,const FrontendPlayerManager& frontend)
{
    if(!player || !result)return WORLD_AUDIO_MESSAGES_OK; // Exact released null gate.
    if(event.GetStatus()!=4) {const uint32_t value=0x40;return worldAudioMessagesEnqueue(messages,0xa8,&value,1);}
    if(!event.IsWinner())return WORLD_AUDIO_MESSAGES_OK;
    const uint32_t first=player->eventFinishedSpeechSent?0u:1u;player->eventFinishedSpeechSent=1;
    // Final conversion tables31C410/31C460/31C498/31C4E8, separate event
    // ranges and game types; there is no invented all-events speech bitmask.
    static constexpr uint32_t pursuitFirst[]={1,2,0,8,0,0x20,0x40,0x80,0,0,0,0,0,0,0x8000,0,0x10000,0,0x40000,0x80000};
    static constexpr uint32_t pursuitLast[]={1,2,4,0,0x10,0x20,0,0,0,0x200,0x400,0x800};
    static constexpr uint32_t championshipFirst[]={0,2,4,8,0x10,0x20,0x40,0x80,0x100,0,0x400,0x800,0x1000,0x2000,0x4000,0x8000,0x10000,0x20000,0x40000,0x80000};
    static constexpr uint32_t championshipLast[]={1,0,4,8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800};
    if(event.number<1 || event.number>32)return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    uint32_t low=0,high=0;
    if(event.number<=20)low=(frontend.gameType==9?pursuitFirst:championshipFirst)[event.number-1];
    else high=(frontend.gameType==9?pursuitLast:championshipLast)[event.number-21];
    if(!low && !high){const uint32_t value=0x400;return worldAudioMessagesEnqueue(messages,0xa8,&value,1);}
    const uint32_t values[]{first,low,high,event.willUnlockBonus?1u:0u};
    return worldAudioMessagesEnqueue(messages,high==0x800 || frontend.gameType==9?0xd8:0xd9,values,4);
}
}

// PAL1369A8 tests replay-demo state9,
// not GameFlow pause state6. Finish and lap producers share this one predicate.
static int32_t chooseVariant(void)
{
    return vectorDefaultRandomBounded(2) == 0;
}

int32_t raceCommentarySuppressed(const RaceCommentary* commentary, const Car* car)
{
    const CarMovement* movement = (const CarMovement*)car->movement;
    if (!movement->bindings.player || !commentary->settings->mode || commentary->race->entryCount < 2) return 1;
    if (commentary->session) {
        const int32_t state = commentary->session->state;
        if (state == 4 || state == 7 || state == 5 || state == 9) return 1;
    }
    if (commentary->settings->raceKind == 2) return 1;
    /* This original path requires the session owner even though its earlier
     * suppression branch accepts an absent session. Native binding validates
     * that requirement before a race starts. */
    if (commentary->session->state == 8 || commentary->session->state == 9) return 1;
    return commentary->settings->raceKind == 0;
}

void raceCommentarySelectSingle(const RaceCommentary* commentary, uint32_t words[4],
                                const Car* car, int32_t lap)
{
    static const uint32_t middlePosition[] = {2, 4, 8, 16, 32, 64};
    static const uint32_t lapNumber[] = {
        2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
        2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 0
    };
    if (lap == commentary->settings->totalLaps - 1) {
        words[3] = 0x400;
        return;
    }
    if ((chooseVariant() || lap >= 20) && commentary->settings->opponentCount > 0) {
        const int32_t position = car->raceEntry->position;
        if (position == 1) words[3] = chooseVariant() ? 1u : 0x200u;
        else if (position == 8) words[3] = chooseVariant() ? 0x80u : 0x100u;
        else words[3] = middlePosition[position - 2];
    } else words[0] = lapNumber[lap - 1];
}

void raceCommentarySelectSplit(const RaceCommentary* commentary, uint32_t words[4], const Car* car)
{
    static const float threshold[] = {0.5f, 0.3f, 0.2f, 0.1f, 0.05f, 0.01f, -0.1f, -0.2f, -99999.0f};
    static const uint32_t progressMessage[3][9] = {
        {1, 64, 8, 32768, 512, 4096, 262144, 524288, 4194304},
        {2, 128, 16, 65536, 1024, 8192, 262144, 1048576, 8388608},
        {4, 32, 256, 2048, 16384, 131072, 262144, 2097152, 16777216}
    };
    static const uint32_t positionMessage[3][8] = {
        {1, 8, 64, 512, 4096, 32768, 262144, 2097152},
        {2, 16, 128, 1024, 8192, 65536, 524288, 4194304},
        {4, 32, 256, 2048, 16384, 131072, 1048576, 8388608}
    };
    const Car* first = commentary->players->players[0]->car->bindings.owner;
    const Car* second = commentary->players->players[1]->car->bindings.owner;
    const int32_t isFirst = first == car;
    if (!commentary->settings->opponentCount) {
        float delta = first->raceEntry->progress - second->raceEntry->progress;
        if (!isFirst) delta = -delta;
        const int32_t variant = chooseVariant() ? 0 : (isFirst ? 1 : 2);
        for (size_t index = 0; index < sizeof(threshold) / sizeof(threshold[0]); ++index) {
            if (threshold[index] <= delta) {
                words[2] = progressMessage[variant][index];
                return;
            }
        }
    } else {
        const int32_t variant = chooseVariant() ? 0 : (isFirst ? 1 : 2);
        words[1] = positionMessage[variant][car->raceEntry->position - 1];
    }
}

int32_t raceCommentaryAnnounceRecord(RaceCommentary* commentary, const Car* car)
{
    const CarMovement* movement = (const CarMovement*)car->movement;
    const int32_t ticks = car->raceEntry->segmentTicks[commentary->race->finishCheckpoint];
    const RaceProfileBestTime* best = raceProfileFindBestTime(commentary->profile,
        commentary->settings->selectedScenario, movement->info.routeVehicleClass, commentary->settings->reverse);
    if (ticks != commentary->race->bestSegmentTicks || ticks >= best->ticks ||
        !best->ticks || best->ticks == INT32_MAX) return 0;
    worldAudioMessagesEnqueue(commentary->audio, 0xA4, NULL, 0);
    return 1;
}

void raceCommentaryAnnounceLap(RaceCommentary* commentary, const Car* car, int32_t lap)
{
    uint32_t words[4] = {0};
    if (raceCommentarySuppressed(commentary, car) || raceCommentaryAnnounceRecord(commentary, car)) return;
    /* The race-kind-three helper returns zero and leaves selection below
     * unchanged; it has no native state or backend work. */
    if (commentary->settings->mode == 2) raceCommentarySelectSplit(commentary, words, car);
    else raceCommentarySelectSingle(commentary, words, car, lap);
    /* Only the first word is passed by this original caller. The other
     * selection words are intentionally retained in the selection helpers. */
    worldAudioMessagesEnqueue(commentary->audio, 0xA0, words, 1);
}

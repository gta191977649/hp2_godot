#include "Joylog.hpp"
#include "indep/inc/Timer.hpp"
#include "bware/inc/bMath.hpp"
#include "Replay.hpp"

#include <string.h>
#include <cmath>

int32_t gameTimerAdvance(int32_t* ticks,float elapsed)
{
    if (*ticks!=0 && *ticks!=INT32_MAX) {
        const uint32_t remaining=(uint32_t)*ticks-(uint32_t)convertFloatToInt32(elapsed*4000.0f);
        memcpy(ticks,&remaining,sizeof(remaining));
        if (*ticks<0) *ticks=0;
    }
    return *ticks!=0 && *ticks!=INT32_MAX;
}

int32_t gameTimeQuantizeElapsed(float seconds)
{
    return convertFloatToInt32(bScalarAdd(bScalarMultiply(seconds,3600.0f),0.5f));
}

void gameTimeInitializeWorldTime(GameTime* state)
{
    state->worldTimer = convertFloatToInt32(4000.5f);
    state->worldTime = 1;
    state->worldTimeFrames = 1;
    state->worldTimeSeconds = bScalarMultiply((float)state->worldTimer,0.000250000012f);
    state->worldTimeElapsed = 0.0f;
    state->worldTimeRemainder = 0.0f;
    state->worldLoopCounter = 0;
    state->worldTimerPolicy = state->nativeTimingEnabled ? GAME_TIME_TIMER_NATIVE : GAME_TIME_TIMER_LEGACY;
}

GameTimerTextTemplates gameTimerTextTemplates{{
    "--h:--:--", "--h:--'--\"", "--:--.--", "--:--:--", "--'--\"--",
    "   --.--", "   --:--", "   --\"--", "    0.--", "    0:--", "    0\"--"
}};

void gameTimerGetHoursMinutesSeconds(const int32_t* ticks,int32_t* hours,
    int32_t* minutes,int32_t* seconds,int32_t* milliseconds)
{
    const int32_t value=*ticks;
    const int32_t totalSeconds=value/4000;
    const int32_t hour=totalSeconds/3600;
    const int32_t withinHour=totalSeconds-hour*3600;
    const int32_t minute=withinHour/60;
    *hours=hour;
    *minutes=minute;
    *seconds=withinHour-minute*60;
    *milliseconds=(value-totalSeconds*4000)/4;
}

void gameTimerPrintToString(int32_t* ticks,char* output,int32_t flags,
    const int32_t* language)
{
    int32_t hours=0,minutes=0,seconds=0,milliseconds=0;
    if (*ticks && *ticks!=INT32_MAX) {
        if (*ticks<0) {
            uint32_t magnitude=0u-(uint32_t)*ticks;
            memcpy(ticks,&magnitude,sizeof(magnitude));
            gameTimerGetHoursMinutesSeconds(ticks,&hours,&minutes,&seconds,&milliseconds);
            magnitude=0u-(uint32_t)*ticks;
            memcpy(ticks,&magnitude,sizeof(magnitude));
        } else {
            gameTimerGetHoursMinutesSeconds(ticks,&hours,&minutes,&seconds,&milliseconds);
        }
        if (*ticks<0)flags|=1;
    }
    const int32_t hundredths=milliseconds/10;
    const bool quoted=*language==1 || *language==3;
    const bool colon=*language==2 || *language==4 || *language==5;
    const auto field=[](char* destination,int32_t value,bool zeroPad) {
        for (int32_t index=1;index>=0;--index) {
            const int32_t digit=value%10;
            destination[index]=!zeroPad && !digit && value<10 ? ' ' : (char)('0'+digit);
            value/=10;
        }
    };
    // These computations inline the actual final formatting helpers. Their
    // standalone source paths have no named PDR proof and are not counted.
    char* selected;
    bool shiftSign=false;
    if (hours) {
        selected=gameTimerTextTemplates.text[quoted ? 1 : 0];
        memcpy(output,selected,strlen(selected)+1);
        field(output,hours,false);field(output+4,minutes,true);field(output+7,seconds,true);
        shiftSign=(flags&3)!=0 && hours>=10;
    } else if (minutes) {
        selected=gameTimerTextTemplates.text[quoted ? 4 : colon ? 3 : 2];
        memcpy(output,selected,strlen(selected)+1);
        field(output,minutes,false);field(output+3,seconds,true);field(output+6,hundredths,true);
        shiftSign=(flags&3)!=0 && minutes>=10;
    } else if (seconds) {
        selected=gameTimerTextTemplates.text[quoted ? 7 : colon ? 6 : 5];
        memcpy(output,selected,strlen(selected)+1);
        field(output+3,seconds,false);field(output+6,hundredths,true);
    } else {
        selected=gameTimerTextTemplates.text[quoted ? 10 : colon ? 9 : 8];
        memcpy(output,selected,strlen(selected)+1);field(output+6,hundredths,true);
    }
    if (shiftSign) {
        const size_t length=strlen(output)-1;
        memcpy(selected,output,length);
        memcpy(output+1,selected,length);
    }
    if (flags&1)output[0]='-';
    else if (flags&2)output[0]='+';
}

static const float timeQuantum = 0.000277777785f;
static const float tickerScale = 1.01670003f;
static const float ntscMin = 0.0166666675f;
static const float ntscMax = 0.0500000045f;
static const float palMin = 0.0199999996f;
static const float palMax = 0.0599999987f;
static const float frameSeconds = 0.0166666675f;
static const float timerSeconds = 0.000250000012f;

static int32_t timeToTicks(float seconds)
{
    return convertFloatToInt32(bScalarAdd(bScalarMultiply(seconds,4000.0f),0.5f));
}

static int32_t addTicks(int32_t value,int32_t step)
{
    const uint32_t bits=(uint32_t)value+(uint32_t)step;
    int32_t result;memcpy(&result,&bits,sizeof(result));return result;
}

void gameTimeInitialize(GameTime* state)
{
    memset(state, 0, sizeof(*state));
    state->videoMode = HP2_VIDEO_NTSC;
    state->realTime = 1;
    state->realTimeFrames = 1;
    state->worldTime = 1;
    state->worldTimeFrames = 1;
    state->worldTimeSeconds = 1.0f;
    state->worldLoopCounter = 100000;
    state->prepareWorldTimestep = 1; // PAL static cell330C94 before the first World frame.
}

int32_t gameTimeGetVideoMode(const GameTime* state)
{
    return state->videoMode;
}

int32_t gameTimeSetVideoMode(GameTime* state, int32_t mode)
{
    const int32_t previous = state->videoMode;
    state->videoMode = mode;
    return previous;
}

float gameTimeElapsedFromQuantized(int32_t value)
{
    // PAL1A33A8..1A33C0 defines the byte time quantum. This is an ordinary
    // native binary32 multiply, not an EE arithmetic instruction model.
    return (float)value*timeQuantum;
}

int gameTimePrepareRealTimestep(GameTime* state, float elapsed,
                                ReplayIntegerLog* log)
{
    int32_t quantized;
    const int32_t loop = state->realLoopCounter & 15;
    if (log && log->replaying) {
        quantized = (int32_t)replayIntegerLogRead(log, 8, 2);
        (void)replayIntegerLogRead(log, 4, 4);
    } else {
        float scaled = bScalarMultiply(elapsed,tickerScale);
        scaled = bScalarAdd(scaled,state->realTimeError);
        const float lower = state->videoMode != 0 ? ntscMin : palMin;
        const float upper = state->videoMode != 0 ? ntscMax : palMax;
        if (scaled < lower) scaled = lower;
        if (scaled > upper) scaled = upper;
        quantized = gameTimeQuantizeElapsed(scaled);
        const float seconds = gameTimeElapsedFromQuantized(quantized);
        state->realTimeError = bScalarAdd(scaled,-bScalarMultiply((float)timeToTicks(seconds),timerSeconds));
        if (log) {
            replayIntegerLogWrite(log, (uint32_t)quantized, 8, 2);
            replayIntegerLogWrite(log, (uint32_t)loop, 4, 4);
        }
    }
    state->realTimeQuantized = quantized;
    state->realTimeElapsed = gameTimeElapsedFromQuantized(quantized);
    return 1;
}

void gameTimeUseNativeTiming(GameTime* state)
{
    state->nativeTimingEnabled = 1;
    state->worldTimerPolicy = GAME_TIME_TIMER_NATIVE;
    state->realTimeError = 0.0f;
}

int gameTimePrepareNativeRealTimestep(GameTime* state, float elapsed,
                                      ReplayIntegerLog* log)
{
    // A recorded Joylog already owns its quantized input interval. Read its
    // original channels without scaling or rewriting the retained values.
    if (log && log->replaying)return gameTimePrepareRealTimestep(state,elapsed,log);
    if (!std::isfinite(elapsed) || elapsed<0.0f || elapsed>255.0f/3600.0f)return 0;
    const int32_t quantized=gameTimeQuantizeElapsed(elapsed);
    if (quantized<0 || quantized>255)return 0;
    state->realTimeError=0.0f;
    state->realTimeQuantized=quantized;
    state->realTimeElapsed=gameTimeElapsedFromQuantized(quantized);
    if (log) {
        replayIntegerLogWrite(log,(uint32_t)quantized,8,2);
        replayIntegerLogWrite(log,(uint32_t)(state->realLoopCounter&15),4,4);
    }
    return 1;
}

// Timer has 4000 ticks/second whereas the animation clock has 60. Taking the
// difference of the rounded cumulative endpoints retains the 2/3 tick phase.
// The existing frame/remainder pair is serialized by WorldTime, so restoring
// a new native snapshot also restores this phase without extra payload bytes.
static int32_t nativeTimerDelta(int32_t previousFrames,int32_t elapsedFrames,
                                float previousRemainder,float remainder)
{
    const uint32_t phase=(uint32_t)previousFrames%3u;
    const double previous=(double)phase*(200.0/3.0)+(double)previousRemainder*4000.0;
    const double next=((double)phase+(double)elapsedFrames)*(200.0/3.0)+(double)remainder*4000.0;
    return (int32_t)(std::floor(next+0.5)-std::floor(previous+0.5));
}

void gameTimeAdvanceNativeRealTime(GameTime* state)
{
    const float previousRemainder=state->realTimeRemainder;
    const float total=bScalarAdd(previousRemainder,state->realTimeElapsed);
    const int32_t frames=convertFloatToInt32(bScalarMultiply(total,60.0f));
    const float remainder=bScalarAdd(total,-bScalarMultiply((float)frames,frameSeconds));
    state->realTimer=addTicks(state->realTimer,
        nativeTimerDelta(state->realTimeFrames,frames,previousRemainder,remainder));
    const int32_t next=addTicks(state->realTimeFrames,frames);
    state->realTime=next;state->realTimeFrames=next;
    state->realTimeRemainder=remainder;
    state->realLoopCounter=addTicks(state->realLoopCounter,1);
}

void gameTimeAdvanceNativeWorldTime(GameTime* state)
{
    // PAL version 8 and the previously delivered native version 9 REC files retain the exact
    // original per-step rounding. Only explicitly marked native version 10 snapshots
    // and new native sessions use cumulative Timer conversion.
    if (state->worldTimerPolicy!=GAME_TIME_TIMER_NATIVE) {
        gameTimeAdvanceWorldTime(state);return;
    }
    state->prepareWorldTimestep=1;
    const float elapsed=state->worldTimeElapsed;
    if (elapsed==0.0f)return;
    const float previousRemainder=state->worldTimeRemainder;
    const float total=bScalarAdd(previousRemainder,elapsed);
    const int32_t frames=convertFloatToInt32(bScalarMultiply(total,60.0f));
    const float remainder=bScalarAdd(total,-bScalarMultiply((float)frames,frameSeconds));
    state->worldTimer=addTicks(state->worldTimer,
        nativeTimerDelta(state->worldTimeFrames,frames,previousRemainder,remainder));
    const int32_t next=addTicks(state->worldTimeFrames,frames);
    state->worldTime=next;state->worldTimeFrames=next;
    state->worldTimeRemainder=remainder;
    state->worldTimeSeconds=bScalarMultiply((float)state->worldTimer,timerSeconds);
    state->worldTimeElapsed=0.0f;
    state->worldLoopCounter=addTicks(state->worldLoopCounter,1);
}

void gameTimeAdvanceRealTime(GameTime* state)
{
    const float total = bScalarAdd(state->realTimeRemainder,state->realTimeElapsed);
    const int32_t frames = convertFloatToInt32(bScalarMultiply(total,60.0f));
    const int32_t step = frames != 0 ? frames : 1;
    const int32_t next=addTicks(state->realTimeFrames,step);
    state->realTime=next;
    state->realTimeFrames=next;
    state->realTimeRemainder = bScalarAdd(total,-bScalarMultiply((float)frames,frameSeconds));
    state->realTimer=addTicks(state->realTimer,timeToTicks(state->realTimeElapsed));
    state->realLoopCounter=addTicks(state->realLoopCounter,1);
}



void gameTimeSetWorldElapsed(GameTime* state, float elapsed)
{
    state->worldTimeElapsed = elapsed;
    state->prepareWorldTimestep = 0;
}

void gameTimeAdvanceWorldTime(GameTime* state)
{
    state->prepareWorldTimestep = 1;
    const float elapsed = state->worldTimeElapsed;
    if (elapsed == 0.0f) return;
    const float total = bScalarAdd(state->worldTimeRemainder,elapsed);
    const int32_t frames = convertFloatToInt32(bScalarMultiply(total,60.0f));
    const int32_t next=addTicks(state->worldTimeFrames,frames);
    state->worldTime=next;
    state->worldTimeFrames=next;
    state->worldTimeRemainder = bScalarAdd(total,-bScalarMultiply((float)frames,frameSeconds));
    state->worldTimer=addTicks(state->worldTimer,timeToTicks(elapsed));
    state->worldTimeSeconds = bScalarMultiply((float)state->worldTimer,timerSeconds);
    state->worldTimeElapsed = 0.0f;
    state->worldLoopCounter=addTicks(state->worldLoopCounter,1);
}

// PAL1A36C0..1A375C. The elapsed field is VerifyData, not MoveData: it
// participates only in checksum modes and is not restored from a snapshot.
void gameTimeSnapshot(GameTime* state,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"WorldTime");
    replaySnapshotMoveData(snapshot,&state->worldTimer,4,-1,0);
    replaySnapshotMoveFloat(snapshot,&state->worldTimeSeconds);
    replaySnapshotMoveInteger(snapshot,&state->worldLoopCounter,4);
    replaySnapshotMoveInteger(snapshot,&state->worldTimeFrames,4);
    replaySnapshotMoveInteger(snapshot,&state->worldTime,4);
    if (snapshot->recordingChecksums || snapshot->verifyingChecksums)
        replaySnapshotMoveData(snapshot,&state->worldTimeElapsed,4,-1,0);
    replaySnapshotMoveFloat(snapshot,&state->worldTimeRemainder);
    replaySnapshotEndSection(snapshot);
    if (snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
        if (snapshot->clockPolicy!=GAME_TIME_TIMER_LEGACY &&
            snapshot->clockPolicy!=GAME_TIME_TIMER_NATIVE) {
            snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
        }
        state->worldTimerPolicy=snapshot->clockPolicy;
    }
}

float gameTimerGetSecondsRounded(const int32_t* ticks,float resolution)
{
    const float seconds=bScalarMultiply((float)*ticks,0.000250000012f);
    const float units=bScalarDivide(seconds,resolution);
    float rounded=(float)convertFloatToInt32(units);
    if (units<rounded)rounded=bScalarAdd(rounded,-1.0f);
    if (0.5f<=bScalarAdd(units,-rounded))rounded=bScalarAdd(rounded,1.0f);
    const float raw=bScalarMultiply(bScalarMultiply(rounded,resolution),4000.0f);
    float integral=(float)convertFloatToInt32(raw);
    if (integral<raw)integral=bScalarAdd(integral,1.0f);
    return bScalarMultiply(integral,0.000250000012f);
}
void gameTimerRoundTime(int32_t* ticks,float resolution)
{
    *ticks=timeToTicks(gameTimerGetSecondsRounded(ticks,resolution));
}
void gameTimerPrintRounded(const int32_t* ticks,char* output,float resolution,
    int32_t flags,const int32_t* language)
{
    int32_t rounded=timeToTicks(gameTimerGetSecondsRounded(ticks,resolution));
    gameTimerPrintToString(&rounded,output,flags,language);
}

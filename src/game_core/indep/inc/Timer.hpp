#ifndef NFSHP2_GAME_TIMER_H
#define NFSHP2_GAME_TIMER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

void physicsSecondsToTicks(int32_t* ticks, float seconds);

/* Exact final GetVideoMode branches: zero uses 50 Hz bounds, every nonzero
 * value uses 60 Hz bounds. The authored static initializer is raw value1. */
enum { HP2_VIDEO_PAL = 0, HP2_VIDEO_NTSC = 1 };

/* PC clock policy is typed host metadata. It is not part of the 24-byte
 * WorldTime snapshot payload or the authored PAL clock layout. */
enum { GAME_TIME_TIMER_LEGACY = 0, GAME_TIME_TIMER_NATIVE = 1 };

typedef struct GameTime {
    int32_t videoMode;
    int32_t realLoopCounter;
    int32_t realTimer;
    int32_t realTime;
    int32_t realTimeFrames;
    int32_t realTimeQuantized;
    float realTimeElapsed;
    float realTimeRemainder;
    float realTimeError;
    int32_t worldTimer;
    float worldTimeSeconds;
    int32_t worldTimeFrames;
    int32_t worldTime;
    float worldTimeElapsed;
    float worldTimeRemainder;
    int32_t worldLoopCounter;
    int32_t prepareWorldTimestep;
    int32_t nativeTimingEnabled;
    int32_t worldTimerPolicy;
} GameTime;

struct ReplayIntegerLog;

void gameTimeInitialize(GameTime* state);
int32_t gameTimeGetVideoMode(const GameTime* state);
int32_t gameTimeSetVideoMode(GameTime* state, int32_t mode);
float gameTimeElapsedFromQuantized(int32_t value);
int32_t gameTimeQuantizeElapsed(float seconds);
int gameTimePrepareRealTimestep(GameTime* state, float elapsed,
                                struct ReplayIntegerLog* log);
void gameTimeAdvanceRealTime(GameTime* state);
void gameTimeInitializeWorldTime(GameTime* state);
void gameTimeSetWorldElapsed(GameTime* state, float elapsed);
void gameTimeAdvanceWorldTime(GameTime* state);
/* Native runtime calls these once per logical update, independently of the
 * presentation rate. Original Timer entry points remain available for
 * legacy Joylog and original-contract verification. */
void gameTimeUseNativeTiming(GameTime* state);
int gameTimePrepareNativeRealTimestep(GameTime* state, float elapsed,
                                      struct ReplayIntegerLog* log);
void gameTimeAdvanceNativeRealTime(GameTime* state);
void gameTimeAdvanceNativeWorldTime(GameTime* state);
struct ReplaySnapshot;
void gameTimeSnapshot(GameTime* state,struct ReplaySnapshot* snapshot);
/* The native source clock publishes measured 50/59.94 Hz intervals into
 * the original platform VBlank cell, separately from Timer loop counters
 * and physical GPU/display completion. */
uint32_t platformVideoBlankCount(void);
void platformNotifyVideoBlank(void);


/* Zero and INT32_MAX are inactive. An elapsed interval consumes truncated
 * 4000 Hz ticks; expiration clamps to zero. */
int32_t gameTimerAdvance(int32_t* ticks,float elapsed);

enum { GAME_TIMER_TEXT_CAPACITY = 16 };

/* One mutable projection of the eleven final time-format templates. The
 * sign-formatting path writes the selected template before shifting text. */
typedef struct GameTimerTextTemplates {
    char text[11][GAME_TIMER_TEXT_CAPACITY];
} GameTimerTextTemplates;
extern GameTimerTextTemplates gameTimerTextTemplates;

void gameTimerGetHoursMinutesSeconds(const int32_t* ticks,int32_t* hours,
    int32_t* minutes,int32_t* seconds,int32_t* milliseconds);

/* output owns at least GAME_TIMER_TEXT_CAPACITY bytes and does not overlap
 * the template owner. language borrows LanguageManager.currentLanguage. */
void gameTimerPrintToString(int32_t* ticks,char* output,int32_t flags,
    const int32_t* language);
float gameTimerGetSecondsRounded(const int32_t* ticks,float resolution);
void gameTimerRoundTime(int32_t* ticks,float resolution);
void gameTimerPrintRounded(const int32_t* ticks,char* output,float resolution,
    int32_t flags,const int32_t* language);

#ifdef __cplusplus
}
#include <chrono>
#include <cstdint>
namespace hp2 {
// Bounded native performance observations only. No source timer, frame,
// readiness or presentation state is advanced by this monotonic measurement.
struct NativeDurationStatistics {
    std::uint64_t calls{},totalNanoseconds{},maximumNanoseconds{};
    void Observe(std::uint64_t elapsed) noexcept {
        ++calls;totalNanoseconds+=elapsed;
        if(elapsed>maximumNanoseconds)maximumNanoseconds=elapsed;
    }
};
class ScopedNativeDuration final {
public:
    explicit ScopedNativeDuration(NativeDurationStatistics& output) noexcept
        :output_(&output),start_(std::chrono::steady_clock::now()) {}
    ~ScopedNativeDuration() {Finish();}
    ScopedNativeDuration(const ScopedNativeDuration&)=delete;
    ScopedNativeDuration& operator=(const ScopedNativeDuration&)=delete;
    void Finish() noexcept {
        if(!output_)return;
        const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now()-start_).count();
        output_->Observe(elapsed>0?static_cast<std::uint64_t>(elapsed):0);
        output_=nullptr;
    }
private:
    NativeDurationStatistics* output_;
    std::chrono::steady_clock::time_point start_;
};

}
#endif
#endif

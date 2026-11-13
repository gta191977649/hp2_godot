#include "bware/inc/bWare.hpp"
#include <chrono>
#include <atomic>


namespace hp2 {
namespace {std::atomic<NativeSessionSink> sessionSink{nullptr};}
void BindSessionSink(NativeSessionSink sink) noexcept
{sessionSink.store(sink,std::memory_order_release);}
void RecordSessionEvent(const char* event,const char* detail) noexcept
{if(const auto sink=sessionSink.load(std::memory_order_acquire))sink(event,detail);}

std::uint32_t TicksFromNanoseconds(std::uint64_t ns) noexcept {
    constexpr std::uint64_t kBillion = 1000000000ull;
    const auto seconds = ns / kBillion;
    const auto remainder = ns % kBillion;
    const std::uint64_t integerTicks = (seconds * kTickerFrequency) & 0xffffffffull;
    const std::uint64_t fractionalTicks = (remainder * kTickerFrequency) / kBillion;
    return static_cast<std::uint32_t>(integerTicks + fractionalTicks);
}

std::uint32_t TicksNow() noexcept {
    const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
    return TicksFromNanoseconds(static_cast<std::uint64_t>(nanoseconds));
}

float tickerMilliseconds(std::uint32_t from, std::uint32_t to) noexcept {
    const std::uint32_t difference = to - from;
    float value;
    if (static_cast<std::int32_t>(difference) >= 0) {
        value = static_cast<float>(static_cast<std::int32_t>(difference));
    } else {
        value = static_cast<float>(static_cast<std::int32_t>((difference & 1u) | (difference >> 1)));
        value += value;
    }
    // Retain the game's original clock conversion constant.
    return value / kTimerConversionFrequency * 1000.0f;
}

std::uint32_t fixedTickerDifference(std::uint32_t from, std::uint32_t to) noexcept {
    return ((to - from) << 6) / 293u;
}
} // namespace hp2




#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

extern "C" std::uint32_t hp2NativeTickerSource(void) {
    return hp2::TicksNow();
}

/* PAL250970 asks its actual development host for the computer name through
 * bFunk command58. On Windows the operating system owns that value. Linux
 * supplies the same host-name operation for the explicitly separate CPU
 * verification build. A failed transport produces an empty string/false,
 * matching the source failure branch; it never reports synthetic success. */
extern "C" int32_t bGetComputerName(char* output,int32_t capacity)
{
    if(!output || capacity<=0) return 0;
#if defined(_WIN32)
    DWORD available=static_cast<DWORD>(capacity);
    if(GetComputerNameA(output,&available)) return 1;
#else
    if(gethostname(output,static_cast<size_t>(capacity))==0) return 1;
#endif
    output[0]='\0';
    return 0;
}

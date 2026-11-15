#include "indep/inc/Timer.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Flags.hpp"
#include "bware/inc/bFile.hpp"

#include <atomic>
#include <algorithm>
#include <array>
#include <filesystem>
#include <exception>
#include <csignal>
#include <cmath>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#endif

namespace {
std::atomic<uint32_t> videoBlankCount{0};
}

uint32_t platformVideoBlankCount(void)
{
    return videoBlankCount.load(std::memory_order_acquire);
}

void platformNotifyVideoBlank(void)
{
    // The native source-clock transport publishes measured 50/59.94 Hz
    // intervals into this original VBlank cell. GPU presentation/fences,
    // simulation steps and real-time quantization have separate counters.
    videoBlankCount.fetch_add(1u, std::memory_order_release);
}

namespace hp2 {
namespace {
using NativeFatalSink=void(*)(const char*) noexcept;
std::atomic<NativeFatalSink> nativeFatalSink{nullptr};
}
extern "C" void hp2NativeRecordFailure(const char* reason)
{
    if(const auto sink=nativeFatalSink.load(std::memory_order_acquire))sink(reason);
    else if(reason)std::fputs(reason,stderr);
}
// InitDisplaySystem, PAL1007B8..100804; beta100750's complete body and
// immediate N_SOL/PDR/MAP record identify this original Platform.cpp owner.
// The physical window already exists on PC. Its source video clock still
// follows the disc marker, including the PAL first-launch language policy.
void InitDisplaySystem(const GameResources& files,GameTime& clock)
{
    gameTimeSetVideoMode(&clock,files.TryOpenPhysical("PAL.TAG") ? HP2_VIDEO_PAL : HP2_VIDEO_NTSC);
}

std::uint64_t NativeRefreshClock::Observe(std::uint64_t monotonicQpc,
    std::uint64_t qpcFrequency,std::int32_t videoMode)
{
    constexpr std::uint64_t scale=262144; // Exact denominator of float32 0x426FC28F.
    if(!qpcFrequency || qpcFrequency>std::numeric_limits<std::uint64_t>::max()/scale)
        throw std::invalid_argument("Invalid native display QPC frequency");
    const auto add=[](std::uint64_t left,std::uint64_t right) {
        if(right>std::numeric_limits<std::uint64_t>::max()-left)
            throw std::overflow_error("Native display clock overflow");
        return left+right;
    };
    const bool ntsc=videoMode!=0;
    if(!initialized_ || ntsc!=ntsc_ || frequency_!=qpcFrequency || monotonicQpc<lastQpc_) {
        // Rebase only for an actual source-mode/frequency change or a
        // backward native timestamp. Repeated polls keep fractional carry.
        initialized_=true;ntsc_=ntsc;frequency_=qpcFrequency;
        lastQpc_=monotonicQpc;remainder_=0;
        return 0;
    }
    const auto elapsed=monotonicQpc-lastQpc_;
    lastQpc_=monotonicQpc;
    const std::uint64_t denominator=qpcFrequency*scale;
    const std::uint32_t numerator=ntsc ? 15712911u:13107200u;
    // Compute elapsed*numerator/denominator with an exact fractional carry.
    // The 24-bit multiply is split into quotient/remainder additions so a
    // long idle cannot overflow an intermediate QPC product. This remains
    // ordinary native clock conversion, independent of render completion.
    std::uint64_t ticks=0,tail=0;
    const auto whole=elapsed/denominator,part=elapsed%denominator;
    const auto addRemainder=[&](std::uint64_t value) {
        if(tail>=denominator-value) {tail-=denominator-value;return std::uint64_t{1};}
        tail+=value;return std::uint64_t{0};
    };
    for(int bit=23;bit>=0;--bit) {
        ticks=add(ticks,ticks);ticks=add(ticks,addRemainder(tail));
        if(numerator&(std::uint32_t{1}<<bit)) {
            ticks=add(ticks,whole);ticks=add(ticks,addRemainder(part));
        }
    }
    ticks=add(ticks,addRemainder(remainder_));remainder_=tail;
    return ticks;
}

std::uint64_t NativeRefreshClock::QpcUntilNextRefresh() const
{
    if(!initialized_)throw std::logic_error("Native source clock has no QPC baseline");
    constexpr std::uint64_t scale=262144;
    const auto remaining=frequency_*scale-remainder_;
    const std::uint32_t numerator=ntsc_ ? 15712911u:13107200u;
    // Exact ceiling without overflowing remaining+numerator-1. A wait that
    // wakes early must observe QPC again; no elapsed tick is assumed here.
    return remaining/numerator+(remaining%numerator!=0);
}
bool NativeRefreshClock::Advanced(std::uint32_t before,std::uint32_t current) noexcept
{
    // PAL MainLoop12F64C..12F65C: signed32(current-start)>0. Spell the
    // two's-complement test in unsigned arithmetic, including counter wrap.
    const auto difference=current-before;
    return difference!=0 && difference<0x80000000u;
}
void NativeFrameTiming::Reset(Clock::time_point now) noexcept
{
    *this={};initialized_=true;windowStart_=lastObservation_=now;
}

bool NativeFrameTiming::Observe(Clock::time_point now,
    const EcstasyVulkanFrameStatistics* renderer,double updateMilliseconds) noexcept
{
    if(!initialized_ || now<lastObservation_) {Reset(now);return false;}
    if(now==lastObservation_)return false;
    lastObservation_=now;
    if(renderer) {
        hasPresentationTarget_=hasPresentationTarget_ || renderer->presentationTarget;
        if(renderer->submittedFrame && renderer->submittedFrame!=lastSubmission_) {
            lastSubmission_=renderer->submittedFrame;
            if(renderer->presentationTarget && renderer->presentAccepted)++presentedFrames_;
            const auto cpu=updateMilliseconds+renderer->gatherMilliseconds+renderer->recordSubmitMilliseconds;
            if(std::isfinite(cpu) && cpu>=0.0) {cpuMilliseconds_+=cpu;++cpuSamples_;}
        }
        if(renderer->gpuTimeAvailable && renderer->completedGpuFrame &&
            renderer->completedGpuFrame!=lastGpuSubmission_ &&
            std::isfinite(renderer->gpuMilliseconds) && renderer->gpuMilliseconds>0.0) {
            lastGpuSubmission_=renderer->completedGpuFrame;
            gpuMilliseconds_+=renderer->gpuMilliseconds;++gpuSamples_;
        }
    }
    const auto elapsed=std::chrono::duration<double>(now-windowStart_).count();
    if(elapsed<0.5)return false;
    published_={};
    published_.presentedFrames=presentedFrames_;
    published_.cpuSamples=cpuSamples_;published_.gpuSamples=gpuSamples_;
    published_.frameRateAvailable=hasPresentationTarget_ && presentedFrames_!=0;
    published_.cpuTimeAvailable=cpuSamples_!=0;published_.gpuTimeAvailable=gpuSamples_!=0;
    if(published_.frameRateAvailable) {
        published_.framesPerSecond=static_cast<double>(presentedFrames_)/elapsed;
        published_.frameMilliseconds=elapsed*1000.0/static_cast<double>(presentedFrames_);
    }
    if(cpuSamples_)published_.cpuMilliseconds=cpuMilliseconds_/static_cast<double>(cpuSamples_);
    if(gpuSamples_)published_.gpuMilliseconds=gpuMilliseconds_/static_cast<double>(gpuSamples_);
    windowStart_=now;presentedFrames_=cpuSamples_=gpuSamples_=0;
    cpuMilliseconds_=gpuMilliseconds_=0.0;hasPresentationTarget_=false;
    return true;
}

void NativeSimulationClock::Reset(Clock::time_point now) noexcept
{
    deadline_=now;remainder_=0;initialized_=true;
}
bool NativeSimulationClock::UpdateDue(Clock::time_point now) const noexcept
{
    return initialized_ && now>=deadline_;
}
void NativeSimulationClock::CompleteStep(std::uint32_t elapsedQuanta)
{
    if(!initialized_ || !elapsedQuanta || elapsedQuanta>255u)
        throw std::invalid_argument("Native logical update has an invalid recorded timestep");
    constexpr std::uint64_t nanosecondsPerSecond=1000000000u;
    const auto scaled=static_cast<std::uint64_t>(elapsedQuanta)*nanosecondsPerSecond+remainder_;
    const auto elapsed=scaled/3600u;remainder_=static_cast<std::uint32_t>(scaled%3600u);
    deadline_+=std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(elapsed));
}

bool NativeApplicationHost::WaitForSourceRefresh(std::uint32_t)
{
    return PollEvents();
}

std::filesystem::path nativeExecutablePath()
{
#ifdef _WIN32
    std::vector<wchar_t> buffer(512);
    for(;;) {
        const DWORD count=GetModuleFileNameW(nullptr,buffer.data(),static_cast<DWORD>(buffer.size()));
        if(!count)throw std::system_error(static_cast<int>(GetLastError()),std::system_category(),"Locate NFSHP2.exe");
        if(count<buffer.size())return std::filesystem::path(std::wstring(buffer.data(),count));
        if(buffer.size()>=32768)throw std::runtime_error("Executable path exceeds Windows path limit");
        buffer.resize(std::min<std::size_t>(32768,buffer.size()*2));
    }
#elif defined(__linux__)
    return std::filesystem::read_symlink("/proc/self/exe");
#else
    throw std::runtime_error("Executable-directory lookup is unavailable on this host");
#endif
}

}

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

namespace {
namespace fs=std::filesystem;
std::string UpperPathAscii(std::string text)
{
    for(char& c:text)if(c>='a' && c<='z')c=static_cast<char>(c-'a'+'A');
    return text;
}
fs::path ProcessChild(const fs::path& directory,const char* name)
{
    std::error_code error;
    if(!fs::is_directory(directory,error))return {};
    const auto wanted=UpperPathAscii(name);fs::path result;
    fs::directory_iterator current(directory,error),end;
    if(error)throw std::system_error(error,"Read native game-data directory: "+directory.u8string());
    for(;current!=end;current.increment(error)) {
        if(error)throw std::system_error(error,"Read native game-data directory: "+directory.u8string());
        if(UpperPathAscii(current->path().filename().u8string())!=wanted)continue;
        if(!result.empty())throw std::runtime_error("Case-ambiguous native data path: "+directory.u8string()+"/"+name);
        result=current->path();
    }
    if(error)throw std::system_error(error,"Read native game-data directory: "+directory.u8string());
    return result;
}
bool OrdinaryFile(const fs::path& path)
{
    if(path.empty())return false;
    std::error_code error;return fs::is_regular_file(path,error);
}
fs::path ResolveArgumentPath(const fs::path& value,const fs::path& directory)
{
    // D:\\F:\\... was the invalid path in the first user launch. Do not
    // silently strip a drive name and accidentally open another game tree.
    const auto text=value.u8string();
    const auto firstColon=text.find(':');
    if(firstColon!=std::string::npos && text.find(':',firstColon+1)!=std::string::npos)
        throw std::invalid_argument("Invalid native path with more than one drive prefix: "+text);
    return (value.is_absolute()?value:directory/value).lexically_normal();
}
int32_t DisplayInteger(const fs::path& value,const char* option)
{
    const auto text=value.u8string();std::size_t used{};long number{};
    try {number=std::stol(text,&used,10);}catch(const std::exception&) {
        throw std::invalid_argument(std::string("Invalid ")+option+" value: "+text);
    }
    if(used!=text.size() || number<1 || number>16384)
        throw std::invalid_argument(std::string("Invalid ")+option+" value: "+text);
    return static_cast<int32_t>(number);
}
std::vector<fs::path> NativeDataDirectories(const fs::path& executableDirectory)
{
    std::vector<fs::path> directories;
    auto add=[&](const fs::path& path) {
        if(path.empty())return;
        const auto normalized=path.lexically_normal();
        if(std::find(directories.begin(),directories.end(),normalized)==directories.end())
            directories.push_back(normalized);
    };
    // Automatic discovery belongs to this installation only. A shortcut's
    // working directory or a parent project can contain a different build.
    add(executableDirectory);
    for(const auto* name:{"NFSHP2","@ZZDATA","ZZDATA","Data"})
        add(ProcessChild(executableDirectory,name));
    const auto game=ProcessChild(executableDirectory,"NFSHP2");
    if(!game.empty()) {add(ProcessChild(game,"@ZZDATA"));add(ProcessChild(game,"ZZDATA"));}
    return directories;
}
bool LooseGameDirectory(const fs::path& directory)
{
    const auto global=ProcessChild(directory,"GLOBAL");
    if(global.empty())return false;
    return OrdinaryFile(ProcessChild(global,"GLOBALB.LZC")) || OrdinaryFile(ProcessChild(global,"GLOBALB.BUN"));
}
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

void NativePresentationClock::SetLimit(std::int32_t framesPerSecond)
{
    if(framesPerSecond<0 || framesPerSecond>1000)throw std::invalid_argument("Presentation limit must be 0..1000 FPS");
    limit_=framesPerSecond;remainder_=0;deadline_={};
}
void NativePresentationClock::Reset(Clock::time_point now) noexcept
{
    deadline_=now;remainder_=0;
}
bool NativePresentationClock::Due(Clock::time_point now) const noexcept
{
    return limit_==0 || now>=deadline_;
}
void NativePresentationClock::Presented(Clock::time_point started) noexcept
{
    if(!limit_)return;
    const auto rate=static_cast<std::uint32_t>(limit_);
    const std::uint64_t scaled=1000000000ull+remainder_;
    remainder_=static_cast<std::uint32_t>(scaled%rate);
    deadline_=started+std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(scaled/rate));
}
NativePresentationClock::Clock::time_point NativePresentationClock::Deadline() const noexcept
{
    return limit_==0?Clock::time_point::max():deadline_;
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

NativeProcessLaunch resolveNativeProcessLaunch(const std::filesystem::path& executable,
    const std::filesystem::path& workingDirectory,const std::vector<std::filesystem::path>& arguments)
{
    const auto work=std::filesystem::absolute(workingDirectory).lexically_normal();
    const auto directory=ResolveArgumentPath(executable,work).parent_path();
    NativeProcessLaunch result;result.profileDirectory=directory/"Saves";
    bool sourceSelected=false;
    for(std::size_t i=0;i<arguments.size();++i) {
        const auto& argument=arguments[i];
        auto next=[&]() -> const std::filesystem::path& {
            if(++i>=arguments.size())throw std::invalid_argument("Missing value after "+argument.u8string());
            return arguments[i];
        };
        if(argument=="--data" || argument=="--zzdata") {
            if(sourceSelected)throw std::invalid_argument("Use one data source: --data or --zzdata");
            sourceSelected=true;result.archiveData=argument=="--zzdata";
            if(result.archiveData && (i+1==arguments.size() || arguments[i+1].u8string().rfind("--",0)==0))continue;
            result.dataSource=ResolveArgumentPath(next(),work);
        } else if(argument=="--profile") {result.profileDirectory=ResolveArgumentPath(next(),work);result.profileSpecified=true;}
        else if(argument=="--config") {result.configuration=ResolveArgumentPath(next(),work);result.configurationSpecified=true;}
        else if(argument=="--width") {result.width=DisplayInteger(next(),"--width");result.changeDisplay=true;}
        else if(argument=="--height") {result.height=DisplayInteger(next(),"--height");result.changeDisplay=true;}
        else if(argument=="--fullscreen" || argument=="--windowed") {
            result.fullscreen=argument=="--fullscreen";result.fullscreenSpecified=true;result.changeDisplay=true;
        } else if(argument=="--vsync" || argument=="--no-vsync") {
            result.vsync=argument=="--vsync";result.vsyncSpecified=true;result.changeDisplay=true;
        } else if(argument=="--show-fps" || argument=="--hide-fps") {
            result.showFps=argument=="--show-fps";
        } else if(argument=="--debug" || argument=="--no-debug") {
            result.debugTools=argument=="--debug";result.debugSpecified=true;
        } else if(argument=="--unlock-all" || argument=="--no-unlock-all") {
            result.unlockAll=argument=="--unlock-all";result.unlockSpecified=true;
        } else if(argument=="--filter") {
            const auto value=next().u8string();
            if(value=="authored")result.textureFilter=0;
            else if(value=="bilinear")result.textureFilter=1;
            else if(value=="trilinear")result.textureFilter=2;
            else if(value=="anisotropic")result.textureFilter=3;
            else throw std::invalid_argument("--filter must be authored, bilinear, trilinear or anisotropic");
        } else if(argument=="--aa") {
            const auto value=next().u8string();
            if(value=="off")result.antialiasing=0;
            else if(value=="msaa2")result.antialiasing=2;
            else if(value=="msaa4")result.antialiasing=3;
            else if(value=="msaa8")result.antialiasing=4;
            else throw std::invalid_argument("--aa must be off, msaa2, msaa4 or msaa8");
        } else if(argument=="--anisotropy") {
            const auto value=next().u8string();
            if(value=="2" || value=="4" || value=="8" || value=="16")result.anisotropy=std::stoi(value);
            else throw std::invalid_argument("--anisotropy must be 2, 4, 8 or 16");
        } else if(argument=="--skip-movies" || argument=="--play-movies")result.skipMovies=argument=="--skip-movies";
        else if(argument=="--no-configurator")result.skipConfigurator=true;
        else if(argument=="--configure")result.configureOnly=true;
        else if(argument=="--help" || argument=="-h")result.showHelp=true;
        else throw std::invalid_argument("Unknown native application option: "+argument.u8string());
    }
    if(!result.configurationSpecified)result.configuration=result.profileDirectory/"config.ini";
    if(result.showHelp)return result;
    if(result.width && *result.width<320)throw std::invalid_argument("--width must be at least 320");
    if(result.height && *result.height<240)throw std::invalid_argument("--height must be at least 240");
    if(result.configureOnly)return result;
    if(!result.dataSource.empty()) {
        const auto& source=result.dataSource;
        if(result.archiveData) {
            if(!OrdinaryFile(source) || !OrdinaryFile(ProcessChild(source.parent_path(),"ZDIR.BIN")))
                throw std::runtime_error("Selected game archive requires an existing data file and adjacent ZDIR.BIN: "+source.u8string());
        } else if(std::filesystem::is_directory(source)) {
            const auto archivePair=[&](const std::filesystem::path& root) {
                return OrdinaryFile(ProcessChild(root,"ZZDATA.BIN")) && OrdinaryFile(ProcessChild(root,"ZDIR.BIN"));
            };
            if(!LooseGameDirectory(source) && !archivePair(source) && !archivePair(ProcessChild(source,"NFSHP2")))
                throw std::runtime_error("Selected game-data directory has no GLOBAL resources or complete ZZDATA.BIN/ZDIR.BIN pair: "+source.u8string());
        } else if(!OrdinaryFile(source)) {
            throw std::runtime_error("Selected game-data source does not exist or is not an ordinary file/directory: "+source.u8string());
        }
        return result;
    }
    const auto directories=NativeDataDirectories(directory);
    std::filesystem::path incompletePair;
    // Native PC releases use editable, unpacked resources by default. The
    // original archive is a fallback for installations without extraction,
    // or an explicit --zzdata request. Preserve hashed unknown-file lookup
    // through the extracted __archive/ZDIR.BIN sidecar in GameResources.
    if(!result.archiveData)for(const auto& candidate:directories)if(LooseGameDirectory(candidate)) {
        result.dataSource=candidate;return result;
    }
    // Archive selection is still available for an archive-only installation.
    for(const auto& candidate:directories) {
        const auto index=ProcessChild(candidate,"ZDIR.BIN");
        const auto data=ProcessChild(candidate,"ZZDATA.BIN");
        if(OrdinaryFile(index) && OrdinaryFile(data)) {
            result.dataSource=data;result.archiveData=true;return result;
        }
        if(incompletePair.empty() && (OrdinaryFile(index) || OrdinaryFile(data)))incompletePair=candidate;
    }
    if(!incompletePair.empty())throw std::runtime_error("Incomplete game archive in "+incompletePair.u8string()+
        ": ZDIR.BIN and ZZDATA.BIN must both be present");
    throw std::runtime_error("Game data not found beside "+executable.u8string()+
        ". Put NFSHP2.exe beside ZDIR.BIN and ZZDATA.BIN or in the extracted @ZZDATA directory. "
        "An explicit source may be selected with --zzdata <ZZDATA.BIN> or --data <directory>.");
}
}

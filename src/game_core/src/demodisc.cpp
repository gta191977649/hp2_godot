#include "DemoDisc.hpp"
#include "Flags.hpp"
#include "Timer.hpp"
#include "CarInfo.hpp"
#include <stdexcept>

DemoDiscManager TheDemoDiscManager;

DemoDiscManager::DemoDiscManager()
    :parameters_{0,0,0,60,600,1,10,0},activation_(0),
     initialRealTime_(0),lastRealTime_(0),endReason_(-1),exitRequested_(false)
{
}

void DemoDiscManager::Init(const GameTime& clock,ApplicationLaunchOptions& launch,
    const DemoDiscConfiguration& configuration)
{
    // The console's SDK argument decoders and optical-disc probe are retired.
    // Retain its actual configuration activation path and the same content
    // policy for native launch, loading screens and CarInfo publication.
    if (configuration.useTweakables) {
        parameters_.language=configuration.language;
        parameters_.aspect=configuration.aspect;
        parameters_.playMode=configuration.playMode;
        parameters_.inactiveTimeoutSeconds=configuration.inactiveTimeoutSeconds;
        parameters_.totalTimeoutSeconds=configuration.totalTimeoutSeconds;
        parameters_.masterVolumeScale=configuration.masterVolumeScale;
        activation_=1;
    }
    if (activation_!=0) {
        // PAL100818 is a jump to the real language-file transition at
        // 21E0D0. Its resource/OnChange graph must be connected before this
        // optional configured-demo branch can complete on Windows.
        if (parameters_.language>=1 && parameters_.language<=4)
            throw std::logic_error("Configured demo language needs the canonical language transition");
        launch.skipFrontend=0;
    }
    initialRealTime_=clock.realTimer;
    lastRealTime_=clock.realTimer;
}

void DemoDiscManager::ForceActive(const GameTime& clock,ApplicationLaunchOptions& launch,
    const DemoDiscConfiguration& configuration)
{
    activation_=1;
    Init(clock,launch,configuration);
}

bool DemoDiscManager::IsAttractMode() const noexcept
{
    return parameters_.playMode==1;
}

hp2::CarInfoAvailability DemoDiscManager::Availability() const noexcept
{
    return activation_!=0 ? hp2::CarInfoAvailability::DemoDisc : hp2::CarInfoAvailability::FullGame;
}

void DemoDiscManager::SetEndReason(std::int32_t reason) noexcept
{
    endReason_=reason;
}

void DemoDiscManager::EndDemo() noexcept
{
    if (endReason_==-1)SetEndReason(5);
    exitRequested_=true;
}

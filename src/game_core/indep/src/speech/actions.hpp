#ifndef NFSHP2_SPEECH_ACTIONS_HPP
#define NFSHP2_SPEECH_ACTIONS_HPP

#include "indep/src/speech/mgr.hpp"
#include "indep/src/speech/SEDWedge.hpp"
#include "bware/inc/bMath.hpp"
#include <initializer_list>

/* Game-side message arguments are retained in order. Resolution and
 * PCM playback belong to the explicit native SDK adaptation boundary. */
inline WorldSpeechResult worldSpeechEnqueueAction(WorldAudioMessages& messages,
    uint32_t category, std::initializer_list<uint32_t> parameters)
{
    const WorldAudioMessagesResult result = worldAudioMessagesEnqueue(&messages,
        category, parameters.begin(), parameters.size());
    return result == WORLD_AUDIO_MESSAGES_OK ? WORLD_SPEECH_OK : WORLD_SPEECH_AUDIO_FAILURE;
}



/* PAL 0x20daa0..0x20dacc; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchHelicopterNA(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 202u, {static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20dad0..0x20dafc; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchRoadBlockNA(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 201u, {static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20db00..0x20db28; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchBeOnLookout(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 203u, {static_cast<uint32_t>(1u), static_cast<uint32_t>(context.location)});
}

/* PAL 0x20db28..0x20db90; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechUpdateReckless(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 50u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL 0x20db90..0x20dbe0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechTacticCutoff(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 23u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20dbe0..0x20dc30; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechTacticDivert(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 24u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20dc30..0x20dc5c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechTacticHitInformal(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 28u, {static_cast<uint32_t>(ids.TargetCopId(context))});
}

/* PAL 0x20dc60..0x20dcc8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechTacticHitFormal(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 25u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.TargetToId(context))});
}

/* PAL 0x20dcc8..0x20dce8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSupervisorRotorOK(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 96u, {});
}

/* PAL 0x20dce8..0x20dd50; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSuspectLost(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 63u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL 0x20dd50..0x20dd7c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSupervisorConfirm(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 180u, {static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20dd80..0x20ddec; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSuspectFound(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 62u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(context.location)});
}

/* PAL 0x20ddf0..0x20de58; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSuspectCrashTraffic(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 61u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL 0x20de58..0x20dea8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechFlatTire(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 177u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20dea8..0x20df10; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSuspectCrash(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 58u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL 0x20df10..0x20df30; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSuperBackOff(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 100u, {});
}

/* PAL 0x20df30..0x20df60; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSupervisorRoadBlockOK(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 97u, {static_cast<uint32_t>(ids.ToId(context)), static_cast<uint32_t>(1u)});
}

/* PAL 0x20df60..0x20df90; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSupervisorSpikeStripOK(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 98u, {static_cast<uint32_t>(ids.ToId(context)), static_cast<uint32_t>(1u)});
}

/* PAL 0x20df90..0x20dfb0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSupervisorInitiates(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 101u, {});
}

/* PAL 0x20dfb0..0x20e000; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSuperRequest(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 57u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20e000..0x20e068; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSpikeStripTireBlow(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 37u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL 0x20e068..0x20e120; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSpikeStripSetRight(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    if ((context.copLocation == 0u)) {
        return worldSpeechEnqueueAction(messages, 175u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
    }
    return worldSpeechEnqueueAction(messages, 39u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(context.copLocation)});
}

/* PAL 0x20e120..0x20e1d8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSpikeStripSetLeft(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    if ((context.copLocation == 0u)) {
        return worldSpeechEnqueueAction(messages, 174u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
    }
    return worldSpeechEnqueueAction(messages, 38u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(context.copLocation)});
}

/* PAL 0x20e1d8..0x20e290; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSpikeStripSet(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    if ((context.copLocation == 0u)) {
        return worldSpeechEnqueueAction(messages, 173u, {static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(ids.FromCopId(context))});
    }
    return worldSpeechEnqueueAction(messages, 40u, {static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(context.copLocation)});
}

/* PAL 0x20e290..0x20e2f8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRoadBlockSetting(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 52u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20e2f8..0x20e360; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSpikeStripRequest(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 32u, {static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20e360..0x20e3c8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSpikeStripFailure(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 36u, {static_cast<uint32_t>(ids.TargetFromCopId(context)), static_cast<uint32_t>(ids.TargetCopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL 0x20e3c8..0x20e3e8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRotorSpikeFailed(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 181u, {});
}

/* PAL 0x20e3e8..0x20e40c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRotorLocation(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 109u, {static_cast<uint32_t>(context.location)});
}

/* PAL 0x20e410..0x20e434; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRotorTraffic(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 121u, {static_cast<uint32_t>(1u)});
}

/* PAL 0x20e438..0x20e468; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRotorChatter(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 112u, {static_cast<uint32_t>((context.rotorChatterVariant == 0 ? 4 : 128))});
}

/* PAL 0x20e468..0x20e4d0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRotorRequest(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 53u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL 0x20e4d0..0x20e588; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRoadBlockSet(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    if ((context.copLocation == 0u)) {
        return worldSpeechEnqueueAction(messages, 172u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
    }
    return worldSpeechEnqueueAction(messages, 35u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(context.copLocation)});
}

/* PAL 0x20e588..0x20e5f0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRoadblockRequest(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 31u, {static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20e5f0..0x20e658; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRoadBlockFailureThrough(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 34u, {static_cast<uint32_t>(ids.TargetFromCopId(context)), static_cast<uint32_t>(ids.TargetCopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL 0x20e658..0x20e6c0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRoadBlockFailureAvoid(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 33u, {static_cast<uint32_t>(ids.TargetFromCopId(context)), static_cast<uint32_t>(ids.TargetCopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL 0x20e6c0..0x20e728; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRadioArrest(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 178u, {static_cast<uint32_t>(context.arrestType), static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20e728..0x20e7c0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRacerCaught(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    if ((context.otherCopId == 1u)) {
        return worldSpeechEnqueueAction(messages, 194u, {static_cast<uint32_t>(ids.CopId(context))});
    }
    if ((context.otherCopId != 1u) && (context.otherCopId == 2u)) {
        return worldSpeechEnqueueAction(messages, 195u, {static_cast<uint32_t>(ids.CopId(context))});
    }
    return worldSpeechEnqueueAction(messages, 0u, {static_cast<uint32_t>(context.arrestType), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20e7c0..0x20e85c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechPursuitUpdateVehicleIdSpecific(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 65u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(1u), static_cast<uint32_t>(context.vehicleColor), static_cast<uint32_t>(context.exactVehicle), static_cast<uint32_t>(context.approximateVehicle)});
}

/* PAL 0x20e860..0x20e8cc; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechPursuitUpdateSubjectLocation(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 56u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(context.location)});
}

/* PAL 0x20e8d0..0x20e940; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechPursuitUpdateSpeed(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 54u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(context.vehicleSpeed), static_cast<uint32_t>(context.location)});
}

/* PAL 0x20e940..0x20e96c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechPursuitCopHit(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 22u, {static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20e970..0x20e99c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchTowConfirm(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 81u, {static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20e9a0..0x20e9c0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchRotorDeployed(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 72u, {});
}

/* PAL 0x20e9c0..0x20ea28; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchBeginVehicleIdSpecific(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 92u, {static_cast<uint32_t>(ids.ToId(context)), static_cast<uint32_t>(context.vehicleColor), static_cast<uint32_t>(context.exactVehicle)});
}

/* PAL 0x20ea28..0x20ea54; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchConfirm(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 13u, {static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20ea58..0x20ea84; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchBeginPursuitSpeed(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 91u, {static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20ea88..0x20eab8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchAmbulanceConfirm(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 93u, {static_cast<uint32_t>(ids.ToId(context)), static_cast<uint32_t>(1u)});
}

/* PAL 0x20eab8..0x20eb0c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDirection(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 48u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(context.direction)});
}

/* PAL 0x20eb10..0x20eb64; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechCurrentSpeed(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 47u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(context.vehicleSpeed)});
}

/* PAL 0x20eb68..0x20ebd0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechCrashBackup(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 198u, {static_cast<uint32_t>(ids.TargetFromCopId(context)), static_cast<uint32_t>(ids.TargetCopId(context)), static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20ebd0..0x20ec20; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechCrashTow(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 45u, {static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.FromCopId(context))});
}

/* PAL 0x20ec20..0x20ec70; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechCrashAmbulance(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 46u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20ec70..0x20ecc0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechCrash(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 44u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20ecc0..0x20ecec; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechChatterTraffic(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 20u, {static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20ecf0..0x20ed1c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechChatterGeneral(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 21u, {static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20ed20..0x20ed70; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechCopPursuitAvoidance(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 10u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20ed70..0x20edc0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBreakPursuit(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 43u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20edc0..0x20ee28; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechRequestAnotherRoadBlock(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 189u, {static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20ee28..0x20ee90; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechCarStopped(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 188u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(context.exactVehicle)});
}

/* PAL 0x20ee90..0x20eee0; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechCopBackOff(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 187u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20eee0..0x20ef00; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechStandByForSuper(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 185u, {});
}

/* PAL 0x20ef00..0x20ef20; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechSuperTerminate(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 179u, {});
}

/* PAL 0x20ef20..0x20ef40; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchTerminate(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 186u, {});
}

/* PAL 0x20ef40..0x20ef6c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchIdRequest(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 67u, {static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20ef70..0x20efcc; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchSpikeStripSet(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    if (context.copLocation != 0 && vectorDefaultRandomBounded(2) == 0) {
        return worldSpeechEnqueueAction(messages, 170u, {static_cast<uint32_t>(context.copLocation)});
    }
    return worldSpeechEnqueueAction(messages, 73u, {});
}

/* PAL 0x20efd0..0x20f02c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchRoadBlockUpdate(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    if (context.copLocation != 0 && vectorDefaultRandomBounded(2) == 0) {
        return worldSpeechEnqueueAction(messages, 71u, {static_cast<uint32_t>(context.copLocation)});
    }
    return worldSpeechEnqueueAction(messages, 171u, {});
}

/* PAL 0x20f030..0x20f05c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchBeginPursuitHitRunCop(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 86u, {static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20f060..0x20f08c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchBeginPursuitHitRunTraffic(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 85u, {static_cast<uint32_t>(ids.ToId(context))});
}

/* PAL 0x20f0b8..0x20f0dc; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechDispatchBeginPursuitReckless(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    return worldSpeechEnqueueAction(messages, 84u, {static_cast<uint32_t>(context.location)});
}

/* PAL 0x20f0e0..0x20f198; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitReckless(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 12u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(context.location), static_cast<uint32_t>(8u), static_cast<uint32_t>(context.approximateVehicle), static_cast<uint32_t>(context.vehicleColor), static_cast<uint32_t>(ids.ToId(context)), static_cast<uint32_t>(context.exactVehicle)});
}

/* PAL 0x20f198..0x20f1e8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitNoId(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 9u, {static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.FromCopId(context))});
}

/* PAL 0x20f1e8..0x20f238; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitErratic(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 6u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20f238..0x20f28c; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitSpeed(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 15u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(context.vehicleSpeed)});
}

/* PAL 0x20f290..0x20f2fc; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitRacing(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 8u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(context.racingVariant), static_cast<uint32_t>(context.location)});
}

/* PAL 0x20f300..0x20f380; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitAgain(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 199u, {static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(context.exactVehicle), static_cast<uint32_t>(context.vehicleColor)});
}

/* PAL 0x20f380..0x20f418; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitIdSpecific(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 17u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context)), static_cast<uint32_t>(context.vehicleColor), static_cast<uint32_t>(context.exactVehicle)});
}

/* PAL 0x20f418..0x20f468; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitGeneral(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 7u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20f468..0x20f4bc; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitHitRunTraffic(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 14u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(context.location)});
}

/* PAL 0x20f4c0..0x20f540; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitIdGeneral(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 16u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(context.approximateVehicle), static_cast<uint32_t>(context.vehicleColor)});
}

/* PAL 0x20f540..0x20f594; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBeginPursuitCopHitRun(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 11u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(context.location)});
}

/* PAL 0x20f598..0x20f5ec; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBackupRequestLocation(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 42u, {static_cast<uint32_t>(context.location), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.FromCopId(context))});
}

/* PAL 0x20f5f0..0x20f640; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBackupRequest(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 41u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context))});
}

/* PAL 0x20f640..0x20f6a8; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechBackupConfirm(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 19u, {static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.TargetToId(context))});
}

/* PAL 0x20f6a8..0x20f710; individual definition in actions.hpp. */
inline WorldSpeechResult worldSpeechAirborne(WorldAudioMessages& messages,
    [[maybe_unused]] const WorldSpeechIdentityMap& identityMap,
    [[maybe_unused]] const WorldSpeechContext& context)
{
    const WorldSpeechIdView ids{identityMap};
    return worldSpeechEnqueueAction(messages, 51u, {static_cast<uint32_t>(ids.FromCopId(context)), static_cast<uint32_t>(ids.CopId(context)), static_cast<uint32_t>(ids.Supervisor(context))});
}

/* PAL20F090..20F0B8 adds this location gate relative to beta. */
inline WorldSpeechResult worldSpeechDispatchBeginPursuitRacing(WorldAudioMessages& messages,
    const WorldSpeechIdentityMap&, const WorldSpeechContext& context)
{
    if (!context.location) return WORLD_SPEECH_OK;
    return worldSpeechEnqueueAction(messages, 83u, {static_cast<uint32_t>(context.location)});
}

/* Exact empty do-action20F710; its actual vtable STILL uses the normal
 * action.execute wrapper and therefore still starts squelch. The distinct
 * parameterless empty20F718 overload is not that virtual wrapper. */
inline WorldSpeechResult worldSpeechNull(WorldAudioMessages&,
    const WorldSpeechIdentityMap&, const WorldSpeechContext&)
{
    return WORLD_SPEECH_OK;
}

#endif

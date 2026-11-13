#include "indep/src/speech/SEDWedge.hpp"

/* PAL295AB0..295AFC. A missing external timer reports the actual diagnostic
 * and returns0. A present provider is the released SED timer, including its
 * source signed conversion and wrap to the unsigned SDK tick. */
uint32_t spchTick(const SpeechEngineState* engine)
{
    const struct WorldStreamAudio* external=spchGetExtVecs(engine);
    if (external)return (uint32_t)worldStreamAudioTimerGetTick(external);
    spchDebugPrint(engine,"gettick vector not installed.");
    return 0;
}

#include "indep/src/speech/SEDWedge.hpp"

/* PAL2908C8: the same installed external-vector owner, never a copy of its
 * timer or another set of function pointers. Native null is an invalid
 * capability and therefore has no installed external provider. */
struct WorldStreamAudio* spchGetExtVecs(const SpeechEngineState* engine)
{
    return engine ? engine->externalOwner : NULL;
}

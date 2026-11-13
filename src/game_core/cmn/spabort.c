#include "indep/src/speech/SEDWedge.hpp"
#include "indep/inc/Flags.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* PAL294FA8: format the diagnostic; decorate with the installed file/line;
 * dispatch the actual SED abort role (console output), or save the SDK log
 * and terminate when there is no external provider. Native fatal exit is
 * EXIT_FAILURE instead of PAL's exit(0), so a failing run is never success.
 * All original buffers are bounded; no callback or gameplay owner is added. */
void spchAbortMessage(SpeechEngineState* engine,const char* format,...)
{
    char message[512];
    char decorated[1024];
    if (format) {
        va_list arguments;
        va_start(arguments,format);
        (void)vsnprintf(message,sizeof(message),format,arguments);
        va_end(arguments);
        message[sizeof(message)-1]='\0';
    } else message[0]='\0';
    if (engine && engine->diagnosticFile && engine->diagnosticLine)
        (void)snprintf(decorated,sizeof(decorated),"%sFILE %s LINE %d\n",message,
            engine->diagnosticFile,(int)(int32_t)engine->diagnosticLine);
    else (void)snprintf(decorated,sizeof(decorated),"%s",message);
    if (spchGetExtVecs(engine)) {
        fputs(decorated,stderr);
        return;
    }
    if (engine)spchEventLogSave(engine);
    hp2NativeRecordFailure(decorated);
    exit(EXIT_FAILURE);
}

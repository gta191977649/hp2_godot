#include "indep/src/speech/SEDWedge.hpp"

#include <stdarg.h>
#include <stdio.h>

/* PAL294F18 formats a512-byte message before selecting the external debug
 * provider or the platform console. Bounded vsnprintf replaces the source
 * unsafe vsprintf, without inventing any gameplay result. The installed
 * SED debug provider2059D0 has no side effects and returns0; that actual
 * source behavior is folded here, with no empty runtime callback shim. */
void spchDebugPrint(const SpeechEngineState* engine,const char* format,...)
{
    char message[512];
    if (format) {
        va_list arguments;
        va_start(arguments,format);
        (void)vsnprintf(message,sizeof(message),format,arguments);
        va_end(arguments);
        message[sizeof(message)-1]='\0';
    } else message[0]='\0';
    if (!spchGetExtVecs(engine))fputs(message,stdout);
}

#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>

namespace hp2 {

// XAudio2 needs an initialized COM apartment, not an exclusive MTA.
// SDL controller discovery may already own an STA on this same thread.
// Both S_OK and S_FALSE acquire one reference for the audio device to
// release after its voices/engine. A failed attempt acquires none.
inline HRESULT InitializeNativeAudioCom() noexcept
{
    HRESULT result=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(result==RPC_E_CHANGED_MODE)
        result=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    // A second conflict (for example, an NTA context) is still a failure.
    // Do not borrow another owner's apartment or undo its initialization.
    return result;
}

} // namespace hp2
#endif

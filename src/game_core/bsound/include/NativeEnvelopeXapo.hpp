#pragma once

#ifdef _WIN32
#include "bsound/include/NativeEnvelope.hpp"
#include <windows.h>
#include <objbase.h>
#include <xapo.h>
#include <cstring>
#include <new>

namespace hp2 {

// Use each supported SDK's public global typedef. Do not introduce a local
// struct or deduce this type from a COM member-function calling convention.
#if defined(__MINGW32__)
using NativeXapoLock=::XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS;
#else
using NativeXapoLock=::XAPO_LOCKFORPROCESS_PARAMETERS;
#endif

class NativeGainXapo final : public IXAPO {
public:
    NativeGainXapo(NativeOutputClock& clock,NativeEnvelopeParameters parameters,bool outputClock=false) noexcept :
        clock_(clock),output_(clock,parameters),isClock_(outputClock) {}

    void KeyOn() noexcept { output_.KeyOn(); }
    void KeyOff(bool streamStop) noexcept { output_.KeyOff(streamStop); }
    bool Finished() const noexcept { return output_.Finished(); }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** object) override {
        if (!object)return E_POINTER;
        *object=nullptr;
        if (!IsEqualIID(id,__uuidof(IUnknown)) && !IsEqualIID(id,__uuidof(IXAPO)))return E_NOINTERFACE;
        *object=static_cast<IXAPO*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return references_.fetch_add(1,std::memory_order_relaxed)+1; }
    ULONG STDMETHODCALLTYPE Release() override {
        const auto left=references_.fetch_sub(1,std::memory_order_acq_rel)-1;
        if (!left)delete this;
        return left;
    }
    HRESULT STDMETHODCALLTYPE GetRegistrationProperties(XAPO_REGISTRATION_PROPERTIES** result) override {
        if (!result)return E_POINTER;
        *result=static_cast<XAPO_REGISTRATION_PROPERTIES*>(CoTaskMemAlloc(sizeof(XAPO_REGISTRATION_PROPERTIES)));
        if (!*result)return E_OUTOFMEMORY;
        auto& props=**result;props={};
        props.clsid=isClock_ ?
            CLSID{0x4866b1c8,0x7d72,0x4f9a,{0x83,0xda,0xe2,0x91,0xae,0x8a,0x35,0x19}}:
            CLSID{0x241e9617,0x4278,0x46bb,{0x9e,0x55,0xca,0x82,0x87,0x59,0xad,0x12}};
        constexpr wchar_t name[]=L"NFSHP2 native output gain and clock";
        constexpr wchar_t author[]=L"Native PC audio implementation";
        std::memcpy(props.FriendlyName,name,sizeof(name));
        std::memcpy(props.CopyrightInfo,author,sizeof(author));
        props.MajorVersion=1;
        props.Flags=XAPO_FLAG_CHANNELS_MUST_MATCH|XAPO_FLAG_FRAMERATE_MUST_MATCH|
            XAPO_FLAG_BITSPERSAMPLE_MUST_MATCH|XAPO_FLAG_BUFFERCOUNT_MUST_MATCH|
            XAPO_FLAG_INPLACE_SUPPORTED|XAPO_FLAG_INPLACE_REQUIRED;
        props.MinInputBufferCount=props.MaxInputBufferCount=1;
        props.MinOutputBufferCount=props.MaxOutputBufferCount=1;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE IsInputFormatSupported(const WAVEFORMATEX* output,
        const WAVEFORMATEX* input,WAVEFORMATEX** closest) override { return Formats(output,input,closest); }
    HRESULT STDMETHODCALLTYPE IsOutputFormatSupported(const WAVEFORMATEX* input,
        const WAVEFORMATEX* output,WAVEFORMATEX** closest) override { return Formats(input,output,closest); }
    HRESULT STDMETHODCALLTYPE Initialize(const void* data,UINT32 bytes) override {
        return !data && !bytes ? S_OK:E_INVALIDARG;
    }
    void STDMETHODCALLTYPE Reset() override {
        // XAudio2 may reset DSP state independently of a new source PLAY.
        // The native gain has no delay line; only explicit key-on restarts it.
    }
    HRESULT STDMETHODCALLTYPE LockForProcess(UINT32 inputs,const NativeXapoLock* input,
        UINT32 outputs,const NativeXapoLock* output) override {
        if (locked_)return S_OK;
        if (inputs!=1 || outputs!=1 || !input || !output ||
            !input->MaxFrameCount || input->MaxFrameCount!=output->MaxFrameCount ||
            Formats(input->pFormat,output->pFormat,nullptr)!=S_OK)return E_INVALIDARG;
        channels_=input->pFormat->nChannels;maximum_=input->MaxFrameCount;locked_=true;
        return S_OK;
    }
    void STDMETHODCALLTYPE UnlockForProcess() override { locked_=false;channels_=0;maximum_=0; }
    void STDMETHODCALLTYPE Process(UINT32 inputs,const XAPO_PROCESS_BUFFER_PARAMETERS* input,
        UINT32 outputs,XAPO_PROCESS_BUFFER_PARAMETERS* output,BOOL enabled) override {
        if (!locked_ || inputs!=1 || outputs!=1 || !input || !output ||
            input->ValidFrameCount>maximum_) {
            if (outputs && output) { output->ValidFrameCount=0;output->BufferFlags=XAPO_BUFFER_SILENT; }
            return;
        }
        const auto frames=input->ValidFrameCount;
        output->ValidFrameCount=frames;output->BufferFlags=input->BufferFlags;
        const bool valid=input->BufferFlags==XAPO_BUFFER_VALID;
        if (!output->pBuffer || (valid && !input->pBuffer)) {
            output->BufferFlags=XAPO_BUFFER_SILENT;return;
        }
        if (valid && input->pBuffer!=output->pBuffer)
            std::memcpy(output->pBuffer,input->pBuffer,static_cast<std::size_t>(frames)*channels_*sizeof(float));
        if (isClock_) {
            // The master runs after every source/submix for this output
            // interval, including silence and paused source voices.
            clock_.frames.fetch_add(frames,std::memory_order_release);
        } else if (enabled) {
            const bool audible=output_.Process(static_cast<float*>(output->pBuffer),frames,channels_,valid);
            output->BufferFlags=audible ? XAPO_BUFFER_VALID:XAPO_BUFFER_SILENT;
        }
    }
    UINT32 STDMETHODCALLTYPE CalcInputFrames(UINT32 frames) override { return frames; }
    UINT32 STDMETHODCALLTYPE CalcOutputFrames(UINT32 frames) override { return frames; }

private:
    std::atomic<ULONG> references_{1};
    NativeOutputClock& clock_;
    NativeEnvelopeOutput output_;
    bool isClock_{},locked_{};
    UINT32 channels_{},maximum_{};
    ~NativeGainXapo()=default;

    static bool FloatFormat(const WAVEFORMATEX* format) noexcept {
        if (!format || (format->nChannels!=1 && format->nChannels!=2) ||
            format->nSamplesPerSec!=NativeEnvelope::OutputRate || format->wBitsPerSample!=32 ||
            format->nBlockAlign!=format->nChannels*sizeof(float) ||
            format->nAvgBytesPerSec!=format->nSamplesPerSec*format->nBlockAlign)return false;
        if (format->wFormatTag==WAVE_FORMAT_IEEE_FLOAT)return true;
        if (format->wFormatTag!=WAVE_FORMAT_EXTENSIBLE ||
            format->cbSize<sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX))return false;
        const auto& extended=*reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(format);
        constexpr GUID floatId{WAVE_FORMAT_IEEE_FLOAT,0,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}};
        return extended.Samples.wValidBitsPerSample==32 && IsEqualGUID(extended.SubFormat,floatId);
    }
    static HRESULT Formats(const WAVEFORMATEX* fixed,const WAVEFORMATEX* requested,WAVEFORMATEX** closest) {
        if (closest)*closest=nullptr;
        if (FloatFormat(fixed) && FloatFormat(requested) && fixed->nChannels==requested->nChannels)return S_OK;
        if (closest) {
            *closest=static_cast<WAVEFORMATEX*>(CoTaskMemAlloc(sizeof(WAVEFORMATEX)));
            if (!*closest)return E_OUTOFMEMORY;
            auto& format=**closest;format={};format.wFormatTag=WAVE_FORMAT_IEEE_FLOAT;
            format.nChannels=fixed && (fixed->nChannels==1 || fixed->nChannels==2) ? fixed->nChannels:WORD{2};
            format.nSamplesPerSec=NativeEnvelope::OutputRate;format.wBitsPerSample=32;
            format.nBlockAlign=static_cast<WORD>(format.nChannels*sizeof(float));
            format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        }
        // The public XAPO format error has this documented HRESULT on both
        // SDKs; the older MinGW xapo.h omits its symbolic macro.
        return static_cast<HRESULT>(0x88970001u);
    }
};

} // namespace hp2
#endif

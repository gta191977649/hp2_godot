// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#include "movie_audio.hpp"
#include <stdexcept>

namespace hp2::port::audio {
namespace {
void big(std::vector<std::uint8_t>& bytes,std::size_t o,std::uint32_t value) {
    for(unsigned i=0;i<4;++i) bytes[o+i]=static_cast<std::uint8_t>(value>>(8*(3-i)));
}
}
MovieAudio decode_movie_audio(const std::vector<std::uint8_t>& original_audio_bytes) {
    if(original_audio_bytes.empty() || original_audio_bytes.size()%(16*2))
        throw std::runtime_error("movie stereo ADPCM stream missing or not frame-aligned");
    // An in-memory codec envelope only: original AVI audio does not contain a VAG header.
    std::vector<std::uint8_t> envelope(48,0);
    big(envelope,0,0x56414770);big(envelope,12,static_cast<std::uint32_t>(original_audio_bytes.size()));
    big(envelope,16,48000);big(envelope,36,0x56414778);big(envelope,44,2);
    envelope.insert(envelope.end(),original_audio_bytes.begin(),original_audio_bytes.end());
    MovieAudio result;
    //001AE608 passes runtime buffer65536;002611C0 stereo mode halves it to32768.
    result.pcm=decode_vag(envelope,0x8000);
    // Movie category=-1, direct amplitude0x3fff, objectfactor1 => full nominal gain.
    result.pcm.nominal_gain=1.0f;
    return result;
}
} // namespace hp2::port::audio

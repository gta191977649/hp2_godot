#include "bsound/include/bSample.h"
#include "bsound/include/bSound.h"
#include "bware/inc/bWare.hpp"

#include <cstring>

bSegment::bSegment(const bSound& sound)
{
    hp2::Require(sound.GetStatus()==1,"bSegment requires the initialized sound library");
    initialized_=false;
    offset_=bytes_=cursor_=flags_=0;
    filename_[0]='\0';
}

void bSegment::Initialize(const char* filename,uint32_t offset,uint32_t bytes,uint32_t flags)
{
    hp2::Require(!initialized_,"bSegment is already initialized");
    hp2::Require(filename!=nullptr,"bSegment filename is absent");
    // PAL276424/45C uses the same bounded255-byte copy twice. Native storage
    // has a zero final byte from construction; no console pointer is kept.
    std::strncpy(filename_.data(),filename,255);
    initialized_=true;
    offset_=offset;
    bytes_=bytes;
    cursor_=0;
    flags_=flags;
    std::strncpy(filename_.data(),filename,255);
}

void bSegment::Cleanup()
{
    initialized_=false;
    offset_=bytes_=cursor_=flags_=0;
    filename_[0]='\0';
}

uint32_t bSegment::GetOffset() const noexcept
{
    return offset_;
}

uint32_t bSegment::GetSize() const noexcept
{
    return bytes_;
}

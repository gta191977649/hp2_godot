#include "bware/inc/bChunk.hpp"
#include "bware/inc/bFile.hpp"
#include "bware/inc/bWare.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <memory>

namespace hp2 {
namespace {
constexpr std::uint32_t CompTag=0x504d4f43u;

void putWord(std::uint8_t* output,std::uint32_t value)
{
    output[0]=static_cast<std::uint8_t>(value);
    output[1]=static_cast<std::uint8_t>(value>>8);
}

std::uint32_t decompress(const std::uint8_t* input,std::uint8_t* output,
    std::size_t inputBytes,std::size_t outputBytes,bool checked)
{
    if (!input || !output)return 0;
    if (checked)Require(inputBytes>=16,"COMP file header is truncated");
    if (ReadU32(input)!=CompTag)return 0;
    const auto packedBytes=ReadU32(input+12);
    if (checked)Require(packedBytes>=16 && packedBytes<=inputBytes,"COMP stored length is outside its file");
    if (ReadU16(input+6)==1) {
        const auto count=packedBytes-16u;
        if (checked)Require(count<=outputBytes,"Stored COMP data exceeds its destination");
        std::memmove(output,input+16,count);
        return count;
    }
    std::size_t at=16,written=0;
    std::uint32_t flags=1;
    // The original terminates at packedBytes, not the advertised output
    // length. Its16-token fast path and one-token tail share this sentinel.
    while (at<packedBytes) {
        if (flags==1) {
            if (checked)Require(Inside(at,2,packedBytes),"COMP token flags are truncated");
            flags=ReadU16(input+at)|0x10000u;at+=2;
        }
        if (flags&1u) {
            if (checked)Require(Inside(at,2,packedBytes),"COMP back-reference is truncated");
            const auto first=input[at++],second=input[at++];
            const auto distance=static_cast<std::size_t>(((first&0xf0u)<<4)|second);
            const auto count=static_cast<std::size_t>((first&15u)+3u);
            if (checked)Require(distance && distance<=written && Inside(written,count,outputBytes),
                "COMP back-reference exceeds its decoded history or destination");
            for (std::size_t i=0;i<count;++i) {
                output[written]=output[written-distance];++written;
            }
        } else {
            if (checked)Require(at<packedBytes && written<outputBytes,"COMP literal exceeds its file or destination");
            output[written++]=input[at++];
        }
        flags>>=1;
    }
    return static_cast<std::uint32_t>(written);
}
}

std::uint32_t LZDecompress(const std::uint8_t* input,std::uint8_t* output)
{
    return decompress(input,output,0,0,false);
}

std::uint32_t LZCompress(const std::uint8_t* input,std::uint32_t inputBytes,std::uint8_t* output)
{
    if (!input || !inputBytes || !output)return 0;
    // The source SDK obtains a temporary128KiB table. Native indices replace
    // PS2 addresses; initializing invalid entries avoids indeterminate C++
    // pointer reads. The original rejects entries outside input/history too.
    auto dictionary=std::make_unique<std::array<std::uint32_t,32768>>();
    dictionary->fill(std::numeric_limits<std::uint32_t>::max());
    PutU32(output,CompTag);putWord(output+4,0x1001u);putWord(output+6,0);
    PutU32(output+8,inputBytes);PutU32(output+12,0);
    std::size_t at=0,out=18,flagOffset=16;
    std::uint32_t flags=0xffff0000u;
    for (;;) {
        // This check follows flag-word reservation, including the otherwise
        // unused word after an exact16-token group, as in the whole source.
        if (out>inputBytes) {
            std::memmove(output+16,input,inputBytes);
            putWord(output+6,1);out=static_cast<std::size_t>(inputBytes)+16;
            break;
        }
        if (at==inputBytes) {
            while (flags&0xffff0000u)flags>>=1;
            putWord(output+flagOffset,flags);
            if (flagOffset+2==out)out-=2;
            break;
        }
        const auto batch=inputBytes-at>=288 ? 16u : 1u;
        for (unsigned token=0;token<batch;++token) {
            std::size_t length=0,distance=0;
            if (inputBytes-at>=18) {
                std::uint32_t key=(static_cast<std::uint32_t>(input[at])<<4)^input[at+1];
                key=(key<<4)^input[at+2];
                const auto hash=((key*0x9e5fu)>>4)&0x7fffu;
                auto& slot=(*dictionary)[hash];const auto previous=slot;
                slot=static_cast<std::uint32_t>(at);
                if (previous<at && at-previous<0x1000u) {
                    while (length<18 && input[previous+length]==input[at+length])++length;
                    if (length>=3)distance=at-previous;
                }
            }
            if (distance) {
                output[out++]=static_cast<std::uint8_t>(((distance&0xf00u)>>4)|(length-3));
                output[out++]=static_cast<std::uint8_t>(distance);at+=length;
            } else {
                output[out++]=input[at++];flags&=0xfffeffffu;
            }
            flags>>=1;
        }
        if (!(flags&0xffff0000u)) {
            putWord(output+flagOffset,flags);flags=0xffff0000u;
            flagOffset=out;out+=2;
        }
    }
    PutU32(output+12,static_cast<std::uint32_t>(out));
    return static_cast<std::uint32_t>(out);
}

std::vector<std::uint8_t> DecodeComp(const FileRange& resource)
{
    Require(resource.Size()>=16 && resource.Size()<=(128u<<20),"COMP input extent is invalid");
    return DecodeComp(resource.Read(0,static_cast<std::size_t>(resource.Size())));
}

std::vector<std::uint8_t> DecodeComp(const std::vector<std::uint8_t>& input)
{
    Require(input.size()>=16 && input.size()<=(128u<<20),"COMP input extent is invalid");
    Require(ReadU32(input.data())==CompTag,"Resource has no COMP header");
    Require(ReadU16(input.data()+4)==0x1001u && ReadU16(input.data()+6)<=1,
        "Unsupported EA COMP variant or storage mode");
    const auto count=ReadU32(input.data()+8);
    Require(count && count<=(128u<<20),"COMP decoded length is invalid");
    std::vector<std::uint8_t> output(count);
    Require(decompress(input.data(),output.data(),input.size(),output.size(),true)==count,
        "COMP decoded size differs from its authored header");
    return output;
}
} // namespace hp2

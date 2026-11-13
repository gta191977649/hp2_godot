#include "indep/inc/Texture.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"

namespace hp2 {

// Complete PAL100A50..100AB0. Both integer conversions retain the engine's
// original rounding rule; fmod/fract would change negative and half values.
float TextureInfo::GetScroll(float time,float rate,std::int32_t mode,float interval)
{
    if (!mode)return 0.0f;
    if (mode==2) {
        Require(interval!=0.0f,"Texture discrete scroll interval is zero");
        time=static_cast<float>(convertFloatToInt32(time/interval));
    }
    const float distance=time*rate;
    return distance-static_cast<float>(convertFloatToInt32(distance));
}

// PAL100AF8/100B40/100B98 operate on the retained 32-bit pixel stream.
// They neither decode a second texture nor modify the palette or alpha
// while replacing colour. Native extents validate the caller's allocation.
void TextureInfo::GetAlphaChannel(std::uint8_t* output,std::size_t outputBytes) const
{
    if (pixelBytes<4)return;
    const auto count=static_cast<std::size_t>(pixelBytes)/4;
    Require(output && outputBytes>=count && pixels.Data() && pixels.size>=count*4,
        "Texture alpha channel exceeds its actual pixel/output storage");
    const auto* input=pixels.Data();
    for (std::size_t index=0;index<count;++index)output[index]=input[index*4+3];
}

void TextureInfo::SetColourChannel(const std::uint8_t* input,std::size_t inputBytes)
{
    if (pixelBytes<4)return;
    const auto count=static_cast<std::size_t>(pixelBytes)/4;
    Require(input && inputBytes>=count*3 && pixels.MutableData() && pixels.size>=count*4,
        "Texture colour update exceeds its actual pixel/input storage");
    auto* output=pixels.MutableData();
    for (std::size_t index=0;index<count;++index) {
        output[index*4]=input[index*3];
        output[index*4+1]=input[index*3+1];
        output[index*4+2]=input[index*3+2];
    }
}

void TextureInfo::GetColourChannel(std::uint8_t* output,std::size_t outputBytes) const
{
    if (pixelBytes<4)return;
    const auto count=static_cast<std::size_t>(pixelBytes)/4;
    Require(output && outputBytes>=count*3 && pixels.Data() && pixels.size>=count*4,
        "Texture colour channel exceeds its actual pixel/output storage");
    const auto* input=pixels.Data();
    for (std::size_t index=0;index<count;++index) {
        output[index*3]=input[index*4];
        output[index*3+1]=input[index*4+1];
        output[index*3+2]=input[index*4+2];
    }
}

} // namespace hp2

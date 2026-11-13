#include "MV3Codec.h"

namespace hp2 {

// PAL238E28..238E64. Base Codec construction precedes the one embedded image.
// C++ member/base lifetime expresses the original ownership without raw ABI
// overlays or a second codec/frame registry.
MV3Codec::MV3Codec() : Codec(), image_() {}

// PAL238E68..238EB4. The embedded DCTImage is released before Codec drops
// the shared clamp reference. Native C++ owns deleting-destructor semantics.
MV3Codec::~MV3Codec() = default;

} // namespace hp2

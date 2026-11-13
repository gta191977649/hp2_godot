#ifndef NFSHP2_NATIVE_RENDER_SETTINGS_HPP
#define NFSHP2_NATIVE_RENDER_SETTINGS_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hp2 {
struct NativeRenderSettings {
    float drawDistanceScale{1.0f};       // World views: 1..4 times authored visibility.
    float mirrorDistanceScale{1.0f};     // Rear view only: 0.5..4 times authored visibility.
};
inline NativeRenderSettings validateNativeRenderSettings(NativeRenderSettings value) noexcept
{
    value.drawDistanceScale=std::isfinite(value.drawDistanceScale) ?
        std::clamp(value.drawDistanceScale,1.0f,4.0f):1.0f;
    value.mirrorDistanceScale=std::isfinite(value.mirrorDistanceScale) ?
        std::clamp(value.mirrorDistanceScale,0.5f,4.0f):1.0f;
    return value;
}

// Configured on the application thread before a frame is built. Gather workers
// borrow these immutable settings for that frame; launcher edits apply at startup.
inline NativeRenderSettings nativeRenderSettingsStorage{};
inline void configureNativeRenderSettings(const NativeRenderSettings& value) noexcept
{
    nativeRenderSettingsStorage=validateNativeRenderSettings(value);
}
inline const NativeRenderSettings& nativeRenderSettings() noexcept
{
    return nativeRenderSettingsStorage;
}
inline float nativeViewDistanceScale(std::int32_t viewId) noexcept
{
    return viewId==3 ? nativeRenderSettings().mirrorDistanceScale :
        viewId==1 || viewId==2 ? nativeRenderSettings().drawDistanceScale:1.0f;
}

} // namespace hp2
#endif

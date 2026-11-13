#ifndef NFSHP2_TRACKMAP_HPP
#define NFSHP2_TRACKMAP_HPP

#include "Track.hpp"
#include "FlailerChunks.hpp"
#include <array>
#include <cstdint>

struct CarMovement;
struct CarFramePlayerState;
struct CarCinematicServices;
struct RaceParameters;
namespace hp2 {
// TrackMap.hpp is the original declaration unit of the individually matched
// TrackMap definitions. Native resource services borrow their sole owners.
struct TrackMapModeInfo {
    std::uint16_t tilt{};
    float perspectiveDepth{};
    std::uint16_t screenX{},screenY{};
    float mapWidth{},mapHeight{};
    std::uint16_t clipX{},clipY{};
    float clipWidth{},clipHeight{},upperExtent{},lowerExtent{};
    bool targetZoom{};
    float markerSize{},markerWidth{},markerHeight{};
    bool followCamera{},localMap{},orientedMarker{};
    float opacity{};
    bool mapEnabled{},borderEnabled{};
};
using TrackMapPoint=std::array<float,2>;
using TrackMapQuad=std::array<TrackMapPoint,4>;
class TrackMap final {
public:
    TrackMap(const TrackResourceInfo* track,CarMovement* car,CarFramePlayerState* player,
        FlailerResources& resources,FlailerRenderManager& renderer,
        CarCinematicServices& cameras,std::int32_t overlayViewNumber,
        const RaceParameters& parameters,bool playerHeading=false);
    bool Valid() const noexcept {return valid_;}
    void SetMode(std::int32_t mode);
    std::int32_t Mode() const noexcept {return configuredMode_;}
    void Render();
    static const std::array<TrackMapModeInfo,11>& Modes();
    TrackMapPoint GetCarUVPosition(const float position[3]) const;
    static TrackMapQuad GetBoxOnMap(const TrackMapPoint& center,std::uint16_t angle,
        float upper,float lower,float left,float right);
    static TrackMapPoint CalculateCentroidOfQuad(const TrackMapQuad& quad);
    static void EvalCubic(float& position,float& velocity,float target,float elapsed);
private:
    const TrackMapModeInfo& CurrentMode() const;
    bool GetCameraDirection(std::array<float,4>& direction);
    bool GetAngle(std::uint16_t& angle);
    void DrawMap(TrackMapPoint& center,TrackMapQuad& bounds,std::uint16_t angle);
    void DrawMarker(const float position[3],const TrackMapPoint& center,std::uint16_t angle,
        const TrackMapQuad& bounds,const std::array<std::uint8_t,3>& color,std::int32_t depth,
        const EditorMarkerTexture* texture,float width,float height,std::uint16_t rotation);
    void DrawOverOrUnderLay(const EditorMarkerTexture* texture,std::int32_t depth);
    void MapToScreen(FlailerPoly& polygon,std::array<std::array<float,4>,4>& coordinates) const;
    void ClipPolygon(const FlailerPoly& polygon,const EditorMarkerTexture* texture,std::int32_t depth);
    std::array<float,2> direction_{},velocity_{};
    std::int32_t timestamp_{};
    float mapTransform_[16]{};
    const TrackResourceInfo* track_{};
    CarMovement* car_{};
    CarFramePlayerState* player_{};
    CarMovement* target_{};
    FlailerResources& resources_;
    FlailerRenderManager& renderer_;
    CarCinematicServices& cameras_;
    const RaceParameters& parameters_;
    std::int32_t overlayViewNumber_{};
    const EditorMarkerTexture *mapTexture_{},*marker_{},*orientedMarker_{},*frame_{},*background_{},*startLine_{},*roadBlock_{};
    float upperExtent_{},lowerExtent_{};
    bool valid_{};
    bool playerHeading_{};
    std::int32_t configuredMode_{6},activeMode_{6};
};
}
#endif

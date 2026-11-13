#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include <cstring>

namespace hp2 {
namespace {
// PAL11C450..11C5F8 SkyInitModel. The decoded Scenery owns the same
// placement and model-level objects consumed here; there is no second sky
// transform reconstructed from bounds or from beta asset data.
bool SkyInitModel(EditorMarkerModel& model,float* frame,EditorMarkerResources& resources,
    Scenery& scenery,std::uint32_t key)
{
    auto* placement=sceneryFindInstance(&scenery,key);
    if(!placement) {
        editorMarkerModelDispose(&model);
        rotationMatrixIdentity(frame);
        return false;
    }
    editorMarkerModelInitialize(&model,&resources,key);
    // SkyInitModel11C450..11C5F8 decodes this authored rotation with
    // 1/16384, independently of ordinary scenery's 1/32768 frame helper.
    rotationMatrixIdentity(frame);
    for(std::size_t column=0;column<3;++column)
        for(std::size_t row=0;row<3;++row)
            frame[column*4+row]=static_cast<float>(placement->rotation[column*3+row])*(1.0f/16384.0f);
    for(std::size_t axis=0;axis<3;++axis)frame[12+axis]=placement->position[axis];
    // The source removes all three original scenery-model levels so that
    // the same sky is subsequently submitted only by StuffSky.
    for(std::size_t section=0;section<scenery.loadedSectionCount;++section) {
        auto& group=*scenery.loadedSections[section];
        for(std::size_t i=0;i<group.modelCount;++i)if(group.models[i].levels[0].key==key) {
            for(auto& level:group.models[i].levels)editorMarkerModelDispose(&level);
            return true;
        }
    }
    throw std::runtime_error("Sky placement has no corresponding scenery-model owner");
}
}

// PAL11C5F8..11C6B4. The second source sky deliberately falls back to the
// first model and placement when its own authored placement is absent.
SkyResources::SkyResources(EditorMarkerResources& resources,Scenery& scenery)
{
    try {
        SkyInitModel(models_[0],transforms_[0].data(),resources,scenery,0xbe43edbbu);
        if(!SkyInitModel(models_[1],transforms_[1].data(),resources,scenery,0x72923c81u)) {
            editorMarkerModelInitialize(&models_[1],&resources,0xbe43edbbu);
            transforms_[1]=transforms_[0];
        }
    } catch(...) {
        for(auto& model:models_)editorMarkerModelDispose(&model);
        throw;
    }
}

// PAL11C6B8..11C6F8. Release source model references and reset both frames.
SkyResources::~SkyResources()
{
    for(std::size_t i=0;i<models_.size();++i) {
        editorMarkerModelDispose(&models_[i]);
        rotationMatrixIdentity(transforms_[i].data());
    }
}
const EditorMarkerModel& SkyResources::Model(std::size_t index) const
{
    Require(index<models_.size(),"Sky model index is outside its source bank");return models_[index];
}
const float* SkyResources::Transform(std::size_t index) const
{
    Require(index<transforms_.size(),"Sky transform index is outside its source bank");return transforms_[index].data();
}
void SkyResources::StuffSky(const CarCinematicScreen& view,EcstasyFrame& frame,
    std::int32_t worldFrames) const
{
    // PAL11C6F8..11C764: views1,2,3 select the first bank; all
    // other views select the second. Missing sky models use the existing
    // source eModel-null branch, not synthetic sky polygons. PAL11C6FC
    // tests the same DrawSky control written by Stomper's wireframe group.
    if(!nativeRenderPolicy.drawSky)return;
    const std::size_t index=view.id>=1 && view.id<=3 ? 0u:1u;
    frame.RenderModel(const_cast<EditorMarkerModel&>(models_[index]),transforms_[index].data(),
        view,0,1.0f,worldFrames);
}
}

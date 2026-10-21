// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// TASK-0028/TASK-0033. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): CarRenderInfo::Render() [0x002ACDF0]
// Diagnostic text (not a callable signature): CarRenderInfo::RenderDashboard() [0x002ACE48]
// Port-side values used by the explicitly bounded recovered slices below.
namespace port {
struct CarTextureReplacement { std::uint32_t source_hash{}, target_hash{}; };
struct CarGeometryBinding {
    std::uint32_t object_offset{};
    std::string name;
    std::string fallback;
    std::vector<CarTextureReplacement> texture_replacements;
};
struct CarRenderBindings {
    std::array<CarGeometryBinding, 4> body;
    std::array<CarGeometryBinding, 3> tire_front;
    std::array<CarGeometryBinding, 3> tire_rear;
    std::array<CarGeometryBinding, 2> wheel_blur;
    std::vector<CarGeometryBinding> accessories;
};
struct CarRenderConfig {
    std::string code;
    std::string prefix;
    std::int32_t Field_0x538{};
    std::uint32_t car_index{};
    std::uint32_t material_hash{};
    std::array<std::array<float, 4>, 4> Field_0x120_0x180{};
    std::array<float,4> Field_0x138_0x198{};
};
// Port reader for original aligned chunk0x34600, E-0033-B04 / E-0022-C01.
CarRenderConfig read_car_render_config(const std::vector<std::uint8_t>& global_bundle,
                                      const std::string& code, std::uint8_t skin = 1);
struct CarAttachmentMatrices {
    std::array<std::array<float,16>,4> tire;
    std::array<std::array<float,16>,4> brake;
    std::array<std::array<float,16>,4> blur;
};
// E-0035-W01. Bounded original wheel matrix/opacity slices; port input containers.
// ELF 001C0D78 radians->16-bit angle, 0011B950 Y rotation, 00120EA8 strength.
std::array<float,16> Sub_0011B950(std::uint16_t angle);
// E-0035-R02: original look-at matrix (negative up cross forward), source row vectors.
std::array<float,16> eCreateLookAtMatrix(const std::array<float,3>& eye,const std::array<float,3>& target,
                               const std::array<float,3>& up);
float Sub_00120EA8(float angular_velocity);
CarAttachmentMatrices CalculateWheelMatrices(const CarRenderConfig& config,
                                  const std::array<float,4>& phases,std::uint16_t steering);
struct CarLodSelection {
    int body{};
    int tire{};
    bool visible{};
    bool mirrors{};
    bool license_plates{};
};
struct CarMaterial {
    std::uint32_t hash{};
    std::string name;
    std::array<float,40> parameters{};
    std::array<float,32> blocks{};
};
// E-0033-B13: shared material coefficient projection and original registry lookup.
std::array<float,16> Sub_00116828(const std::array<float,19>& parameters);
CarMaterial read_car_material(const std::vector<std::uint8_t>& global_bundle, std::uint32_t hash);
// E-0033-B14: original frontend mode3, default lighting branch (00198418==0).
// Uses original initialized world vectors; transpose-part rotation, then normalize.
std::array<float,28> Sub_00115978(const std::array<float,16>& source_part_transform);
struct CarUvFactors {
    std::array<float,2> original;
    std::array<float,2> cropped_image;
};
// E-0033-B11, original 0010C158 resize/rebind factors and explicit host crop mapping.
CarUvFactors FixStripSTs(std::uint16_t width, std::uint16_t height,
                          std::uint16_t Field_0x4, std::uint16_t Field_0x6);
// Shared original trig/rotation functions, E-0033-B05; no class ownership.
float Sub_0011B6E0(float angle);
std::array<float,16> Sub_0011B7D8(std::uint16_t angle);
// ELF 0x00107600, E-0033-B03. Shared function; class ownership unknown.
// Explicit inputs replace PS2 view fields +0x8 / camera +0x40 / +0x50.
int GetPixelSize(float radius, float projection_scale,
                 const std::array<float, 3>& position,
                 const std::array<float, 3>& camera_position,
                 const std::array<float, 3>& camera_forward);
}
namespace port::driving {class Memory;}
namespace port::track {struct Scene;struct ShadowInput;struct ShadowPacket;}
namespace hp2::recovered {
class CarRenderInfo {
public:
    // TASK0052 bounded live-car shadow branches, original src/care.cpp Render.
    static port::track::ShadowPacket Render(port::driving::Memory&,const port::track::Scene&,const port::track::ShadowInput&);
    // ELF 0x00120EF8, E-0034-M01. Bounded lighting-context delegation within Render:
    // calls shared 0x00115978 for a static part in default frontend mode 3.
    // Explicit native transform/result are port-side interfaces, not the PS2 ABI.
    // Does not implement the whole render entry or alternate/race lighting.
    static std::array<float,28> Sub_00120EF8(const std::array<float,16>& source_part_transform);
    // ELF 0x0011E860, E-0033-B01/B07/B09/B10. Geometry, paint, window and static lights-off bindings.
    // Static undamaged path; Field_0x538 is the original config integer.
    // No allocation, resolved texture objects, damage, markers or full PS2 layout.
    // Lights-off assumes null car and no original environment override.
    // skin=1 is the explicit viewer default; original default is not established.
    static port::CarRenderBindings Sub_0011E860(const std::string& prefix,
                                               std::int32_t Field_0x538,
                                               std::uint8_t skin = 1);
    // ELF 0x00120EF8, E-0033-B02. Null-car (static viewer) LOD slice only.
    // projected_size is the already-truncated/scaled integer from 0x00107600.
    // Presence entries correspond to body +0x4c,+0x64,+0x7c,+0x94 pointers.
    // Debug overrides and live-car cap are outside this slice.
    static port::CarLodSelection Sub_00120EF8(int projected_size,
                                             const std::array<bool, 4>& presence);
    // E-0033-B05/B06, ELF 0x00120EF8 submission matrices with caller 0x00197CE0.
    // Explicit static pose: zero wheel spin; steering is a caller-selected 16-bit angle.
    // Source row-vector matrices; translation occupies lanes12..14.
    static port::CarAttachmentMatrices Sub_00120EF8(const port::CarRenderConfig& config,
                                                   std::uint16_t steering);
    // ELF00120EF8, E-0033-B02/E-0035-W01/E-0038-W01. Render-side basis
    // composition for raw001C0D78 tire/second-group matrices. Wheels0/3 use
    // the original source-Z half turn (tire/blur) or source-Y reflection(brake).
    // Inputs are source row-vector matrices, before this render-only composition.
    static port::CarAttachmentMatrices Sub_00120EF8(const port::CarAttachmentMatrices& source);
    // Whole-function entry points remain untranslated; overloads above are slices.
    void Sub_0011E860();
    void Sub_00120EF8();
};
} // namespace hp2::recovered

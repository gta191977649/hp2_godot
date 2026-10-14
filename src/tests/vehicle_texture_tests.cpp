#include "port/vehicle/textures.hpp"
#include "port/vehicle/texture_state.hpp"
#include "platform/ps2_asset_root.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class F> void must_fail(F action) {
    bool failed = false;
    try { action(); } catch (const std::runtime_error&) { failed = true; }
    require(failed, "expected an explicit texture failure");
}
struct Source final : port::assets::Source {
    std::string root_ = "synthetic";
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint8_t> shared_bytes;
    unsigned reads = 0;
    const std::string& root() const override { return root_; }
    std::vector<std::uint8_t> read(const std::string& path) override {
        require(path == "CARS/TEXTURES.BIN" || path == "GLOBAL/GLOBALB.BUN", "unexpected texture path");
        ++reads;
        return path == "CARS/TEXTURES.BIN" ? bytes : shared_bytes;
    }
};
void u32(std::vector<std::uint8_t>& b, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) b.at(offset + i) = static_cast<std::uint8_t>(value >> (8*i));
}
std::vector<std::uint8_t> fixture() {
    std::vector<std::uint8_t> b(352);
    u32(b, 0, hp2::assets::kMainChunkId); u32(b, 4, 344);
    u32(b, 8, hp2::assets::kEntryTableChunkId); u32(b, 12, 164);
    const std::size_t e = 16;
    b[e+8] = 'T'; u32(b, e+0x20, 0x12345678);
    b[e+0x24] = 2; b[e+0x26] = 2; b[e+0x28] = 4;
    u32(b, e+0x34, 16); u32(b, e+0x38, 2); u32(b, e+0x3c, 64);
    b[e+0x48] = 1; b[e+0x49] = 1; b[e+0x4a] = 20;
    b[e+0x6f] = 1; b[e+0x70] = 3;
    u32(b, 180, hp2::assets::kDataPoolChunkId); u32(b, 184, 164);
    b[256] = 0x10; b[257] = 0x32;
    for (unsigned i = 0; i < 16; ++i) {
        b[272+4*i] = static_cast<std::uint8_t>(i*10);
        b[272+4*i+3] = 0x80;
    }
    return b;
}
}
int main() {
    auto direct=fixture();direct[16+0x28]=32;direct[16+0x4a]=0;
    u32(direct,16+0x30,0);u32(direct,16+0x38,16);u32(direct,16+0x3c,0);
    for(int i=0;i<16;++i)direct[256+i]=std::uint8_t(i+1);
    auto archive=hp2::assets::Hp2TpkArchive::load(direct);
    const auto direct_image=archive->decode_linear32(archive->headers().front());
    require(direct_image&&direct_image->rgba.front()==1&&direct_image->rgba[8]==9&&direct_image->rgba[3]==4,"PSMCT32 row/byte/alpha ordering");
    auto bad_header=archive->headers().front();bad_header.image_size=15;
    require(!archive->decode_linear32(bad_header),"Truncated PSMCT32 must reject");
    bad_header=archive->headers().front();bad_header.is_swizzled=true;
    require(!archive->decode_linear32(bad_header),"Unknown swizzled PSMCT32 must reject");
    hp2::assets::TpkTextureHeader state_header;
    state_header.Field_0x6f = 1; state_header.Field_0x70 = 3;
    state_header.Field_0x50 = 1;
    state_header.alpha_bits = 0x0a;
    auto state = port::vehicle::texture_state_00108CC0(state_header);
    require(state.blend == port::vehicle::BlendMode::Copy && !state.deferred && state.texture_alpha,
            "original copy texture state mismatch");
    require(state.tex1_register == 0xe0 && state.magnification_linear && !state.minification_linear &&
            !state.repeat_u && !state.repeat_v, "original base texture filtering/clamp mismatch");
    state_header.Field_0x4c = 3;
    const auto repeat_state = port::vehicle::texture_state_00108CC0(state_header);
    require(repeat_state.repeat_u && repeat_state.repeat_v, "raw CLAMP input bits were not preserved");
    state_header.Field_0x4c = 0;
    const auto opaque_test = port::vehicle::test_state_00105AC0(state_header);
    require(opaque_test.test_register == 0x5030d && opaque_test.write_depth &&
            !opaque_test.accepts_alpha(48) && opaque_test.accepts_alpha(49),
            "original normal-target TEST must reject alpha <= 48 and write depth");
    require(!port::vehicle::test_state_00105AC0(state_header, true).write_depth,
            "original second context must mask depth writes");
    state_header.Field_0x74 = 1;
    const auto selected_test = port::vehicle::test_state_00105AC0(state_header);
    require(selected_test.test_register == 0x5000b && selected_test.write_depth && selected_test.accepts_alpha(0),
            "raw texture byte +0x74 must select TEST independently of depth-write state");
    require(!port::vehicle::test_state_00105AC0(state_header, false, false).write_depth,
            "original 001033C0 global gate must suppress queue depth-write selection");
    state_header.Field_0x50 = 0;
    require(!port::vehicle::test_state_00105AC0(state_header).write_depth,
            "raw texture byte +0x50 zero must independently mask depth writes");
    state_header.Field_0x74 = 0;
    require(port::vehicle::test_state_00105AC0(state_header).test_register == 0x5030d &&
            !port::vehicle::test_state_00105AC0(state_header).write_depth,
            "State index zero must preserve the original forced depth mask");
    state_header.Field_0x50 = 1;
    state_header.Field_0x74 = 0; state_header.is_any_semitransparency = 1;
    require(port::vehicle::test_state_00105AC0(state_header).test_register == 0x5000b,
            "texture +0x4F must independently select the same TEST state");
    state_header.is_any_semitransparency = 0;
    state_header.alpha_bits = 0x44; state_header.Field_0x4d = 1;
    state = port::vehicle::texture_state_00108CC0(state_header);
    require(state.blend == port::vehicle::BlendMode::SourceOver && state.deferred && state.alpha_register == 0x44,
            "original window texture state mismatch");
    require(port::vehicle::blend_channel_00108CC0(state.blend, 100, 200, 64) == 150,
            "source-over must use GS alpha denominator 128");
    require(port::vehicle::blend_channel_00108CC0(state.blend, 100, 200, 0) == 200 &&
            port::vehicle::blend_channel_00108CC0(state.blend, 100, 200, 128) == 100,
            "source-over zero/full alpha limits mismatch");
    require(port::vehicle::blend_channel_00108CC0(state.blend, 0, 1, 1) == 0,
            "negative blend product must follow arithmetic-shift rounding");
    require(port::vehicle::blend_channel_00108CC0(state.blend, 0, 128, 255) == -127,
            "GS alpha above 128 must not silently clamp to host source-over range");
    state_header.texture_fx = 1;
    require(!hp2::guard([&]{port::vehicle::texture_state_00108CC0(state_header);}).empty(),
            "unsupported TFX must fail explicitly");
    state_header.texture_fx = 0; state_header.alpha_bits = 0x48;
    require(!hp2::guard([&]{port::vehicle::texture_state_00108CC0(state_header);}).empty(),
            "unsupported additive ALPHA must fail explicitly");
    state_header.alpha_bits = 0x0a; state_header.Field_0x6e = 2;
    require(!hp2::guard([&]{port::vehicle::texture_state_00108CC0(state_header);}).empty(),
            "unsupported original mip levels must fail explicitly");

    auto alpha_fixture = fixture();
    const std::uint8_t alpha_values[]{0,64,128,255};
    for (unsigned i = 0; i < 4; ++i) alpha_fixture[272+4*i+3] = alpha_values[i];
    auto alpha_archive = hp2::assets::Hp2TpkArchive::load(std::move(alpha_fixture));
    require(alpha_archive.has_value(), "alpha fixture archive invalid");
    auto frontend_alpha = alpha_archive->decode(alpha_archive->headers()[0], false);
    auto gs_alpha = alpha_archive->decode(alpha_archive->headers()[0], false, true);
    require(frontend_alpha && gs_alpha, "alpha fixture decode failed");
    const std::uint8_t expected_raw[]{128,255,0,64}, expected_frontend[]{255,255,0,127};
    for (unsigned i = 0; i < 4; ++i) {
        require(gs_alpha->rgba[4*i+3] == expected_raw[i], "raw GS palette alpha changed");
        require(frontend_alpha->rgba[4*i+3] == expected_frontend[i], "default frontend alpha expansion changed");
        for (unsigned c = 0; c < 3; ++c)
            require(frontend_alpha->rgba[4*i+c] == gs_alpha->rgba[4*i+c], "alpha option changed RGB decode");
    }
    Source source;
    source.bytes = fixture();
    source.bytes[16+0x74] = 0x81;
    source.bytes[16+0x4d] = 0x81;
    source.bytes[16+0x50] = 0x81;
    source.shared_bytes = fixture(); u32(source.shared_bytes, 16+0x20, 0x87654321);
    port::vehicle::Textures textures(source);
    require(textures.load({}).empty() && source.reads == 0, "empty requests must not read assets");
    const auto decoded = textures.load({0x12345678, 0x12345678});
    require(decoded.size() == 1 && source.reads == 2, "hash deduplication or lazy shared load failed");
    require(decoded[0].archive_path == "CARS/TEXTURES.BIN", "car archive provenance missing");
    require(decoded[0].header.Field_0x74 == 0x81, "raw texture byte +0x74 was lost or reinterpreted");
    require(decoded[0].header.Field_0x4d == 0x81, "raw texture partition byte +0x4d was lost or reinterpreted");
    require(decoded[0].header.Field_0x50 == 0x81, "raw texture state-index byte +0x50 was lost or reinterpreted");
    const auto shared = textures.load({0x87654321});
    require(shared.size() == 1 && shared[0].archive_path == "GLOBAL/GLOBALB.BUN", "shared archive hash lookup failed");
    require(shared[0].header.Field_0x74 == 0, "raw texture byte +0x74 zero was not preserved");
    require(shared[0].header.Field_0x4d == 0, "raw texture partition byte +0x4d zero was not preserved");
    require(shared[0].header.Field_0x50 == 0, "raw texture state-index byte +0x50 zero was not preserved");
    require(decoded[0].bitmap.width == 2 && decoded[0].bitmap.height == 2, "wrong dimensions");
    // Source nibbles 0,1 are row zero; 2,3 are row one. Original VU T=0
    // must sample the former, independent of the legacy decoder's row flip.
    require(decoded[0].bitmap.rgba[0] == 0 && decoded[0].bitmap.rgba[8] == 20,
            "GS T=0 must address source row zero, not the vertically reversed row");
    require(decoded[0].bitmap.rgba[3] == 128, "vehicle texture service must preserve raw GS alpha");
    must_fail([&]{ textures.load({0}); });
    require(source.reads == 2, "archive unexpectedly reread");
    textures.reset(); source.shared_bytes = fixture();
    must_fail([&]{ textures.load({0x12345678}); });
    u32(source.shared_bytes, 16+0x20, 0x87654321);
    textures.reset();
    source.bytes.clear();
    must_fail([&]{ textures.load({0x12345678}); });
    source.bytes = fixture(); u32(source.bytes, 16+0x3c, 0);
    textures.reset(); must_fail([&]{ textures.load({0x12345678}); });
    source.bytes = fixture(); source.bytes[16+0x4a] = 19;
    textures.reset(); must_fail([&]{ textures.load({0x12345678}); });
    source.bytes = fixture(); source.bytes[16+0x48] = 255;
    textures.reset(); must_fail([&]{ textures.load({0x12345678}); });
    source.bytes = fixture(); u32(source.bytes, 16+0x38, 1);
    textures.reset(); must_fail([&]{ textures.load({0x12345678}); });
    source.bytes = fixture(); u32(source.bytes, 16+0x34, 1000000);
    textures.reset(); must_fail([&]{ textures.load({0x12345678}); });

    const auto root = hp2::platform::resolve_zzdata_root(std::filesystem::current_path().string());
    if (!root) {
        const char* environment = std::getenv(hp2::platform::kAssetRootEnvVar);
        require(!(environment && *environment) &&
                !hp2::platform::resolve_local_config_value(std::filesystem::current_path().string(), "root"),
                "original asset root is configured but invalid; real texture checks cannot run");
        std::puts("SKIPPED real car texture checks: original assets are not configured; synthetic checks passed");
        return 0;
    }
    port::assets::DirectorySource real(*root);
    port::vehicle::Textures actual(real);
    const auto env=actual.frontend_environment();
    require(env.header.name=="CARSELECTENVMAP"&&env.bitmap.width==256&&env.bitmap.height==256&&env.bitmap.rgba.size()==256*256*4,"Original frontend reflection texture identity");
    // Original archive entries for the three acceptance vehicle families, E-0033-T02.
    const auto samples = actual.load({0x1568fc4b, 0x1f51c325, 0x0bab0358});
    require(samples.size() == 3, "configured original car texture sample count mismatch");
    require(samples[0].header.name == "911TURBO_SKIN1", "wrong player-car texture binding");
    require(samples[1].header.name == "BMWM5COP_BRAKELIGHT", "wrong police-car texture binding");
    require(samples[2].header.name == "4DR_SEDAN_SKIN1", "wrong traffic-car texture binding");
    for (const auto& sample : samples) {
        require(sample.bitmap.rgba.size() == static_cast<std::size_t>(sample.header.width)*sample.header.height*4,
                "configured original texture decode size mismatch");
        require(std::any_of(sample.bitmap.rgba.begin(), sample.bitmap.rgba.end(), [](auto x){return x != 0;}),
                "configured original texture decoded to zero");
    }
    must_fail([&]{ actual.load({0x310960d7}); }); // DASHWINDOW: unsupported direct-color entry.
    const auto underbody = actual.load({0xbd1f0e0a});
    require(underbody.size() == 1 && underbody[0].header.name == "CARBOTTOM" &&
            underbody[0].archive_path == "GLOBAL/GLOBALB.BUN", "original shared CARBOTTOM binding failed");
    require(port::vehicle::texture_state_00108CC0(underbody[0].header).blend == port::vehicle::BlendMode::Copy,
            "original CARBOTTOM must select the recorded copy ALPHA equation");
    require(port::vehicle::test_state_00105AC0(underbody[0].header).write_depth &&
            port::vehicle::test_state_00105AC0(samples[0].header).write_depth,
            "Original underbody and paint textures must select depth writes in the normal enabled route");
    const auto window = actual.load({0x834b8017});
    require(window.size() == 1 && window[0].header.name == "WINDOW" &&
            port::vehicle::texture_state_00108CC0(window[0].header).blend == port::vehicle::BlendMode::SourceOver &&
            port::vehicle::texture_state_00108CC0(window[0].header).deferred,
            "original WINDOW must select the recorded deferred source-over state");
    require(!port::vehicle::test_state_00105AC0(window[0].header).write_depth,
            "Original WINDOW must retain masked depth writes");
    must_fail([&]{ actual.load({0}); });
    std::puts("PASSED vehicle texture adapter: synthetic failures and three configured vehicle families");
}

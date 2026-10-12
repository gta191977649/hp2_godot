#include "game_core/assets/hp2_tpk_texture.hpp"
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>

// Cross-validates the C++ port of the HP2 TPK texture decoder against the Python reference
// (tools/asset_extractor/hp2_tpk_texture.py) by decoding the same real texture from the user's
// configured ZZDATA copy and diffing the raw RGBA bytes. This test is skipped (not failed) when
// no ZZDATA root is configured, since the original assets are not part of this repository.
using namespace hp2::assets;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::uint8_t> read_file(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}
} // namespace

int main() {
    const char* zzdata_root = std::getenv("HP2_PS2_ASSET_ROOT");
    const char* reference_path = "local/vipergts_tach_python.rgba";
    auto reference = read_file(reference_path);
    if (!zzdata_root || reference.empty()) {
        std::printf("SKIPPED: HP2_PS2_ASSET_ROOT not set or %s missing (expected outside a "
                     "configured dev machine); see docs/ASSET_PIPELINE.md\n", reference_path);
        return 0;
    }

    std::string path = std::string(zzdata_root) + "/GLOBAL/HUDTEX.BIN";
    auto buffer = read_file(path.c_str());
    require(!buffer.empty(), "could not read GLOBAL/HUDTEX.BIN from HP2_PS2_ASSET_ROOT");

    auto archive = Hp2TpkArchive::load(std::move(buffer));
    require(archive.has_value(), "failed to parse HUDTEX.BIN as an HP2 TPK archive");
    require(archive->headers().size() == 89, "expected 89 texture headers in HUDTEX.BIN");

    auto decoded = archive->decode_by_name("VIPERGTS_TACH", /*flip_vertical=*/true);
    require(decoded.has_value(), "failed to decode VIPERGTS_TACH");
    require(decoded->width == 128 && decoded->height == 128, "unexpected VIPERGTS_TACH dimensions");
    require(decoded->rgba.size() == reference.size(), "C++ and Python RGBA byte counts differ");
    require(decoded->rgba == reference, "C++ decode does not byte-for-byte match the Python reference");

    std::printf("PASSED: C++ Hp2TpkArchive decode of VIPERGTS_TACH matches the Python reference "
                 "byte-for-byte (%zu bytes)\n", decoded->rgba.size());
    return 0;
}

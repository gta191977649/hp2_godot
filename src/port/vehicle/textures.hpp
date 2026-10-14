#pragma once
#include "port/assets/catalog.hpp"
#include <array>
#include <map>
#include <memory>

// Port-side resource service, not an original PS2 class or ABI. E-0033-T01/T02.
namespace port::vehicle {
struct Texture {
    hp2::assets::TpkTextureHeader header;
    // RGB bytes and original GS alpha bytes, unity alpha=128 (not frontend-expanded).
    hp2::assets::DecodedTexture bitmap;
    std::string archive_path;
};

struct TextureCache {
    std::array<std::optional<hp2::assets::Hp2TpkArchive>,2> archives;
    std::map<std::uint32_t,std::pair<std::size_t,hp2::assets::TpkTextureHeader>> index;
    std::map<std::uint32_t,std::pair<Texture,std::uint64_t>> decoded;
    std::size_t bytes=0;std::uint64_t clock=0,hits=0,decodes=0;double decode_ms=0;
    std::optional<Texture> frontend_environment;
};
class Textures {
public:
    explicit Textures(assets::Source& source,std::shared_ptr<TextureCache> cache=std::make_shared<TextureCache>()) : source_(source),cache_(std::move(cache)) {}
    // Returns each requested hash once, in first-request order. No texture substitution.
    // Retains raw material metadata without inferring PS2 blending/culling behavior.
    std::vector<Texture> load(const std::vector<std::uint32_t>& hashes);
    Texture frontend_environment();
    void reset() { *cache_=TextureCache{}; }
private:
    assets::Source& source_;
    std::shared_ptr<TextureCache> cache_;
};
} // namespace port::vehicle

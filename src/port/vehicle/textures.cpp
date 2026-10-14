#include "textures.hpp"
#include <set>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace port::vehicle {
Texture Textures::frontend_environment(){
    if(cache_->frontend_environment)return *cache_->frontend_environment;
    auto archive=hp2::assets::Hp2TpkArchive::load(source_.read("FRONTEND/FRONTB.BUN"));
    if(!archive)throw std::runtime_error("Original frontend reflection archive missing");
    for(const auto& header:archive->headers())if(header.tex_hash==0x4ecc769c){
        if(header.width!=256||header.height!=256)throw std::runtime_error("Original CARSELECTENVMAP must match the recovered 256x256 context");
        const auto bitmap=archive->decode_linear32(header);
        if(!bitmap)throw std::runtime_error("Unsupported original CARSELECTENVMAP PSMCT32 transfer");
        cache_->frontend_environment=Texture{header,*bitmap,"FRONTEND/FRONTB.BUN"};
        return *cache_->frontend_environment;
    }
    throw std::runtime_error("Original CARSELECTENVMAP hash4ECC769C absent");
}
namespace {
[[noreturn]] void fail(std::uint32_t hash, const char* reason) {
    std::ostringstream message;
    message << "Original vehicle texture 0x" << std::hex << hash << ": " << reason;
    throw std::runtime_error(message.str());
}
}

std::vector<Texture> Textures::load(const std::vector<std::uint32_t>& hashes) {
    if (hashes.empty()) return {};
    auto& archives_=cache_->archives;
    // E-0033-T04: vehicles reference both the car pack and original shared GLOBAL pack.
    // 001ABF48 searches loaded packs with move-to-front ordering, not a fixed priority.
    // This bounded viewer service requires a unique hash across these two archives;
    // it does not invent priority, substitute a default hash or search unrelated files.
    static constexpr std::array<const char*, 2> paths{{"CARS/TEXTURES.BIN", "GLOBAL/GLOBALB.BUN"}};
    for (std::size_t i = 0; i < archives_.size(); ++i) {
        if (archives_[i]) continue;
        archives_[i] = hp2::assets::Hp2TpkArchive::load(source_.read(paths[i]));
        if (!archives_[i]) throw std::runtime_error(std::string(paths[i]) + " is not a supported TPK archive");
    }
    if(cache_->index.empty())for(std::size_t i=0;i<archives_.size();++i)for(const auto& header:archives_[i]->headers()){
        const auto entry=cache_->index.emplace(header.tex_hash,std::make_pair(i,header));if(!entry.second)entry.first->second.first=std::size_t(-1);
    }
    std::vector<Texture> result;
    std::set<std::uint32_t> requested;
    for (auto hash : hashes) {
        if (!requested.insert(hash).second) continue;
        auto cached=cache_->decoded.find(hash);
        if(cached!=cache_->decoded.end()){++cache_->hits;cached->second.second=++cache_->clock;result.push_back(cached->second.first);continue;}
        const auto found=cache_->index.find(hash);if(found==cache_->index.end())fail(hash,"hash is absent");
        const auto pack=found->second.first;if(pack==std::size_t(-1))fail(hash,"ambiguous duplicate hash across original shared packs");const auto& header=found->second.second;
        // Restrict this adapter to the existing indexed decoder's supported inputs.
        // In particular DASHWINDOW is 32-bit with no CLUT; never invent its decode.
        if (!((header.bit_depth == 4 && header.pixel_storage_mode == 20 && header.palette_size == 64) ||
              (header.bit_depth == 8 && header.pixel_storage_mode == 19 && header.palette_size == 1024)))
            fail(hash, "unsupported pixel/CLUT format");
        if (header.shift_width >= 16 || header.shift_height >= 16 ||
            !header.width || !header.height ||
            header.width > (1u << header.shift_width) ||
            header.height > (1u << header.shift_height))
            fail(hash, "invalid dimensions or buffer shifts");
        const auto minimum_bytes = (static_cast<std::size_t>(header.width) * header.height *
                                    header.bit_depth + 7) / 8;
        if (header.image_size < 0 || static_cast<std::size_t>(header.image_size) < minimum_bytes)
            fail(hash, "truncated pixel data");
        // E-0034-U01: the legacy indexed decoder emits bottom-first rows internally.
        // Restore GS row-zero order for the original VU ST coordinates. Do not invert
        // vertex T or change the frontend decoder's existing public convention.
        const auto decode_start=std::chrono::steady_clock::now();
        auto decoded = archives_[pack]->decode(header, true, true);
        cache_->decode_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-decode_start).count();
        if (!decoded) fail(hash, "pixel decode failed");
        Texture texture{header,std::move(*decoded),paths[pack]};++cache_->decodes;
        cache_->bytes+=texture.bitmap.rgba.size();cache_->decoded.emplace(hash,std::make_pair(texture,++cache_->clock));
        while(cache_->bytes>128u*1024u*1024u&&cache_->decoded.size()>1){
            auto oldest=std::min_element(cache_->decoded.begin(),cache_->decoded.end(),[](const auto& a,const auto& b){return a.second.second<b.second.second;});
            cache_->bytes-=oldest->second.first.bitmap.rgba.size();cache_->decoded.erase(oldest);
        }
        result.push_back(std::move(texture));
    }
    return result;
}
} // namespace port::vehicle

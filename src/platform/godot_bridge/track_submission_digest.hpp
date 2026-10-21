#pragma once
// Port-only diagnostic of submitted planner/material command state. This is not
// a PS2 packet signature, equivalence proof, source hash or collision-free ID.
#include "game_core/port/track/runtime.hpp"
#include <cstdint>
#include <cstring>
#include <string>
namespace port::godot_bridge {
namespace track_submission_digest_detail {
struct Fnv {
    std::uint64_t value=14695981039346656037ull;
    void integer(std::uint64_t bits,unsigned bytes){for(unsigned i=0;i<bytes;++i){value^=std::uint8_t(bits>>(8*i));value*=1099511628211ull;}}
    void flag(bool bit){integer(bit?1:0,1);}
    void scalar(float number){static_assert(sizeof(float)==sizeof(std::uint32_t),"Diagnostic requires32bit float");std::uint32_t bits;std::memcpy(&bits,&number,sizeof(bits));integer(bits,4);}
};
}
inline std::string track_submission_digest(const port::track::Frame& frame){
    track_submission_digest_detail::Fnv hash;
    hash.integer(1,4); // Host diagnostic schema, fixed-width little-endian fields.
    hash.integer(std::uint32_t(frame.zone),4);
    hash.integer(frame.sections.size(),8);for(int section:frame.sections)hash.integer(std::uint32_t(section),4);
    hash.integer(frame.commands.size(),8);
    for(const auto& command:frame.commands){
        hash.integer(command.instance,8);hash.integer(command.solid,8);hash.integer(command.surface,8);
        hash.integer(command.lod,4);hash.integer(command.visibility,4);hash.flag(command.identity);
        for(float value:command.matrix)hash.scalar(value);
        for(float value:command.light_context)hash.scalar(value);
        hash.flag(command.has_light_context);hash.scalar(command.depth);hash.integer(command.original_order,8);
        const auto& effect=command.effect;const auto& texture=effect.texture;const auto& test=effect.test;
        hash.integer(effect.flags,2);hash.integer(effect.matrix_pack,1);hash.integer(std::uint8_t(effect.rendering_order),1);
        hash.flag(effect.alpha_pass);hash.flag(effect.rewrite_alpha);hash.flag(effect.second_pass_320);hash.flag(effect.environment_pass);
        hash.integer(std::uint32_t(texture.blend),4);hash.flag(texture.deferred);hash.flag(texture.texture_alpha);
        hash.integer(texture.alpha_register,8);hash.integer(texture.tex1_register,8);
        hash.flag(texture.magnification_linear);hash.flag(texture.minification_linear);hash.flag(texture.repeat_u);hash.flag(texture.repeat_v);
        hash.integer(texture.maximum_mip_level,1);hash.flag(texture.requires_lod_projection);hash.flag(texture.additive);
        hash.integer(test.test_register,4);hash.flag(test.write_depth);hash.integer(test.alpha_test,1);hash.integer(test.alpha_reference,1);hash.integer(test.depth_test,1);
    }
    const char* digits="0123456789abcdef";std::string result(16,'0');
    for(unsigned i=0;i<16;++i)result[15-i]=digits[(hash.value>>(i*4))&15];return result;
}
}

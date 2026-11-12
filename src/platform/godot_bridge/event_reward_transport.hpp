#pragma once
// TASK-0112 port-only physical reward payload transport; no original ABI.
#include "port/runtime/event_reward_codec.hpp"
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <cstring>
namespace port::godot_bridge {
inline godot::PackedByteArray reward_transfer(const ::port::runtime::EventRewardTransfer& value){
 const auto bytes=::port::runtime::encode_debug_reward_payload(value);godot::PackedByteArray out;out.resize(bytes.size());std::memcpy(out.ptrw(),bytes.data(),bytes.size());return out;
}
inline ::port::runtime::EventRewardTransfer reward_transfer(const godot::PackedByteArray& value){
 std::vector<std::uint8_t> bytes;if(value.size())bytes.assign(value.ptr(),value.ptr()+value.size());return ::port::runtime::decode_debug_reward_payload(bytes);
}
}

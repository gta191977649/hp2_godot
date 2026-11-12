// Port ownership of complete authored records. Required Debug/Retail consumed
// field correspondence is reviewed in reverse/task0099/player-construction.md.
#include "player_construction.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <functional>
#include <stdexcept>
namespace port::race {
void NisInfoResources::load_bundle(const std::vector<std::uint8_t>& bytes){
 const auto nodes=hp2::assets::parse_chunks_top(bytes);std::function<void(const std::vector<hp2::assets::Chunk>&)> visit=[&](const auto& children){for(const auto& node:children){if(node.tag!=0x37010){visit(node.children);continue;}if(!chunks_.empty())throw std::logic_error("Port NIS source owner already has an active table");if(node.size%0xa0)throw std::runtime_error("Authored NIS table has a partial A0-byte record");const auto chunk=memory_.allocate(std::size_t(node.size)+8);chunks_.push_back(chunk);for(std::size_t i=0;i<std::size_t(node.size)+8;++i)memory_.byte(chunk+std::uint32_t(i),bytes.at(node.header_offset+i));if(!hp2::debug::LoaderNisInfo(memory_,chunk))throw std::logic_error("Original NIS loader rejected its actual source chunk");}};visit(nodes);
}
void NisInfoResources::close(){while(!chunks_.empty()){const auto chunk=chunks_.back();hp2::debug::UnloaderNisInfo(memory_,chunk);memory_.release(chunk);chunks_.pop_back();}}
}

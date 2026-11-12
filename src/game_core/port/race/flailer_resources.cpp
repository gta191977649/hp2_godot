// Port source-byte ownership; original mutations/lookup remain in flailerdb.cpp.
#include "flailer_resources.hpp"
#include "game_core/indep/inc/Font.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <functional>
#include <stdexcept>
namespace port::race {
void FlailerResources::load_bundle(const std::vector<std::uint8_t>& bytes){const auto tree=hp2::assets::parse_chunks_top(bytes);std::function<void(const std::vector<hp2::assets::Chunk>&)> visit=[&](const auto& nodes){for(const auto& node:nodes){if(node.tag==0x80010200){bool project=false;for(const auto& child:node.children){if(child.tag==0x10213){if(child.size<20)throw std::runtime_error("Truncated source Flailer project");project=true;}else if(child.tag==0x80010201){if(!project)throw std::runtime_error("Source Flailer screen precedes project");bool screen=false;for(const auto& object:child.children){if(object.tag==0x10203){if(object.size<40)throw std::runtime_error("Truncated source Flailer screen");screen=true;}else if(!screen||object.size<80)throw std::runtime_error("Invalid source Flailer object sequence/extent");}}}const auto base=memory_.allocate(std::size_t(node.size)+8);chunks_.push_back(base);for(std::size_t i=0;i<std::size_t(node.size)+8;++i)memory_.byte(base+std::uint32_t(i),bytes.at(node.header_offset+i));if(!hp2::debug::LoaderFlailerChunk(memory_,base))throw std::logic_error("Original Flailer loader rejected its source chunk");}else if(node.tag==0x15010){if(node.size<20)throw std::runtime_error("Truncated source font header");const auto base=memory_.allocate(std::size_t(node.size)+8);chunks_.push_back(base);for(std::size_t i=0;i<std::size_t(node.size)+8;++i)memory_.byte(base+std::uint32_t(i),bytes.at(node.header_offset+i));if(!hp2::debug::LoaderFontInfo(memory_,base))throw std::logic_error("Original font loader rejected its source chunk");}else visit(node.children);}};visit(tree);}
void FlailerResources::close(){
 // Original font unload finds the FIRST registered hash, even when bundles
 // contain duplicate fonts. Retain every raw borrower target until all source
 // unloaders have completed, then release host buffers. E-0112-H01.
 for(auto it=chunks_.rbegin();it!=chunks_.rend();++it){const auto chunk=*it;if(memory_.word(chunk)==0x15010)hp2::debug::UnloaderFontInfo(memory_,chunk);else hp2::debug::UnloaderFlailerChunk(memory_,chunk);}
 while(!chunks_.empty()){memory_.release(chunks_.back());chunks_.pop_back();}
}
}

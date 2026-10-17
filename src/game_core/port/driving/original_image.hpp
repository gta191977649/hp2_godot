#pragma once
// Port-side player-owned ELF data loader. Does not execute original machine code.
#include "memory.hpp"
#include <filesystem>
#include <string>
namespace port::driving {
std::string image_sha256(const std::vector<std::uint8_t>&);
Memory read_original_image(const std::filesystem::path&);
std::filesystem::path original_image_path(const std::filesystem::path& project_root);
}

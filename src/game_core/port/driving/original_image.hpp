#pragma once
// Port-side player-owned ELF data loader. Does not execute original machine code.
#include "memory.hpp"
#include <filesystem>
#include <string>
namespace port::driving {
std::string image_sha256(const std::vector<std::uint8_t>&);
Memory read_original_image(const std::filesystem::path&);
std::filesystem::path original_image_path(const std::filesystem::path& project_root);
// TASK-0044: driving uses this separate identity/configuration. Never falls back to Retail.
inline constexpr const char* debug_elf_sha256="815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b";
Memory read_debug_image(const std::filesystem::path&);
std::filesystem::path debug_image_path(const std::filesystem::path& project_root);
}

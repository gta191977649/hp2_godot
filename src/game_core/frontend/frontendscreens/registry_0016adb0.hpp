#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace port::frontend {

// Port-side immutable projection of ELF registration operands, not the original
// descriptor layout or its runtime linked list. Original source TU is unknown.
// SLUS_203.62, sha256:7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Evidence: reverse/functions/<binary-id>/frontend/0016adb0.md, E-0015-REG-01.
struct Registration {
    std::string_view class_name;
    std::uint32_t document_hash;
    std::uint32_t handler_address;
};

const std::array<Registration, 37>& registrations() noexcept;
const Registration* find_registration(std::string_view class_name) noexcept;
// Multiple classes may share a document. First row is for diagnostics only;
// a document hash cannot recover the originating class or handler uniquely.
const Registration* find_registration_for_document_hash(std::uint32_t hash) noexcept;

} // namespace port::frontend

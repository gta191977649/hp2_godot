#pragma once

#include <stdexcept>
#include <string>

namespace hp2 {

class NotImplemented final : public std::logic_error {
public:
    NotImplemented(const char* address, const char* reason)
        : std::logic_error(std::string(address) + ": " + reason) {}
};

}

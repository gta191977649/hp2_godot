#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace {
namespace fs = std::filesystem;

fs::path executable_path()
{
#ifdef __APPLE__
    uint32_t bytes = 1024;
    std::string path(bytes, '\0');
    if (_NSGetExecutablePath(path.data(), &bytes) == 0) return fs::canonical(path.c_str());
    path.resize(bytes);
    if (_NSGetExecutablePath(path.data(), &bytes) != 0)
        throw std::runtime_error("Cannot locate HP2.app.");
    return fs::canonical(path.c_str());
#else
    return fs::canonical("/proc/self/exe");
#endif
}

std::string saved_renderer(const fs::path& config)
{
#ifdef __APPLE__
    const std::string_view key = "rendering_device/driver.macos";
    const std::string defaultDriver = "metal";
#else
    const std::string_view key = "rendering_device/driver";
    const std::string defaultDriver = "vulkan";
#endif
    std::ifstream input(config);
    bool rendering = false;
    for (std::string line; std::getline(input, line);) {
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos) continue;
        const auto last = line.find_last_not_of(" \t\r");
        std::string_view field(line.data() + first, last - first + 1);
        if (field.front() == '[') rendering = field == "[rendering]";
        if (!rendering || field.substr(0, key.size()) != key) continue;
        field.remove_prefix(key.size());
        const auto separator = field.find_first_not_of(" \t");
        if (separator == field.npos || field[separator] != '=') continue;
        field.remove_prefix(separator + 1);
        const auto value = field.find_first_not_of(" \t");
        if (value == field.npos) continue;
        field.remove_prefix(value);
        if (field == "\"vulkan\"") return "vulkan";
#ifdef __APPLE__
        if (field == "\"metal\"") return "metal";
#endif
    }
    return defaultDriver;
}

void set_environment(const char* key, const fs::path& value)
{
    if (setenv(key, value.c_str(), 1) != 0)
        throw std::runtime_error(std::string("Cannot set ") + key);
}
}

int main(int argc, char** argv)
{
    try {
        const auto bin = executable_path().parent_path();
#ifdef __APPLE__
        const auto root = bin.parent_path().parent_path().parent_path();
        const auto runtime = bin / "HP2Runtime";
        set_environment("HP2_LAUNCH_ROOT", root);
#else
        const auto root = bin;
        const auto runtime = bin / "HP2Runtime.x86_64";
        const auto cache = root / ".hp2-runtime";
        fs::create_directories(cache);
        set_environment("XDG_DATA_HOME", cache / "data");
        set_environment("XDG_CONFIG_HOME", cache / "config");
        set_environment("XDG_CACHE_HOME", cache / "cache");
        const auto* old = getenv("LD_LIBRARY_PATH");
        set_environment("LD_LIBRARY_PATH", (root / "bin").string() + (old && *old ? ":" + std::string(old) : ""));
#endif
        fs::current_path(root);
        const auto logDirectory = root / ".hp2-runtime";
        fs::create_directories(logDirectory);
        bool explicitDriver = false, explicitLog = false, headless = false;
        for (int i = 1; i < argc && std::string_view(argv[i]) != "--"; ++i) {
            explicitDriver |= std::string_view(argv[i]) == "--rendering-driver";
            explicitLog |= std::string_view(argv[i]) == "--log-file";
            headless |= std::string_view(argv[i]) == "--headless";
        }
        // Official templates discover the matching PCK beside the runtime
        // (or in macOS Resources); they disable --main-pack path overrides.
        std::vector<std::string> arguments{runtime.string()};
        if (!explicitDriver && !headless) {
            arguments.emplace_back("--rendering-driver");
            arguments.emplace_back(saved_renderer(root / "launcher.cfg"));
        }
        if (!explicitLog) {
            arguments.emplace_back("--log-file");
            arguments.emplace_back((logDirectory / "hp2.log").string());
        }
        for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
        std::vector<char*> command;
        for (auto& arg : arguments) command.push_back(arg.data());
        command.push_back(nullptr);
        execv(runtime.c_str(), command.data());
        perror("Cannot launch HP2Runtime");
    } catch (const std::exception& error) {
        std::cerr << "HP2: " << error.what() << '\n';
    }
    return 1;
}

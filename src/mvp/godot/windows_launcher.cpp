#include <windows.h>

#include <filesystem>
#include <string>

namespace {
int launch_error(const wchar_t* message)
{
    MessageBoxW(nullptr, message, L"HP2", MB_OK | MB_ICONERROR);
    return 1;
}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR arguments, int)
{
    std::wstring executable(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    if (!length || length >= executable.size()) return launch_error(L"Cannot find the HP2 installation directory.");
    executable.resize(length);
    const auto root = std::filesystem::path(executable).parent_path();
    const auto runtime = root / L"HP2Runtime.exe";
    const auto cache = root / L".hp2-runtime";
    if (!CreateDirectoryW(cache.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
        return launch_error(L"Cannot create the runtime cache beside HP2.exe.");
    if (!SetEnvironmentVariableW(L"APPDATA", cache.c_str()) ||
        !SetEnvironmentVariableW(L"LOCALAPPDATA", cache.c_str()))
        return launch_error(L"Cannot set the portable runtime cache directory.");

    std::wstring command = L"\"" + runtime.wstring() + L"\"";
    if (*arguments) {
        command += L' ';
        command += arguments;
    }
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    // The engine reads its project override before creating the renderer.
    // Starting it in the package directory makes launcher.cfg the override.
    if (!CreateProcessW(runtime.c_str(), command.data(), nullptr, nullptr, TRUE, 0, nullptr,
            root.c_str(), &startup, &process))
        return launch_error(L"Cannot start HP2Runtime.exe.");
    CloseHandle(process.hThread);
    DWORD result = 1;
    if (WaitForSingleObject(process.hProcess, INFINITE) == WAIT_OBJECT_0)
        GetExitCodeProcess(process.hProcess, &result);
    CloseHandle(process.hProcess);
    return static_cast<int>(result);
}
